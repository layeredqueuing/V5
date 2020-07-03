/*  -*- c++ -*-
 * $Id: phase.cc 13570 2020-05-27 15:10:55Z greg $
 *
 * Everything you wanted to know about an phase, but were afraid to ask.
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
#include <cstdlib>
#include <lqio/input.h>
#include <lqio/error.h>
#include <lqio/dom_histogram.h>
#include "fpgoop.h"
#include "model.h"
#include "phase.h"
#include "cltn.h"
#include "stack.h"
#include "vector.h"
#include "entry.h"
#include "entity.h"
#include "task.h"
#include "processor.h"
#include "call.h"
#include "prob.h"
#include "gamma.h"
#include "lqns.h"
#include "pragma.h"
#include "variance.h"
#include "submodel.h"
#include "errmsg.h"


/* ---------------------- Overloaded Operators ------------------------ */

ostream& operator<<( ostream& output, const Phase& self )
{
    self.print( output );
    return output;
}


NullPhase::NullPhase( const NullPhase& ) 
{
    abort();
}


NullPhase& NullPhase::operator=( const NullPhase& ) 
{
    abort();
    return *this;
}


/*
 * Allocate array space for submodels
 */

void
NullPhase::configure( const unsigned n, const unsigned )
{
    myWait.grow( n );
}


NullPhase& 
NullPhase::setDOM(LQIO::DOM::Phase* phaseInfo)
{
    myDOMPhase = phaseInfo;
    return *this;
}

NullPhase& 
NullPhase::setServiceTime( const double t ) 
{
    if (myDOMPhase != NULL) {
	abort();
    } else {
	myServiceTime = t; 
    }
	
    return *this; 
}


NullPhase& 
NullPhase::addServiceTime( const double t ) 
{ 
    if (myDOMPhase != NULL) {
	abort();
    } else {
	myServiceTime += t; 
    }
	
    return *this; 
}

double
NullPhase::serviceTime() const
{
    if ( myDOMPhase == NULL ) return myServiceTime;
    try {
	return myDOMPhase->getServiceTimeValue();
    }
    catch ( const std::domain_error& e ) {
	solution_error( LQIO::ERR_INVALID_PARAMETER, "service time", myDOMPhase->getTypeName(), name(), e.what() );
	throw_bad_parameter();
    }
    return 0.0;
}


double
NullPhase::thinkTime() const
{
    try {
	return myDOMPhase->getThinkTimeValue();
    }
    catch ( const std::domain_error& e ) {
	solution_error( LQIO::ERR_INVALID_PARAMETER, "think time", myDOMPhase->getTypeName(), name(), e.what() );
	throw_bad_parameter();
    }
    return 0.;
}

double
NullPhase::CV_sqr() const
{
    try {
	return myDOMPhase->getCoeffOfVariationSquaredValue();
    }
    catch ( const std::domain_error& e ) {
	solution_error( LQIO::ERR_INVALID_PARAMETER, "CVsqr", myDOMPhase->getTypeName(), name(), e.what() );
	throw_bad_parameter();
    }
    return 1.;
}


double
NullPhase::computeCV_sqr() const
{
    double w = waitExcept(0);
    if ( w ) {
	return variance() / square(w);
    } else {
	return 0.0;
    }
}


/*
 * Return per-call waiting to processor (includes service time.)
 * EXCEPT for waiting in submodel g.
 */

double
NullPhase::waitExcept( const unsigned submodel ) const
{
    const unsigned n = myWait.size();
 
    double sum = 0.0;
    for ( unsigned i = 1; i <= n; ++i ) {
	if ( i != submodel ) {
	    sum += myWait[i];
	    if (myWait[i] < 0 && flags.trace_quorum) { 
		cout << "\nNullPhase::waitExcept(submodel=" << submodel << 
		    "): submodel number "<<i<<" has less than zero wait. sum of waits=" << sum << endl;
	    }
	}
    }

    if (sum < 0) {
	sum =0; //two-phase quorum semantics. Because of distribution fitting, the difference
	// in the mean before and after fitting might be negative.
    }
    return sum;
}


/*
 * Compute a histogram based on a gamma distribution with a mean m and variance v.
 */

void
NullPhase::insertDOMHistogram( LQIO::DOM::Histogram * histogram, const double m, const double v )
{
    const unsigned int n_bins = histogram->getBins();		/* +2 for under and over-flow */
    const double bin_size = histogram->getBinSize();

    if ( v > 0 ) {
	/* Compute gamma stuff. */
	const double b = v / m;
	const double k = (m*m) / v;
	Gamma_Distribution dist( k, b );

	/* Convert the Cumulative Distribution into a discrete probability distribution */
	double x = histogram->getMin();
	double prev = 0.0;
	for ( unsigned int i = 0; i <= n_bins; ++i, x += bin_size ) {	
	    const double temp = dist.getCDF(x);
	    histogram->setBinMeanVariance( i, temp - prev );
	    prev = temp;
	}
	histogram->setBinMeanVariance( n_bins + 1, 1.0 - prev );	/* overflow */
    } else {
	/* deterministic */
	histogram->setBinMeanVariance( histogram->getBinIndex( m ), 1.0 );
    }
}

/*----------------------------------------------------------------------*/
/*                                 Phase                                */
/*----------------------------------------------------------------------*/
		
/*
 * Constructor.
 */

Phase::Phase( const char * aName )
    : myProcessorCall(0),
      myThinkCall(0),
      myProcessorEntry(0),
      myThinkEntry(0),
      iWasChanged(false)	        /* True if reinit required      */
{
    if ( aName ) {
	myName = aName;
    }      
}


/*
 * Free up resources.
 */

Phase::~Phase()
{
    if ( myProcessorCall ) {
	LQIO::DOM::Call* callDOM = myProcessorCall->getCallDOM();
	if ( callDOM ) delete callDOM;
	delete myProcessorCall;
	myProcessorCall = 0;
    }
    if ( myProcessorEntry ) {
	delete myProcessorEntry;
	myProcessorEntry = 0;
    }
    if ( myThinkCall ) {
	myCalls -= myThinkCall;	/* Remove so deleteContents doesn't try */
	LQIO::DOM::Call* callDOM = myThinkCall->getCallDOM();
	if ( callDOM ) delete callDOM;
	delete myThinkCall;
	myThinkCall = 0;
    }
    if ( myThinkEntry ) {
	delete myThinkEntry;
	myThinkEntry = 0;
    }

    /* Release forward links */
	
    myCalls.deleteContents();
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
Phase::findChildren( CallStack& callStack, const bool directPath ) const
{
    unsigned max_depth = callStack.size();

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isForwardedCall() ) continue;

	const Entity * dstTask = aCall->dstTask();
	try {

	    /* 
	     * Chase the call if there is no loop, and if following the path results in pushing
	     * tasks down.  Open class requests can loop up.  Always check the stack because of the
	     * short-circuit test with directPath.
	     */

	    if (( callStack.find( aCall, directPath ) == 0 && callStack.size() >= dstTask->submodel() )
		|| directPath ) {					/* Always (for forwarding)	*/

		callStack.push( aCall );
		if ( aCall->hasForwarding() && directPath ) {
		    addForwardingRendezvous( callStack );
		} 
		max_depth = max( dstTask->findChildren( callStack, directPath ), max_depth );
		callStack.pop();

	    }
	}
	catch ( const call_cycle& error ) {
	    if ( directPath && pragma.getCycles() == DISALLOW_CYCLES ) {
		LQIO::solution_error( LQIO::ERR_CYCLE_IN_CALL_GRAPH, error.what() );
	    }
	}
    }
    if ( processorCall() ) {
	callStack.push( processorCall() );
	const Entity * child = processorCall()->dstTask();
	max_depth = max( child->findChildren( callStack, directPath ), max_depth );
	callStack.pop();
    }
    return max_depth;
}




/* 
 * We have to add a psuedo forwarding arc.  Search back for on the
 * call stack for a rendezvous.  On drawing, we may have to do some
 * fancy labelling.  We may have to have more than one proxy.
 */

Phase const& 
Phase::addForwardingRendezvous( CallStack& callStack ) const
{
    double rate     = 1.0;
    for ( unsigned i = callStack.size2(); i > 0 && callStack[i]; --i ) {
	const Call * aCall = callStack[i];
	if ( aCall->hasRendezvous() ) {
	    rate   *= aCall->rendezvous();
	    const_cast<Phase *>(aCall->srcPhase())->forwardedRendezvous( callStack.top(), rate );
	    break;
	} else if ( aCall->hasSendNoReply() ) {
	    break;
	} else if ( aCall->hasForwarding() ) {
	    rate   *= aCall->forward();
	} else {
	    abort();
	}
    }
    return *this;
}


/*
 * Grow mySurrogateDelay array as neccesary.  Initialize to zero.  Used
 * by Newton Raphson step.
 */
 
void
Phase::initReplication( const unsigned maxSize )
{
    const unsigned size = mySurrogateDelay.size();
    if ( size < maxSize ) {
	mySurrogateDelay.grow( maxSize - size );
    }
}



/*
 * Initialize waiting time from lower level servers.
 */

void
Phase::initWait()
{
    Sequence<Call *> nextCall(callList());
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->initWait();
    }

    if ( myProcessorCall ) {
	myProcessorCall->initWait();
    }
}



/*
 * Initialize variance based on the CV_sqr() and service time.  This is only done
 * for device entries.
 */

void
Phase::initVariance() 
{ 
    myVariance = CV_sqr() * square( serviceTime() ); 
}


/*
 * Clear replication variables.
 */

void
Phase::resetReplication()
{
    mySurrogateDelay = 0;
}


/*
 * Make sure deterministic phases are correct.
 */

void
Phase::check( const unsigned p ) const
{
    if ( !isPresent() ) return;
    char p_str[2];
    p_str[0] = p + '0';
    p_str[1] = '\0';

    /* Service time for the entry? */
    if ( serviceTime() == 0 ) {
	if ( isActivity() ) {
	    LQIO::solution_error( LQIO::WRN_NO_SERVICE_TIME_FOR, "Task", owner()->name(), myDOMPhase->getTypeName(),  name() );
	} else {
	    LQIO::solution_error( LQIO::WRN_NO_SERVICE_TIME_FOR, "Entry", entry()->name(), myDOMPhase->getTypeName(),  p_str );
	}
    }

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->check();
    }

    if ( phaseTypeFlag() == PHASE_STOCHASTIC && CV_sqr() != 1.0 ) {
	if ( isActivity() ) {			/* c, phase_flag are incompatible  */
	    LQIO::solution_error( WRN_COEFFICIENT_OF_VARIATION, "Task", owner()->name(), myDOMPhase->getTypeName(), name() );
	} else {
	    LQIO::solution_error( WRN_COEFFICIENT_OF_VARIATION, "Entry", entry()->name(), myDOMPhase->getTypeName(), p_str );
	}
    }
}



/*
 * Initialize variance.
 */

bool
Phase::hasVariance() const
{
    return CV_sqr() != 1.0 || sumOfRendezvous() > 0;		/* Bug 234 */
}


/*
 * Aggregate whatever aFunc into the entry at the top of stack. 
 * Follow the activitylist and continue.
 */

void
Phase::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList *, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->submodel() == submodel ) {
	    (aCall->*aFunc)( k, p, rate );
	}
    }
    
    aCall = processorCall();
    if ( aCall ) {
	if ( aCall->submodel() == submodel ) {
	    (aCall->*aFunc)( k, p, rate );
	}
    }
}




/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Phase::findCall( const Entry * anEntry, const queryFunc aFunc ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->dstEntry() == anEntry && !aCall->isForwardedCall() && (!aFunc || (aCall->*aFunc)()) ) return aCall;
    }

    return aCall;
}



/*
 * Locate the destination anEntry in the list of destinations.
 * The call must be a proxy for a forward.
 */

Call *
Phase::findFwdCall( const Entry * anEntry ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( ( aCall = nextCall() ) && ( aCall->dstEntry() != anEntry || !aCall->isForwardedCall() ) );

    return aCall;
}



/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

Call *
Phase::findOrAddCall( const Entry * anEntry, const queryFunc aFunc  )
{
    Call * aCall = findCall( anEntry, aFunc );

    if ( !aCall ) {
	aCall = new TaskCall( this, anEntry );
    }

    return aCall;
}


/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

Call *
Phase::findOrAddFwdCall( const Entry * anEntry, const Call * fwdCall )
{
    Call * aCall = findFwdCall( anEntry );

    if ( !aCall ) {
	aCall = new ForwardedCall( this, anEntry, fwdCall );
    }

    return aCall;
}


/*
 * Return the number of calls to aTask from this phase.
 */

double
Phase::rendezvous( const Entity * aTask ) const
{
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    double sum = 0.0;
    while ( aCall = nextCall() ) {
	if ( aCall->dstTask() != aTask ) continue;

	sum += aCall->rendezvous() * aCall->fanOut();
    }
    return sum;
}



/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Phase&
Phase::rendezvous( Entry * toEntry, LQIO::DOM::Call* callDOMInfo )
{
    if ( callDOMInfo != NULL && toEntry->isCalled( RENDEZVOUS_REQUEST ) ) {
	if ( owner() ) {
	    const_cast<Entity *>(owner())->isPureServer( false );
	}
		
	Call * aCall = findOrAddCall( toEntry, &Call::hasRendezvousOrNone  );
	aCall->rendezvous( callDOMInfo );
    }

    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

double
Phase::rendezvous( const Entry * anEntry ) const
{
    const Call * aCall = findCall( anEntry, &Call::hasRendezvous );
    if ( aCall ) {
	return aCall->rendezvous();
    } else {
	return 0.0;
    }
}



/*
 * Return the sum of all calls from the receiver during it's phase `p'.
 */

double
Phase::sumOfRendezvous() const
{
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    double sum = 0.0;
    while ( aCall = nextCall() ) {
	sum += aCall->rendezvous() * aCall->fanOut();
    }
    return sum;
}



/*
 * Set the value of send-no-reply calls to entry `toEntry', `phase'.
 * Retotal.
 */

Phase&
Phase::sendNoReply( Entry * toEntry, LQIO::DOM::Call* callDOMInfo )
{
    if ( callDOMInfo != NULL && toEntry->isCalled( SEND_NO_REPLY_REQUEST ) ) {
	if ( owner() ) {
	    const_cast<Entity *>(owner())->isPureServer( false );
	}

	Call * aCall = findOrAddCall( toEntry, &Call::hasSendNoReplyOrNone );
	aCall->sendNoReply( callDOMInfo );
    }
	
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

double
Phase::sendNoReply( const Entry * anEntry ) const
{
    const Call * aCall = findCall( anEntry, &Call::hasSendNoReply  );
    if ( aCall ) {
	return aCall->sendNoReply();
    } else {
	return 0.0;
    }
}



/*
 * Create a forwarded rendezvous type arc.
 */

Phase&
Phase::forwardedRendezvous( const Call * fwdCall, const double value )
{
    const Entry * toEntry = fwdCall->dstEntry();
    if ( value > 0.0 && const_cast<Entry *>(toEntry)->isCalled( RENDEZVOUS_REQUEST ) ) {
	if ( owner() ) {
	    const_cast<Entity *>(owner())->isPureServer( false );
	}
	Call * aCall = findOrAddFwdCall( toEntry, fwdCall );
	LQIO::DOM::Phase* aDOM = getDOM();
	LQIO::DOM::Call* rendezvousCall = new LQIO::DOM::Call( aDOM->getDocument(), LQIO::DOM::Call::RENDEZVOUS,
							       myDOMPhase, toEntry->getDOM(), 
							       new LQIO::DOM::ConstantExternalVariable(value));
	aCall->rendezvous(rendezvousCall);
    }
    return *this;
}



/*
 * Return the sum of all calls from the receiver.
 * Forwarded calls are not counted because they
 * are pseudo calls from this entry.
 */

double
Phase::numberOfSlices() const
{
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    double sum = 1.0;
    while ( aCall = nextCall() ) {
	if ( aCall->isForwardedCall() ) continue;

	sum += aCall->rendezvous() * aCall->fanOut();
    }
    return sum;
}



/*
 * Store forwarding probability in call list.
 */

Phase&
Phase::forward( Entry * toEntry, LQIO::DOM::Call* callDOMInfo )
{
    if ( callDOMInfo != NULL && toEntry->isCalled( RENDEZVOUS_REQUEST ) ) {
	if ( owner() ) {
	    const_cast<Entity *>(owner())->isPureServer( false );
	}

	Call * aCall = findOrAddCall( toEntry, &Call::hasForwardingOrNone );
	aCall->forward( callDOMInfo );
    }

    return *this;
}



/*
 * Retrieve forwarding probability to entry.
 */

double
Phase::forward( const Entry * toEntry ) const
{
    const Call * aCall = findCall( toEntry, &Call::hasForwarding );

    if ( aCall ) {
	return aCall->forward();
    } else {
	return 0.0;
    }
}




/*
 * Return utilization for phase.  
 */

double
Phase::utilization() const
{
    if ( isfinite( throughput() ) ) {
	return throughput() * elapsedTime();
    } else {
	return 0.0;
    }
}



/*
 * Return number of calls to the processor.
 */

double
Phase::processorCalls() const
{
    return processorCall() ? processorCall()->rendezvous() * processorCall()->fanOut() : 0.0;
}


/*
 * Return time in queue to processor.
 */

double
Phase::queueingTime() const
{
    return processorCall() ? processorCall()->rendezvous() * processorCall()->queueingTime() : 0.0;
}


/*
 * Return per-call waiting to processor (includes service time.)
 */

double
Phase::processorWait() const
{
    return processorCall() ? processorCall()->wait() : 0.0;
}



/*
 * Return variance at processor.  
 */

double
Phase::processorVariance() const
{
    return processorCall() ? processorCall()->variance() : 0.0;
}



/*
 * Return utilization for phase.
 */

double
Phase::processorUtilization() const
{
    if ( processorCall() ) {
	const Processor * aProc = dynamic_cast<const Processor *>(processorCall()->dstTask());
	return throughput() * serviceTime()  / (processorCall()->fanOut() * aProc->rate() );
    } else {
	return 0.0;		/* No processor == no utilization */
    }
}



/*
 * Return waiting time.  Normally, we exclude all of chain k, but with
 * replication, we have to include replicas-1 wait for chain k too.
 */


double
Phase::waitExceptChain( const unsigned submodel, const unsigned k )
{
	
    if ( k <= mySurrogateDelay.size()) {
	return mySurrogateDelay[k] + waitExcept( submodel );
    } else {
	return waitExcept( submodel );
    }
}




double
Phase::nrFactor( const Call * aCall, const Submodel& aSubmodel ) const
{
    unsigned submodel = aSubmodel.number();
    double nr_factor = 0.0;
    unsigned count = 0;
	
    Task * aTask = const_cast<Task *>(dynamic_cast<const Task *>(owner()));
    const ChainVector& aChain = aTask->clientChains( submodel );
    for ( unsigned ix = 1; ix <= aChain.size(); ++ix, ++count ) {
	nr_factor = aCall->nrFactor( aSubmodel, aChain[ix] );
    }

    if ( count ) {
	return nr_factor / count;
    } else {
	return 0;
    }
}



/*
 * Calculate total wait for a particular submodel and save.  Return
 * the difference between this pass and the previous.  
 */

Phase&
Phase::updateWait( const Submodel& aSubmodel, const double relax ) 
{
    const unsigned submodel = aSubmodel.number();
    const double oldWait    = myWait[submodel];
    double newWait   = 0.0;

    if ( oldWait && flags.trace_delta_wait ) {
	cout << "Phase::updateWait(" << submodel << "," << relax << ") for " << name() << endl;
    }
	
    /* Sum up waits to all other tasks in this submodel */

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->submodel() == submodel ) {
	    newWait += aCall->rendezvousDelay();

	    if ( oldWait && flags.trace_delta_wait ) {
		cout << "        to entry " << aCall->dstName() << " wait="
		     << aCall->rendezvousDelay()
		     << endl;
	    }
	}
    }

    /* Tack on processor delay if necessary */

    if ( processorCall() && processorCall()->submodel() == submodel ) {
	newWait += processorCall()->rendezvousDelay();
			
	if ( oldWait && flags.trace_delta_wait ) {
	    cout << "        to processor " << processorCall()->dstName() << " wait="
		 << processorCall()->rendezvousDelay()
		 << endl;
	}
    }

    /* Now update waiting values */

    under_relax( myWait[submodel], newWait, relax );

    if ( oldWait && flags.trace_delta_wait ) {
	cout << "        Sum of wait=" << newWait << ", myWait[" << submodel << "]=" << myWait[submodel] << endl;
    }

    return *this;
}




double
Phase::getReplicationProcWait( unsigned int submodel, const double relax ) 
{
   
    double newWait   = 0.0;

    if ( processorCall() && processorCall()->submodel() == submodel ) {
		

	int k= processorCall()->getChain();
	//procWait += waitExceptChain( submodel, k );	
	if ( processorCall()->dstTask()->hasServerChain(k) ) {
	    // newWait +=  mySurrogateDelay[k];

	    if (flags.trace_quorum) {
		
		cout << "\nPhase::getReplicationProcWait(): Call " << this->name() << ", Submodel=" <<  processorCall()->submodel() 
		     << ", mySurrogateDelay[" <<k<<"]="<<mySurrogateDelay[k] << endl;
		fflush(stdout);
	    }
	}


	newWait += processorCall()->wait();// * processorCall()->fanOut();
			
    }

    /* Now update waiting values */

    // under_relax( myWait[submodel], newWait, relax );

   
    return newWait;

}

double
Phase::getReplicationTaskWait( unsigned int submodel, const double relax ) 
{
    double newWait   = 0.0;

    /* Sum up waits to all other tasks in this submodel */

    Call * aCall;
    Sequence<Call *> nextCall( callList() );
    while ( aCall = nextCall() ) {
	newWait += aCall->wait();
    }

    return newWait;
}


double 
Phase::getReplicationRendezvous( unsigned int submodel, const double relax ) //tomari : quorum
{
    double totalRendezvous   = 0.0;
	 
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->submodel() == submodel ) {
	    totalRendezvous += aCall->rendezvous() * aCall->fanOut();
	}
    }

    return totalRendezvous;
}


double
Phase::getProcWait( unsigned int submodel, const double relax ) //tomari : quorum
{
    double newWait   = 0.0;

    if ( processorCall() && processorCall()->submodel() == submodel ) {
		
	newWait += processorCall()->rendezvousDelay();

	if (flags.trace_quorum) {
	    cout << "\nPhase::getProcWait(): Call " << this->name() << ", Submodel=" <<  processorCall()->submodel() 
		 << ", newWait="<<newWait << endl;
	    fflush(stdout);
	}
			
    }

    return newWait;
}

//tomari quorum: Used in a closed form formula to estimate the thread service time. 
//The closed form formula was originally developed with an assumption 
//that an activity calls only one server. The current code is modified to 
//average the service times if an activity calls more than one server.  
double
Phase::getTaskWait( unsigned int submodel, const double relax ) //tomari : quorum
{
    double newWait   = 0.0;
    double totalRendezvous = 0;

    Sequence<Call *> xCall( callList() );
    const Call * aCall;
    while ( aCall = xCall() ) {
	if ( aCall->hasRendezvous() && aCall->submodel() == submodel ) {
	    totalRendezvous += aCall->rendezvous();

	}
    }

    /* Sum up waits to all other tasks in this submodel */

    Sequence<Call *> nextCall( callList() );
    while ( aCall = nextCall() ) {
	if ( aCall->hasRendezvous() && aCall->submodel() == submodel ) {
	    newWait += aCall->wait() * (aCall->rendezvous() /totalRendezvous);

	}
    }

    return newWait;
}


double 
Phase::getRendezvous( unsigned int submodel, const double relax ) //tomari : quorum
{
    double totalRendezvous   = 0.0;
	 
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->hasRendezvous() && aCall->submodel() == submodel ) {
	    totalRendezvous += aCall->rendezvous();

	}
    }

    return totalRendezvous;
}



/* 
 * Calculate the surrogatedelay of a chain k of a
 *specific thread....tomari
 */

double
Phase::updateWaitReplication( const Submodel& aSubmodel ) 
{
    double delta = 0.0;
   
    // Over all chains k that belong to the owner thread
    // of this phase...
    
    if ( flags.trace_replication ) {
	cout <<"\nPhase::updateWaitReplication()........Current phase is " << name();
	cout <<" ..............." <<endl; 
    }
    const ChainVector& aChain = const_cast<Task *>(dynamic_cast<const Task *>(owner()))->clientChains(aSubmodel.number());
    Sequence<Call *> nextCall( callList() );

    for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	const unsigned k = aChain[ix];

	//cout <<"\n***Start processing master chain k = " << k << " *****" << endl;

	const Task * ownerTask = dynamic_cast<const Task *>(owner());

	bool include = false;
	Call * aCall;
	bool proc_mod = false;
	double nr_factor = 0.0;
	double newWait = 0.0;

	const Thread * thread = ownerTask->getThread( aSubmodel.number(), k );

	while ( aCall = nextCall() ) {
	    if ( aCall->submodel() != aSubmodel.number() ) continue;
           
	    if ( thread != ownerTask->getThread( aSubmodel.number(), aCall->getChain() ) ) {
		continue;
	    }
	    
	    const double temp = aCall->rendezvousDelay( k );
	    if ( aCall->dstTask()->hasServerChain(k) ) {
		include = true;
             
		nr_factor = aCall->nrFactor( aSubmodel, k );
	    }
	    if ( flags.trace_replication ) {
		cout << "\nCallChainNum=" << aCall->getChain() << ",Src=" << aCall->srcName() << ",Dst=" << aCall->dstName() << ",Wait=" << temp << endl;
	    }
	    newWait += temp;
 
	    //Take note if there are zero visits to task.

	    if ( aCall->rendezvousDelay() > 0 ) {
		proc_mod = true;
	    } 
	}
 
	if ( processorCall() && proc_mod && processorCall()->submodel() == aSubmodel.number() ) { 
   
	    if ( thread == ownerTask->getThread( aSubmodel.number(), processorCall()->getChain() ) ) {
		double temp=  processorCall()->rendezvousDelay( k );
		newWait += temp;
		if ( flags.trace_replication ) {
		    cout << "\n Processor Call: CallChainNum="<<processorCall()->getChain() <<  ",Src=" << processorCall()->srcName() << ",Dst= " << processorCall()->dstName()
			 << ",Wait=" << temp << endl;
		}
		if ( processorCall()->dstTask()->hasServerChain(k) ) {
		    include = true;
		    nr_factor = processorCall()->nrFactor( aSubmodel, k );
		}
	    }   
	}
	
	//REP NEWTON-RAPHSON (if NR selected)

	if ( include ) { 
	    newWait = (newWait + mySurrogateDelay[k] * nr_factor) / (1.0 + nr_factor);
	} else {
	    newWait = 0; 
	}

	delta = max( delta, square( (mySurrogateDelay[k] - newWait) * throughput() ) );
	under_relax( mySurrogateDelay[k], newWait, 1.0 );
	
	if ( flags.trace_replication ) {

	    cout <<"\nMySurrogateDelay of current master chain " <<k<< " =" << mySurrogateDelay[k]<< endl;
	
	    cout << name() << ": waitExceptChain(submodel=" << aSubmodel.number() << ",k=" << k << ") = " << newWait
		 << ", nr_factor = " << nr_factor 
		 << "\n, waitExcept(submodel=" << aSubmodel.number() << ") = " << waitExcept( aSubmodel.number() ) << endl;
	}
    }
    return delta;
}



/*
 * Go through the call list, looking for deterministic
 * rendezvous/async calls, to activity entries then follow them to a
 * join.
 */

unsigned
Phase::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase )
{
    unsigned max_depth = entryStack.size();
	
    /* Store in path list */

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    while ( aCall = nextCall() ) {
	max_depth = max( aCall->followInterlock( entryStack, globalCalls, callingPhase ), max_depth );
    }

    if ( processorCall() ) {
	max_depth = max( processorCall()->followInterlock( entryStack, globalCalls, callingPhase ), max_depth );
    }
    return max_depth;

}


/*
 * Recursively search from this entry to any entry on myServer.
 * When we pop back up the call stack we add all calling tasks
 * for each arc which calls myServer.  The task adder
 * will ignore duplicates.
 *
 * Note: we can't short circuit the search because there may be interlocking
 * on multiple branches.
 */

bool
Phase::getInterlockedTasks( Stack<const Entry *>& stack, const Entity * myServer, 
			    Cltn<const Entity *>& interlockedTasks, const unsigned ) const
{
    bool found = false;

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    while ( aCall = nextCall() ) {
	const Entry * dstEntry = aCall->dstEntry();
	if ( !stack.find( dstEntry ) && dstEntry->getInterlockedTasks( stack, myServer, interlockedTasks ) ) {
	    found = true;
	}
    }

    if ( processorCall() ) {
	if ( processorCall()->dstEntry()->getInterlockedTasks( stack, myServer, interlockedTasks ) ) {
	    found = true;
	}
    }
    return found;
}


void
Phase::insertDOMResults() const
{
    myDOMPhase->setResultServiceTime(elapsedTime())
	.setResultVarianceServiceTime(variance())
	.setResultUtilization(utilization())
	.setResultProcessorWaiting(queueingTime());

    /*+ BUG 675 */
    if ( myDOMPhase->hasHistogram() ) {
	insertDOMHistogram( const_cast<LQIO::DOM::Histogram *>(myDOMPhase->getHistogram()), elapsedTime(), variance() );
    }
    /*- BUG 675 */

    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    while ( aCall = nextCall() ) {
	if ( !aCall->hasForwarding() ) {
	    aCall->insertDOMResults();		/* Forwarded calls are done by their proxys (ForwardCall) */
	}
    }
}


/*
 * Recalculate the service time and visits to the processor.  Only go
 * through the hoops if the value changes.
 */

void 
Phase::recalculateDynamicValues()
{	
    if ( !myProcessorEntry ) return;
	
    const bool phase_is_present = serviceTime() > 0 || isPseudo();
    const double nCalls = phase_is_present ? numberOfSlices() : 0.0;
    const double newSliceTime = phase_is_present ? serviceTime() / nCalls : 0.0;
    const double oldSliceTime = myProcessorEntry->serviceTime(1);
    if ( oldSliceTime != newSliceTime || flags.full_reinitialize ) {
	myProcessorEntry->setServiceTime(newSliceTime).setCV_sqr(CV_sqr());

	myProcessorEntry->initVariance();	// Reset variance.
	myProcessorEntry->initWait();		// Reset values in wait[].
		
	LQIO::DOM::Call* callDOM = myProcessorCall->getCallDOM();
	const LQIO::DOM::Document * aDocument = getDOM()->getDocument();
	if ( callDOM ) delete callDOM;
	myProcessorCall->rendezvous( new LQIO::DOM::Call(aDocument, LQIO::DOM::Call::QUASI_RENDEZVOUS, 
							 NULL, myProcessorEntry->getDOM(), new LQIO::DOM::ConstantExternalVariable(nCalls)) );
	/* Recompute dynamic values. */
		
	const double newWait = oldSliceTime > 0.0 ? myProcessorCall->wait() * newSliceTime / oldSliceTime : newSliceTime;
	myProcessorCall->setWait( newWait );		/* extrapolate value */
    }

    if ( hasThinkTime() ) {
	const double newThinkTime = thinkTime();
	const double oldThinkTime = myThinkEntry->serviceTime(1);
	if ( oldThinkTime != newThinkTime || flags.full_reinitialize ) {
	    myThinkEntry->setServiceTime(newThinkTime);
	    myThinkEntry->initVariance();	// Reset variance.
	    myThinkEntry->initWait();		// Reset values in wait[].
		
	    /* Recompute dynamic values. */
		
	    myThinkCall->setWait( newThinkTime );
	}
    }
}

/*----------------------------------------------------------------------*/
/*                       Variance Calculation                           */
/*----------------------------------------------------------------------*/

/*
 * Compute variance. 
 */

double
Phase::computeVariance() 
{
    if ( !isfinite( elapsedTime() ) ) {
	myVariance = elapsedTime();
    } else switch ( pragma.getVariance() ) {

	case MOL_VARIANCE:
	    if ( phaseTypeFlag() == PHASE_STOCHASTIC ) {
		myVariance =  mol_phase();
		break;
	    } else {
		myVariance =  deterministic_phase();
	    }
	    break;

	case STOCHASTIC_VARIANCE:
	    if ( phaseTypeFlag() == PHASE_STOCHASTIC ) {
		myVariance =  stochastic_phase();
		break;
	    } else {
		myVariance =  deterministic_phase();
	    }
	    break;
		
	case DEFAULT_VARIANCE:
	    if ( phaseTypeFlag() == PHASE_STOCHASTIC ) {
		myVariance =  stochastic_phase();
	    } else {
		myVariance =  deterministic_phase();
	    }
	    break;

	default:
	    myVariance =  square( elapsedTime() );
	    break;
	}

    return myVariance;
}



/* --------------------Random Sum Calculation-----------------------
 *
 * Classic variance calculation based on a random sum:
 *
 * A phase is a sum of a slice, plus a random sum for each call (a
 * random number of calls to that entry). Each element in one of these
 * random sums is itself the call blocking delay plus a processor
 * slice.
 */

double
Phase::stochastic_phase() const
{
    /* set up the running sums with the first slice, and save slice values*/
    double proc_wait = processorWait();
    double proc_var = processorVariance();

    Positive sumOfVariance = proc_var;

    /* over all calls made in this phase */
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	if ( !aCall->hasRendezvous() ) continue;

	/* Formula for a random sum: add terms due to one call */

	/* first extract the properties of the delay for one call instance*/
	const double blocking_mean = aCall->wait(); //includes service ph 1
	// + Positive( aCall->dstEntry()->elapsedTime(1) ); 
	/* mean delay for one of these calls */
	if ( !isfinite( blocking_mean ) ) {
	    return blocking_mean;
	}

	const double blocking_var = aCall->variance();		/* BUG_655 */
	if ( !isfinite( blocking_var ) ) {
	    return blocking_var;
	}
	// this includes variance due to service
	// + square (Positive( aCall->dstEntry()->elapsedTime(1) )) * Positive( aCall->dstEntry()->computeCV_sqr(1) ); 
	/*  variance of one of these calls */
 
	/* then add up; the sum accounts for no of calls and fanout  */
	//accumulate mean sum
	const unsigned int fan_out = aCall->fanOut();
	double calls_var = aCall->rendezvous() * fan_out * (aCall->rendezvous() * fan_out + 1 );  //var of no of calls if geometric
	sumOfVariance += aCall->rendezvous() * fan_out * (blocking_var + proc_var) 
	    + calls_var * square(proc_wait + blocking_mean ); //accumulate variance sum
    }

    return sumOfVariance;
}




/* ------------------------- Method Of Layers -------------------------
 *
 * Variance calculation from pages 124-134 of:
 *     author =  "Rolia, Jerome Alexander",
 *     title =   "Predicting the Performance of Software Systems",
 *     school =  "Univerisity of Toronto",
 *     year =    1992,
 *     address = "Toronto, Ontario, Canada.  M5S 1A1",
 *     month =   jan,
 *     note =    "Also as: CSRI Technical Report CSRI-260",
 */

/*
 * Compute the variance for a phase of an entry.  We have to chase
 * down all calls that the entry's phase makes and generate a
 * series parallel model.
 */


double
Phase::mol_phase() const
{
    const double sumOfRNV = sumOfRendezvous();
    Positive variance;
    if ( sumOfRNV == 0 ) {
	variance = processorVariance();
	return variance;
    }
	
    /*
     * Construct a "Series Parallel" model over all entries that I call...
     */
    SeriesParallel SP_Model;

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	if ( !aCall->hasRendezvous() ) continue;


	Probability prVisit = aCall->rendezvous() / sumOfRNV;
	for ( unsigned i = 1; i <= aCall->fanOut(); ++i ) {
#ifdef NOTDEF
	    SP_Model.addStage( prVisit, aCall->wait(), Positive( aCall->CV_sqr() ) );
#else
	    /* Use variance of phase, not of phase+arc */
	    SP_Model.addStage( prVisit, aCall->wait(), 
			       Positive( aCall->dstEntry()->computeCV_sqr(1) ) );
#endif
	}
    }

    /*
     * Now compute the variance for the phase of the entry itself.
     * Don't forget to include the visit to the processor (+ 1).
     */

    const Probability q   = 1.0 / (sumOfRNV + 1);
    const Probability v   = 1.0 - q;
    const double r_iter   = processorWait() + SP_Model.S();		// Pg 132 says: v * r_ncs.
    const double var_iter = processorVariance() + v * SP_Model.variance() 
	+ v * ( 1.0 - v ) * square(SP_Model.S());
    variance = var_iter / q + ( 1.0 - q ) / square( q ) * square( r_iter );

    return variance;
}


/*
 * Variance calculation for deterministic phases (See eqn 12 and 13
 * from srvn6 paper.)  Code is from srvn6 solver.  There are some
 * discrepencies between the two. 
 */

double
Phase::deterministic_phase() const
{
    Positive variance = 0;

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
	
    while ( aCall = nextCall() ) {
	const double var = aCall->variance();
	if ( isfinite( var ) ) {
	    variance += aCall->fanOut() * aCall->rendezvous() * var;
	}
    }

    /* Add processor term (from old code.) */

    variance += numberOfSlices() * processorVariance();		/* BUG 447 */

    return variance;
}



/*
 * Variance calculation for deterministic phases (See eqn 12 and 13
 * from srvn6 paper.)  Code is from srvn6 solver.  There are some
 * discrepencies between the two. 
 */

double
Phase::random_phase() const
{
    Positive var_x = 0;
    Positive sum_x = 0; 
    Positive mean_n = sumOfRendezvous();

    if ( mean_n == 0.0 ) {
	return processorVariance();
    }

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
	
    while ( aCall = nextCall() ) {
	if ( !aCall->hasRendezvous() ) continue;

#ifdef NOTDEF
	var_x += aCall->fanOut() * aCall->rendezvous() * (aCall->dstEntry()->computeCV_sqr(1) * square(aCall->wait()));
#else
	var_x += aCall->fanOut() * aCall->rendezvous() * aCall->variance();
#endif
	sum_x += aCall->rendezvousDelay();
    }

    var_x  += processorCalls() * processorVariance();
    sum_x  += processorCalls() * processorWait();

    return square(mean_n) * square( sum_x / mean_n ) + var_x;
}



/*
 * If there is a processor associated with this phase initialize it.
 * Do this during initialization, rather than construction as we
 * don't know how many slices exist until the entire model is loaded.
 */
	
void
Phase::initProcessor()
{	
    if ( !owner()->processor() || myProcessorEntry ) return;
	
    /* If I don't have an entry, create one */
	
    if ( getDOM()->hasServiceTime() ) {
	string entry_name( owner()->processor()->name() );
	entry_name += ':';
	entry_name += name();
	
	double nCalls = numberOfSlices();
		
	/* 
	 * [MM] myProcessorEntry used to have an Entry* in front of it, however it is also the name
	 * of an instance variable of phase, which means that it was being shadowed and never set. 
	 * I switched that out, now we have a proper link.
	 */

	const LQIO::DOM::Document * aDocument = getDOM()->getDocument();
	myProcessorEntry = new DeviceEntry( new LQIO::DOM::Entry(aDocument, entry_name.c_str()), ::entry.size() + 1, 
					    const_cast<Processor *>( owner()->processor()) );
	::entry.insert( myProcessorEntry );
		
	myProcessorEntry->setServiceTime( serviceTime() / nCalls )
	    .setCV_sqr( CV_sqr() )
	    .setPriority( owner()->priority() );
	myProcessorEntry->initVariance();

	/* 
	 * We may have to change this at some point.  However, we can't do
	 * priority by class in the analytic solver anyway - only by
	 * chain.
	 */	

	myProcessorCall = newProcessorCall( myProcessorEntry );
	LQIO::DOM::Call* processorCallDom = new LQIO::DOM::Call(aDocument, LQIO::DOM::Call::QUASI_RENDEZVOUS, 
								NULL, myProcessorEntry->getDOM(), 
								new LQIO::DOM::ConstantExternalVariable(nCalls));
	myProcessorCall->rendezvous( processorCallDom );
    }

    /*
     * Now create entries and connect.  Note that the entries are DeviceEntries
     * rather than TaskEntries (so that DeltaWait and friends don't get confused).
     * DeviceEntries do not have processors, nor do they call anyone.
     */

    if ( hasThinkTime() ) {
			
	string think_entry_name;
	think_entry_name = Model::thinkServer->name();
	think_entry_name += "-";
	think_entry_name += name();

	const LQIO::DOM::Document * aDocument = getDOM()->getDocument();
	myThinkEntry = new DeviceEntry(new LQIO::DOM::Entry(aDocument, think_entry_name.c_str()), ::entry.size() + 1, 
				       Model::thinkServer );
	::entry.insert( myThinkEntry );
		
	myThinkEntry->setServiceTime(thinkTime()).setCV_sqr(1.0);
	myThinkEntry->initVariance();

	myThinkCall = newProcessorCall( myThinkEntry );
	LQIO::DOM::Call* thinkCallDom = new LQIO::DOM::Call(aDocument, LQIO::DOM::Call::QUASI_RENDEZVOUS, 
							    NULL, myThinkEntry->getDOM(), 
							    new LQIO::DOM::ConstantExternalVariable(1));

	myThinkCall->rendezvous( thinkCallDom );
	addSrcCall( myThinkCall );
    }
}

/* ------------------------------------------------------------------------ */
GenericPhase::GenericPhase()
    : myEntry(0)
{
}




/*
 * Set source entry and phase.
 */

void
GenericPhase::initialize( Entry * src, const int p )
{
    myEntry = src;

    myName = src->name();
    if ( 0 < p && p <= MAX_PHASES ) {
	myName += ':';
	myName += (p + '0');
    } else {
	abort();
    }
}



ProcessorCall *
GenericPhase::newProcessorCall( Entry * procEntry )
{
    return new ProcessorCall( this, procEntry );
}


/*
 * Return the task of this phase.  Overridden by class activity.
 */

const Entity *
GenericPhase::owner() const
{
    return myEntry->owner();
}


/*
 * Return throughut for phase.
 */

double
GenericPhase::throughput() const
{
    return myEntry->throughput();
}
