/* target.cc	-- Greg Franks Tue Jun 23 2009
 *
 * ------------------------------------------------------------------------
 * $Id: target.cc 17501 2024-11-27 21:32:50Z greg $
 * ------------------------------------------------------------------------
 */

#include "lqsim.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <lqio/error.h>
#include "entry.h"
#include "entry.h"
#include "errmsg.h"
#include "instance.h"
#include "message.h"
#include "pragma.h"
#include "target.h"
#include "task.h"

Call::Call( Entry * entry, LQIO::DOM::Call * dom ) 
    : r_delay("Wait",dom),
      r_delay_sqr("Wait sq",dom),
      r_loss_prob("Loss",dom),
      _entry(entry), _link(-1), _tprob(0.0), _calls(0.0), _reply(false), _call(dom)
{
    _reply = (dom->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS || dom->getCallType() == LQIO::DOM::Call::Type::FORWARD);
}

/*
 * Special constructor for open arrivals.
 */

Call::Call( Entry * entry, double calls )
    : r_delay("Wait",nullptr),
      r_delay_sqr("Wait sqr",nullptr),
      r_loss_prob("Loss",nullptr),
      _entry(entry), _link(-1), _tprob(0.0), _calls(calls), _reply(false), _call(nullptr)
{
}


void
Call::initialize()
{
}



/*
 * Rendezvous message.  Recycle message.
 */

void
Call::send_synchronous( const Entry * src, const int priority, const long reply_port )
{
    long j1; 			/* junk args			*/
    long acceptor_port;		/* Std port of acceptor.	*/
    Message *acceptor_id;	/* id str of msg acceptor.	*/
    double time_stamp;		/* Time of reception.		*/
    Message msg( src, this );

#if HAVE_PARASOL
    Instance::Instance * ip = object_tab[ps_myself];

    ip->timeline_trace( SYNC_INTERACTION_INITIATED, src, _entry );
    if ( link() >= 0 ) {	/* !!!SEND!!! */
	ps_link_send( _link, _entry->get_port(),
		      _entry->entry_id(),
		      LINKS_MESSAGE_SIZE,
		      (char *)&msg, reply_port );
    } else if ( ps_send_priority( _entry->get_port(), _entry->entry_id(),
			       (char *)&msg, reply_port,
			       priority + _entry->priority() ) == SYSERR ) {
	throw std::runtime_error( "Call::send_synchronous" );
    }

    ps_my_schedule_time = ps_now;		/* In case we don't block...	*/

    ps_receive( reply_port, NEVER, &j1, &time_stamp, (char **)&acceptor_id, &acceptor_port );
    ip->timeline_trace( SYNC_INTERACTION_COMPLETED, src, acceptor_id->client );
#endif
}



/*
 * Send-no-reply message.  Do not forget to allocate a new message
 * for each message send.
 */

void
Call::send_asynchronous( const Entry * src, const int priority )
{
#if HAVE_PARASOL
    Entry * dst = _entry;
    Task * cp = dst->task();
    Message * msg = cp->alloc_message();
    if ( msg != nullptr ) {
	msg->init( src, this );

	r_loss_prob.record( 0 );
	Instance::Instance * ip = object_tab[ps_myself];
	ip->timeline_trace( ASYNC_INTERACTION_INITIATED, src, _entry );

	if ( link() >= 0 ) {	/* !!!SEND!!! */
	    ps_link_send( _link, _entry->get_port(), _entry->entry_id(),
			  LINKS_MESSAGE_SIZE, (char *)msg, -1 );
	} else if ( ps_send_priority( _entry->get_port(), _entry->entry_id(),
				      (char *)msg, -1,
				      priority + _entry->priority() ) == SYSERR ) {
	    throw std::runtime_error( "Call::send_asynchronous" );
	}
    } else {
	r_loss_prob.record( 1 );
	if ( Pragma::__pragmas->abort_on_dropped_message() ) {
	    LQIO::runtime_error( ERR_MSG_POOL_EMPTY, src->name().c_str(), _entry->name().c_str() );
	    throw std::runtime_error( "Call::send_asynchronous" );
	}
    }
#endif
}

void
Call::configure()
{
    if ( _entry->task()->is_reference_task() ) {
	_entry->task()->getDOM()->runtime_error( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, _entry->name().c_str() );
    } else if ( _call != nullptr ) {
	try { 
	    _calls = _call->getCallMeanValue();
	    if ( _call->getCallType() != LQIO::DOM::Call::Type::FORWARD && dynamic_cast<const LQIO::DOM::Phase *>(_call->getSourceObject())->getPhaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC && _calls != std::rint( _calls ) ) {
		throw std::domain_error( "invalid integer" );
	    } else if ( _call->getCallType() == LQIO::DOM::Call::Type::FORWARD && _calls > 1.0 ) {
		throw std::domain_error( "invalid probability" );
	    }
	}
	catch ( const std::domain_error &e ) {
	    _call->throw_invalid_parameter( "mean value", e.what() );
	}
    }
}



bool 
Call::dropped_messages() const
{ 
    return !reply() && r_loss_prob.mean() > 0.005; 
}


double
Call::mean_delay() const
{
    if ( dropped_messages() ) {
	return std::numeric_limits<double>::infinity();
    } else {
	return r_delay.mean();
    }
}

double
Call::variance_delay() const
{
    if ( dropped_messages() ) {
	return std::numeric_limits<double>::infinity();
    } else {
	return r_delay.variance();
    }
}


/*
 * return phase 1 service time.
 */

double
Call::compute_minimum_service_time( std::deque<Entry *>& stack ) const
{
    if ( reply() ) {
	if ( entry()->_minimum_service_time[0] == 0. ) {
	    entry()->compute_minimum_service_time( stack );
	}
	return calls() * entry()->_minimum_service_time[0];
    } else {
	return 0.0;
    }
}

FILE *
Call::print( FILE * output ) const
{
    (void) fprintf( output, "%5.2f * %s", calls(), _entry->name().c_str() );
    return output;
}



Call&
Call::insertDOMResults()
{
    if ( _call == nullptr ) return *this;

    double meanDelay, meanDelayVariance;
    const double meanLossProbability = r_loss_prob.mean();

    if ( reply() || meanLossProbability < 0.005 ) {
	meanDelay = r_delay.mean();
	meanDelayVariance = r_delay_sqr.mean();
    } else {
	meanDelay = std::numeric_limits<double>::infinity();
	meanDelayVariance = std::numeric_limits<double>::infinity();
    }
    _call->setResultWaitingTime(meanDelay)
	.setResultVarianceWaitingTime(meanDelayVariance);
    if ( !reply() ) {
	_call->setResultDropProbability( meanLossProbability );
    }

    if ( number_blocks > 1 ) {

	double varDelay, varDelayVariance;
	const double meanLossVariance = r_loss_prob.variance();

	if ( reply() || r_loss_prob.mean() < 0.005 ) {
	    varDelay  = r_delay.variance();
	    varDelayVariance  = r_delay_sqr.variance();
	} else {
	    varDelay = std::numeric_limits<double>::infinity();
	    varDelayVariance = std::numeric_limits<double>::infinity();
	}

	_call->setResultWaitingTimeVariance(varDelay)
	    .setResultVarianceWaitingTimeVariance(varDelayVariance);
	if ( !reply() ) {
	    _call->setResultDropProbabilityVariance( meanLossVariance );
	}
    }
    return *this;
}


std::ostream&
Call::print( std::ostream& output ) const
{
    output << r_delay
	   << r_delay_sqr;
    if ( !reply() ) {
	output << r_loss_prob;
    }
    return output;
}

/*
 * Store target information.  Mallocate space as needed.
 */

void
Targets::store_target_info( Entry * to_entry, LQIO::DOM::Call* call  )
{
    if ( std::any_of( _target.begin(), _target.end(), [=]( const Call& target ){ return target.entry() == to_entry; } ) ) {
	call->input_error( LQIO::WRN_MULTIPLE_SPECIFICATION );
    } else {
	_target.emplace_back( Call( to_entry, call ) );
    }
}


/*
 * Store target information.  Mallocate space as needed.
 */

void
Targets::store_target_info( Entry * to_entry, double value )
{
    if ( std::any_of( _target.begin(), _target.end(), [=]( const Call& target ){ return target.entry() == to_entry; } ) ) {
//	call->input_error( LQIO::WRN_MULTIPLE_SPECIFICATION );
    } else if ( value > 0 ) {
	_target.emplace_back( Call( to_entry, value ) );
    }
}



/*
 * Compute the PDF for arg.  If normalize is true, the 'calls' field of
 * tinfo_ptr->target is not normalized, so perform a normalization step.
 * In no calls are made at all, tprob is set to zero and sum is set to 1.0.
 * Otherwise, validate sum for proper probability.  Return the sum.
 */

double
Targets::configure( LQIO::DOM::Phase::Type type, bool normalize )
{
    _type = type;
    
    std::for_each( _target.begin(), _target.end(), std::mem_fn( &Call::configure ) );
    const double sum = std::accumulate( _target.begin(), _target.end(), static_cast<double>(0.0),
				  []( double sum, Call& target ){ target._tprob = sum + target.calls(); return target._tprob; } );

    if ( _type != LQIO::DOM::Phase::Type::DETERMINISTIC && normalize ) {	// STOCHASTIC conflicts with Parasol.
	/* Normalize STOCHASTIC, but not forwarding. */
	std::for_each( _target.begin(), _target.end(), [=]( Call& target ){ target._tprob /= (sum + 1); } );
    }
    return sum;
}


void
Targets::initialize()
{
    std::for_each( _target.begin(), _target.end(), std::mem_fn( &Call::initialize ) );
}


/*
 * Compute the index of the task to send to.  `i' is the index into
 * the target array and is used to route the message.  `j' is an index
 * used for deterministic phase scheduling.  `i' and `j' returned as
 * they are needed for deterministic scheduling history.
 */

Call *
Targets::get_next_target( std::pair<size_t,size_t>& history ) const
{
    if ( size() == 0 ) return nullptr;
    double p;		/* branch probability		*/
    size_t& i = history.first;
    size_t& j = history.second;
    
    switch ( _type ) {

    case LQIO::DOM::Phase::Type::STOCHASTIC:
	p = Random::number();
	for ( i = 0; i < size() && p >= _target[i]._tprob; i = i + 1 );
	break;

    case LQIO::DOM::Phase::Type::DETERMINISTIC:
	j = j + 1;
	while ( i < size() && j > _target[i].calls() ) {
	    j = 1;
	    i = i + 1;
	}
	break;

    default:
	abort();
    }
    if ( i < size() ) {
	return const_cast<Call *>(&_target[i]);
    }
    return nullptr;
}



Targets&
Targets::accumulate_data()
{
    for ( std::vector<Call>::iterator tp = _target.begin(); tp != _target.end(); ++tp ) {
	tp->r_delay_sqr.accumulate_variance( tp->r_delay.accumulate() );
	if ( !tp->reply() ) {
	    tp->r_loss_prob.accumulate();
	}
    }
    return *this;
}



Targets&
Targets::reset_stats()
{
    for ( std::vector<Call>::iterator tp = _target.begin(); tp != _target.end(); ++tp ) {
 	tp->r_delay.reset();
 	tp->r_delay_sqr.reset();
 	if ( !tp->reply() ) {
 	    tp->r_loss_prob.reset();
	}
    }
    return *this;
}



std::ostream&
Targets::print( std::ostream& output ) const
{
    std::for_each( _target.begin(), _target.end(), [&]( const Call& target ){ target.print( output ); } );
    return output;
}


Targets&
Targets::insertDOMResults()
{
    std::for_each( _target.begin(), _target.end(), std::mem_fn( &Call::insertDOMResults ) );
    return *this;
}
