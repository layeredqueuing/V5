/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/************************************************************************/

/*
 * $Id: petrisrvn.cc 10943 2012-06-13 20:21:13Z greg $
 *
 * Generate a Petri-net from an SRVN description.
 *
 */

#include <algorithm>
#include <set>
#include <cassert>
#include <cmath>
#include <lqio/glblerr.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_task.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_extvar.h>
#include <lqio/common_io.h>
#include "errmsg.h"
#include "results.h"
#include "petrisrvn.h"
#include "task.h"
#include "entry.h"
#include "processor.h"
#include "phase.h"
#include "activity.h"
#include "actlist.h"
#include "makeobj.h"
#include "results.h"
#include "pragma.h"

using namespace std;

#define SUM_BRANCHES  true
#define FOLLOW_BRANCH false
#define FOLLOW_RNVS   true
#define IGNORE_RNVS   false

std::vector<Task *> task;
double Task::__server_x_offset;		/* Starting offset for next server.	*/
double Task::__client_x_offset;		/* Starting offset for next client.	*/
double Task::__server_y_offset;
double Task::__queue_y_offset;

/*
 * Add a task to the model.
 */

Task::Task( LQIO::DOM::Task* dom, task_type type, Processor * processor )
    : Place( dom ),
      entries(),
#if !defined(BUFFER_BY_ENTRY)
      ZZ(0),
#endif
      _processor(processor),
      _type(type),
      _sync_server(false),
      _has_main_thread(false),
      _inservice_flag(false),
      _needs_flush(false),
      _queue_made(false),
      _n_phases(1),
      _n_threads(1),
      _max_queue_length(0),
      _max_k(0),				/* input queues. 		*/
#if !defined(BUFFER_BY_ENTRY)
      _open_tokens(open_model_tokens),
#endif
      _proc_queue_count(0),
      _requestor_no(0)
{
    initialize();
    _inservice_flag = (bool)(inservice_match_pattern != 0
    			     && regexec( inservice_match_pattern, 
					 name(), 0, 0, 0 ) != REG_NOMATCH );
#if !defined(BUFFER_BY_ENTRY)
    if ( dom && dom->hasQueueLength() ) {
	_open_tokens = open_model_tokens;
    }
#endif
}


void Task::initialize()
{
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	TX[m] = 0;			/* Task place.			*/
#if defined(BUG_163)
	SyX[m] = 0;			/* Sync wait place.		*/
#endif
	GdX[m] = 0;			/* Guard Place			*/
	gdX[m] = 0;			/* Guard fork transition.	*/
	LX[m] = 0;			/* Lock Place	(BUG_164)	*/
	_utilization[m] = 0;		/* Result for finding util.	*/
	task_tokens[m] = 0;		/* Result. 			*/
	lock_tokens[m] = 0;		/* Result.			*/
    }
#if !defined(BUFFER_BY_ENTRY)
    ZZ = 0;				/* For open requests.		*/
#endif
    _queue_made = false;		/* true if queue made for task.	*/
    _max_queue_length = 0;	
    _max_k = 0;				/* input queues. 		*/
}



/*
 * Clean out old transitions after run because we check recursively for 0 on construction.
 */

void 
Task::remove_netobj()
{
    for ( vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
        (*e)->remove_netobj();
    }
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
        TX[m] = 0;		/* Task place.			*/
#if defined(BUG_163)
	SyX[m] = 0;		/* Sync wait place.		*/
#endif
	GdX[m] = 0;		/* Guard Place			*/
	gdX[m] = 0;		/* Guard fork transition.	*/
	LX[m] = 0;		/* Lock Place	(BUG_164)	*/
#if !defined(BUFFER_BY_ENTRY)
#endif
        
    }
    ZZ = 0;			/* For open requests.		*/

    for ( vector<ActivityList *>::const_iterator l = act_lists.begin(); l != act_lists.end(); ++l ) {
	(*l)->remove_netobj( );
    }
}


Task * 
Task::create( LQIO::DOM::Task * dom )
{
    const string& task_name = dom->getName();
    if ( task_name.size() == 0 ) abort();

    if ( dom->getReplicasValue() != 1 ) {
	LQIO::input_error2( ERR_REPLICATION, "task", task_name.c_str() );
    }


    Processor * processor = Processor::find( dom->getProcessor()->getName() );
    if ( !processor ) {
	input_error2( LQIO::ERR_NOT_DEFINED, dom->getProcessor()->getName().c_str() );
	return 0;
    }

    if ( !LQIO::DOM::Common_IO::is_default_value( dom->getPriority(), 0. ) && ( bit_test( processor->scheduling(), SCHED_FIFO_BIT|SCHED_PS_BIT|SCHED_RAND_BIT ) ) ) {
	LQIO::input_error2( LQIO::WRN_PRIO_TASK_ON_FIFO_PROC, task_name.c_str(), processor->name() );
    }

    /* Override scheduling */

    scheduling_type sched_type = dom->getSchedulingType();
    if ( !bit_test( sched_type, SCHED_BURST_BIT|SCHED_UNIFORM_BIT|SCHED_CUSTOMER_BIT|SCHED_DELAY_BIT|SCHED_SEMAPHORE_BIT) ) {
	if ( dom->isInfinite() ) {
	    sched_type = SCHEDULE_DELAY;
	    dom->setSchedulingType( sched_type );
	} else if ( !Pragma::__pragmas->default_task_scheduling() ) {
	    sched_type = Pragma::__pragmas->task_scheduling();
	    dom->setSchedulingType( sched_type );
	}
    }

    Task * task = nullptr;

    switch ( sched_type ) {
    case SCHEDULE_BURST:
    case SCHEDULE_UNIFORM:
	LQIO::input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label[sched_type].str, dom->getTypeName(), task_name.c_str() );
	/* fall through */
    case SCHEDULE_CUSTOMER:
	if ( dom->hasQueueLength() ) {
	    LQIO::input_error2( LQIO::WRN_QUEUE_LENGTH, task_name.c_str() );
	}
	if ( dom->isInfinite() ) {
	    input_error2( LQIO::ERR_REFERENCE_TASK_IS_INFINITE, task_name.c_str() );
	}
	task = new Task( dom, REF_TASK, processor );
	break;
	
    case SCHEDULE_PPR:
    case SCHEDULE_HOL:
	LQIO::input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label[sched_type].str, dom->getTypeName(), task_name.c_str() );
	/* fall through */
    case SCHEDULE_FIFO:
    default:
	if ( dom->hasThinkTime() ) {
	    input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name.c_str() );
	}
        task = new Task( dom, SERVER, processor );
	break;
	
    case SCHEDULE_DELAY:
	if ( dom->hasThinkTime() ) {
	    input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name.c_str() );
	}
	if ( dom->hasQueueLength() ) {
	    LQIO::input_error2( LQIO::WRN_QUEUE_LENGTH, task_name.c_str() );
	}
	if ( dom->isMultiserver() ) {
	    LQIO::input_error2( LQIO::WRN_INFINITE_MULTI_SERVER, "Task", task_name.c_str(), dom->getCopiesValue() );
	}	
	task = new Task( dom, SERVER, processor );
	break;

    case SCHEDULE_SEMAPHORE:
	if ( dom->hasQueueLength() ) {
	    LQIO::input_error2( LQIO::WRN_QUEUE_LENGTH, task_name.c_str() );
	}
	if ( dom->getCopiesValue() != 1 ) {
	    input_error2( LQIO::ERR_INFINITE_TASK, task_name.c_str() );
	}
#if 0
	if ( n_copies <= 0 ) {
	    LQIO::input_error2( LQIO::ERR_INFINITE_TASK, task_name.c_str() );
	    dom->setCopiesValue( 1 );
	} else if ( n_copies > MAX_MULT ) {
	    LQIO::input_error2( LQIO::ERR_TOO_MANY_X, "multi-server copies", MAX_MULT );
	    dom->setCopiesValue( MAX_MULT );
	}
#endif
	task = new Task( dom, SEMAPHORE, processor );
	break;
    }

    processor->add_task( task );
    assert( task != 0 );
    ::task.push_back( task );
    return task;
}


bool
Task::check() 
{
    if ( !entries.size() ) {
	LQIO::solution_error( LQIO::ERR_NO_ENTRIES_DEFINED_FOR_TASK, name() );
    }
    if ( n_activities() ) {
	bool hasActivityEntry = false;
	for ( vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	    if ( (*e)->start_activity() ) {
		hasActivityEntry = true;
	    }
	}
	for ( vector<Activity *>::const_iterator a = activities.begin(); a != activities.end(); ++a ) {
	    double calls = (*a)->check();
	    if ( !(*a)->is_specified() ) {
		solution_error( LQIO::ERR_ACTIVITY_NOT_SPECIFIED, name(), (*a)->name() );
	    }
	    if ( calls > 0 && (*a)->s() == 0.0 ) {
		solution_error( LQIO::WRN_NO_SERVICE_TIME_FOR, "Task", name(), "Activity", (*a)->name() );
	    }
	}

	if ( !hasActivityEntry ) {
	    solution_error( LQIO::ERR_NO_START_ACTIVITIES, name() );
	}
    }

    if ( type() == SEMAPHORE ) {
	if ( n_entries() != N_SEMAPHORE_ENTRIES ) {
	    solution_error( LQIO::ERR_ENTRY_COUNT_FOR_TASK, name(), n_entries(), N_SEMAPHORE_ENTRIES );
	}
	if ( entries[0]->semaphore_type() == SEMAPHORE_SIGNAL ) {
	    if ( entries[1]->semaphore_type() != SEMAPHORE_WAIT ) {
		solution_error( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
	    }
	} else if ( entries[0]->semaphore_type() == SEMAPHORE_WAIT ) {
	    if ( entries[1]->semaphore_type() != SEMAPHORE_SIGNAL ) {
		solution_error( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
	    }
	} else {
	    solution_error( LQIO::ERR_NO_SEMAPHORE, name() );
	}
    }

    /* Check for external joins. */

    for ( vector<ActivityList *>::const_iterator l = act_lists.begin(); l != act_lists.end(); ++l ) {
	if ( (*l)->check_external_joins( ) ) {
	    _sync_server = true;
	}
	if ( (*l)->check_fork_no_join( ) || (*l)->check_quorum_join( ) ) {
	    _needs_flush = true;
	}
	if ( (*l)->check_fork_has_join( ) ) {
	    _has_main_thread = true;
	}
	if ( (*l)->type() == ACT_AND_FORK_LIST ) {
	    if ( (*l)->n_acts() > n_threads() ) {
		_n_threads = (*l)->n_acts();
	    }
	}
    }

    if ( _sync_server && multiplicity() != 1 ) {
	LQIO::solution_error( ERR_MULTI_SYNC_SERVER, name() );
    }

    /* Can't have in-service probabilites on single phase task! */
	
    if ( n_phases() < 2 ) {
	_inservice_flag = false;
    }
	
    for ( vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	double ysum = 0.0;
	double zsum = 0.0;

	for ( vector<Task *>::const_iterator i = ::task.begin(); i != ::task.end(); ++i ) {

	    /* e of i called by d of j ?*/

	    for ( vector<Entry *>::const_iterator d = (*i)->entries.begin(); d != (*i)->entries.end(); ++d ) {
		ysum += (*d)->yy(*e) + (*d)->prob_fwd(*e);
		zsum += (*d)->zz(*e);
	    }

	    for ( vector<Activity *>::const_iterator d = (*i)->activities.begin(); d != (*i)->activities.end(); ++d ) {
		ysum += (*d)->y(*e);
		zsum += (*d)->z(*e);
	    }
	}

	if ( ysum > 0.0 && zsum > 0.0 ) {
	    solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, name() );
	} else if ( ysum + zsum == 0.0 && is_server() ) {
	    solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, name() );
	}
    }

    return !LQIO::io_vars.anError();
}

/* Priority for this task.	*/
int Task::priority() const
{
    int value = 0;
    try {
	value = dynamic_cast<LQIO::DOM::Task *>(get_dom())->getPriorityValue();
    }
    catch ( const std::domain_error &e ) {
	LQIO::solution_error( LQIO::ERR_INVALID_PARAMETER, "priority", get_dom()->getTypeName(), name(), e.what() );
    }
    return value;
}

double Task::think_time() const
{
    if ( dynamic_cast<LQIO::DOM::Task *>(get_dom())->hasThinkTime() ) {
	try {
	    return dynamic_cast<LQIO::DOM::Task *>(get_dom())->getThinkTimeValue();
	}
	catch ( const std::domain_error &e ) {
	    LQIO::solution_error( LQIO::ERR_INVALID_PARAMETER, "think time", get_dom()->getTypeName(), name(), e.what() );
	}
    }
    return 0.;
}


bool Task::is_server() const
{
    return bit_test( type(), SERVER_BIT|SEMAPHORE_BIT );
}

bool Task::is_client() const
{
    return bit_test( type(), REF_TASK_BIT|OPEN_SRC_BIT );
}


bool Task::is_single_place_task() const
{
    return type() == REF_TASK && (customers_flag
				  || (n_threads() > 1 && !processor()->is_infinite()));
}

unsigned int Task::ref_count() const
{
    if ( is_infinite() ) {
	return max_queue_length();
    } else {
	return multiplicity();
    }
}


/*
 * Return the number of "customers" to generate.  Open arrival sources
 * alway generate one copy. Reference tasks generate one copy if the
 * customers_flag is set.
 */

unsigned Task::n_customers() const
{
    if ( is_single_place_task() || type() == OPEN_SRC) {
	return 1;
    } else {
	return multiplicity();
    }
}

/* 
 * Create a new activity assigned to a given task and set the information DOM entry for it 
 */

Activity * 
Task::add_activity( LQIO::DOM::Activity * dom )
{
    Activity * activity = find_activity( dom->getName() );
    if ( activity ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Activity", dom->getName().c_str() );
    } else {
	activity = new Activity( dom, this );
	activities.push_back( activity );
    }
    return activity;
}




/*
 * Find the activity.  Return error if not found.
 */

Activity *
Task::find_activity( const std::string& name ) const
{
    vector<Activity *>::const_iterator ap = find_if( activities.begin(), activities.end(), eqActivityStr( name ) );
    if ( ap != activities.end() ) {
	return *ap;
    } else {
	return 0;
    }
}



void Task::set_start_activity( LQIO::DOM::Entry* dom )
{
    Activity* activity = find_activity( dom->getStartActivity()->getName() );
    Entry* realEntry = Entry::find( dom->getName() );
	
    realEntry->set_start_activity(activity);
}

/*----------------------------------------------------------------------*/
/* Tasks.								*/
/*----------------------------------------------------------------------*/

/*
 * Initialize stuff for fifo queueing centers.
 */

unsigned int Task::set_queue_length()
{
    unsigned length = 0;
#if !defined(BUFFER_BY_ENTRY)
    bool open_model = false;
#endif
	
    if ( is_client() ) return 0;  				/* Skip reference tasks. 	*/

    /*
     * Count calls to my entries.
     */

    for ( vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	set<const Task *> visit;

	/* Count rendezvous from other tasks 'i' */
	    
	for ( vector<Task *>::const_iterator i = ::task.begin(); i != ::task.end(); ++i ) {
	    if ( (*i) == this || (*i)->type() == OPEN_SRC ) continue;
	    
	    for ( vector<Entry *>::const_iterator d = (*i)->entries.begin(); d != (*i)->entries.end(); ++d ) {
		if ( (*d)->prob_fwd(*e) == 0.0 ) continue;

		for ( vector<Forwarding *>::const_iterator f = (*d)->forwards.begin(); f != (*d)->forwards.end(); ++f ) {
		    const Task * root = (*f)->_root->task();
		    if ( visit.find( root ) != visit.end() ) continue;
		    length += root->multiplicity();
		    visit.insert( root );
		}
	    }

	    if( visit.find( (*i) ) != visit.end() ) goto again;		/* No point.. already counted */

	    for ( vector<Entry *>::const_iterator d = (*i)->entries.begin(); d != (*i)->entries.end(); ++d ) {
		if ( !(*d)->is_regular_entry() ) continue;
		    
		for ( unsigned p = 1; p <= (*d)->n_phases(); p++) {
		    if ( (*d)->phase[p].y(*e) == 0.0 && (*d)->phase[p].z(*e) == 0.0 ) continue;
		    if ( visit.find( *i ) == visit.end() ) {
			length += (*i)->multiplicity();
			visit.insert( *i );
			goto again;
		    }
		}
	    }

	    for ( vector<Activity *>::const_iterator a = (*i)->activities.begin(); a != (*i)->activities.end(); ++a ) {
		if ( (*a)->y(*e) == 0.0 ) continue;
		if ( visit.find( *i ) == visit.end() ) {
		    length += (*i)->multiplicity();
		    visit.insert( *i );
		    goto again;
		}
	    }
	again: ;
	}

	/* Count open arrivals at entries. */

	if ( (*e)->requests() == SEND_NO_REPLY_REQUEST && !is_infinite() ) {
#if defined(BUFFER_BY_ENTRY)
	    length += open_model_tokens;
#else
	    if ( !open_model ) {
		open_model = true;
		if ( _sync_server || has_random_queueing() || bit_test( type(), SEMAPHORE_BIT) || is_infinite() ) {
		    length += 1;
		} else {
		    length += _open_tokens;
		}
	    }
#endif
	}
    }

/* 	assert( length > 0 ); */
    _max_queue_length = length;
    _requestor_no     = 0;

    return length;
}



/*
 * Starting from the reference tasks, trace all forwarding requests.
 */

void
Task::build_forwarding_lists()
{
//    if ( type() != REF_TASK ) return;
		
    for ( vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	for ( unsigned p = 1; p <= (*e)->n_phases(); ++p ) {
	    (*e)->phase[p].build_forwarding_list();
	}
    }

    for ( vector<Activity *>::const_iterator a = activities.begin(); a != activities.end(); ++a ) {
	(*a)->build_forwarding_list();
    }
}



/*
 * Create places used to control fifo queues for entries.
 */

void
Task::make_queue_places()
{
    const double x_pos = get_x_pos() - 0.5;
    const double y_pos = get_y_pos();	    
    const unsigned ne  = n_entries();  
		
    if ( _queue_made ) return;
    _queue_made = true;

    /*
     * Create places for queueing at entry.
     */
		
    for ( unsigned k = 1; k <= max_queue_length(); ++k ) {
        (void) move_place_tag( create_place( X_OFFSET(1,0.0), y_pos + __queue_y_offset + (double)k, FIFO_LAYER, 1, "Sh%s%d", name(), k ), PLACE_X_OFFSET, PLACE_Y_OFFSET );
    }
}
		


/*
 * Template logic to create a task and its associated processors.
 * Entries are created later.
 */

void
Task::transmorgrify()
{
    double x_pos;
    double y_pos;
    double next_x = 0;
    const unsigned ne = n_entries();

    if ( is_client() ) {
	x_pos = __client_x_offset;
	y_pos = CLIENT_Y_OFFSET;
    } else {
	x_pos = __server_x_offset;
	y_pos = __server_y_offset;
    }
    set_origin( x_pos, y_pos );

    /* On tasks with dedicated processors, move the processor place. */

    if ( processor() && processor()->PX && processor()->is_single_place_processor() ) {
        processor()->set_origin( X_OFFSET(4+1,0), y_pos );
	processor()->PX->center.x = IN_TO_PIX( processor()->get_x_pos() );
	processor()->PX->center.y = IN_TO_PIX( processor()->get_y_pos() );
    }

    /* task places */

    if ( is_single_place_task()
	 || type() == OPEN_SRC
	 || (is_infinite() && this->n_threads() == 1) ) {
		
	next_x = create_instance( x_pos, y_pos, 0, INFINITE_SERVER );
		
    } else if ( is_infinite() ) {
	unsigned m;		/* Multiserver index.	*/

	for ( m = 0; m < max_queue_length(); ++m ) {
	    next_x = create_instance( x_pos+m*0.25, y_pos+m*0.25, m, 1 );
	}
    } else {
	unsigned m;		/* Multiserver index.	*/

	for ( m = 0; m < this->multiplicity(); ++m ) {
	    next_x = create_instance( x_pos+m*0.25, y_pos+m*0.25, m, 1 );
	}
    }
		
    if ( is_client() ) {
	__client_x_offset = next_x;
    } else {
	__server_x_offset = next_x;
    }
}



/*
 * Make an instance of a task.
 */

double
Task::create_instance( double base_x_pos, double base_y_pos, unsigned m, short enabling )
{
    double x_pos	= base_x_pos;
    double y_pos	= base_y_pos;
    struct place_object * d_place = 0;
    unsigned ix_e;
    const unsigned ne 	= n_entries();

    double temp_x;
    double max_pos 	= base_x_pos;

    unsigned customers;

    if ( is_infinite() ) {
	customers = open_model_tokens;
    } else if ( is_single_place_task() || type() == OPEN_SRC ) {
	customers = this->multiplicity();
    } else {
	customers = 1;
    }

    if ( n_activities() > 1 ) {
	temp_x = X_OFFSET(1,n_entries()*0.5);
    } else if ( this->n_phases() == 1 ) {
	temp_x = X_OFFSET(1,0.5);
    } else {
	temp_x = X_OFFSET(this->n_phases()*3,0);
    }

    d_place = create_place( temp_x, Y_OFFSET(0.0), make_layer_mask( m ), customers, "T%s%d", this->name(), m );
    this->TX[m] = d_place;

    if ( type() == SEMAPHORE ) {	/* BUG_164 */
	this->LX[m] = create_place( temp_x+0.5, Y_OFFSET(0.0), make_layer_mask( m ), 0, "LX%s%d", this->name(), m );
    } else if ( this->_sync_server ) { 
	d_place = create_place( temp_x+0.5, Y_OFFSET(0.0), make_layer_mask( m ), 0, "Gd%s%d", this->name(), m );
	this->GdX[m] = d_place;
	this->gdX[m] = create_trans( temp_x+0.5, Y_OFFSET(0.0)-0.5, make_layer_mask( m ),  1.0, 1, IMMEDIATE, "gd%s%d", this->name(), m );
	create_arc( make_layer_mask( m ), TO_TRANS, this->gdX[m], d_place );
	create_arc( make_layer_mask( m ), TO_PLACE, this->gdX[m], this->TX[m] );
#if defined(BUG_163)
	this->SyX[m] = create_place( temp_x+1.0, Y_OFFSET(0.0), make_layer_mask( m ), 0, "SYNC%s%d" , this->name(), m );
#endif
    } else if ( this->_needs_flush ) {
	struct trans_object * d_trans = create_trans( temp_x+0.5, Y_OFFSET(0.0)-0.5, make_layer_mask( m ), 1.0, 1, IMMEDIATE, "gd%s%d", this->name(), m );
	this->gdX[m] = d_trans;
	create_arc( make_layer_mask( m ), TO_PLACE, d_trans, this->TX[m] );
	d_place = create_place( temp_x+0.5, Y_OFFSET(0.0), make_layer_mask( m ), 0, "Gd%s%d", this->name(), m ); 	/* We don't allow multiple copies */
	this->GdX[m] = d_place;
	create_arc( make_layer_mask( m ), TO_TRANS, d_trans, d_place );
    }

    /*
     * Places for replies. Create here for activities because sync
     * servers can reply to entries before the entry itself is
     * created
     */

    ix_e = 0;
     for ( std::vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	if ( is_server() && (*e)->requests() == RENDEZVOUS_REQUEST ) {
	    const LAYER layer_mask = ENTRY_LAYER((*e)->entry_id())|(m == 0 ? PRIMARY_LAYER : 0);

	    x_pos = base_x_pos + ix_e * 0.5;
	    (*e)->DX[m] = move_place_tag( create_place( X_OFFSET(3,0), Y_OFFSET(0.0), layer_mask, 0, "D%s%d", (*e)->name(), m ), PLACE_X_OFFSET, -0.25 );
	}
#if defined(BUG_622)
	if ( !(*e)->is_regular_entry() ) {
	    unsigned p;
	    double task_y_offset = Y_OFFSET(1.0);
	    if ( inservice_flag() ) {
		task_y_offset += 1.0;
	    }
	    for ( p = 1; p <= (*e)->n_phases(); ++p ) {
		(*e)->phase[p].XX[m] = create_place( X_OFFSET(p,1.0), task_y_offset-1.0, MEASUREMENT_LAYER, 0, "X%s%d%d", (*e)->name(), p, m );
	    }
	}
#endif
	++ix_e;
    }

    /* Create the entries */
    
    ix_e = 0;
    for ( std::vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	Entry * curr_entry = *e;
	double next_pos = curr_entry->transmorgrify( base_x_pos, base_y_pos, ix_e, d_place, m, enabling );
	if ( next_pos > max_pos ) {
	    max_pos = next_pos;
	}
	++ix_e;
    }

    return max_pos;
}
	

    
/*
 * Create an entry mask for this task.
 */

LAYER
Task::make_layer_mask( const unsigned m )
{
    LAYER mask = (m == 0 ? PRIMARY_LAYER : 0);
	
    for ( std::vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	mask |= ENTRY_LAYER((*e)->entry_id());
    }
    return mask;
}

/* ------------------------------------------------------------------------ */
/* Results								    */
/* ------------------------------------------------------------------------ */

void
Task::get_results()
{
    const unsigned int max_m = n_customers();
    for ( unsigned int m = 0; m < max_m; ++m ) {
	get_results(m);
    }
    for ( std::vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	(*e)->check_open_result();
    }
}



void
Task::get_results( unsigned m )
{
    if ( is_single_place_task() ) {
	_utilization[m] = multiplicity() - get_pmmean( "T%s%d", name(), m );
    } else if ( !is_infinite() ) {
	_utilization[m] = 1.0 - get_pmmean( "T%s%d", name(), m );
    } else {
	_utilization[m] = open_model_tokens - get_pmmean( "T%s%d", name(), m );
    }
    this->task_tokens[m] = 0.0;

    if ( type() == SEMAPHORE ) {
	this->lock_tokens[m] = get_pmmean( "LX%s%d", name(), m );
    } else {
	this->lock_tokens[m] = 0.0;
    }

    /* for each entry of i	    */
	
    for ( std::vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	if ( (*e)->is_regular_entry() ) {

	    for ( unsigned p = 1; p <= (*e)->n_phases(); ++p ) {
	        Phase& phase = (*e)->phase[p];
		if ( !phase.get_dom()->isPresent() ) continue;
		(*e)->_throughput[m] = get_throughput( (*e), &phase, m );

		/* Task utilization (includes queue) */
		
		this->task_tokens[m] += phase.get_utilization( m );

		/* Procesor utilization (ignores queue) */

		if ( processor() ) {
		    processor()->proc_tokens[m] += phase.get_processor_utilization( m );
		}
	    }
	    
	} else {

	    /* Task utilization */
	    
	    double tokens[DIMPH+1];
	    (*e)->_throughput[m] = get_throughput( *e, (*e)->start_activity(), m );

	    for ( unsigned p = 0; p <= DIMPH; ++p ) {
		tokens[p] = 0;
	    }
	    (*e)->start_activity()->follow_activity_for_tokens( (*e), 1, m, FOLLOW_BRANCH, 1.0, &Phase::get_utilization, tokens );

#if defined(BUG_622)
	    /* Use tokens found from instrumentation */
	    for ( unsigned int p = 1; p <= (*e)->n_phases(); ++p ) {
		(*e)->phase[p].task_tokens[m] = get_pmmean( "X%s%d%d", (*e)->name(), p, m );	/* Phase service time.	*/
		this->task_tokens[m]    += (*e)->phase[p].task_tokens[m];
	    }
#else
	    /* Use tokens found by traversing graph */
	    
	    for ( p = 1; p <= (*e)->n_phases; ++p ) {
		(*e)->phase[p].task_tokens[m] = tokens[p];
		this->task_tokens[m]         += tokens[p];
	    }
#endif

	    /* Procesor utilization */

	    if ( processor() ) {
		for ( unsigned p = 0; p <= DIMPH; ++p ) {
		    tokens[p] = 0;
		}
		(*e)->start_activity()->follow_activity_for_tokens( (*e), 1, m, SUM_BRANCHES, 1.0, &Phase::get_processor_utilization, tokens );
		for ( unsigned p = 1; p <= (*e)->n_phases(); ++p ) {
		    (*e)->phase[p].proc_tokens[m]   = tokens[p];
		    processor()->proc_tokens[m]    += tokens[p];
		    tokens[p] = 0;
		}
	    }
	}
    }
}



/*
 * Find entry throughput.
 */
				
double
Task::get_throughput( const Entry * d, const Phase * phase_d, unsigned m  )
{
    double throughput = 0.0;
	
    if ( !inservice_flag() || !is_server() || d == 0 ) {
      //	throughput = get_tput( IMMEDIATE, "done%s%d", phase_d->name(), m );	/* done_P transition  */
        throughput = phase_d->doneX[m]->f_time;		/* Access directly */
    } else {
	unsigned p_d = this->n_phases() == 1 ? 1 : 2;
	for ( vector<Entry *>::const_iterator e = ::entry.begin(); e != ::entry.end(); ++e ) {
	    unsigned max_m = n_customers();
	    unsigned p_e;
			
	    for ( p_e = 1; p_e <= (*e)->n_phases(); ++p_e ) {
		const Phase * phase_e = &(*e)->phase[p_e];
		if ( phase_e->y(d) + phase_e->z(d) == 0.0 ) continue;
				
		for ( unsigned int n = 0; n < max_m; ++n ) {
		    throughput += get_tput( IMMEDIATE,"ph%d%s%d%s%d", p_d, phase_e->name(), n, d->name(), m  );
		}
	    }
	}
    }
	
    if ( debug_flag && d ) {
	(void) fprintf( stddbg, "%-20.20s tput=%15.10g ", d->name(), throughput );
    }
    return throughput;
}



/*
 * Return the total throughput between i and j.
 */

void
Task::get_total_throughput( Task * dst, double tot_tput[] )
{
    unsigned p;			/* phase index.			*/

    for ( p = 0; p <= n_phases(); ++p ) {
	tot_tput[p] = 0.0;
    }
	
    for ( vector<Entry *>::const_iterator d = entries.begin(); d != entries.end(); ++d ) {
	for ( vector<Entry *>::const_iterator e = dst->entries.begin(); e != dst->entries.end(); ++e ) {
	    for ( p = 1; p <= (*d)->n_phases(); ++p ) {
		tot_tput[p] += (*d)->phase[p].lambda( 0, *e, &(*d)->phase[p] );
	    }
	}
    }

    for ( p = 1; p <= n_phases(); ++p ) {
	tot_tput[0] += tot_tput[p];
    }
}




/*
 * Print throughputs and utilizations
 */

void
Task::insert_DOM_results()
{
    double tput_sum     = 0.0;
    double util_sum     = 0.0;
    unsigned max_m      = n_customers();
    double col_tot[DIMPH];

    for ( unsigned p = 0; p < DIMPH; p++ ) {
	col_tot[p] = 0;
    }

    /* for each entry of i	    */

    for ( std::vector<Entry *>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	double row_tot = 0.0;
	double tput = 0.0;

	(*e)->insert_DOM_results();

	for ( unsigned m = 0; m < max_m; ++m ) {
	    tput += (*e)->_throughput[m];
	}
	if ( type() != SEMAPHORE || e == entries.begin() ) {
	    tput_sum += tput;		// For semaphore tasks, only count throughput once.
	}

	for ( unsigned p = 1; p <= n_phases(); p++ ) {
	    double util = (*e)->task_utilization( p );
	    row_tot      += util;
	    col_tot[p-1] += util;
	    util_sum     += util;
	}
    }

    if ( is_sync_server() ) {
	tput_sum = get_tput( IMMEDIATE, "gd%s%d", name(), 0 );
    }

    get_dom()->setResultPhaseUtilizations(n_phases(),col_tot)
	.setResultUtilization(util_sum)
	.setResultThroughput(tput_sum);

    for ( vector<Activity *>::const_iterator a = activities.begin(); a != activities.end(); ++a ) {
	(*a)->insert_DOM_results();
    }


    /* Forks-Join lists.		*/
    for ( std::vector<ActivityList *>::const_iterator l = act_lists.begin(); l != act_lists.end(); ++l ) {
	if ( (*l)->type() != ACT_AND_JOIN_LIST ) continue;
	(*l)->insert_DOM_results();
    }


}

/* -------------------------------------------------------------------- */
/* Open Arrival Tasks							*/
/* -------------------------------------------------------------------- */

void
OpenTask::get_results( unsigned m )
{
    Phase& phase = entries[0]->phase[1];
    Call call;
    LQIO::DOM::ConstantExternalVariable value(1.0);
    LQIO::DOM::Call dom( _document, LQIO::DOM::Call::SEND_NO_REPLY, 0, 0, &value );
    call._dom = &dom;
    phase.compute_queueing_delay( call, m, _dst, multiplicity(), &phase );
}


/*
 * Print estimated waiting time at servers with open arrival.
 */

void
OpenTask::insert_DOM_results()
{
    LQIO::DOM::Entry * entry = const_cast<LQIO::DOM::Entry *>(_dst->get_dom());
    entry->setResultWaitingTime( entries[0]->phase[1].response_time( _dst ) );
}
