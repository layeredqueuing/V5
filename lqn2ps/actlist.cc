/* actlist.cc	-- Greg Franks Thu Apr 10 2003
 *
 * Connections between activities are activity lists.  For srvn output, 
 * this is all the stuff printed after the ':'.  For xml output, this
 * is all of the precendence stuff.
 * 
 * $Id: actlist.cc 15555 2022-04-18 21:06:51Z greg $
 */


#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <lqio/error.h>
#include <lqio/dom_actlist.h>
#include "actlist.h"
#include "task.h"
#include "entry.h"
#include "activity.h"
#include "errmsg.h"
#include "label.h"
#include <string>

template <> struct ExecXY<Arc>
{
    typedef Arc& (Arc::*funcPtrXY)( double x, double y );
    ExecXY<Arc>( funcPtrXY f, double x, double y ) : _f(f), _x(x), _y(y) {};
    void operator()( const std::pair<Activity *,Arc *>& object ) const { (object.second->*_f)( _x, _y ); }
private:
    funcPtrXY _f;
    double _x;
    double _y;
};

template <> struct ExecXY<Label>
{
    typedef Label& (Label::*funcPtrXY)( double x, double y );
    ExecXY<Label>( funcPtrXY f, double x, double y ) : _f(f), _x(x), _y(y) {};
    void operator()( const std::pair<Activity *,Label *>& object ) const { (object.second->*_f)( _x, _y ); }
private:
    funcPtrXY _f;
    double _x;
    double _y;
};


class ActivityListManip {
public:
    ActivityListManip( std::ostream& (*ff)( std::ostream& ) )
	: f(ff) {}
private:
    std::ostream& (*f)( std::ostream& );

    friend std::ostream& operator<<(std::ostream & os, const ActivityListManip& m ) 
	{ return m.f(os); }
};


bool ActivityList::first = true;

/* -------------------------------------------------------------------- */
/*               Activity Lists -- Abstract Superclass                  */
/* -------------------------------------------------------------------- */

ActivityList::ActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom )
    : _owner(owner), _dom(dom) 
{
    if ( owner ) {
	const_cast<Task *>(owner)->addPrecedence(this);
    }
}

ActivityList::~ActivityList()
{
}


ActivityList&
ActivityList::setOwner( const Task * owner ) 
{
    _owner = owner;
    return *this;
}



/*
 * Return the next link.
 */
 
ActivityList*
ActivityList::next() const
{
    throw should_not_implement( "ActivityList::next", __FILE__, __LINE__ );
    return nullptr;
}


/*
 * Return the next link.
 */
 
ActivityList*
ActivityList::prev() const
{
    throw should_not_implement( "ActivityList::prev", __FILE__, __LINE__ );
    return nullptr;
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


Graphic::Colour 
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



const SequentialActivityList&
SequentialActivityList::draw( std::ostream& output ) const
{
    if ( myActivity && myArc->srcPoint() != myArc->secondPoint() ) {
	myArc->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() );
	myArc->draw( output );
    }
    return *this;
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
    return new ForkActivityList( nullptr, new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType() ) );
}



size_t
ForkActivityList::findChildren( CallStack& callStack, const unsigned directPath, std::deque<const Activity *>& activityStack ) const
{
    if ( myActivity ) {
	return myActivity->findChildren( callStack, directPath, activityStack );
    } else {
	return callStack.size();
    }
}

size_t
ForkActivityList::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * anEntry, size_t depth, const unsigned p, const double rate ) const
{
    if ( myActivity ) {
	return myActivity->findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    } else {
	return depth;
    }
}



/*
 * Return the sum of aFunc.
 */

double
ForkActivityList::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, aggregateFunc aFunc ) 
{
    if ( myActivity ) {
	return myActivity->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
	return 0.0;
    }
}


unsigned
ForkActivityList::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callPredicate aFunc ) const
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
	return std::numeric_limits<double>::max();
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
    return new JoinActivityList( nullptr, new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType() ) ); 
}



size_t
JoinActivityList::findChildren( CallStack& callStack, const unsigned directPath, std::deque<const Activity *>& activityStack ) const
{
    if ( next() ) {
	return next()->findChildren( callStack, directPath, activityStack );
    } else {
	return callStack.size();
    }
}


size_t
JoinActivityList::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * anEntry, size_t depth, const unsigned p, const double rate ) const
{
    size_t nextLevel = depth;
    if ( next() ) {
	nextLevel = next()->findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    }
    return nextLevel;
}


/*
 * Return the sum of aFunc.  When aggregating service time, collapse
 * strings of sequential activities.
 */

double
JoinActivityList::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, aggregateFunc aFunc ) 
{
    if ( next() ) {
	double count = next()->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
	if ( aFunc == &Activity::aggregateService
	     && (Flags::aggregation() == Aggregate::SEQUENCES
		 || Flags::aggregation() == Aggregate::THREADS)
	     && dynamic_cast<ForkActivityList *>(next())
	     && !dynamic_cast<RepeatActivityList *>(next()) ) {

	    /* Simple sequence -- aggregate */
	    
	    Activity * nextActivity = dynamic_cast<ForkActivityList *>(next())->myActivity;
	    myActivity->merge( *nextActivity ).disconnect( nextActivity );
	    delete nextActivity;
	} 
	return count;
    } else {
	return 0.0;
    }
}


unsigned
JoinActivityList::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callPredicate aFunc ) const
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
	return std::numeric_limits<double>::max();
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
	    myArc->arrowhead( Graphic::ArrowHead::NONE );
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
    : ActivityList( owner, dom_activitylist )
{
    myNode = Node::newNode( 12.0, 12.0 );
}


ForkJoinActivityList::~ForkJoinActivityList()
{
    delete myNode;
    _activityList.clear();
    for ( std::map<Activity *,Arc *>::iterator arc = myArcList.begin(); arc != myArcList.end(); ++arc ) {
	delete arc->second;
    }
}


/*
 * Add an item to the activity list.
 */

ForkJoinActivityList&
ForkJoinActivityList::add( Activity * anActivity )
{
    _activityList.push_back( anActivity );
    myArcList[anActivity] = Arc::newArc();
    return *this;
}


ForkJoinActivityList& 
ForkJoinActivityList::scaleBy( const double sx, const double sy )
{
    for_each( myArcList.begin(), myArcList.end(), ExecXY<Arc>( &Arc::scaleBy, sx, sy ) );
    myNode->scaleBy( sx, sy );
    return *this;
}



ForkJoinActivityList& 
ForkJoinActivityList::translateY( const double dy )
{
    for_each( myArcList.begin(), myArcList.end(), ExecX<Arc,std::pair<Activity *,Arc *>,double>( &Arc::translateY, dy ) );
    myNode->translateY( dy );
    return *this;
}



ForkJoinActivityList& 
ForkJoinActivityList::depth( unsigned depth )
{
    for_each( myArcList.begin(), myArcList.end(), ExecX<Graphic,std::pair<Activity *, Arc *>,unsigned>( &Graphic::depth, depth ) );
    myNode->depth( depth-2 );
    return *this;
}


Point
ForkJoinActivityList::findSrcPoint() const
{
    Point p1( activityList().front()->topCenter() );
    Point p2( activityList().back()->topCenter() );
    return Point( (p1.x() + p2.x()) / 2.0, p1.y() + height() );
}

Point
ForkJoinActivityList::findDstPoint() const
{
    Point p1( activityList().front()->bottomCenter() );
    Point p2( activityList().back()->bottomCenter() );
    return Point( (p1.x() + p2.x()) / 2, p1.y() - height() );
}


double
ForkJoinActivityList::radius() const
{
    return fabs( myNode->height() ) / 2.0;
}


const ForkJoinActivityList& 
ForkJoinActivityList::draw( std::ostream& output ) const
{
    const Graphic::Colour pen_colour = colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour();
    for_each( myArcList.begin(), myArcList.end(), ExecX<Graphic,std::pair<Activity *, Arc *>,Graphic::Colour>( &Graphic::penColour, pen_colour ) );
    const Point ctr( myNode->center() );
    myNode->penColour( pen_colour ).fillColour( colour() );
    myNode->circle( output, ctr, radius() );
    Point aPoint( ctr.x(), (ctr.y() + myNode->bottom()) / 2. );

    myNode->text( output, aPoint, typeStr() );

    return *this;
}

ForkJoinActivityList& ForkJoinActivityList::sort( compare_func_ptr compare )
{
    std::sort( _activityList.begin(), _activityList.end(), compare );
    return *this;
}



/*
 * Construct a list name.
 */

std::string 
ForkJoinActivityList::getName() const
{
    std::string name;
    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	if ( activity != activityList().begin() ) {
	    name += ' ';
	    name += typeStr();
	    name += ' ';
	}
	name += (*activity)->name();
    }
    return name;
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
    return new OrForkActivityList( nullptr, new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType() ) ); 
}

size_t
AndOrForkActivityList::findChildren( CallStack& callStack, const unsigned directPath, std::deque<const Activity *>& activityStack ) const
{
    size_t nextLevel = callStack.size();
    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	nextLevel = std::max( (*activity)->findChildren( callStack, directPath, activityStack ), nextLevel );
    } 
    return nextLevel;
}



double
AndOrForkActivityList::getIndex() const
{
    if ( prev() ) {
	return prev()->getIndex();
    } else {
	return std::numeric_limits<double>::max();
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
    // std::cerr << "AndOrForkActivityList::moveSrcTo( " << src.x() << ", " << src.y() << ")" << std::endl;
    myNode->moveTo( src.x() - radius(), src.y() - 2 * radius() );
    const Point ctr( myNode->center() );
    for ( std::map<Activity *,Arc *>::iterator arc = myArcList.begin(); arc != myArcList.end(); ++arc ) {
	arc->second->moveSrc( ctr );
	const Point src2 = arc->second->srcIntersectsCircle( ctr, radius() );
	arc->second->moveSrc( src2 );
    }
    
    return *this; 
} 


AndOrForkActivityList&
AndOrForkActivityList::moveDstTo( const Point& dst, Activity * anActivity )
{ 
    const std::map<Activity *,Arc *>::const_iterator arc = myArcList.find( anActivity );

    if ( arc != myArcList.end() && prev() ) {
	const Point p1( activityList().front()->topCenter() );
	const Point p2( activityList().back()->topCenter() );
	// std::cerr << "AndOrForkActivityList::moveDstTo( "
	// 	  << (p1.x() + p2.x()) / 2. - radius() << ", "
	// 	  << prev()->findSrcPoint().y() << ")" << std::endl;
	myNode->moveTo( (p1.x() + p2.x()) / 2. - radius(), prev()->findSrcPoint().y() );
	const Point src = myNode->center(); // 

	arc->second->moveDst( dst );
	arc->second->moveSrc( src );
	const Point p3 = arc->second->dstIntersectsCircle( src, radius() );
	arc->second->moveSrc( p3 );

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

const AndOrForkActivityList&
AndOrForkActivityList::draw( std::ostream& output ) const
{
    ForkJoinActivityList::draw( output );
    for_each( myArcList.begin(), myArcList.end(), ConstExecX<Arc,std::pair<Activity *,Arc *>,std::ostream&>( &Arc::draw, output ) );
    return *this;
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
    return new OrJoinActivityList( nullptr, new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType() ) ); 
} 


OrForkActivityList::~OrForkActivityList()
{
    for ( std::map<Activity *, Label *>::iterator label = _labelList.begin(); label != _labelList.end(); ++label ) {
	delete label->second;
    }
}


const LQIO::DOM::ExternalVariable& 
OrForkActivityList::prBranch( const Activity * anActivity ) const
{
    return *getDOM()->getParameter(dynamic_cast<const LQIO::DOM::Activity *>(anActivity->getDOM()));
}


OrForkActivityList&
OrForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );
    Label * aLabel = Label::newLabel();
    if ( aLabel ) {
	_labelList[anActivity] = aLabel;
	aLabel->justification( Flags::label_justification );
    }
    return *this;
}


/*
 * Aggregation here....  Try to figure out if this or fork is aggregatable.	
 */

size_t
OrForkActivityList::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * anEntry, size_t depth, const unsigned p, const double rate ) const
{
    size_t nextLevel = depth;

    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	nextLevel = std::max( (*activity)->findActivityChildren( activityStack, forkStack, anEntry, depth, p, /* prBranch(i) * */ rate ), nextLevel );
	/* sum += prBranch(i); */
    } 

    // if ( fabs( 1.0 - sum ) > EPSILON ) {
    // 	ForkJoinName aName( *this );
    // 	LQIO::solution_error( LQIO::ERR_MISSING_OR_BRANCH, aName(), owner()->name().c_str(), sum );
    // }

    return nextLevel;
}



/*
 * Return the sum of aFunc.  For service time aggregation, we're
 * trying to get rid of the or-forks.  After aggregating the branches,
 * if we end up with a fork, activity, join, then we can smash them
 * all together. Needless to say, this is all recursize.
 */

double
OrForkActivityList::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, aggregateFunc aFunc )
{
    double sum = 0.0;
    next_p = curr_p;
    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	unsigned branch_p = curr_p;
	double prob = 0.0;
	const LQIO::DOM::ExternalVariable& pr_branch = prBranch(*activity);
	if ( pr_branch.wasSet() ) {
	    pr_branch.getValue( prob );
	} else {
	    prob = 1.0 / activityList().size();
	}
	sum += (*activity)->aggregate( anEntry, curr_p, branch_p, rate * prob, activityStack, aFunc );
	next_p = std::max( next_p, branch_p );
    }

    /* 
     * Try to figure out if we can collapse this or-fork.  It amounts
     * to ensuring that there is only one activity between the fork
     * and the join, and the lists are the same size.
     */

    ActivityList * joinList = activityList().front()->outputTo();

    if ( aFunc == &Activity::aggregateService
	 && joinList->size() == size()
	 && Flags::aggregation() == Aggregate::THREADS
	 && dynamic_cast<OrJoinActivityList *>(joinList)
	 && dynamic_cast<ForkActivityList *>(joinList->next())
	 && !dynamic_cast<RepeatActivityList *>(joinList->next()) ) {

	for ( std::vector<Activity *>::const_iterator activity = activityList().begin() + 1; activity != activityList().end(); ++activity ) {
	    if ( !dynamic_cast<OrJoinActivityList *>((*activity)->outputTo()) || (*activity)->outputTo() != joinList ) {
		return sum;
	    }
	} 

	/* Success */

	/* Aggregate everything into joinList->next()->myActivity */

	Activity * nextActivity = dynamic_cast<ForkActivityList *>(joinList->next())->myActivity;
	size_t currLevel = 0;
	for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	    currLevel = std::max( currLevel, (*activity)->level() );
	    double prob;
	    const LQIO::DOM::ExternalVariable& pr_branch = prBranch(*activity);
	    if ( pr_branch.wasSet() ) {
		pr_branch.getValue(prob);
		nextActivity->merge( *(*activity), prob );
	    }
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

	for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	    const_cast<Task *>(nextActivity->owner())->removeActivity( (*activity) );
	}
    }
    return sum;
}



unsigned
OrForkActivityList::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callPredicate aFunc ) const
{
    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	next_k = (*activity)->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } 
    return next_k;
}



OrForkActivityList& 
OrForkActivityList::translateY( const double dy )
{
    ForkJoinActivityList::translateY( dy );
    for_each( _labelList.begin(), _labelList.end(), ExecX<Label,std::pair<Activity *,Label *>,double>( &Label::translateY, dy ) );
    return *this;
}


OrForkActivityList& 
OrForkActivityList::scaleBy( const double sx, const double sy )
{
    ForkJoinActivityList::scaleBy( sx, sy );
    for_each( _labelList.begin(), _labelList.end(), ExecXY<Label>( &Label::scaleBy, sx, sy ) );
    return *this;
}



OrForkActivityList&
OrForkActivityList::label()
{
    if ( Flags::print_input_parameters() ) {
	for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	    *(_labelList[*activity]) << prBranch( *activity );
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
	_labelList[i]->moveTo( aPoint );
    }
#endif
    return *this;
}



OrForkActivityList&
OrForkActivityList::moveDstTo( const Point& dst, Activity * anActivity )
{
    AndOrForkActivityList::moveDstTo( dst, anActivity );
    for ( std::map<Activity *,Arc *>::iterator arc = myArcList.begin(); arc != myArcList.end(); ++arc ) {
	Point aPoint = arc->second->pointFromDst(height()/3.0);
	_labelList[arc->first]->moveTo( aPoint );
    }
    return *this;
}



const OrForkActivityList&
OrForkActivityList::draw( std::ostream& output ) const
{
    AndOrForkActivityList::draw( output );
    for_each( myArcList.begin(), myArcList.end(), ConstExecX<Arc,std::pair<Activity *,Arc *>,std::ostream&>( &Arc::draw, output ) );
    for_each( _labelList.begin(), _labelList.end(), ConstExecX<Label,std::pair<Activity *,Label *>,std::ostream&>( &Label::draw, output ) );
    return *this;
}

/* -------------------------------------------------------------------- */
/*                      And Fork Activity Lists                         */
/* -------------------------------------------------------------------- */



AndForkActivityList *
AndForkActivityList::clone() const 
{ 
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new AndForkActivityList( nullptr, new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType() ) ); 
} 



AndForkActivityList&
AndForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );
    return *this;

}



size_t
AndForkActivityList::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * anEntry, size_t depth, const unsigned p, const double rate ) const
{
    size_t nextLevel = depth;

    forkStack.push_back( this );
    try { 
	for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	    nextLevel = std::max( (*activity)->findActivityChildren( activityStack, forkStack, anEntry, depth, p, /* prBranch(i) * */ rate ), nextLevel );
	}
    }
    catch ( const bad_internal_join& error ) {
	LQIO::solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, owner()->name().c_str(), error.what(), getName().c_str() );
    }
    forkStack.pop_back();
    return nextLevel;
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

void
AndForkActivityList::backtrack( const std::deque<const AndForkActivityList *>& forkStack, std::set<const AndForkActivityList *>& forkSet, std::set<const AndOrJoinActivityList *>& joinSet ) const
{
    if ( std::find( forkStack.begin(), forkStack.end(), this ) != forkStack.end() ) forkSet.insert( this );
    prev()->backtrack( forkStack, forkSet, joinSet );
}



/*
 * Return the sum of aFunc.
 */

double
AndForkActivityList::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, aggregateFunc aFunc )
{
    double sum = 0.0;

    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	unsigned branch_p = curr_p;
	sum += (*activity)->aggregate( anEntry, curr_p, branch_p, rate, activityStack, aFunc );
	next_p = std::max( next_p, branch_p );
    } 

    /* Now follow the activities after the join */

    if ( myJoinList && myJoinList->next() ) {
	sum += myJoinList->next()->aggregate( anEntry, next_p, next_p, rate, activityStack, aFunc );
    }
    return sum;
}


/*
 * Set the chains.  Since this is a fork, we create a new chain for
 * each branch.  The activities that follow the join are part of the
 * original chain.
 */

unsigned
AndForkActivityList::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callPredicate aFunc ) const
{
    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	next_k += 1;
	next_k = (*activity)->setChain( activityStack, next_k, next_k, aServer, aFunc );
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

size_t
AndOrJoinActivityList::findChildren( CallStack& callStack, const unsigned directPath, std::deque<const Activity *>& activityStack ) const
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

void
AndOrJoinActivityList::backtrack( const std::deque<const AndForkActivityList *>& forkStack, std::set<const AndForkActivityList *>& forkSet, std::set<const AndOrJoinActivityList *>& joinSet ) const
{
    if ( std::find( joinSet.begin(), joinSet.end(), this ) != joinSet.end() ) return;	/* cycle in graph */
    joinSet.insert( this );
    for_each ( activityList().begin(), activityList().end(), ConstExec3<Activity,const std::deque<const AndForkActivityList *>&,std::set<const AndForkActivityList *>&,std::set<const AndOrJoinActivityList *>&>( &Activity::backtrack, forkStack, forkSet, joinSet ) );
}



double
AndOrJoinActivityList::getIndex() const
{
    double anIndex = std::numeric_limits<double>::max();
    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	anIndex = std::min( anIndex, (*activity)->index() );
    }
    return anIndex;
}



AndOrJoinActivityList&
AndOrJoinActivityList::reconnect( Activity * curr, Activity * next )
{
    ActivityList::reconnect( curr, next );
    std::vector<Activity *>::iterator activity = std::find( _activityList.begin(), _activityList.end(), curr );
    if ( activity != activityList().end() ) {
	*activity = next;
    }
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
    const std::map<Activity *,Arc *>::const_iterator arc = myArcList.find( anActivity );

    if ( arc != myArcList.end() && next() ) {
	const Point p1( activityList().front()->topCenter() );
	const Point p2( activityList().back()->topCenter() );
	myNode->moveTo( (p1.x() + p2.x()) / 2. - radius(), next()->findSrcPoint().y() );
	const Point dst = myNode->center(); // 

	arc->second->moveSrc( src );
	arc->second->moveDst( dst );
	const Point p3 = arc->second->dstIntersectsCircle( dst, radius() );
	arc->second->moveDst( p3 );

	next()->moveSrcTo( myNode->bottomCenter() );
    }

    return *this; 
} 


AndOrJoinActivityList&
AndOrJoinActivityList::moveDstTo( const Point& dst, Activity * )
{ 
    myNode->moveTo( dst.x() - radius(), dst.y() - 2 * radius() );
    const Point ctr( myNode->center() );
    for ( std::map<Activity *,Arc *>::iterator arc = myArcList.begin(); arc != myArcList.end(); ++arc ) {
	arc->second->moveDst( ctr );
	const Point dst2 = arc->second->srcIntersectsCircle( ctr, radius() );
	arc->second->moveDst( dst2 );
    }
    
    return *this; 
} 

const AndOrJoinActivityList&
AndOrJoinActivityList::draw( std::ostream& output ) const
{
    ForkJoinActivityList::draw( output );
    for_each( myArcList.begin(), myArcList.end(), ConstExecX<Arc,std::pair<Activity *,Arc *>,std::ostream&>( &Arc::draw, output ) );
    return *this;
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



size_t
OrJoinActivityList::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * anEntry, size_t depth, const unsigned p, const double rate ) const
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
OrJoinActivityList::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, aggregateFunc aFunc )
{
    if ( next() ) {
	return next()->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
	return 0.0;
    }
}



unsigned
OrJoinActivityList::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callPredicate aFunc ) const
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
      _label(nullptr),
      _typeStr(),
      _joinType(JoinType::NOT_DEFINED),
      _forkList(nullptr),
      _depth(0)
{
    _label = Label::newLabel();
    if ( _label ) {
	_label->justification( Flags::label_justification );
    }
    const LQIO::DOM::AndJoinActivityList * dom = dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(dom_activitylist);
    if ( dom && dom->hasQuorumCount() ) {
        _typeStr = std::to_string( dom->getQuorumCountValue() );
    } else {
	_typeStr = "&";		/* Can be changed if quorum is set. */
    }
}


AndJoinActivityList::~AndJoinActivityList()
{
    if ( _label ) {
	delete _label;
    }
}



AndJoinActivityList * 
AndJoinActivityList::clone() const
{
    AndJoinActivityList * newList = new AndJoinActivityList( nullptr, new LQIO::DOM::AndJoinActivityList( *dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(getDOM()) ) );
    newList->quorumCount( quorumCount() );
    return newList;
}


AndJoinActivityList&
AndJoinActivityList::add( Activity * anActivity )
{
    Activity::hasJoins = true;
    ForkJoinActivityList::add( anActivity );
    return *this;
}


bool
AndJoinActivityList::joinType( const JoinType aType ) 
{
    if ( _joinType == JoinType::NOT_DEFINED ) {
	_joinType = aType;
	return true;
    } else {
	return aType == _joinType;
    }
}

    

const char * 
AndJoinActivityList::typeStr() const
{
    return _typeStr.c_str();
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
	_typeStr = std::to_string( quorumCount );
    } else {
	_typeStr = "&";
    }
    return *this; 
}


size_t
AndJoinActivityList::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * anEntry, size_t depth, const unsigned p, const double rate ) const
{
    /* Aggregating activities */
    _depth = std::max( _depth, depth );

    /* Look for the fork on the fork stack */

    if ( forkList() == nullptr ) {
	std::set<const AndForkActivityList *> resultSet(forkStack.begin(),forkStack.end());

	/* Go up all of the branches looking for forks found on forkStack */

	for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	    if ( *activity == activityStack.back() ) continue;		/* No need -- this is resultSet */
	
	    /* Find all forks from this activity that match anything in forkStack */
	
	    std::set<const AndForkActivityList *> branchSet;
	    std::set<const AndOrJoinActivityList *> joinSet;
	    (*activity)->backtrack( forkStack, branchSet, joinSet );			/* find fork lists on this branch */

	    /* Find intersection of branches */
	
	    std::set<const AndForkActivityList *> intersection;
	    std::set_intersection( branchSet.begin(), branchSet.end(),
				   resultSet.begin(), resultSet.end(),
				   std::inserter( intersection, intersection.end() ) );
	    resultSet = intersection;
	}

	/* Result should be all forks that match on all branches.  Take the one closest to top-of-stack */

	if ( resultSet.size() > 0 ) {
	    for ( std::deque<const AndForkActivityList *>::const_reverse_iterator fork_list = forkStack.rbegin(); fork_list != forkStack.rend() && forkList() == nullptr; ++fork_list ) {
		if ( resultSet.find( *fork_list ) == resultSet.end() ) continue;
	    
		if ( !const_cast<AndJoinActivityList *>(this)->joinType( JoinType::INTERNAL_FORK_JOIN  ) ) {
		    throw bad_internal_join( *this );
		}
		const_cast<AndForkActivityList *>(*fork_list)->myJoinList = this;		/* Random choice :-) */
		const_cast<AndJoinActivityList *>(this)->_forkList = *fork_list;
	    }
	} else if ( !const_cast<AndJoinActivityList *>(this)->joinType( JoinType::SYNCHRONIZATION_POINT ) ) {
	    throw bad_internal_join( *this );
	}
    }

    if ( next() ) {
	return next()->findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    } else {
	return _depth;
    }
}





/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.  
 * Otherwise, press on.
 */

double
AndJoinActivityList::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, aggregateFunc aFunc )
{
    if ( isSynchPoint() && next() ) {
	return next()->aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else if ( aFunc == &Activity::aggregateReplies && quorumCount() > 0 ) {
	const_cast<Entry *>(anEntry)->getPhase( 2 );
    } 

    return 0.0;
}



unsigned
AndJoinActivityList::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callPredicate aFunc ) const
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
    _label->moveTo( myNode->center() ).moveBy( radius(), 0.0 ).justification( Justification::LEFT );
    return *this;
}



AndJoinActivityList& 
AndJoinActivityList::translateY( const double dy )
{
    ForkJoinActivityList::translateY( dy );
    _label->translateY( dy );
    return *this;
}



AndJoinActivityList& 
AndJoinActivityList::scaleBy( const double sx, const double sy )
{
    ForkJoinActivityList::scaleBy( sx, sy );
    _label->scaleBy( sx, sy );
    return *this;
}



AndJoinActivityList&
AndJoinActivityList::label()
{
    if ( Flags::have_results && Flags::print[JOIN_DELAYS].opts.value.b ) {
	*_label << begin_math() << opt_pct(joinDelay()) << end_math();
    }
    return *this;
}



const AndJoinActivityList&
AndJoinActivityList::draw( std::ostream& output ) const
{
    AndOrJoinActivityList::draw( output );
    for_each( myArcList.begin(), myArcList.end(), ConstExecX<Arc,std::pair<Activity *,Arc *>,std::ostream&>( &Arc::draw, output ) );
    output << *_label;
    return *this;
}

/*----------------------------------------------------------------------*/
/*                           Repetition node.                           */
/*----------------------------------------------------------------------*/

RepeatActivityList::RepeatActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
    : ForkActivityList( owner, dom_activitylist ), 
      prevLink(0)
{
    myNode = Node::newNode( 12.0, 12.0 );
}


RepeatActivityList::~RepeatActivityList()
{
    delete myNode;
    for ( std::map<Activity *,Arc *>::iterator arc = myArcList.begin(); arc != myArcList.end(); ++arc ) {
	delete arc->second;
    }
    for ( std::map<Activity *,Label *>::iterator label = _labelList.begin(); label != _labelList.end(); ++label ) {
	delete label->second;
    }
}



RepeatActivityList * 
RepeatActivityList::clone() const 
{ 
    const LQIO::DOM::ActivityList& src = *getDOM();
    return new RepeatActivityList( nullptr, new LQIO::DOM::ActivityList( src.getDocument(), 0, src.getListType() ) ); 
} 



RepeatActivityList&
RepeatActivityList::label()
{
    if ( Flags::print_input_parameters() ) {
	for ( std::map<Activity *,Label *>::iterator label = _labelList.begin(); label != _labelList.end(); ++label ) {
	    const LQIO::DOM::ExternalVariable * var = rateBranch(label->first);
	    if ( var ) {
		*label->second << *var;
	    }
	}
    }
    return *this;
}



/*
 * Return the sum of aFunc.
 */

double
RepeatActivityList::aggregate( Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, std::deque<const Activity *>& activityStack, aggregateFunc aFunc )
{
    double sum = ForkActivityList::aggregate( anEntry, curr_p, next_p, rate, activityStack, aFunc );

    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	unsigned branch_p = curr_p;
	double mult = 1.0;
	const LQIO::DOM::ExternalVariable * var = rateBranch(*activity);
	if ( var && var->wasSet() ) {
	    var->getValue( mult );
	}
	sum += (*activity)->aggregate( anEntry, curr_p, branch_p, mult * rate, activityStack, aFunc );
    }
    return sum;
}




unsigned
RepeatActivityList::setChain( std::deque<const Activity *>& activityStack, unsigned curr_k, unsigned next_k, const Entity * aServer, const callPredicate aFunc ) const
{
    next_k = ForkActivityList::setChain( activityStack, curr_k, next_k, aServer, aFunc );

    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	next_k = (*activity)->setChain( activityStack, curr_k, next_k, aServer, aFunc );
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

	_activityList.push_back(anActivity);

	Label * aLabel = Label::newLabel();
	if ( aLabel ) {
	    _labelList[anActivity] = aLabel;
	    aLabel->justification( Flags::label_justification );
	}

	Arc * anArc = Arc::newArc();
	if ( anArc ) {
	    anArc->linestyle( Graphic::LineStyle::DASHED );
	    myArcList[anActivity] = anArc;
	}

    } else {

	/* End of list */

	ForkActivityList::add( anActivity );
    }

    return *this;
}

const LQIO::DOM::ExternalVariable *
RepeatActivityList::rateBranch( const Activity * anActivity ) const
{
    return getDOM()->getParameter(dynamic_cast<const LQIO::DOM::Activity *>(anActivity->getDOM()));
}


unsigned 
RepeatActivityList::size() const
{
    return ForkActivityList::size() + activityList().size();
}



size_t
RepeatActivityList::findChildren( CallStack& callStack, const unsigned directPath, std::deque<const Activity *>& activityStack ) const
{
    size_t nextLevel = ForkActivityList::findChildren( callStack, directPath, activityStack );

    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	nextLevel = std::max( (*activity)->findChildren( callStack, directPath, activityStack ), nextLevel );
    } 
    return nextLevel;
}


size_t
RepeatActivityList::findActivityChildren( std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack, Entry * anEntry, size_t depth, const unsigned p, const double rate ) const
{
    size_t nextLevel = ForkActivityList::findActivityChildren( activityStack, forkStack, anEntry, depth, p, rate );
    for ( std::vector<Activity *>::const_iterator activity = activityList().begin(); activity != activityList().end(); ++activity ) {
	std::deque<const AndForkActivityList *> branchForkStack; 	// For matching forks/joins.
	nextLevel = std::max( (*activity)->findActivityChildren( activityStack, branchForkStack, anEntry, depth, 
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
    for_each( myArcList.begin(), myArcList.end(), ExecXY<Arc>( &Arc::scaleBy, sx, sy ) );
    for_each( _labelList.begin(), _labelList.end(), ExecXY<Label>( &Label::scaleBy, sx, sy ) );
    if ( activityList().size() ) {
	myNode->scaleBy( sx, sy );
    }
    return *this;
}



RepeatActivityList& 
RepeatActivityList::translateY( const double dy )
{
    myArc->translateY( dy );
    for_each( myArcList.begin(), myArcList.end(), ExecX<Arc,std::pair<Activity *,Arc *>,double>( &Arc::translateY, dy ) );
    for_each( _labelList.begin(), _labelList.end(), ExecX<Label,std::pair<Activity *,Label *>,double>( &Label::translateY, dy ) );
    if ( activityList().size() ) {
	myNode->translateY( dy );
    }
    return *this;
}



RepeatActivityList& 
RepeatActivityList::depth( unsigned depth )
{
    myArc->depth( depth );
    for_each( myArcList.begin(), myArcList.end(), ExecX<Graphic,std::pair<Activity *, Arc *>,unsigned>( &Graphic::depth, depth ) );
    for_each( _labelList.begin(), _labelList.end(), ExecX<Graphic,std::pair<Activity *,Label *>,unsigned>( &Graphic::depth, depth ) );
    if ( activityList().size() ) {
	myNode->depth( depth );
    }
    return *this;
}


Point
RepeatActivityList::findSrcPoint() const
{
    if ( activityList().size() > 1 ) {
	Point p1( activityList().front()->topCenter() );
	Point p2( activityList().back()->topCenter() );
	return Point( (p1.x() + p2.x()) / 2.0, p1.y() + height() );
    } else if ( activityList().size() == 1 ) {
	Point p2( activityList().at(0)->topCenter() );
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

    for ( std::map<Activity *,Arc *>::iterator arc = myArcList.begin(); arc != myArcList.end(); ++arc ) {
	arc->second->moveSrc( src );
	const Point src3 = arc->second->srcIntersectsCircle( src, radius() );
	arc->second->moveSrc( src3 );
	const Point aPoint = arc->second->pointFromDst(height()/3.0);
	_labelList[arc->first]->moveTo( aPoint );
    }

    return *this; 
} 


RepeatActivityList&
RepeatActivityList::moveDstTo( const Point& dst, Activity * anActivity )
{ 
    if ( anActivity == myActivity ) {
	myArc->moveDst( dst );
    } else {
	std::map<Activity *,Arc *>::const_iterator arc = myArcList.find(anActivity);
	if ( arc != myArcList.end() ) {
	    arc->second->moveDst( dst );
	}
    }
    return *this;
}


double
RepeatActivityList::radius() const
{
    return fabs( myNode->height() ) / 2.0;
}



const RepeatActivityList&
RepeatActivityList::draw( std::ostream& output ) const
{
    const Graphic::Colour pen_colour = colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour();
    for_each( myArcList.begin(), myArcList.end(), ExecX<Graphic,std::pair<Activity *, Arc *>,Graphic::Colour>( &Graphic::penColour, pen_colour ) );

    ForkActivityList::draw( output );
    for_each( myArcList.begin(), myArcList.end(), ConstExecX<Arc,std::pair<Activity *,Arc *>,std::ostream&>( &Arc::draw, output ) );
    for_each( _labelList.begin(), _labelList.end(), ConstExecX<Label,std::pair<Activity *,Label *>,std::ostream&>( &Label::draw, output ) );

    const Point ctr( myNode->center() );
    myNode->penColour( pen_colour ).fillColour( colour() );
    myNode->circle( output, ctr, radius() );
    Point aPoint( ctr.x(), (ctr.y() + myNode->bottom()) / 2. );
    myNode->text( output, aPoint, typeStr() );

    return *this;
}

/* ------------------------ Exception Handling ------------------------ */

bad_internal_join::bad_internal_join( const ForkJoinActivityList& list )
    : std::runtime_error( list.getName() )
{
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


std::ostream&
ActivityList::newline( std::ostream& output )
{
    if ( first ) {
	output << ':';
    } else {
	output << ';';
    }
    first = false;
    output << std::endl << "  ";
    return output;
}

