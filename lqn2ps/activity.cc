/* activity.cc	-- Greg Franks Thu Apr  3 2003
 *
 * $Id: activity.cc 15958 2022-10-07 20:27:02Z greg $
 */

#include "activity.h"
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>
#include <numeric>
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_call.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_extvar.h>
#include <lqio/dom_task.h>
#include <lqio/error.h>
#include <lqx/SyntaxTree.h>
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
      _inputFrom(nullptr),
      _outputTo(nullptr),
      _replies(),
      _rootEntry(nullptr),
      _caller(nullptr),
      iAmSpecified(false),
      _level(0),
      _reachableFrom(nullptr)
{
    _node = Node::newNode( Flags::entry_width - Flags::act_x_spacing / 2., Flags::entry_height );
    _label = Label::newLabel();
}


/*
 * Free resources.
 */

Activity::~Activity()
{
    _inputFrom = nullptr;
    _outputTo = nullptr;
    std::for_each( calls().begin(), calls().end(), Delete<Call *> );
    for ( std::map<Entry *,Reply *>::const_iterator reply = replyArcs().begin(); reply != replyArcs().end(); ++reply ){
	delete reply->second;
    }

    delete _node;
    delete _label;
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

	// if ( phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC
	//      && ( remainder( dstCall->sumOfRendezvous(), 1.0 ) > EPSILON
	// 	  || remainder( dstCall->sumOfSendNoReply(), 1.0 ) > EPSILON ) ) {
	//     phaseTypeFlag( LQIO::DOM::Phase::Type::STOCHASTIC );

	dstEntry->removeDstCall( (*src_call) );	/* Unlink the activities calls. */
    }

    /* Aggregate the reply lists  */

    appendReplyList( src );

    /* Aggregate the service time. */

    const_cast<LQIO::DOM::Phase *>(getDOM())->setServiceTimeValue(to_double(*getDOM()->getServiceTime()) * rate);

    return *this;
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
	getDOM()->runtime_error( LQIO::WRN_NOT_USED );
    } else if ( !hasServiceTime() ) {
	std::string owner_type = owner()->getDOM()->getTypeName();
	owner_type[0] = std::toupper( owner_type[0] );
	getDOM()->runtime_error( LQIO::WRN_XXXX_TIME_DEFINED_BUT_ZERO, "service" );
    }

    return Phase::check();
}



const LQIO::DOM::ExternalVariable&
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
    if ( value && toEntry->isCalledBy( request_type::RENDEZVOUS ) ) {
	Model::rendezvousCount[0] += 1;

	Call * aCall = findOrAddCall( toEntry );
	aCall->rendezvous( 1, value );
    }
    return *this;
}


const LQIO::DOM::ExternalVariable&
Activity::sendNoReply( const Entry * toEntry ) const
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
    if ( value && toEntry->isCalledBy( request_type::SEND_NO_REPLY ) ) {
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
						 LQIO::DOM::Call::Type::RENDEZVOUS,
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
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, inputFrom()->getDOM()->getLineNumber() );
    } else if ( isStartActivity() ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
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
    if ( outputTo() && aList != nullptr ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, outputTo()->getDOM()->getLineNumber() );
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
Activity::replies( const std::vector<Entry *>& reply_list )
{
    _replies.clear();
    /* Delete reply arcs */
    for ( std::map<Entry *,Reply *>::const_iterator reply = replyArcs().begin(); reply != replyArcs().end(); ++reply ){
	delete reply->second;
    }
    _replyArcs.clear();
    if ( !reply_list.empty() && owner()->isReferenceTask() ) {
	getDOM()->input_error( LQIO::ERR_REFERENCE_TASK_REPLIES, reply_list.front()->name().c_str() );
    } else {
	_replies = reply_list;
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
	throw cycle_error( activityStack );
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
	throw cycle_error( activityStack );
    }
    activityStack.push_back( this );

    size_t max_depth = std::max( depth+1, level() );
    const_cast<Activity *>(this)->level( max_depth );

    if ( repliesTo( srcEntry ) ) {
	if ( p == 2 ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_DUPLICATE, srcEntry->name().c_str() );
	}
	if (  srcEntry->requestType() == request_type::SEND_NO_REPLY || srcEntry->requestType() == request_type::OPEN_ARRIVAL ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_FOR_SNR_ENTRY, srcEntry->name().c_str() );
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

const Activity&
Activity::backtrack( const std::deque<const AndForkActivityList *>& forkStack, std::set<const AndForkActivityList *>& forkSet, std::set<const AndOrJoinActivityList *>& joinSet ) const
{
    if ( inputFrom() ) inputFrom()->backtrack( forkStack, forkSet, joinSet );
    return *this;
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
	anIndex = std::numeric_limits<double>::max();
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

    if ( aFunc != &GenericCall::hasSendNoReply && (!aServer || (owner()->hasProcessor( dynamic_cast<const Processor *>(aServer) ) ) ) ) {
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
	if (  anEntry->requestType() == request_type::SEND_NO_REPLY || anEntry->requestType() == request_type::OPEN_ARRIVAL ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_FOR_SNR_ENTRY, anEntry->name().c_str() );
	} else if ( rate <= 0 ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_FROM_BRANCH, anEntry->name().c_str() );
	} else if ( p > 1 ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_DUPLICATE, anEntry->name().c_str() );
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

    switch ( Flags::aggregation() ) {
    case Aggregate::ENTRIES:
    case Aggregate::PHASES:
    case Aggregate::ACTIVITIES:
	const_cast<Entry *>(anEntry)->aggregateService( this, p, rate );
	std::map<Entry *,Reply *>::iterator reply = _replyArcs.find(anEntry);
	if ( reply != replyArcs().end() ) {
	    anEntry->deleteActivityReplyArc( reply->second );
	    delete reply->second;
	    _replyArcs.erase( reply );
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
 * Called by entry to transform a sequence of activities into a single activity.
 */

void
Activity::transmorgrify( std::deque<const Activity *>& activityStack, const double rate )
{
    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) return;
    activityStack.push_back( this );

    ActivityList * joinList = outputTo();
    LQIO::DOM::Activity * currDOM = const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(getDOM()));
    LQIO::DOM::Task * taskDOM = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(owner()->getDOM()));

    while ( joinList != nullptr && joinList->next() != nullptr ) {
	if ( dynamic_cast<ForkActivityList *>(joinList->next()) ) {	/* Sequence... */

	    ForkActivityList * forkList = dynamic_cast<ForkActivityList *>(joinList->next());
	    const LQIO::DOM::ActivityList * forkDOM = forkList->getDOM();
	    Activity * nextActivity = forkList->getMyActivity();
	    const LQIO::DOM::Activity * nextDOM = forkDOM->getList().front();

	    /* Update values */
	    
	    Phase::merge( *currDOM, *nextDOM, rate );

	    /* do calls */

	    for ( std::vector<Call *>::const_iterator call = nextActivity->calls().begin(); call != nextActivity->calls().end(); ++call ) {
		Entry * dstEntry = const_cast<Entry *>((*call)->dstEntry());

		/* Aggregate the calls made by the activity to the entry */

		Call * dstCall;
		if ( (*call)->isPseudoCall() ) {
		    dstCall = findOrAddFwdCall( dstEntry );
		} else {
		    dstCall = findOrAddCall( dstEntry );
		}
		dstCall->merge( *this, **call, rate );
		//	anActivity->removeDstCall( *call );	/* Unlink the activity's call. */
	    }
	    
	    /* Replies */

	    const std::vector<Entry *>& nextReplies = nextActivity->replies();
	    if ( nextReplies.size() > 0 ) {
		_replies.insert( _replies.end(), nextReplies.begin(), nextReplies.end() );
		std::vector<LQIO::DOM::Entry *>& currRepliesDOM = currDOM->getReplyList();
		const std::vector<LQIO::DOM::Entry *>& nextRepliesDOM = nextDOM->getReplyList();
		currRepliesDOM.insert( currRepliesDOM.end(), nextRepliesDOM.begin(), nextRepliesDOM.end() );
	    }
		
	    /* reconnect lists */
	    
	    taskDOM->removeActivityList( currDOM->getOutputToList()->getNext() );	/* Fork List */
	    taskDOM->removeActivityList( currDOM->getOutputToList() );			/* Join List */
	    joinList = dynamic_cast<ForkActivityList *>(joinList->next())->getMyActivity()->outputTo();
	    currDOM->outputTo( nullptr );
	    currDOM->outputTo( const_cast<LQIO::DOM::ActivityList *>(nextDOM->getOutputToList()));
	    outputTo( nullptr );
	    outputTo( joinList );
	    taskDOM->removeActivity( const_cast<LQIO::DOM::Activity *>(nextDOM) );
	    const_cast<Task *>(owner())->removeActivity( this );
	    
	    /* if replyto(), recurse... !!! */

	    if ( replies().size() > 0 ) {
		break;
	    }
	    
	} else if ( dynamic_cast<OrForkActivityList *>(outputTo()->next()) ) {
	    /* or fork - merge. !!! */
	    abort();
	} else if ( dynamic_cast<AndForkActivityList *>(outputTo()->next()) ) {
	    /* and fork - recurse !!! */
	    abort();
	} else {
	    abort();
	}
    }
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
    return nullptr;
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
    return nullptr;
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
    return std::any_of( calls().begin(), calls().end(), ::Predicate<GenericCall>( &GenericCall::hasRendezvous ) );
}

bool
Activity::hasSendNoReply() const
{
    return std::any_of( calls().begin(), calls().end(), ::Predicate<GenericCall>( &GenericCall::hasSendNoReply ) );
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
	    time += (*call)->sumOfRendezvous()->invoke(nullptr)->getDoubleValue() * ((*call)->waiting(1) + (*call)->dstEntry()->executionTime(1));
	}
    }

    /* Add in processor queueing is it isn't selected */

    if ( std::any_of( owner()->processors().begin(), owner()->processors().end(), Predicate<Processor>( &Processor::isSelected ) ) ) {
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
    return std::any_of( calls().begin(), calls().end(), GenericCall::PredicateAndSelected( predicate ) );
}



/*
 * Return true is this entry is selected for printing.
 */

bool
Activity::isSelectedIndirectly() const
{
    if ( Flags::chain() ) {
	return hasPath( Flags::chain() );
    } else if ( owner()->isSelected() ) {
	return true;
    }
    return std::any_of( calls().begin(), calls().end(), ::Predicate<GenericCall>( &GenericCall::isSelected ) );
}



/*
 * disconnect nextActivity from this list.
 */

Activity&
Activity::disconnect( Activity* nextActivity )
{
    /* Reconnect lists */

    outputTo( nextActivity->outputTo() );
    nextActivity->outputTo( nullptr );
    if ( outputTo() ) {
	outputTo()->reconnect( nextActivity, this );
    }

    const_cast<Task *>(owner())->removeActivity( nextActivity );

    return *this;
}



Activity&
Activity::sort()
{
    std::sort( _calls.begin(), _calls.end(), &Call::compareSrc );
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
    _label->moveTo( center() );

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
    *_label << name();
    if ( Flags::print_input_parameters() && hasServiceTime() ) {
	_label->newLine() << '[' << serviceTime()  << ']';
    }
    if ( Flags::have_results ) {
	if ( Flags::print[SERVICE].opts.value.b ) {
	    _label->newLine() << begin_math() << opt_pct(executionTime()) << end_math();
	}
	if ( Flags::print[VARIANCE].opts.value.b ) {
	    _label->newLine() << begin_math( &Label::sigma ) << "=" << opt_pct(variance()) << end_math();
	}
    }

    /* Now do calls. */

    for_each ( calls().begin(), calls().end(), Exec<GenericCall>( &GenericCall::label ) );

    return *this;
}



Graphic::Colour
Activity::colour() const
{
    if ( !reachable() ) {
	return Graphic::Colour::RED;
    } else switch ( Flags::colouring() ) {
	case Colouring::RESULTS:
	case Colouring::DIFFERENCES:
	return owner()->colour();
    default:
	return Graphic::Colour::DEFAULT;
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
	assert( fan_out <= dst_replicas );

	for ( unsigned k = 0; k < fan_out; k++ ) {
	    Entry *dstEntry = Entry::find_replica( (*call)->dstEntry()->name(),
						   ((k + (replica - 1) * fan_out) % dst_replicas) + 1 );

	    LQIO::DOM::Entry * dst_dom = const_cast<LQIO::DOM::Entry*>(dynamic_cast<const LQIO::DOM::Entry*>(dstEntry->getDOM()));
	    if ( dst_dom == nullptr || (!(*call)->hasRendezvous() && !(*call)->hasSendNoReply()) ) continue;
	    LQIO::DOM::Call * dom_call = (*call)->getDOM(1)->clone();
	    dom_call->setSourceObject( const_cast<LQIO::DOM::Phase *>(getDOM()) );
	    dom_call->setDestinationEntry( dst_dom );
#if BUG_299
	    dom_call->setCallMeanValue( dom_call->getCallMeanValue() / fan_out );			    /*+ BUG 299 */
#endif
	    if ( (*call)->hasRendezvous() ) {
		rendezvous( dstEntry, dom_call );
	    } else if ( (*call)->hasSendNoReply() ) {
		sendNoReply( dstEntry, dom_call );
	    }
	    dom_activity->addCall( dom_call );
	}
    }
    return *this;
}



/*
 * Strip suffix _<N>.  Merge results from replicas 2..N to 1.
 */

Activity&
Activity::replicateActivity( LQIO::DOM::Activity * root, unsigned int replica )
{
    const static struct {
	set_function first;
	get_function second;
    } activity_mean[] = { 
	{ &LQIO::DOM::DocumentObject::setResultThroughput, &LQIO::DOM::DocumentObject::getResultThroughput },
	{ &LQIO::DOM::DocumentObject::setResultProcessorUtilization, &LQIO::DOM::DocumentObject::getResultProcessorUtilization },
	{ &LQIO::DOM::DocumentObject::setResultSquaredCoeffVariation, &LQIO::DOM::DocumentObject::getResultSquaredCoeffVariation },
	{ nullptr, nullptr }
    };

    const static struct {
	set_function first;
	get_function second;
    } activity_variance[] = { 
	{ &LQIO::DOM::DocumentObject::setResultProcessorUtilizationVariance, &LQIO::DOM::DocumentObject::getResultProcessorWaitingVariance },
	{ &LQIO::DOM::DocumentObject::setResultThroughput, &LQIO::DOM::DocumentObject::getResultThroughputVariance },
	{ nullptr, nullptr }
    };

    if ( root == nullptr || getDOM() == nullptr ) return *this;

    replicatePhase( root, replica );	// Super will replicate phase part.
    if ( replica > 1 ) {
	for ( unsigned int i = 0; activity_mean[i].first != nullptr; ++i ) {
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
    
    Call * root = nullptr;
    for_each( old_calls.begin(), old_calls.end(), Exec2<Call, std::vector<Call *>&, Call **>( &Call::replicateCall, _calls, &root ) );
    return *this;
}

#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                 */
/* ------------------------------------------------------------------------ */

const Activity&
Activity::draw( std::ostream & output ) const
{
    std::ostringstream aComment;
    aComment << "Activity " << name();
    if ( hasServiceTime() ) {
	aComment << " s [" << serviceTime() << "]";
    }
    aComment << " level=" << level();

    _node->comment( output, aComment.str() );
    _node->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() ).fillColour( colour() );
    _node->rectangle( output );

    for_each( calls().begin(), calls().end(), ConstExec1<GenericCall,std::ostream&>( &GenericCall::draw, output ) );

    _label->backgroundColour( colour() );
    output << *_label;

    /* Don't draw reply arcs here ... draw from entry (for layering purposes) */

    return *this;
}


/*
 * Print out the name of the activity along with any entries to whom
 * it replies.  IFF we are generating a submodel, the reply list will
 * be suppressed if the task is being converted into one which accepts
 * open arrivals.
 */

std::ostream&
Activity::printNameWithReply( std::ostream& output ) const
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

Activity::cycle_error::cycle_error( std::deque<const Activity *>& activityStack )
    : std::runtime_error( std::accumulate( std::next( activityStack.rbegin() ), activityStack.rend(), activityStack.back()->name(), fold  ) ),
      _depth( activityStack.size() )
{
}

std::string
Activity::cycle_error::fold( const std::string& s1, const Activity * a2 )
{
    return s1 + ", " + a2->name();
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
    if ( domActivity == nullptr ) return *this;	/* Bizarre */
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
	    if (domCall->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY) {
		sendNoReply(destEntry, domCall);
	    } else if (domCall->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS) {
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
	if (myEntry == nullptr) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domEntry->getName().c_str() );
	} else if (myEntry->owner() != owner()) {
	    getDOM()->input_error( LQIO::ERR_WRONG_TASK_FOR_ENTRY, owner()->name().c_str() );
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
    if (domAct == nullptr) { return *this; }
    const Task * task = owner();

    /* May as well start with the _outputTo, this is done with various methods */
    LQIO::DOM::ActivityList* joinList = domAct->getOutputToList();
    ActivityList * localActivityList = nullptr;
    if (joinList != nullptr && domToNative.find(joinList) == domToNative.end()) {
	const std::vector<const LQIO::DOM::Activity*>& list = joinList->getList();
	for (std::vector<const LQIO::DOM::Activity*>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;

	    /* Add the activity to the appropriate list based on what kind of list we have */
	    Activity * nextActivity = task->findActivity( domAct->getName() );
	    if ( !nextActivity ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }

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
	    Activity * nextActivity = task->findActivity( domAct->getName() );
	    if ( !nextActivity ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }

	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( forkList->getListType() ) {
	    case LQIO::DOM::ActivityList::Type::FORK:
		localActivityList = nextActivity->act_fork_item( forkList );
		break;
	    case LQIO::DOM::ActivityList::Type::AND_FORK:
		localActivityList = nextActivity->act_and_fork_list(localActivityList, forkList );
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
        getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
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
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
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
	if ( src != nullptr && dst != nullptr ) {
	    ActivityList::act_connect(src, dst);
	}
    }
}
