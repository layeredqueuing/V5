/*  -*- c++ -*-
 * $Id$
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
#include "call.h"
#include "cltn.h"
#include "stack.h"
#include "entry.h"
#include "entity.h"
#include "task.h"
#include "submodel.h"
#include "server.h"
#include "phase.h"
#include "activity.h"
#include "errmsg.h"
#include "lqns.h"

/*----------------------------------------------------------------------*/
/*                            Generic  Calls                            */
/*----------------------------------------------------------------------*/

/*
 * Initialize and zero fields.   Reverse links are set here.  Forward
 * links are done by subclass.  Processor calls are linked specially.
 */

Call::Call( const Phase * fromPhase, const Entry * toEntry )
    : source(fromPhase), 
      myWait(0.0), 
      destination(toEntry), 
      myCallDOM(NULL)
{
    const_cast<Entry *>(destination)->addDstCall( this );	/* Set reverse link	*/
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


void
Call::check() const
{
    double value = min( rendezvous(), sendNoReply() );
    if ( value < 0. ) {
	if ( isActivityCall() ) {
	    LQIO::solution_error( LQIO::ERR_NEGATIVE_CALLS_FOR, "Task", srcTask()->name(), "activity",  srcName(), value, dstName() );
	} else {
	    LQIO::solution_error( LQIO::ERR_NEGATIVE_CALLS_FOR, "Entry", srcEntry()->name(), "phase",  srcName(), value, dstName()  );
	}
    } else if ( srcPhase()->phaseTypeFlag() == PHASE_DETERMINISTIC ) {
	value = 0.0;
	if ( ::fmod( rendezvous(), 1.0 ) > 1e-6 ) {
	    value = rendezvous();
	} else if ( ::fmod( sendNoReply(), 1.0 ) > 1e-6 ) {
	    value = sendNoReply();
	}
	if ( value != 0.0 ) {
	    if ( isActivityCall() ) {
		LQIO::solution_error( LQIO::ERR_NON_INTEGRAL_CALLS_FOR, "Task", srcTask()->name(), "activity", srcName(), value, dstName() );
	    } else {
		LQIO::solution_error( LQIO::ERR_NON_INTEGRAL_CALLS_FOR, "Entry", srcEntry()->name(), "phase", srcName(), value, dstName() );
	    }
	}
    }
}


double 
Call::rendezvous() const
{
    if ( myCallDOM != NULL && (myCallDOM->getCallType() == LQIO::DOM::Call::RENDEZVOUS || myCallDOM->getCallType() == LQIO::DOM::Call::QUASI_RENDEZVOUS) ) {
	return myCallDOM->getCallMeanValue();
    } else {
	return 0.0;
    }
}

double 
Call::sendNoReply() const
{
    if ( myCallDOM != NULL && myCallDOM->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY ) {
	return myCallDOM->getCallMeanValue();
    } else {
	return 0.0;
    }
}

double 
Call::forward() const
{
    if ( myCallDOM != NULL && myCallDOM->getCallType() == LQIO::DOM::Call::FORWARD ) {
	return myCallDOM->getCallMeanValue();
    } else {
	return 0.0;
    }
}

unsigned
Call::fanIn() const
{
    return 0;
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

const char *
Call::srcName() const
{
    return srcEntry()->name();
}



/*
 * Return the name of the destination entry.
 */

const char *
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
 * Return the total wait along this arc.  Take into
 * account replication.
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



/*
 * Return the source task of this call.  Sources are always tasks.
 */

const Task *
Call::srcTask() const
{
    return dynamic_cast<const Task *>(srcEntry()->owner());
}



double
Call::elapsedTime() const 
{ 
    if (flags.trace_quorum) {
	cout <<"\nCall::elapsedTime(): call " << this->srcName() << " to " << dstEntry()->name() << endl; 
    }

    if ( hasRendezvous() ) {
	return dstEntry()->elapsedTime(1);
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
	return max( myWait - elapsedTime(), 0.0 );
    } else if ( hasSendNoReply() ) {
	return myWait;
    } else {
	return 0.0;
    }
}


void 
Call::insertDOMResults() const
{
    getCallDOM()->setResultWaitingTime(queueingTime());
}


/*
 * Return the adjustment factor for this call.  //REPL
 */

double
Call::nrFactor( const Submodel& aSubmodel, const unsigned k ) const
{
    const Entity * dst_task = dstTask();
    return aSubmodel.nrFactor( dst_task->serverStation(), index(), k ) * fanOut() * rendezvous();	// 5.20
}



/*
 * Return variance of this arc.
 */

double
Call::variance() const
{
    if ( hasRendezvous() ) {
	return dstEntry()->variance(1) + square(queueingTime());
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



/*
 * Follow the call to its destination.
 */

unsigned
Call::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    unsigned max_depth = entryStack.size();

    /* Mark current */

    if ( ( entryStack.size() == 1 || callingPhase == 1 ) && ( rendezvous() > 0.0 ) ) {

	InterlockInfo next = globalCalls * rendezvous();
	if ( callingPhase != 1 ) {
	    next.ph1 = 0.0;
	}

	max_depth = max( const_cast<Entry *>(dstEntry())->initInterlock( entryStack, next ), max_depth );
    }
    return max_depth;
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
   
	aStation->addVisits( e, k, p, rendezvous() * rate );
    }
}


//tomari: set the chain number associated with this call.
void
Call::setChain( const unsigned k, const unsigned p, const double rate )
{
    const Entity * aServer = dstTask();
    if ( aServer->hasServerChain( k )  ){
	
	chainNumber = k; 

	if ( flags.trace_replication ) {
	    cout <<"\nCall::setChain, k=" << k<< "  " ;
	    cout <<",call from "<< srcName() << " To " << dstName()<<endl;
	}
    }
}

//tomari
unsigned Call::getChain()
{
    return chainNumber;
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
    myWait = 0.0;
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
	myWait = aStation->W[e][0][p];
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
	myWait = aStation->W[e][k][p];
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

void
TaskCall::initWait()
{
    myWait = elapsedTime();			/* Initialize arc wait. 	*/
}




/*----------------------------------------------------------------------*/
/*                           Forwarded Calls                            */
/*----------------------------------------------------------------------*/

/*
 * call added to transform forwarding to standard model.
 */

ForwardedCall::ForwardedCall( const Phase * fromPhase, const Entry * toEntry, const Call * fwdCall )
    : TaskCall( fromPhase, toEntry ), myFwdCall(fwdCall)
{
}

const char * 
ForwardedCall::srcName() const
{
    return myFwdCall->srcName();
}


void 
ForwardedCall::insertDOMResults() const
{
    LQIO::DOM::Call* fwdDOM = myFwdCall->getCallDOM();		/* Proxy */
    fwdDOM->setResultWaitingTime(queueingTime());
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

void
ProcessorCall::initWait()
{
    myWait = dstEntry()->serviceTime(1);		/* Initialize arc wait. 	*/
}


void ProcessorCall::setWait(double newWait)
{
    myWait = newWait;
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

const char *
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
    throw should_not_implement( "ActProcCall::srcEntry", __FILE__, __LINE__ );
    return 0;
}


/*
 * Return the name of the source activity.
 */

const char *
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

/* 
 * We are looking for matching tasks for calls.
 */

unsigned 
CallStack::find( const Call * dstCall, const bool direct_path ) const
{
    const Task * dstTask = dynamic_cast<const Task *>(dstCall->dstTask());
    const Entry * dstEntry = dstCall->dstEntry();
    const unsigned sz = Stack<const Call *>::size();
    bool broken = false; 
    for ( unsigned j = sz; j > 0; --j ) {
	const Call * aCall = (*this)[j];
	if ( !aCall ) continue;
	if ( aCall->hasSendNoReply() ) broken = true;		/* Cycle broken - async call */
	if ( aCall->dstTask() == dstTask ) {
	    if ( aCall->hasRendezvous() && dstCall->hasRendezvous() && !broken ) {
		throw call_cycle( dstCall, *this );		/* Dead lock */
	    } if ( aCall->dstEntry() == dstEntry && direct_path ) {
		throw call_cycle( dstCall, *this );		/* Livelock */
	    } else {
		return j;
	    }
	}
    }
    return 0;
}


/*
 * We may skip back over forwarded calls when computing the size.
 */

unsigned 
CallStack::size() const
{
    const unsigned sz = Stack<const Call *>::size();
    unsigned k = 0;
    for ( unsigned j = 1; j <= sz; ++j ) {
	const Call * aCall = (*this)[j];
	if ( !aCall || aCall->hasRendezvous() || aCall->hasSendNoReply() ) {
	    k += 1;
	}
    }
    return k;
}


/* 
 * Return size of stack, regarless of model printing type.
 */

unsigned 
CallStack::size2() const
{
    return Stack<const Call *>::size();
}

/* ------------------------ Exception Handling ------------------------ */

call_cycle::call_cycle( const Call * aCall, const CallStack& callStack )
    : path_error( callStack.size() )
{
    myMsg = aCall->dstName();
    for ( unsigned i = callStack.size(); i > 0; --i ) {
	if ( !callStack[i] ) continue;
	myMsg += ", ";
	myMsg += callStack[i]->dstName();
    }
}
