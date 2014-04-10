/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996								*/
/* January 2005                                                         */
/************************************************************************/

/*
 * Input output processing.
 *
 * $URL$
 *
 * $Id$
 */

#include <parasol.h>
#include "lqsim.h"
#include <cmath>
#include <algorithm>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include "errmsg.h"
#include "entry.h"
#include "activity.h"
#include "task.h"
#include "instance.h"
#include "processor.h"
#include "model.h"
#include "stack.h"
#include "pragma.h"

unsigned int open_arrival_count = 0;

/*
 * The following arrays use the global entry index to locate the
 * task and entry information.
 */

set <Entry *, ltEntry> entry;			/* Entry table.		*/
Entry * Entry::entry_table[MAX_PORTS+1];	/* Reverse map		*/

Entry::Entry( LQIO::DOM::Entry* aDomEntry, Task * task )
    : entry_id(::entry.size() + 1),
      local_id(-1),
      port(-1),
      activity(0),
      _dom_entry(aDomEntry),
      _recv(RECEIVE_NONE),
      _task(task)
{
    entry_table[entry_id] = this;

    if ( aDomEntry ) {
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    char * activity_name = new char[strlen( name() ) + 16];
	    (void) sprintf( activity_name, "Entry %s - Ph %d", name(), p );
	    phase[p].rename( activity_name );
	    delete [] activity_name;
	}
    }
}


Entry::~Entry()
{
}

/*
 * Initialization code done BEFORE the simulation starts.  Store the
 * open arrival rate for entry.  This act is accomplished by setting
 * up a fake task to generate open arrivals.
 */

Entry&
Entry::initialize()
{
    if ( _dom_entry && open_arrival_rate() > 0 && test_and_set_recv( Entry::RECEIVE_SEND_NO_REPLY ) ) {
	const double arrival_rate = open_arrival_rate();

	char * task_name = new char[strlen( name() ) + 20];
	(void) sprintf( task_name, "(%s)", name() );
	Task * cp = new Pseudo_Task( task_name );
	::task.insert( cp );
	
	Entry * from_entry = new Pseudo_Entry( task_name, cp );
	from_entry->initialize();
	::entry.insert( from_entry );

	/* Set up entry information for my arrival rate. */

	from_entry->set_service( 1, 1.0 / arrival_rate );
	from_entry->test_and_set( LQIO::DOM::Entry::ENTRY_STANDARD );

	/* Set up a task to handle it */

	cp->_entry.push_back( from_entry );

	/* Set up calls per cycle.  1 call is made per cycle */

	from_entry->phase[1].tinfo.store_target_info( this, 1.0 );

	open_arrival_count += 1;
    }

    return *this;
}

/*
 * Initialization code for entries done AFTER the simulation starts.
 */

double
Entry::configure()
{
    if ( debug_flag ) {
	print_debug_info();
    }
		
    r_cycle.init( SAMPLE, "Entry %-11.11s  - Cycle Time      ", name() );

    switch ( task()->type() ) {
    case Task::CLIENT:
	port = -1;
	break;

    case Task::SEMAPHORE:
	if ( is_signal() ) {
	    port = dynamic_cast<const Semaphore_Task *>(task())->signal_task()->std_port();
	} else if ( is_wait() ) {
	    port = task()->std_port();
	} else {
	    port = -1;
	}
	break;

    default:
	port = task()->std_port();
	break;
    }
		
    if ( !is_defined() ) {
	LQIO::solution_error( LQIO::ERR_ENTRY_NOT_SPECIFIED, name() );
	get_DOM()->setEntryType( LQIO::DOM::Entry::ENTRY_STANDARD );
    }

    double total_calls = 0.0;
    if ( is_regular() ) {

	/* link phases */

	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    Activity * ap = &phase[p];

	    if ( (ap->service() + ap->think_time()) > 0. || ap->tinfo.size() > 0 ) {
		if ( task()->max_phases < p ) {
		    task()->max_phases = p;
		}
	    } else if ( ap->_hist_data ) {
		LQIO::solution_error( WRN_NO_PHASE_FOR_HISTOGRAM, name(), p );
	    }

	    ap->my_phase = p;
	    total_calls += ap->configure( task() );
	}
			
    } else {

	para_stack_t activity_stack;
	para_stack_t fork_stack;
	unsigned int next_phase = 1;
	double n_replies;
	    
	stack_init( &activity_stack );
	stack_init( &fork_stack );    
	activity->find_children( &activity_stack, &fork_stack, this );
	n_replies = activity->count_replies( &activity_stack, this, 1.0, 1, &next_phase );
	stack_delete( &activity_stack );
	stack_delete( &fork_stack );    

	if ( is_rendezvous() ) {
	    if ( n_replies == 0 ) {
		/* tomari: disable to allow a quorum use the default reply which
		   is after all threads complete exection. */
		if ( !pragma.quorum_delayed_calls() ) {	/* Quorum reply (BUG_311)	*/
		    LQIO::solution_error( LQIO::ERR_REPLY_NOT_GENERATED, name() );
		}
	    } else if ( fabs( n_replies - 1.0 ) > EPSILON) {
		LQIO::solution_error( LQIO::ERR_NON_UNITY_REPLIES, n_replies, name() );
	    }
	}

	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    phase[p].configure( task() );	/* for stats .*/
	    if ( !phase[p]._hist_data && _dom_entry->hasHistogramForPhase( p ) ) {		/* BUG_668 */
		phase[p]._hist_data = new Histogram( _dom_entry->getHistogramForPhase( p ) );
	    }
	}
    }
    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	active[p] =  0;
    }
	
    if ( (is_signal() || is_wait()) && task()->type() != Task::SEMAPHORE ) {
	LQIO::solution_error( LQIO::ERR_NOT_SEMAPHORE_TASK, task()->name(),
			      (is_signal() ? "signal" : "wait"),
			      name() );
    }

    if ( (is_r_lock() || is_r_unlock() || is_w_lock() || is_w_unlock()) && task()->type() != Task::RWLOCK ) {
	if ( is_r_lock() || is_r_unlock() ) {
	    LQIO::solution_error( LQIO::ERR_NOT_RWLOCK_TASK, task()->name(),
				  (is_r_lock() ? "r_lock" : "r_unlock"),
				  name() );
	}
	else{
	    LQIO::solution_error( LQIO::ERR_NOT_RWLOCK_TASK, task()->name(),
				  (is_w_lock() ? "w_lock" : "w_unlock"),
				  name() );
	}
    } 

    /* forwarding component */
			
    if ( is_rendezvous() ) {
	fwd.compute_PDF( false, PHASE_STOCHASTIC, name() );
    }

    return total_calls;
}


bool 
Entry::is_regular() const
{
    return get_DOM()->getEntryType() == LQIO::DOM::Entry::ENTRY_STANDARD;
}

bool 
Entry::is_activity() const
{
    return get_DOM()->getEntryType() == LQIO::DOM::Entry::ENTRY_ACTIVITY;
}

bool Entry::is_semaphore() const
{
    return get_DOM()->getSemaphoreFlag() != SEMAPHORE_NONE;
}

bool Entry::is_signal() const
{ 
    return get_DOM()->getSemaphoreFlag() == SEMAPHORE_SIGNAL;
}

bool Entry::is_wait() const
{ 
    return get_DOM()->getSemaphoreFlag() == SEMAPHORE_WAIT;
}

bool 
Entry::is_rwlock() const
{
    return get_DOM()->getRWLockFlag() != RWLOCK_NONE;
}

bool 
Entry::is_r_unlock() const
{
    return get_DOM()->getRWLockFlag() == RWLOCK_R_UNLOCK;
}

bool 
Entry::is_r_lock() const
{
    return get_DOM()->getRWLockFlag() == RWLOCK_R_LOCK;
}

bool 
Entry::is_w_unlock() const
{
    return get_DOM()->getRWLockFlag() == RWLOCK_W_UNLOCK;
}

bool 
Entry::is_w_lock() const
{
    return get_DOM()->getRWLockFlag() == RWLOCK_W_LOCK;
}

bool
Entry::has_lost_messages() const
{
    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	if ( phase[p].has_lost_messages() ) return true;
    }
    return false;
}

/*
 * Set fields denoting run of the mill entry.
 */

bool
Entry::test_and_set( LQIO::DOM::Entry::EntryType type )
{
    const bool rc = get_DOM()->entryTypeOk( type );
    if ( !rc ) {
	input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, name() );
    }
    return rc;
}

bool
Entry::test_and_set_recv( receive_type recv ) 
{
    if ( _recv != RECEIVE_NONE && _recv != recv ) {
	input_error2( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, name() );
	return false;
    } else {
	_recv = recv;
	return true;
    }
}

bool
Entry::test_and_set_semaphore( semaphore_entry_type sema ) 
{
    const bool rc = get_DOM()->entrySemaphoreTypeOk( sema );
    if ( !rc ) {
	input_error2( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
    } 
    return rc;
}

bool
Entry::test_and_set_rwlock( rwlock_entry_type rw ) 
{
    const bool rc = get_DOM()->entryRWLockTypeOk( rw );
    if ( !rc ) {
	input_error2( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    } 
    return rc;
}

Entry& 
Entry::set_DOM( unsigned ph, LQIO::DOM::Phase* phaseInfo )
{
    if (phaseInfo == NULL) return *this;
    phase[ph].set_DOM(phaseInfo);
    return *this;
}


Entry& 
Entry::add_forwarding( Entry* to_entry, LQIO::DOM::Call * call )
{
    if ( !to_entry->test_and_set_recv( Entry::RECEIVE_RENDEZVOUS ) ) return *this;

    /* Do some checks for sanity */
    if ( task()->is_reference_task() ) {
	LQIO::input_error2( LQIO::ERR_REF_TASK_FORWARDING, task()->name(), name() );
    } else {
	fwd.store_target_info( to_entry, call );
    }
    return *this;
}



void
Entry::insertDOMResults() 
{
    _dom_entry->resetResultFlags();

    double sum_cycle          = 0.0;
    double sum_cycle_var      = 0.0;
    double sum_task_util      = 0.0;
    double sum_task_util_var  = 0.0;
    double sum_proc_util      = 0.0;
    double sum_proc_util_var  = 0.0;

    /*
     * Entry results
     */
	    
    for ( unsigned p = 1; p <= task()->max_phases; ++p ) {
	if ( !is_activity() ) { 
	    if ( phase[p].is_specified() ) {
		phase[p].insertDOMResults();
	    }
	} else {
	    _dom_entry->setResultPhasePServiceTime( p, phase[p].r_cycle.mean() )
		.setResultPhasePVarianceServiceTime( p, phase[p].r_cycle_sqr.mean() )
		.setResultPhasePUtilization( p, phase[p].r_util.mean() )
		.setResultPhasePProcessorWaiting(p,phase[p].r_proc_delay.mean());
	    if ( number_blocks > 1 ) {
		_dom_entry->setResultPhasePServiceTimeVariance( p, phase[p].r_cycle.variance() )
		    .setResultPhasePVarianceServiceTimeVariance( p, phase[p].r_cycle_sqr.variance() )
		    .setResultPhasePUtilizationVariance( p, phase[p].r_util.variance() )
		    .setResultPhasePProcessorWaitingVariance(p,phase[p].r_proc_delay.variance());
	    }
	}

	sum_task_util      += phase[p].r_util.mean();
	sum_task_util_var  += phase[p].r_util.variance();
	sum_cycle          += phase[p].r_cycle.mean();
	sum_cycle_var      += phase[p].r_cycle_sqr.mean();
	sum_proc_util      += phase[p].r_cpu_util.mean();
	sum_proc_util_var  += phase[p].r_cpu_util.variance();
    }

    /*
     * Service times.
     */
	    
    if (is_activity()) {
	for ( unsigned p = 1; p <= 2; ++p ) {
	    _dom_entry->setResultPhasePServiceTime( p, phase[p].r_cycle.mean() )
		.setResultPhasePVarianceServiceTime( p, phase[p].r_cycle_sqr.mean() )
		.setResultPhasePUtilization( p, phase[p].r_util.mean() );
	    if ( number_blocks > 1 ) {
		_dom_entry->setResultPhasePServiceTimeVariance( p, phase[p].r_cycle.variance() )
		    .setResultPhasePVarianceServiceTimeVariance( p, phase[p].r_cycle_sqr.variance() )
		    .setResultPhasePUtilization( p, phase[p].r_util.variance() );
	    }
	    if ( phase[p]._hist_data ) {
		phase[p]._hist_data->insertDOMResults();
	    }
	}
    }

    /*
     * Entry results (regardless of phases/activities)
     */

    _dom_entry->setResultThroughput(throughput())
	.setResultUtilization(sum_task_util)
	.setResultProcessorUtilization(sum_proc_util);
    if ( number_blocks > 1 ) {
	_dom_entry->setResultThroughputVariance(throughput_variance())
	    .setResultUtilizationVariance(sum_task_util_var)
	    .setResultProcessorUtilizationVariance(sum_proc_util_var);	
    }

    if ( sum_cycle > 0.0 ) {
	_dom_entry->setResultSquaredCoeffVariation(sum_cycle_var/square(sum_cycle));
    }
	      
    fwd.insertDOMResults();

    /* Open arrivals are done in Task::PseudoTask */
}

double
Entry::throughput() const
{
    return r_cycle.mean_count() / Model::block_period();
}

double
Entry::throughput_variance() const
{
    return r_cycle.variance_count() / square(Model::block_period());
}

/*----------------------------------------------------------------------*/
/*	 Input processing.  Called from load.cc::prepareModel() 	*/
/*----------------------------------------------------------------------*/

Pseudo_Entry::Pseudo_Entry( const char * name, Task * task ) 
    : Entry (0,task), _name(name) 
{
    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	char * activity_name = new char[strlen( name ) + 16];
	(void) sprintf( activity_name, "Entry %s - Ph %d", name, p );
	phase[p].rename( activity_name );
	delete activity_name;
    }
}


Entry&
Pseudo_Entry::set_service( const unsigned p, const double s ) 
{ 
    phase[p].set_service(s);
    return *this; 
}

/*----------------------------------------------------------------------*/
/*	 Input processing.  Called from load.cc::prepareModel() 	*/
/*----------------------------------------------------------------------*/

/*
 *  Add an entry.
 */

Entry *
Entry::add( LQIO::DOM::Entry* domEntry, Task * task )
{
    Entry * ep = 0;	
    if ( ::entry.size() >= MAX_PORTS ) {
	input_error2( LQIO::ERR_TOO_MANY_X, "entries", MAX_PORTS );
    } else {
	const char* entry_name = domEntry->getName().c_str();
	set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( entry_name ) );
	if ( nextEntry != entry.end() ) {
	    LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", entry_name );
	} else {
	    ep = new Entry( domEntry, task );
	    ::entry.insert( ep );
	    ep->initialize();
	}
    }
    return ep;
}



void 
Entry::add_call( const unsigned int p, LQIO::DOM::Call* domCall )
{
    /* Begin by extracting the from/to DOM entries from the call and their names */
    assert( get_DOM() == domCall->getSourceEntry() );
    assert( 0 < p && p <= MAX_PHASES );
    const LQIO::DOM::Entry* toDOMEntry = domCall->getDestinationEntry();

    /* Make sure this is one of the supported call types */
    if (domCall->getCallType() != LQIO::DOM::Call::SEND_NO_REPLY && 
	domCall->getCallType() != LQIO::DOM::Call::RENDEZVOUS &&
	domCall->getCallType() != LQIO::DOM::Call::QUASI_SEND_NO_REPLY) {
	abort();
    }
	
    /* Internal Entry references */
    const char* to_entry_name = toDOMEntry->getName().c_str();
    Entry * to_entry = Entry::find( to_entry_name );
    if ( !to_entry ) return;
    if ( !test_and_set( LQIO::DOM::Entry::ENTRY_STANDARD ) ) return;
    if ( domCall->getCallType() == LQIO::DOM::Call::RENDEZVOUS && !to_entry->test_and_set_recv( Entry::RECEIVE_RENDEZVOUS ) ) return;
    if ( (domCall->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY || domCall->getCallType() == LQIO::DOM::Call::QUASI_SEND_NO_REPLY) && !to_entry->test_and_set_recv( Entry::RECEIVE_SEND_NO_REPLY ) ) return;

    phase[p].tinfo.store_target_info( to_entry, domCall );
}

/*
 * Locate the entry.  Return nil on error.
 */

Entry *
Entry::find( const char * entry_name )
{
    set<Entry *,ltEntry>::const_iterator nextEntry = find_if( ::entry.begin(), ::entry.end(), eqEntryStr( entry_name ) );
    if ( nextEntry == ::entry.end() ) {
	input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	return 0;
    } else {
	return *nextEntry;
    }
}


/*
 * Locate both entries.  return false on error.
 */

bool
Entry::find( const char * from_entry_name, Entry * & from_entry, const char * to_entry_name, Entry * & to_entry )
{
    bool rc    = true;
    from_entry = find( from_entry_name );
    to_entry   = find( to_entry_name );

    if ( !to_entry ) {
	rc = false;
    }

    if ( !from_entry ) {
	rc = false;
    } else if ( from_entry == to_entry ) {
	input_error2( LQIO::ERR_SRC_EQUALS_DST, to_entry_name, from_entry_name );
	rc = false;
    }
    return rc;
}


/*
 * Debugging function.
 */

void
Entry::print_debug_info()
{
    (void) fprintf( stddbg, "---------- Entry %s ----------\n", name() );

    if ( fwd.size() > 0 ) {
	fprintf( stddbg, "\tfwds:  " );
	vector<tar_t>::iterator tp;
	for ( tp = fwd.target.begin(); tp != fwd.target.end(); ++tp ) {
	    if ( tp != fwd.target.begin() ) {
		(void) fprintf( stddbg, ", " );
	    }
	    tp->print( stddbg );
	}
	(void) fprintf( stddbg, ".\n" );    
    }
}
