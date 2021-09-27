/* target.cc	-- Greg Franks Tue Jun 23 2009
 * $HeadURL$
 *
 * ------------------------------------------------------------------------
 * $Id: target.cc 14997 2021-09-27 18:13:17Z greg $
 * ------------------------------------------------------------------------
 */

#include <parasol.h>
#include "lqsim.h"
#include <lqio/error.h>
#include <lqio/input.h>
#include "target.h"
#include "task.h"
#include "entry.h"
#include "errmsg.h"
#include "entry.h"
#include "message.h"
#include "instance.h"
#include "pragma.h"

tar_t::tar_t()
  : entry(0),
    _link(-1),
    _tprob(0.0),
    _calls(0),
    _reply(0),
    _type(undefined)
{
}

/*
 * Rendezvous message.  Recycle message.
 */

void
tar_t::send_synchronous( const Entry * src, const int priority, const long reply_port )
{
    long j1; 			/* junk args			*/
    long acceptor_port;		/* Std port of acceptor.	*/
    Message *acceptor_id;	/* id str of msg acceptor.	*/
    double time_stamp;		/* Time of reception.		*/
    Message msg( src, this );

    Instance * ip = object_tab[ps_myself];

    ip->timeline_trace( SYNC_INTERACTION_INITIATED, src, entry );
    if ( link() >= 0 ) {	/* !!!SEND!!! */
	ps_link_send( _link, entry->get_port(),
		      entry->entry_id(),
		      LINKS_MESSAGE_SIZE,
		      (char *)&msg, reply_port );
    } else if ( ps_send_priority( entry->get_port(), entry->entry_id(),
			       (char *)&msg, reply_port,
			       priority + entry->priority() ) == SYSERR ) {
	throw std::runtime_error( "tar_t::send_synchronous" );
    }

    ps_my_schedule_time = ps_now;		/* In case we don't block...	*/

    ps_receive( reply_port, NEVER, &j1, &time_stamp, (char **)&acceptor_id, &acceptor_port );
    ip->timeline_trace( SYNC_INTERACTION_COMPLETED, src, acceptor_id->client );
}



/*
 * Send-no-reply message.  Do not forget to allocate a new message
 * for each message send.
 */

void
tar_t::send_asynchronous( const Entry * src, const int priority )
{
    Entry * dst = this->entry;
    Task * cp = dst->task();
    Message * msg = cp->alloc_message();
    if ( msg != NULL ) {
	msg->init( src, this );

	ps_record_stat( r_loss_prob.raw, 0 );
	Instance * ip = object_tab[ps_myself];
	ip->timeline_trace( ASYNC_INTERACTION_INITIATED, src, entry );

	if ( link() >= 0 ) {	/* !!!SEND!!! */
	    ps_link_send( _link, entry->get_port(), entry->entry_id(),
			  LINKS_MESSAGE_SIZE, (char *)msg, -1 );
	} else if ( ps_send_priority( entry->get_port(), entry->entry_id(),
				      (char *)msg, -1,
				      priority + entry->priority() ) == SYSERR ) {
	    throw std::runtime_error( "tar_t::send_asynchronous" );
	}
    } else {
	ps_record_stat( r_loss_prob.raw, 1 );
	if ( Pragma::__pragmas->abort_on_dropped_message() ) {
	    LQIO::solution_error( FTL_MSG_POOL_EMPTY, src->name(), entry->name() );
	} else {
	    messages_lost = true;
	}
    }
}

void
tar_t::configure()
{
    if ( _type == call ) {
	_calls = _dom._call->getCallMeanValue();
	_reply = (_dom._call->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS || _dom._call->getCallType() == LQIO::DOM::Call::Type::FORWARD);
    } else if ( _type != constant ) {
	abort();
    }
}



bool 
tar_t::dropped_messages() const
{ 
    return !reply() &&  r_loss_prob.mean() > 0.005; 
}


double
tar_t::mean_delay() const
{
    if ( dropped_messages() ) {
	return Model::get_infinity();
    } else {
	return r_delay.mean();
    }
}

double
tar_t::variance_delay() const
{
    if ( dropped_messages() ) {
	return Model::get_infinity();
    } else {
	return r_delay.variance();
    }
}


/*
 * return phase 1 service time.
 */

double
tar_t::compute_minimum_service_time() const
{
    if ( reply() ) {
	if ( entry->_minimum_service_time[0] == 0. ) {
	    entry->compute_minimum_service_time();
	}
	return calls() * entry->_minimum_service_time[0];
    } else {
	return 0.0;
    }
}

FILE *
tar_t::print( FILE * output ) const
{
    (void) fprintf( output, "%5.2f * %s", calls(), entry->name() );
    return output;
}



void
tar_t::insertDOMResults()
{
    if ( _type != call ) return;

    double meanDelay, meanDelayVariance;
    const double meanLossProbability = r_loss_prob.mean();

    if ( reply() || meanLossProbability < 0.005 ) {
	meanDelay = r_delay.mean();
	meanDelayVariance = r_delay_sqr.mean();
    } else {
	meanDelay = Model::get_infinity();
	meanDelayVariance = Model::get_infinity();
    }
    _dom._call->setResultWaitingTime(meanDelay)
	.setResultVarianceWaitingTime(meanDelayVariance);
    if ( !reply() ) {
	_dom._call->setResultDropProbability( meanLossProbability );
    }

    if ( number_blocks > 1 ) {
	double varDelay, varDelayVariance;
	const double meanLossVariance = r_loss_prob.variance();

	if ( reply() || r_loss_prob.mean() < 0.005 ) {
	    varDelay  = r_delay.variance();
	    varDelayVariance  = r_delay_sqr.variance();
	} else {
	    varDelay = Model::get_infinity();
	    varDelayVariance = Model::get_infinity();
	}

	_dom._call->setResultWaitingTimeVariance(varDelay)
	    .setResultVarianceWaitingTimeVariance(varDelayVariance);
	if ( !reply() ) {
	    _dom._call->setResultDropProbabilityVariance( meanLossVariance );
	}
    }
}

/*
 * Store target information.  Mallocate space as needed.
 */

void
Targets::store_target_info (  Entry * to_entry, LQIO::DOM::Call* a_call  )
{
    const size_t offset = target.size();
    if ( a_call && alloc_target_info( to_entry ) ) {
	target[offset].initialize( to_entry, a_call );
    }
}


/*
 * Store target information.  Mallocate space as needed.
 */

void
Targets::store_target_info ( Entry * to_entry, double a_const )
{
    const size_t offset = target.size();
    if ( a_const > 0.0 &&  alloc_target_info( to_entry ) ) {
	target[offset].initialize( to_entry, a_const );
    }
}



bool
Targets::alloc_target_info( Entry * to_entry ) 
{
    /* Check for duplicate call. */
	
    for ( unsigned t = 0; t < size(); ++t ) {
	if ( target[t].entry == to_entry ) {
	    input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
	    return false;
	}
    }
    const size_t size = target.size();
    target.resize(size+1);
    return true;
}



/*
 * Compute the PDF for arg.  If normalize is true, the 'calls' field of
 * tinfo_ptr->target is not normalized, so perform a normalization step.
 * In no calls are made at all, tprob is set to zero and sum is set to 1.0.
 * Otherwise, validate sum for proper probability.  Return the sum.
 */

double
Targets::configure( const LQIO::DOM::DocumentObject * dom, bool normalize )
{
    double sum	= 0.0;
    const char * srcName = (dom != nullptr) ? dom->getName().c_str() : "-";
    _type = (dynamic_cast<const LQIO::DOM::Phase *>(dom) != nullptr) ? dynamic_cast<const LQIO::DOM::Phase *>(dom)->getPhaseTypeFlag() : LQIO::DOM::Phase::Type::STOCHASTIC;
    
    for ( std::vector<tar_t>::iterator tp = target.begin(); tp != target.end(); ++tp ) {
	const char * dstName = tp->entry->name();
	try {
	    tp->configure();
	    if ( _type == LQIO::DOM::Phase::Type::DETERMINISTIC && tp->calls() != trunc( tp->calls() ) ) throw std::domain_error( "invalid integer" );
	}
	catch ( const std::domain_error& e ) {
	    if ( dynamic_cast<const LQIO::DOM::Entry *>(dom) ) {
		const LQIO::DOM::Entry * src = dynamic_cast<const LQIO::DOM::Entry *>(dom);
		LQIO::solution_error( LQIO::ERR_INVALID_FWDING_PARAMETER, src->getName().c_str(), dstName, e.what() );
	    } else if ( dynamic_cast<const LQIO::DOM::Activity *>(dom) ) {
		const LQIO::DOM::Activity * src = dynamic_cast<const LQIO::DOM::Activity *>(dom);
		LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "task", src->getTask()->getName().c_str(),
				      "activity", srcName, dstName, e.what() );
	    } else if ( dynamic_cast<const LQIO::DOM::Phase *>(dom) ) {
		const LQIO::DOM::Phase * src = dynamic_cast<const LQIO::DOM::Phase *>(dom);
		LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "entry", src->getSourceEntry()->getName().c_str(),
				      "phase", srcName, dstName, e.what() );
	    } else {
		abort();
	    }
	    throw_bad_parameter();
	}
	sum += tp->calls();
	tp->_tprob = sum;

    }

    if ( _type != LQIO::DOM::Phase::Type::DETERMINISTIC ) {
	if ( normalize ) {
	    sum += 1.0;
	    for ( std::vector<tar_t>::iterator tp = target.begin(); tp != target.end(); ++tp ) {
		tp->_tprob /= sum;
	    }
	} else if ( sum > 1. ) {
	    LQIO::solution_error( LQIO::ERR_INVALID_FORWARDING_PROBABILITY, srcName, sum );
	}
    }

    return sum;
}


void
Targets::initialize( const char * srcName )
{
    for ( std::vector<tar_t>::iterator tp = target.begin(); tp != target.end(); ++tp ) {
	const char * dstName = tp->entry->name();
    
	tp->r_delay.init( SAMPLE,     "Wait %-11.11s %-11.11s          ", srcName, dstName );
	tp->r_delay_sqr.init( SAMPLE, "Wait %-11.11s %-11.11s          ", srcName, dstName );
	tp->r_loss_prob.init( SAMPLE, "Loss %-11.11s %-11.11s          ", srcName, dstName );
    }
}


/*
 * Compute the index of the task to send to.  `i' is the index into
 * the target array and is used to route the message.  `j' is an index
 * used for deterministic phase scheduling.  `i' and `j' returned as
 * they are needed for deterministic scheduling history.
 */

tar_t *
Targets::entry_to_send_to ( unsigned int&i, unsigned int& j ) const
{
    if ( size() != 0 ) {
	double	p;		/* branch probability		*/

	switch ( _type ) {

	case LQIO::DOM::Phase::Type::STOCHASTIC:
	    p = ps_random;
	    for ( i = 0; i < size() && p >= target[i]._tprob; i = i + 1 );
	    break;

	case LQIO::DOM::Phase::Type::DETERMINISTIC:
	    j = j + 1;
	    while ( i < size() && j > target[i].calls() ) {
		j = 1;
		i = i + 1;
	    }
	    break;

	default:
	    abort();
	}
	if ( i < size() ) {
	    return const_cast<tar_t *>(&target[i]);
	}
    }

    return 0;
}



Targets&
Targets::accumulate_data()
{
    std::vector<tar_t>::iterator tp;
    for ( tp = target.begin(); tp != target.end(); ++tp ) {
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
    std::vector<tar_t>::iterator tp;
    for ( tp = target.begin(); tp != target.end(); ++tp ) {
 	tp->r_delay.reset();
 	tp->r_delay_sqr.reset();
 	if ( !tp->reply() ) {
 	    tp->r_loss_prob.reset();
	}
    }
    return *this;
}


const Targets&
Targets::print_raw_stat( FILE * output ) const
{
    std::vector<tar_t>::const_iterator tp;
    for ( tp = target.begin(); tp != target.end(); ++tp ) {
	Entry * ep = tp->entry;
	tp->r_delay.print_raw( output,      "Calling %-11.11s- delay", ep->name() );
	tp->r_delay_sqr.print_raw( output,  "Calling %-11.11s- delay sqr", ep->name() );
	if ( !tp->reply() ) {
	    tp->r_loss_prob.print_raw( output, "Calling %-11.11s- loss prob", ep->name() );
	}
    }
    return *this;
}


Targets&
Targets::insertDOMResults()
{
    std::vector<tar_t>::iterator tp;
    for ( tp = target.begin(); tp != target.end(); ++tp ) {
	tp->insertDOMResults();
    }
    return *this;
}
