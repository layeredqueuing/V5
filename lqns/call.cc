/*  -*- c++ -*-
 * $Id: call.cc 14642 2021-05-14 00:45:32Z greg $
 *
 * Everything you wanted to know about a call to an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <mva/server.h>
#include <mva/fpgoop.h>
#include "call.h"
#include "entry.h"
#include "task.h"
#include "submodel.h"
#include "phase.h"
#include "activity.h"
#include "errmsg.h"
#include "lqns.h"

/*----------------------------------------------------------------------*/
/*      Input processing.  Called from model.cc::prepareModel()         */
/*----------------------------------------------------------------------*/

void
Call::Create::operator()( const LQIO::DOM::Call * call )
{
    _src->add_call( _p, call );
}


std::set<Task *>& Call::add_client( std::set<Task *>& clients, const Call * call )
{
    if ( !call->hasForwarding() && call->srcTask()->isUsed() ) {
	clients.insert(const_cast<Task *>(call->srcTask()));
    }
    return clients;
}

std::set<Entity *>& Call::add_server( std::set<Entity *>& servers, const Call * call )
{
    servers.insert(const_cast<Entity *>(call->dstTask()));
    return servers;
}

/*----------------------------------------------------------------------*/
/*                            Generic  Calls                            */
/*----------------------------------------------------------------------*/

/*
 * Initialize and zero fields.   Reverse links are set here.  Forward
 * links are done by subclass.  Processor calls are linked specially.
 */

Call::Call( const Phase * fromPhase, const Entry * toEntry )
    : source(fromPhase),
      _wait(0.0),
      destination(toEntry), 
      _dom(nullptr),
      _chainNumber(0)
{
    if ( toEntry != nullptr ) {
	const_cast<Entry *>(destination)->addDstCall( this );	/* Set reverse link	*/
    }
}


/*
 * Clean up the mess.
 */

Call::~Call()
{
    source = 0;			/* Calling entry.		*/
    destination = 0;		/* to whom I am referring to	*/
}


int
Call::operator==( const Call& item ) const
{
    return (dstEntry() == item.dstEntry());
}


int
Call::operator!=( const Call& item ) const
{
    return (dstEntry() != item.dstEntry());
}


bool
Call::check() const
{
    const int srcReplicas = srcTask()->replicas();
    const int dstReplicas = dstTask()->replicas();
    if ( srcReplicas > 1 || dstReplicas > 1 ) {
	const int fanOut = srcTask()->fanOut( dstTask() );
	const int fanIn  = dstTask()->fanIn( srcTask() );
	if ( fanIn == 0 || fanOut == 0 || srcReplicas * fanOut != dstReplicas * fanIn ) {
	    const std::string& srcName = srcTask()->name();
	    const std::string& dstName = dstTask()->name();
	    LQIO::solution_error( ERR_REPLICATION, 
				  fanOut, srcName.c_str(), srcReplicas,
				  fanIn,  dstName.c_str(), dstReplicas );
	    return false;
	}
    }
    return true;
}


double
Call::rendezvous() const
{
    if ( hasRendezvous() ) {
	try {
	    const double value = getDOM()->getCallMeanValue();
	    if ( srcPhase()->phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC && value != std::floor( value ) ) throw std::domain_error( "invalid integer" );
	    return value;
	}
	catch ( const std::domain_error &e ) {
	    if ( isActivityCall() ) {
		LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "task", srcTask()->name().c_str(), srcPhase()->getDOM()->getTypeName(),
				      srcPhase()->getDOM()->getName().c_str(), dstName().c_str(), e.what() );
	    } else {
		LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "entry", srcEntry()->name().c_str(), srcPhase()->getDOM()->getTypeName(),
				      srcPhase()->getDOM()->getName().c_str(), dstName().c_str(), e.what() );
	    }
	    throw_bad_parameter();
	}
    }
    return 0.0;
}

double
Call::sendNoReply() const
{
    if ( hasSendNoReply() ) {
	try {
	    const double value = getDOM()->getCallMeanValue();
	    if ( srcPhase()->phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC && value != std::floor( value ) ) throw std::domain_error( "invalid integer" );
	    return value;
	}
	catch ( const std::domain_error &e ) {
	    if ( isActivityCall() ) {
		LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "task", srcTask()->name().c_str(), srcPhase()->getDOM()->getTypeName(), 
				      srcPhase()->getDOM()->getName().c_str(), dstName().c_str(), e.what() );
	    } else {
		LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "entry", srcEntry()->name().c_str(), srcPhase()->getDOM()->getTypeName(), 
				      srcPhase()->getDOM()->getName().c_str(), dstName().c_str(), e.what() );
	    }
	    throw_bad_parameter();
	}
    }
    return 0.0;
}

double
Call::forward() const
{
    if ( getDOM() != NULL && getDOM()->getCallType() == LQIO::DOM::Call::Type::FORWARD ) {
	try {
	    const double value = getDOM()->getCallMeanValue();
	    if ( value > 1.0 ) {
		std::stringstream ss;
		ss << value << " > " << 1;
		throw std::domain_error( ss.str() );
	    }
	    return value;
	}
	catch ( const std::domain_error &e ) {
	    LQIO::solution_error( LQIO::ERR_INVALID_FWDING_PARAMETER, srcEntry()->name().c_str(), dstName().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }
    return 0.0;
}


unsigned
Call::fanOut() const
{
    return srcTask()->fanOut( dstTask() );
}



/*
 * Return entry for this call.
 */

const Entry *
Call::srcEntry() const
{
    return source->entry();
}



/*
 * Return the name of the source entry.
 */

const std::string&
Call::srcName() const
{
    return srcEntry()->name();
}



/*
 * Return the name of the destination entry.
 */

const std::string&
Call::dstName() const
{
    return dstEntry()->name();
}



/*
 * Return the submodel number.
 */

unsigned
Call::submodel() const
{
    return dstTask()->submodel();
}


bool
Call::hasOvertaking() const
{
    return hasRendezvous() && dstEntry()->maxPhase() > 1;
}



/*
 * Return the total wait along this arc.  Take into account
 * replication.  This applies for both Pan and Bug 299 replication.
 */

double
Call::rendezvousDelay() const
{
    if ( hasRendezvous() ) {
	return rendezvous() * wait() * fanOut();
    } else {
	return 0.0;
    }
}


#if PAN_REPLICATION
/*
 * Compute and save old rendezvous delay.		// REPL
 */

double
Call::rendezvousDelay( const unsigned k )
{
    if ( dstTask()->hasServerChain(k) ) {
	return rendezvous() * wait() * (fanOut() - 1);
    } else {
	return Call::rendezvousDelay();	// rendezvousDelay is already multiplied by fanOut.
    }
}
#endif



/*
 * Return the source task of this call.  Sources are always tasks.
 */

const Task *
Call::srcTask() const
{
    return dynamic_cast<const Task *>(source->owner());
}



double
Call::elapsedTime() const
{
    if (flags.trace_quorum) {
	std::cout <<"\nCall::elapsedTime(): call " << this->srcName() << " to " << dstEntry()->name() << std::endl;
    }

    if ( hasRendezvous() ) {
	return dstEntry()->elapsedTimeForPhase(1);
    } else {
	return 0.0;
    }
}



/*
 * Return time spent in the queue for call to this entry.
 */

double
Call::queueingTime() const
{
    if ( hasRendezvous() ) {
	if ( std::isinf( _wait ) ) return _wait;
	const double q = _wait - elapsedTime();
	if ( q <= 0.000001 ) {
	    return 0.0;
	} else if ( q * elapsedTime() > 0. && (q/elapsedTime()) <= 0.0001 ) {
	    return 0.0;
	} else {
	    return q;
	}
    } else if ( hasSendNoReply() ) {
	return _wait;
    } else {
	return 0.0;
    }
}


const Call&
Call::insertDOMResults() const
{
    const_cast<LQIO::DOM::Call *>(getDOM())->setResultWaitingTime(queueingTime());
    return *this;
}


#if PAN_REPLICATION
/*
 * Return the adjustment factor for this call.  //REPL
 */

double
Call::nrFactor( const Submodel& aSubmodel, const unsigned k ) const
{
    const Entity * dst_task = dstTask();
    return aSubmodel.nrFactor( dst_task->serverStation(), index(), k ) * fanOut() * rendezvous();	// 5.20
}
#endif



/*
 * Return variance of this arc.
 */

double
Call::variance() const
{
    if ( hasRendezvous() ) {
	return dstEntry()->varianceForPhase(1) + square(queueingTime());
    } else {
	return 0.0;
    }
}



/*
 * Return the coefficient of variation for this particular call.
 */

double
Call::CV_sqr() const
{
#ifdef NOTDEF
    return dstEntry()->variance(1) / square(elapsedTime());
#endif
    return variance() / square(wait());
}



/*
 * Follow the call to its destination.
 */

const Call&
Call::followInterlock( Interlock::CollectTable& path ) const
{
    /* Mark current */

    if ( rendezvous() > 0.0 && !path.prune() ) {
	Interlock::CollectTable branch( path, path.calls() * rendezvous() );
	const_cast<Entry *>(dstEntry())->initInterlock( branch );
    }
    return *this;
}



/*
 * Set the visit ratio at the destinations station.
 */

void
Call::setVisits( const unsigned k, const unsigned p, const double rate )
{
    const Entity * aServer = dstTask();
    if ( aServer->hasServerChain( k ) && hasRendezvous() && !srcTask()->hasInfinitePopulation() ) {
	Server * aStation = aServer->serverStation();
	const unsigned e = dstEntry()->index();
#if BUG_299
	aStation->addVisits( e, k, p, rendezvous() / fanOut() * rate );
#else
	aStation->addVisits( e, k, p, rendezvous() * rate );
#endif
    }
}


//tomari: set the chain number associated with this call.
void
Call::setChain( const unsigned k, const unsigned p, const double rate )
{
    const Entity * aServer = dstTask();
    if ( aServer->hasServerChain( k )  ){

	_chainNumber = k;

	if ( flags.trace_replication ) {
	    std::cout <<"\nCall::setChain, k=" << k<< "  " ;
	    std::cout <<",call from "<< srcName() << " To " << dstName()<< std::endl;
	}
    }
}




/*
 * Set the open arrival rate to the destination's station.
 */

void
Call::setLambda( const unsigned, const unsigned p, const double rate )
{
    Server * aStation = dstTask()->serverStation();
    const unsigned e = dstEntry()->index();
    if ( hasSendNoReply() ) {
	aStation->addVisits( e, 0, p, srcPhase()->throughput() * sendNoReply() );
    } else if ( hasRendezvous() && srcTask()->isInOpenModel() && srcTask()->isInfinite() ) {
	aStation->addVisits( e, 0, p, srcPhase()->throughput() * rendezvous() );
    }
}


/*
 * Clear waiting time.
 */

void
Call::clearWait( const unsigned k, const unsigned p, const double )
{
    _wait = 0.0;
}



/*
 * Get the waiting time for this call from the mva submodel.  A call
 * can potentially orginate from multiple chains, so add them all up.
 * (Call clearWait first.)
 */

void
Call::saveOpen( const unsigned, const unsigned p, const double )
{
    const unsigned e = dstEntry()->index();
    const Server * aStation = dstTask()->serverStation();

    if ( aStation->V( e, 0, p ) > 0.0 ) {
	_wait = aStation->W[e][0][p];
    }
}



/*
 * Get the waiting time for this call from the mva submodel.  A call
 * can potentially orginate from multiple chains, so add them all up.
 * (Call clearWait first.).  This may havve to be changed if the
 * result varies by chain.  Priorities perhaps?
 */

void
Call::saveWait( const unsigned k, const unsigned p, const double )
{
    const Entity * aServer = dstTask();
    const unsigned e = dstEntry()->index();
    const Server * aStation = aServer->serverStation();

    if ( aStation->V( e, k, p ) > 0.0 ) {
	_wait = aStation->W[e][k][p];
    }
}

/*----------------------------------------------------------------------*/
/*                              Task Calls                              */
/*----------------------------------------------------------------------*/

/*
 * call to task entry.
 */

TaskCall::TaskCall( const Phase * fromPhase, const Entry * toEntry )
    : Call( fromPhase, toEntry )
{
    const_cast<Phase *>(source)->addSrcCall( this );
}



/*
 * Initialize waiting time.
 */

TaskCall&
TaskCall::initWait()
{
    _wait = elapsedTime();			/* Initialize arc wait. 	*/
    return *this;
}

/*----------------------------------------------------------------------*/
/*                           Forwarded Calls                            */
/*----------------------------------------------------------------------*/

/*
 * call added to transform forwarding to standard model.
 */

ForwardedCall::ForwardedCall( const Phase * fromPhase, const Entry * toEntry, const Call * fwdCall )
    : TaskCall( fromPhase, toEntry ), _fwdCall(fwdCall)
{
}

bool
ForwardedCall::check() const
{
    const Task * srcTask = dynamic_cast<const Task *>(_fwdCall->srcPhase()->owner());
    const int srcReplicas = srcTask->replicas();
    const int dstReplicas = dstTask()->replicas();
    if ( srcReplicas > 1 || dstReplicas > 1 ) {
	const int fanOut = srcTask->fanOut( dstTask() );
	const int fanIn  = dstTask()->fanIn( srcTask );
	if ( fanIn == 0 || fanOut == 0 || srcReplicas * fanOut != dstReplicas * fanIn ) {
	    const std::string& srcName = srcTask->name();
	    const std::string& dstName = dstTask()->name();
	    LQIO::solution_error( ERR_REPLICATION, 
				  fanOut, srcName.c_str(), srcReplicas,
				  fanIn,  dstName.c_str(), dstReplicas );
	    return false;
	}
    }
    return true;
}

const std::string&
ForwardedCall::srcName() const
{
    return _fwdCall->srcName();
}


const ForwardedCall&
ForwardedCall::insertDOMResults() const
{
    LQIO::DOM::Call* fwdDOM = const_cast<LQIO::DOM::Call *>(_fwdCall->getDOM());		/* Proxy */
    fwdDOM->setResultWaitingTime(queueingTime());
    return *this;
}

/*----------------------------------------------------------------------*/
/*                           Processor Calls                            */
/*----------------------------------------------------------------------*/

/*
 * Call to processor entry.
 */

ProcessorCall::ProcessorCall( const Phase * fromPhase, const Entry * toEntry )
    : Call( fromPhase, toEntry )
{
}



/*
 * Set up waiting time to processors.
 */

ProcessorCall&
ProcessorCall::initWait()
{
    _wait = dstEntry()->serviceTimeForPhase(1);		/* Initialize arc wait. 	*/
    return *this;
}


void
ProcessorCall::setWait(double newWait)
{
    _wait = newWait;
}


/*----------------------------------------------------------------------*/
/*                            Activity Calls                            */
/*----------------------------------------------------------------------*/

/*
 * Call added for activities.
 */

ActivityCall::ActivityCall( const Phase * fromActivity, const Entry * toEntry )
    : TaskCall( fromActivity, toEntry )
{
}


/*
 * Return entry for this call.
 */

const Entry *
ActivityCall::srcEntry() const
{
    throw should_not_implement( "ActivityCall::srcEntry", __FILE__, __LINE__ );
    return 0;
}


/*
 * Return the name of the source activity.
 */

const std::string&
ActivityCall::srcName() const
{
    return source->name();
}


/*
 * Return entry for this call.
 */

const Task *
ActivityCall::srcTask() const
{
    return dynamic_cast<const Task *>(source->owner());
}

/*----------------------------------------------------------------------*/
/*                       Activity Forwarded Calls                       */
/*----------------------------------------------------------------------*/

/*
 * call added to transform forwarding to standard model.
 */

ActivityForwardedCall::ActivityForwardedCall( const Phase * fromPhase, const Entry * toEntry )
    : ActivityCall( fromPhase, toEntry )
{
}

/*----------------------------------------------------------------------*/
/*                       Activity Processor Calls                       */
/*----------------------------------------------------------------------*/

/*
 * Call to processor entry.
 */

ActProcCall::ActProcCall( const Phase * fromPhase, const Entry * toEntry )
    : ProcessorCall( fromPhase, toEntry )
{
}



/*
 * Return entry for this call.
 */

const Entry *
ActProcCall::srcEntry() const
{
    std::cout<<"ActProcCall:"<<srcName()<<std::endl;
    throw should_not_implement( "ActProcCall::srcEntry", __FILE__, __LINE__ );
    return nullptr;
}


/*
 * Return the name of the source activity.
 */

const std::string&
ActProcCall::srcName() const
{
    return source->name();
}


/*
 * Return entry for this call.
 */

const Task *
ActProcCall::srcTask() const
{
    return dynamic_cast<const Task *>(source->owner());
}

/*----------------------------------------------------------------------*/
/*                          CallStack Functions                         */
/*----------------------------------------------------------------------*/

/*
 * We are looking for matching tasks for calls.
 */

bool 
Call::Find::operator()( const Call * call ) const
{
    if ( call == nullptr || call->getDOM() == nullptr ) return false;

    if ( call->hasSendNoReply() ) _broken = true;		/* Cycle broken - async call */
    if ( call->dstTask() == _call->dstTask() ) {
	if ( call->hasRendezvous() && _call->hasRendezvous() && !_broken ) {
	    throw call_cycle();		/* Dead lock */
	} else if ( call->dstEntry() == _call->dstEntry() && _direct_path ) {
	    throw call_cycle();		/* Livelock */
	} else {
	    return true;
	}
    }
    return false;
}

/*
 * We may skip back over forwarded calls when computing the size.
 */

unsigned
Call::stack::depth() const	
{
    return std::count_if( begin(), end(), Predicate<Call>( &Call::hasNoForwarding ) );
}
