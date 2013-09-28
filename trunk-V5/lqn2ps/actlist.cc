/* actlist.cc	-- Greg Franks Thu Apr 10 2003
 *
 * Connections between activities are activity lists.  For srvn output, 
 * this is all the stuff printed after the ':'.  For xml output, this
 * is all of the precendence stuff.
 * 
 * $Id$
 */


#include "actlist.h"
#include <cmath>
#include <cstdlib>
#include <limits.h>
#if HAVE_VALUES_H
#include <values.h>
#endif
#if HAVE_FLOAT_H
#include <float.h>
#endif
#include <lqio/error.h>
#include <lqio/dom_actlist.h>
#include "stack.h"
#include "task.h"
#include "entry.h"
#include "activity.h"
#include "errmsg.h"
#include "label.h"
#include <string>

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

ostream& 
operator<<( ostream& output, const ActivityList& self ) 
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


class ActivityListManip {
public:
    ActivityListManip( ostream& (*ff)( ostream& ) )
	: f(ff) {}
private:
    const ActivityList * anActivityList;
    ostream& (*f)( ostream& );

    friend ostream& operator<<(ostream & os, const ActivityListManip& m ) 
	{ return m.f(os); }
};


bool ActivityList::first = true;

/* -------------------------------------------------------------------- */
/*               Activity Lists -- Abstract Superclass                  */
/* -------------------------------------------------------------------- */

ActivityList::ActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist )
    : myOwner(owner), myDOM(dom_activitylist) 
{
    const_cast<Task *>(owner)->addPrecedence(this);
}

ActivityList::~ActivityList()
{
}


ActivityList&
ActivityList::reconnect( Activity *curr, Activity *next )
{
    return *this;
}



/*
 * Return the next link.
 */
 
ActivityList*
ActivityList::next() const
{
    throw should_not_implement( "ActivityList::next", __FILE__, __LINE__ );
    return 0;
}


/*
 * Return the next link.
 */
 
ActivityList*
ActivityList::prev() const
{
    throw should_not_implement( "ActivityList::prev", __FILE__, __LINE__ );
    return 0;
}


/*
 * Set the next link.
 */
 
ActivityList&
ActivityList::next( ActivityList * ) 
{
    throw should_not_implement( "ActivityList::next", __FILE__, __LINE__ );
    return *this;
}


/*
 * Set the prev link.
 */
 
ActivityList&
ActivityList::prev( ActivityList * ) 
{
    throw should_not_implement( "ActivityList::prev", __FILE__, __LINE__ );
    return *this;
}


Graphic::colour_type 
ActivityList::colour() const
{
    return owner()->colour();
}

/* -------------------------------------------------------------------- */
/*                       Simple Activity Lists                          */
/* -------------------------------------------------------------------- */

SequentialActivityList::SequentialActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : ActivityList( owner,dom_activitylist), myActivity(0) 
{
    myArc = Arc::newArc();
}


SequentialActivityList::~SequentialActivityList()
{
    delete myArc;
}


/*
 * Add an activity to a simple activity list.
 */

SequentialActivityList&
SequentialActivityList::add( Activity * anActivity )
{
    myActivity = anActivity;
    return *this;
}


SequentialActivityList& 
SequentialActivityList::scaleBy( const double sx, const double sy )
{
    myArc->scaleBy( sx, sy );
    return *this;
}



SequentialActivityList& 
SequentialActivityList::translateY( const double dy )
{
    myArc->translateY( dy );
    return *this;
}



SequentialActivityList& 
SequentialActivityList::depth( const unsigned curDepth )
{
    myArc->depth( curDepth );
    return *this;
}


Point
SequentialActivityList::findSrcPoint() const
{
    Point aPoint = myActivity->topCenter();
    aPoint.moveBy( 0, height() );
    return aPoint;
}

Point
SequentialActivityList::findDstPoint() const
{
    Point aPoint = myActivity->bottomCenter();
    aPoint.moveBy( 0, -height() );
    return aPoint;
}



ostream&
SequentialActivityList::draw( ostream& output ) const
{
    myArc->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() );
    if ( myActivity && (*myArc)[1] != (*myArc)[2] ) {
	output << *myArc;
    }
    return output;
}

/* -------------------------------------------------------------------- */
/*                       Simple Forks (rvalues)                         */
/* -------------------------------------------------------------------- */

ForkActivityList::ForkActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : SequentialActivityList( owner, dom_activitylist ), 
      prevLink(0) 
{
}


ForkActivityList * 
ForkActivityList::clone() const
{
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new ForkActivityList( owner(), new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType(), 0 ) );
}



unsigned
ForkActivityList::findChildren( CallStack& callStack, const unsigned directPath, Stack<const Activity *>& activityStack ) const
{
    if ( myActivity ) {
	return myActivity->findChildren( callStack, directPath, activityStack );
    } else {
	return callStack.size();
    }
}

unsigned
ForkActivityList::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * anEntry, const unsigned depth, const unsigned p, const double rate ) const
{
    if ( myActivity ) {
	return myActivity->findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    } else {
	return depth;
    }
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

unsigned
ForkActivityList::backtrack( Stack<const AndForkActivityList *>& forkStack ) const
{
    if ( prev() ) {
	return prev()->backtrack( forkStack );
    } else {
	return 0;
    }
}



/*
 * Return the sum of aFunc.
 */

double
ForkActivityList::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, aggregateFunc aFunc ) const
{
    if ( myActivity ) {
	return myActivity->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
	return 0.0;
    }
}


unsigned
ForkActivityList::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callFunc aFunc ) const
{
    if ( myActivity ) {
	return myActivity->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } else {
	return next_k;
    }
}



double
ForkActivityList::getIndex() const
{
    if ( prev() ) {
	return prev()->getIndex();
    } else {
	return MAXDOUBLE;
    }
}



ForkActivityList&
ForkActivityList::reconnect( Activity *, Activity * )
{
    throw should_not_implement( "ForkActivityList::reconnect", __FILE__, __LINE__ );
    return *this;
}


/*
 * Called from JoinList::moveTo().
 */

ForkActivityList&
ForkActivityList::moveSrcTo( const Point& src, Activity * )
{ 
    myArc->moveSrc( src );
    return *this; 
} 


ForkActivityList&
ForkActivityList::moveDstTo( const Point& dst, Activity * )
{ 
    myArc->moveDst( dst );
    if ( prev() ) {
	const Point src( prev()->findDstPoint() );
	myArc->moveSrc( src );
	prev()->moveDstTo( src );
    }
    return *this;
}


/*
 * Try to straighten out the fork list. 
 */

double
ForkActivityList::align() const
{
    return prev()->findDstPoint().x() - myArc->dstPoint().x();
}

/* -------------------------------------------------------------------- */
/*                      Simple Joins (lvalues)                          */
/* -------------------------------------------------------------------- */

JoinActivityList::JoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : SequentialActivityList( owner, dom_activitylist ), 
      nextLink(0) 
{
}


JoinActivityList *
JoinActivityList::clone() const
{
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new JoinActivityList( owner(), new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType(), 0 ) ); 
}



unsigned
JoinActivityList::findChildren( CallStack& callStack, const unsigned directPath, Stack<const Activity *>& activityStack ) const
{
    if ( next() ) {
	return next()->findChildren( callStack, directPath, activityStack );
    } else {
	return callStack.size();
    }
}


unsigned
JoinActivityList::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * anEntry, const unsigned depth, const unsigned p, const double rate ) const
{
    unsigned nextLevel = depth;
    if ( next() ) {
	nextLevel = next()->findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    }
    return nextLevel;
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

unsigned
JoinActivityList::backtrack( Stack<const AndForkActivityList *>& forkStack ) const
{
    if ( myActivity ) {
	return myActivity->backtrack( forkStack );
    } else {
	return 0;
    }
}



/*
 * Return the sum of aFunc.  When aggregating service time, collapse
 * strings of sequential activities.
 */

double
JoinActivityList::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, aggregateFunc aFunc ) const
{
    if ( next() ) {
	double count = next()->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
	if ( aFunc == &Activity::aggregateService
	     && (Flags::print[AGGREGATION].value.i == AGGREGATE_SEQUENCES
		 || Flags::print[AGGREGATION].value.i == AGGREGATE_THREADS)
	     && dynamic_cast<ForkActivityList *>(next())
	     && !dynamic_cast<RepeatActivityList *>(next()) ) {

	    /* Simple sequence -- aggregate */
	    
	    Activity * nextActivity = dynamic_cast<ForkActivityList *>(next())->myActivity;
	    const_cast<Activity *>(myActivity)->merge( *nextActivity ).disconnect( nextActivity );
	    delete nextActivity;
	} 
	return count;
    } else {
	return 0.0;
    }
}


unsigned
JoinActivityList::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callFunc aFunc ) const
{
    if ( next() ) {
	return next()->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } else {
	return next_k;
    }
}



double
JoinActivityList::getIndex() const
{
    if ( myActivity ) {
	return myActivity->index();
    } else {
	return MAXDOUBLE;
    }
}


/*
 * Change from curr to next.
 */

JoinActivityList&
JoinActivityList::reconnect( Activity * curr,  Activity * next ) 
{
    ActivityList::reconnect( curr, next );
    myActivity = next;
    return *this;
}


double
JoinActivityList::height() const 
{ 
    double h = interActivitySpace / 2; 
    if ( next() ) {
	h += next()->height();
    }
    return h;
}


/*
 * called from Activity::moveTo()
 */

JoinActivityList&
JoinActivityList::moveSrcTo( const Point& src, Activity * )
{ 
    if ( next() ) {
	if ( dynamic_cast<ForkActivityList *>(next()) ) {
	    myArc->arrowhead( Graphic::NO_ARROW );
	}
	myArc->moveSrc( src ); 
	const Point dst( next()->findSrcPoint() );
	myArc->moveDst( dst );
	next()->moveSrcTo( dst );
    }
    return *this; 
} 



JoinActivityList&
JoinActivityList::moveDstTo( const Point& dst, Activity * )
{ 
    myArc->moveDst( dst );
    return *this;
}

/*----------------------------------------------------------------------*/
/*                  Activity lists that fork or join.                   */
/*----------------------------------------------------------------------*/

ForkJoinActivityList::ForkJoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : ActivityList( owner, dom_activitylist ), iHaveBeenDrawn(false),  iHaveBeenLabelled(false)
{
    myNode = Node::newNode( 12.0, 12.0 );
}


ForkJoinActivityList::~ForkJoinActivityList()
{
    delete myNode;
    myActivityList.clearContents();
    myArcList.deleteContents();
}


/*
 * Add an item to the activity list.
 */

ForkJoinActivityList&
ForkJoinActivityList::add( Activity * anActivity )
{
    myActivityList << anActivity;
    Arc * anArc = Arc::newArc();
    if ( anArc ) {
	myArcList << anArc;
    }
    return *this;
}


ForkJoinActivityList& 
ForkJoinActivityList::scaleBy( const double sx, const double sy )
{
    Sequence<Arc *> nextArc( myArcList );
    Arc * anArc;

    while ( anArc = nextArc() ) {
	anArc->scaleBy( sx, sy );
    }
    myNode->scaleBy( sx, sy );
    return *this;
}



ForkJoinActivityList& 
ForkJoinActivityList::translateY( const double dy )
{
    Sequence<Arc *> nextArc( myArcList );
    Arc * anArc;

    while ( anArc = nextArc() ) {
	anArc->translateY( dy );
    }
    myNode->translateY( dy );
    return *this;
}



ForkJoinActivityList& 
ForkJoinActivityList::depth( const unsigned curDepth )
{
    Sequence<Arc *> nextArc( myArcList );
    Arc * anArc;

    while ( anArc = nextArc() ) {
	anArc->depth( curDepth );
    }
    myNode->depth( curDepth-2 );
    return *this;
}


Point
ForkJoinActivityList::findSrcPoint() const
{
    Point p1( myActivityList.first()->topCenter() );
    Point p2( myActivityList.last()->topCenter() );
    return Point( (p1.x() + p2.x()) / 2.0, p1.y() + height() );
}

Point
ForkJoinActivityList::findDstPoint() const
{
    Point p1( myActivityList.first()->bottomCenter() );
    Point p2( myActivityList.last()->bottomCenter() );
    return Point( (p1.x() + p2.x()) / 2, p1.y() - height() );
}


double
ForkJoinActivityList::radius() const
{
    return fabs( myNode->extent.y() ) / 2.0;
}


ostream& 
ForkJoinActivityList::draw( ostream& output ) const
{
    if ( testAndSetDrawn() ) return output;

    Sequence<Arc *> nextArc( myArcList );
    Arc * anArc;

    while ( anArc = nextArc() ) {
	anArc->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() );
    }

    const Point ctr( myNode->center() );
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    myNode->circle( output, ctr, radius() );
    Point aPoint( ctr.x(), (ctr.y() + myNode->bottom()) / 2. );

    myNode->text( output, aPoint, typeStr() );

    return output;
}


/*
 * Construct a list name.
 */

ForkJoinActivityList::ForkJoinName::ForkJoinName( const ForkJoinActivityList& aList )
{
    BackwardsSequence<Activity *> nextActivity( aList.myActivityList );
    Activity * anActivity;

    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
	if ( i > 1 ) {
	    aString += ' ';
	    aString += aList.typeStr();
	    aString += ' ';
	}
	aString += anActivity->name();
    }
    aString += '\0'; 	/* null terminate */
}


const char * 
ForkJoinActivityList::ForkJoinName::operator()()
{
    return aString.c_str();
}

/* -------------------------------------------------------------------- */
/*          Activity Lists that fork -- abstract superclass             */
/* -------------------------------------------------------------------- */

AndOrForkActivityList::AndOrForkActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : ForkJoinActivityList( owner, dom_activitylist ), 
      prevLink(0) 
{
}

OrForkActivityList *
OrForkActivityList::clone() const
{
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new OrForkActivityList( owner(), new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType(), 0  ) ); 
}

unsigned
AndOrForkActivityList::findChildren( CallStack& callStack, const unsigned directPath, Stack<const Activity *>& activityStack ) const
{
    int nextLevel = callStack.size();

    for ( unsigned int i = 1; i <= size(); ++i ) {
	Activity * anActivity = myActivityList[i];		/* Skip non-interesting branches */
	nextLevel = max( anActivity->findChildren( callStack, directPath, activityStack ), nextLevel );
    } 
    return nextLevel;
}



double
AndOrForkActivityList::getIndex() const
{
    if ( prev() ) {
	return prev()->getIndex();
    } else {
	return MAXDOUBLE;
    }
}


AndOrForkActivityList&
AndOrForkActivityList::reconnect( Activity *, Activity * )
{
    throw should_not_implement( "AndOrForkActivityList::reconnect", __FILE__, __LINE__ );
    return *this;
}

Point
AndOrForkActivityList::srcPoint() const
{
    return myNode->topCenter();
}

Point
AndOrForkActivityList::dstPoint() const
{
    throw not_implemented( "AndOrForkActivityList::dstPoint", __FILE__, __LINE__ );
    return Point(0,0);
}


/*
 * Called from JoinList::moveTo().
 */

AndOrForkActivityList&
AndOrForkActivityList::moveSrcTo( const Point& src, Activity * )
{ 
    myNode->moveTo( src.x() - radius(), src.y() - 2 * radius() );
    const Point ctr( myNode->center() );
    for ( unsigned int i = 1; i <= size(); ++i ) {
	myArcList[i]->moveSrc( ctr );
	const Point src2 = myArcList[i]->srcIntersectsCircle( ctr, radius() );
	myArcList[i]->moveSrc( src2 );
    }
    
    return *this; 
} 


AndOrForkActivityList&
AndOrForkActivityList::moveDstTo( const Point& dst, Activity * anActivity )
{ 
    const int i = myActivityList.find( anActivity );

    if ( i > 0 && prev() ) {
	const Point p1( myActivityList.first()->topCenter() );
	const Point p2( myActivityList.last()->topCenter() );
	myNode->moveTo( (p1.x() + p2.x()) / 2. - radius(), prev()->findSrcPoint().y() );
	const Point src = myNode->center(); // 

	myArcList[i]->moveDst( dst );
	myArcList[i]->moveSrc( src );
	const Point p3 = myArcList[i]->dstIntersectsCircle( src, radius() );
	myArcList[i]->moveSrc( p3 );

	prev()->moveDstTo( myNode->topCenter() );
    }
    return *this;
}



/*
 * 
 */

double 
AndOrForkActivityList::align() const
{
    return prev()->findDstPoint().x() - findSrcPoint().x();
}

ostream&
AndOrForkActivityList::draw( ostream& output ) const
{
    if ( drawn() ) return output;

    ForkJoinActivityList::draw( output );

    for ( unsigned int i = 1; i <= size(); ++i ) {
	output << *myArcList[i];
    }

    return output;
}

/* -------------------------------------------------------------------- */
/*                      Or Fork Activity Lists                          */
/* -------------------------------------------------------------------- */

OrForkActivityList::OrForkActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : AndOrForkActivityList( owner, dom_activitylist ) 
{
}


OrJoinActivityList *
OrJoinActivityList::clone() const 
{ 
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new OrJoinActivityList( owner(), new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType(), 0  ) ); 
} 


OrForkActivityList::~OrForkActivityList()
{
    myLabelList.deleteContents();
}


LQIO::DOM::ExternalVariable& 
OrForkActivityList::prBranch( const unsigned int i ) const
{
    return *getDOM()->getParameter(dynamic_cast<const LQIO::DOM::Activity *>(myActivityList[i]->getDOM()));
}


OrForkActivityList&
OrForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );
    Label * aLabel = Label::newLabel();
    if ( aLabel ) {
	myLabelList << aLabel;
	aLabel->justification( Flags::label_justification );
    }
    return *this;
}


/*
 * Aggregation here....  Try to figure out if this or fork is aggregatable.	
 */

unsigned
OrForkActivityList::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * anEntry, const unsigned depth, const unsigned p, const double rate ) const
{

    Activity * anActivity;
    unsigned nextLevel = depth;

    for ( unsigned int i = 1; i <= size(); ++i ) {
	anActivity = myActivityList[i];		/* Skip non-interesting branches */
	nextLevel = max( anActivity->findActivityChildren( activityStack, forkStack, anEntry, depth, p, /* prBranch(i) * */ rate ), nextLevel );
	/* sum += prBranch(i); */
    } 

    // if ( fabs( 1.0 - sum ) > EPSILON ) {
    // 	ForkJoinName aName( *this );
    // 	LQIO::solution_error( LQIO::ERR_MISSING_OR_BRANCH, aName(), owner()->name().c_str(), sum );
    // }

    return nextLevel;
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

unsigned
OrForkActivityList::backtrack( Stack<const AndForkActivityList *>& forkStack ) const
{
    if ( prev() ) {
	return prev()->backtrack( forkStack );
    } else {
	return 0;
    }
}



/*
 * Return the sum of aFunc.  For service time aggregation, we're
 * trying to get rid of the or-forks.  After aggregating the branches,
 * if we end up with a fork, activity, join, then we can smash them
 * all together. Needless to say, this is all recursize.
 */

double
OrForkActivityList::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, aggregateFunc aFunc ) const
{
    double sum = 0.0;

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    next_p = curr_p;
    for ( unsigned i = 1; anActivity = nextActivity(); ++i  ) {
	unsigned branch_p = curr_p;
	double prob;
	prBranch(i).getValue( prob );
	sum += anActivity->aggregate( anEntry, curr_p, branch_p, rate * prob, activityStack, aFunc );
	next_p = max( next_p, branch_p );
    } 

    /* 
     * Try to figure out if we can collapse this or-fork.  It amounts
     * to ensuring that there is only one activity between the fork
     * and the join, and the lists are the same size.
     */

    ActivityList * joinList = myActivityList[1]->outputToList;

    if ( aFunc == &Activity::aggregateService
	 && joinList->size() == size()
	 && Flags::print[AGGREGATION].value.i == AGGREGATE_THREADS
	 && dynamic_cast<OrJoinActivityList *>(joinList)
	 && dynamic_cast<ForkActivityList *>(joinList->next())
	 && !dynamic_cast<RepeatActivityList *>(joinList->next()) ) {

	Activity * anActivity;

	for ( unsigned i = 2; i <= size(); ++i ) {
	    anActivity = myActivityList[i];
	    if ( !dynamic_cast<OrJoinActivityList *>(anActivity->outputToList) 
		 || anActivity->outputToList != joinList ) {
		return sum;
	    }
	} 

	/* Success */

	/* Aggregate everything into joinList->next()->myActivity */

	Activity * nextActivity = dynamic_cast<ForkActivityList *>(joinList->next())->myActivity;
	unsigned currLevel = 0;
	for ( unsigned i = 1; i <= size(); ++i ) {
	    anActivity = myActivityList[i];
	    currLevel = max( currLevel, anActivity->level() );
	    double prob;
	    prBranch(i).getValue(prob);
	    nextActivity->merge( *anActivity, prob );
	}

	/* connect joinLlist->next() into this->prev() */

	prev()->next( joinList->next() );
	joinList->next()->prev( prev() );
	joinList->next( 0 );
	const_cast<OrForkActivityList *>(this)->prev( 0 );

	/* 
	 * Delete the or fork and join lists.  DON'T delete the
	 * originator here, because we need to access
	 * myActivityList 
	 */

	for ( unsigned i = 1; i <= size(); ++i ) {
	    anActivity = myActivityList[i];
	    const_cast<Task *>(nextActivity->owner())->removeActivity( anActivity );
	}
    }
    return sum;
}



unsigned
OrForkActivityList::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callFunc aFunc ) const
{
    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	next_k = anActivity->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } 
    return next_k;
}



OrForkActivityList& 
OrForkActivityList::translateY( const double dy )
{
    ForkJoinActivityList::translateY( dy );
    for ( unsigned int i = 1; i <= size(); ++i ) {
	myLabelList[i]->translateY( dy );
    }
    return *this;
}


OrForkActivityList& 
OrForkActivityList::scaleBy( const double sx, const double sy )
{
    ForkJoinActivityList::scaleBy( sx, sy );
    for ( unsigned int i = 1; i <= size(); ++i ) {
	myLabelList[i]->scaleBy( sx, sy );
    }
    return *this;
}



OrForkActivityList&
OrForkActivityList::label()
{
    if ( !labelled() && Flags::print[INPUT_PARAMETERS].value.b ) {
	for ( unsigned int i = 1; i <= size(); ++i ) {
	    *(myLabelList[i]) << prBranch( i );
	}
    }
    return *this;
}



OrForkActivityList&
OrForkActivityList::moveSrcTo( const Point& src, Activity * anActivity )
{
    AndOrForkActivityList::moveSrcTo( src, anActivity );
#if 0
    for ( unsigned int i = 1; i <= size(); ++i ) {
	Point aPoint = myArcList[i]->pointFromDst(height()/3.0);
	myLabelList[i]->moveTo( aPoint );
    }
#endif
    return *this;
}



OrForkActivityList&
OrForkActivityList::moveDstTo( const Point& dst, Activity * anActivity )
{
    AndOrForkActivityList::moveDstTo( dst, anActivity );
    for ( unsigned int i = 1; i <= size(); ++i ) {
	Point aPoint = myArcList[i]->pointFromDst(height()/3.0);
	myLabelList[i]->moveTo( aPoint );
    }
    return *this;
}



ostream&
OrForkActivityList::draw( ostream& output ) const
{
    if ( drawn() ) return output;

    AndOrForkActivityList::draw( output );

    for ( unsigned int i = 1; i <= size(); ++i ) {
	output << *(myArcList[i]);
	output << *(myLabelList[i]);
    }
    return output;
}

/* -------------------------------------------------------------------- */
/*                      And Fork Activity Lists                         */
/* -------------------------------------------------------------------- */



AndForkActivityList *
AndForkActivityList::clone() const 
{ 
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new AndForkActivityList( owner(), new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType(), 0  ) ); 
} 



AndForkActivityList&
AndForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );
    return *this;

}



unsigned
AndForkActivityList::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * anEntry, const unsigned depth, const unsigned p, const double rate ) const
{
    Activity * anActivity;
    unsigned nextLevel = depth;

    forkStack.push( this );
    for ( unsigned int i = 1; i <= size(); ++i ) {
	anActivity = myActivityList[i];		/* Skip non-interesting branches */
	nextLevel = max( anActivity->findActivityChildren( activityStack, forkStack, anEntry, depth, p, /* prBranch(i) * */ rate ), nextLevel );
    } 
    forkStack.pop();
    return nextLevel;
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

unsigned
AndForkActivityList::backtrack( Stack<const AndForkActivityList *>& forkStack ) const
{
    unsigned i = forkStack.find( this );
    if ( i > 0 ) {
	return i;
    } else if ( prev() ) {
	return prev()->backtrack( forkStack );
    } else {
	return 0;
    }
}



/*
 * Return the sum of aFunc.
 */

double
AndForkActivityList::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, aggregateFunc aFunc ) const
{
    double sum = 0.0;

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    for ( unsigned i = 1; anActivity = nextActivity(); ++i  ) {
	unsigned branch_p = curr_p;
	sum += anActivity->aggregate( anEntry, curr_p, branch_p, rate, activityStack, aFunc );
	next_p = max( next_p, branch_p );
    } 

    /* Now follow the activities after the join */

    if ( myJoinList && myJoinList->next() ) {
	sum += myJoinList->next()->aggregate( anEntry, next_p, next_p, rate, activityStack, aFunc );
    } else {
	/* Flushing... */
	const_cast<Entry *>(anEntry)->phaseIsPresent( next_p, true );
    }
    return sum;
}


/*
 * Set the chains.  Since this is a fork, we create a new chain for
 * each branch.  The activities that follow the join are part of the
 * original chain.
 */

unsigned
AndForkActivityList::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callFunc aFunc ) const
{
    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	next_k += 1;
	next_k = anActivity->setChain( activityStack, next_k, next_k, aServer, aFunc );
    } 

    /* Now follow the activities after the join */

    if ( myJoinList && myJoinList->next() ) {
	next_k = myJoinList->next()->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    }

    return next_k;
}

/* -------------------------------------------------------------------- */
/*        And Or Join Activity Lists -- Abstract superclass             */
/* -------------------------------------------------------------------- */

AndOrJoinActivityList::AndOrJoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : ForkJoinActivityList( owner, dom_activitylist ), 
      nextLink(0) 
{
}

unsigned
AndOrJoinActivityList::findChildren( CallStack& callStack, const unsigned directPath, Stack<const Activity *>& activityStack ) const
{
    if ( next() ) {
	return next()->findChildren( callStack, directPath, activityStack );
    } else {
	return callStack.size();
    }
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

unsigned
AndOrJoinActivityList::backtrack( Stack<const AndForkActivityList *>& forkStack ) const
{
    unsigned depth = 0;
    Sequence<Activity *> nextActivity( myActivityList );
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	depth = max( depth, anActivity->backtrack( forkStack ) );
    }
    return depth;
}



double
AndOrJoinActivityList::getIndex() const
{
    Sequence<Activity *> nextActivity( myActivityList );
    Activity * anActivity;
    double anIndex = MAXDOUBLE;

    while ( anActivity = nextActivity() ) {
	anIndex = min( anIndex, anActivity->index() );
    }
    return anIndex;
}



AndOrJoinActivityList&
AndOrJoinActivityList::reconnect( Activity * curr, Activity * next )
{
    ActivityList::reconnect( curr, next );
    int i = myActivityList.find( curr );
    myActivityList[i] = next;
    return *this;
}



Point
AndOrJoinActivityList::srcPoint() const 
{ 
    throw not_implemented( "AndOrJoinActivityList::dstPoint", __FILE__, __LINE__ );
    return Point(0,0);
}


Point
AndOrJoinActivityList::dstPoint() const
{
    return myNode->bottomCenter(); 
}



double
AndOrJoinActivityList::height() const 
{ 
    double h = interActivitySpace; 
    if ( next() ) {
	h += next()->height();
    }
    return h;
}


/*
 * called from Activity::moveTo()
 */

AndOrJoinActivityList&
AndOrJoinActivityList::moveSrcTo( const Point& src, Activity * anActivity )
{ 
    int i = myActivityList.find( anActivity );

    if ( i > 0 && next() ) {
	const Point p1( myActivityList.first()->topCenter() );
	const Point p2( myActivityList.last()->topCenter() );
	myNode->moveTo( (p1.x() + p2.x()) / 2. - radius(), next()->findSrcPoint().y() );
	const Point dst = myNode->center(); // 

	myArcList[i]->moveSrc( src );
	myArcList[i]->moveDst( dst );
	const Point p3 = myArcList[i]->dstIntersectsCircle( dst, radius() );
	myArcList[i]->moveDst( p3 );

	next()->moveSrcTo( myNode->bottomCenter() );
    }

    return *this; 
} 


AndOrJoinActivityList&
AndOrJoinActivityList::moveDstTo( const Point& dst, Activity * )
{ 
    myNode->moveTo( dst.x() - radius(), dst.y() - 2 * radius() );
    const Point ctr( myNode->center() );
    for ( unsigned int i = 1; i <= size(); ++i ) {
	myArcList[i]->moveDst( ctr );
	const Point dst2 = myArcList[i]->srcIntersectsCircle( ctr, radius() );
	myArcList[i]->moveDst( dst2 );
    }
    
    return *this; 
} 

ostream&
AndOrJoinActivityList::draw( ostream& output ) const
{
    if ( drawn() ) return output;

    ForkJoinActivityList::draw( output );

    Sequence<Arc *> nextArc( myArcList );
    Arc * anArc;

    while ( anArc = nextArc() ) {
	output << *anArc;
    }

    return output;
}

/* -------------------------------------------------------------------- */
/*                      Or Join Activity Lists                          */
/* -------------------------------------------------------------------- */

OrJoinActivityList&
OrJoinActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );
    return *this;
}



unsigned
OrJoinActivityList::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * anEntry, const unsigned depth, const unsigned p, const double rate ) const
{
    if ( next() ) {
	return next()->findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    } else {
	return depth;
    }
}



/*
 * Return the sum of aFunc.
 */

double
OrJoinActivityList::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, aggregateFunc aFunc )  const
{
    if ( next() ) {
	return next()->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
	return 0.0;
    }
}



unsigned
OrJoinActivityList::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callFunc aFunc ) const
{
    if ( next() ) {
	return next()->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } else {
	return next_k;
    }
}

/* -------------------------------------------------------------------- */
/*                     And Join Activity Lists                          */
/* -------------------------------------------------------------------- */

AndJoinActivityList::AndJoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : AndOrJoinActivityList( owner, dom_activitylist ), 
      myJoinType(JOIN_NOT_DEFINED)
{
    myLabel = Label::newLabel();
    if ( myLabel ) {
	myLabel->justification( Flags::label_justification );
    }
    const LQIO::DOM::AndJoinActivityList * dom = dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(dom_activitylist);
    if ( dom && dom->hasQuorumCount() ) {
	char buf[10];
	sprintf( buf, "%d", dom->getQuorumCountValue() );
	myTypeStr = buf;
    } else {
	myTypeStr = "&";		/* Can be changed if quorum is set. */
    }
}


AndJoinActivityList::~AndJoinActivityList()
{
    if ( myLabel ) {
	delete myLabel;
    }
}



AndJoinActivityList * 
AndJoinActivityList::clone() const
{
    AndJoinActivityList * newList = new AndJoinActivityList( owner(), new LQIO::DOM::AndJoinActivityList( *dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(getDOM()) ) );
    newList->quorumCount( quorumCount() );
    return newList;
}


AndJoinActivityList&
AndJoinActivityList::add( Activity * anActivity )
{
    Activity::hasJoins = true;

    ForkJoinActivityList::add( anActivity );
    myForkList.grow(1);
    myForkList[myForkList.size()] = 0;
    myEntryList.grow(1);
    myEntryList[myEntryList.size()] = 0;

    return *this;
}


bool
AndJoinActivityList::joinType( const join_type aType ) 
{
    if ( myJoinType == JOIN_NOT_DEFINED ) {
	myJoinType = aType;
	return true;
    } else {
	return aType == myJoinType;
    }
}

    

const char * 
AndJoinActivityList::typeStr() const
{
    return myTypeStr.c_str();
}


int
AndJoinActivityList::quorumCount() const
{ 
    const LQIO::DOM::AndJoinActivityList * dom = dynamic_cast<const LQIO::DOM::AndJoinActivityList * >(getDOM());
    return dom->getQuorumCountValue(); 
}

/*
 * Set quorumCount and adjust the typeStr used to fill icons when the model is drawn.
 */

AndJoinActivityList& 
AndJoinActivityList::quorumCount(int quorumCount)
{ 
    const LQIO::DOM::AndJoinActivityList * dom = dynamic_cast<const LQIO::DOM::AndJoinActivityList * >(getDOM());
    const_cast<LQIO::DOM::AndJoinActivityList *>(dom)->setQuorumCountValue( quorumCount ); 
    if ( quorumCount > 0 && graphical_output() ) {
	char buf[10];
	sprintf( buf, "%d", quorumCount );
	myTypeStr = buf;
    } else {
	myTypeStr = "&";
    }
    return *this; 
}


unsigned
AndJoinActivityList::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * anEntry, const unsigned depth, const unsigned p, const double rate ) const
{
    /* Aggregating activities */
    myDepth = max( myDepth, depth );

    /* Look for the fork on the fork stack */

    for ( unsigned i = 1; i <= myActivityList.size(); ++i ) {

	if ( myActivityList[i] != activityStack.top() ) {
	    unsigned j = myActivityList[i]->backtrack( forkStack );
	    if ( j ) {
		if ( !const_cast<AndJoinActivityList *>(this)->joinType( INTERNAL_FORK_JOIN  ) ) {
		    throw bad_internal_join( activityStack );
		} else if ( !myForkList[i] || forkStack.find( myForkList[i] ) ) {
		    const AndForkActivityList * aFork = forkStack[j];
		    const_cast<AndForkActivityList *>(aFork)->myJoinList = this;		/* Random choice :-) */
		    myForkList[i] = aFork;
		}
	    } else if ( !const_cast<AndJoinActivityList *>(this)->joinType( SYNCHRONIZATION_POINT ) ) {
		throw bad_internal_join( activityStack );
	    } else if ( !addToEntryList( i, anEntry ) ) {
		throw bad_internal_join( activityStack );
	    }
	}
    }

    if ( next() ) {
	return next()->findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    } else {
	return myDepth;
    }
}


/*
 * Add anEntry to the entry list provided it isn't there already and
 * the slot that it is to go in isn't already occupied.
 */

bool
AndJoinActivityList::addToEntryList( unsigned i, Entry * anEntry ) const
{
    if ( myEntryList[i] != 0 && myEntryList[i] != anEntry ) {
	return false;
    } else {
	myEntryList[i] = anEntry;
    }

    for ( unsigned j = 1; j <= myEntryList.size(); ++j ) {
	if ( j != i && myEntryList[j] == anEntry ) return false;
    }
    return true;
}



/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.  
 * Otherwise, press on.
 */

double
AndJoinActivityList::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, aggregateFunc aFunc ) const
{
    if ( isSynchPoint() && next() ) {
	return next()->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else if ( aFunc == &Activity::aggregateReplies && quorumCount() > 0 ) {
	const_cast<Entry *>(anEntry)->phaseIsPresent( 2, true );
    } 

    return 0.0;
}



unsigned
AndJoinActivityList::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callFunc aFunc ) const
{
    if ( isSynchPoint() && next() ) {
	return next()->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } else {
	return next_k;
    }
}



double 
AndJoinActivityList::joinDelay() const
{
    const LQIO::DOM::AndJoinActivityList * dom = dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(getDOM());
    return dom ? dom->getResultJoinDelay() : 0.0;
}


double 
AndJoinActivityList::joinVariance() const
{
    const LQIO::DOM::AndJoinActivityList * dom = dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(getDOM());
    return dom ? dom->getResultVarianceJoinDelay() : 0.0;
}


AndJoinActivityList&
AndJoinActivityList::moveSrcTo( const Point& src, Activity * anActivity )
{
    AndOrJoinActivityList::moveSrcTo( src, anActivity );
    myLabel->moveTo( myNode->center() ).moveBy( radius(), 0.0 ).justification( LEFT_JUSTIFY );
    return *this;
}



AndJoinActivityList& 
AndJoinActivityList::translateY( const double dy )
{
    ForkJoinActivityList::translateY( dy );
    myLabel->translateY( dy );
    return *this;
}



AndJoinActivityList& 
AndJoinActivityList::scaleBy( const double sx, const double sy )
{
    ForkJoinActivityList::scaleBy( sx, sy );
    myLabel->scaleBy( sx, sy );
    return *this;
}



AndJoinActivityList&
AndJoinActivityList::label()
{
    if ( !labelled() && Flags::have_results && Flags::print[JOIN_DELAYS].value.b ) {
	*myLabel << begin_math() << joinDelay() << end_math();
    }
    return *this;
}



ostream&
AndJoinActivityList::draw( ostream& output ) const
{
    if ( drawn() ) return output;

    AndOrJoinActivityList::draw( output );

    output << *myLabel;
    for ( unsigned int i = 1; i <= size(); ++i ) {
	output << *(myArcList[i]);
    }
    return output;
}

/*----------------------------------------------------------------------*/
/*                           Repetition node.                           */
/*----------------------------------------------------------------------*/

RepeatActivityList::RepeatActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : ForkActivityList( owner, dom_activitylist ), 
      prevLink(0), iHaveBeenLabelled(false), iHaveBeenDrawn(false)
{
    myNode = Node::newNode( 12.0, 12.0 );
}


RepeatActivityList::~RepeatActivityList()
{
    delete myNode;
    myLabelList.deleteContents();
    myArcList.deleteContents();
}



RepeatActivityList * 
RepeatActivityList::clone() const 
{ 
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new RepeatActivityList( owner(), new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType(), 0  ) ); 
} 



RepeatActivityList&
RepeatActivityList::label()
{
    if ( !labelled() && Flags::print[INPUT_PARAMETERS].value.b ) {
	for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	    const LQIO::DOM::ExternalVariable * var = rateBranch(i);
	    if ( var ) {
		*(myLabelList[i]) << *var;
	    } 
	}
    }
    return *this;
}



/*
 * Return the sum of aFunc.
 * Rate is set to zero for branches so that we don't count replies.
 */

double
RepeatActivityList::aggregate( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, aggregateFunc aFunc ) const
{
    double sum = ForkActivityList::aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
	unsigned branch_p = curr_p;
	sum += anActivity->aggregate( anEntry, curr_p, branch_p, i == 1 ? 1: 0.0, activityStack, aFunc );
    }
    return sum;
}




unsigned
RepeatActivityList::setChain( Stack<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callFunc aFunc ) const
{
    next_k = ForkActivityList::setChain( activityStack, curr_k, next_k, aServer, aFunc );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	next_k = anActivity->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    }

    return next_k;
}



/*
 * Add a sublist.  
 */
 
RepeatActivityList&
RepeatActivityList::add( Activity * anActivity )
{
    const LQIO::DOM::Activity * dom = dynamic_cast<const LQIO::DOM::Activity *>(anActivity->getDOM());
    if ( dom ) {
//	const LQIO::DOM::ExternalVariable * arg = getDOM()->getParameter(dom);

	myActivityList << anActivity;

	Label * aLabel = Label::newLabel();
	if ( aLabel ) {
	    myLabelList << aLabel;
	    aLabel->justification( Flags::label_justification );
	}

	Arc * anArc = Arc::newArc();
	if ( anArc ) {
	    anArc->linestyle( Graphic::DASHED );
	    myArcList << anArc;
	}

    } else {

	/* End of list */

	ForkActivityList::add( anActivity );
    }

    return *this;
}

LQIO::DOM::ExternalVariable *
RepeatActivityList::rateBranch( const unsigned int i ) const
{
    return getDOM()->getParameter(dynamic_cast<const LQIO::DOM::Activity *>(myActivityList[i]->getDOM()));
}


unsigned 
RepeatActivityList::size() const
{
    return ForkActivityList::size() + myActivityList.size();
}



unsigned
RepeatActivityList::findChildren( CallStack& callStack, const unsigned directPath, Stack<const Activity *>& activityStack ) const
{
    int nextLevel = ForkActivityList::findChildren( callStack, directPath, activityStack );

    for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	Activity * anActivity = myActivityList[i];		/* Skip non-interesting branches */
	nextLevel = max( anActivity->findChildren( callStack, directPath, activityStack ), nextLevel );
    } 
    return nextLevel;
}


unsigned
RepeatActivityList::findActivityChildren( Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack, Entry * anEntry, const unsigned depth, const unsigned p, const double rate ) const
{
    unsigned nextLevel = ForkActivityList::findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    const unsigned size = owner()->activities().size();
    for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	Stack<const AndForkActivityList *> branchForkStack( size ); 	// For matching forks/joins.
	nextLevel = max( myActivityList[i]->findActivityChildren( activityStack, branchForkStack, anEntry, depth, 
								  p, /* rateBranch(i) * */ rate ), nextLevel );
    } 
    return nextLevel;
}


double
RepeatActivityList::getIndex() const
{
    return ForkActivityList::getIndex();
}


RepeatActivityList& 
RepeatActivityList::scaleBy( const double sx, const double sy )
{
    myArc->scaleBy( sx, sy );
    for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	myArcList[i]->scaleBy( sx, sy );
	myLabelList[i]->scaleBy( sx, sy );
    }
    if ( myActivityList.size() ) {
	myNode->scaleBy( sx, sy );
    }
    return *this;
}



RepeatActivityList& 
RepeatActivityList::translateY( const double dy )
{
    myArc->translateY( dy );
    for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	myArcList[i]->translateY( dy );
	myLabelList[i]->translateY( dy );
    }
    if ( myActivityList.size() ) {
	myNode->translateY( dy );
    }
    return *this;
}



RepeatActivityList& 
RepeatActivityList::depth( const unsigned curDepth )
{
    myArc->depth( curDepth );
    for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	myArcList[i]->depth( curDepth );
	myLabelList[i]->depth( curDepth );
    }
    if ( myActivityList.size() ) {
	myNode->depth( curDepth );
    }
    return *this;
}


Point
RepeatActivityList::findSrcPoint() const
{
    if ( myActivityList.size() > 1 ) {
	Point p1( myActivityList.first()->topCenter() );
	Point p2( myActivityList.last()->topCenter() );
	return Point( (p1.x() + p2.x()) / 2.0, p1.y() + height() );
    } else if ( myActivityList.size() == 1 ) {
	Point p2( myActivityList.first()->topCenter() );
	if ( myActivity ) {
	    Point p1( myActivity->topCenter() );
	    return Point( (p1.x() + p2.x()) / 2.0, p1.y() + height() );
	} else {
	    return Point( p2.x(), p2.y() + height() );
	}
    } else {
	return ForkActivityList::findSrcPoint();
    }
}

/*
 * Called from JoinList::moveTo().
 */

RepeatActivityList&
RepeatActivityList::moveSrcTo( const Point& src, Activity * anActivity )
{ 
    myNode->moveTo( src.x() - radius(), src.y() - 2 * radius());
    const Point ctr( myNode->center() );
    myArc->moveSrc( ctr );
    const Point src2 = myArc->srcIntersectsCircle( ctr, radius() );
    myArc->moveSrc( src2 );
    
    /* Now move the arc for the sub activity */

    for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	myArcList[i]->moveSrc( src );
	const Point src3 = myArcList[i]->srcIntersectsCircle( src, radius() );
	myArcList[i]->moveSrc( src3 );
	const Point aPoint = myArcList[i]->pointFromDst(height()/3.0);
	myLabelList[i]->moveTo( aPoint );
    }

    return *this; 
} 


RepeatActivityList&
RepeatActivityList::moveDstTo( const Point& dst, Activity * anActivity )
{ 
    if ( anActivity == myActivity ) {
	myArc->moveDst( dst );
    } else {
	const int i = myActivityList.find( anActivity );

	if ( i > 0 ) {
	    myArcList[i]->moveDst( dst );
	}
    }
    return *this;
}


double
RepeatActivityList::radius() const
{
    return fabs( myNode->extent.y() ) / 2.0;
}



ostream&
RepeatActivityList::draw( ostream& output ) const
{
    if ( testAndSetDrawn() ) return output;

    ForkActivityList::draw( output );
    for ( unsigned int i = 1; i <= myActivityList.size(); ++i ) {
	myArcList[i]->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() );
	output << *(myArcList[i]);
	output << *(myLabelList[i]);
    }

    const Point ctr( myNode->center() );
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    myNode->circle( output, ctr, radius() );
    Point aPoint( ctr.x(), (ctr.y() + myNode->bottom()) / 2. );
    myNode->text( output, aPoint, typeStr() );

    return output;
}

/* ------------------------ Exception Handling ------------------------ */

bad_internal_join::bad_internal_join( const Stack<const Activity *>& activityStack )
    : path_error( activityStack.size() )
{
    const Activity * anActivity = activityStack.top();
    myMsg = anActivity->name();
    for ( unsigned i = activityStack.size() - 1; i > 0; --i ) {
        myMsg += ", ";
        myMsg += activityStack[i]->name();
    }
}

/* ---------------------------------------------------------------------- */

/*
 * Connect the src and dst lists together.
 */

void
ActivityList::act_connect( ActivityList * src, ActivityList * dst )
{
    if ( src ) {
        src->next( dst  );
    }
    if ( dst ) {
        dst->prev( src );
    }
}


ostream&
ActivityList::newline( ostream& output )
{
    if ( first ) {
	output << ':';
    } else {
	output << ';';
    }
    first = false;
    output << endl << "  ";
    return output;
}

