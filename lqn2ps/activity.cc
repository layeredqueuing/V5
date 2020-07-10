/* activity.cc	-- Greg Franks Thu Apr  3 2003
 *
 * $Id: activity.cc 13675 2020-07-10 15:29:36Z greg $
 */

#include "activity.h"
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <vector>
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
#include "entry.h"
#include "label.h"
#include "call.h"
#include "task.h"
#include "processor.h"
#include "share.h"
#include "arc.h"

bool Activity::hasJoins = 0;
std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> Activity::actConnections;
std::map<LQIO::DOM::ActivityList*, ActivityList *> Activity::domToNative;

struct ExecReplyXY
{
    typedef GenericCall& (GenericCall::*funcPtrXY)( double x, double y );
    ExecReplyXY( funcPtrXY f, double x, double y ) : _f(f), _x(x), _y(y) {};
    void operator()( const std::pair<Entry *,Reply *>& object ) const { (object.second->*_f)( _x, _y ); }
private:
    funcPtrXY _f;
    double _x;
    double _y;
};

/*----------------------------------------------------------------------*/
/*                    Activities are like phases....                    */
/*----------------------------------------------------------------------*/

/*
 * Construct and activity.
 */

Activity::Activity( const Task * aTask, const LQIO::DOM::DocumentObject * dom )
    : Element( dom, aTask->nActivities()+1 ), Phase(),
      _task(aTask),
      _inputFrom(NULL),
      _outputTo(NULL),
      _replies(),
      _rootEntry(NULL),
      _caller(NULL),
      iAmSpecified(false),
      _level(0),
      _reachableFrom(NULL)
{
    myNode = Node::newNode( Flags::entry_width - Flags::act_x_spacing / 2., Flags::entry_height );
    myLabel = Label::newLabel();
}


Activity&
Activity::merge( const Activity &src, const double rate )
{
    /* Aggregate the calls made by the activity to the entry */

    for ( std::vector<Call *>::const_iterator src_call = src.calls().begin(); src_call != src.calls().end(); ++src_call ) {
	Entry * dstEntry = const_cast<Entry *>((*src_call)->dstEntry());

	Call * dstCall;
	if ( (*src_call)->isPseudoCall() ) {
	    dstCall = findOrAddFwdCall( dstEntry );
	} else {
	    dstCall = findOrAddCall( dstEntry );
	}
	dstCall->merge( *this, *(*src_call), rate );

	/* Set phase type to stochastic iff we have a non-integral number of calls. */

	if ( phaseTypeFlag() == PHASE_DETERMINISTIC
	     && ( remainder( dstCall->sumOfRendezvous(), 1.0 ) > EPSILON
		  || remainder( dstCall->sumOfSendNoReply(), 1.0 ) > EPSILON ) ) {
	    phaseTypeFlag( PHASE_STOCHASTIC );
	}

	dstEntry->removeDstCall( (*src_call) );	/* Unlink the activities calls. */
    }

    /* Aggregate the reply lists  */

    appendReplyList( src );

    /* Aggregate the service time. */

    const_cast<LQIO::DOM::Phase *>(getDOM())->setServiceTimeValue(to_double(*getDOM()->getServiceTime()) * rate);

    return *this;
}



/*
 * Free resources.
 */

Activity::~Activity()
{
    _inputFrom = NULL;
    _outputTo = NULL;
    for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	delete *call;
    }
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
 * _documentObject is in both Phase and Element, so force to Element.  We don't draw phases.
 */

const LQIO::DOM::Phase * 
Activity::getDOM() const
{ 
    return dynamic_cast<const LQIO::DOM::Phase *>(Element::getDOM()); 
}

Activity&
Activity::resetLevel()
{
    _level = 0;
    return *this;
}


/*
 * Can we call this activity?
 */

bool
Activity::check() const
{
    if ( !reachable() ) {
	LQIO::solution_error( LQIO::WRN_NOT_USED, "Activity", name().c_str() );
    } else if ( !hasServiceTime() ) {
	LQIO::solution_error( LQIO::WRN_NO_SERVICE_TIME, name().c_str() );
    }

    return Phase::check();
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
						 const_cast<LQIO::DOM::Entry*>(dynamic_cast<const LQIO::DOM::Entry*>(dstEntry->getDOM())),
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
    if ( inputFrom() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, name().c_str() );
    } else if ( isStartActivity() ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name().c_str() );
    } else {
	_inputFrom = aList;
    }
    return aList;
}



/*
 * Join list (LValue)
 */

ActivityList *
Activity::outputTo( ActivityList * aList )
{
    if ( outputTo() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, name().c_str() );
    } else {
	_outputTo = aList;
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
Activity::replies( const std::vector<Entry *>& aList )
{
    _replies.clear();
    /* Delete reply arcs */
    for ( std::map<Entry *,Reply *>::const_iterator reply = replyArcs().begin(); reply != replyArcs().end(); ++reply ){
	delete reply->second;
    }
    _replyArcs.clear();
    if ( aList.size() && owner()->isReferenceTask() ) {
	LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_REPLIES, owner()->name().c_str(), aList.at(0)->name().c_str(), name().c_str() );
    } else {
	_replies = aList;
	for ( std::vector<Entry *>::const_iterator entry = replies().begin(); entry != replies().end(); ++entry ) {
	    _replyArcs[*entry] = new Reply( this, *entry );
	}
    }
    return *this;
}



/*
 * We always have to rebuild the replyArcList for all replies added from src.
 */

Activity&
Activity::appendReplyList( const Activity& src )
{
    if ( src.replies().size() == 0 ) return *this;	// Nop

    _replies.insert( _replies.end(), src.replies().begin(), src.replies().end() );

    /* Delete reply arcs */
    for ( std::map<Entry *,Reply *>::const_iterator reply = replyArcs().begin(); reply != replyArcs().end(); ++reply ){
	delete reply->second;
    }
    _replyArcs.clear();
    for ( std::vector<Entry *>::const_iterator entry = replies().begin(); entry != replies().end(); ++entry ) {
	_replyArcs[*entry] = new Reply( this, *entry );
    }

    return *this;
}




Activity&
Activity::rootEntry( const Entry * anEntry, const Arc * aCall )
{
    _rootEntry = anEntry;
    _caller = aCall;
    return *this;
}



unsigned
Activity::countArcs( const callPredicate predicate ) const
{
    return count_if( calls().begin(), calls().end(), GenericCall::PredicateAndSelected( predicate ) );
}




/*
 * Chase calls looking for cycles and the depth in the call tree.
 * The return value reflects the deepest depth in the call tree.
 */

size_t
Activity::findChildren( CallStack& callStack, const unsigned directPath, std::deque<const Activity *>& activityStack ) const
{
    /* Check for cyles. */

    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) {
	throw activity_cycle( this, activityStack );
    }

    activityStack.push_back( this );
    _reachableFrom = activityStack.front();

    std::pair<std::vector<Call *>::const_iterator,std::vector<Call *>::const_iterator> call(calls().begin(),calls().end());
    size_t max_depth = std::max( followCalls( call, callStack, directPath ), callStack.size() );

    if ( outputTo() ) {
	max_depth = std::max( outputTo()->findChildren( callStack, directPath, activityStack ), max_depth );
    }

    activityStack.pop_back();
    return max_depth;
}


/*
 * Chase calls.  the return reflects the depth of the activity tree
 * and we aggregate serviced times.  Activity aggregation occurs here.
 */

size_t
Activity::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * srcEntry, size_t depth, unsigned p, const double rate ) const
{
    /* Check for cyles. */
    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) {
	throw activity_cycle( this, activityStack );
    }
    activityStack.push_back( this );

    size_t max_depth = std::max( depth+1, level() );
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

    if ( outputTo() ) {
	max_depth = std::max( outputTo()->findActivityChildren( activityStack, forkStack, srcEntry, max_depth, p, rate ), max_depth );
    }
    activityStack.pop_back();

    return max_depth;
}


/*
 * Search backwards up activity list looking for a match on forkStack
 */

size_t
Activity::backtrack( std::deque<const AndForkActivityList *>& forkStack ) const
{
    if ( inputFrom() ) {
	return inputFrom()->backtrack( forkStack );
    } else {
	return ~0;
    }
}



/*
 * Get my index (used for sorting activities).
 */

double
Activity::getIndex() const
{
    double anIndex;
    if ( rootEntry() ) {
	anIndex = rootEntry()->index();
    } else if ( inputFrom() ) {
	anIndex = inputFrom()->getIndex();
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
Activity::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, const aggregateFunc aFunc )
{
    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) {
	return 0.0;		// throw CycleErr().
    }
    next_p = curr_p;
    if ( repliesTo( anEntry ) ) {
	next_p = 2;		/* aFunc may clobber the repliesTo list, so check first. */
    }
    double sum = (this->*aFunc)( anEntry, curr_p, rate );
    if ( outputTo() ) {
	activityStack.push_back( this );
	sum += outputTo()->aggregate( anEntry, next_p, next_p, rate, activityStack, aFunc );
	activityStack.pop_back();
    }
    return sum;
}



unsigned
Activity::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k,
		    const Entity * aServer, const callPredicate aFunc  )
{
    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) {
	return next_k;
    }

    if ( aFunc != &GenericCall::hasSendNoReply && (!aServer || (owner()->processor() == aServer) ) ) {
	setServerChain( curr_k ).setClientClosedChain( curr_k );		/* Catch case where there are no calls. */
    }

    Phase::setChain( curr_k, aServer, aFunc );

    if ( outputTo() ) {
	activityStack.push_back( this );
	next_k = outputTo()->setChain( activityStack, curr_k, next_k, aServer, aFunc );
	activityStack.pop_back();
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
Activity::aggregateReplies( Entry * anEntry, const unsigned p, const double rate )
{
    double sum = 0;
    if ( p > 1 && (hasServiceTime() || hasRendezvous() ) ) {
	anEntry->getPhase(p);
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
Activity::aggregateService( Entry * anEntry, const unsigned p, const double rate )
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
	std::map<Entry *,Reply *>::iterator reply = _replyArcs.find(anEntry);
	if ( reply != replyArcs().end() ) {
	    anEntry->deleteActivityReplyArc( reply->second );
	    _replyArcs.erase( reply );
	    delete reply->second;
	    std::vector<Entry *>::iterator entry = find( _replies.begin(), _replies.end(), anEntry );
	    if ( entry != replies().end() ) {
		_replies.erase( entry );
	    }
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
    for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( !(*call)->isPseudoCall() && (*call)->dstEntry() == anEntry ) return *call;
    }
    return 0;
}



/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Activity::findFwdCall( const Entry * anEntry ) const
{
    for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( (*call)->isPseudoCall() && (*call)->dstEntry() == anEntry ) return *call;
    }
    return 0;
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
Activity::hasRendezvous() const
{
    return find_if( calls().begin(), calls().end(), ::Predicate<GenericCall>( &GenericCall::hasRendezvous ) ) != calls().end();
}

bool
Activity::hasSendNoReply() const
{
    return find_if( calls().begin(), calls().end(), ::Predicate<GenericCall>( &GenericCall::hasSendNoReply ) ) != calls().end();
}


/*
 * Return true if this activity generates a reply to
 * the named entry.
 */

bool
Activity::repliesTo( const Entry * anEntry ) const
{
    return find( replies().begin(), replies().end(), anEntry ) != replies().end();
}


/*
 * Compute the service time for this entry.
 * We subtract off all time to "selected" entries.
 */

double
Activity::serviceTimeForSRVNInput() const
{
    double time = to_double(*getDOM()->getServiceTime());
    for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( !(*call)->isSelected() && (*call)->hasRendezvous() ) {
	    time += (*call)->sumOfRendezvous() * ((*call)->waiting(1) + (*call)->dstEntry()->executionTime(1));
	}
    }

    /* Add in processor queueing is it isn't selected */

    if ( !owner()->processor()->isSelected() ) {
	time += queueingTime();		/* queueing time is already multiplied my nRendezvous.  See lqns/parasrvn. */
    }

    const_cast<LQIO::DOM::Phase *>(getDOM())->setServiceTimeValue(time);
    return time;
}



/*
 * Return true if any aFunc returns true for any call
 */

bool
Activity::hasCalls( const callPredicate predicate ) const
{
    return find_if( calls().begin(), calls().end(), GenericCall::PredicateAndSelected( predicate ) ) != calls().end();
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
    return find_if( calls().begin(), calls().end(), ::Predicate<GenericCall>( &GenericCall::isSelected ) ) != calls().end();
}




/*
 * Called by entry to transform a sequence of activities into a standard entry.
 * Returns true iff possible.
 */

bool
Activity::transmorgrify()
{
    if ( rootEntry() ) {
	if ( !outputTo() || !outputTo()->next() ) {
	    const_cast<Entry *>(rootEntry())->aggregateService( this, 1, 1.0 );
	    const_cast<Task *>(owner())->removeActivity( this );
	    delete this;
	    return true;
	} else if ( outputTo()
		    && repliesTo( rootEntry() )
		    && dynamic_cast<JoinActivityList *>(outputTo())
		    && dynamic_cast<ForkActivityList *>(outputTo()->next()) ) {
	    Activity * nextActivity = dynamic_cast<ForkActivityList *>(outputTo()->next())->myActivity;
	    if ( !nextActivity->outputTo() || !nextActivity->outputTo()->next() ) {
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

    outputTo( nextActivity->outputTo() );
    nextActivity->outputTo( NULL );
    if ( outputTo() ) {
	outputTo()->reconnect( nextActivity, this );
    }

    const_cast<Task *>(owner())->removeActivity( nextActivity );

    return *this;
}



Activity&
Activity::sort()
{
    ::sort( _calls.begin(), _calls.end(), &Call::compareSrc );
    return *this;
}


double
Activity::height() const
{
    double h = Element::height();
    if ( outputTo() ) {
	h += outputTo()->height();
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

    std::vector<Call *> left_side;
    std::vector<Call *> right_side;

    Point srcPoint = bottomCenter();

    /* Sort left and right */

    for ( std::vector<Call *>::const_iterator src_call = calls().begin(); src_call != calls().end(); ++src_call ) {
	if ( (*src_call)->isSelected() ) {
	    if ( (*src_call)->pointAt(1).x() < srcPoint.x() ) {
		left_side.push_back(*src_call);
	    } else {
		right_side.push_back(*src_call);
	    }
	}
    }

    /* move leftCltn */

    double delta = width() / (static_cast<double>(1+left_side.size()) * 2.0 );
    srcPoint = bottomLeft();
    for ( std::vector<Call *>::const_iterator call = left_side.begin(); call != left_side.end(); ++call ) {
	srcPoint.moveBy( delta, 0 );
	(*call)->moveSrc( srcPoint );
    }

    /* move rightCltn */

    delta = width() / (static_cast<double>(1+right_side.size()) * 2.0);
    srcPoint = bottomCenter();
    for ( std::vector<Call *>::const_iterator call = right_side.begin(); call != right_side.end(); ++call ) {
	srcPoint.moveBy( delta, 0 );
	(*call)->moveSrc( srcPoint );
    }

    if ( _caller ) {
	const_cast<Arc *>(_caller)->moveDst( topCenter() );
    }

    if ( outputTo() ) {
	/* Always to a JOIN list */
	outputTo()->moveSrcTo( bottomCenter(), this );
    }
    if ( inputFrom() ) {
	inputFrom()->moveDstTo( topCenter(), this );
    }

    for ( std::map<Entry *,Reply *>::const_iterator reply = replyArcs().begin(); reply != replyArcs().end(); ++reply ) {
	Point srcPoint( topCenter() );
	if ( left() > reply->second->dstEntry()->left() ) {
	    srcPoint.moveBy( -width() / 4.0, 0 );
	} else {
	    srcPoint.moveBy( width() / 4.0, 0 );
	}
	reply->second->moveSrc( srcPoint );
    }

    return *this;
}



Activity&
Activity::scaleBy( const double sx, const double sy )
{
    Element::scaleBy( sx, sy );
    for_each( calls().begin(), calls().end(), ExecXY<GenericCall>( &GenericCall::scaleBy, sx, sy ) );
    for_each( replyArcs().begin(), replyArcs().end(), ExecReplyXY( &GenericCall::scaleBy, sx, sy ) );
    return *this;
}



Activity&
Activity::translateY( const double dy )
{
    Element::translateY( dy );
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,double>( &GenericCall::translateY, dy ) );
    for_each( replyArcs().begin(), replyArcs().end(), ExecX<GenericCall,std::pair<Entry *,Reply *>,double>( &GenericCall::translateY, dy ) );
    return *this;
}



Activity&
Activity::depth( const unsigned depth  )
{
    Element::depth( depth-3 );
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,unsigned int>( &GenericCall::depth, depth-2 ) );
    for_each( replyArcs().begin(), replyArcs().end(), ExecX<GenericCall,std::pair<Entry *,Reply *>,unsigned>( &GenericCall::depth, depth-1 ) );
    return *this;
}



/*
 * Return the amount I should move by IFF I can be aligned.
 */

double
Activity::align() const
{
    if ( inputFrom() ) {
	return inputFrom()->align();
    } else {
	return 0.0;
    }
}




Activity&
Activity::label()
{
    *myLabel << name();
    if ( Flags::print[INPUT_PARAMETERS].value.b && hasServiceTime() ) {
	myLabel->newLine() << '[' << serviceTime()  << ']';
    }
    if ( Flags::have_results ) {
	if ( Flags::print[SERVICE].value.b ) {
	    myLabel->newLine() << begin_math() << opt_pct(executionTime()) << end_math();
	}
	if ( Flags::print[VARIANCE].value.b ) {
	    myLabel->newLine() << begin_math( &Label::sigma ) << "=" << opt_pct(variance()) << end_math();
	}
    }

    /* Now do calls. */

    for_each ( calls().begin(), calls().end(), Exec<GenericCall>( &GenericCall::label ) );

    return *this;
}



Graphic::colour_type
Activity::colour() const
{
    if ( !reachable() ) {
	return Graphic::RED;
    } else switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_RESULTS:
    case COLOUR_DIFFERENCES:
	return owner()->colour();
    default:
	return Graphic::DEFAULT_COLOUR;
    }
}


Activity&
Activity::rename()
{
    std::ostringstream name;
    name << "a" << elementId();
    const_cast<LQIO::DOM::Phase *>(getDOM())->setName( name.str() );
    return *this;
}


#if defined(REP2FLAT)
Activity&
Activity::expandActivityCalls( const Activity& src, int replica )
{
    LQIO::DOM::Activity * dom_activity = const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(getDOM()));
    for ( std::vector<Call *>::const_iterator call = src.calls().begin(); call != src.calls().end(); ++call ) {
	const unsigned fan_out = (*call)->fanOut();
	const unsigned dst_replicas = (*call)->dstEntry()->owner()->replicasValue();
	if (fan_out > dst_replicas) {
	    ostringstream msg;
	    msg << "Activity::expandActivityCalls(): fanout of activity " << name()
		<< " is greater than the number of replicas of the destination Entry'"
		<< (*call)->dstEntry()->name() << "'";
	    throw runtime_error( msg.str() );
	}

	LQIO::DOM::Call * dom_call;
	for (unsigned k = 0; k < fan_out; k++) {
	    Entry *dstEntry = Entry::find_replica( (*call)->dstEntry()->name(),
						   ((k + (replica - 1) * fan_out) % dst_replicas) + 1 );

	    LQIO::DOM::Entry * dst_dom = const_cast<LQIO::DOM::Entry*>(dynamic_cast<const LQIO::DOM::Entry*>(dstEntry->getDOM()));
	    if ( (*call)->hasRendezvous() ) {
		dom_call = (*call)->getDOM(1)->clone();
		dom_call->setSourceObject( const_cast<LQIO::DOM::Phase *>(getDOM()) );
		dom_call->setDestinationEntry( dst_dom );
		rendezvous( dstEntry, dom_call );
		dom_activity->addCall( dom_call );

	    } else if ( (*call)->hasSendNoReply() ) {
		dom_call = (*call)->getDOM(1)->clone();
		dom_call->setSourceObject( const_cast<LQIO::DOM::Phase *>(getDOM()) );
		dom_call->setDestinationEntry( dst_dom );
		sendNoReply( dstEntry, dom_call );
		dom_activity->addCall( dom_call );
	    }
	}
    }
    return *this;
}



static struct {
    set_function first;
    get_function second;
} activity_mean[] = { 
// static std::pair<set_function,get_function> activity_mean[] = {
    { &LQIO::DOM::DocumentObject::setResultThroughput, &LQIO::DOM::DocumentObject::getResultThroughput },
    { &LQIO::DOM::DocumentObject::setResultProcessorUtilization, &LQIO::DOM::DocumentObject::getResultProcessorUtilization },
    { &LQIO::DOM::DocumentObject::setResultSquaredCoeffVariation, &LQIO::DOM::DocumentObject::getResultSquaredCoeffVariation },
    { NULL, NULL }
};

static struct {
    set_function first;
    get_function second;
} activity_variance[] = { 
//static std::pair<set_function,get_function> activity_variance[] = {
    { &LQIO::DOM::DocumentObject::setResultProcessorUtilizationVariance, &LQIO::DOM::DocumentObject::getResultProcessorWaitingVariance },
    { &LQIO::DOM::DocumentObject::setResultThroughput, &LQIO::DOM::DocumentObject::getResultThroughputVariance },
    { NULL, NULL }
};



/*
 * Strip suffix _<N>.  Merge results from replicas 2..N to 1.
 */

Activity&
Activity::replicateActivity( LQIO::DOM::Activity * root, unsigned int replica )
{
    if ( root == nullptr || getDOM() == nullptr ) return *this;

    replicatePhase( root, replica );	// Super will replicate phase part.
    if ( replica > 1 ) {
	for ( unsigned int i = 0; activity_mean[i].first != NULL; ++i ) {
	    update_mean( root, activity_mean[i].first, getDOM(), activity_mean[i].second, replica );
	    update_variance( root, activity_variance[i].first, getDOM(), activity_variance[i].second );
	}
    }
    return *this;
}


Activity&
Activity::replicateCall()
{
    std::vector<Call *> old_calls = _calls;
    _calls.clear();

    Phase::replicateCall();		/* Reset DOM calls */
    
    Call * root = NULL;
    for_each( old_calls.begin(), old_calls.end(), Exec2<Call, std::vector<Call *>&, Call **>( &Call::replicateCall, _calls, &root ) );
    return *this;
}

#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                 */
/* ------------------------------------------------------------------------ */

const Activity&
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

    for_each( calls().begin(), calls().end(), ConstExec1<GenericCall,ostream&>( &GenericCall::draw, output ) );

    myLabel->backgroundColour( colour() );
    output << *myLabel;

    /* Don't draw reply arcs here ... draw from entry (for layering purposes) */

    return *this;
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
    if ( replies().size() && !owner()->canConvertToOpenArrivals() ) {
	output << '[';
	for ( std::vector<Entry *>::const_iterator entry = replies().begin(); entry != replies().end(); ++entry ) {
	    if ( entry != replies().begin() ) output << ',';
	    output << (*entry)->name();
	}
	output << ']';
    }
    return output;
}

/*
 * Compare entries (for sorting)
 */

bool
Activity::compareCoord( const Activity * a1, const Activity * a2 )
{
    return a1->index() < a2->index();
}

/* ------------------------ Exception Handling ------------------------ */

activity_cycle::activity_cycle( const Activity * anActivity, std::deque<const Activity *>& activityStack )
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
    if ( domActivity == NULL ) return *this;	/* Bizarre */
    const std::vector<LQIO::DOM::Call*>& calls = domActivity->getCalls();
    std::vector<LQIO::DOM::Call*>::const_iterator iter;

    /* This provides us with the DOM Call which we can then take apart */
    for (iter = calls.begin(); iter != calls.end(); ++iter) {
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
    std::vector<Entry*> entryList;

    /* Walk over the list and do the equivalent of calling act_add_reply n times */
    std::vector<LQIO::DOM::Entry*>::const_iterator iter;
    for (iter = domReplyList.begin(); iter != domReplyList.end(); ++iter) {
	LQIO::DOM::Entry* domEntry = const_cast<LQIO::DOM::Entry*>(*iter);
	Entry* myEntry = Entry::find(domEntry->getName());

	/* Check it out and add it to the list */
	if (myEntry == NULL) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domEntry->getName().c_str() );
	} else if (myEntry->owner() != owner()) {
	    LQIO::input_error2( LQIO::ERR_WRONG_TASK_FOR_ENTRY, domEntry->getName().c_str(), owner()->name().c_str() );
	} else {
	    entryList.push_back( myEntry );
	}
    }

    /* Store the reply list for the activity */
    replies(entryList);
    return *this;
}

Activity&
Activity::add_activity_lists()
{
    /* Obtain the Task and Activity information DOM records */
    const LQIO::DOM::Activity* domAct = dynamic_cast<const LQIO::DOM::Activity *>(getDOM());
    if (domAct == NULL) { return *this; }
    const Task * task = owner();

    /* May as well start with the _outputTo, this is done with various methods */
    LQIO::DOM::ActivityList* joinList = domAct->getOutputToList();
    ActivityList * localActivityList = NULL;
    if (joinList != NULL && joinList->getProcessed() == false) {
	const std::vector<const LQIO::DOM::Activity*>& list = joinList->getList();
	std::vector<const LQIO::DOM::Activity*>::const_iterator iter;
	joinList->setProcessed(true);
	for (iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;

	    /* Add the activity to the appropriate list based on what kind of list we have */
	    Activity * nextActivity = task->findActivity( domAct->getName() );
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
	    Activity * nextActivity = task->findActivity( domAct->getName() );
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
	if ( src != NULL && dst != NULL ) {
	    ActivityList::act_connect(src, dst);
	}
    }
}
