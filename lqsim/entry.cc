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
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqsim/entry.cc $
 *
 * $Id: entry.cc 15000 2021-09-27 18:48:30Z greg $
 */

#include <parasol.h>
#include "lqsim.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits.h>
#include <stdarg.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include "activity.h"
#include "entry.h"
#include "errmsg.h"
#include "instance.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "task.h"

unsigned int open_arrival_count = 0;

/*
 * The following arrays use the global entry index to locate the
 * task and entry information.
 */

std::set <Entry *, ltEntry> entry;		/* Entry table.		*/
Entry * Entry::entry_table[MAX_PORTS+1];	/* Reverse map		*/

Entry::Entry( LQIO::DOM::Entry* dom, Task * task )
    : _phase(MAX_PHASES),
      _active(MAX_PHASES),
      r_cycle(),
      _minimum_service_time(MAX_PHASES),
      _dom(dom),
      _entry_id(::entry.size() + 1),
      _local_id(task->n_entries()),
      _port(-1),
      _activity(nullptr),
      _recv(Type::NONE),
      _task(task),
      _fwd(),
      _join_list(nullptr)
{
    entry_table[_entry_id] = this;

    for ( unsigned p = 0; p < MAX_PHASES; ++p ) {
	std::string activity_name = "Entry ";
	activity_name += this->name();
	activity_name += " - Ph ";
	activity_name += "123"[p];
	_phase[p].rename( activity_name );
	_minimum_service_time[p] = 0.0;
    }
}


Entry::~Entry()
{
}

/*
 * Initialization code for entries done AFTER the simulation starts.
 */

double
Entry::configure()
{
    if ( !is_defined() ) {
	LQIO::solution_error( LQIO::ERR_ENTRY_NOT_SPECIFIED, name() );
	get_DOM()->setEntryType( LQIO::DOM::Entry::Type::STANDARD );
    }

    double total_calls = 0.0;
    if ( is_regular() ) {

	/* link phases */

	for ( std::vector<Activity>::iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    const size_t p = phase - _phase.begin() + 1;
	    phase->set_phase( p );
	    phase->set_task( task() );
	    total_calls += phase->configure();

	    if ( phase->has_service_time() || phase->has_think_time() || phase->tinfo.size() > 0 ) {
		if ( task()->max_phases() < p ) {
		    task()->max_phases( p );
		}
	    } else if ( phase->_hist_data ) {
		LQIO::solution_error( WRN_NO_PHASE_FOR_HISTOGRAM, name(), p );
	    }

	}
			
    } else {

	std::deque<Activity *> activity_stack;
	std::deque<ActivityList *> fork_stack;
	double n_replies;
	    
	_activity->find_children( activity_stack, fork_stack, this );
	ActivityList::Collect data( this, &Activity::count_replies );
	n_replies = _activity->collect( activity_stack, data );

	if ( is_rendezvous() ) {
	    if ( n_replies == 0 ) {
		/* tomari: disable to allow a quorum use the default reply which
		   is after all threads complete exection. */
		if ( !Pragma::__pragmas->quorum_delayed_calls() ) {	/* Quorum reply (BUG_311)	*/
		    LQIO::solution_error( LQIO::ERR_REPLY_NOT_GENERATED, name() );
		}
	    } else if ( fabs( n_replies - 1.0 ) > EPSILON) {
		LQIO::solution_error( LQIO::ERR_NON_UNITY_REPLIES, n_replies, name() );
	    }
	}

	for ( std::vector<Activity>::iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    const size_t p = phase - _phase.begin() + 1;
	    phase->set_task( task() );
	    phase->configure();	/* for stats .*/
	    if ( !phase->_hist_data && _dom->hasHistogramForPhase( p ) ) {		/* BUG_668 */
		phase->_hist_data = new Histogram( _dom->getHistogramForPhase( p ) );
	    }
	}
    }

    _active.assign( MAX_PHASES, 0 );
	
    if ( (is_signal() || is_wait()) && task()->type() != Task::Type::SEMAPHORE ) {
	LQIO::solution_error( LQIO::ERR_NOT_SEMAPHORE_TASK, task()->name(),
			      (is_signal() ? "signal" : "wait"),
			      name() );
    }

    if ( (is_r_lock() || is_r_unlock() || is_w_lock() || is_w_unlock()) && task()->type() != Task::Type::RWLOCK ) {
	if ( is_r_lock() || is_r_unlock() ) {
	    LQIO::solution_error( LQIO::ERR_NOT_RWLOCK_TASK, task()->name(),
				  (is_r_lock() ? "r_lock" : "r_unlock"),
				  name() );
	} else {
	    LQIO::solution_error( LQIO::ERR_NOT_RWLOCK_TASK, task()->name(),
				  (is_w_lock() ? "w_lock" : "w_unlock"),
				  name() );
	}
    } 

    /* forwarding component */
			
    if ( is_rendezvous() ) {
	_fwd.configure( get_DOM(), false );
    }

    return total_calls;
}


Entry&
Entry::initialize()
{
    if ( debug_flag ) {
	print_debug_info();
    }
		
    _join_list = nullptr;		/* Reset */
    
    r_cycle.init( SAMPLE, "Entry %-11.11s  - Cycle Time      ", name() );

    switch ( task()->type() ) {
    case Task::Type::CLIENT:
	_port = -1;
	break;

    case Task::Type::SEMAPHORE:
	if ( is_signal() ) {
	    _port = dynamic_cast<const Semaphore_Task *>(task())->signal_task()->std_port();
	} else if ( is_wait() ) {
	    _port = task()->std_port();
	} else {
	    _port = -1;
	}
	break;

    default:
	_port = task()->std_port();
	break;
    }

    for ( std::vector<Activity>::iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	phase->initialize();
    }
    
    /* forwarding component */
			
    if ( is_rendezvous() ) {
	_fwd.initialize( name() );
    }

    return *this;
}


bool 
Entry::is_regular() const
{
    return get_DOM()->getEntryType() == LQIO::DOM::Entry::Type::STANDARD;
}

bool 
Entry::is_activity() const
{
    return get_DOM()->getEntryType() == LQIO::DOM::Entry::Type::ACTIVITY;
}

bool Entry::is_semaphore() const
{
    return get_DOM()->getSemaphoreFlag() != LQIO::DOM::Entry::Semaphore::NONE;
}

bool Entry::is_signal() const
{ 
    return get_DOM()->getSemaphoreFlag() == LQIO::DOM::Entry::Semaphore::SIGNAL;
}

bool Entry::is_wait() const
{ 
    return get_DOM()->getSemaphoreFlag() == LQIO::DOM::Entry::Semaphore::WAIT;
}

bool 
Entry::is_rwlock() const
{
    return get_DOM()->getRWLockFlag() != LQIO::DOM::Entry::RWLock::NONE;
}

bool 
Entry::is_r_unlock() const
{
    return get_DOM()->getRWLockFlag() == LQIO::DOM::Entry::RWLock::READ_UNLOCK;
}

bool 
Entry::is_r_lock() const
{
    return get_DOM()->getRWLockFlag() == LQIO::DOM::Entry::RWLock::READ_LOCK;
}

bool 
Entry::is_w_unlock() const
{
    return get_DOM()->getRWLockFlag() == LQIO::DOM::Entry::RWLock::WRITE_UNLOCK;
}

bool 
Entry::is_w_lock() const
{
    return get_DOM()->getRWLockFlag() == LQIO::DOM::Entry::RWLock::WRITE_LOCK;
}

bool
Entry::has_lost_messages() const
{
    return std::any_of( _phase.begin(), _phase.end(), Predicate<Activity>( &Activity::has_lost_messages ) );
}

/*
 * Set fields denoting run of the mill entry.
 */

bool
Entry::test_and_set( LQIO::DOM::Entry::Type type )
{
    const bool rc = get_DOM()->entryTypeOk( type );
    if ( !rc ) {
	input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, name() );
    }
    return rc;
}

bool
Entry::test_and_set_recv( Type recv ) 
{
    if ( _recv != Type::NONE && _recv != recv ) {
	input_error2( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, name() );
	return false;
    } else {
	_recv = recv;
	return true;
    }
}

bool
Entry::test_and_set_semaphore( LQIO::DOM::Entry::Semaphore sema ) 
{
    const bool rc = get_DOM()->entrySemaphoreTypeOk( sema );
    if ( !rc ) {
	input_error2( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
    } 
    return rc;
}

bool
Entry::test_and_set_rwlock( LQIO::DOM::Entry::RWLock rw ) 
{
    const bool rc = get_DOM()->entryRWLockTypeOk( rw );
    if ( !rc ) {
	input_error2( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    } 
    return rc;
}

Entry& 
Entry::set_DOM( unsigned p, LQIO::DOM::Phase* phaseInfo )
{
    if (phaseInfo == nullptr) return *this;
    assert( 0 < p && p <= _phase.size() );
    _phase[p-1].set_DOM(phaseInfo);
    return *this;
}


Entry& 
Entry::add_forwarding( Entry* to_entry, LQIO::DOM::Call * call )
{
    if ( !to_entry->test_and_set_recv( Entry::Type::RENDEZVOUS ) ) return *this;

    /* Do some checks for sanity */
    if ( task()->is_reference_task() ) {
	LQIO::input_error2( LQIO::ERR_REF_TASK_FORWARDING, task()->name(), name() );
    } else {
	_fwd.store_target_info( to_entry, call );
    }
    return *this;
}



Entry&
Entry::accumulate_data()
{
    r_cycle.accumulate();
    for_each( _phase.begin(), _phase.end(), Exec<Activity>( &Activity::accumulate_data ) );

    /* Forwarding */

    if ( is_rendezvous() ) {
	_fwd.accumulate_data();
    }
    return *this;
}

Entry&
Entry::reset_stats()
{
    r_cycle.reset();
    for_each( _phase.begin(), _phase.end(), Exec<Activity>( &Activity::reset_stats ) );

    /* Forwarding */
	    
    if ( is_rendezvous() ) {
	_fwd.reset_stats();
    }
    return *this;
}


Entry&
Entry::insertDOMResults() 
{
    double sum_cycle          = 0.0;
    double sum_cycle_var      = 0.0;
    double sum_task_util      = 0.0;
    double sum_task_util_var  = 0.0;
    double sum_proc_util      = 0.0;
    double sum_proc_util_var  = 0.0;

    /*
     * Entry results
     */

    double t = minimum_service_time();
    if ( t > 0. ) {
	_dom->setResultThroughputBound( 1.0 / t );
    }

    for ( unsigned p = 1; p <= task()->max_phases(); ++p ) {
	Activity * phase = &_phase[p-1];
	if ( !is_activity() ) { 
	    if ( phase->is_specified() ) {
		phase->insertDOMResults();
	    }
	} else {
	    _dom->setResultPhasePServiceTime( p, phase->r_cycle.mean() )
		.setResultPhasePVarianceServiceTime( p, phase->r_cycle_sqr.mean() )
		.setResultPhasePUtilization( p, phase->r_util.mean() )
		.setResultPhasePProcessorWaiting(p,phase->r_proc_delay.mean());
	    if ( number_blocks > 1 ) {
		_dom->setResultPhasePServiceTimeVariance( p, phase->r_cycle.variance() )
		    .setResultPhasePVarianceServiceTimeVariance( p, phase->r_cycle_sqr.variance() )
		    .setResultPhasePUtilizationVariance( p, phase->r_util.variance() )
		    .setResultPhasePProcessorWaitingVariance(p,phase->r_proc_delay.variance());
	    }
	}

	sum_task_util      += phase->r_util.mean();
	sum_task_util_var  += phase->r_util.variance();
	sum_cycle          += phase->r_cycle.mean();
	sum_cycle_var      += phase->r_cycle_sqr.mean();
	sum_proc_util      += phase->r_cpu_util.mean();
	sum_proc_util_var  += phase->r_cpu_util.variance();
    }

    /*
     * Service times.
     */
	    
    if (is_activity()) {
	for ( unsigned p = 1; p <= 2; ++p ) {
	    Activity * phase = &_phase[p-1];
	    _dom->setResultPhasePServiceTime( p, phase->r_cycle.mean() )
		.setResultPhasePVarianceServiceTime( p, phase->r_cycle_sqr.mean() )
		.setResultPhasePUtilization( p, phase->r_util.mean() );
	    if ( number_blocks > 1 ) {
		_dom->setResultPhasePServiceTimeVariance( p, phase->r_cycle.variance() )
		    .setResultPhasePVarianceServiceTimeVariance( p, phase->r_cycle_sqr.variance() )
		    .setResultPhasePUtilization( p, phase->r_util.variance() );
	    }
	    if ( phase->_hist_data ) {
		phase->_hist_data->insertDOMResults();
	    }
	}
    }

    /*
     * Entry results (regardless of phases/activities)
     */

    _dom->setResultThroughput(throughput())
	.setResultUtilization(sum_task_util)
	.setResultProcessorUtilization(sum_proc_util);
    if ( number_blocks > 1 ) {
	_dom->setResultThroughputVariance(throughput_variance())
	    .setResultUtilizationVariance(sum_task_util_var)
	    .setResultProcessorUtilizationVariance(sum_proc_util_var);	
    }

    if ( sum_cycle > 0.0 ) {
	_dom->setResultSquaredCoeffVariation(sum_cycle_var/square(sum_cycle));
    }
	      
    _fwd.insertDOMResults();

    /* Open arrivals are done in Task::PseudoTask */
    return *this;
}


/*
 * Minimum service time for an entry is the time bewtween the receive and reply (phase 1).
 * For Reference tasks, it's the total time.
 */

double
Entry::compute_minimum_service_time()
{
    if ( minimum_service_time() == 0. ) {
	if ( is_regular() ) {
	    for ( std::vector<Activity>::iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
		const size_t p = phase - _phase.begin();
		_minimum_service_time[p] = phase->compute_minimum_service_time();
	    }
	} else {
	    std::deque<Activity *> activity_stack;
	    ActivityList::Collect data( this, &Activity::compute_minimum_service_time );
	    _activity->collect( activity_stack, data );
	}
	if ( debug_flag ) {
	    (void) fprintf( stderr, "Entry %s: minimum service time=", name() );
	    for ( std::vector<double>::const_iterator i = _minimum_service_time.begin(); i != _minimum_service_time.end(); ++i ) {
		if ( i != _minimum_service_time.begin() ) (void) fputc( ',', stderr );
		(void) fprintf( stderr, "%g", *i );
	    }
	    (void) fputc( '\n', stderr );
	}
    }

    double sum = 0.0;
    if ( task()->type() == Task::Type::CLIENT || open_arrival_rate() != 0. ) {
	for( std::vector<double>::const_iterator i = _minimum_service_time.begin(); i != _minimum_service_time.end(); ++i ) {
	    sum += *i;
	}
    }
    return sum;
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


double
Entry::minimum_service_time() const
{
    double sum = 0.;
    for ( std::vector<double>::const_iterator i = _minimum_service_time.begin(); i != _minimum_service_time.end(); ++i ) {
	sum += *i;
    }
    return sum;
}


/*----------------------------------------------------------------------*/
/*	 Input processing.  Called from load.cc::prepareModel() 	*/
/*----------------------------------------------------------------------*/

/*
 * A Pseudo entry is used to source requests to open arrivals at real entries.
 * The DOM for the pseudo entry is the DOM for the actual entry.
 */

Pseudo_Entry::Pseudo_Entry( LQIO::DOM::Entry * dom, Task * task ) 
    : Entry (dom,task), _name(task->name()) 
{
}

double
Pseudo_Entry::configure()
{
    assert( get_DOM() && get_DOM()->hasOpenArrivalRate() );

    double arrival_rate = 1.;
    try {
	arrival_rate = get_DOM()->getOpenArrivalRateValue();
	if ( arrival_rate == 0.0 ) throw std::domain_error( "zero" );
    }
    catch ( const std::domain_error& e ) {
	solution_error( LQIO::ERR_INVALID_PARAMETER, "arrival rate", "entry", name(), e.what() );
	throw_bad_parameter();
    }
    _phase[0].set_arrival_rate( 1.0 / arrival_rate );

    return Entry::configure();
}


/*
 * The Pseudo Entry DOM is the actual entry's DOM.  This code only
 * sets the open arrival waiting time from the values stored in the
 * pseudo entry. See Pseudo_Task.
 */

Entry&
Pseudo_Entry::insertDOMResults()
{
    for ( std::vector<tar_t>::iterator tp = _phase[0].tinfo.target.begin(); tp != _phase[0].tinfo.target.end(); ++tp ) {
	Entry * ep = tp->entry;
	LQIO::DOM::Entry * dom = ep->get_DOM();
	assert( dom == get_DOM() );
	dom->setResultWaitingTime( tp->mean_delay() );
	if ( number_blocks > 1 ) {
	    dom->setResultWaitingTimeVariance( tp->variance_delay() );
	}
    }
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
	std::set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( entry_name ) );
	if ( nextEntry != entry.end() ) {
	    LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", entry_name );
	} else {
	    ep = new Entry( domEntry, task );
	    ::entry.insert( ep );
	    ep->add_open_arrival_task();
	}
    }
    return ep;
}



/*
 * Initialization code done BEFORE the simulation starts.  Store the
 * open arrival rate for entry.  This act is accomplished by setting
 * up a fake task to generate open arrivals.
 */

Entry&
Entry::add_open_arrival_task()
{
    if ( !_dom || !_dom->hasOpenArrivalRate() || !test_and_set_recv( Entry::Type::SEND_NO_REPLY ) || dynamic_cast<Pseudo_Entry *>(this) != NULL ) return *this;

    char * task_name = new char[strlen( name() ) + 20];
    (void) sprintf( task_name, "(%s)", name() );
    Task * cp = new Pseudo_Task( task_name );
    ::task.insert( cp );
	
    Entry * from_entry = new Pseudo_Entry( _dom, cp );
    from_entry->initialize();
    ::entry.insert( from_entry );

    /* Set up entry information for my arrival rate. */

    from_entry->test_and_set( LQIO::DOM::Entry::Type::STANDARD );

    /* Set up a task to handle it */

    cp->_entry.push_back( from_entry );

    /* Set up calls per cycle.  1 call is made per cycle */

    from_entry->_phase[0].tinfo.store_target_info( this, 1.0 );

    open_arrival_count += 1;

    return *this;
}


void 
Entry::add_call( const unsigned int p, LQIO::DOM::Call* domCall )
{
    /* Begin by extracting the from/to DOM entries from the call and their names */
    assert( 0 < p && p <= MAX_PHASES );
    const LQIO::DOM::Entry* toDOMEntry = domCall->getDestinationEntry();

    /* Make sure this is one of the supported call types */
    if (domCall->getCallType() != LQIO::DOM::Call::Type::SEND_NO_REPLY && 
	domCall->getCallType() != LQIO::DOM::Call::Type::RENDEZVOUS ) {
	abort();
    }
	
    /* Internal Entry references */
    const char* to_entry_name = toDOMEntry->getName().c_str();
    Entry * to_entry = Entry::find( to_entry_name );
    if ( !to_entry ) return;
    if ( !test_and_set( LQIO::DOM::Entry::Type::STANDARD ) ) return;
    if ( domCall->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS && !to_entry->test_and_set_recv( Entry::Type::RENDEZVOUS ) ) return;
    if ( domCall->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY && !to_entry->test_and_set_recv( Entry::Type::SEND_NO_REPLY ) ) return;

    _phase.at(p-1).tinfo.store_target_info( to_entry, domCall );
}

/*
 * Locate the entry.  Return nil on error.
 */

Entry *
Entry::find( const char * entry_name )
{
    std::set<Entry *,ltEntry>::const_iterator nextEntry = find_if( ::entry.begin(), ::entry.end(), eqEntryStr( entry_name ) );
    if ( nextEntry == ::entry.end() ) {
	input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	return nullptr;
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

    if ( _fwd.size() > 0 ) {
	fprintf( stddbg, "\tfwds:  " );
	std::vector<tar_t>::iterator tp;
	for ( tp = _fwd.target.begin(); tp != _fwd.target.end(); ++tp ) {
	    if ( tp != _fwd.target.begin() ) {
		(void) fprintf( stddbg, ", " );
	    }
	    tp->print( stddbg );
	}
	(void) fprintf( stddbg, ".\n" );    
    }
}
