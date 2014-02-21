/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* June 2010.								*/
/************************************************************************/

/*
 * $Id$
 *
 * Load the SRVN model.
 */

/* Debug Messages for Loading */
#if defined(DEBUG_MESSAGES)
#define DEBUG(x) cout << x
#else
#define DEBUG(X)
#endif

#include "petrisrvn.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <unistd.h>
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif
#if HAVE_UNISTD_H
#include <sys/stat.h>
#include <unistd.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <sys/errno.h>
#include <fcntl.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_call.h>
#include <lqio/glblerr.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/srvn_output.h>
#include <wspnlib/wspn.h>
#include <wspnlib/global.h>
#include "stack.h"
#include "actlist.h"
#include "task.h"
#include "entry.h"
#include "phase.h"
#include "errmsg.h"
#include "makeobj.h"
#include "stack.h"
#include "model.h"
#include "processor.h"
#include "runlqx.h"
#include "results.h"
#include "pragma.h"

#if HAVE_SYS_TIMES_H
typedef struct tms tms_t;
#else
typedef clock_t tms_t;
#endif

using namespace std;

bool Model::__forwarding_present;
bool Model::__open_class_error;
clock_t Model::__start_time = 0;
LQIO::DOM::Document::input_format Model::__input_format = LQIO::DOM::Document::AUTOMATIC_INPUT;

/* define	UNCONDITIONAL_PROBS */
/* define DERIVE_UTIL */


/* ---------- */

#if 0
double inter_proc_delay;
double comm_delay[DIMP+1][DIMP+1];	/* delay in sending a message.	*/
#endif
 
/* ------------------------------------------------------------------------ */
/* */
/* ------------------------------------------------------------------------ */

Model::Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name )
    : _document( document ),
      _input_file_name( input_file_name ),
      _output_file_name( output_file_name ),
      _n_phases(0)
{
}


/*
 * Delete the model.
 */

Model::~Model()
{
    for ( vector<Processor *>::const_iterator h = processor.begin(); h != processor.end(); ++h ) {
	Processor * aProcessor = *h;
	delete aProcessor;
    }
    processor.clear();

    for ( vector<Task *>::const_iterator t = task.begin(); t != task.end(); ++t ) {
	const Task * aTask = *t;
	delete aTask;
    }
    task.clear();
    entry.clear();

    for ( int i = 0; i <= layer_num; ++i) {
	if ( layer_name[i] ) {
	    free( layer_name[i] );
	    layer_name[i] = 0;
	}
    }
    layer_num = 0;

    free_group_store();
}



LQIO::DOM::Document*
Model::load( const string& input_filename, const string& output_filename )
{
#if HAVE_SYS_TIMES_H
    tms_t time_buf;

    Model::__start_time = times( &time_buf );
#else
    Model::__start_time = time( NULL );
#endif

    if ( verbose_flag ) {
	cerr << "Load: " << input_filename << "..." << endl;
    }

    io_vars.n_processors     = 0;
    io_vars.n_tasks	     = 0;
    io_vars.n_entries	     = 0;
    io_vars.anError          = false;
    io_vars.error_count      = 0;

    __forwarding_present     = false;
    __open_class_error	     = false;

    /*
     * Initialize everything that needs it before parsing
     */

    Entry::__next_entry_id   = 1;
    Activity::actConnections.clear();
    Activity::domToNative.clear();
    clear_hash_table();

    /*
     * Read input file and parse it.
     */

    unsigned errorCode = 0;
    return LQIO::DOM::Document::load(input_filename, __input_format, output_filename, &::io_vars, errorCode, false);
}


/*
 *	Called from the parser to set important modelling parameters.
 *	It can also check validity of same if so desired.
 */

/*ARGSUSED*/
void
Model::set_comment()
{
    const std::string& comment 	= _document->getModelComment();
    struct com_object * buf	= (struct com_object *)malloc( CMMOBJ_SIZE );

    netobj->comment = buf;

    const char * p = comment.c_str();
    buf->line = (char *)0;
    do { 
	const char * q = p;
	while ( *p != '\0' && *p != '\n' ) {
	    ++p;	/* Look for newlines	*/
	}
	buf->line = static_cast<char *>(malloc( (size_t)(p - q + 1) ));
	if ( p - q ) {
	    (void) strncpy( buf->line, q, p - q );
	}
	buf->line[p-q] = '\0';
	if ( *p == '\n' ) {
	    ++p;
	}
	if ( *p ) {
	    buf->next = (struct com_object *)malloc( CMMOBJ_SIZE );
	    buf = buf->next;
	}
    } while ( *p );

    buf->next = NULL;
}

Model&
Model::set_n_phases( const unsigned int n )
{
    if ( n > _n_phases ) _n_phases = n;
    return *this;
}

/*----------------------------------------------------------------------*/
/* Main...								*/
/*----------------------------------------------------------------------*/

bool
Model::construct()
{
    if ( verbose_flag ) {
	cerr << "Create: " << _input_file_name << "..." << endl;
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 0: Add Pragmas] */
    const map<string,string>& pragmaList = _document->getPragmaList();
    map<string,string>::const_iterator pragmaIter;
    for (pragmaIter = pragmaList.begin(); pragmaIter != pragmaList.end(); ++pragmaIter) {
	pragma( pragmaIter->first, pragmaIter->second );
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */
	
    /* We need to add all of the processors */
    const std::map<std::string,LQIO::DOM::Processor*>& processorList = _document->getProcessors();

    /* Add all of the processors we will be needing */
    for ( std::map<std::string,LQIO::DOM::Processor*>::const_iterator h = processorList.begin(); h != processorList.end(); ++h ) {
	LQIO::DOM::Processor* processor = h->second;
	DEBUG("[1]: Adding processor (" << processor->getName() << ")" << endl);
	Processor::create(processor);
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */
	
    /* In the DOM, tasks have entries, but here entries need to go first */
    const std::map<std::string,LQIO::DOM::Task*>& taskList = _document->getTasks();
	
    /* Add all of the tasks we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator t = taskList.begin(); t != taskList.end(); ++t ) {
	LQIO::DOM::Task* task = t->second;
	/* Now we can go ahead and add the task */
	DEBUG("[3]: Adding Task (" << task->getName() << ")" << endl);
	Task* newTask = Task::create(task);

	std::vector<LQIO::DOM::Entry*>::const_iterator nextEntry;
	std::vector<LQIO::DOM::Entry*> activityEntries;
	/* Before we can add a task we have to add all of its entries */
	DEBUG("[2]: Preparing to add entries for Task (" << task->getName() << ")" << endl);
		
		
	/* Add the entries so we can reverse them */
	for ( nextEntry = task->getEntryList().begin(); nextEntry != task->getEntryList().end(); ++nextEntry ) {
	    newTask->entries.push_back( Entry::create( *nextEntry, newTask ) );
	    if ((*nextEntry)->getStartActivity() != NULL) {
		activityEntries.push_back(*nextEntry);
	    }
	}
		
	/* Add activities for the task (all of them) */
	const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
	std::map<std::string,LQIO::DOM::Activity*>::const_iterator iter;
	for (iter = activities.begin(); iter != activities.end(); ++iter) {
	    const LQIO::DOM::Activity* activity = iter->second;
	    DEBUG("[3][a]: Adding Activity (" << activity->getName() << ") to Task." << endl);
	    newTask->add_activity(const_cast<LQIO::DOM::Activity*>(activity));
	}
		
	/* Set all the start activities */
	std::vector<LQIO::DOM::Entry*>::iterator entryIter;
	for (entryIter = activityEntries.begin(); entryIter != activityEntries.end(); ++entryIter) {
	    LQIO::DOM::Entry* theDOMEntry = *entryIter;
	    DEBUG("[3][b]: Setting Start Activity (" << theDOMEntry->getStartActivity()->getName().c_str() 
		  << ") for Entry (" << theDOMEntry->getName().c_str() << ")" << endl);
	    newTask->set_start_activity(theDOMEntry);
	}
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 3: Add Calls/Phase Parameters] */

    /* Add all of the calls for all phases to the system */
    /* We use this to add all calls */
    const std::map<std::string,LQIO::DOM::Entry*>& allEntries = _document->getEntries();
    for ( std::map<std::string,LQIO::DOM::Entry*>::const_iterator nextEntry = allEntries.begin(); nextEntry != allEntries.end(); ++nextEntry ) {
	LQIO::DOM::Entry* entry = nextEntry->second;
	Entry* newEntry = Entry::find(entry->getName());
	assert(newEntry != NULL);

	/* Go over all of the entry's phases and add the calls */
	for (unsigned p = 1; p <= entry->getMaximumPhase(); ++p) {
	    LQIO::DOM::Phase* phase = entry->getPhase(p);
	    const std::vector<LQIO::DOM::Call*>& originatingCalls = phase->getCalls();
	    std::vector<LQIO::DOM::Call*>::const_iterator iter;

	    /* Add all of the calls to the system */
	    for (iter = originatingCalls.begin(); iter != originatingCalls.end(); ++iter) {
		LQIO::DOM::Call* call = *iter;
#if defined(DEBUG_MESSAGES)
		LQIO::DOM::Entry* src = const_cast<LQIO::DOM::Entry*>(call->getSourceEntry());
		LQIO::DOM::Entry* dst = const_cast<LQIO::DOM::Entry*>(call->getDestinationEntry());
		DEBUG("[4]: Phase " << call->getPhase() << " Call (" << src->getName() << ") -> (" << dst->getName() << ")" << endl);
#endif
		/* Add the call to the system */
		Entry::add_call(call);
	    }
	}

	/* Add in all of the P(frwd) calls */
	const std::vector<LQIO::DOM::Call*>& forwarding = entry->getForwarding();
	std::vector<LQIO::DOM::Call*>::const_iterator nextFwd;
	for ( nextFwd = forwarding.begin(); nextFwd != forwarding.end(); ++nextFwd ) {
	    LQIO::DOM::Call* call = *nextFwd;
	    Entry::add_fwd_call(call);
	}
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 4: Add Calls/Lists for Activities] */

    /* Go back and add all of the lists and calls now that activities all exist */
    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	for ( std::vector<Activity*>::const_iterator a = (*t)->activities.begin(); a != (*t)->activities.end(); ++a) {
	    const LQIO::DOM::Activity* activity = dynamic_cast<const LQIO::DOM::Activity *>((*a)->get_dom());

	    const std::vector<LQIO::DOM::Call*>& originatingCalls = activity->getCalls();
	    std::vector<LQIO::DOM::Call*>::const_iterator iter;
	    /* Add all of the calls to the system */
	    for (iter = originatingCalls.begin(); iter != originatingCalls.end(); ++iter) {
		LQIO::DOM::Call* call = *iter;

		/* Add the call to the system */
		(*a)->add_call(call);
	    }
	    (*a)->add_reply_list()
		.add_activity_lists();

	}
    }

    /* Use the generated connections list to finish up */
    DEBUG("[5]: Adding connections." << endl);
    Activity::complete_activity_connections();


    return !io_vars.anError;
}


void Model::initialize() 
{
    Model::__forwarding_present = false;
    Model::__open_class_error = false;
    Phase::__parameter_x   = 0.5;
    Phase::__parameter_y   = 0.5;
    Task::__server_x_offset = 1;
    Task::__client_x_offset = 1;
    Processor::__x_offset   = 1;

    for ( vector<Processor *>::const_iterator p = ::processor.begin(); p != ::processor.end(); ++p ) {
	(*p)->initialize();
    }
    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	(*t)->initialize();
    }
    for ( vector<Entry *>::const_iterator e = ::entry.begin(); e != ::entry.end(); ++e ) {
	(*e)->initialize();
    }
}


/*
 * Fold, mutilate, and spindle...
 */

bool
Model::transform()
{
    unsigned max_width = 0;

    build_open_arrivals();

    for ( vector<Entry *>::const_iterator e = ::entry.begin(); e != ::entry.end(); ++e ) {
	(*e)->check();
    }
    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	(*t)->check();
	set_n_phases((*t)->n_phases());
    }
    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	(*t)->build_forwarding_lists();
    }

    if ( io_vars.anError ) return false;

    const unsigned int max_queue_length = set_queue_length();		/* Do after forwarding */
    const unsigned int max_proc_queue_length = Processor::set_queue_length();

    if ( !init_net() ) {
	cerr << io_vars.lq_toolname << ": Cannot initialize net " << _input_file_name << "!" << endl;
	exit( EXCEPTION_EXIT );
    }

    /* ------------------------------------------------------------------------ */

    /*
     * Set up layers.  strdup so that we do not bus-error on net free.
     */

    layer_name[0]			= strdup( "the_whole_net" );
    layer_name[PROC_LAYER_NUM]	   	= strdup( "processors" );
    layer_name[PRIMARY_LAYER_NUM]	= strdup( "primary_copy" );
    layer_name[SERVICE_RATE_LAYER_NUM]  = strdup( "service_rates" );
    layer_name[CALL_RATE_LAYER_NUM]	= strdup( "call_rates" );
    layer_name[MEASUREMENT_LAYER_NUM]	= strdup( "measurement" );
    layer_name[FIFO_LAYER_NUM]	   	= strdup( "fifo_queue" );
    layer_name[JOIN_LAYER_NUM]	   	= strdup( "joins" );

    unsigned int i = 0;
    for ( vector<Entry *>::const_iterator e = ::entry.begin(); e != ::entry.end(); ++e, ++i ) {
	layer_name[ENTRY_LAYER_NUM+i] = strdup( (*e)->name() );
    }
    layer_num = ENTRY_LAYER_NUM+::entry.size()-1;

    /*
     * Set comment
     */

    set_comment();

    /*
     * Compute offsets.
     */

    if ( pragma.task_scheduling() == SCHEDULE_RAND ) {
	Task::__queue_y_offset  = -3.;
    } else {
	Task::__queue_y_offset  = -static_cast<double>(max_queue_length+2);
    }

    Task::__server_y_offset = Place::SERVER_Y_OFFSET - Task::__queue_y_offset;

    if ( __forwarding_present ) {
	Task::__server_y_offset += 1.0;
    }
#if 0
    if ( comm_delay_flag ) {
	Task::__server_y_offset += 2.0;
    }
#endif

    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	if ( (*t)->type() != REF_TASK || (*t)->n_activities() == 0 ) continue;

	for ( vector<ActivityList *>::const_iterator l = (*t)->act_lists.begin(); l != (*t)->act_lists.end(); ++l ) {
	    if ( ( (*l)->type() == ACT_AND_FORK_LIST || (*l)->type() == ACT_OR_FORK_LIST ) && (*l)->n_acts() > max_width ) {
		max_width = (*l)->n_acts();
	    } else if ( (*l)->type() == ACT_LOOP_LIST && max_width < 2 ) {
		max_width = 2;
	    }
	}
    }
    if ( max_width > 1 ) {
	Task::__server_y_offset += max_width - 1;
    }
		
    /* Create rate and result parameters */

    trans_rpar();
    trans_res();

    /* Build processors. */

    for ( vector<Processor *>::const_iterator p = ::processor.begin(); p != ::processor.end(); ++p ) {
	(*p)->transmorgrify( max_proc_queue_length );
    }

    /* Build tasks. */

    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	(*t)->transmorgrify();
    }

    for ( vector<Entry *>::const_iterator e = ::entry.begin(); e != ::entry.end(); ++e ) {
	(*e)->create_forwarding_gspn();
    }

    /* Build queues twixt tasks. */

    make_queues();

    /* Format for solver */

    groupize();
    shift_rpars( Task::__client_x_offset, Phase::__parameter_y );

    return !io_vars.anError;
}


unsigned int
Model::set_queue_length()  const
{
    unsigned max_queue_length = 0;

    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	unsigned int length = (*t)->set_queue_length();
	if ( length > max_queue_length ) {
	    max_queue_length = length;
	}
    }

    return max_queue_length;
}


/* 
 * Reset associations in preparation for another run.
 */

void
Model::remove_netobj()
{
    for ( vector<Processor *>::const_iterator p = processor.begin(); p != processor.end(); ++p ) {
	(*p)->remove_netobj();
    }
    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	(*t)->remove_netobj();
    }

    free_netobj( netobj );
    netobj = (struct net_object *)0;
}

/* -------------------------------------------------------------------- */
/* Solve.								*/
/* -------------------------------------------------------------------- */

/*
 * Solve the model.
 */

int
Model::solve()
{
    int rc = 0;

    initialize();
    if ( !transform() ) {
	cerr << io_vars.lq_toolname << ": Input model " << _input_file_name << " was not transformed successfully." << endl;
	return EXCEPTION_EXIT;
    }

    const LQIO::Filename netname( _input_file_name.c_str(), static_cast<const char *>(0), static_cast<const char *>(0), 
				  _document->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix.c_str() : static_cast<const char *>(0) );

    if ( reload_net_flag ) {
	sprintf(edit_file, "%s", netname() );
    } else {
	if ( verbose_flag ) {
	    cerr << "Solve: " << _input_file_name << "..." << endl;
	}

	save_net_files( io_vars.lq_toolname, netname() );

	if ( no_execute_flag ) {
	    return NORMAL_TERMINATION;
	} else if ( trace_flag ) {
	    if ( !solve2( netname(), 2, SOLVE_STEADY_STATE ) ) {	/* output to stderr */
		rc = EXCEPTION_EXIT;
	    }
	} else {
	    int null_fd = open( "/dev/null", O_RDWR );
	    if ( !solve2( netname(), null_fd, SOLVE_STEADY_STATE ) ) {
		rc = EXCEPTION_EXIT;
	    }
	    close( null_fd );
	}
    }

    if ( verbose_flag ) {
	cerr << "Done: " << endl;
    }

    if ( rc == NORMAL_TERMINATION ) {
	 solution_stats_t stats;
	 if ( !solution_stats( &stats.tangible, &stats.vanishing, &stats.precision )
	      || !collect_res( FALSE, (char *)io_vars.lq_toolname ) ) {
	     (void) fprintf( stderr, "%s: Cannot read results for %s\n", io_vars.lq_toolname, netname() );
	     rc = FILEIO_ERROR;
	 } else {
	     if ( stats.precision >= 0.01 || __open_class_error ) {
		 rc = INVALID_OUTPUT;
	     }
	     for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
		 (*t)->get_results();		/* Read net to get tokens. */
	     }
	     insert_DOM_results( rc == 0, stats );	/* Save results */
	     print();		/* Now output them. */
	     if ( verbose_flag ) {
		 cerr << stats;
	     }
	 }
     }

    /* Clean up in preparation for another run.	*/

    if ( !keep_flag ) {
	remove_result_files( netname() );
    }

    remove_netobj();
    return rc;
}



/*
 * Read result files only.  LQX print uses these results.
 */

int
Model::reload()
{
    /* Default mapping */

    LQIO::Filename directory_name( has_output_file_name() ? _output_file_name.c_str() : _input_file_name.c_str(), "d" );		/* Get the base file name */
    const char * suffix = SolverInterface::Solve::customSuffix.c_str();

    if ( access( directory_name(), R_OK|W_OK|X_OK ) < 0 ) {
	solution_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name(), strerror( errno ) );
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    }

    LQIO::Filename filename( _input_file_name.c_str(), "lqxo", directory_name(), suffix );
		
    unsigned int errorCode;
    if ( !_document->loadResults( filename(), errorCode ) ) {
	cerr << io_vars.lq_toolname << ": Input model was not loaded successfully." << endl;
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    } else {
	return NORMAL_TERMINATION;
    }
}


int
Model::restart()
{
    if ( reload() == NORMAL_TERMINATION && _document->getResultValid() ) {
	return NORMAL_TERMINATION;
    } else {
	return solve();
    }
}

/*----------------------------------------------------------------------*/
/*			     Transitions				*/
/*----------------------------------------------------------------------*/

/*
 * Create queues at tasks.  The argument specifies the queue creation function.
 */

void
Model::make_queues()
{
    struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT];

    /* Make queues */
	
    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	if ( (*t)->is_client() ) continue;  	/* Skip reference tasks. */

	double x_pos	= (*t)->get_x_pos() - 0.5;                           
	double y_pos	= (*t)->get_y_pos();                                 
	unsigned ne	= (*t)->n_entries();                             
	double idle_x;
	unsigned k 	= 0;		/* Queue Kounter		*/
	queue_fnptr queue_func;		/* Local version.	*/
	bool sync_server = (*t)->is_sync_server() || (*t)->has_random_queueing() || bit_test( (*t)->type(), INF_SERV_BIT|SEMAPHORE_BIT);

	/* Override if dest is a join function. */
		
	if ( sync_server ) {
	    idle_x = x_pos + 1.0 + 0.5;
	    queue_func = &Model::random_queue;
	} else {
	    idle_x = x_pos + static_cast<double>((*t)->max_queue_length() + 0.5);
	    queue_func = &Model::fifo_queue;
	}
			
	/* Create and connect all queues for task 'j'. */

	for ( vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {
	    for ( vector<Task *>::const_iterator i = ::task.begin(); i != ::task.end(); ++i ) {
		unsigned max_m = (*i)->n_customers();

		for ( vector<Entry *>::const_iterator d = (*i)->entries.begin(); d != (*i)->entries.end(); ++d ) {
		    if ( (*d)->is_regular_entry() ) {
			for ( unsigned p = 1; p <= (*d)->n_phases(); p++ ) {
			    k = make_queue( x_pos, y_pos, idle_x, &(*d)->phase[p], *e, ne, max_m, k, ins_place, queue_func );
			}
		    }

		    if ( (*d)->prob_fwd(*e) > 0.0 ) {
			for ( vector<Forwarding *>::const_iterator f = (*d)->forwards.begin(); f != (*d)->forwards.end(); ++f ) {
			    k += 1;
			    (this->*queue_func)(X_OFFSET(1,0.0) + k * 0.5, y_pos, idle_x, 
						&(*d)->phase[1], 0, *e, (*f)->_root, 
						(*f)->_slice_no, (*f)->_m, (*d)->prob_fwd(*e),  
						k, false, ins_place );
			}
		    }
		}

		for ( vector<Activity *>::const_iterator a = (*i)->activities.begin(); a != (*i)->activities.end(); ++a ) {
		    k = make_queue( x_pos, y_pos, idle_x, (*a), *e, ne, max_m, k, ins_place, queue_func );
		}
		if ( sync_server ) {
		    k += 1;
		} 
	    } /* a */
			
	} /* lj */

	(*t)->set_max_k(k);

	/*
	 * Now adjust debug stuff...
	 */

	if ( (*t)->inservice_flag() ) {
	    double temp_y = y_pos + Task::__queue_y_offset + 1.5;

	    /* Be shifty... */

	    for ( unsigned int m = 0; m < (*t)->multiplicity(); ++m ) {
		double m_delta = (double)m / 4.0;
		(*t)->TX[m]->center.x  = IN_TO_PIX( idle_x + m_delta );
		(*t)->TX[m]->center.y += IN_TO_PIX( m_delta );
#if defined(BUG_163)
		if ( (*t)->is_sync_server() ) { 
		    (*t)->SyX[m]->center.x  = IN_TO_PIX( idle_x + 0.5 + m_delta );
		    (*t)->SyX[m]->center.y += IN_TO_PIX( m_delta );
		}
#endif
	    }

	    /*
	     * connect all state places to instrumentation transitions
	     */
			
	    for ( vector<Entry *>::const_iterator b = (*t)->entries.begin(); b != (*t)->entries.end(); ++b ) {
		for ( vector<Task *>::const_iterator i = ::task.begin(); i != ::task.end(); ++i ) {
		    unsigned max_m = (*i)->n_customers();

		    for ( vector<Entry *>::const_iterator e = (*i)->entries.begin(); e != (*i)->entries.end(); ++e ) {
				
			if ( !(*e)->is_regular_entry() ) continue;
						
			for ( unsigned p = 1; p <= (*e)->n_phases(); p++ ) {
			    Phase * curr_phase = &(*e)->phase[p];

			    if (curr_phase->y(*b) == 0.0) continue;

			    for ( unsigned int m = 0; m < max_m; ++m ) {
				create_inservice_net( idle_x, temp_y, curr_phase, *b, m, ins_place );
				temp_y += 0.5;
			    }
			} /* p */
		    } /* li */
		} /* i */
	    } /* lj */
	} /* inservice_flag */
    } /* j */
}


/*
 * Make a single queue for a slice.  
 */

unsigned
Model::make_queue( double x_pos,		/* x coordinate.		*/
		   double y_pos,		/* y coordinate.		*/
		   double idle_x,
		   Phase * a,			/* Source Entry (send from)	*/
		   Entry * b,			/* Destination entry.		*/
		   const unsigned ne,
		   const unsigned max_m,	/* Multiplicity of Src.		*/
		   unsigned k,			/* an index.			*/
		   struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT],
		   queue_fnptr queue_func )
{
    unsigned m;
    double calls = a->y(b) + a->z(b);
	
    if ( calls == 0.0) return k;	/* No operation. */
	
    for ( m = 0; m < max_m; ++m ) {
	bool async_call = a->z(b) > 0 || a->task()->type() == OPEN_SRC;
		
	if ( a->has_stochastic_calls() ) {
	    k += 1;
	    (this->*queue_func)( X_OFFSET(1,0.0) + k * 0.5, y_pos, idle_x,
				 a, 0, b, a, 0, m, 0.0, k,
				 async_call, ins_place );
	} else {
	    unsigned s;
	    unsigned off = a->compute_offset( b );			/* Compute offset */
	    for ( s = 0; s < calls; ++s ) {
		k += 1;
		(this->*queue_func)( X_OFFSET(1,0.0) + k * 0.5, y_pos, idle_x,
				     a, s+off, b, a, s+off+1, m, 0.0, k,
				     async_call, ins_place );
	    }
	}

    }
    return k;
}
				



#define INS_OFFSET( x, delta )	((x)+idle_x+(delta)-0.5)

/*
 * Create queue (and some of the tracing logic if requested).
 */

void
Model::fifo_queue( double x_pos,		/* x coordinate.		*/
		   double y_pos,		/* y coordinate.		*/
		   double idle_x,
		   Phase * a,			/* Source Entry (send from)	*/
		   const unsigned s_a,		/* Sending slice number.	*/
		   Entry * b,			/* Destination entry.		*/
		   const Phase * e,		/* Entry to reply to.		*/
		   const unsigned s_e,		/* Slice to reply to.		*/
		   const unsigned m,		/* Multiplicity of Src.		*/
		   const double prob_fwd,
		   const unsigned k,		/* an index.			*/
		   const bool async_call,
		   struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] )
{
    struct trans_object * c_trans;
    struct place_object * r_place = 0;
    struct trans_object * r_trans;
    const Task * j	= b->task();
    unsigned b_m;			/* Mult index of dst.		*/
    const LAYER layer_mask = ENTRY_LAYER(b->entry_id())|(m == 0 ? PRIMARY_LAYER : 0);
	
    const char * task_name = j->name();

    b->set_random_queueing(false);

    const_cast<Task *>(j)->make_queue_places();

    c_trans = queue_prologue( x_pos, y_pos + Task::__queue_y_offset + 0.5, a, s_a, b, j->multiplicity(),
			      e, s_e, m, prob_fwd, async_call, &r_trans );

    create_arc( FIFO_LAYER, TO_TRANS, c_trans, no_place( "Sh%s1", task_name ) );

    for ( unsigned l = 1; l <= j->max_queue_length(); l++ ) {

	r_place = create_place( x_pos, y_pos + Task::__queue_y_offset + (double)l, layer_mask, 0,
				"I%s%d%s%s%d%d", a->name(), s_a, b->name(), e->name(), m, l );

	create_arc( layer_mask, TO_PLACE, c_trans, r_place );

	if ( l < j->max_queue_length() ) {

	    c_trans = create_trans( x_pos, y_pos + Task::__queue_y_offset + (double)l + 0.5,
				    layer_mask,
				    1.0, 1, IMMEDIATE,
				    "i%s%d%s%s%d%d", a->name(), s_a, b->name(), e->name(), m, l );
	    create_arc( FIFO_LAYER, TO_PLACE, c_trans, no_place( "Sh%s%d", task_name, l ) );
	    create_arc( layer_mask, TO_TRANS, c_trans, r_place );
	    create_arc( FIFO_LAYER, TO_TRANS, c_trans, no_place( "Sh%s%d", task_name, l + 1 ) );

	}
    } /* l */

    const unsigned int l = j->max_queue_length();

    for ( b_m = 0; b_m < j->multiplicity(); ++b_m ) {

	double n_delta = (double)b_m / 4.0;
	struct trans_object * q_trans;

	q_trans = create_trans( x_pos + n_delta, y_pos + Task::__queue_y_offset + (double)l + 0.5 + n_delta,
				layer_mask, 1.0, 1, IMMEDIATE,
				"i%s%d%s%d%s%d%d", a->name(), s_a, b->name(), b_m, e->name(), m, l );
	create_arc( FIFO_LAYER, TO_PLACE, q_trans, no_place( "Sh%s%d", task_name, l ) );
	create_arc( layer_mask, TO_TRANS, q_trans, r_place );
	/*+ BUG_164 */
	if ( j->type() == SEMAPHORE ) {
	    abort();	/* Should not use FIFO queue for semaphore task */
	} else {
	    create_arc( layer_mask, TO_TRANS, q_trans, j->TX[b_m] );
	}
	/*- BUG_164 */

	if ( b->is_regular_entry() ) {
	    create_arc( layer_mask, TO_PLACE, q_trans, b->phase[1].ZX[b_m] );
	} else {
#if defined(BUG_622)
	    create_arc( MEASUREMENT_LAYER, TO_PLACE, q_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
#endif
	    create_arc( layer_mask, TO_PLACE, q_trans, b->start_activity()->ZX[b_m] );
	}

	struct trans_object * s_trans = 0;
#if defined(BUG_163)
	if ( j->is_sync_server() ) {
	    /* Create the second "i" transition to handle the SYNC wait. */
	    s_trans = create_trans( x_pos + n_delta + 0.5, y_pos + Task::__queue_y_offset + (double)l + 0.5 + n_delta,
				    layer_mask, 1.0, 1, IMMEDIATE,
				    "sync%s%d%s%d%s%d%d", a->name(), s_a, b->name(), b_m, e->name(), m, l );
	    create_arc( FIFO_LAYER, TO_PLACE, s_trans, no_place( "Sh%s%d", task_name, l ) );
	    create_arc( layer_mask, TO_TRANS, s_trans, r_place );
	    create_arc( layer_mask, TO_TRANS, s_trans, j->SyX[b_m] );
	    if ( b->is_regular_entry() ) {
		create_arc( layer_mask, TO_PLACE, s_trans, b->phase[1].ZX[b_m] );
	    } else {
#if defined(BUG_622)
		create_arc( MEASUREMENT_LAYER, TO_PLACE, s_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
#endif
		create_arc( layer_mask, TO_PLACE, s_trans, b->start_activity()->ZX[b_m] );
	    }
	}
#endif

	queue_epilogue( x_pos + n_delta, y_pos + n_delta, a, s_a, b, b_m, e, s_e, m, async_call, q_trans, s_trans );

	/*
	 * Bonus stuff in case we want in-service probabilites.
	 * These states mark the active entry and phase.
	 */

	if ( j->inservice_flag() ) {
	    create_phase_instr_net( idle_x, y_pos, a, m, b, b_m, k, r_trans, q_trans, s_trans, ins_place );
	}
    } /* b_m */
}



void
Model::random_queue( double x_pos,		/* x coordinate.		*/
		     double y_pos,		/* y coordinate.		*/
		     double idle_x,
		     Phase * a,			/* Source Entry (send from)	*/
		     const unsigned s_a,	/* Slice number of a.		*/
		     Entry * b,			/* Destination entry.		*/
		     const Phase * e,		/* Entry to reply to.		*/
		     const unsigned s_e,	/* Slice to reply to.		*/
		     const unsigned m,		/* Multiplicity of Src.		*/
		     const double prob_fwd,
		     const unsigned k,		/* an index.			*/
		     const bool async_call,
		     struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] )
{
    struct place_object * r_place;
    struct trans_object * c_trans;
    struct trans_object * r_trans;
    struct trans_object * q_trans;
    struct trans_object * s_trans = 0;
    const Task * j	= b->task();
    unsigned b_m	= j->multiplicity();
    double n_delta;
    double temp_y	= y_pos + Task::__queue_y_offset + 0.5;
    const LAYER layer_mask = ENTRY_LAYER(b->entry_id())|(b_m == 0 ? PRIMARY_LAYER : 0);
	
    b->set_random_queueing(true);
    c_trans = queue_prologue( x_pos, temp_y - 0.5, a, s_a, b, j->multiplicity(),
			      e, s_e, m, prob_fwd, async_call, &r_trans );

    r_place = create_place( x_pos, temp_y + 0.5, layer_mask, 0,
			    "I%s%d%s%s%d", a->name(), s_a, b->name(), e->name(), m );
    create_arc( layer_mask, TO_PLACE, c_trans, r_place );

    for ( b_m = 0; b_m < j->multiplicity(); ++b_m ) {

	n_delta = (double)b_m / 4.0;
		
	q_trans = create_trans( x_pos + n_delta, temp_y + 1.0 + n_delta,
				layer_mask,
				1.0, 1, IMMEDIATE,
				"i%s%d%s%s%d%d", a->name(), s_a, b->name(), e->name(), b_m, m );

	create_arc( layer_mask, TO_TRANS, q_trans, r_place );
	/*+ BUG_164 */
	if ( j->type() == SEMAPHORE && b->semaphore_type() == SEMAPHORE_SIGNAL ) {
	    create_arc( layer_mask, TO_TRANS, q_trans, j->LX[b_m] );
	} else {
	    create_arc( layer_mask, TO_TRANS, q_trans, j->TX[b_m] );
	}
	/*- BUG_164 */
	if ( b->is_regular_entry() ) {
	    create_arc( layer_mask, TO_PLACE, q_trans, b->phase[1].ZX[b_m] );
	} else {
#if defined(BUG_622)
	    create_arc( MEASUREMENT_LAYER, TO_PLACE, q_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
#endif
	    create_arc( layer_mask, TO_PLACE, q_trans, b->start_activity()->ZX[b_m] );
	}
#if defined(BUG_163)
	if ( j->is_sync_server() ) {
	    /* Create the second "i" transition to handle the SYNC wait. */
	    s_trans = create_trans( x_pos + n_delta + 0.5, temp_y + 1.0 + n_delta,
				    layer_mask,
				    1.0, 1, IMMEDIATE,
				    "sync%s%d%s%s%d%d", a->name(), s_a, b->name(), e->name(), b_m, m );
	    create_arc( layer_mask, TO_TRANS, s_trans, r_place );
	    create_arc( layer_mask, TO_TRANS, s_trans, j->SyX[b_m] );
	    if ( b->is_regular_entry() ) {
		create_arc( layer_mask, TO_PLACE, s_trans, b->phase[1].ZX[b_m] );
	    } else {
#if defined(BUG_622)
		create_arc( MEASUREMENT_LAYER, TO_PLACE, s_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
#endif
		create_arc( layer_mask, TO_PLACE, s_trans, b->start_activity()->ZX[b_m] );
	    }
	}
#endif

	queue_epilogue( x_pos + n_delta, temp_y + 2.5 + n_delta, a, s_a, b, b_m, e, s_e, m, async_call, q_trans, s_trans );

	/*
	 * Bonus stuff in case we want in-service probabilites.
	 * These states mark the active entry and phase.
	 */

	if ( j->inservice_flag() ) {
	    create_phase_instr_net( idle_x, y_pos, a, m, b, b_m, k, r_trans, q_trans, s_trans, ins_place );
	}
    }
}



/*
 * Common code for head of queue.
 */

struct trans_object *
Model::queue_prologue( double x_pos,		/* X coordinate.		*/
		       double y_pos,		/* Y coordinate.		*/
		       Phase * a,		/* sending entry.		*/
		       unsigned s_a,		/* Slice number of phase	*/
		       Entry * b,		/* receiving entry.		*/
		       unsigned b_m,		/* instance number of b.	*/
		       const Phase * e,		/* Entry to reply to.		*/
		       const unsigned s_e,	/* Slice to reply to.		*/
		       unsigned m,		/* instance number of a.	*/
		       double prob_fwd,
		       bool async_call,	/* True if z type call.		*/
		       struct trans_object **r_trans )
{
    struct trans_object * c_trans;
    struct place_object * c_place;

    const LAYER layer_mask_a = ENTRY_LAYER(a->entry_id())|(m == 0 ? PRIMARY_LAYER : 0);
    const LAYER layer_mask_b = ENTRY_LAYER(b->entry_id())|(m == 0 ? PRIMARY_LAYER : 0);

#if 0
    if ( comm_delay_flag && comm_delay[from_proc][to_proc] > 0.0 ) {
	y_pos -= 1.5;
    } else {
	y_pos += 0.5;
    }
#endif
	
    if ( prob_fwd > 0.0 ) {

	/* Forwarding! */
		
	c_trans = create_trans( x_pos, y_pos, layer_mask_a|layer_mask_b,
				prob_fwd, 1, IMMEDIATE, "req%s%d%s%s%d",
				a->name(), s_a, b->name(), e->name(), m );

	c_place = no_place( "FWD%s%d%s%d", e->name(), s_e, a->name(), m );
	create_arc( layer_mask_a, TO_TRANS, c_trans, c_place );
		
	*r_trans = c_trans;

    } else {

	c_trans = create_trans( x_pos, y_pos, layer_mask_a|layer_mask_b,
				-a->rpar_y(b), 1, IMMEDIATE,
				"req%s%d%s%s%d", a->name(), s_a, b->name(), e->name(), m );
	create_arc( layer_mask_a|layer_mask_b, TO_TRANS, c_trans, a->_slice[s_a].ChX[m] );
	
	*r_trans = c_trans;
	
	if ( async_call ) {
	    struct trans_object * d_trans;
#if !defined(BUFFER_BY_ENTRY)
	    const Task * j = b->task();
#endif

	    create_arc( layer_mask_b, TO_PLACE, c_trans, e->_slice[s_e].WX[m] );	/* Reply immediately */

	    /* Place to limit size of open requests */

#if defined(BUFFER_BY_ENTRY)
	    if ( !b->ZZ ) {
		b->ZZ = create_place( x_pos, y_pos - 0.5, layer_mask_a|layer_mask_b,
				      open_model_tokens, "ZZ%s", b->name()  );
	    }
	    c_place = b->ZZ;
#else
	    if ( !j->ZZ ) {
		const_cast<Task *>(j)->ZZ = create_place( x_pos, y_pos - 0.5, layer_mask_a|layer_mask_b,
							  j->n_open_tokens(), "ZZ%s", j->name()  );
	    }
	    c_place = j->ZZ;
#endif
	    create_arc( layer_mask_b, TO_TRANS, c_trans, c_place );	/* req also needs token from ZZ */

	    /* Loop for dropping requests. */
	    
	    d_trans = create_trans( x_pos, y_pos - 1.0, layer_mask_a,
				    -a->rpar_y(b), 1, IMMEDIATE,
				"drop%s%d%s%s%d", a->name(), s_a, b->name(), e->name(), m );
	    create_arc( layer_mask_a, TO_TRANS, d_trans, a->_slice[s_a].ChX[m] );
	    create_arc( layer_mask_a, INHIBITOR, d_trans, c_place );
	    create_arc( layer_mask_b, TO_PLACE, d_trans, e->_slice[s_e].WX[m] );	/* Reply immediately */
	}
    }

#if 0
    /* Add inter-processor delay components */

    if ( comm_delay_flag && comm_delay[from_proc][to_proc] > 0.0 ) {
	c_place = create_place( x_pos, y_pos + 0.5, layer_mask_b, 0,
				"DLYB%s%s%d", a->name(), b->name(), m );
	create_arc( layer_mask_b, TO_PLACE, c_trans, c_place );
	c_trans = create_trans( x_pos, y_pos + 1.0, layer_mask_b,
				1.0 / comm_delay[from_proc][to_proc], 1, EXPONENTIAL,
				"dlyb%s%s%d", a->name(), b->name(), m );
	create_arc( layer_mask_b, TO_TRANS, c_trans, c_place );
	c_place = create_place( x_pos, y_pos + 1.5, layer_mask_b, 0,
				"DLYE%s%s%d", a->name(), b->name(), m );
	create_arc( layer_mask_b, TO_PLACE, c_trans, c_place );
	c_trans = create_trans( x_pos, y_pos + 2.0, layer_mask_b, 1.0, 1, IMMEDIATE,
				"dlye%s%s%d", a->name(), b->name(), m );
	create_arc( layer_mask_b, TO_TRANS, c_trans, c_place );
    }
#endif
    return c_trans;
}



/*
 * Common code for tail of queue.
 */

void
Model::queue_epilogue( double x_pos, double y_pos,
		       Phase * a, unsigned s_a,		/* Source Phase 	*/
		       Entry * b, unsigned b_m,		/* Destination Entry 	*/
		       const Phase * e, unsigned s_e,
		       unsigned m,			/* Source multiplicity 	*/
		       bool async_call,			/* True if z type call.	*/
		       struct trans_object * q_trans, struct trans_object * s_trans )
{
    struct place_object * c_place;
    struct trans_object * c_trans;
    const Task * cur_task	= b->task();
    const LAYER layer_mask_a 	= ENTRY_LAYER(b->entry_id())|(m == 0 ? PRIMARY_LAYER : 0);
    const LAYER layer_mask_b 	= ENTRY_LAYER(b->entry_id())|(b_m == 0 ? PRIMARY_LAYER : 0);
	
    /* Guard place for external joins */

    if ( cur_task->is_sync_server() ) {
	struct place_object * g_place;
	if ( b->GdX[b_m] == 0 ) {
	    g_place = move_place_tag( create_place( x_pos+cur_task->n_entries(), y_pos - 1.0,
						    layer_mask_b, 1,
						    "Gd%s%d%s%d", a->name(), s_a, b->name(), b_m ),
				      Place::PLACE_X_OFFSET, -0.25 );
	    b->GdX[b_m] = g_place;
	    create_arc( layer_mask_b, TO_PLACE, cur_task->gdX[b_m], g_place );
	} else {
	    g_place = b->GdX[b_m];
	}
	create_arc( layer_mask_b, TO_TRANS, q_trans, g_place );
#if defined(BUG_163)
	if ( s_trans ) {
	    create_arc( layer_mask_b, TO_TRANS, s_trans, g_place );
	}
#endif
    } 
		
    c_place = create_place( x_pos, y_pos - 1.0, layer_mask_a, 0,
			    "R%s%d%s%d%s%d", a->name(), s_a, b->name(), b_m, e->name(), m );

    create_arc( layer_mask_a, TO_PLACE, q_trans, c_place );
#if defined(BUG_163)
    if ( s_trans ) {
	create_arc( layer_mask_a, TO_PLACE, s_trans, c_place );
    }
#endif

    c_trans = create_trans( x_pos, y_pos - 0.5, layer_mask_a, b->release_prob(), 1, IMMEDIATE + 1,
			    "rel%s%d%s%s%d", a->name(), s_a, b->name(), e->name(), m );
    create_arc( layer_mask_a, TO_TRANS, c_trans, c_place );

    /* Connect release to appropriate task provided that we do not have to worry about forwarding. */

    if ( b->requests() == RENDEZVOUS_REQUEST ) {		/* BUG_629 */
	create_arc( layer_mask_a, TO_TRANS, c_trans, b->DX[b_m] );
    }
	
    if ( async_call ) {	
#if defined(BUFFER_BY_ENTRY)
	c_place = entry[b->entry]->ZZ;	/* Reply to token pool */
#else
	c_place = cur_task->ZZ;		/* Reply to token pool */
#endif
    } else if ( b->release_prob() == 1.0 ) {
	c_place = e->_slice[s_e].WX[m];
    } else {
	c_place = no_place( "FWD%s%d%s%d", e->name(), s_e, b->phase[1].name(), m );
    }
    c_place->layer |= ENTRY_LAYER(b->entry_id());
    create_arc( ENTRY_LAYER(b->entry_id())|(m == 0 ? PRIMARY_LAYER : 0), TO_PLACE, c_trans, c_place );

#if defined(BUG_263)
    /*
     * We have to find all of the 'sink' places and put inhibitor arcs
     * to disallow requests until all threads terminate.
     */
#endif
}


/*
 * Create the subnet used to instrument the phase of service of
 * entry b's task.
 */

void
Model::create_phase_instr_net( double idle_x, double y_pos, 
			       Phase * a, unsigned m,
			       Entry * b, unsigned n, unsigned k,
			       struct trans_object * r_trans, struct trans_object * q_trans, struct trans_object * s_trans, 
			       struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] )
{
    unsigned q;			/* Another phase index.		*/
    struct place_object * c_place;
    struct trans_object * c_trans = q_trans;
    double n_delta = (double)n / 4.0;
    double temp_x;
    double temp_y;
    const Task * j     = b->task();

    if ( k > DIMDBGPLC ) (void) abort();

    for ( q = 1; q <= b->n_phases(); ++q ) {
	ins_place[q][k][n].c = a;
	ins_place[q][k][n].d = &b->phase[q];
	ins_place[q][k][n].m = m;

	temp_x = INS_OFFSET(k,(double)(q-1)/2.0) + n_delta;
	temp_y = y_pos + n_delta + (double)(q-1) - 2;
		
	ins_place[q][k][n].place = create_place( temp_x, temp_y, MEASUREMENT_LAYER, 0,
						 "PH%d%s%d%s%d", q,
						 a->name(), m,
						 b->name(), n );
	create_arc( MEASUREMENT_LAYER, TO_PLACE, c_trans, ins_place[q][k][n].place );
	if ( s_trans ) {
	    create_arc( MEASUREMENT_LAYER, TO_PLACE, s_trans, ins_place[q][k][n].place );
	    s_trans = 0;
	}
		
	c_trans = create_trans( b->phase[q].done_xpos[m], b->phase[q].done_ypos[m],
				MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), 
				1.0, 1, IMMEDIATE, "ph%d%s%d%s%d", q,
				a->name(), m,
				b->name(), n );
		
	if ( q == 1 && b->requests() == RENDEZVOUS_REQUEST ) {	/* BUG_629 */
	    create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_PLACE, c_trans, b->DX[n] );
	}
	if ( q == b->n_phases() ) {
	    create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_PLACE, c_trans, j->TX[n] );
	} else {
	    create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_PLACE, c_trans, b->phase[q+1]._slice[0].WX[n] );
	}

	create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, ins_place[q][k][n].place );
	create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_TRANS, c_trans, b->phase[q]._slice[0].ChX[n] );
    }
	
    /* --- */
	
    temp_y = y_pos + Task::__queue_y_offset + 1.0;
	
    c_place = create_place( INS_OFFSET(k,0.0) + n_delta, temp_y + n_delta, MEASUREMENT_LAYER, 0,
			    "Arr%s%d%s%d", a->name(), m, b->name(), n );
	
    c_trans = create_trans( idle_x + n_delta, temp_y + k * 0.5 + n_delta, MEASUREMENT_LAYER,
			    1.0, 1, IMMEDIATE+1,
			    "ArI%s%d%s%d", a->name(), m, b->name(), n );
	
    create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, c_place );
    create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, j->TX[n] );
    create_arc( MEASUREMENT_LAYER, TO_PLACE, c_trans, j->TX[n] );
    create_arc( MEASUREMENT_LAYER, TO_PLACE, r_trans, c_place );
}



/*
 * Create transitions for measuring in-service probabilites.
 */

void
Model::create_inservice_net( double x_pos, double y_pos,
		      Phase * a,	/* Entry of calling task 'i'	*/
		      Entry * b,	/* Entry of server 'j'		*/
		      unsigned m,	/* Instance of task 'i'		*/
		      struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] )
{
    const Task * j	= b->task();
    struct trans_object * c_trans;
    unsigned n;

    for ( n = 0; n < j->multiplicity(); ++n ) {
	double n_delta = (double)n/4.0;

	unsigned k;
	for ( k = 1; k <= j->max_k(); ++k ) {
	    double temp_x = x_pos + k + n_delta - 1.0;
	    double temp_y = y_pos + n_delta;
	    unsigned q;			/* Another phase index		*/
	    struct place_object * c_place;

	    c_place = no_place("Arr%s%d%s%d", a->name(), m, b->name(), n );
			
	    for ( q = 2; q <= b->n_phases(); ++q ) {
		c_trans = create_trans( temp_x + (double)q/2.0, temp_y, MEASUREMENT_LAYER,
					1.0, 1, IMMEDIATE+1,
					"ArX%s%d%s%d%s%d%s%d",
					a->name(), m,
					b->name(), n,
					ins_place[q][k][n].c->name(), ins_place[q][k][n].m,
					ins_place[q][k][n].d->name(), n );
		create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, c_place );
		create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, ins_place[q][k][n].place );
		create_arc( MEASUREMENT_LAYER, TO_PLACE, c_trans, ins_place[q][k][n].place );
	    }
	}
    }
}


/*
 * Open arrivals are converted to a psuedo reference task with one
 * customer.  The reference task makes asycnhronous calls to the
 * server. We need to check the resutls to ensure that the throughput
 * is correct.
 */

void
Model::build_open_arrivals ()
{
    const unsigned n_entries = ::entry.size();
    for ( unsigned int e = 0; e < n_entries; ++e  ) {
	Entry * dst_entry = ::entry[e];
	if ( dst_entry->task()->type() == OPEN_SRC ) continue;

	LQIO::DOM::Entry * dst_dom = dst_entry->get_dom();
	if ( !dst_dom->hasOpenArrivalRate() ) continue;

	std::string buf = "OT";
	buf += dst_entry->name();
	OpenTask * a_task = new OpenTask( _document, buf, dst_entry );
	::task.push_back( a_task );

	buf = "OE";
	buf += dst_entry->name();
	LQIO::DOM::Entry * pseudo = new LQIO::DOM::Entry( dst_dom->getDocument(), buf.c_str(), NULL );
	pseudo->setEntryType( LQIO::DOM::Entry::ENTRY_STANDARD );
	LQIO::DOM::Phase* phase = pseudo->getPhase(1);
	phase->setServiceTimeValue( 1.0 / dst_dom->getOpenArrivalRateValue() );
	phase->setName(buf);

	Entry * an_entry = new Entry( pseudo, a_task );
	::entry.push_back( an_entry );		// Do this because add_call will try to find it.
	a_task->entries.push_back( an_entry );

	LQIO::DOM::ConstantExternalVariable  * var  = new LQIO::DOM::ConstantExternalVariable( 1 );
	LQIO::DOM::Call * call = new LQIO::DOM::Call( dst_dom->getDocument(), LQIO::DOM::Call::QUASI_SEND_NO_REPLY, phase, dst_dom, 1, var, 0 );
	phase->addCall( call );
	an_entry->phase[1].add_call( call );
    }
}

/*----------------------------------------------------------------------*/
/*			   RATE PARAMETERS				*/
/*----------------------------------------------------------------------*/

/*
 * Save the rate parameters.
 */

void
Model::trans_rpar()
{
    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {

	/* Service time at entries. */

	for ( vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {
	    if ( !(*e)->is_regular_entry() ) continue;
	    for ( unsigned p = 1; p <= (*e)->n_phases(); p++) {
		(*e)->phase[p].create_spar();
	    }
	    Phase::inc_par_offsets();
	}
		
	for ( vector<Activity *>::const_iterator a = (*t)->activities.begin(); a != (*t)->activities.end(); ++a ) {
	    (*a)->create_spar();
	}
		
	Phase::inc_par_offsets();
    }

    /* Call rates. */

    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {

	/* Service time at entries. */

	for ( vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {

	    for ( vector<Entry *>::const_iterator d = ::entry.begin(); d != ::entry.end(); ++d ) {
		for ( unsigned int p = 1; p <= (*e)->n_phases(); p++ ) {
		    (*e)->phase[p].create_ypar( *d );
		}
	    }
	}

	for ( vector<Activity *>::const_iterator a = (*t)->activities.begin(); a != (*t)->activities.end(); ++a ) {
	    for ( vector<Entry *>::const_iterator d = ::entry.begin(); d != ::entry.end(); ++d ) {
		(*a)->create_ypar( *d );
	    }

	    Phase::inc_par_offsets();
	}
    }
}



/*
 * Generate the result lists.
 */

void
Model::trans_res ()
{
    char name_str[BUFSIZ];

    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	if ( (*t)->type() != REF_TASK ) continue;
			
	for ( vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {

	    name_str[0] = '\0';
	    if ( (*e)->is_regular_entry() ) {
		for ( unsigned int p = 1; p <= (*e)->n_phases(); p++) {
		    if ( (*e)->phase[p].s() ) {	/* BUG_21 */
			sprintf( name_str, "S%s00", (*e)->phase[p].name() );
			break;			/* Stop on first found */
		    }
		}
	    } else {
		sprintf( name_str, "S%s00", (*e)->start_activity()->name() );
	    }
	    if ( name_str[0] ) {
		char * name = strdup32x( name_str );
		(void) create_res( Phase::__parameter_x, Phase::__parameter_y, "E%s", "E{#%s};", name );
		free( name );
		Phase::inc_par_offsets();
	    }
	}
    }

    for ( vector<Processor *>::const_iterator p = ::processor.begin(); p != ::processor.end(); ++p ) {
	sprintf( name_str, "P%s", (*p)->name() );
	char * name = strdup32x( name_str );
	(void) create_res( Phase::__parameter_x, Phase::__parameter_y, "U%s", "P{#%s=0};", name );
	free( name );
	Phase::inc_par_offsets();
    }
}
	
/*----------------------------------------------------------------------*/
/* Output processing.							*/
/*----------------------------------------------------------------------*/

void
Model::print() const
{
    /* override is true for '-p -o filename.out filename.in' == '-p filename.in' */

    const std::string directory_name = createDirectory();
    const string suffix = _document->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix : "";

    /* override is true for '-p -o filename.out filename.in' == '-p filename.in' */
 
    bool override = false;
    if ( has_output_file_name() && LQIO::Filename::isRegularFile( _output_file_name.c_str() ) != 0 ) {
	LQIO::Filename filename( _input_file_name.c_str(), rtf_flag ? "rtf" : "out" );
	override = filename() == _output_file_name;
    }

    /* SRVN type statistics */

    if ( override || ((!has_output_file_name() || directory_name.size() > 0 ) && strcmp( LQIO::input_file_name, "-" ) != 0) ) {
	ofstream output;

	if ( _document->isXMLDOMPresent() || xml_flag ) {
	    LQIO::Filename filename( _input_file_name.c_str(), "lqxo", directory_name.c_str(), suffix.c_str() );
	    filename.backup();
	    _document->serializeDOM( filename(), true );
	}

	/* Parseable output. */

	if ( parse_flag ) {
	    LQIO::Filename filename( _input_file_name.c_str(), "p", directory_name.c_str(), suffix.c_str() );
	    output.open( filename(), ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::PARSEABLE_OUTPUT );
	    }
	    output.close();
	}

	/* Regular output */

	LQIO::Filename filename( _input_file_name.c_str(), rtf_flag ? "rtf" : "out", directory_name.c_str(), suffix.c_str() );
	output.open( filename(), ios::out );
	if ( !output ) {
	    solution_error( LQIO::ERR_CANT_OPEN_FILE, filename(), strerror( errno ) );
	} else {
	    _document->print( output, rtf_flag ? LQIO::DOM::Document::RTF_OUTPUT : LQIO::DOM::Document::TEXT_OUTPUT );
	    if ( inservice_match_pattern != 0 ) {
		print_inservice_probability( output );
	    }
	}
	output.close();

    } else if ( _output_file_name == "-" || _input_file_name == "-" ) {

	if ( parse_flag ) {
	    _document->print( cout, LQIO::DOM::Document::PARSEABLE_OUTPUT );
	} else if ( rtf_flag ) {
	    _document->print( cout, rtf_flag ? LQIO::DOM::Document::RTF_OUTPUT : LQIO::DOM::Document::TEXT_OUTPUT );
	    if ( inservice_match_pattern != 0 ) {
		print_inservice_probability( cout );
	    }
	}

    } else {

	/* Do not map filename. */

	LQIO::Filename::backup( _output_file_name.c_str() );

	if ( xml_flag ) {
	    _document->serializeDOM( _output_file_name.c_str(), true );
	} else {
	    ofstream output;
	    output.open( _output_file_name.c_str(), ios::out );
	    if ( !output ) {
	        solution_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
	    } else if ( parse_flag ) {
	        _document->print( output, LQIO::DOM::Document::PARSEABLE_OUTPUT );
	    } else {
	        _document->print( output, rtf_flag ? LQIO::DOM::Document::RTF_OUTPUT : LQIO::DOM::Document::TEXT_OUTPUT );
		if ( inservice_match_pattern != 0 ) {
		    print_inservice_probability( output );
		}
	    }
	    output.close();
	}
    }

    cout.flush();
    cerr.flush();
}


/*
 * Go through data structures and insert results into the DOM
 */

void
Model::insert_DOM_results( const bool valid, const solution_stats_t& stats )
{
    /* Insert general information about simulation run */

    _document->setResultConvergenceValue(stats.precision).setResultValid(valid);

    std::stringstream buf;
    buf << "Tangible: " << stats.tangible << ", Vanishing: " << stats.vanishing;
    _document->setExtraComment( buf.str() );

#if HAVE_SYS_TIMES_H
    tms_t time_buf;

    clock_t stop_time = times( &time_buf );
    _document->setResultUserTime( time_buf.tms_utime );
    _document->setResultSysTime( time_buf.tms_stime );
#else
    clock_t stop_time = time( NULL );
#endif
    _document->setResultElapsedTime(stop_time - __start_time);

#if defined(HAVE_UNAME)
    struct utsname uu;		/* Get system triva. */

    uname( &uu );
    buf.seekp(0);
    buf << uu.nodename << " " << uu.sysname << " " << uu.release;
    _document->setResultPlatformInformation( buf.str() );
#endif
    buf.seekp(0);
    buf << io_vars.lq_toolname << " " << VERSION;
    _document->setResultSolverInformation( buf.str() );

    for ( vector<Task *>::const_iterator t = ::task.begin(); t != ::task.end(); ++t ) {
	(*t)->insert_DOM_results();
    }
    for ( vector<Processor *>::const_iterator p = ::processor.begin(); p != ::processor.end(); ++p ) {
	(*p)->insert_DOM_results();
    }
}



/*----------------------------------------------------------------------*/
/* 			  Utility routines.				*/
/*----------------------------------------------------------------------*/

/*
 * Create a directory (if needed)
 */

string
Model::createDirectory() const
{
    string directory_name;
    if ( has_output_file_name() && LQIO::Filename::isDirectory( _output_file_name.c_str() ) > 0 ) {
	directory_name = _output_file_name;
    }

    if ( _document->getResultInvocationNumber() > 0 ) {
	if ( directory_name.size() == 0 ) {
	    /* We need to create a directory to store output. */
	    LQIO::Filename filename( has_output_file_name() ? _output_file_name.c_str() : _input_file_name.c_str(), "d" );		/* Get the base file name */
	    directory_name = filename();
	}
    }

    if ( directory_name.size() > 0 ) {
	int rc = access( directory_name.c_str(), R_OK|W_OK|X_OK );
	if ( rc < 0 ) {
	    if ( errno == ENOENT ) {
#if defined(WINNT)
		rc = mkdir( directory_name.c_str() );
#else
		rc = mkdir( directory_name.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH );
#endif
	    }
	    if ( rc < 0 ) {
		solution_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name.c_str(), strerror( errno ) );
	    }
	}
    }
    return directory_name;
}

/*
 * Print out inservice probabilities Pr{InService(a,p_a,b,c,d,p_d)}.
 */

void
Model::print_inservice_probability( ostream& output ) const
{
    std::_Ios_Fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
    output << endl << endl;
    if ( uncondition_flag ) {
	output << "UNconditioned in-";
    } else {
	output << "In-";
    }
    output << "service probabilities (p->'a'):" << endl << endl;
    for ( unsigned int i = 0; i < 4; ++i ) {
	output << "Entry " << "abcd"[i] << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-7) << " ";
    }
    output << "p_d ";
    for ( unsigned int p = 1; p <= n_phases(); ++p ) {
	output << "Phase " << p << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-7) << " ";
    }
    if ( uncondition_flag ) {
	output << "Total";
    } else {
	output << "Mean";
    }
    output << endl;
	
    for ( vector<Task *>::const_iterator t_i = ::task.begin(); t_i != ::task.end(); ++t_i ) {
	for ( vector<Task *>::const_iterator t_j = ::task.begin(); t_j != ::task.end(); ++t_j ) {
	    if ( !(*t_j)->inservice_flag() ) continue;	/* Only do ones we have! */

	    unsigned int count = 0;
	    double col_sum[DIMPH+1];
	    double tot_tput[DIMPH+1];	/* Throughput of this task.	*/

	    for ( unsigned int p = 0; p <= DIMPH; ++p ) {
		col_sum[p] = 0.0;
	    }

	    /* Over all entries of task i and j... */
			
	    if ( uncondition_flag ) {
		(*t_i)->get_total_throughput( *t_j, tot_tput );
	    }
			
	    for ( vector<Entry *>::const_iterator e_i = (*t_i)->entries.begin(); e_i != (*t_i)->entries.end(); ++e_i ) {
		Entry * a = *e_i;

		for ( vector<Entry *>::const_iterator e_j = (*t_j)->entries.begin(); e_j != (*t_j)->entries.end(); ++e_j ) {
		    Entry * b = *e_j;

		    if ( a->yy(b) == 0.0 ) continue;

		    if ( !uncondition_flag ) {
			tot_tput[0] = 0.0;
			for ( unsigned int p = 1; p <= a->n_phases(); ++p ) {
			    const Phase * phase = &a->phase[p];
			    tot_tput[p]  = phase->lambda( 0, b, phase );
			    tot_tput[0] += tot_tput[p];
			}
		    }
					
		    count += print_inservice_cd( output, a, b, *t_j, tot_tput, col_sum );
		} /* b */
	    } /* a */

	    if ( count > 1 && uncondition_flag ) {
		double row_sum	= 0.0;
				
		output << setw(LQIO::SRVN::ObjectOutput::__maxStrLen) << "Task total"
		       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << (*t_i)->name() << " "
		       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << (*t_j)->name() << " "
		       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen) << " "
		       << "Sum ";
		for ( unsigned int p = 1; p <= (*t_i)->n_phases(); ++p ) {
		    output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum[p] << " ";
		    row_sum += col_sum[p];
		}
		output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << row_sum;
	    }
	    if ( count > 0 ) {
		output << endl;
	    }
	}
    }
    output << endl;
    output.flags(oldFlags);
}




/*
 * Print out the results for a, p_a, b, c, d, p_d.
 */

unsigned
Model::print_inservice_cd( ostream& output, const Entry * a, const Entry * b, const Task * j, 
			   double tot_tput[], double col_sum[DIMPH+1] ) const
{
    unsigned count    = 0;	/* Count of all c,d		*/
    double col_sum_cd[DIMPH+1];	/* sum over all c,d for a,b	*/
	
    for ( unsigned int p = 0; p <= DIMPH; ++p ) {
	col_sum_cd[p] = 0.0;
    }
	
    /* Now find prob of token following c->d path instead of a->b OT or idle */
	
    for ( vector<Task *>::const_iterator t_i = ::task.begin(); t_i != ::task.end(); ++t_i ) {
	if ( *t_i == j ) continue;

	for ( vector<Entry *>::const_iterator e_i = (*t_i)->entries.begin(); e_i != (*t_i)->entries.end(); ++e_i ) {
	    Entry * c = *e_i;

	    const bool overtaking = a->task() == c->task();
			
	    for ( vector<Entry *>::const_iterator e_j = j->entries.begin(); e_j != j->entries.end(); ++e_j ) {
		Entry * d = (*e_j);		/* Called entry index.		*/
		unsigned count_Pd = 0;		/* Count of phases.		*/
		double col_sum_Pd[DIMPH+1];	/* sum over phase of entry d	*/
		double tput[DIMPH+1];

		if ( c->yy(d) == 0.0 ) continue;
				
		for ( unsigned int p = 0; p <= DIMPH; ++p ) {
		    col_sum_Pd[p] = 0.0;
		}
				
		for ( unsigned int pd = (overtaking ? 2 : 1); pd <= j->n_phases(); pd++ ) {
		    for ( unsigned int pa = 1; pa <= DIMPH; ++pa ) {
			tput[pa] = 0.0;
		    }
		    count_Pd += 1;
					
		    for ( unsigned int pc = 1; pc <= c->n_phases(); ++pc ) {
			if ( c->phase[pc].y(d) == 0.0 ) continue;
			for ( unsigned int pa = 1; pa <= a->n_phases(); ++pa ) {
			    if ( a->phase[pa].y(b) == 0.0 ) continue;
			    tput[pa] += get_tput( IMMEDIATE, "ArX%s%d%s%d%s%d%s%d",
						  a->phase[pa].name(), 0,
						  b->name(), 0,
						  c->phase[pc].name(), 0,
						  d->phase[pd].name(), 0 );
			}
		    }

		    if ( count_Pd == 1 ) {
			output << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << a->name() << " "
			       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << b->name() << " " 
			       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << c->name() << " "
			       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << d->name() << " ";
		    } else {
			output << setw(LQIO::SRVN::ObjectOutput::__maxStrLen*4) << " ";
		    }
		    output << " " << pd << "  ";

		    tput[0] = 0.0;
		    double prob = 0.0;
		    for ( unsigned int pa = 1; pa <= n_phases(); ++pa ) {
			tput[0] += tput[pa];
			if ( uncondition_flag ) {
			    prob = tput[pa] / tot_tput[0];
			} else if ( tot_tput[pa] > 0.0 ) { 
			    prob = tput[pa] / tot_tput[pa];
			} else {
			    prob = 0.0;
			}
			output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << prob << " ";
			col_sum[pa]    += prob;
			col_sum_Pd[pa] += prob;
			col_sum_cd[pa] += prob;
		    }
					
		    prob = tot_tput[0] > 0.0 ? tput[0] / tot_tput[0] : 0.0;
		    if ( overtaking ) {
			output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << prob << " OT" << endl;
		    } else {
			output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << prob << endl;
		    }
		    col_sum[0]    += prob;
		    col_sum_Pd[0] += prob;
		    col_sum_cd[0] += prob;
		} /* p_d */

		if ( count_Pd >= 2 ) {
		    output << setw(LQIO::SRVN::ObjectOutput::__maxStrLen*4) << " " << "Sum ";
		    for ( unsigned int pa = 1; pa <= n_phases(); ++pa ) {
			output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_Pd[pa] << " ";
		    }
		    output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_Pd[0] << (overtaking ? " OT" : "") << endl;
		}
		count += 1;
				
	    } /* d */
	} /* c */
    } /* i */

    /* Total over all c,d for a,b */

    if ( count >= 2 ) {
	output << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << a->name() << " "
	       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << b->name() << " "
	       << setw(LQIO::SRVN::ObjectOutput::__maxStrLen*2) << " " << "Sum ";
	for ( unsigned int pa = 1; pa <= n_phases(); ++pa ) {
	    output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_cd[pa] << " ";
	}
	output << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_cd[0] << endl << endl;
    }
	
    return count;
}
