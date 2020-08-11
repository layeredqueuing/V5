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
 * $Id: activity.cc 13751 2020-08-10 02:27:53Z greg $
 */

#include <parasol.h>
#include "lqsim.h"
#include <cstdarg>
#include <algorithm>
#include <string.h>
#include <limits.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/dom_histogram.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include "model.h"
#include "entry.h"
#include "activity.h"
#include "task.h"
#include "errmsg.h"
#include "message.h"
#include "processor.h"

Activity * junk_activity_list = 0;
unsigned join_count = 0;		/* non-zero if any joins	*/
unsigned fork_count = 0;		/* non-zero if any forks	*/

static void activity_cycle_error( int, std::deque<Activity *>& activity_stack );

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
      _arrival_rate(0.0),
      _cv_sqr(0.0),
      _think_time(0.0),
      _task(cp),
      _phase(0),
      _scale(0.0),
      _shape(1.0),		/* coefficient of variation squared (usually) -- Exponential assumed */
      distribution_func(0),
      _index(cp ? cp->_activity.size(): -1),
      _prewaiting(0),
      _reply(),
      _is_reachable(false),
      _is_start_activity(false),
      _input(NULL),
      _output(NULL),
      _active(0),
      _cpu_active(0),
      _hist_data(0)
{
}


Activity::~Activity()
{
    if ( _hist_data ) {
	delete _hist_data;
    }
    _input = 0;
    _output = 0;

    _reply.clear();
}


Activity&
Activity::rename( const std::string& s ) 
{      
    const_cast<std::string&>(_name) = s;
    return *this;
}

Activity& 
Activity::set_DOM(LQIO::DOM::Phase* phaseInfo)
{
    _dom = phaseInfo;
    return *this;
}

bool
Activity::has_lost_messages() const
{
    for ( vector<tar_t>::const_iterator tp = tinfo.target.begin(); tp != tinfo.target.end(); ++tp ) {
	if ( (*tp).dropped_messages() ) return true;
    }
    return false;
}

double
Activity::service() const
{
    if (_dom == NULL) return _arrival_rate;		/* Psuedo entry sets this for open arrival */
    try {
	return get_DOM()->getServiceTimeValue();
    }
    catch ( const std::domain_error& e ) {
	solution_error( LQIO::ERR_INVALID_PARAMETER, "service time", get_DOM()->getTypeName(), name(), e.what() );
	throw_bad_parameter();
    }
    return 0.;
}

double
Activity::configure()
{
    const double n_calls = tinfo.compute_PDF( _dom, (bool)(type() == PHASE_STOCHASTIC) );
    double slice;
    
    _active = 0;
    _cpu_active = 0;

    if ( _dom ) {
	try { 
	    _think_time = _dom->getThinkTimeValue();
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "think time", get_DOM()->getTypeName(), name(), e.what() );
	    throw_bad_parameter();
	}
	if ( !_hist_data && (_dom->hasHistogram() || _dom->hasMaxServiceTimeExceeded()) ) {
	    _hist_data = new Histogram( _dom->getHistogram() );
	}
    }

    r_util.init( VARIABLE, "%30.30s Utilization", name() );
    r_cpu_util.init( VARIABLE, "%30.30s Execution  ", name() );
    r_service.init( SAMPLE,   "%30.30s Service    ", name() );
    r_slices.init( SAMPLE,   "%30.30s Slices     ", name() );
    r_sends.init( SAMPLE,   "%30.30s Sends      ", name() );
    r_proc_delay.init( SAMPLE,   "%30.30s Proc delay ", name() );
    r_proc_delay_sqr.init( SAMPLE,   "%30.30s Pr dly Sqr ", name() );
    r_cycle.init( SAMPLE,   "%30.30s Cycle Time ", name() );
    r_cycle_sqr.init( SAMPLE,   "%30.30s Cycle Sqred", name() );
    r_afterQuorumThreadWait.init( SAMPLE,   "%30.30s afterQuorumThreadWait Raw Data", name() );	/* tomari quorum */
	
    if ( type() == PHASE_DETERMINISTIC ) {
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
	distribution_func = rv_pareto;
    } else if ( task()->discipline() == SCHEDULE_UNIFORM ) {
	_shape = 0;
	_scale = slice * 2.;			/* Mean of uniform is 1/2 upper limit. */
	distribution_func = rv_uniform;
    } else if ( cv_sqr() == 0 ) {
	distribution_func = rv_constant;	/* args ignored */
	_shape = 0;
	_scale = slice;
    } else if ( cv_sqr() < 1.0 ) {
	_shape = 1.0 / cv_sqr();
	_scale = slice * cv_sqr();
	distribution_func = rv_gamma;
    } else if ( cv_sqr() == 1.0 ) {
	distribution_func = rv_exponential;	/* coeff-of-var ignored */
	_scale = slice;
	_shape = 1.0;
    } else {
	distribution_func = rv_hyperexponential;
	_scale = slice;
	_shape = cv_sqr();
    }
    if ( debug_flag ) {
	print_debug_info();
    }

    return n_calls;
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

double
Activity::find_children( std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0;

    if ( std::find( activity_stack.begin(), activity_stack.end(), this ) != activity_stack.end() ) {
	activity_cycle_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, activity_stack );
    } else if ( _output ) {
	activity_stack.push_back( this );
	sum += join_find_children( _output, activity_stack, fork_stack, ep );
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
	    sum += join_collect( _output, activity_stack, data );
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
	    LQIO::solution_error( LQIO::ERR_DUPLICATE_REPLY, cp->name(), name(), ep->name() );
	} else if ( !data.can_reply || data.rate == 0 || data.rate > 1.0 ) {
	    LQIO::solution_error( LQIO::ERR_INVALID_REPLY, cp->name(), name(), ep->name() );
	} else if ( ep->is_send_no_reply() ) {
	    LQIO::solution_error( LQIO::ERR_REPLY_SPECIFIED_FOR_SNR_ENTRY, cp->name(), name(), ep->name() );
	} else {
	    return data.rate;
	}
    } else if ( cp->max_phases < data.phase ) {
	cp->max_phases = data.phase;
    }
    return 0;
}


/*
 * Find the minimum service time for this activity (this method is also used for phases).
 */

double
Activity::compute_minimum_service_time() const
{
    double sum = for_each( tinfo.target.begin(), tinfo.target.end(), ConstExecSum<const tar_t,double>( &tar_t::compute_minimum_service_time ) ).sum();
    return service() + sum;
}
    

/*
 * Find the minimum service time for this activity.  For non-reference tasks, it's phase one only.
 */

double
Activity::compute_minimum_service_time( ActivityList::Collect& data ) const
{
    double time = compute_minimum_service_time();
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

    tinfo.reset_stats();
 
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
    tinfo.accumulate_data();

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
	Entry* destEntry = Entry::find(toDOMEntry->getName().c_str());
		
	/* Make sure all is well */
	if (!destEntry) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, toDOMEntry->getName().c_str() );
	} else if ( domCall->getCallType() == LQIO::DOM::Call::RENDEZVOUS && !destEntry->test_and_set_recv( Entry::RECEIVE_RENDEZVOUS ) ) {
	    continue;
	} else if ( domCall->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY && !destEntry->test_and_set_recv( Entry::RECEIVE_SEND_NO_REPLY ) ) {
	    continue;
	} else if ( !destEntry->task()->is_reference_task()) {
	    tinfo.store_target_info( destEntry, domCall );
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
	const char * entry_name = domEntry->getName().c_str();
	Entry * ep = Entry::find( entry_name );
	
	if ( !ep ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	} else if ( ep->task() != task() ) {
	    LQIO::input_error2( LQIO::ERR_WRONG_TASK_FOR_ENTRY, entry_name, task()->name() );
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
    LQIO::DOM::Activity* domAct = dynamic_cast<LQIO::DOM::Activity*>(get_DOM());
    if (domAct == NULL) { return *this; }
	
    /* May as well start with the outputToList, this is done with various methods */
    LQIO::DOM::ActivityList* joinList = domAct->getOutputToList();
    ActivityList * localActivityList = NULL;
    if (joinList != NULL && joinList->getProcessed() == false) {
	const std::vector<const LQIO::DOM::Activity*>& list = joinList->getList();
	std::vector<const LQIO::DOM::Activity*>::const_iterator iter;
	joinList->setProcessed(true);
	for (iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;
	    Activity * nextActivity = task()->find_activity( domAct->getName().c_str() );
	    if ( !nextActivity ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }
			
	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( joinList->getListType() ) {
	    case LQIO::DOM::ActivityList::JOIN_ACTIVITY_LIST:
		localActivityList = nextActivity->act_join_item( joinList );
		break;
	    case LQIO::DOM::ActivityList::AND_JOIN_ACTIVITY_LIST:
		localActivityList = nextActivity->act_and_join_list( localActivityList, joinList );
		break;
	    case LQIO::DOM::ActivityList::OR_JOIN_ACTIVITY_LIST:
		localActivityList = nextActivity->act_or_join_list( localActivityList, joinList );
		break;
	    default:
		abort();
	    }
	}
		
	/* Create the association for the activity list */
	domToNative[joinList] = localActivityList;
	if (joinList->getNext() != NULL) {
	    actConnections[joinList] = joinList->getNext();
	}
    }
	
    /* Now we move onto the inputList, or the fork list */
    LQIO::DOM::ActivityList* forkList = domAct->getInputFromList();
    localActivityList = NULL;
    if (forkList != NULL && forkList->getProcessed() == false) {
	const std::vector<const LQIO::DOM::Activity*>& list = forkList->getList();
	std::vector<const LQIO::DOM::Activity*>::const_iterator iter;
	forkList->setProcessed(true);
	for (iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;
	    Activity * nextActivity = task()->find_activity( domAct->getName().c_str() );
	    if ( !nextActivity ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }
			
	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( forkList->getListType() ) {
	    case LQIO::DOM::ActivityList::FORK_ACTIVITY_LIST:	
		localActivityList = nextActivity->act_fork_item( forkList );
		break;
	    case LQIO::DOM::ActivityList::AND_FORK_ACTIVITY_LIST:
		localActivityList = nextActivity->act_and_fork_list( localActivityList, forkList  );
		break;
	    case LQIO::DOM::ActivityList::OR_FORK_ACTIVITY_LIST:
		localActivityList = nextActivity->act_or_fork_list( localActivityList, forkList );
		break;
	    case LQIO::DOM::ActivityList::REPEAT_ACTIVITY_LIST:
		localActivityList = nextActivity->act_loop_list( localActivityList, forkList );
		break;
	    default:
		abort();
	    }
	}
		
	/* Create the association for the activity list */
	domToNative[forkList] = localActivityList;
	if (forkList->getNext() != NULL) {
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
	
    (void) fprintf( stddbg, "----------\n%s, phase %d %s\n", name(), _phase, type() == PHASE_DETERMINISTIC ? "Deterministic" : "" );

    (void) fprintf( stddbg, "\tservice: %5.2g {%5.2g, %5.2g}\n", service(), _shape, _scale );

    if ( tinfo.size() > 0 ) {
	fprintf( stddbg, "\tcalls:  " );
	vector<tar_t>::iterator tp;
	for ( tp = tinfo.target.begin(); tp != tinfo.target.end(); ++tp ) {
	    if ( tp != tinfo.target.begin() ) {
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

    tinfo.insertDOMResults();
    return *this;
}

/*
 * Add activity to the sequence list.  
 */

ActivityList *
Activity::act_join_item( LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = 0;

    if ( _output ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, task()->name(), name() );
    } else {
	list = realloc_list( ACT_JOIN_LIST, 0, dom_activitylist );
	list->u.join.quorumCount = 0;
	_output = list;
    }

    return list;
}


/*
 * Add activity to the activity_list.  This list is for AND joining.
 */

ActivityList *
Activity::act_and_join_list ( ActivityList* input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( _output ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, task()->name(), name() );
	return input_list;
    } 

    ActivityList * list = realloc_list( ACT_AND_JOIN_LIST, input_list, dom_activitylist );
    _output = list;

    Task * cp = task();
    if ( list->u.join.quorumCount > 0 && cp->max_phases == 1 ) {
	cp->max_phases++;
    }
          
    /* Tack new and_join_list onto join list for task */

    if ( input_list == 0 ) {
	cp->add_join(list);
	join_count  += 1;	/* Global count */
    }

    return list;
}



/*
 * Add activity to the activity_list.  This list is for OR joining.
 */

ActivityList *
Activity::act_or_join_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;

    if ( _output ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, task()->name(), name() );
    } else {
	list = realloc_list( ACT_OR_JOIN_LIST, input_list, dom_activitylist );
	_output = list;
    }
    return list;
}



ActivityList *
Activity::act_fork_item( LQIO::DOM::ActivityList * dom_activitylist)
{
    ActivityList * list = 0;

    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, task()->name(), name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, task()->name(), name() );
    } else {
	list = realloc_list( ACT_FORK_LIST, 0, dom_activitylist );
	_input = list;
    }

    return list;
}


/*
 * Add activity to the activity_list.  This list is for AND forking.
 */

ActivityList *
Activity::act_and_fork_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;

    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, task()->name(), name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, task()->name(), name() );
    } else {
	list = realloc_list( ACT_AND_FORK_LIST, input_list, dom_activitylist );
	_input = list;

	/* Tack new and_fork_list onto join list for task */

	if ( input_list == 0 ) {
	    Task * cp = task();
	    cp->add_fork(list);
	    fork_count  += 1;	/* Global count */
	}

    }
    return list;
}



/*
 * Add activity to the activity_list.  This list is for OR forking.
 */

ActivityList *
Activity::act_or_fork_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;

    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, task()->name(), name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, task()->name(), name() );
    } else {
	unsigned i;
	double sum = 0.0;
	list = realloc_list( ACT_OR_FORK_LIST, input_list, dom_activitylist );
	if ( list ) {
	    for ( i = 0; i < list->na; ++i ) {
		sum += list->u.fork.prob[i];
	    }
	    if ( sum - 1.0 > EPSILON ) {
		LQIO::input_error2( LQIO::ERR_INVALID_PROBABILITY, sum );
	    }
	}
	_input = list;
    }
    return list;
}



/*
 * Add activity to the activity_list.  This list is for Looping.
 */

ActivityList *
Activity::act_loop_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;
	  
    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, task()->name(), name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, task()->name(), name() );
    } else {
	list = realloc_list( ACT_LOOP_LIST, input_list, dom_activitylist );
	_input = list;
    }
    return list;
}

/* ---------------------------------------------------------------------- */

/*
 * Allocate a list and storage for items in it.  The list will grow if necessary.
 */

ActivityList *
Activity::realloc_list( const list_type type, const ActivityList * input_list, LQIO::DOM::ActivityList * dom_activity_list )
{
    ActivityList * list;
    unsigned i;

    if ( !input_list ) {
	list = static_cast<ActivityList *>(my_malloc( sizeof( ActivityList ) ));
	list->type          = type;
	list->na            = 0;
	list->maxa          = 3;
	list->list          = (Activity **)my_malloc( sizeof( Activity *) * 3);
	list->list[0]       = 0;

	task()->_act_list.push_back( list );

	switch ( type ) {
	case ACT_FORK_LIST:
	case ACT_OR_FORK_LIST:
	case ACT_AND_FORK_LIST:
	    list->u.fork.prev = 0;
	    list->u.fork.join = 0;
	    if ( type == ACT_OR_FORK_LIST ) {
		list->u.fork.prob = (double *)my_malloc( sizeof( double ) * list->maxa );
		for ( i = 0; i < list->maxa; ++i ) {
		    list->u.fork.prob[i] = 0.0;
		}
		list->u.fork.visit = 0;
	    } else if ( type == ACT_AND_FORK_LIST ) {
		list->u.fork.visit = (bool *) my_malloc( sizeof( bool ) * list->maxa );
		for ( i = 0; i < list->maxa; ++i ) {
		    list->u.fork.visit[i] = false;
		}
		list->u.fork.prob  = 0;
	    } else {
		list->u.fork.visit = 0;
		list->u.fork.prob  = 0;
	    }
	    list->u.fork.visit_count = 0;
	    break;

	case ACT_LOOP_LIST:
	    list->u.loop.prev = 0;
	    list->u.loop.endlist = 0;
	    list->u.loop.count = (double *)my_malloc( sizeof( double ) * list->maxa );
	    for ( i = 0; i < list->maxa; ++i ) {
		list->u.loop.count[i] = 0.0;
	    }
	    list->u.loop.total = 0.0;
	    break;

	case ACT_AND_JOIN_LIST:
	case ACT_JOIN_LIST:
	case ACT_OR_JOIN_LIST:
	    list->u.join.next   = 0;
	    if ( type == ACT_AND_JOIN_LIST ) {
		list->u.join.fork = static_cast<ActivityList **>(my_malloc( sizeof( ActivityList * ) * list->maxa ));
		list->u.join.source = static_cast<Activity **>(my_malloc( sizeof( Activity * ) * list->maxa ));
		for ( i = 0; i < list->maxa; ++i ) {
		    list->u.join.fork[i] = 0;
		    list->u.join.source[i] = 0;
		}
		list->u.join._hist_data = 0;
//		list->u.join.hist_data = new Histogram();
		/* Need to allocate stuff for histogram -- need to specify service time somehow...*/
		/* I might need to make the M (histogram option) parameter to be applied
		   to the join list in the input LQN grammar. */

/* !!!		create_histogram(list->u.join.hist_data, 8, hist_alpha); */
	    } else {
		list->u.join.fork = 0;
		list->u.join.source = 0;
	    }
	    list->u.join.join_type   = JOIN_UNDEFINED;
	    list->u.join.quorumCount = 0;
	    break;

	default:
	    abort();
	}

    } else if ( input_list->na >= input_list->maxa ) {

	int old_maxa = input_list->maxa;
	list = const_cast<ActivityList *>(input_list);
	list->maxa += 5;
	list->list = static_cast<Activity **>(realloc( list->list, sizeof( Activity *) * list->maxa ));

	switch ( type ) {
	case ACT_OR_FORK_LIST:
	    list->u.fork.prob = (double *)my_realloc( list->u.fork.prob, sizeof( double ) * list->maxa );
	    for ( i = old_maxa; i < list->maxa; ++i ) {
		list->u.fork.prob[i] = 0;
	    }
	    break;

	case ACT_AND_FORK_LIST:
	    list->u.fork.visit = (bool *)my_realloc( list->u.fork.visit, sizeof( bool ) * list->maxa );
	    for ( i = old_maxa; i < list->maxa; ++i ) {
		list->u.fork.visit[i] = false;
	    }
	    break;

	case ACT_AND_JOIN_LIST:
	    list->u.join.fork = static_cast<ActivityList **>(my_realloc( list->u.join.fork, sizeof( ActivityList * ) * list->maxa ));
	    list->u.join.source = static_cast<Activity **>(my_realloc( list->u.join.source, sizeof( Activity * ) * list->maxa ));
	    for ( i = old_maxa; i < list->maxa; ++i ) {
		list->u.join.fork[i] = 0;
		list->u.join.source[i] = 0;
	    }
	    break;

	case ACT_LOOP_LIST:
	    list->u.loop.count = (double *)my_realloc( list->u.loop.count, sizeof( double ) * list->maxa );
	    for ( i = old_maxa; i < list->maxa; ++i ) {
		list->u.loop.count[i] = 0;
	    }
	    break;

	}

    } else {
	list = const_cast<ActivityList *>(input_list);

    }

    LQIO::DOM::ExternalVariable * count;
    if ( type == ACT_LOOP_LIST && (count = dom_activity_list->getParameter(dynamic_cast<LQIO::DOM::Activity *>(get_DOM()))) == NULL ) {
	list->u.loop.endlist = this;
    } else {
	list->list[list->na] = this;
	list->na += 1;
    }
    list->_dom_actlist = dom_activity_list;

    return list;
}

/* ---------------------------------------------------------------------- */

/*
 * Find the activity.  If not found, create it.
 */

Activity *
find_or_create_activity ( const void * task, const char * activity_name )
{
    abort();
}

/*
 * Check the reply list for entry ep.
 */

const Entry * 
Activity::find_reply( const Entry * ep ) const
{
    for ( vector<const Entry *>::const_iterator rp = _reply.begin(); rp != _reply.end(); ++rp ) {
	if ( (*rp) == ep ) return ep;
    }
    return 0;
}


/*
 * Print out raw data.
 */

const Activity&
Activity::print_raw_stat( FILE * output ) const
{
    if ( r_cycle.has_results() ) {
	r_util.print_raw( output,           "%-24.24s Utilization", name() );
	r_service.print_raw( output,        "%-24.24s Service    ", name() );
	r_slices.print_raw( output,         "%-24.24s Slices     ", name() );
	r_sends.print_raw( output,          "%-24.24s Sends      ", name() );
	r_cpu_util.print_raw( output,       "%-24.24s Proc. Util.", name() );
	r_proc_delay.print_raw( output,     "%-24.24s Proc. delay", name() );
	r_proc_delay_sqr.print_raw( output, "%-24.24s Prc dly sqr", name() );
	r_cycle.print_raw( output,          "%-24.24s Cycle Time ", name() );
	r_cycle_sqr.print_raw( output,      "%-24.24s Cycle Sqred", name() );
	r_afterQuorumThreadWait.print_raw( output, "%-24.24s afterQuorumThreadWait Raw Data", name() );	/* tomari quorum */
    }

    tinfo.print_raw_stat( output );
    return *this;
}


static void
activity_cycle_error( int err, std::deque<Activity *>& activity_stack )
{
    std::string buf;
    Activity * ap = activity_stack.back();
    
    for ( std::deque<Activity *>::const_reverse_iterator i = activity_stack.rbegin(); i != activity_stack.rend(); ++i ) {
	if ( i != activity_stack.rbegin() ) {
	    buf += ", ";
	}
	buf += (*i)->name();
    }
    LQIO::solution_error( err, ap->task()->name(), buf.c_str() );
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
 * Returns a RV with a hyper-exponential distribution.  See:
 *
 * Akylidiz, Ian F. and Sieber, Albrecht, "Approximate Analysis of Load
 * Dependent General Queueing Networks", IEEE Transactions on Software
 * Engineering", Vol 14, No 11, November, 1988.
 */

static double
rv_hyperexponential( const double scale, const double shape )
{
    if ( drand48() <= 0.5 / (shape - 0.5) ) {
	return ps_exponential( scale * shape );
    } else {
	return ps_exponential( scale / 2.0 );
    }
}


/*
 * Returns a RV with a Pareto distribution.  See Jain, P495.
 */

static double
rv_pareto( const double scale, const double shape )
{
    return scale * pow( 1.0 - drand48(), -1.0 / shape );
}
