/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* June 2010.								*/
/************************************************************************/

/*
 * $Id: model.cc 17069 2024-02-27 23:16:21Z greg $
 *
 * Load the SRVN model.
 */

#include "petrisrvn.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_bindings.h>
#include <lqio/dom_entry.h>
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <lqio/input.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
#include <wspnlib/global.h>
#include <wspnlib/wspn.h>
#include "actlist.h"
#include "entry.h"
#include "errmsg.h"
#include "makeobj.h"
#include "model.h"
#include "phase.h"
#include "pragma.h"
#include "processor.h"
#include "results.h"
#include "runlqx.h"
#include "task.h"

#if HAVE_SYS_TIMES_H
typedef struct tms tms_t;
#else
typedef double tms_t;
#endif

bool Model::__forwarding_present;
bool Model::__open_class_error;
LQIO::DOM::CPUTime Model::__start_time;

/* define	UNCONDITIONAL_PROBS */
/* define DERIVE_UTIL */
 
/* ------------------------------------------------------------------------ */
/* */
/* ------------------------------------------------------------------------ */

Model::Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, LQIO::DOM::Document::OutputFormat output_format )
    : _document( document ),
      _input_file_name( input_file_name ),
      _output_file_name( output_file_name ),
      _output_format( output_format ),
      _n_phases(0)
{
}


/*
 * Delete the model.
 */

Model::~Model()
{
    std::for_each( __processor.begin(), __processor.end(), Model::Delete<Processor *> );
    __processor.clear();

    std::for_each( __task.begin(), __task.end(), Model::Delete<Task *> );
    __task.clear();
    __entry.clear();

    for ( int i = 0; i <= layer_num; ++i) {
	if ( layer_name[i] ) {
	    free( layer_name[i] );
	    layer_name[i] = nullptr;
	}
    }
    layer_num = 0;

    free_group_store();
}



/*
 * Process input and save.
 */

int
Model::solve( solve_using solver_function, const std::string& inputFileName, LQIO::DOM::Document::InputFormat inputFormat, const std::string& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat, const LQIO::DOM::Pragma& pragmas )
{
    LQIO::DOM::Document* document = Model::load( inputFileName, inputFormat );

    /* Make sure we got a document */
    if ( document == nullptr || LQIO::io_vars.anError() ) return FILEIO_ERROR;
    document->setResultDescription();			/* Wipe out any description and replace with generic. */

    document->mergePragmas( pragmas.getList() );	/* Save pragmas -- prepare will process */

    switch ( document->getInputFormat() ) {
    case LQIO::DOM::Document::InputFormat::JSON:
    case LQIO::DOM::Document::InputFormat::XML:
	if ( LQIO::Spex::__no_header ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --no-header is ignored for " << inputFileName << "." << std::endl;
	}
	if ( LQIO::Spex::__print_comment ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --print-comment is ignored for " << inputFileName << "." << std::endl;
	}
	break;
    default:;
    }

    /* declare Model * at this scope but don't instantiate due to problems with LQX programs and registering external symbols*/
    Model model( document, inputFileName,  outputFileName, outputFormat );
    if ( !model.construct() ) return FILEIO_ERROR;

    int status = 0;
    LQX::Program * program = document->getLQXProgram();
    if ( program != nullptr ) {
	/* We can simply run if there's no control program */
	if (program == nullptr) {
	    LQIO::runtime_error( LQIO::ERR_LQX_COMPILATION, inputFileName.c_str() );
	    status = FILEIO_ERROR;
	} else {
	    document->registerExternalSymbolsWithProgram(program);
	    program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, solver_function, &model));
	    LQIO::RegisterBindings(program->getEnvironment(), document);

	    FILE * output = 0;
	    if ( outputFileName.size() > 0 && outputFileName != "-" && LQIO::Filename::isRegularFile(outputFileName) ) {
		output = fopen( outputFileName.c_str(), "w" );
		if ( !output ) {
		    LQIO::runtime_error( LQIO::ERR_CANT_OPEN_FILE, outputFileName.c_str(), strerror( errno ) );
		    status = FILEIO_ERROR;
		} else {
		    program->getEnvironment()->setDefaultOutput( output );	/* Default is stdout */
		}
	    }

	    if ( status == 0 ) {
		/* Invoke the LQX program itself */
		if ( !program->invoke() ) {
		    LQIO::runtime_error( LQIO::ERR_LQX_EXECUTION, inputFileName.c_str() );
		    status = FILEIO_ERROR;
		} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
		    /* There was no call to solve the LQX */
		    LQIO::runtime_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, inputFileName.c_str() );
		    std::vector<LQX::SymbolAutoRef> args;
		    program->getEnvironment()->invokeGlobalMethod("solve", &args);
		}
	    }
	    if ( output ) {
		fclose( output );
	    }
	}
	delete program;

    } else {
	/* There is no control flow program, check for $-variables */
	if (document->getSymbolExternalVariableCount() != 0) {
	    LQIO::runtime_error( LQIO::ERR_LQX_VARIABLE_RESOLUTION, inputFileName.c_str() );
	    status = FILEIO_ERROR;
	} else {
	    model.recalculateDynamicValues( document );
	    try {
		if ( !model.compute() ) {
		    status = FILEIO_ERROR;		/* Simply invoke the solver for the current DOM state */
		}
	    }
	    catch ( const std::runtime_error & error ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": runtime error - " << error.what() << std::endl;
		LQIO::io_vars.error_count += 1;
		status = EXCEPTION_EXIT;
	    }
	    catch ( const std::domain_error& error ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": domain error - " << error.what() << std::endl;
		LQIO::io_vars.error_count += 1;
		status = INVALID_INPUT;
	    }
	}
    }
    return status;
}



LQIO::DOM::Document*
Model::load( const std::string& input_filename, LQIO::DOM::Document::InputFormat input_format )
{
    Model::__start_time.init();

    if ( verbose_flag ) {
	std::cerr << "Load: " << input_filename << "..." << std::endl;
    }

    LQIO::io_vars.reset();

    __forwarding_present     = false;
    __open_class_error	     = false;

    /*
     * Initialize everything that needs it before parsing
     */

    Entry::__next_entry_id   = 1;
    Activity::actConnections.clear();
    Activity::domToNative.clear();
    netobj_name_table.clear();

    /*
     * Read input file and parse it.
     */

    unsigned errorCode = 0;
    return LQIO::DOM::Document::load(input_filename, input_format, errorCode, false);
}


/*
 *	Called from the parser to set important modelling parameters.
 *	It can also check validity of same if so desired.
 */

/*ARGSUSED*/
void
Model::set_comment()
{
    struct com_object * buf	= (struct com_object *)malloc( CMMOBJ_SIZE );
    const std::string& comment = _document->getModelComment();
    const char * p = comment.c_str();

    netobj->comment = buf;

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

    buf->next = nullptr;
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
	std::cerr << "Create: " << _input_file_name << "..." << std::endl;
    }

    Pragma::set( _document->getPragmaList() );
    LQIO::Spex::__no_header = !Pragma::__pragmas->spex_header();
    LQIO::Spex::__print_comment = Pragma::__pragmas->spex_comment();
    LQIO::io_vars.severity_level = Pragma::__pragmas->severity_level();

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */

    const std::map<std::string,LQIO::DOM::Processor*>& procList = _document->getProcessors();
    for_each( procList.begin(), procList.end(), Processor::create );

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */

    /* In the DOM, tasks have entries, but here entries need to go first */
    const std::map<std::string,LQIO::DOM::Task*>& taskList = _document->getTasks();

    /* Add all of the tasks we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator t = taskList.begin(); t != taskList.end(); ++t ) {
	const LQIO::DOM::Task* task = t->second;
	/* Now we can go ahead and add the task */
	Task* newTask = Task::create(task);

	std::vector<LQIO::DOM::Entry*> activityEntries;

	/* Add the entries so we can reverse them */
	for ( std::vector<LQIO::DOM::Entry*>::const_iterator entry = task->getEntryList().begin(); entry != task->getEntryList().end(); ++entry ) {
	    newTask->entries.push_back( Entry::create( *entry, newTask ) );
	    if ((*entry)->getStartActivity() != nullptr) {
		activityEntries.push_back(*entry);
	    }
	}

	/* Add activities for the task (all of them) */
	const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
	for (std::map<std::string,LQIO::DOM::Activity*>::const_iterator activity = activities.begin(); activity != activities.end(); ++activity) {
	    newTask->add_activity(activity->second);
	}

	/* Set all the start activities */
	for (std::vector<LQIO::DOM::Entry*>::iterator entry = activityEntries.begin(); entry != activityEntries.end(); ++entry) {
	    newTask->set_start_activity(*entry);
	}
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 3: Add Calls/Phase Parameters] */

    /* Add all of the calls for all phases to the system */
    /* We use this to add all calls */
    const std::map<std::string,LQIO::DOM::Entry*>& allEntries = _document->getEntries();
    for ( std::map<std::string,LQIO::DOM::Entry*>::const_iterator nextEntry = allEntries.begin(); nextEntry != allEntries.end(); ++nextEntry ) {
	LQIO::DOM::Entry* entry = nextEntry->second;
	Entry* newEntry = Entry::find(entry->getName());
	if ( newEntry == nullptr ) continue;

	/* Go over all of the entry's phases and add the calls */
	for (unsigned p = 1; p <= entry->getMaximumPhase(); ++p) {
	    LQIO::DOM::Phase* phase = entry->getPhase(p);
	    /* Add all of the calls to the system */

	    const std::vector<LQIO::DOM::Call*>& originatingCalls = phase->getCalls();
	    for (std::vector<LQIO::DOM::Call*>::const_iterator call = originatingCalls.begin(); call != originatingCalls.end(); ++call) {
		newEntry->add_call(p, *call);			/* Add the call to the system */
	    }
	}

	/* Add in all of the P(frwd) calls */
	const std::vector<LQIO::DOM::Call*>& forwarding = entry->getForwarding();
	for ( std::vector<LQIO::DOM::Call*>::const_iterator call = forwarding.begin(); call != forwarding.end(); ++call ) {
	    Entry::add_fwd_call(*call);
	}
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 4: Add Calls/Lists for Activities] */

    /* Go back and add all of the lists and calls now that activities all exist */
    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {
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

    /* Add open arrivals. */

    build_open_arrivals();

    /* Use the generated connections list to finish up */
    Activity::complete_activity_connections();

    return !LQIO::io_vars.anError();
}


/*
 * Dynamic Updates / Late Finalization
 * In order to integrate LQX's support for model changes we need to
 * have a way of re-calculating what used to be static for all
 * dynamically editable values.
 *
 * For petrisrvn, the model is rebuilt from scratch.  However, the value
 * for the service time for the pseudo open arrival source entry has to be
 * set.
 */

void
Model::recalculateDynamicValues( const LQIO::DOM::Document* document )
{
    setModelParameters(document);

    /* Find the pseudo phases for open arrivals and set the service time */

    const unsigned n_entries = document->getNumberOfEntries();
    for ( unsigned int e = 0; e < n_entries; ++e  ) {
	LQIO::DOM::Phase * phase = __entry[e]->open_arrival_phase();
	if ( phase == nullptr ) continue;
	const LQIO::DOM::Entry * entry = __entry[e]->get_dom();

	try {
	    phase->setServiceTimeValue( 1.0 / entry->getOpenArrivalRateValue() );
	}
	catch ( const std::domain_error& e ) {
	    entry->runtime_error( LQIO::ERR_INVALID_PARAMETER, "open arrival rate", "entry", e.what() );
	    throw std::domain_error( std::string( "invalid parameter: " ) + e.what() );
	}
    }
}


/*
 * Only used for SPEX stuff.
 */

void Model::setModelParameters( const LQIO::DOM::Document * document )
{
    if ( Pragma::__pragmas->spex_convergence() > 0 ) {
	const_cast<LQIO::DOM::Document *>(document)->setSpexConvergence( new LQIO::DOM::ConstantExternalVariable( Pragma::__pragmas->spex_convergence() ) );
    }
    if ( Pragma::__pragmas->spex_iteration_limit() > 0 ) {
	const_cast<LQIO::DOM::Document *>(document)->setSpexIterationLimit( new LQIO::DOM::ConstantExternalVariable( Pragma::__pragmas->spex_iteration_limit() ) );
    }
    if ( Pragma::__pragmas->spex_underrelaxation() > 0 ) {
	const_cast<LQIO::DOM::Document *>(document)->setSpexUnderrelaxation( new LQIO::DOM::ConstantExternalVariable( Pragma::__pragmas->spex_underrelaxation() ) );
    }
}


void Model::clear()
{
    Model::__forwarding_present = false;
    Model::__open_class_error = false;
    Phase::__parameter_x   = 0.5;
    Phase::__parameter_y   = 0.5;
    Task::__server_x_offset = 1;
    Task::__client_x_offset = 1;
    Processor::__x_offset   = 1;

    std::for_each( ::__processor.begin(), ::__processor.end(), std::mem_fn( &Processor::clear ) );
    std::for_each( ::__task.begin(), ::__task.end(), std::mem_fn( &Task::clear ) );
    std::for_each( ::__entry.begin(), ::__entry.end(), std::mem_fn( &Entry::clear ) );
}


/*
 * Fold, mutilate, and spindle...
 */

bool
Model::transform()
{
    unsigned max_width = 0;

    std::for_each( ::__processor.begin(), ::__processor.end(), std::mem_fn( &Processor::initialize ) );
    std::for_each( ::__entry.begin(), ::__entry.end(), std::mem_fn( &Entry::initialize ) );
    std::for_each( ::__task.begin(), ::__task.end(), std::mem_fn( &Task::initialize ) );

    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {
	set_n_phases((*t)->n_phases());
    }

    if ( LQIO::io_vars.anError() ) return false;

    const unsigned int max_queue_length = set_queue_length();		/* Do after forwarding */
    const unsigned int max_proc_queue_length = Processor::set_queue_length();

    if ( !init_net() ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot initialize net " << _input_file_name << "!" << std::endl;
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
    for ( std::vector<Entry *>::const_iterator e = ::__entry.begin(); e != ::__entry.end(); ++e, ++i ) {
	layer_name[ENTRY_LAYER_NUM+i] = strdup( (*e)->name() );
    }
    layer_num = ENTRY_LAYER_NUM+::__entry.size()-1;

    /*
     * Set comment
     */

    set_comment();

    /*
     * Compute offsets.
     */

    if ( Pragma::__pragmas->task_scheduling() == SCHEDULE_RAND ) {
	Task::__queue_y_offset  = -3.;
    } else {
	Task::__queue_y_offset  = -static_cast<double>(max_queue_length+2);
    }

    Task::__server_y_offset = Place::SERVER_Y_OFFSET - Task::__queue_y_offset;

    if ( __forwarding_present ) {
	Task::__server_y_offset += 1.0;
    }

    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {
	if ( (*t)->type() != Task::Type::REF_TASK || (*t)->n_activities() == 0 ) continue;

	for ( std::vector<ActivityList *>::const_iterator l = (*t)->act_lists.begin(); l != (*t)->act_lists.end(); ++l ) {
	    if ( ( (*l)->type() == ActivityList::Type::AND_FORK || (*l)->type() == ActivityList::Type::OR_FORK ) && (*l)->n_acts() > max_width ) {
		max_width = (*l)->n_acts();
	    } else if ( (*l)->type() == ActivityList::Type::LOOP && max_width < 2 ) {
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

    for ( std::vector<Processor *>::const_iterator p = ::__processor.begin(); p != ::__processor.end(); ++p ) {
	(*p)->transmorgrify( max_proc_queue_length );
    }

    /* Build tasks. */

    std::for_each( ::__task.begin(), ::__task.end(), std::mem_fn( &Task::transmorgrify ) );

    std::for_each( ::__entry.begin(), ::__entry.end(), std::mem_fn( &Entry::create_forwarding_gspn ) );

    /* Build queues twixt tasks. */

    make_queues();

    /* Format for solver */

    groupize();
    shift_rpars( Task::__client_x_offset, Phase::__parameter_y );

    return !LQIO::io_vars.anError();
}


unsigned int
Model::set_queue_length()  const
{
    unsigned max_queue_length = 0;

    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {
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
    std::for_each( __processor.begin(), __processor.end(), std::mem_fn( &Processor::remove_netobj ) );
    std::for_each( ::__task.begin(), ::__task.end(), std::mem_fn( &Task::remove_netobj ) );

    free_netobj( netobj );
    netobj = (struct net_object *)0;
}

/* -------------------------------------------------------------------- */
/* Solve.								*/
/* -------------------------------------------------------------------- */

/*
 * Solve the model.
 */

bool
Model::compute()
{
    bool rc = true;

    clear();
    if ( !transform() ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Input model " << _input_file_name << " was not transformed successfully." << std::endl;
	return false;
    }

    const std::string suffix = _document->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix : "";
    const LQIO::Filename netname( _input_file_name, "", "", suffix );

    if ( !reload_net_flag ) {
	if ( verbose_flag ) {
	    std::cerr << "Solve: " << _input_file_name << "..." << std::endl;
	}

	save_net_files( LQIO::io_vars.toolname(), netname().c_str() );

	if ( no_execute_flag ) {
	    return true;
	} else if ( trace_flag ) {
	    if ( !solve2( netname().c_str(), 2, SOLVE_STEADY_STATE ) ) {	/* output to stderr */
		rc = false;
	    }
	} else {
	    int null_fd = open( "/dev/null", O_RDWR );
	    if ( !solve2( netname().c_str(), null_fd, SOLVE_STEADY_STATE ) ) {
		rc = false;
	    }
	    close( null_fd );
	}
    }

    if ( verbose_flag ) {
	std::cerr << "Done: " << std::endl;
    }

    if ( rc == true ) {
	solution_stats_t stats;
	if ( !solution_stats( &stats.tangible, &stats.vanishing, &stats.precision )
	     || !collect_res( FALSE, LQIO::io_vars.toolname() ) ) {
	    (void) fprintf( stderr, "%s: Cannot read results for %s\n", LQIO::io_vars.toolname(), netname().c_str() );
	    rc = false;
	} else {
	    std::for_each( ::__task.begin(), __task.end(), std::mem_fn( &Task::get_results ) );	/* Read net to get tokens. */

	    if ( stats.precision >= 0.01 || __open_class_error || LQIO::io_vars.anError() ) {
		rc = false;
	    }
	    insert_DOM_results( rc == true, stats );	/* Save results */

	    _document->print( _output_file_name, suffix, _output_format, rtf_flag );

	    if ( inservice_match_pattern != nullptr ) {
		print_inservice_probability( std::cout );
	    }
	    if ( verbose_flag ) {
		std::cerr << stats;
	    }
	}
    }

    /* Clean up in preparation for another run.	*/

    if ( !keep_flag ) {
	remove_result_files( netname().c_str() );
    }

    remove_netobj();
    return rc;
}



/*
 * Read result files only.  LQX print uses these results.
 */

bool
Model::reload()
{
    /* Default mapping */

    LQIO::Filename directory_name( has_output_file_name() ? _output_file_name : _input_file_name, "d" );		/* Get the base file name */

    if ( access( directory_name().c_str(), R_OK|W_OK|X_OK ) < 0 ) {
	runtime_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name().c_str(), strerror( errno ) );
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    }

    unsigned int errorCode;
    if ( !_document->loadResults( directory_name(), _input_file_name, SolverInterface::Solve::customSuffix, _output_format, errorCode ) ) {
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    } else {
	return true;
    }
}


/*
 * Read the result files only.  If not found, or invalid, solve it the hard way.
 */

bool
Model::restart()
{
    try {
	if ( reload() && _document->getResultValid() ) return true;
    }
    catch ( const LQX::RuntimeException& ) {
	/* Ignore error and fall through */
    }
    return compute();
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
    /* Make queues */

    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {
	if ( (*t)->is_client() ) continue;  	/* Skip reference tasks. */

	double x_pos	= (*t)->get_x_pos() - 0.5;
	double y_pos	= (*t)->get_y_pos();
	unsigned ne	= (*t)->n_entries();
	double idle_x;
	unsigned k 	= 0;			/* Queue Kounter	*/
	queue_fnptr queue_func;			/* Local version.	*/
	bool random_queueing = (*t)->is_sync_server() || (*t)->has_random_queueing() || (*t)->type() == Task::Type::SEMAPHORE || (*t)->is_infinite();
	std::vector<Model::inst_arrival> ph2_place;

	/* Override if dest is a join function. */

	if ( random_queueing ) {
	    idle_x = x_pos + 1.0 + 0.5;
	    queue_func = &Model::random_queue;
	} else {
	    idle_x = x_pos + static_cast<double>((*t)->max_queue_length() + 0.5);
	    queue_func = &Model::fifo_queue;
	}

	/* Create and connect all queues for task 'j'. */

	for ( std::vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {
	    for ( std::vector<Task *>::const_iterator i = ::__task.begin(); i != ::__task.end(); ++i ) {
		const unsigned max_m = (*i)->n_customers();

		for ( std::vector<Entry *>::const_iterator d = (*i)->entries.begin(); d != (*i)->entries.end(); ++d ) {
		    if ( (*d)->is_regular_entry() ) {
			for ( unsigned p = 1; p <= (*d)->n_phases(); p++ ) {
			    k = make_queue( x_pos, y_pos, idle_x, &(*d)->phase[p], *e, ne, max_m, k, queue_func, ph2_place );
			}
		    }

		    if ( (*d)->prob_fwd(*e) > 0.0 ) {
			for ( std::vector<Forwarding *>::const_iterator f = (*d)->forwards.begin(); f != (*d)->forwards.end(); ++f ) {
			    k += 1;
			    (this->*queue_func)(X_OFFSET(1,0.0) + k * 0.5, y_pos, idle_x,
						&(*d)->phase[1], 0, *e, (*f)->_root,
						(*f)->_slice_no, (*f)->_m, (*d)->prob_fwd(*e),
						k, false, ph2_place );
			}
		    }
		}

		for ( std::vector<Activity *>::const_iterator a = (*i)->activities.begin(); a != (*i)->activities.end(); ++a ) {
		    k = make_queue( x_pos, y_pos, idle_x, (*a), *e, ne, max_m, k, queue_func, ph2_place );
		}
		if ( random_queueing ) {
		    k += 1;
		}
	    } /* a */

	} /* lj */

	(*t)->set_max_k(k);

	/*
	 * Now adjust debug stuff...
	 */

	if ( (*t)->inservice_flag() ) {

	    /* Be shifty... */

	    for ( unsigned int m = 0; m < (*t)->multiplicity(); ++m ) {
		double m_delta = (double)m / 4.0;
		(*t)->TX[m]->center.x  = IN_TO_PIX( idle_x + m_delta );
		(*t)->TX[m]->center.y += IN_TO_PIX( m_delta );
#if BUG_163
		if ( (*t)->is_sync_server() ) {
		    (*t)->SyX[m]->center.x  = IN_TO_PIX( idle_x + 0.5 + m_delta );
		    (*t)->SyX[m]->center.y += IN_TO_PIX( m_delta );
		}
#endif
	    }
	    k = 0;	/* Used to offset the transition by queue */
	    for ( std::vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {
		for ( std::vector<Task *>::const_iterator i = ::__task.begin(); i != ::__task.end(); ++i ) {
		    for ( std::vector<Entry *>::const_iterator d = (*i)->entries.begin(); d != (*i)->entries.end(); ++d ) {
			if ( (*d)->is_regular_entry() ) {
			    for ( unsigned p = 1; p <= (*d)->n_phases(); p++ ) {
				if ( (*d)->phase[p].y(*e) == 0. ) continue;	/* No call, so skip */
				create_inservice_net( &(*d)->phase[p], *e, k, ph2_place );
				k += 1;
			    }
			}
		    }
		}
	    }
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
		   Entry * b,			/* Destination __entry.		*/
		   const unsigned ne,
		   const unsigned max_m,	/* Multiplicity of Src.		*/
		   unsigned k,			/* an index.			*/
		   queue_fnptr queue_func,
		   std::vector<Model::inst_arrival>& ph2_place )
{
    const double calls = a->y(b) + a->z(b);
    if ( calls == 0.0) return k;	/* No operation. */

    for ( unsigned m = 0; m < max_m; ++m ) {
	bool async_call = a->z(b) > 0 || a->task()->type() == Task::Type::OPEN_SRC;

	if ( a->has_stochastic_calls() ) {
	    k += 1;
	    (this->*queue_func)( X_OFFSET(1,0.0) + k * 0.5, y_pos, idle_x,
				 a, 0, b, a, 0, m, 0.0, k, async_call, ph2_place );
	} else {
	    unsigned s;
	    unsigned off = a->compute_offset( b );			/* Compute offset */
	    for ( s = 0; s < calls; ++s ) {
		k += 1;
		(this->*queue_func)( X_OFFSET(1,0.0) + k * 0.5, y_pos, idle_x,
				     a, s+off, b, a, s+off+1, m, 0.0, k, async_call, ph2_place );
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
		   Entry * b,			/* Destination __entry.		*/
		   const Phase * e,		/* Entry to reply to.		*/
		   const unsigned s_e,		/* Slice to reply to.		*/
		   const unsigned m,		/* Multiplicity of Src.		*/
		   const double prob_fwd,
		   const unsigned k,		/* an index.			*/
		   const bool async_call,
		   std::vector<Model::inst_arrival>& ph2_place )
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
	if ( j->type() == Task::Type::SEMAPHORE ) {
	    abort();	/* Should not use FIFO queue for semaphore task */
	} else {
	    create_arc( layer_mask, TO_TRANS, q_trans, j->TX[b_m] );
	}
	/*- BUG_164 */

	if ( b->is_regular_entry() ) {
	    create_arc( layer_mask, TO_PLACE, q_trans, b->phase[1].ZX[b_m] );
	} else {
	    create_arc( MEASUREMENT_LAYER, TO_PLACE, q_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
	    create_arc( layer_mask, TO_PLACE, q_trans, b->start_activity()->ZX[b_m] );
	}

	struct trans_object * s_trans = 0;
#if BUG_163
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
		create_arc( MEASUREMENT_LAYER, TO_PLACE, s_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
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
	    create_phase_instr_net( idle_x, y_pos, a, m, b, b_m, k, r_trans, q_trans, s_trans, ph2_place );
	}
    } /* b_m */
}



void
Model::random_queue( double x_pos,		/* x coordinate.		*/
		     double y_pos,		/* y coordinate.		*/
		     double idle_x,
		     Phase * a,			/* Source Entry (send from)	*/
		     const unsigned s_a,	/* Slice number of a.		*/
		     Entry * b,			/* Destination __entry.		*/
		     const Phase * e,		/* Entry to reply to.		*/
		     const unsigned s_e,	/* Slice to reply to.		*/
		     const unsigned m,		/* Multiplicity of Src.		*/
		     const double prob_fwd,
		     const unsigned k,		/* an index.			*/
		     const bool async_call,
		     std::vector<Model::inst_arrival>& ph2_place )
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
	if ( j->type() == Task::Type::SEMAPHORE && b->semaphore_type() == LQIO::DOM::Entry::Semaphore::SIGNAL ) {
	    create_arc( layer_mask, TO_TRANS, q_trans, j->LX[b_m] );
	} else {
	    create_arc( layer_mask, TO_TRANS, q_trans, j->TX[b_m] );
	}
	/*- BUG_164 */
	if ( b->is_regular_entry() ) {
	    create_arc( layer_mask, TO_PLACE, q_trans, b->phase[1].ZX[b_m] );
	} else {
	    create_arc( MEASUREMENT_LAYER, TO_PLACE, q_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
	    create_arc( layer_mask, TO_PLACE, q_trans, b->start_activity()->ZX[b_m] );
	}
#if BUG_163
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
		create_arc( MEASUREMENT_LAYER, TO_PLACE, s_trans, b->phase[1].XX[b_m] );	/* start phase 1 */
		create_arc( layer_mask, TO_PLACE, s_trans, b->start_activity()->ZX[b_m] );
	    }
	}
#endif

	queue_epilogue( x_pos + n_delta, temp_y + 2.5 + n_delta, a, s_a, b, b_m, e, s_e, m, async_call, q_trans, s_trans );

	/*
	 * Bonus stuff in case we want in-service probabilites.  These
	 * states mark the active entry and phase.  Only do the first
	 * instance because all the rest are the same.
	 */

	if ( j->inservice_flag() && m == 0 && b_m == 0 ) {
	    create_phase_instr_net( idle_x, y_pos, a, m, b, b_m, k, r_trans, q_trans, s_trans, ph2_place );
	}
    }
}



/*
 * Common code for head of queue.
 */

struct trans_object *
Model::queue_prologue( double x_pos,		/* X coordinate.		*/
		       double y_pos,		/* Y coordinate.		*/
		       Phase * a,		/* sending __entry.		*/
		       unsigned s_a,		/* Slice number of phase	*/
		       Entry * b,		/* receiving __entry.		*/
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
				      Task::__open_model_tokens, "ZZ%s", b->name()  );
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
#if BUG_163
	if ( s_trans ) {
	    create_arc( layer_mask_b, TO_TRANS, s_trans, g_place );
	}
#endif
    }

    c_place = create_place( x_pos, y_pos - 1.0, layer_mask_a, 0,
			    "R%s%d%s%d%s%d", a->name(), s_a, b->name(), b_m, e->name(), m );

    create_arc( layer_mask_a, TO_PLACE, q_trans, c_place );
#if BUG_163
    if ( s_trans ) {
	create_arc( layer_mask_a, TO_PLACE, s_trans, c_place );
    }
#endif

    c_trans = create_trans( x_pos, y_pos - 0.5, layer_mask_a, b->release_prob(), 1, IMMEDIATE + 1,
			    "rel%s%d%s%d%s%d", a->name(), s_a, b->name(), b_m, e->name(), m );
    create_arc( layer_mask_a, TO_TRANS, c_trans, c_place );

    /* Connect release to appropriate task provided that we do not have to worry about forwarding. */

    if ( b->requests() == Requesting_Type::RENDEZVOUS ) {		/* BUG_629 */
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

#if BUG_423
    /*
     * We have to find all of the 'sink' places and put inhibitor arcs
     * to disallow requests until all threads terminate.
     */
#endif
}


/*
 * Create the subnet used to instrument the phase of service of
 * entry b's __task.
 */

void
Model::create_phase_instr_net( double idle_x, double y_pos,
			       Phase * a, unsigned m,
			       Entry * b, unsigned n, unsigned k,
			       struct trans_object * r_trans, struct trans_object * q_trans, struct trans_object * s_trans,
			       std::vector<Model::inst_arrival>& ph2_place )
{
    if ( n != 0 || m != 0 ) return;	/* Ignore copies */
    unsigned q;			/* Another phase index.		*/
    struct place_object * c_place;
    struct trans_object * c_trans = q_trans;
    double n_delta = (double)n / 4.0;
    double temp_x;
    double temp_y;
    const Task * j     = b->task();

    for ( q = 1; q <= b->n_phases(); ++q ) {
	temp_x = INS_OFFSET(k,(double)(q-1)/2.0) + n_delta;
	temp_y = y_pos + n_delta - 2;

	c_place = create_place( temp_x, temp_y, MEASUREMENT_LAYER, 0,
						 "PH%d%s%d%s%d", q,
						 a->name(), m,
						 b->name(), n );
	if ( q == 2 ) {
	    ph2_place.emplace_back( inst_arrival( temp_x, temp_y, a, m, b, n, c_place ) );
	}
	create_arc( MEASUREMENT_LAYER, TO_PLACE, c_trans, c_place );
	if ( s_trans ) {
	    create_arc( MEASUREMENT_LAYER, TO_PLACE, s_trans, c_place );
	    s_trans = 0;
	}

	c_trans = create_trans( b->phase[q].done_xpos[m], b->phase[q].done_ypos[m],
				MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()),
				1.0, 1, IMMEDIATE, "ph%d%s%d%s%d", q,
				a->name(), m,
				b->name(), n );

	if ( q == 1 && b->requests() == Requesting_Type::RENDEZVOUS ) {	/* BUG_629 */
	    create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_PLACE, c_trans, b->DX[n] );
	}
	if ( q == b->n_phases() ) {
	    create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_PLACE, c_trans, j->TX[n] );
	} else {
	    create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_PLACE, c_trans, b->phase[q+1]._slice[0].WX[n] );
	}

	create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, c_place );
	create_arc( MEASUREMENT_LAYER|ENTRY_LAYER(b->entry_id()), TO_TRANS, c_trans, b->phase[q]._slice[0].ChX[n] );
    }

    /* Arrival at queue k, phase 1 */

    temp_x = INS_OFFSET(k,0.0) + n_delta;
    temp_y = y_pos + Task::__queue_y_offset + 1.0 + n_delta;

    c_place = create_place( temp_x, temp_y, MEASUREMENT_LAYER, 0,
			    "Arr%s%d%s%d", a->name(), m, b->name(), n );

    c_trans = create_trans( temp_x, temp_y + 0.5, MEASUREMENT_LAYER,
			    1.0, 1, IMMEDIATE+1,
			    "ArI%s%d%s%d", a->name(), m, b->name(), n );

    create_arc( MEASUREMENT_LAYER, TO_PLACE, r_trans, c_place );
    create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, c_place );
    create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, j->TX[n] );
    create_arc( MEASUREMENT_LAYER, TO_PLACE, c_trans, j->TX[n] );
}


/* For each phase 2, do... */

void
Model::create_inservice_net( const Phase * a, const Entry * b, unsigned int k, const std::vector<Model::inst_arrival>& ph2_place )
{
    const unsigned int m = 0;	/* Only do the first instance */
    const unsigned int n = 0;	/* Only do the first instance */

    const struct place_object * ab_place = no_place( "Arr%s%d%s%d", a->name(), m, b->name(), n );

    for ( const auto& cd : ph2_place ) {
	const Phase * c = cd.a;
	const Entry * d = cd.b;
	const struct place_object * cd_place = cd.ab_place;

	struct trans_object * c_trans = create_trans( cd.x_pos, cd.y_pos + ((k+1) * 0.5), MEASUREMENT_LAYER,
						      1.0, 1, IMMEDIATE+1,
						      "ArX%s%d%s%d%s%d%s%d",
						      a->name(), m, b->name(), n,
						      c->name(), m, d->phase[2].name(), n );
	create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, ab_place );
	create_arc( MEASUREMENT_LAYER, TO_TRANS, c_trans, cd_place );
	create_arc( MEASUREMENT_LAYER, TO_PLACE, c_trans, cd_place );
#if 0
	fprintf( stderr, "Create ArX%s%d%s%d%s%d%s%d\n",
		 a->name(), m, b->name(), n,
		 c->name(), m, d->phase[2].name(), n );		// debug BUG 369
#endif
    }
}



/*
 * Open arrivals are converted to a psuedo reference task with one
 * customer.  The reference task makes asycnhronous calls to the
 * server. We need to check the results to ensure that the throughput
 * is correct.
 */

void
Model::build_open_arrivals ()
{
    const unsigned n_entries = ::__entry.size();
    for ( unsigned int e = 0; e < n_entries; ++e  ) {
	Entry * dst_entry = ::__entry[e];
	if ( dst_entry->task()->type() == Task::Type::OPEN_SRC ) continue;

	LQIO::DOM::Entry * dst_dom = dst_entry->get_dom();
	if ( !dst_dom->hasOpenArrivalRate() ) continue;

	std::string buf = "OT";
	buf += dst_entry->name();
	OpenTask * a_task = new OpenTask( _document, buf, dst_entry );
	::__task.push_back( a_task );

	buf = "OE";
	buf += dst_entry->name();
	LQIO::DOM::Entry * pseudo = new LQIO::DOM::Entry( dst_dom->getDocument(), buf.c_str() );
	pseudo->setEntryType( LQIO::DOM::Entry::Type::STANDARD );
	LQIO::DOM::Phase* phase = pseudo->getPhase(1);
	phase->setName( buf );
	dst_entry->set_open_arrival_phase( phase );

	Entry * an_entry = new Entry( pseudo, a_task );
	::__entry.push_back( an_entry );		// Do this because add_call will try to find it.
	a_task->entries.push_back( an_entry );

	LQIO::DOM::ConstantExternalVariable  * var  = new LQIO::DOM::ConstantExternalVariable( 1 );
	LQIO::DOM::Call * call = new LQIO::DOM::Call( dst_dom->getDocument(), LQIO::DOM::Call::Type::SEND_NO_REPLY, phase, dst_dom, var );
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
    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {

	/* Service time at entries. */

	for ( std::vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {
	    if ( !(*e)->is_regular_entry() ) continue;
	    for ( unsigned p = 1; p <= (*e)->n_phases(); p++) {
		(*e)->phase[p].create_spar();
	    }
	    Phase::inc_par_offsets();
	}

	std::for_each( (*t)->activities.begin(), (*t)->activities.end(), std::mem_fn( &Activity::create_spar ) );

	Phase::inc_par_offsets();
    }

    /* Call rates. */

    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {

	/* Service time at entries. */

	for ( std::vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {

	    for ( std::vector<Entry *>::const_iterator d = ::__entry.begin(); d != ::__entry.end(); ++d ) {
		for ( unsigned int p = 1; p <= (*e)->n_phases(); p++ ) {
		    (*e)->phase[p].create_ypar( *d );
		}
	    }
	}

	for ( std::vector<Activity *>::const_iterator a = (*t)->activities.begin(); a != (*t)->activities.end(); ++a ) {
	    for ( std::vector<Entry *>::const_iterator d = ::__entry.begin(); d != ::__entry.end(); ++d ) {
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
    std::string name_str;

    for ( std::vector<Task *>::const_iterator t = ::__task.begin(); t != ::__task.end(); ++t ) {
	if ( (*t)->type() != Task::Type::REF_TASK ) continue;

	for ( std::vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {

	    if ( (*e)->is_regular_entry() ) {
		for ( unsigned int p = 1; p <= (*e)->n_phases(); p++) {
		    if ( (*e)->phase[p].s() ) {	/* BUG_21 */
			name_str = std::string( "S" ) + (*e)->phase[p].name() + "00";
			break;			/* Stop on first found */
		    }
		}
	    } else {
		name_str = std::string( "S" ) + (*e)->start_activity()->name() + "00";
	    }
	    if ( name_str[0] ) {
		(void) create_res( Phase::__parameter_x, Phase::__parameter_y, "E%s", "E{#%s};", insert_netobj_name( name_str ).c_str() );
		Phase::inc_par_offsets();
	    }
	}
    }

    for ( std::vector<Processor *>::const_iterator p = ::__processor.begin(); p != ::__processor.end(); ++p ) {
        if ( !(*p)->PX ) continue;
	name_str = std::string( "P" ) + (*p)->name();
	(void) create_res( Phase::__parameter_x, Phase::__parameter_y, "U%s", "P{#%s=0};", insert_netobj_name( name_str ).c_str() );
	Phase::inc_par_offsets();
    }
}

/*----------------------------------------------------------------------*/
/* Output processing.							*/
/*----------------------------------------------------------------------*/

/*
 * Go through data structures and insert results into the DOM
 */

void
Model::insert_DOM_results( const bool valid, const solution_stats_t& stats ) const
{
    /* Insert general information about simulation run */

    _document->setResultConvergenceValue(stats.precision)
	.setResultValid(valid)
	.setResultSolverInformation()
	.setResultPlatformInformation();

    std::stringstream buf;
    buf << "Tangible: " << stats.tangible << ", Vanishing: " << stats.vanishing;
    _document->setExtraComment( buf.str() );

    LQIO::DOM::CPUTime stop_time;
    stop_time.init();
    stop_time -= __start_time;
    stop_time.insertDOMResults( *_document );

    std::for_each( ::__task.begin(), ::__task.end(), std::mem_fn( &Task::insert_DOM_results ) );
    std::for_each( ::__processor.begin(), ::__processor.end(), std::mem_fn( &Processor::insert_DOM_results ) );
}

/*----------------------------------------------------------------------*/
/* 			  Utility routines.				*/
/*----------------------------------------------------------------------*/

/*
 * Print out inservice probabilities Pr{InService(a,p_a,b,c,d,p_d)}.
 */

void
Model::print_inservice_probability( std::ostream& output ) const
{
    std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
    output << std::endl << std::endl;
    if ( uncondition_flag ) {
	output << "UNconditioned in-";
    } else {
	output << "In-";
    }
    output << "service probabilities (p->'a'):" << std::endl << std::endl;
    for ( unsigned int i = 0; i < 4; ++i ) {
	output << "Entry " << "abcd"[i] << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-7) << " ";
    }
    output << "p_d ";
    for ( unsigned int p = 1; p <= n_phases(); ++p ) {
	output << "Phase " << p << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-7) << " ";
    }
    if ( uncondition_flag ) {
	output << "Total";
    } else {
	output << "Mean";
    }
    output << std::endl;

    for ( std::vector<Task *>::const_iterator t_i = ::__task.begin(); t_i != ::__task.end(); ++t_i ) {
	for ( std::vector<Task *>::const_iterator t_j = ::__task.begin(); t_j != ::__task.end(); ++t_j ) {
	    if ( !(*t_j)->inservice_flag() ) continue;	/* Only do ones we have! */

	    unsigned int count = 0;
	    double col_sum[DIMPH+1];
	    double tot_tput[DIMPH+1];	/* Throughput of this __task.	*/

	    for ( unsigned int p = 0; p <= DIMPH; ++p ) {
		col_sum[p] = 0.0;
	    }

	    /* Over all entries of task i and j... */

	    if ( uncondition_flag ) {
		(*t_i)->get_total_throughput( *t_j, tot_tput );
	    }

	    for ( std::vector<Entry *>::const_iterator e_i = (*t_i)->entries.begin(); e_i != (*t_i)->entries.end(); ++e_i ) {
		Entry * a = *e_i;

		for ( std::vector<Entry *>::const_iterator e_j = (*t_j)->entries.begin(); e_j != (*t_j)->entries.end(); ++e_j ) {
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

		output << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen) << "Task total"
		       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << (*t_i)->name() << " "
		       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << (*t_j)->name() << " "
		       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen) << " "
		       << "Sum ";
		for ( unsigned int p = 1; p <= (*t_i)->n_phases(); ++p ) {
		    output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum[p] << " ";
		    row_sum += col_sum[p];
		}
		output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << row_sum;
	    }
	    if ( count > 0 ) {
		output << std::endl;
	    }
	}
    }
    output << std::endl;
    output.flags(oldFlags);
}




/*
 * Print out the results for a, p_a, b, c, d, p_d.
 */

unsigned
Model::print_inservice_cd( std::ostream& output, const Entry * a, const Entry * b, const Task * j,
			   double tot_tput[], double col_sum[DIMPH+1] ) const
{
    unsigned count    = 0;	/* Count of all c,d		*/
    double col_sum_cd[DIMPH+1];	/* sum over all c,d for a,b	*/

    for ( unsigned int p = 0; p <= DIMPH; ++p ) {
	col_sum_cd[p] = 0.0;
    }

    /* Now find prob of token following c->d path instead of a->b OT or idle */

    for ( std::vector<Task *>::const_iterator t_i = ::__task.begin(); t_i != ::__task.end(); ++t_i ) {
	if ( *t_i == j ) continue;

	for ( std::vector<Entry *>::const_iterator e_i = (*t_i)->entries.begin(); e_i != (*t_i)->entries.end(); ++e_i ) {
	    Entry * c = *e_i;

	    const bool overtaking = a->task() == c->task();

	    for ( std::vector<Entry *>::const_iterator e_j = j->entries.begin(); e_j != j->entries.end(); ++e_j ) {
		Entry * d = (*e_j);		/* Called entry index.		*/
		unsigned count_Pd = 0;		/* Count of phases.		*/
		double col_sum_Pd[DIMPH+1];	/* sum over phase of entry d	*/
		double tput[DIMPH+1];

		if ( c->yy(d) == 0.0 ) continue;

		for ( unsigned int p = 0; p <= DIMPH; ++p ) {
		    col_sum_Pd[p] = 0.0;
		}

		for ( unsigned int pd = (overtaking ? 2 : 1); pd <= d->n_phases(); pd++ ) {
		    for ( unsigned int pa = 1; pa <= DIMPH; ++pa ) {
			tput[pa] = 0.0;
		    }
		    count_Pd += 1;

		    if ( count_Pd == 1 ) {
			output << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << a->name() << " "
			       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << b->name() << " "
			       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << c->name() << " "
			       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << d->name() << " ";
		    } else {
			output << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen*4) << " ";
		    }
		    output << " " << pd << "  ";

		    for ( unsigned int pc = 1; pc <= c->n_phases(); ++pc ) {
			if ( c->phase[pc].y(d) == 0.0 ) continue;
			for ( unsigned int pa = 1; pa <= a->n_phases(); ++pa ) {
			    if ( a->phase[pa].y(b) == 0.0 ) continue;
#if 1
			    tput[pa] += get_tput( IMMEDIATE, "ArX%s%d%s%d%s%d%s%d",
						  a->phase[pa].name(), 0,
						  b->name(), 0,
						  c->phase[pc].name(), 0,
						  d->phase[pd].name(), 0 );
#else
			    tput[pa] += get_tput( IMMEDIATE, "ArX%s%d%s%d",
						  c->phase[pc].name(), 0,
						  d->phase[pd].name(), 0 );
#endif
			}
		    }

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
			output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << prob << " ";
			col_sum[pa]    += prob;
			col_sum_Pd[pa] += prob;
			col_sum_cd[pa] += prob;
		    }

		    prob = tot_tput[0] > 0.0 ? tput[0] / tot_tput[0] : 0.0;
		    if ( overtaking ) {
			output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << prob << " OT" << std::endl;
		    } else {
			output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << prob << std::endl;
		    }
		    col_sum[0]    += prob;
		    col_sum_Pd[0] += prob;
		    col_sum_cd[0] += prob;
		} /* p_d */

		if ( count_Pd >= 2 ) {
		    output << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen*4) << " " << "Sum ";
		    for ( unsigned int pa = 1; pa <= n_phases(); ++pa ) {
			output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_Pd[pa] << " ";
		    }
		    output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_Pd[0] << (overtaking ? " OT" : "") << std::endl;
		}
		count += 1;

	    } /* d */
	} /* c */
    } /* i */

    /* Total over all c,d for a,b */

    if ( count >= 2 ) {
	output << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << a->name() << " "
	       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << b->name() << " "
	       << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen*2) << " " << "Sum ";
	for ( unsigned int pa = 1; pa <= n_phases(); ++pa ) {
	    output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_cd[pa] << " ";
	}
	output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << col_sum_cd[0] << std::endl << std::endl;
    }

    return count;
}
