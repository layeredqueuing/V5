/* activity.cc	-- Greg Franks Thu Apr  3 2003
 *
 * $Id$
 */


#include "activity.h"
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <limits.h>
#if HAVE_VALUES_H
#include <values.h>
#endif
#if HAVE_FLOAT_H
#include <float.h>
#endif
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_call.h>
#include <lqio/dom_extvar.h>
#include "model.h"
#include "actlist.h"
#include "errmsg.h"
#include "cltn.h"
#include "stack.h"
#include "entry.h"
#include "label.h"
#include "call.h"
#include "task.h"
#include "processor.h"
#include "share.h"
#include "arc.h"

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

bool Activity::hasJoins = 0;
std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> Activity::actConnections;
std::map<LQIO::DOM::ActivityList*, ActivityList *> Activity::domToNative;

/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Print out service time of entry in standard output format.
 */

ostream&
operator<<( ostream& output, const Activity& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_SRVN:
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

/*----------------------------------------------------------------------*/
/*                    Activities are like phases....                    */
/*----------------------------------------------------------------------*/

/*
 * Construct and activity.  
 */

Activity::Activity( const Task * aTask, const LQIO::DOM::DocumentObject * dom )
    : Element( dom, aTask->nActivities()+1 ), Phase(),
      myTask(aTask),
      inputFromList(0),
      outputToList(0),
      myReplyList(0),
      myRootEntry(0),
      myCaller(0),
      iAmSpecified(false),
      myLevel(0),
      myIndex(UINT_MAX),
      reachableFrom(0)
{
    myNode = Node::newNode( Flags::entry_width - Flags::act_x_spacing / 2., Flags::entry_height );
    myLabel = Label::newLabel();
}


Activity&
Activity::merge( const Activity &src, const double rate )
{
    /* Aggregate the calls made by the activity to the entry */

    Sequence<Call *> nextCall( src.callList() );
    Call * srcCall;
    while ( srcCall = nextCall() ) {
	Entry * dstEntry = const_cast<Entry *>(srcCall->dstEntry());

	Call * dstCall;
	if ( srcCall->isPseudoCall() ) {
	    dstCall = findOrAddFwdCall( dstEntry );
	} else {
	    dstCall = findOrAddCall( dstEntry );
	}
	dstCall->merge( *srcCall, rate );

	/* Set phase type to stochastic iff we have a non-integral number of calls. */

	if ( phaseTypeFlag() == PHASE_DETERMINISTIC 
	     && ( remainder( dstCall->sumOfRendezvous(), 1.0 ) > EPSILON
		  || remainder( dstCall->sumOfSendNoReply(), 1.0 ) > EPSILON ) ) {
	    phaseTypeFlag( PHASE_STOCHASTIC );
	}

	dstEntry->removeDstCall( srcCall );	/* Unlink the activities calls. */
    }

    /* Aggregate the reply lists  */

    if ( src.myReplyList && src.myReplyList->size() != 0 ) {
	appendReplyList( src );
    }

    /* Aggregate the service time. */

    LQIO::DOM::ExternalVariable& time = *const_cast<LQIO::DOM::ExternalVariable *>(getDOM()->getServiceTime());
    time += src.serviceTime() * rate;

    return *this;
}



/*
 * Free resources.
 */

Activity::~Activity()
{
    replyList( 0 );

    inputFromList = 0;
    outputToList = 0;

    myCalls.deleteContents();

    delete myNode;
    delete myLabel;
}



/* static */ void
Activity::reset()
{
    hasJoins = 0;
    actConnections.clear();
    domToNative.clear();
}

/* ------------------------ Instance Methods -------------------------- */

/*
 * 
 */

const LQIO::DOM::Phase * 
Activity::getDOM() const
{ 
    return dynamic_cast<const LQIO::DOM::Phase *>(Element::getDOM()); 
}



Activity&
Activity::resetLevel() 
{
    myLevel = 0;
    return *this;
}


/*
 * Can we call this activity?
 */

void
Activity::check() const
{
    if ( !reachable() ) {
	LQIO::solution_error( LQIO::WRN_NOT_USED, "Activity", name().c_str() );
    } else if ( !hasServiceTime() ) {
	LQIO::solution_error( LQIO::WRN_NO_SERVICE_TIME, name().c_str() );
    }

    /* Terminate lists (for lqn2lqn) */

    if ( myReplyList && myReplyList->size() > 0 && !outputTo() ) {
	ActivityList * activity_list = const_cast<Activity *>(this)->outputTo( new JoinActivityList( const_cast<Task *>(owner()), 0 ) );
	activity_list->add( const_cast<Activity *>(this) );
    }

    Phase::check();
}



const LQIO::DOM::ExternalVariable &
Activity::rendezvous ( const Entry * toEntry )  const
{
    Call * aCall = dynamic_cast<Call *>(findCall( toEntry ));
    if ( aCall ) {
	return aCall->rendezvous();
    } else {
	abort();
    }
}


Activity& 
Activity::rendezvous (Entry * toEntry, const LQIO::DOM::Call * value ) 
{
    if ( value && toEntry->isCalled( RENDEZVOUS_REQUEST ) ) {
	Model::rendezvousCount[0] += 1;

	Call * aCall = findOrAddCall( toEntry );
	aCall->rendezvous( 1, value );
    }
    return *this;
}


const LQIO::DOM::ExternalVariable &
Activity::sendNoReply ( const Entry * toEntry ) const
{
    Call * aCall = dynamic_cast<Call *>(findCall( toEntry ));
    if ( aCall ) {
	return aCall->sendNoReply();
    } else {
	abort();
    }
}


Activity& 
Activity::sendNoReply (Entry * toEntry, const LQIO::DOM::Call * value )
{ 
    if ( value && toEntry->isCalled( SEND_NO_REPLY_REQUEST ) ) {
	Model::sendNoReplyCount[0] += 1;

	Call * aCall = findOrAddCall( toEntry );
	aCall->sendNoReply( 1, value );
    }
    return *this; 
}


/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Call *
Activity::forwardingRendezvous( Entry * dstEntry, const double rate )
{
    ProxyActivityCall * aCall = findOrAddFwdCall( dstEntry );
    LQIO::DOM::Call * dom = new LQIO::DOM::Call( getDOM()->getDocument(),
						 LQIO::DOM::Call::RENDEZVOUS, 
						 const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(getDOM())),
						 const_cast<LQIO::DOM::Entry*>(dynamic_cast<const LQIO::DOM::Entry*>(dstEntry->getDOM())), 0,
						 new LQIO::DOM::ConstantExternalVariable( rendezvous( aCall->dstEntry() ) ) );
    aCall->rendezvous( 1, dom );
    return aCall;
}



/*
 * Fork list (RValue)
 */

ActivityList *
Activity::inputFrom( ActivityList * aList )
{
    if ( inputFromList ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, name().c_str() );
    } else if ( isStartActivity() ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name().c_str() );
    } else {
	inputFromList = aList;
    } 
    return aList;
}



/*
 * Join list (LValue)
 */

ActivityList *
Activity::outputTo( ActivityList * aList )
{
    if ( outputToList ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, name().c_str() );
    } else {
	outputToList = aList;
    }
    return aList;
}

/* -------------------------- Result Queries -------------------------- */

double
Activity::processorUtilization() const
{
    return dynamic_cast<const LQIO::DOM::Activity *>(getDOM())->getResultProcessorUtilization();
}

double
Activity::throughput() const
{
    return dynamic_cast<const LQIO::DOM::Activity *>(getDOM())->getResultThroughput();
}

/*
 * Add a reply list to this activity.
 */

Activity&
Activity::replyList( Cltn<const Entry *> * aList )
{
    if ( myReplyList ) {
	/* Delete reply arcs */
	myReplyArcList.deleteContents();
	myReplyList->clearContents();
	delete myReplyList;
    }
    if ( aList && aList->size() && owner()->isReferenceTask() ) {
	LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_REPLIES, owner()->name().c_str(), (*aList)[1]->name().c_str(), name().c_str() );
    } else {
	myReplyList = aList;
	if ( aList ) {
	    for ( unsigned int i = 1; i <= myReplyList->size(); ++i ) {
		Entry * anEntry = const_cast<Entry *>((*myReplyList)[i]);
		Reply * aReply = new Reply( this, anEntry );
		if ( aReply ) {
		    myReplyArcList << aReply;
		}
	    }
	} 
    }
    return *this;
}



/*
 * We always have to rebuilt the replyArcList for all replies added from src.
 */

Activity&
Activity::appendReplyList( const Activity& src ) 
{
    if ( !myReplyList ) {
	myReplyList = src.myReplyList;
    } else {
	*myReplyList += *(src.myReplyList);
	delete src.myReplyList;
    }
    myReplyArcList.deleteContents();

    for ( unsigned i = 1; i <= myReplyList->size(); ++i ) {
	Entry * anEntry = const_cast<Entry *>((*myReplyList)[i]);
	Reply * aReply = new Reply( this, anEntry );
	if ( aReply ) {
	    myReplyArcList << aReply;
	}
    }

    return *this;
}




Activity&
Activity::rootEntry( const Entry * anEntry, const Arc * aCall )
{
    myRootEntry = anEntry;
    myCaller = aCall;
    return *this;
}



unsigned
Activity::countArcs( const callFunc aFunc ) const
{
    unsigned count = 0;

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (!aFunc || (aCall->*aFunc)()) ) {
	    count += 1;
	}
    }
    return count;
}




/*
 * Chase calls looking for cycles and the depth in the call tree.  
 * The return value reflects the deepest depth in the call tree.
 */

unsigned
Activity::findChildren( CallStack& callStack, const unsigned directPath, Stack<const Activity *>& activityStack ) const
{
    /* Check for cyles. */

    if ( activityStack.find( this ) ) {
	throw activity_cycle( this, activityStack );
    }

    activityStack.push( this );
    reachableFrom = activityStack.bottom();

    Sequence<Call *> nextCall( callList() );
    unsigned max_depth = max( followCalls( owner(), nextCall, callStack, directPath ), callStack.size() );

    if ( outputToList ) {
	max_depth = max( outputToList->findChildren( callStack, directPath, activityStack ), max_depth );
    }

    activityStack.pop();
    return max_depth;
}


/*
 * Chase calls.  the return reflects the depth of the activity tree
 * and we aggregate serviced times.  Activity aggregation occurs here.
 */

unsigned
Activity::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * srcEntry, const unsigned depth, unsigned p, const double rate ) const
{
    /* Check for cyles. */
    if ( activityStack.find( this ) ) {
	throw activity_cycle( this, activityStack );
    }
    activityStack.push( this );

    unsigned max_depth = max( depth+1, level() );
    const_cast<Activity *>(this)->level( max_depth );

    if ( repliesTo( srcEntry ) ) {
	if ( p == 2 ) {
	    LQIO::solution_error( LQIO::ERR_DUPLICATE_REPLY, owner()->name().c_str(), name().c_str(), srcEntry->name().c_str() );
	}
	if (  srcEntry->isCalled() == SEND_NO_REPLY_REQUEST || srcEntry->isCalled() == OPEN_ARRIVAL_REQUEST ) {
	    LQIO::solution_error( LQIO::ERR_REPLY_SPECIFIED_FOR_SNR_ENTRY, owner()->name().c_str(), name().c_str(), srcEntry->name().c_str() );
	}
	p = 2;
    }

    if ( outputToList ) {
	max_depth = max( outputToList->findActivityChildren( activityStack, forkStack, srcEntry, max_depth, p, rate ), max_depth );
    } 
    activityStack.pop();

    return max_depth;
}


/*
 * Search backwards up activity list looking for a match on forkStack
 */

unsigned
Activity::backtrack( Stack<const AndForkActivityList *>& forkStack ) const
{
    if ( inputFromList ) {
	return inputFromList->backtrack( forkStack );
    } else {
	return 0;
    }
}



/*
 * Get my index (used for sorting activities). 
 */

double
Activity::getIndex() const
{
    double anIndex;
    if ( myRootEntry ) {
	anIndex = myRootEntry->index();
    } else if ( inputFromList ) {
	anIndex = inputFromList->getIndex();
    } else {
	anIndex = MAXDOUBLE;
    }
    return anIndex;
}


/*
 * Apply aFunc to this activity.  Basically used to call
 * aggregateReplies and aggregateService.  We then follow the graph. 
 */

double
Activity::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, const aggregateFunc aFunc ) const
{
    if ( activityStack.find( this ) ) {
	return 0.0;		// throw CycleErr().
    }
    next_p = curr_p;
    if ( repliesTo( anEntry ) ) {
	next_p = 2;		/* aFunc may clobber the repliesTo list, so check first. */
    }
    double sum = (this->*aFunc)( anEntry, curr_p, rate );
    if ( outputToList ) {
	activityStack.push( this );
	sum += outputToList->aggregate( anEntry, next_p, next_p, rate, activityStack, aFunc );
	activityStack.pop();
    }
    return sum;
}



unsigned 
Activity::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, 
		    const Entity * aServer, const callFunc aFunc  )
{
    if ( activityStack.find( this ) ) {
	return next_k;
    }

    if ( aFunc != &GenericCall::hasSendNoReply && (!aServer || (owner()->processor() == aServer) ) ) { 
	setServerChain( curr_k ).setClientClosedChain( curr_k );		/* Catch case where there are no calls. */
    }

    Phase::setChain( curr_k, aServer, aFunc );

    if ( outputToList ) {
	activityStack.push( this );
	next_k = outputToList->setChain( activityStack, curr_k, next_k, aServer, aFunc );
	activityStack.pop();
    }
    return next_k;
}


Activity& 
Activity::setClientClosedChain( unsigned k )
{
    Element::setClientClosedChain( k );
    const_cast<Task *>(owner())->setClientClosedChain( k );
    return *this;
}


Activity& 
Activity::setClientOpenChain( unsigned k ) 
{
    Element::setClientOpenChain( k );
    return *this;
}


Activity& 
Activity::setServerChain( unsigned k )
{
    const_cast<Task *>(owner())->setServerChain( k );
    Element::setServerChain( k );
    return *this;
}


/*
 * Return the number of replies generated by this activity for this entry.
 */

double
Activity::aggregateReplies( const Entry * anEntry, const unsigned p, const double rate ) const
{
    double sum = 0;
    if ( p > 1 && (hasServiceTime() || hasRendezvous() ) ) {
	const_cast<Entry *>(anEntry)->phaseIsPresent( p, true );
    }
    if ( repliesTo( anEntry ) ) {
	if (  anEntry->isCalled() == SEND_NO_REPLY_REQUEST || anEntry->isCalled() == OPEN_ARRIVAL_REQUEST ) {
	    LQIO::solution_error( LQIO::ERR_REPLY_SPECIFIED_FOR_SNR_ENTRY, owner()->name().c_str(), name().c_str(), anEntry->name().c_str() );
	} else if ( rate <= 0 ) {
	    LQIO::solution_error( LQIO::ERR_INVALID_REPLY, owner()->name().c_str(), name().c_str(), anEntry->name().c_str() );
	} else if ( p > 1 ) {
	    LQIO::solution_error( LQIO::ERR_DUPLICATE_REPLY, owner()->name().c_str(), name().c_str(), anEntry->name().c_str() );
	} else {
	    sum = rate;
	}
    }
    return sum;
}



/*
 * Aggregate all service time to anEntry.  If we are getting rid of
 * all activities, then we get rid of the replies that activities make
 * too.
 */

double
Activity::aggregateService( const Entry * anEntry, const unsigned p, const double rate ) const
{
    double sum = 0.0;
    if ( repliesTo( anEntry )  ) {
	sum = rate;
    }

    switch ( Flags::print[AGGREGATION].value.i ) {
    case AGGREGATE_ENTRIES:
    case AGGREGATE_PHASES:
    case AGGREGATE_ACTIVITIES:
	const_cast<Entry *>(anEntry)->aggregateService( this, p, rate );
	int i = repliesTo( anEntry );
	if ( i > 0 ) {
	    Reply * aReply = myReplyArcList[i];
	    const_cast<Entry *>(anEntry)->deleteActivityReplyArc( aReply );
	    const_cast<Activity *>(this)->myReplyArcList -= aReply;
	    delete aReply;
	    *(const_cast<Activity *>(this)->myReplyList) -= anEntry;
	}
	break;
    }

    return sum;
}





/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Activity::findCall( const Entry * anEntry ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( ( aCall = nextCall() ) && (( aCall->isPseudoCall() ) || ( aCall->dstEntry() != anEntry )) );

    return aCall;
}



/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Activity::findFwdCall( const Entry * anEntry ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( ( aCall = nextCall() ) && (( !aCall->isPseudoCall() ) || ( aCall->dstEntry() != anEntry )) );

    return aCall;
}



/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

Call *
Activity::findOrAddCall( Entry * anEntry )
{
    Call * aCall = findCall( anEntry );

    if ( !aCall ) {
	aCall = new ActivityCall( this, anEntry );
	addSrcCall( aCall );
	anEntry->addDstCall( aCall );
    }

    return aCall;
}


/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

ProxyActivityCall *
Activity::findOrAddFwdCall( Entry * anEntry )
{
    ProxyActivityCall * aCall = dynamic_cast<ProxyActivityCall *>(findCall( anEntry ));

    if ( !aCall ) {
	aCall = new ProxyActivityCall( this, anEntry );
	addSrcCall( aCall );
	anEntry->addDstCall( aCall );
    }

    return aCall;
}



bool 
Activity::hasCallsFor( unsigned p ) const
{
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (aCall->hasRendezvous() || aCall->hasSendNoReply() ) ) return true;
    }
    return false;
}


bool
Activity::hasRendezvous() const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    
    while ( aCall = nextCall() ) {
	if ( aCall->hasRendezvous() ) return true;
    }
    return false;
}

bool
Activity::hasSendNoReply() const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    
    while ( aCall = nextCall() ) {
	if ( aCall->hasSendNoReply() ) return true;
    }
    return false;
}


/* 
 * Return the index to the entry if this activity generates a reply to
 * the named entry.  Note that activities that don't reply don't have
 * reply lists, so calling myReplyList->find() directly is a bad idea.
 */

int
Activity::repliesTo( const Entry * anEntry ) const
{
    if ( myReplyList ) {
	return myReplyList->find( anEntry );
    } else {
	return 0;
    }
}


/*
 * Compute the service time for this entry.
 * We subtract off all time to "selected" entries.
 */

double
Activity::serviceTimeForSRVNInput() const
{
    LQIO::DOM::ExternalVariable& time = *const_cast<LQIO::DOM::ExternalVariable *>(getDOM()->getServiceTime());

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() && aCall->hasRendezvous() ) {
	    time += aCall->sumOfRendezvous() * (aCall->waiting(1) + aCall->dstEntry()->executionTime(1));
	}
    }

    /* Add in processor queueing is it isn't selected */

    if ( !owner()->processor()->isSelected() ) {
	time += queueingTime();		/* queueing time is already multiplied my nRendezvous.  See lqns/parasrvn. */
    }
    
    return LQIO::DOM::to_double(time);
}



/*
 * Return true if any aFunc returns true for any call 
 */

bool 
Activity::hasCalls( const callFunc aFunc ) const
{
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (aCall->*aFunc)() ) return true;
    }
    return false;
}



/*
 * Return true is this entry is selected for printing.
 */

bool
Activity::isSelectedIndirectly() const
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




/*
 * Called by entry to transform a sequence of activities into a standard entry.
 * Returns true iff possible.
 */

bool
Activity::transmorgrify()
{
    if ( rootEntry() ) {
	if ( !outputToList || !outputToList->next() ) {
	    const_cast<Entry *>(rootEntry())->aggregateService( this, 1, 1.0 );
	    const_cast<Task *>(owner())->removeActivity( this );
	    delete this;
	    return true;
	} else if ( outputToList 
		    && repliesTo( rootEntry() )
		    && dynamic_cast<JoinActivityList *>(outputToList)
		    && dynamic_cast<ForkActivityList *>(outputToList->next()) ) {
	    Activity * nextActivity = dynamic_cast<ForkActivityList *>(outputToList->next())->myActivity;	
	    if ( !nextActivity->outputToList || !nextActivity->outputToList->next() ) {
		const_cast<Entry *>(rootEntry())->aggregateService( this, 1, 1.0 ).aggregateService( nextActivity, 2, 1.0 );
		const_cast<Task *>(owner())->removeActivity( nextActivity ).removeActivity( this );
		delete nextActivity;
		delete this;
		return true;
	    }
	}
    }
    return false;
}



/*
 * disconnect nextActivity from this list.
 */

Activity&
Activity::disconnect( Activity* nextActivity ) 
{
    /* Reconnect lists */

    outputToList = nextActivity->outputToList;
    nextActivity->outputToList = 0;
    if ( outputToList ) {
	outputToList->reconnect( nextActivity, this );
    }

    const_cast<Task *>(owner())->removeActivity( nextActivity );

    return *this;
}



Activity const&
Activity::sort() const
{
    myCalls.sort( &Call::compareSrc );
    return *this;
}


double
Activity::height() const
{
    double h = Element::height();
    if ( outputToList ) {
	h += outputToList->height();
    } else {
	h += ActivityList::interActivitySpace / 2.0;
    }
    return h;
}


Activity& 
Activity::moveTo( const double x, const double y )
{ 
    Element::moveTo( x, y );
    myLabel->moveTo( center() );

    sort();

    Sequence<Call *> nextCall( callList() );
    Call * srcCall;
    Cltn<Call *> leftCltn;
    Cltn<Call *> rightCltn;
    
    Point srcPoint = bottomCenter();

    /* Sort left and right */

    while ( srcCall = nextCall() ) {
	if ( srcCall->isSelected() ) {
	    if ( srcCall->pointAt(2).x() < srcPoint.x() ) {
		leftCltn << srcCall;
	    } else {
		rightCltn << srcCall;
	    }
	}
    }

    /* move leftCltn */

    double delta = width() / (static_cast<double>(1+leftCltn.size()) * 2.0 );
    srcPoint = bottomLeft();
    for ( unsigned int i = 1; i <= leftCltn.size(); ++i ) {
	srcPoint.moveBy( delta, 0 );
	leftCltn[i]->moveSrc( srcPoint );
    }

    /* move rightCltn */

    delta = width() / (static_cast<double>(1+rightCltn.size()) * 2.0);
    srcPoint = bottomCenter();
    for ( unsigned int i = 1; i <= rightCltn.size(); ++i ) {
	srcPoint.moveBy( delta, 0 );
	rightCltn[i]->moveSrc( srcPoint );
    }

    if ( myCaller ) {
	const_cast<Arc *>(myCaller)->moveDst( topCenter() );
    }

    if ( outputToList ) {
	/* Always to a JOIN list */
	outputToList->moveSrcTo( bottomCenter(), this );
    }
    if ( inputFromList ) {
	inputFromList->moveDstTo( topCenter(), this );
    }

    Sequence<Reply *> nextReply( myReplyArcList );
    Reply * aReply;
    while ( aReply = nextReply() ) {
	Point srcPoint( topCenter() );
	if ( left() > aReply->dstEntry()->left() ) {
	    srcPoint.moveBy( -width() / 4.0, 0 );
	} else {
	    srcPoint.moveBy( width() / 4.0, 0 );
	}
	aReply->moveSrc( srcPoint );
    }

    return *this;
}



Activity& 
Activity::scaleBy( const double sx, const double sy )
{
    Element::scaleBy( sx, sy );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->scaleBy( sx, sy );
    }

    Sequence<Reply *> nextReply( myReplyArcList );
    Reply * aReply;
    while ( aReply = nextReply() ) {
	aReply->scaleBy( sx, sy );
    }

    return *this;
}



Activity& 
Activity::translateY( const double dy )
{
    Element::translateY( dy );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->translateY( dy );
    }

    Sequence<Reply *> nextReply( myReplyArcList );
    Reply * aReply;
    while ( aReply = nextReply() ) {
	aReply->translateY( dy );
    }

    return *this;
}



Activity& 
Activity::depth( const unsigned depth  )
{
    Element::depth( depth-3 );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->depth( depth-2 );
    }

    Sequence<Reply *> nextReply( myReplyArcList );
    Reply * aReply;
    while ( aReply = nextReply() ) {
	aReply->depth( depth-1 );
    }

    return *this;
}



/*
 * Return the amount I should move by IFF I can be aligned.
 */

double
Activity::align() const
{
    if ( inputFromList ) {
	return inputFromList->align();
    } else {
	return 0.0;
    }
}




Activity&
Activity::label()
{
    myLabel->initialize( name() );
    if ( Flags::print[INPUT_PARAMETERS].value.b && hasServiceTime() ) {
	myLabel->newLine() << '[' << serviceTime()  << ']';
    }
    if ( Flags::have_results ) {
	if ( Flags::print[SERVICE].value.b ) {
	    myLabel->newLine() << begin_math() << executionTime() << end_math();
	}
	if ( Flags::print[VARIANCE].value.b ) {
	    myLabel->newLine() << begin_math( &Label::sigma ) << "=" << variance() << end_math();
	}
    }

    /* Now do calls. */

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->label();
    }

    if ( outputToList ) {
	outputToList->label();
    }
    if ( inputFromList ) {
	inputFromList->label();
    }

    return *this;
}



Graphic::colour_type 
Activity::colour() const
{
    if ( !reachable() ) {
	return Graphic::RED;
    } else switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_RESULTS:
	return owner()->colour();
    default:
	return Graphic::DEFAULT_COLOUR;
    }
}


#if defined(REP2FLAT)
Activity&
Activity::expandActivityCalls( const Activity& src, int replica ) 
{
    Sequence<Call *> nextCall(src.callList());
    const Call *aCall;
    LQIO::DOM::Activity * dom_activity = const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(getDOM()));
    while ( aCall = nextCall() ) {

	const unsigned fan_out = aCall->fanOut();
	const unsigned dst_replicas = aCall->dstEntry()->owner()->replicas();
	if (fan_out > dst_replicas) {
	    ostringstream msg;
	    msg << "Activity::expandActivityCalls(): fanout of activity " << name() 
		<< " is greater than the number of replicas of the destination Entry'" 
		<< aCall->dstEntry()->name() << "'";
	    throw runtime_error( msg.str() );
	}

	LQIO::DOM::Call * dom_call;
	for (unsigned k = 0; k < fan_out; k++) {
	    Entry *dstEntry = Entry::find_replica( aCall->dstEntry()->name(), 
						   ((k + (replica - 1) * fan_out) % dst_replicas) + 1 );

	    LQIO::DOM::Entry * dst_dom = const_cast<LQIO::DOM::Entry*>(dynamic_cast<const LQIO::DOM::Entry*>(dstEntry->getDOM()));
	    if ( aCall->hasRendezvous() ) {
		dom_call = aCall->getDOM(1)->clone();
		dom_call->setDestinationEntry( dst_dom );
		rendezvous( dstEntry, dom_call );
		dom_activity->addCall( dom_call );
		
	    } else if ( aCall->hasSendNoReply() ) {
		dom_call = aCall->getDOM(1)->clone();
		dom_call->setDestinationEntry( dst_dom );
		sendNoReply( dstEntry, dom_call );
		dom_activity->addCall( dom_call );
	    }
	}
    }
    return *this;
}
#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                 */
/* ------------------------------------------------------------------------ */

ostream&  
Activity::draw( ostream & output ) const 
{ 
    ostringstream aComment;
    aComment << "Activity " << name();
    if ( hasServiceTime() ) {
	aComment << " s [" << serviceTime() << "]";
    }
    aComment << " level=" << level();

    myNode->comment( output, aComment.str() );
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    myNode->rectangle( output );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() ) {
	    output << *aCall;
	}
    }

    myLabel->backgroundColour( colour() );
    output << *myLabel;

    if ( outputToList && outputToList->next() ) {
	output << *outputToList;
    }
    if ( inputFromList && inputFromList->prev() ) {
	output << *inputFromList;
    }

    /* Don't draw reply arcs here ... draw from entry (for layering purposes) */

    return output; 
}


/*
 * Print out the name of the activity along with any entries to whom
 * it replies.  IFF we are generating a submodel, the reply list will
 * be suppressed if the task is being converted into one which accepts
 * open arrivals.
 */

ostream&
Activity::printNameWithReply( ostream& output ) const
{
    output << name();
    if ( myReplyList && !owner()->canConvertToOpenArrivals() ) {
	output << '[';
	Sequence<const Entry *> nextEntry( *myReplyList );
	const Entry * anEntry;
	for  ( int i = 0; anEntry = nextEntry(); ++i ) {
	    if ( i != 0 ) output << ',';
	    output << anEntry->name();
	}
	output << ']';
    }
    return output;
}

/*
 * Compare entries (for sorting)
 */

int
Activity::compare( const void * n1, const void *n2 )
{
    return Element::compare( *static_cast<Activity **>(const_cast<void *>(n1)), 
			     *static_cast<Activity **>(const_cast<void *>(n2)) );
}



/*
 * Compare entries (for sorting)
 */

int
Activity::compareCoord( const void * n1, const void *n2 )
{
    const Activity * a1 = *static_cast<Activity **>(const_cast<void *>(n1));
    const Activity * a2 = *static_cast<Activity **>(const_cast<void *>(n2));

    double diff = a1->index() - a2->index();
    if ( diff != 0 ) {
	return static_cast<int>(copysign( 1.0, diff ) );
    } else {
	return 0;
    }
}

/* ------------------------ Exception Handling ------------------------ */

activity_cycle::activity_cycle( const Activity * anActivity, Stack<const Activity *>& activityStack )
    : path_error( activityStack.size() )
{
    myMsg = anActivity->name();
    for ( unsigned i = activityStack.size(); i > 0; --i ) {
	myMsg += ", ";
	myMsg += activityStack[i]->name();
	if ( activityStack[i] == anActivity ) break;
    }
}

/************************************************************************/
/*                     Functions called by loader.                      */
/************************************************************************/

Activity* 
Activity::create( Task* newTask, LQIO::DOM::Activity* activity )
{
    /* Create a new activity assigned to a given task and set the information DOM entry for it */
    Activity * anActivity = newTask->findOrAddActivity( activity );
	
    /* Find out if we can specify the activity */
    if (activity->isSpecified() == true) {
	anActivity->isSpecified(true);
    }

    return anActivity;
}


Activity&
Activity::add_calls()
{
    /* Go over all of the calls specified within this activity and do something similar to store_snr/rnv */
    const LQIO::DOM::Activity* domActivity = dynamic_cast<const LQIO::DOM::Activity *>(getDOM());
    const std::vector<LQIO::DOM::Call*>& callList = domActivity->getCalls();
    std::vector<LQIO::DOM::Call*>::const_iterator iter;
	
    /* This provides us with the DOM Call which we can then take apart */
    for (iter = callList.begin(); iter != callList.end(); ++iter) {
	const LQIO::DOM::Call* domCall = *iter;
	const LQIO::DOM::Entry* toDOMEntry = domCall->getDestinationEntry();
	Entry* destEntry = Entry::find(toDOMEntry->getName());
		
	/* Make sure all is well */
	if (!destEntry) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, toDOMEntry->getName().c_str() );
	} else if (!destEntry->isReferenceTaskEntry()) {
	    isSpecified(true);
	    if (domCall->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY) {
		sendNoReply(destEntry, domCall);
	    } else if (domCall->getCallType() == LQIO::DOM::Call::RENDEZVOUS) {
		rendezvous(destEntry, domCall);
	    }
	}
    }
    return *this;
}

/* ---------------------------------------------------------------------- */

static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;
static std::map<LQIO::DOM::ActivityList*, void*> domToNative;

Activity&
Activity::add_reply_list ()
{
    /* This information is stored in the LQIO DOM itself */
    const LQIO::DOM::Activity* domActivity = dynamic_cast<const LQIO::DOM::Activity *>(getDOM());
    const std::vector<LQIO::DOM::Entry*>& domReplyList = domActivity->getReplyList();
    if (domReplyList.size() == 0 ) {
	return *this;
    }
	
    /* We must begin by building up an entry list */
    Cltn<const Entry*>* entryList = new Cltn<const Entry*>();
	
    /* Walk over the list and do the equivalent of calling act_add_reply n times */
    std::vector<LQIO::DOM::Entry*>::const_iterator iter;
    for (iter = domReplyList.begin(); iter != domReplyList.end(); ++iter) {
	LQIO::DOM::Entry* domEntry = const_cast<LQIO::DOM::Entry*>(*iter);
	const Entry* myEntry = Entry::find(domEntry->getName());
		
	/* Check it out and add it to the list */
	if (myEntry == NULL) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domEntry->getName().c_str() );
	} else if (myEntry->owner() != owner()) {
	    LQIO::input_error2( LQIO::ERR_WRONG_TASK_FOR_ENTRY, domEntry->getName().c_str(), owner()->name().c_str() );
	} else {
	    (*entryList) << myEntry;
	}
    }
	
    /* Store the reply list for the activity */
    replyList(entryList);
    return *this;
}

Activity&
Activity::add_activity_lists()
{  
    /* Obtain the Task and Activity information DOM records */
    const LQIO::DOM::Activity* domAct = dynamic_cast<const LQIO::DOM::Activity *>(getDOM());
    if (domAct == NULL) { return *this; }
    const Task * task = owner();
	
    /* May as well start with the outputToList, this is done with various methods */
    LQIO::DOM::ActivityList* joinList = domAct->getOutputToList();
    ActivityList * localActivityList = NULL;
    if (joinList != NULL && joinList->getProcessed() == false) {
	const std::vector<const LQIO::DOM::Activity*>& list = joinList->getList();
	std::vector<const LQIO::DOM::Activity*>::const_iterator iter;
	joinList->setProcessed(true);
	for (iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;

	    /* Add the activity to the appropriate list based on what kind of list we have */
	    Activity * nextActivity = task->findActivity( domAct->getName().c_str() );
	    if ( !nextActivity ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }

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
	    Activity * nextActivity = task->findActivity( domAct->getName().c_str() );
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
		localActivityList = nextActivity->act_and_fork_list(localActivityList, forkList );
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

/* ---------------------------------------------------------------------- */
/*                           Left side (joins)                            */
/* ---------------------------------------------------------------------- */

/*
 * Add activity to the sequence list.
 */

ActivityList *
Activity::act_join_item( LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * activity_list = outputTo( new JoinActivityList( const_cast<Task *>(owner()), dom_activitylist ) );
    activity_list->add( this );
    return activity_list;
}

/*
 * Add activity to the activity_list.  This list is for AND joining.  Used by task.cc for quorum.
 */

ActivityList *
Activity::act_and_join_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( !activityList ) {
	activityList = new AndJoinActivityList( const_cast<Task *>(owner()), dom_activitylist );
    } else if ( !dynamic_cast<AndJoinActivityList *>(activityList) ) {
	abort();		// Wrong list type -- quoi?
    }
    outputTo( activityList );
    activityList->add( this );

    return activityList;
}



/*
 * Add activity to the activity_list.  This list is for OR joining.
 */

ActivityList *
Activity::act_or_join_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( !activityList ) {
	activityList = new OrJoinActivityList( const_cast<Task *>(owner()), dom_activitylist );
    } else if ( !dynamic_cast<OrJoinActivityList *>(activityList) ) {
	abort();
    }
    outputTo( activityList );
    activityList->add( this );
    return activityList;
}

/* ---------------------------------------------------------------------- */
/*                          Right side (forks)                            */
/* ---------------------------------------------------------------------- */

/*
 * Add activity to the sequence list.  Used by task.cc for quorum.
 */

ActivityList *
Activity::act_fork_item( LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * activityList = inputFrom( new ForkActivityList( const_cast<Task *>(owner()), dom_activitylist ) );
    activityList->add( this );
    return activityList;
}



/*
 * Add activity to the activity_list.  This list is for AND forking.  Used by task.cc for quorum.
 */

ActivityList *
Activity::act_and_fork_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( isStartActivity() ) {
        LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name().c_str() );
	return activityList;
    } else if ( !activityList ) {
	activityList = new AndForkActivityList( const_cast<Task *>(owner()), dom_activitylist );
    } else if ( !dynamic_cast<AndForkActivityList *>(activityList) ) {
	abort();
    } 

    activityList->add( this );
    inputFrom( activityList );

    return activityList;
}



/*
 * Add activity to the activity_list.  This list is for OR forking.
 */

ActivityList *
Activity::act_or_fork_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( isStartActivity() ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name().c_str() );
	return activityList;
    } else if ( !activityList ) {
	activityList = new OrForkActivityList( const_cast<Task *>(owner()), dom_activitylist );
    } else if ( !dynamic_cast<OrForkActivityList *>(activityList) ) {
	abort();
    } 
    
    inputFrom( activityList );
    activityList->add( this );

    return activityList;
}


/*
 * Add activity to the loop list.
 */

ActivityList *
Activity::act_loop_list ( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( !activity_list ) {
	activity_list = new RepeatActivityList( const_cast<Task *>(owner()), dom_activitylist );
    } else if ( !dynamic_cast<RepeatActivityList *>(activity_list ) ) {
	abort();   
    }

    activity_list->add( this );
    inputFrom( activity_list );

    return activity_list;
}


void 
Activity::complete_activity_connections ()
{
    /* We stored all the necessary connections and resolved the list identifiers so finalize */
    std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*>::iterator iter;
    for (iter = Activity::actConnections.begin(); iter != Activity::actConnections.end(); ++iter) {
	ActivityList* src = Activity::domToNative[iter->first];
	ActivityList* dst = Activity::domToNative[iter->second];
	assert(src != NULL && dst != NULL);
	ActivityList::act_connect(src, dst);
    }
}

ostream& 
Activity::print_reply_activity_name( ostream& output, const Activity * anActivity )
{
    if ( anActivity ) {
	anActivity->printNameWithReply( output );
    }
    return output;
}

ActivityManip
reply_activity_name( Activity * anActivity )
{
    return ActivityManip( Activity::print_reply_activity_name, anActivity );
}
