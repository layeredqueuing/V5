/*  -*- c++ -*-
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January 2003
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#include "lqn2ps.h"
#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <algorithm>
#include <ctype.h>
#include <cmath>
#include <algorithm>
#if defined(HAVE_IEEEFP_H) && !defined(MSDOS)
#include <ieeefp.h>
#endif
#include <limits.h>
#if HAVE_VALUES_H
#include <values.h>
#endif
#if HAVE_FLOAT_H
#include <float.h>
#endif
#include <lqio/input.h>
#include <lqio/error.h>
#include <lqio/dom_extvar.h>
#include <lqio/dom_document.h>
#include "errmsg.h"
#include "cltn.h"
#include "stack.h"
#include "stack.h"
#include "entry.h"
#include "call.h"
#include "task.h"
#include "activity.h"
#include "processor.h"
#include "model.h"
#include "label.h"
#include "arc.h"

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

set<Entry *,ltEntry> entry;

unsigned Entry::max_phases		= 0;

const char * Entry::phaseTypeFlagStr [] = { "Stochastic", "Determin" };

static LabelEntryManip label_execution_time( const Entry& anEntry );


/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Print out service time of entry in standard output format.
 */

ostream&
operator<<( ostream& output, const Entry& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_SRVN:
	break;
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
#endif
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
#endif
    case FORMAT_XML:
	break;
    default:
	self.draw( output );
	break;
    }
    return output;
}

/* ------------------------ Constructors etc. ------------------------- */


Entry::Entry( const LQIO::DOM::DocumentObject * dom )
    : Element( dom, ::entry.size()+1 ),		// Give this entry a unique id
      drawLeft(true),
      drawRight(true),
      myOwner(0),
      myIndex(UINT_MAX),
      myMaxPhase(0),
      calledFlag(NOT_CALLED),
      myCalls(),
      myCallers(),
      isPresent(),
      myActivity(0),
      myActivityCall(0),
      myActivityCallers()
{
    /* Allocate phases */

    phase.grow(MAX_PHASES);
    isPresent.grow(MAX_PHASES);

    const LQIO::DOM::Entry * entryDOM  = dynamic_cast<const LQIO::DOM::Entry *>(dom);
    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	if ( entryDOM->hasPhase(p) ) {
	    phaseIsPresent(p,true);
	    phase[p].setDOM(const_cast<LQIO::DOM::Entry *>(entryDOM)->getPhase(p));
	} 
	phase[p].initialize( this, p );
    }
    myNode = Node::newNode( Flags::entry_width, Flags::entry_height );
    myLabel = Label::newLabel();
}


/*
 * copies everything except who I call etc.
 */

Entry::Entry( const Entry& src )
    : Element( dynamic_cast<const LQIO::DOM::Entry *>(src.getDOM())->clone(), ::entry.size()+1 ),
      drawLeft(true), 
      drawRight(true),
      myOwner(0),
      myIndex(UINT_MAX),
      myMaxPhase(src.myMaxPhase),
      calledFlag(NOT_CALLED),
      myCalls(),
      myCallers(),
      isPresent(),
      myActivity(0),
      myActivityCall(0),
      myActivityCallers()
{	
    phase.grow(MAX_PHASES);
    isPresent.grow(MAX_PHASES);

    LQIO::DOM::Entry * src_dom  = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(src.getDOM()));
    LQIO::DOM::Entry * dst_dom  = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(getDOM()));
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	phase[p] = src.phase[p];
	phase[p].initialize( this, p );
	if ( src.isPresent[p] ) {
	    isPresent[p] = src.isPresent[p];
	    LQIO::DOM::Phase * phase_dom = new LQIO::DOM::Phase( *src_dom->getPhase(p) );
	    phase[p].setDOM( phase_dom );	/* Deep copy */
	    dst_dom->setPhase( p, phase_dom );
	} else {
	    phase[p].setDOM( 0 );
	}
    }
    myNode = Node::newNode( Flags::entry_width, Flags::entry_height );
    myLabel = Label::newLabel();
}




/*
 * Compare entry names for equality.
 */

int
Entry::operator==( const Entry& anEntry ) const
{
    return name() == anEntry.name();
}


/*
 * Reset globals.
 */
 
void
Entry::reset()
{
    max_phases = 0;
}
	


/*
 * Free storage allocated in wait.  Storage was allocated by layerize.c
 * by calling configure.
 */

Entry::~Entry()
{
    /* Release forward links */
	
    myCalls.deleteContents();
    delete myNode;
    delete myLabel;

    if ( myActivityCall ) {
	delete myActivityCall;
    }
    myActivityCallers.clearContents();
}

/* ------------------------ Instance Methods -------------------------- */


bool
Entry::hasPriority() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    
    return dom && dom->getEntryPriority();
}



const LQIO::DOM::ExternalVariable& 
Entry::priority() const
{
    return *dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getEntryPriority();
}



/*
 * Set the max service time value.
 */

Entry& 
Entry::histogram( const unsigned p, const double min, const double max, const unsigned n_bins ) 
{ 
    phase[p].histogram( min, max, n_bins ); 
    return *this; 
}


Entry&
Entry::histogramBin(  const unsigned p, const double begin, const double end, const double prob, const double conf95, const double conf99 )
{
    phase[p].histogramBin( begin, end, prob, conf95, conf99 );
    return *this;
}


const LQIO::DOM::ExternalVariable & 
Entry::Cv_sqr( const unsigned p ) const
{
    assert( phase[p].getDOM() );
    return phase[p].Cv_sqr(); 
}



/*
 * Return a reference so the operators +,-,*,/,<< work.
 */

const LQIO::DOM::ExternalVariable&
Entry::openArrivalRate() const
{
    return *dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getOpenArrivalRate();
}



/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Entry&
Entry::rendezvous( const Entry * toEntry, unsigned int p, const LQIO::DOM::Call * value )
{
    if ( value && const_cast<Entry *>(toEntry)->isCalled( RENDEZVOUS_REQUEST ) ) {
 	Model::rendezvousCount[0] += 1;
	Model::rendezvousCount[p] += 1;
	phaseIsPresent( p, true );
	Call * aCall = findOrAddCall( toEntry, &GenericCall::hasRendezvousOrNone );
	aCall->rendezvous( p, value );
    }

    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

const LQIO::DOM::ExternalVariable &
Entry::rendezvous( const Entry * anEntry, const unsigned p ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasRendezvous );
    if ( aCall ) {
	return aCall->rendezvous(p);
    } else {
	abort();
    }
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

double
Entry::rendezvous( const Entry * anEntry ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasRendezvous  );
    if ( aCall ) {
	return aCall->sumOfRendezvous();
    } else {
	return 0.0;
    }
}



/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Entry&
Entry::sendNoReply( const Entry * toEntry, unsigned int p, const LQIO::DOM::Call * value )
{
    if ( value  && const_cast<Entry *>(toEntry)->isCalled( SEND_NO_REPLY_REQUEST ) ) {
	Model::sendNoReplyCount[0] += 1;
	Model::sendNoReplyCount[p] += 1;
	phaseIsPresent( p, true );
	Call * aCall = findOrAddCall( toEntry, &GenericCall::hasSendNoReplyOrNone );
	aCall->sendNoReply( p, value );
    }
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

const LQIO::DOM::ExternalVariable & 
Entry::sendNoReply( const Entry * anEntry, const unsigned p ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasSendNoReply );
    if ( aCall ) {
	return aCall->sendNoReply(p);
    } else {
	abort();
    }
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

double
Entry::sendNoReply( const Entry * anEntry ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasSendNoReply  );
    if ( aCall ) {
	return aCall->sumOfSendNoReply();
    } else {
	return 0.0;
    }
}



/*
 * Retrieve forwarding probability to entry.
 */

const LQIO::DOM::ExternalVariable &
Entry::forward( const Entry * toEntry ) const
{
    const Call * aCall = findCall( toEntry, &GenericCall::hasForwarding );

    if ( aCall ) {
	return aCall->forward();
    } else {
	abort();
    }
}




Entry&
Entry::forward( const Entry * toEntry, const LQIO::DOM::Call * value ) 
{
    if ( value && const_cast<Entry *>(toEntry)->isCalled( RENDEZVOUS_REQUEST ) ) {
	Model::forwardingCount += 1;
	phaseIsPresent( 1, true );

	Call * aCall = findOrAddCall( toEntry, &GenericCall::hasForwardingOrNone );
	aCall->forward( value );
    }
    return *this;
}




/*
 * Return true if the entry forwards to aTask.
 */

bool
Entry::forwardsTo( const Task * toTask ) const
{
    const Call * aCall = findCall( toTask );

    if ( aCall ) {
	return aCall->hasForwardingLevel();
    } else {
	return false;
    }
}


/*
 * Return fan in.
 */

unsigned
Entry::fanIn( const Entry * toEntry ) const
{
    Call * aCall = findCall( toEntry );
    if ( aCall ) { 
	return aCall->fanIn();
    } else {
	return 0;
    }
}



/*
 * Return fan out.
 */

unsigned
Entry::fanOut( const Entry * toEntry ) const
{
    Call * aCall = findCall( toEntry );
    if ( aCall ) { 
	return aCall->fanOut();
    } else {
	return 0;
    }
}



/*
 * Set starting activity for this entry.
 */

Entry&
Entry::setStartActivity( Activity * anActivity )
{
    myActivity = anActivity;
    myActivityCall = Arc::newArc();
    anActivity->rootEntry( this, myActivityCall );
    myMaxPhase = 1;
    return *this;
}


const LQIO::DOM::Phase * 
Entry::getPhaseDOM( unsigned p ) const
{
    return phase[p].getDOM();
}

/* -------------------------- Result Queries -------------------------- */

double Entry::openWait() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM()); 
    return dom->getResultOpenWaitTime();
}
/* --- */

double Entry::processorUtilization() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM()); 
    return dom->getResultProcessorUtilization();
}

/* --- */

double 
Entry::throughput() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM()); 
    return dom->getResultThroughput();
}

/* --- */

double 
Entry::throughputBound() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM()); 
    return dom->getResultThroughputBound();
}


double
Entry::utilization( const unsigned p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].utilization() : 0.0; 
}


double 
Entry::utilization() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM()); 
    return dom->getResultUtilization();
}

/*
 * Set the value of calls to entry `toEntry', `phase'.  
 */

Call *
Entry::forwardingRendezvous( Entry * toEntry, const unsigned p, const double value )
{
    if ( value > 0.0 && toEntry->isCalled( RENDEZVOUS_REQUEST ) ) {
	ProxyEntryCall * aCall = findOrAddFwdCall( toEntry );
	const LQIO::DOM::Call * dom = aCall->getDOM(p);
	if ( dom ) {
	    /* Reset the value in the old call */
	    LQIO::DOM::ExternalVariable * mean = const_cast<LQIO::DOM::ExternalVariable *>(dom->getCallMean());
	    mean->set(value);
	} else {
	    /* Make a new call */
	    dom = new LQIO::DOM::Call( getDOM()->getDocument(),
				       LQIO::DOM::Call::RENDEZVOUS, 
				       const_cast<LQIO::DOM::Phase *>(phase[p].getDOM()),
				       const_cast<LQIO::DOM::Entry*>(dynamic_cast<const LQIO::DOM::Entry*>(toEntry->getDOM())), p,
				       new LQIO::DOM::ConstantExternalVariable(value) );
	    aCall->rendezvous(p,dom);
	}
	return aCall;
    } else {
	return 0;
    }
}

phase_type 
Entry::phaseTypeFlag( const unsigned p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].phaseTypeFlag() : PHASE_STOCHASTIC;
}


bool 
Entry::hasServiceTime( const unsigned int p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].hasServiceTime() : false; 
}


const LQIO::DOM::ExternalVariable& 
Entry::serviceTime( const unsigned p ) const 
{ 
    assert( phase[p].getDOM() );
    return phase[p].serviceTime(); 
}

double
Entry::serviceTime() const
{
    double sum = 0.0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += LQIO::DOM::to_double(serviceTime(p));
    }
    return sum;
}


double
Entry::executionTime( const unsigned p ) const 
{ 
    if ( phase[p].getDOM() ) {
	return phase[p].executionTime();
    } else if ( getDOM() ) {
	return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getResultPhasePServiceTime(p);
    } else {
	return 0;
    }
}



double
Entry::executionTime() const
{
    double sum = 0.0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += executionTime(p);
    }
    return sum;
}


double
Entry::variance( const unsigned p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].variance() :0.0; 
}


double
Entry::variance() const
{
    double sum = 0.0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += variance(p);
    }
    return sum;
}


double
Entry::serviceExceeded( const unsigned p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].serviceExceeded() :0.0; 
}


double
Entry::serviceExceeded() const
{
    double sum = 0.0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += serviceExceeded(p);
    }
    return sum;
}

/*
 * Set the entry type field.
 */

bool
Entry::isCalled(const requesting_type callType )
{
    if ( calledFlag != NOT_CALLED && calledFlag != callType ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, name().c_str() );
	return false;
    } else {

        /* mark an entry which is called as being present */
	if ( !phaseIsPresent( 1 ) ) {
	    phaseIsPresent( 1, true );
	}

	calledFlag = callType;
	return true;
    }
}


/*
 * Return true if this entry belongs to a reference task.
 * This is an error!
 */

bool 
Entry::isReferenceTaskEntry() const
{
    if ( owner() && owner()->isReferenceTask() ) {
	LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, owner()->name().c_str(), name().c_str() );
	return true;    
    } else {
	return false;
    }
}



/*
 * mark whether the phase is present or not.  
 */

Entry&
Entry::phaseIsPresent( const unsigned ph, const bool yesOrNo )
{
    isPresent[ph] = yesOrNo;
    if ( yesOrNo && myMaxPhase < ph ) {
	myMaxPhase = ph;
		
    } else if ( !yesOrNo && myMaxPhase >= ph ) {
	myMaxPhase = 0;
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    if ( phaseIsPresent(p) ) {
		myMaxPhase = p;
	    }
	}
    }

    max_phases = max( myMaxPhase, max_phases );		/* Set global value.	*/

    return *this;
}



/*
 * Return true is this entry is selected for printing.
 */

bool
Entry::isSelectedIndirectly() const
{
    if ( Flags::print[CHAIN].value.i ) {
	return hasPath( Flags::print[CHAIN].value.i );
    } else if ( owner()->isSelected() ) {
	return true;
    } 

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() ) return true;
    }

    return false;
}



bool
Entry::isActivityEntry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getEntryType() == LQIO::DOM::Entry::ENTRY_ACTIVITY;
}
    
bool
Entry::isStandardEntry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getEntryType() == LQIO::DOM::Entry::ENTRY_STANDARD;
}

bool
Entry::isSignalEntry() const 
{ 
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getSemaphoreFlag() == SEMAPHORE_SIGNAL;
}

bool 
Entry::isWaitEntry() const 
{ 
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getSemaphoreFlag() == SEMAPHORE_WAIT;
}

bool
Entry::is_r_lock_Entry() const 
{ 
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == RWLOCK_R_LOCK;
}	

bool 
Entry::is_r_unlock_Entry() const
{ 
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == RWLOCK_R_UNLOCK;
}

bool 
Entry::is_w_lock_Entry() const
{ 
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == RWLOCK_W_LOCK;
}

bool 
Entry::is_w_unlock_Entry() const
{ 
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == RWLOCK_W_UNLOCK;
}


/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entryTypeOk( const LQIO::DOM::Entry::EntryType aType )
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    const bool rc = const_cast<LQIO::DOM::Entry *>(dom)->entryTypeOk( aType );
    if ( !rc ) {
	LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, name().c_str() );
    }
    return rc;
}


/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entrySemaphoreTypeOk( const semaphore_entry_type aType )
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    const bool rc = const_cast<LQIO::DOM::Entry *>(dom)->entrySemaphoreTypeOk( aType );
    if ( !rc ) {
	LQIO::input_error2( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name().c_str() );
    }
    return rc;
}

bool
Entry::entryRWLockTypeOk( const rwlock_entry_type aType )
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    const bool rc = const_cast<LQIO::DOM::Entry *>(dom)->entryRWLockTypeOk( aType );
    if ( !rc ) {
	LQIO::input_error2( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name().c_str() );
    }
    return rc;
}


/*
 * Return the number of phases for this entry.
 */

unsigned
Entry::numberOfPhases() const
{
    unsigned sum = 0;

    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	if ( phaseIsPresent(p) ) {
	    sum += 1;
	}
    }
    return sum;
}



/*
 * Return the number of execution slices.
 */

double 
Entry::numberSlices( const unsigned p ) const
{
    if ( !hasServiceTime(p) ) return 0.0;

    double nCalls = 1.0;

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	nCalls += LQIO::DOM::to_double(aCall->rendezvous(p));
    }

    return nCalls;
}



/*
 * Return the slice time of this phase
 */

double
Entry::sliceTime( const unsigned p ) const
{
    return hasServiceTime(p) ? LQIO::DOM::to_double(serviceTime(p)) / numberSlices(p) : 0.0;
}



/*
 * Compute and return CV square for this entry.
 */

double
Entry::Cv_sqr() const
{
    const double t = executionTime();

    if ( !finite( t ) ) {
	return t;
    } else if ( t > 0.0 ) {
	return variance() / square(t);
    } else {
	return 0.0;
    }
}



/*
 * Return true if any phase has max service time parameter.
 */

bool
Entry::hasHistogram() const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	if ( phase[p].hasHistogram() ) return true;
    }
    return false;
}



/*
 * Return true if any phase has a call with a rendezvous.
 */

bool
Entry::hasRendezvous() const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    
    while ( aCall = nextCall() ) {
	if ( aCall->hasRendezvous() ) return true;
    }
    return false;
}



/*
 * Return true if any phase has a call with send-no-reply.
 */

bool
Entry::hasSendNoReply() const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    
    while ( aCall = nextCall() ) {
	if ( aCall->hasSendNoReply() ) return true;
    }
    return false;
}



/*
 * Return true if any phase has a call with forwarding.
 */

bool
Entry::hasForwarding() const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    
    while ( aCall = nextCall() ) {
	if ( aCall->hasForwarding() ) return true;
    }
    return false;
}



/*
 * Return true if there is an open arrival rate for this entry.
 */

bool
Entry::hasOpenArrivalRate() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->hasOpenArrivalRate();
}



/*
 * Return true if any phase has a call with forwarding.
 */

bool
Entry::hasForwardingLevel() const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    
    while ( aCall = nextCall() ) {
	if ( aCall->hasForwardingLevel() ) return true;
    }
    return false;
}



bool
Entry::isForwardingTarget() const
{
    Sequence<GenericCall *> nextCall( callerList() );
    GenericCall * aCall;
    
    while ( aCall = nextCall() ) {
	if ( aCall->hasForwardingLevel() ) return true;
    }
    return false;
}



/*
 * Return true if any aFunc returns true for any call 
 */

bool
Entry::hasCalls( const callFunc aFunc ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    
    while ( aCall = nextCall() ) {
	if ( (aCall->*aFunc)() ) return true;
    }
    return false;
}



double 
Entry::maxServiceTime( const unsigned p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].maxServiceTime() : 0.; 
}


/*
 * Return true if any phase has max service time parameter.
 */

bool
Entry::hasMaxServiceTime() const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	if ( phase[p].maxServiceTime() > 0.0 ) return true;
    }
    return false;
}



/*
 * Return 1 if any phase is deterministic.
 */

bool
Entry::hasDeterministicPhases() const
{
    for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	if ( phaseTypeFlag(p) == PHASE_DETERMINISTIC ) return true;
    }
    return false;
}



/*
 * Return 1 if any phase is not exponential $(C^2_v \not= 1)$.
 */

bool
Entry::hasNonExponentialPhases() const
{
    for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	if ( phase[p].isNonExponential() ) return true;
    }
    return false;
}


/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasThinkTime() const
{
    for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	if ( phase[p].hasThinkTime() ) return true;
    }
    return false;
}


bool 
Entry::hasThinkTime( const unsigned int p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].hasThinkTime() : false; 
}


const LQIO::DOM::ExternalVariable& 
Entry::thinkTime( const unsigned p ) const 
{ 
    assert ( phase[p].getDOM() );
    return phase[p].thinkTime(); 
}



double
Entry::queueingTime( const unsigned p ) const 
{ 
    return phase[p].getDOM() != 0 ? phase[p].queueingTime() :0.0; 
}	// Time queued for processor.



/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasQueueingTime() const
{
    for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	if ( queueingTime(p) != 0.0 ) return true;
    }
    return false;
}




/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Entry::findCall( const Entry * anEntry, const callFunc aFunc ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->dstEntry() == anEntry && (!aFunc || (aCall->*aFunc)()) ) return aCall;
    }

    return 0;
}



/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Entry::findCall( const Task * aTask ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isPseudoCall() && aCall->dstTask() == aTask ) return aCall;
    }

    return 0;
}



/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

Call *
Entry::findOrAddCall( const Entry * anEntry, const callFunc aFunc )
{
    Call * aCall = findCall( anEntry, aFunc );

    if ( !aCall ) {
	aCall = new EntryCall( this, anEntry );
	addSrcCall( aCall );
	const_cast<Entry *>(anEntry)->addDstCall( aCall );
    }

    return aCall;
}


/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

ProxyEntryCall *
Entry::findOrAddFwdCall( const Entry * anEntry )
{
    ProxyEntryCall * aCall = dynamic_cast<ProxyEntryCall *>(findCall( anEntry, &GenericCall::isPseudoCall ));

    if ( !aCall ) {
	aCall = new ProxyEntryCall( this, anEntry );
	addSrcCall( aCall );
	const_cast<Entry *>(anEntry)->addDstCall( aCall );
    }

    return aCall;
}


Call *
Entry::findOrAddPseudoCall( const Entry * anEntry )
{
    Call * aCall = findCall( anEntry );

    if ( !aCall ) {
	aCall = new PseudoEntryCall( this, anEntry );
	aCall->linestyle( Graphic::DASHED_DOTTED );
	addSrcCall( aCall );
	const_cast<Entry *>(anEntry)->addDstCall( aCall );
    }

    return aCall;
}


/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Entry::countArcs( const callFunc aFunc ) const
{
    unsigned count = 0;

    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (!aFunc || (aCall->*aFunc)()) ) {
	    count += 1;
	}
    }
    return count;
}




/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Entry::countCallers( const callFunc aFunc ) const
{
    unsigned count = 0;

    Sequence<GenericCall *> nextCall( callerList() );
    const GenericCall * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (!aFunc || (aCall->*aFunc)()) ) {
	    count += 1;
	}
    }
    return count;
}



/*
 * Aggregate activity service time (and calls) to phase p.
 * Results don't make sense, so don't bother with them.  Entry
 * results should be available anyway.
 */

Entry&
Entry::aggregateService( const Activity * anActivity, const unsigned p, const double rate ) 
{
    Sequence<Call *> nextCall( anActivity->callList() );
    Call * srcCall;

    while ( srcCall = nextCall() ) {
	Entry * dstEntry = const_cast<Entry *>(srcCall->dstEntry());

	/* Aggregate the calls made by the activity to the entry */

	Call * dstCall;
	if ( srcCall->isPseudoCall() ) {
	    dstCall = findOrAddFwdCall( dstEntry );
	} else {
	    dstCall = findOrAddCall( dstEntry );
	}
	dstCall->merge( p, *srcCall, rate );

	dstEntry->removeDstCall( srcCall );	/* Unlink the activity's call. */
    }


    /* Aggregate the service time made by the activity to the entry */
    
    if ( phase[p].getDOM() ) {
	LQIO::DOM::ExternalVariable& time = *const_cast<LQIO::DOM::ExternalVariable *>(phase[p].getDOM()->getServiceTime());
	time += anActivity->serviceTime() * rate;
    }
    return *this;
}



/*
 * Move all phases up to phase 1.  Activities must have been aggregated prior to invoking this.
 * (see Task::topologicalSort).
 */

Entry&
Entry::aggregatePhases()
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    while ( aCall = nextCall() ) {
	aCall->aggregatePhases();
    }
    LQIO::DOM::Phase * phase_1 = const_cast<LQIO::DOM::Phase *>(phase[1].getDOM());
    if ( !phase_1 ) {
	phase_1 = new LQIO::DOM::Phase( getDOM()->getDocument(), const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(getDOM())) );
	phase_1->setServiceTimeValue( 0.0 );
	phase[1].setDOM( phase_1 );
	phaseIsPresent( 1, true );
    }
    LQIO::DOM::ExternalVariable * service_time = const_cast<LQIO::DOM::ExternalVariable *>(phase_1->getServiceTime());
    assert( service_time );
    for ( unsigned p = 2; p <= maxPhase(); ++p ) {
	if ( !phase[p].getDOM() ) continue;
	(*service_time) += phase[p].serviceTime();
	phase_1->setResultServiceTime( executionTime( 1 ) + executionTime( p ) );
	phaseIsPresent( p, false );
    }
    return *this;
}



/*
 * Called by xxparse when we don't have a total.
 */

const Entry&
Entry::addThptUtil( double & tput_sum, double & util_sum ) const
{
    tput_sum += throughput();
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	phase[p].addThptUtil( util_sum );
    }
    return *this;
}

/*
 * Chase calls looking for cycles and the depth in the call tree.  
 * The return value reflects the deepest depth in the call tree.
 */

unsigned
Entry::findChildren( CallStack& callStack, const unsigned directPath ) const
{
    Sequence<Call *> nextCall( callList() );
    unsigned max_depth = max( followCalls( owner(), nextCall, callStack, directPath ), callStack.size() );

    Activity * anActivity = startActivity();
    if ( anActivity ) {
	const unsigned size = dynamic_cast<const Task *>(owner())->activities().size();
	Stack<const Activity *> activityStack( size );		// For checking for cycles.
	try {
	    max_depth = max( anActivity->findChildren( callStack, directPath, activityStack ), max_depth );
	}
	catch ( activity_cycle& error ) {
	    LQIO::solution_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, owner()->name().c_str(), error.what() );
	    max_depth = max( max_depth, error.depth() );
	}
    }
    return max_depth;
}



/*
 * Check entry data.
 */

void
Entry::check() const
{
    /* concordance between c, phase_flag */

    if ( startActivity() ) {

	Stack<const Activity *> activityStack( owner()->activities().size() ); 
	unsigned next_p = 1;
	double replies = startActivity()->aggregate( this, 1, next_p, 1.0, activityStack, &Activity::aggregateReplies );
	if ( isCalled() == RENDEZVOUS_REQUEST ) {
	    if ( replies == 0.0 ) {
		LQIO::solution_error( LQIO::ERR_REPLY_NOT_GENERATED, name().c_str() );
	    } else if ( fabs( replies - 1.0 ) > EPSILON ) {
		LQIO::solution_error( LQIO::ERR_NON_UNITY_REPLIES, replies, name().c_str() );
	    }
	}

    } else {
	bool hasServiceTime = false;
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    if ( phaseIsPresent( p ) ) {
		phase[p].check();
		hasServiceTime = hasServiceTime || phase[p].hasServiceTime();
	    }
	}

	/* Service time for the entry? */

	if ( !hasServiceTime ) {
	    LQIO::solution_error( LQIO::WRN_NO_SERVICE_TIME, name().c_str() );
	    const_cast<Entry *>(this)->phaseIsPresent( 1, true );	/* force phase presence. */
	}

	/* Set some globals for output formatting */

	Model::thinkTimePresent     = Model::thinkTimePresent     || hasThinkTime();
	Model::boundsPresent        = Model::boundsPresent        || throughputBound() > 0.0;
    }

    if ( (isSignalEntry() || isWaitEntry()) && owner()->scheduling() != SCHEDULE_SEMAPHORE ) {
	LQIO::solution_error( LQIO::ERR_NOT_SEMAPHORE_TASK, owner()->name().c_str(), (isSignalEntry() ? "signal" : "wait"), name().c_str() );
    }

	if ( (is_r_lock_Entry() || is_r_unlock_Entry() || is_w_unlock_Entry()|| is_w_lock_Entry() ) && owner()->scheduling() != SCHEDULE_RWLOCK ) {
		 if ( is_r_lock_Entry() || is_r_unlock_Entry() ) {
			LQIO::solution_error( LQIO::ERR_NOT_RWLOCK_TASK, owner()->name().c_str(),
			(is_r_lock_Entry() ? "r_lock" : "r_unlock"),
			name().c_str() );
		}else{
			LQIO::solution_error( LQIO::ERR_NOT_RWLOCK_TASK, owner()->name().c_str(),
			(is_w_lock_Entry() ? "w_lock" : "w_unlock"),
			name().c_str());
		}	
    }


    /* Forwarding probabilities o.k.? */
		
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    double sum = 0;
    while ( aCall = nextCall() ) {
	sum += LQIO::DOM::to_double(aCall->forward());		// Overloaded operator.
    }
    if ( sum < 0.0 || 1.0 < sum ) {
	LQIO::solution_error(LQIO::ERR_INVALID_FORWARDING_PROBABILITY, name().c_str(), sum );
    } else if ( sum != 0.0 && owner()->isReferenceTask() ) {
	LQIO::solution_error( LQIO::ERR_REF_TASK_FORWARDING, owner()->name().c_str(), name().c_str() );
    }
}



/*
 * Aggregate activities to this entry.
 */


Entry&
Entry::aggregate()
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    if ( startActivity() ) {

	Stack<const Activity *> activityStack( owner()->activities().size() ); 
	unsigned next_p = 1;
	startActivity()->aggregate( this, 1, next_p, 1.0, activityStack, &Activity::aggregateService );

	switch ( Flags::print[AGGREGATION].value.i ) {
	case AGGREGATE_ACTIVITIES:
	case AGGREGATE_PHASES:
	case AGGREGATE_ENTRIES:
	    const_cast<LQIO::DOM::Entry *>(dom)->setEntryType( LQIO::DOM::Entry::ENTRY_STANDARD );
	    break;

	case AGGREGATE_SEQUENCES:
	case AGGREGATE_THREADS:
	    if ( startActivity()->transmorgrify() ) {
		const_cast<LQIO::DOM::Entry *>(dom)->setEntryType( LQIO::DOM::Entry::ENTRY_STANDARD );
	    }
	    break;

	default:
	    abort();
	}
    }

    /* Convert entry if necessary */

    if ( dom->getEntryType() == LQIO::DOM::Entry::ENTRY_STANDARD ) {
	myActivity = 0;
	if ( myActivityCall ) {
	    delete myActivityCall;
	    myActivityCall = 0;
	}
	myActivityCallers.clearContents();
    }

    switch ( Flags::print[AGGREGATION].value.i ) {
    case AGGREGATE_PHASES:
    case AGGREGATE_ENTRIES:
	aggregatePhases();
	break;
    }

    return *this;
}


/*
 * Aggregate all entries to the task level
 */

const Entry&
Entry::aggregateEntries( const unsigned k ) const
{
    if ( !hasPath( k ) ) return *this;		/* Not for this chain! */

    Task * srcTask = const_cast<Task *>(owner());

    const double scaling = srcTask->throughput() ? (throughput() / srcTask->throughput()) : (1.0 / srcTask->nEntries());
    const double s = srcTask->serviceTime( k ) + scaling * serviceTimeForSRVNInput();
    srcTask->serviceTime( k, s );
    const_cast<Processor *>(owner()->processor())->serviceTime( k, s );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	TaskCall * aTaskCall;
	if ( aCall->isPseudoCall() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddFwdCall( aCall->dstTask() ));
	} else if ( aCall->hasRendezvous() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddCall( aCall->dstTask(), &GenericCall::hasRendezvous ));
	    aTaskCall->rendezvous( aTaskCall->rendezvous() + scaling * aCall->sumOfRendezvous() );
	} else if ( aCall->hasSendNoReply() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddCall( aCall->dstTask(), &GenericCall::hasSendNoReply ));
	    aTaskCall->sendNoReply( aTaskCall->sendNoReply() + scaling * aCall->sumOfSendNoReply() );
	} else if ( aCall->hasForwarding() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddCall( aCall->dstTask(), &GenericCall::hasForwarding ));
	    aTaskCall->taskForward( aTaskCall->forward() + scaling * aCall->forward() );
	} else {
	    abort();
	}
    }

    return *this;
}



/*
 * Set the chains used by this entry.  (for queueing networks).
 */

unsigned 
Entry::setChain( unsigned curr_k, unsigned next_k, const Entity * aServer, callFunc aFunc ) const
{
    if ( aFunc != &GenericCall::hasSendNoReply && (!aServer || (owner()->processor() == aServer) ) ) { 
	const_cast<Entry *>(this)->setServerChain( curr_k ).setClientClosedChain( curr_k );		/* Catch case where there are no calls. */
    }

    if ( startActivity() ) {
	Stack<const Activity *> activityStack( owner()->activities().size() ); 
	return startActivity()->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } else {
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    if ( phaseIsPresent( p ) ) {
		phase[p].setChain( curr_k, aServer, aFunc );
	    }
	}
    }
    return next_k;
}


/*
 * Set the client chain to k.
 */

Entry&
Entry::setClientClosedChain( unsigned k )
{
    Element::setClientClosedChain( k );
    const_cast<Task *>(owner())->setClientClosedChain( k );
    return *this;
}


/*
 * Set the client chain to k.
 */

Entry&
Entry::setClientOpenChain( unsigned k )
{
    Element::setClientOpenChain( k );
    const_cast<Task *>(owner())->setClientOpenChain( k );
    return *this;
}


/*
 * Set the server chain k.
 */

Entry&
Entry::setServerChain( unsigned k )
{
    const_cast<Task *>(owner())->setServerChain( k );
    Element::setServerChain( k );
    return *this;
}


/*
 * Return all clients to this entry.
 */

unsigned
Entry::referenceTasks( Cltn<const Entity *> &clientsCltn, Element * dst ) const
{
    if ( owner()->isReferenceTask() ) {
//!!! Check for phase 2, except reference task.
	clientsCltn += owner();
//!!! Need to create the pseudo arc to the task.
	if ( dynamic_cast<Processor *>(dst) ) {
	    const_cast<Task *>(owner())->findOrAddPseudoCall( dynamic_cast<Processor *>(dst) );
	} else if ( Flags::print[AGGREGATION].value.i ==  AGGREGATE_ENTRIES ) {
	    const_cast<Task *>(owner())->findOrAddPseudoCall( dynamic_cast<Entry *>(dst)->owner() );
	} else {
	    const_cast<Entry *>(this)->findOrAddPseudoCall( dynamic_cast<Entry *>(dst) );
	}
    } else {
	Sequence<GenericCall *> nextCall( callerList() );
	GenericCall * aCall;
	while ( aCall = nextCall() ) {
	    const Entity * aTask = aCall->srcTask();
//	if ( aCall->isSelectedIndirected() && aTask->pathTest() ) {
//	    if ( aTask->pathTest() ) {
		aTask->referenceTasks( clientsCltn, dst );
//	    }
	}
    }
    return clientsCltn.size();
}



/*
 * Return all clients to this entry.
 */

unsigned
Entry::clients( Cltn<const Entity *> &clientsCltn, const callFunc aFunc ) const
{
    Sequence<GenericCall *> nextCall( callerList() );
    GenericCall * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (!aFunc || (aCall->*aFunc)()) && aCall->srcTask()->pathTest() ) {
	    clientsCltn += aCall->srcTask();
	}
    }
    return clientsCltn.size();
}



/*
 * Return the index used for sorting.
 */

double
Entry::getIndex() const
{
    double anIndex = MAXDOUBLE;

    Sequence<GenericCall *> nextCall( callerList() );
    GenericCall * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isPseudoCall() ) {
	    anIndex = min( anIndex, aCall->srcIndex() );
	}
    }

    return anIndex;
}



/*
 * Move entries higher up more left.
 */

int
Entry::span() const
{
    Sequence<GenericCall *> nextCall( callerList() );
    GenericCall * aCall;

    int mySpan = 0;
    const int myLevel = owner()->level();
    while ( aCall = nextCall() ) {
	mySpan = max( mySpan, myLevel - aCall->srcLevel() );
    }
    return mySpan;
}



Graphic::colour_type 
Entry::colour() const
{
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_RESULTS:
	if ( serviceExceeded() > 0. ) {
	    return Graphic::RED;
	}
	break;

    case COLOUR_CLIENTS:
	if ( myPaths.size() ) {
	    return (Graphic::colour_type)(myPaths.min() % 7 + 3);
	} else {
	    return Graphic::DEFAULT_COLOUR;
	}

    } 

    return owner()->colour();
}


/*
 * Move the entry to x, y.  Drag all the arcs along with it.
 * Reply arcs could be smarter.  We can't sort them right now.
 */

Entry& 
Entry::moveTo( const double x, const double y )
{
    Element::moveTo( x, y );
    myLabel->moveTo( myNode->center() );
    
    moveSrc();		/* Move Arcs	*/
    moveDst();

    if ( myActivityCall ) {
	myActivityCall->moveSrc( bottomCenter() );

	myActivityCallers.sort( Call::compareDst );

	Sequence<Reply *> nextReply( myActivityCallers );
	Reply * aReply;
	Cltn<Reply *> leftCltn;
	Cltn<Reply *> rightCltn;

	Point dstPoint = bottomLeft();

	/* Sort left and right */

	while ( aReply = nextReply() ) {
	    if ( aReply->srcActivity()->left() < left() ) {
		rightCltn << aReply;
	    } else {
		leftCltn << aReply;
	    }
	}

	/* move leftCltn */

	double delta = width() / (static_cast<double>(1+leftCltn.size()) * 2.0);
	for ( unsigned int i = 1; i <= leftCltn.size(); ++i ) {
	    dstPoint.moveBy( delta, 0 );
	    leftCltn[i]->moveDst( dstPoint );
	}

	/* move rightCltn */

	delta = width() / (static_cast<double>(1+rightCltn.size()) * 2.0);
	dstPoint = bottomCenter();
	for ( unsigned int i = 1; i <= rightCltn.size(); ++i ) {
	    dstPoint.moveBy( delta, 0 );
	    rightCltn[i]->moveDst( dstPoint );
	}
	
    }
    return *this;
}


/*
 * Move all arcs I source.
 */

Entry&
Entry::moveSrc()
{
    myCalls.sort( Call::compareSrc );

    Sequence<Call *> nextCall( callList() );
    Call * srcCall;

    const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
    Point aPoint = myNode->bottomLeft();
    const double delta = width() / static_cast<double>(countArcs() + 1 - nFwd );
	
    while ( srcCall = nextCall() ) {
	if ( srcCall->isSelected() && !srcCall->hasForwardingLevel() ) {
	    aPoint.moveBy( delta, 0 );
	    srcCall->moveSrc( aPoint );
	}
    }

    return *this;
}



/*
 * Move all arcs I sink.
 */

Entry&
Entry::moveDst()
{
    myCallers.sort( Call::compareDst );
    Sequence<GenericCall *> nextRefr( callerList() );
    GenericCall * dstCall;
    Point aPoint = myNode->topLeft();

    if ( Flags::print_forwarding_by_depth ) {
	const double delta = width() / static_cast<double>(countCallers() + 1 );

	while ( dstCall = nextRefr() ) {
	    if ( dstCall->isSelected() ) {
		aPoint.moveBy( delta, 0 );
		dstCall->moveDst( aPoint );
	    }
	}

    } else {

	/* 
	 * We add the outgoing forwarding arcs to the incomming side of the entry,
	 * so adjust the counts as necessary.
	 */

	const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
	const double delta = width() / static_cast<double>(countCallers() + 1 + nFwd );
	const double fy1 = Flags::print[Y_SPACING].value.f / 2.0 + top();
	const double fy2 = Flags::print[Y_SPACING].value.f / 1.5 + top();

	/* Draw incomming forwarding arcs first. */

	Point leftPoint( left(), fy1 );
	Point rightPoint( right(), fy2 );
	while ( dstCall = nextRefr() ) {
	    if ( dstCall->isSelected() && dstCall->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		dstCall->moveDst( aPoint );
		if ( dstCall->srcIndex() < owner()->index() ) {
		    dstCall->movePenultimate( leftPoint );
		    leftPoint.moveBy( delta, 0 );
		} else {
		    dstCall->movePenultimate( rightPoint );
		    rightPoint.moveBy( -delta, 0 );
		}
	    }
	}

	/* Draw other incomming arcs. */

	while ( dstCall = nextRefr() ) {
	    if ( dstCall->isSelected() && !dstCall->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		if ( dstCall->hasForwarding() ) {
		    dstCall->movePenultimate( aPoint ).moveSecond( aPoint );
		}
		dstCall->moveDst( aPoint );
	    }
	}

	/* Draw outgoing forwarding arcs */

	rightPoint.moveTo( right(), fy1 );
	leftPoint.moveTo( left(), fy2 );
	Sequence<Call *> nextCall( callList() );
	Call * srcCall;
	while ( srcCall = nextCall() ) {
	    if ( srcCall->isSelected() && srcCall->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		srcCall->moveSrc( aPoint );
		if ( srcCall->dstIndex() >= owner()->index() ) {
		    srcCall->moveSecond( rightPoint );
		    rightPoint.moveBy( delta, 0 );
		} else {
		    srcCall->moveSecond( leftPoint );
		    leftPoint.moveBy( -delta, 0 );
		}
	    }
	}
    }

    return *this;
}



Entry& 
Entry::scaleBy( const double sx, const double sy )
{
    Element::scaleBy( sx, sy );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->scaleBy( sx, sy );
    }

    if ( myActivityCall ) {
	myActivityCall->scaleBy( sx, sy );
    }

    return *this;
}



Entry& 
Entry::translateY( const double dy )
{
    Element::translateY( dy );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->translateY( dy );
    }

    if ( myActivityCall ) {
	myActivityCall->translateY( dy );
    }

    return *this;
}



Entry& 
Entry::depth( const unsigned depth  )
{
    Element::depth( depth-1 );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->depth( depth-2 );
    }

    if ( myActivityCall ) {
	myActivityCall->depth( depth-2 );
    }

    return *this;
}



/*
 * Label the node.
 */

Entry&
Entry::label()
{
    myLabel->initialize( name() );
    if ( !startActivity() && Flags::print[INPUT_PARAMETERS].value.b ) {
	myLabel->newLine() << '[' << print_service_time(*this)  << ']';
	if ( hasThinkTime() ) {
	    myLabel->newLine() << "Z=[" << print_think_time(*this)  << ']';
	}
    }
    if ( Flags::have_results ) {
	if ( Flags::print[SERVICE].value.b ) {
	    myLabel->newLine() << begin_math() << label_execution_time(*this) << end_math();
	}
	if ( Flags::print[VARIANCE].value.b && Model::variancePresent ) {
	    myLabel->newLine() << begin_math( &Label::sigma ) << "=" << print_variance(*this) << end_math();
	}
	if ( Flags::print[SERVICE_EXCEEDED].value.b && serviceExceeded() > 0. ) {
	    myLabel->newLine() << "!=";
	}
	if ( Flags::print[THROUGHPUT_BOUNDS].value.b && Model::boundsPresent ) {
	    myLabel->newLine() << begin_math() << "L=" << throughputBound() << end_math();
	}
	if ( Flags::print[ENTRY_THROUGHPUT].value.b ) {
	    myLabel->newLine() << begin_math( &Label::lambda ) << "=" << throughput() << end_math();
	}
	if ( Flags::print[ENTRY_UTILIZATION].value.b ) {
	    myLabel->newLine() << begin_math( &Label::mu ) << "=" << utilization() << end_math();
	}
    }

    /* Now do calls. */

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->label();
    }

    return *this;
}



const Entry& 
Entry::labelQueueingNetworkVisits( Label& aLabel ) const
{
    Sequence<GenericCall *> nextCall( callerList() );
    GenericCall * aCall;

    while ( aCall = nextCall() ) {
	if ( !dynamic_cast<Call *>(aCall) ) continue;
	if ( aCall->hasRendezvous() ) {
	    aLabel << aCall->srcName() << ':' << aCall->dstName() << " ("
		   << print_rendezvous( *dynamic_cast<Call *>(aCall) ) << ")";
	    aLabel.newLine();
	}
	if ( aCall->hasSendNoReply() ) {
	    aLabel << aCall->srcName() << ':' << aCall->dstName() << " ("
		   << print_sendnoreply( *dynamic_cast<Call *>(aCall) ) << ")";
	    aLabel.newLine();
	}
    }

    return *this;
}



const Entry& 
Entry::labelQueueingNetworkWaiting( Label& aLabel ) const
{
    Sequence<GenericCall *> nextCall( callerList() );
    GenericCall * aCall;

    while ( aCall = nextCall() ) {
	const Call * theCall = dynamic_cast<Call *>(aCall);
	if ( theCall ) {
	    aLabel << theCall->srcName() << ':' << theCall->dstName() << "="
		   << print_wait( *theCall );
	    aLabel.newLine();
	}
    }

    return *this;
}



const Entry& 
Entry::labelQueueingNetworkService( Label& aLabel ) const
{
    aLabel.newLine();
    aLabel << name() << " [";
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	if ( p != 1 ) {
	    aLabel << ',';
	}
	aLabel << serviceTimeForQueueingNetwork(p);
    }
    aLabel << "]";
    return *this;
}


/*
 * Compute the service time for this entry.
 */

double
Entry::serviceTimeForSRVNInput() const
{
    double s = 0.;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	s += phase[p].serviceTimeForSRVNInput();
    }
    return s;
}




double
Entry::serviceTimeForSRVNInput( const unsigned p ) const
{
    return phase[p].serviceTimeForSRVNInput();
}

/*
 * Compute the service time for this entry for the queueing network output 
 * display.  We don't count values to "selected" objects.
 */

double
Entry::serviceTimeForQueueingNetwork() const
{
    double s = 0.;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	s += phase[p].serviceTimeForQueueingNetwork();
    }
    return s;
}



double
Entry::serviceTimeForQueueingNetwork(const unsigned p) const
{
    return phase[p].serviceTimeForQueueingNetwork();
}


#if defined(REP2FLAT)
Entry *
Entry::find_replica( const string& entry_name, const unsigned replica ) throw( runtime_error )
{
    ostringstream aName;
    aName << entry_name << "_" << replica;
    set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( aName.str() ) );
    if ( nextEntry == entry.end() ) {
	string msg = "Entry::find_replica: cannot find symbol ";
	msg += aName.str();
	throw runtime_error( msg );
    }
    return *nextEntry;
}



Entry *
Entry::expandEntry( int replica ) const
{
    ostringstream aName;
    aName << name() << "_" << replica;
    set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( aName.str().c_str() ) );
    if ( nextEntry != entry.end() ) {
	string msg = "Entry::expandEntry(): cannot add symbol ";
	msg += aName.str();
	throw runtime_error( msg );
    } 

    Entry * anEntry = new Entry( *this );
    anEntry->setName( aName.str() );
    ::entry.insert(anEntry);
    const_cast<LQIO::DOM::Document *>(getDOM()->getDocument())->addEntry( const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(anEntry->getDOM())) );

    return anEntry;
}


const Entry&
Entry::expandCalls() const
{
    io_vars.anError = false;
    const unsigned int num_replicas = owner()->replicas();

    Sequence<Call *>nextCall(callList());
    Call *aCall;
    while ( aCall = nextCall() ) {
	int next_dst_id = 1;
	for ( unsigned src_replica = 1; src_replica <= num_replicas; src_replica++) {

	    if ( aCall->fanOut() > aCall->dstEntry()->owner()->replicas() ) {
		ostringstream msg;
		msg << "Entry::expandCalls(): fanout of entry " << name() 
		    << " is greater than the number of replicas of the destination Entry'" << aCall->dstEntry()->name() << "'";
		throw runtime_error( msg.str() );
	    }

	    ostringstream srcName;
	    Entry *srcEntry = find_replica( name(), src_replica );
	    LQIO::DOM::Entry * src_dom = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(srcEntry->getDOM()));

	    for ( unsigned int k = 1; k <= aCall->fanOut(); k++) {
		// divide the destination entries equally between calling
		// entries.  
		const int dst_replica = (next_dst_id++ - 1) % (aCall->dstEntry()->owner()->replicas()) + 1;
		Entry *dstEntry = find_replica(aCall->dstEntry()->name(), dst_replica);
		LQIO::DOM::Entry * dst_dom = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(dstEntry->getDOM()));

		LQIO::DOM::Call * dom_call;
		for (unsigned int p = 1; p <= MAX_PHASES; p++) {
		    LQIO::DOM::Phase * dom_phase = const_cast<LQIO::DOM::Phase *>(srcEntry->getPhaseDOM(p));
		    if ( aCall->hasRendezvousForPhase(p) ) {
			dom_call = aCall->getDOM(p)->clone();
			dom_call->setDestinationEntry( dst_dom );
			srcEntry->rendezvous(dstEntry, p, dom_call );
			dom_phase->addCall(dom_call);
		    } else if ( aCall->hasSendNoReplyForPhase(p) ) {
			dom_call = aCall->getDOM(p)->clone();
			dom_call->setDestinationEntry( dst_dom );
			srcEntry->sendNoReply(dstEntry, p, dom_call );
			dom_phase->addCall(dom_call);
		    }

		}
		if ( srcEntry->hasForwarding() ) {
		    dom_call = aCall->getDOMFwd()->clone();
		    dom_call->setDestinationEntry( dst_dom );
		    srcEntry->forward( dstEntry, dom_call );
                    src_dom->addForwardingCall(dom_call);
		}
	    }
	}
    }

    return *this;
}
#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                  */
/* ------------------------------------------------------------------------ */

/*
 * Draw the entry.
 */

ostream&
Entry::draw( ostream& output ) const
{
    ostringstream aComment;
    aComment << "Entry " << name();
    if ( myActivity ) {
	aComment << " A " << myActivity->name();
    } else {
	aComment << " s [" << print_service_time( *this ) << "]";
    };
#if defined(BUG_375)
    aComment << " span=" << span() << ", index=" << index();
#endif
    myNode->comment( output, aComment.str() );

    const double dx = adjustForSlope( fabs(height()) );
    Point points[4];
    points[0] = myNode->topLeft().moveBy( dx, 0 );
    points[1] = myNode->bottomLeft();
    points[2] = myNode->bottomRight().moveBy( -dx, 0 );
    points[3] = myNode->topRight();

    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    if ( drawLeft && drawRight ) {
	myNode->polyline( output, 4, points );
    } else if ( drawLeft ) {
	myNode->polyline( output, 3, &points[0] );
    } else if ( drawRight ) {
	myNode->polyline( output, 3, &points[1] );
    } else {
	myNode->polyline( output, 2, &points[1] );
    }

    myLabel->backgroundColour( colour() ).comment( output, aComment.str() );
    output << *myLabel;

    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    Call * lastCall = 0;
    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() ) {
	    if ( aCall->hasForwarding() ) {

		/* 
		 * Remove duplicate sections in arc.  Do here because
		 * all arcs have been moved by now.  This will likely
		 * have to get smarter over time.
		 */

		if ( lastCall ) {
		    if ( aCall->pointAt(1) == lastCall->pointAt(1)
			 && aCall->pointAt(2) == lastCall->pointAt(2) ) {
			aCall->pointAt(1) = aCall->pointAt(2) = lastCall->pointAt(3);
		    }
		} else {
		    lastCall = aCall;
		}
	    }
	    output << *aCall;
	}
    }

    if ( myActivityCall ) {
	myActivityCall->penColour( colour() );
	output << *myActivityCall;
    }

    /* Draw reply arcs here for PostScript layering */

    Sequence<Reply *> nextReply( myActivityCallers );
    Reply * aReply;
    while ( aReply = nextReply() ) {
	output << *aReply;
    }

    return output;
}

/*
 * translate the from and to entry names to indecies.  Return 1 if all went well.
 */

bool
map_entry_names( const char * from_entry_name, Entry * & fromEntry, 
		 const char * to_entry_name, Entry * & toEntry,
		 err_func_t err_func )
{
    bool rc    = true;
    fromEntry = Entry::find( from_entry_name );
    toEntry   = Entry::find( to_entry_name );

    if ( !fromEntry ) {
	(*err_func)( LQIO::ERR_NOT_DEFINED, from_entry_name );
	rc = false;
    }
    if ( !toEntry ) {
	(*err_func)( LQIO::ERR_NOT_DEFINED, to_entry_name );
	rc = false;
    }
    if ( fromEntry == toEntry && fromEntry ) {
	(*err_func)( LQIO::ERR_SRC_EQUALS_DST, to_entry_name, from_entry_name );
	rc = false;
    }
    return rc;
}



/*
 * Compare entries (for sorting).  Sort forwarded entries first.
 */

int
Entry::compare( const void * n1, const void *n2 )
{
    Entry * e1 = *static_cast<Entry **>(const_cast<void *>(n1));
    Entry * e2 = *static_cast<Entry **>(const_cast<void *>(n2));

    if ( (e1->isForwardingTarget() && !e2->isForwardingTarget())
	|| ( !e1->hasForwardingLevel() && e2->hasForwardingLevel() ) ) {
	return -1;
    } else if ( (!e1->isForwardingTarget() && e2->isForwardingTarget())
	|| ( e1->hasForwardingLevel() && !e2->hasForwardingLevel() ) ) {
	return 1;
    } else {
	return Element::compare( e1, e2 );
    }
}

/*----------------------------------------------------------------------*/
/*		 	   Called from yyparse.  			*/
/*----------------------------------------------------------------------*/

Entry *
Entry::find( const string& name )
{
    set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( name ) );
    if ( nextEntry != entry.end() ) {
	return *nextEntry;
    } else {
	return 0;
    }
}


Entry * 
find_entry( const char * entry_name )
{
    set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( entry_name ) );
    if ( nextEntry != entry.end() ) {
	return *nextEntry;
    } else {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	return 0;
    }
}



/*
 *  link an entry to entry list.
 */

Cltn<Entry *> *
Entry::link( Cltn<Entry *> * aCltn )
{
    if ( !aCltn ) {
	aCltn = new Cltn<Entry *>(0);
    }
    assert( aCltn != 0 );

    *aCltn << this;				// append entry to list.

    return aCltn;
}



/*
 *  Add an entry.
 */

Entry *
Entry::create( LQIO::DOM::Entry* domEntry )
{
    const string& entry_name = domEntry->getName();
    if ( Entry::find( entry_name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", entry_name.c_str() );
	return 0;
    } else {
	Entry * anEntry = new Entry( domEntry );
	assert( anEntry != 0 );
	entry.insert( anEntry );
	return anEntry;
    }
}



/*
 * Set the task of all of the entries in entryList to `aTask'.
 */

void
setEntryOwner( const Cltn<Entry *> & entryList, const Task * aTask )
{
    Sequence<Entry *> nextEntry(entryList);
    Entry * anEntry;

    for ( unsigned i = 1; anEntry = nextEntry(); ++i ) {
	anEntry->owner(aTask);			// Tag with owner.
    }
}

/*----------------------------------------------------------------------*/
/*		 	    Output formatting.   			*/
/*----------------------------------------------------------------------*/

static ostream&
format( ostream& output, int p ) {
    switch ( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_SRVN: 
	if ( p != 1 ) {
	    output << ' '; 
	}
	break;

    case FORMAT_OUTPUT: 
    case FORMAT_PARSEABLE:
    case FORMAT_RTF:
	output << setw( maxDblLen );
	break;

    default: 
	if ( p != 1 ) {
	    output << ','; 
	}
	break;
    }
    return output;
}


static Label&
execution_time_of_label( Label& aLabel, const Entry& anEntry )
{
    for ( unsigned p = 1; p <= anEntry.maxPhase(); ++p ) {
	if ( p > 1 ) {
	    aLabel << ",";
	}
	aLabel << anEntry.executionTime(p);
    }
    return aLabel;
}


static ostream&
queueing_time_of_str( ostream& output, const Entry& anEntry )
{
    for ( unsigned p = 1; p <= anEntry.maxPhase(); ++p ) {
	format( output, p ) << anEntry.queueingTime(p);
    }
    return output;
}


static ostream&
service_time_of_str( ostream& output, const Entry& anEntry )
{
    for ( unsigned p = 1; p <= anEntry.maxPhase(); ++p ) {
	format( output, p );
	if ( anEntry.hasServiceTime(p) ) {
	    output << instantiate( anEntry.serviceTime(p) );
	} else {
	    output << 0.0;
	}
    }
    return output;
}


static ostream&
think_time_of_str( ostream& output, const Entry& anEntry )
{
    for ( unsigned p = 1; p <= Entry::max_phases; ++p ) {
	format( output, p );
	if ( anEntry.hasThinkTime(p) ) {
	    output << instantiate( anEntry.thinkTime(p) );
	} else {
	    output << 0.0;
	}
    }
    return output;
}

static ostream&
variance_of_str( ostream& output, const Entry& anEntry )
{
    for ( unsigned p = 1; p <= anEntry.maxPhase(); ++p ) {
	format( output, p ) << anEntry.variance(p);
    }
    return output;
}


static ostream&
slice_time_of_str( ostream& output, const Entry& anEntry )
{
    for ( unsigned p = 1; p <= anEntry.maxPhase(); ++p ) {
	format( output, p ) << anEntry.sliceTime(p);
    }
    return output;
}

static ostream&
number_slices_of_str( ostream& output, const Entry& anEntry )
{
    for ( unsigned p = 1; p <= anEntry.maxPhase(); ++p ) {
	format( output, p ) << anEntry.numberSlices(p);
    }
    return output;
}

SRVNEntryManip
print_service_time( const Entry& anEntry )
{
    return SRVNEntryManip( &service_time_of_str, anEntry );
}

SRVNEntryManip
print_think_time( const Entry& anEntry )
{
    return SRVNEntryManip( &think_time_of_str, anEntry );
}

static LabelEntryManip
label_execution_time( const Entry& anEntry )
{
    return LabelEntryManip( &execution_time_of_label, anEntry );
}

SRVNEntryManip
print_number_slices( const Entry& anEntry )
{
    return SRVNEntryManip( &number_slices_of_str, anEntry );
}

SRVNEntryManip
print_slice_time( const Entry& anEntry )
{
    return SRVNEntryManip( &slice_time_of_str, anEntry );
}

SRVNEntryManip
print_queueing_time( const Entry& anEntry )
{
    return SRVNEntryManip( &queueing_time_of_str, anEntry );
}

SRVNEntryManip
print_variance( const Entry& anEntry )
{
    return SRVNEntryManip( &variance_of_str, anEntry );
}

