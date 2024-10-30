/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996								*/
/* Octobber 2005							*/
/************************************************************************/

/*
 * Activities are arcs in the graph that do work.
 * Nodes are points in the graph where splits and joins take place.
 *
 * $Id: activity.cc 17403 2024-10-30 01:30:01Z greg $
 */

#include "lqsim.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <numeric>
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/dom_histogram.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include "activity.h"
#include "model.h"
#include "entry.h"
#include "task.h"
#include "errmsg.h"
#include "message.h"
#include "processor.h"

Activity * junk_activity_list = nullptr;

static void activity_cycle_error( std::deque<Activity *>& activity_stack );

static double gamma_dist( const double a, const double b );
static double erlang_dist( const double a, const int m );
/* Distribution computation functions.  All take scale and shape, though shape may be ignored. */
static double rv_constant( const double mean, const double shape );
static double rv_gamma( const double mean, const double shape );
static double rv_exponential( const double mean, const double shape );
static double rv_pareto( const double scale, const double shape );
static double rv_uniform( const double scale, const double shape );
static double rv_hyperexponential( const double a, const double b);

std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> Activity::actConnections;
std::map<LQIO::DOM::ActivityList*, ActivityList *> Activity::domToNative;


/*
 * Initialize fields in activity.
 */

Activity::Activity( Task * cp, LQIO::DOM::Phase * dom )
    : _dom(dom),
      _name(dom ? dom->getName() : ""),     
      _service_time(0.0),
      _cv_sqr(0.0),
      _think_time(0.0),
      _task(cp),
      _phase(0),
      _scale(0.0),
      _shape(1.0),		/* coefficient of variation squared (usually) -- Exponential assumed */
      _distribution(nullptr),
      _index(cp ? cp->_activity.size(): -1),
      _prewaiting(0),
      _reply(),
      _is_reachable(false),
      _is_start_activity(false),
      _input(nullptr),
      _output(nullptr),
      _active(0),
      _cpu_active(0),
      _hist_data(nullptr),
      r_util("Utilization",dom),
      r_cpu_util("Execution",dom),
      r_service("Service",dom),
      r_slices("Slices",dom),
      r_sends("Sends",dom),
      r_proc_delay("Proc delay",dom),
      r_proc_delay_sqr("Pr dly Sqr",dom),
      r_cycle("Cycle Time",dom),
      r_cycle_sqr("Cycle Sqred",dom),
      r_afterQuorumThreadWait("afterQuorumThreadWait Raw Data",dom)
{
    if ( dom && (dom->hasHistogram() || dom->hasMaxServiceTimeExceeded()) ) {
//	    _hist_data = new Histogram( _dom->getHistogram() );
    }
}


Activity::~Activity()
{
    if ( _hist_data ) {
	delete _hist_data;
    }
    _input = nullptr;
    _output = nullptr;

    _reply.clear();
}


Activity&
Activity::rename( const std::string& s ) 
{      
    const_cast<std::string&>(_name) = s;
    return *this;
}


bool
Activity::has_lost_messages() const
{
    return std::any_of(_calls.begin(),_calls.end(), std::mem_fn( &tar_t::dropped_messages ) );
}

double
Activity::service() const
{
    if ( getDOM() == nullptr ) {
	return 0;
    } else if ( task()->type() ==  Task::Type::OPEN_ARRIVAL_SOURCE ) {
	return _service_time;	// Set in entry.cc
    } else {
	try {
	    return getDOM()->getServiceTimeValue();
	}
	catch ( const std::domain_error& e ) {
	    getDOM()->throw_invalid_parameter( "service time", e.what() );
	}
    }
    return 0.;
}


/*
 * Before Parasol start (but after recalculateDynamic).
 */

double
Activity::configure()
{
    const double n_calls = _calls.configure( _dom, (bool)(type() == LQIO::DOM::Phase::Type::STOCHASTIC) );
    double slice;
    
    _active = 0;
    _cpu_active = 0;

    if ( _dom ) {
	try { 
	    _think_time = _dom->getThinkTimeValue();
	}
	catch ( const std::domain_error& e ) {
	    getDOM()->throw_invalid_parameter( "think time", e.what() );
	}
	if ( !_hist_data && (getDOM()->hasHistogram() || getDOM()->hasMaxServiceTimeExceeded()) ) {
	    _hist_data = new Histogram( getDOM()->getHistogram() );
	}
    }

    if ( type() == LQIO::DOM::Phase::Type::DETERMINISTIC ) {
	slice = service() / (n_calls + 1);	/* Spread calls evenly */
    } else if ( n_calls > 0.0 ) {
	slice = service() / n_calls;		/* Geometric distribution */
    } else {
	slice = service();
    }

    if ( slice > Model::max_service ) {
	Model::max_service = slice;
    }
    if ( cv_sqr() < 0 ) {	/* Not initialized by user, assume exponential */
	_cv_sqr = 1.0;
    }

    /* set distribution function here to avoid retesting if elsewhere in local_compute */
    /* Bug_372 -- Adjust mean and cv^2 to scale and shape for Pareto distribution */
    if ( task()->discipline() == SCHEDULE_BURST ) {
	_shape = sqrt( 1.0 / cv_sqr() + 1.0 ) + 1.0;
	_scale = slice * ( _shape - 1.0 ) / _shape;
	_distribution = rv_pareto;
    } else if ( task()->discipline() == SCHEDULE_UNIFORM ) {
	_shape = 0;
	_scale = slice * 2.;			/* Mean of uniform is 1/2 upper limit. */
	_distribution = rv_uniform;
    } else if ( cv_sqr() == 0 ) {
	_distribution = rv_constant;	/* args ignored */
	_shape = 0;
	_scale = slice;
    } else if ( cv_sqr() < 1.0 ) {
	_shape = 1.0 / cv_sqr();
	_scale = slice * cv_sqr();
	_distribution = rv_gamma;
    } else if ( cv_sqr() == 1.0 ) {
	_distribution = rv_exponential;	/* coeff-of-var ignored */
	_scale = slice;
	_shape = 1.0;
    } else {
	_distribution = rv_hyperexponential;
	_scale = slice;
	_shape = cv_sqr();
    }
    if ( debug_flag ) {
	print_debug_info();
    }

    return n_calls;
}


/*
 * After parasol start.
 */

Activity&
Activity::initialize()
{
    r_util.init();
    r_cpu_util.init();
    r_service.init();
    r_slices.init();
    r_sends.init();
    r_proc_delay.init();
    r_proc_delay_sqr.init();
    r_cycle.init();
    r_cycle_sqr.init();
    r_afterQuorumThreadWait.init();

    if ( getDOM() != nullptr) {
	_calls.initialize( name().c_str() );
    }
    return *this;
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */


double
Activity::find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0;

    if ( std::find( activity_stack.begin(), activity_stack.end(), this ) != activity_stack.end() ) {
	activity_cycle_error( activity_stack );
    } else if ( _output ) {
	activity_stack.push_back( this );
	sum += _output->find_children( activity_stack, fork_stack, ep );
	activity_stack.pop_back();
    }
    return sum;
}


double
Activity::collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data )
{
    double sum = 0;

    if ( std::find( activity_stack.begin(), activity_stack.end(), this ) == activity_stack.end() ) {
	_phase = data.phase;
	_is_reachable = true;
	
	sum = (this->*data._f)( data );

	if ( find_reply( data._e ) ) {
	    data.phase = 2;
	}

	if ( _output ) {
	    activity_stack.push_back( this );
	    sum += _output->collect( activity_stack, data );
	    activity_stack.pop_back();
	}
    }
    return sum;
}


/* look at reply list and look for a match */

double Activity::count_replies( ActivityList::Collect& data ) const
{
    Task * cp = data._e->task();
    const Entry * ep = data._e;
    if ( find_reply( ep ) ) {
	if ( data.phase >= 2 ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_DUPLICATE, ep->name().c_str() );
	} else if ( !data.can_reply || data.rate > 1.0 ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_FROM_BRANCH, ep->name().c_str() );
	} else if ( ep->is_send_no_reply() ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_FOR_SNR_ENTRY, ep->name().c_str() );
	} else {
	    return data.rate;
	}
    } else if ( cp->max_phases() < data.phase ) {
	cp->max_phases( data.phase );
    }
    return 0;
}


/*
 * Find the minimum service time for this activity (this method is also used for phases).
 */

double
Activity::compute_minimum_service_time( std::deque<Entry *>& stack ) const
{
    return service() + std::accumulate( _calls.begin(), _calls.end(), 0.0, [=]( double l, const tar_t& r ){ return l + r.compute_minimum_service_time(const_cast<std::deque<Entry *>&>(stack)); } );
}
    

/*
 * Find the minimum service time for this activity.  For non-reference tasks, it's phase one only.
 */

double
Activity::compute_minimum_service_time( ActivityList::Collect& data ) const
{
    std::deque<Entry *> stack;
    double time = compute_minimum_service_time( stack );
    data._e->_minimum_service_time[data.phase-1] += time;
    return time;
}


/*
 * Reset stats for this activity.
 */

Activity&
Activity::reset_stats()
{
    r_util.reset();
    r_cpu_util.reset();
    r_sends.reset();
    r_slices.reset();
    r_proc_delay.reset();
    r_proc_delay_sqr.reset();

    r_cycle.reset();
    r_cycle_sqr.reset();
    r_service.reset();
    r_afterQuorumThreadWait.reset();	/* tomari quorum */

    _calls.reset_stats();
 
    /* Histogram stuff */
 
    if ( _hist_data ) {
	_hist_data->reset();
    }
    return *this;
}


/*
 * Accumulate stats for this activity.
 */

Activity&
Activity::accumulate_data()
{
    r_util.accumulate();
    r_sends.accumulate();
    r_slices.accumulate();
    r_proc_delay_sqr.accumulate_variance( r_proc_delay.accumulate() );
    r_afterQuorumThreadWait.accumulate();	   /* tomari quorum */
 
    /*
     * Note -- do accummulate_stat LAST for r_cycle as
     * accumulate resets the raw statistic.  r_cycle is used by
     * both accumulate_service_stat and accumulate_variance.
     */

    r_service.accumulate_service( r_cycle );			/* do first! */
    if ( task()->derive_utilization() ) {
	r_cpu_util.accumulate_utilization( r_cycle, service() );
    } else {
	r_cpu_util.accumulate();
    }
    r_cycle_sqr.accumulate_variance( r_cycle.accumulate() );	/* Do last! */
    _calls.accumulate_data();

    /* Histogram stuff */

    if ( _hist_data ) {
	_hist_data->accumulate_data();
    }
    return *this;
}

/* ------------------------------------------------------------------------ */
/*             Model constuction (called from Model::prepare())             */
/* ------------------------------------------------------------------------ */


/* 
 * Go over all of the calls specified within this activity and do something similar to store_snr/rnv 
 */

Activity&
Activity::add_calls()
{
    const std::vector<LQIO::DOM::Call*>& callList = get_calls();
    std::vector<LQIO::DOM::Call*>::const_iterator iter;
	
    /* This provides us with the DOM Call which we can then take apart */
    for (iter = callList.begin(); iter != callList.end(); ++iter) {
	LQIO::DOM::Call* domCall = *iter;
	LQIO::DOM::Entry* toDOMEntry = const_cast<LQIO::DOM::Entry*>(domCall->getDestinationEntry());
	Entry* destEntry = Entry::find(toDOMEntry->getName());
		
	/* Make sure all is well */
	if (!destEntry) {
	    LQIO::input_error( LQIO::ERR_NOT_DEFINED, toDOMEntry->getName().c_str() );
	} else if ( domCall->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS && !destEntry->test_and_set_recv( Entry::Type::RENDEZVOUS ) ) {
	    continue;
	} else if ( domCall->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY && !destEntry->test_and_set_recv( Entry::Type::SEND_NO_REPLY ) ) {
	    continue;
	} else if ( !destEntry->task()->is_reference_task()) {
	    _calls.store_target_info( destEntry, domCall );
	}
    }
    return *this;
}



Activity&
Activity::add_reply_list()
{
    /* This information is stored in the LQIO DOM itself */
    LQIO::DOM::Activity* domActivity = dynamic_cast<LQIO::DOM::Activity *>(_dom);
    const std::vector<LQIO::DOM::Entry*>& domReplyList = domActivity->getReplyList();
    if (domReplyList.size() == 0) {
	return * this;
    }
	
    /* Walk over the list and do the equivalent of calling act_add_reply n times */
    std::vector<LQIO::DOM::Entry*>::const_iterator iter;
    for (iter = domReplyList.begin(); iter != domReplyList.end(); ++iter) {
	const LQIO::DOM::Entry* domEntry = *iter;
	const std::string& entry_name = domEntry->getName();
	Entry * ep = Entry::find( entry_name );
	
	if ( !ep ) {
	    LQIO::input_error( LQIO::ERR_NOT_DEFINED, entry_name.c_str() );
	} else if ( ep->task() != task() ) {
	    getDOM()->input_error( LQIO::ERR_WRONG_TASK_FOR_ENTRY, task()->name().c_str() );
	} else {
	    act_add_reply( ep );
	}
    }

    return *this;
}


Activity&
Activity::add_activity_lists()
{  
    /* Obtain the Task and Activity information DOM records */
    LQIO::DOM::Activity* domAct = dynamic_cast<LQIO::DOM::Activity*>(getDOM());
    if (domAct == nullptr) { return *this; }
	
    /* May as well start with the outputToList, this is done with various methods */
    LQIO::DOM::ActivityList* joinList = domAct->getOutputToList();
    ActivityList * localActivityList = nullptr;
    if (joinList != nullptr && domToNative.find(joinList) == domToNative.end()) {
	const std::vector<const LQIO::DOM::Activity*>& list = joinList->getList();
	for (std::vector<const LQIO::DOM::Activity*>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;
	    Activity * nextActivity = task()->find_activity( domAct->getName().c_str() );
	    if ( !nextActivity ) {
		LQIO::input_error( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }
			
	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( joinList->getListType() ) {
	    case LQIO::DOM::ActivityList::Type::JOIN:
		localActivityList = nextActivity->act_join_item( joinList );
		break;
	    case LQIO::DOM::ActivityList::Type::AND_JOIN:
		localActivityList = nextActivity->act_and_join_list( localActivityList, joinList );
		break;
	    case LQIO::DOM::ActivityList::Type::OR_JOIN:
		localActivityList = nextActivity->act_or_join_list( localActivityList, joinList );
		break;
	    default:
		abort();
	    }
	}
		
	/* Create the association for the activity list */
	domToNative[joinList] = localActivityList;
	if (joinList->getNext() != nullptr) {
	    actConnections[joinList] = joinList->getNext();
	}
    }
	
    /* Now we move onto the inputList, or the fork list */
    LQIO::DOM::ActivityList* forkList = domAct->getInputFromList();
    localActivityList = nullptr;
    if (forkList != nullptr && domToNative.find(forkList) == domToNative.end()) {
	const std::vector<const LQIO::DOM::Activity*>& list = forkList->getList();
	for (std::vector<const LQIO::DOM::Activity*>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;
	    Activity * nextActivity = task()->find_activity( domAct->getName().c_str() );
	    if ( !nextActivity ) {
		LQIO::input_error( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }
			
	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( forkList->getListType() ) {
	    case LQIO::DOM::ActivityList::Type::FORK:	
		localActivityList = nextActivity->act_fork_item( forkList );
		break;
	    case LQIO::DOM::ActivityList::Type::AND_FORK:
		localActivityList = nextActivity->act_and_fork_list( localActivityList, forkList  );
		break;
	    case LQIO::DOM::ActivityList::Type::OR_FORK:
		localActivityList = nextActivity->act_or_fork_list( localActivityList, forkList );
		break;
	    case LQIO::DOM::ActivityList::Type::REPEAT:
		localActivityList = nextActivity->act_loop_list( localActivityList, forkList );
		break;
	    default:
		abort();
	    }
	}
		
	/* Create the association for the activity list */
	domToNative[forkList] = localActivityList;
	if (forkList->getNext() != nullptr) {
	    actConnections[forkList] = forkList->getNext();
	}
    }

    return *this;
}


/*
 * Add the entry list to the activity.
 */


Activity&
Activity::act_add_reply ( Entry * ep )
{
    _reply.push_back( ep );
    return *this;
}


/*
 * Debugging function.
 */

void
Activity::print_debug_info()
{
	
    (void) fprintf( stddbg, "----------\n%s, phase %d %s\n", name().c_str(), _phase, type() == LQIO::DOM::Phase::Type::DETERMINISTIC ? "Deterministic" : "" );

    (void) fprintf( stddbg, "\tservice: %5.2g {%5.2g, %5.2g}\n", service(), _shape, _scale );

    if ( _calls.size() > 0 ) {
	fprintf( stddbg, "\tcalls:  " );
	for ( Targets::const_iterator tp = _calls.begin(); tp != _calls.end(); ++tp ) {
	    if ( tp != _calls.begin() ) {
		(void) fprintf( stddbg, ", " );
	    }
	    tp->print( stddbg );
	}
	(void) fprintf( stddbg, ".\n" );
    }

    if ( is_activity() ) {
	print_activity_connectivity( stddbg, this );
    }
}


/*
 * Save the results.  Note that activities in the simulator are used
 * for phases too; this is not the case in the analytic solver.
 * Therefore, we have to figure out whether we're a phase or an
 * activity dynamically here using RTTI on the DOM object (rather than
 * through sub-classing).
 */

Activity&
Activity::insertDOMResults()
{
    _dom->setResultServiceTime(r_cycle.mean())
	.setResultVarianceServiceTime(r_cycle_sqr.mean())
	.setResultUtilization(r_util.mean())
	.setResultProcessorWaiting(r_proc_delay.mean());
    if ( number_blocks > 1 ) {
	_dom->setResultServiceTimeVariance(r_cycle.variance())
	    .setResultVarianceServiceTimeVariance(r_cycle_sqr.variance())
	    .setResultUtilizationVariance(r_util.variance())
	    .setResultProcessorWaitingVariance(r_proc_delay.variance());
    }

    if ( is_activity() ) {
	LQIO::DOM::Activity * dom_activity = dynamic_cast<LQIO::DOM::Activity *>(_dom);
	dom_activity->setResultThroughput(r_cycle.mean_count()/Model::block_period())
	    .setResultProcessorUtilization(r_cpu_util.mean());
	    if ( number_blocks > 1 ) {
		dom_activity->setResultThroughputVariance(r_cycle.variance_count()/Model::block_period())
		    .setResultProcessorUtilizationVariance(r_cpu_util.variance());
	    }
    }

    if ( _hist_data ) {
	_hist_data->insertDOMResults();
    }

    _calls.insertDOMResults();
    return *this;
}

/*
 * Add activity to the sequence list.  
 */

ActivityList *
Activity::act_join_item( LQIO::DOM::ActivityList * dom )
{
    OutputActivityList * list = nullptr;

    if ( _output ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, _output->getDOM()->getLineNumber() );
    } else {
	Task * cp = task();
	list = new OutputActivityList( ActivityList::Type::JOIN_LIST, dom );
	list->push_back( this );
	cp->add_list( list );
    }

    _output = list;
    return list;
}


/*
 * Add activity to the activity_list.  This list is for AND joining.
 */

ActivityList *
Activity::act_and_join_list ( ActivityList* input_list, LQIO::DOM::ActivityList * dom )
{
    if ( _output ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, _output->getDOM()->getLineNumber() );
	return input_list;
    } 

    Task * cp = task();

    AndJoinActivityList * list = nullptr;
    if ( input_list == nullptr ) {
	list = new AndJoinActivityList( ActivityList::Type::AND_JOIN_LIST, dom );
	list->push_back( this );
	cp->add_list(list).add_join(list);
    } else {
	list = dynamic_cast<AndJoinActivityList *>(input_list);
	list->push_back( this );
    }
    _output = list;

    if ( list->get_quorum_count() > 0 ) {
	cp->max_phases( 2 );
    }
          
    return list;
}



/*
 * Add activity to the activity_list.  This list is for OR joining.
 */

ActivityList *
Activity::act_or_join_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom )
{
    OutputActivityList * list = nullptr;

    if ( _output ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, _output->getDOM()->getLineNumber() );
    } else if ( input_list == nullptr ) {
	Task * cp = task();
	list = new OutputActivityList( ActivityList::Type::OR_JOIN_LIST, dom );
	list->push_back( this );
	cp->add_list( list );
    } else {
	list = dynamic_cast<OutputActivityList *>(input_list);
	list->push_back( this );
    }
    _output = list;
    return list;
}



ActivityList *
Activity::act_fork_item( LQIO::DOM::ActivityList * dom )
{
    ForkActivityList * list = nullptr;

    if ( _is_start_activity ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
    } else if ( _input ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, _input->getDOM()->getLineNumber() );
    } else {
	Task * cp = task();
	list = new ForkActivityList( ActivityList::Type::FORK_LIST, dom );
	list->push_back( this );
	cp->add_list( list );
    }
    _input = list;

    return list;
}


/*
 * Add activity to the activity_list.  This list is for AND forking.
 */

ActivityList *
Activity::act_and_fork_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom )
{
    AndForkActivityList * list = nullptr;

    if ( _is_start_activity ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
    } else if ( _input ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, _input->getDOM()->getLineNumber() );
    } else if ( input_list == nullptr ) {
	Task * cp = task();
	list = new AndForkActivityList( ActivityList::Type::AND_FORK_LIST, dom );
	list->push_back( this );
	cp->add_list(list).add_fork(list);
    } else {
	list = dynamic_cast<AndForkActivityList *>( input_list );
	list->push_back( this );
    }
    _input = list;

    return list;
}



/*
 * Add activity to the activity_list.  This list is for OR forking.
 */

ActivityList *
Activity::act_or_fork_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom )
{
    OrForkActivityList * list = nullptr;

    if ( _is_start_activity ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
    } else if ( _input ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, _input->getDOM()->getLineNumber() );
    } else if ( input_list == nullptr ) {
	Task * cp = task();
	list = new OrForkActivityList( ActivityList::Type::OR_FORK_LIST, dom );
	list->push_back( this );
	cp->add_list(list);
    } else {
	list = dynamic_cast<OrForkActivityList *>(input_list);
	list->push_back( this );
    }
    _input = list;
    return list;
}



/*
 * Add activity to the activity_list.  This list is for Looping.
 */

ActivityList *
Activity::act_loop_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom )
{
    LoopActivityList * list = nullptr;
	  
    if ( _is_start_activity ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
    } else if ( _input ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, _input->getDOM()->getLineNumber() );
    } else if ( input_list == nullptr ) {
	Task * cp = task();
	list = new LoopActivityList( ActivityList::Type::LOOP_LIST, dom );
	cp->add_list( list );
    } else {
	list = dynamic_cast<LoopActivityList *>(input_list);
    }

    if ( list ) {
	if ( dom->getParameter(dynamic_cast<LQIO::DOM::Activity *>(getDOM())) == nullptr ) {
	    list->end_list( this );
	} else {
	    list->push_back( this );
	}
    }
    _input = list;
    return list;
}

/* ---------------------------------------------------------------------- */

/*
 * Check the reply list for entry ep.
 */


bool
Activity::find_reply( const Entry * ep ) const
{
    return  std::find( _reply.begin(), _reply.end(), ep ) != _reply.end();
}


/*
 * Print out raw data.
 */

std::ostream&
Activity::print( std::ostream& output ) const
{
    if ( r_cycle.has_results() ) {
	output << r_util
	       << r_service
	       << r_slices
	       << r_sends
	       << r_cpu_util
	       << r_proc_delay
	       << r_proc_delay_sqr
	       << r_cycle
	       << r_cycle_sqr
	       << r_afterQuorumThreadWait;
    }

    _calls.print( output );
    return output;
}


static void
activity_cycle_error( std::deque<Activity *>& activity_stack )
{
    std::string buf;
    Activity * ap = activity_stack.back();
    
    for ( std::deque<Activity *>::const_reverse_iterator i = activity_stack.rbegin(); i != activity_stack.rend(); ++i ) {
	if ( i != activity_stack.rbegin() ) {
	    buf += ", ";
	}
	buf += (*i)->name();
    }
    ap->task()->getDOM()->runtime_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, buf.c_str() );
}

/*----------------------------------------------------------------------*/
/*		       Distribution functions.				*/
/*----------------------------------------------------------------------*/

/*
 * Returns a RV with a gamma distribution.  See Jain, P490 for details.
 */

static double
gamma_dist ( const double a, const double b )
{

    if ( b <= 0.0 ) {
	(void) fprintf( stderr,  "Bogus `b' for gamma distribution function" );
	abort();
    } else if ( b < 1.0 ) {

	/* Beta Distribution, Pg 485. a & b < 1 */

	double x;
	double y;
	do {
	    x = pow( drand48(), 1.0 / b );
	    y = pow( drand48(), 1.0 / (1.0 - b) );
	} while ( x + y > 1.0 );
	return a * x / ( x + y ) * ps_exponential( 1.0 );
    } else {
	double diff = b - floor( b );
	double temp = erlang_dist( a, (int)b );
	if ( diff > 1.0e-5 ) {
	    temp += gamma_dist( a, diff );
	}
	return temp;
    }
    /*NOTREACHED*/
}


/*
 * Returns a RV with a erlang distribution.  See Jain, P488 for details.
 */

static double
erlang_dist( const double a, const int m )
{
    double	prod;
    int	i;

    prod = 1.0;
    for( i = 0; i < m; i++ ) {
	prod *= drand48();
    }
    return -a * log(prod);
}



/*
 * Returns a constant.
 */

static double
rv_constant( const double scale, const double shape )
{
    return scale;
}

/*
 * Returns a uniformly distributed random variable.
 */

static double
rv_uniform( const double scale, const double shape )
{
    return scale * drand48();
}

/*
 * Returns an RV with gamma distribution.
 */

static double
rv_gamma( const double scale, const double shape )
{
    return gamma_dist( scale, shape );;
}


/*
 * Returns a RV with an exponential distribution. 
 */

static double
rv_exponential( const double scale, const double shape )
{
    return ps_exponential( scale );		/* This is macro in parasol */
}


/*
 * returns a psuedo-random variate from Morse's two-stage hyperexponential distribution.
 */

static double
rv_hyperexponential( const double mean, const double cv_sqr )
{
#if 0
    if ( drand48() <= 0.5 / (cv_sqr - 0.5) ) {
	return ps_exponential( mean * cv_sqr );
    } else {
	return ps_exponential( mean / 2.0 );
    }
#else
    const double prob = 0.5 * (1.0 - (std::sqrt((cv_sqr-1.0)/(cv_sqr+1.0))));
    const double temp = (drand48()>prob) ? (mean/(1.0-prob)) : (mean/prob);
    return -0.5 * temp * log(drand48());
#endif
}


/*
 * Returns a RV with a Pareto distribution.  See Jain, P495.
 */

static double
rv_pareto( const double scale, const double shape )
{
    return scale * pow( 1.0 - drand48(), -1.0 / shape );
}
