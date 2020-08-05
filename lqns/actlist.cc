/* actlist.cc   -- Greg Franks Thu Feb 20 1997
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/actlist.cc $
 *
 * Everything you wanted to know about connecting activities, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * February 1997
 *
 * ------------------------------------------------------------------------
 * $Id: actlist.cc 13727 2020-08-04 14:06:18Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <map>
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <lqio/input.h>
#include <lqio/error.h>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include "errmsg.h"
#include "lqns.h"
#include "actlist.h"
#include "activity.h"
#include "entry.h"
#include "task.h"
#include "processor.h"
#include "call.h"
#include "submodel.h"
#include "model.h"
#include "entrythread.h"
#include "pragma.h"
#include "option.h"

unsigned ActivityList::n_joins = 0;
unsigned ActivityList::n_forks = 0;

template <class Type> struct SumDouble
{
    SumDouble<Type>() : _sum(0) {}
    void operator()( const Type& object ) { _sum += object.second; }
    double sum() const { return _sum; }
private:
    double _sum;
};



/* -------------------------------------------------------------------- */
/*               Activity Lists -- Abstract Superclass                  */
/* -------------------------------------------------------------------- */


ActivityList::ActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist )
    : myOwner(owner), myDOMActivityList(dom_activitylist) 
{
    owner->addPrecedence(this);
}



void
ActivityList::reset()
{
    n_joins = 0;
    n_forks = 0;
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
 * Return the previous link.
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
    throw should_not_implement( "ActivityList::addSubList", __FILE__, __LINE__ );
    return *this;
}


/*
 * Return the name of the next item on the list.  For forks/joins, it's the first item
 * in the collection.
 */

const std::string&
ActivityList::lastName() const
{
    throw should_not_implement( "ActivityList::lastName", __FILE__, __LINE__ );
}




/*
 * Return the name of the next item on the list.  For forks/joins, it's the first item
 * in the collection.
 */

const std::string&
ActivityList::firstName() const
{
    throw should_not_implement( "ActivityList::lastName", __FILE__, __LINE__ );
}




/*
 * Common code for aggregation: reset entry information.
 */

void
ActivityList::initEntry( Entry *dstEntry, const Entry * srcEntry, const Activity::Collect& data ) const
{
    const Activity::Function f = data.collect();
    const unsigned int submodel = data.submodel();
    
    if ( f == &Activity::collectServiceTime ) {
        dstEntry->setMaxPhase( max( dstEntry->maxPhase(), srcEntry->maxPhase() ) );
    } else if ( f == &Activity::setThroughput ) {
        dstEntry->setThroughput( srcEntry->throughput() * data.rate() );
    } else if ( f == &Activity::collectWait ) {
        for ( unsigned p = 1; p <= dstEntry->maxPhase(); ++p ) {
            if ( submodel == 0 ) {
                dstEntry->_phase[p].myVariance = 0.0;
            } else {
                dstEntry->_phase[p].myWait[submodel] = 0.0;
            }
        }
    } else if ( f == &Activity::collectReplication ) {
        for ( unsigned p = 1; p <= dstEntry->maxPhase(); ++p ) {
            dstEntry->_phase[p]._surrogateDelay.resize( srcEntry->_phase[p]._surrogateDelay.size() );
            dstEntry->_phase[p].resetReplication();
        }
    }

}

/* -------------------------------------------------------------------- */
/*                       Simple Activity Lists                          */
/* -------------------------------------------------------------------- */

bool
SequentialActivityList::operator==( const ActivityList& operand ) const
{
    const SequentialActivityList * anOperand = dynamic_cast<const SequentialActivityList *>(&operand);
    if ( anOperand ) {
        return anOperand->myActivity == myActivity;
    }
    return false;
}


SequentialActivityList&
SequentialActivityList::add( Activity * anActivity )
{
    myActivity = anActivity;
    return *this;
}





/*
 * return the previous name in the list.
 */

const std::string&
SequentialActivityList::firstName() const
{
    return myActivity->name();
}


/*
 * Print out the next name in the list.
 */

const std::string&
SequentialActivityList::lastName() const
{
    return myActivity->name();
}

/* -------------------------------------------------------------------- */
/*                       Simple Forks (rvalues)                         */
/* -------------------------------------------------------------------- */

ForkActivityList::ForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist )
    : SequentialActivityList( owner, dom_activitylist ),
      prevLink(0)
{
}


/*
 * Configure descendents
 */

void
ForkActivityList::configure( const unsigned n )
{
    if ( myActivity ) {
        myActivity->configure( n );
    }
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
ForkActivityList::findChildren( Call::stack& callStack, const bool directPath, std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack ) const
{
    if ( myActivity ) {
        return myActivity->findChildren( callStack, directPath, activityStack, forkStack );
    } else {
        return callStack.depth();
    }
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

std::deque<const AndForkActivityList *>::const_iterator
ForkActivityList::backtrack( const std::deque<const AndForkActivityList *>& forkStack ) const
{
    if ( prev() ) {
        return prev()->backtrack( forkStack );
    } else {
        return forkStack.end();
    }
}



/*
 * Generate interlocking table.
 */

unsigned
ForkActivityList::followInterlock( std::deque<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    if ( myActivity ) {
        return myActivity->followInterlock( entryStack, globalCalls, callingPhase );
    } else {
        return entryStack.size();
    }
}



/*
 * Follow the path backwards.  Used to set path lists for joins.
 */

Activity::Collect& 
ForkActivityList::collect( std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    if ( myActivity ) {
        return myActivity->collect( entryStack, data );
    } else {
	return data;
    }
}


/*
 * Return the sum of aFunc.
 */

const Activity::Exec&
ForkActivityList::exec( std::deque<const Activity *>& stack, Activity::Exec& data ) const
{
    if ( myActivity ) {
        return myActivity->exec( stack, data );
    } else {
        return data;
    }
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
ForkActivityList::getInterlockedTasks( std::deque<const Entry *>& entryStack, const Entity * myServer,
                                       std::set<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    if ( myActivity ) {
        return myActivity->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase );
    } else {
        return false;
    }
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
ForkActivityList::callsPerform( const Entry * entry, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( myActivity ) {
        myActivity->callsPerform( entry, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
ForkActivityList::concurrentThreads( unsigned n ) const
{
    if ( myActivity ) {
        return myActivity->concurrentThreads( n );
    } else {
        return n;
    }
}

/* -------------------------------------------------------------------- */
/*                      Simple Joins (lvalues)                          */
/* -------------------------------------------------------------------- */

JoinActivityList::JoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist )
    : SequentialActivityList( owner, dom_activitylist ),
      nextLink(0)
{
}


/*
 * Configure descendents
 */

void
JoinActivityList::configure( const unsigned n )
{
    if ( next() ) {
        next()->configure( n );
    }
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
JoinActivityList::findChildren( Call::stack& callStack, const bool directPath, std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack ) const
{
    if ( next() ) {
        return next()->findChildren( callStack, directPath, activityStack, forkStack );
    } else {
        return callStack.depth();
    }
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

std::deque<const AndForkActivityList *>::const_iterator
JoinActivityList::backtrack( const std::deque<const AndForkActivityList *>& forkStack ) const
{
    if ( myActivity ) {
        return myActivity->backtrack( forkStack );
    } else {
        return forkStack.end();
    }
}



/*
 * Link activity.
 */

unsigned
JoinActivityList::followInterlock( std::deque<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    if ( next() ) {
        return next()->followInterlock( entryStack, globalCalls, callingPhase );
    } else {
        return entryStack.size();
    }
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
JoinActivityList::getInterlockedTasks( std::deque<const Entry *>& entryStack, const Entity * myServer,
                                       std::set<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    if ( next() ) {
        return next()->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase );
    } else {
        return false;
    }
}


/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
JoinActivityList::callsPerform( const Entry * entry, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( next() ) {
        next()->callsPerform( entry, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
JoinActivityList::concurrentThreads( unsigned n ) const
{
    if ( next() ) {
        return next()->concurrentThreads( n );
    } else {
        return n;
    }
}




/*
 * Follow the path backwards.  Used to set path lists for joins.
 */

Activity::Collect&
JoinActivityList::collect( std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    if ( next() ) {
        return next()->collect( entryStack, data );
    } else {
	return data;
    }
}


/*
 * Return the sum of aFunc.
 */

const Activity::Exec&
JoinActivityList::exec( std::deque<const Activity *>& stack, Activity::Exec& data ) const
{
    if ( next() ) {
        return next()->exec( stack, data );
    } else {
        return data;
    }
}

/*----------------------------------------------------------------------*/
/*                  Activity lists that fork or join.                   */
/*----------------------------------------------------------------------*/

ForkJoinActivityList::~ForkJoinActivityList()
{
    _activityList.clear();
}


bool
ForkJoinActivityList::operator==( const ActivityList& operand ) const
{
    const ForkJoinActivityList * anOperand = dynamic_cast<const ForkJoinActivityList *>(&operand);
    if ( anOperand ) {
        // Check cltns for equivalence.
        return anOperand->_activityList == _activityList;
    }
    return false;
}

/*
 * Add an item to the activity list.
 */

ForkJoinActivityList&
ForkJoinActivityList::add( Activity * anActivity )
{
    _activityList.push_back( anActivity );
    return *this;
}



/*
 * Construct a list name.
 */

ForkJoinActivityList::ForkJoinName::ForkJoinName( const ForkJoinActivityList& aList )
{
    const std::vector<Activity *>& activities = aList.getMyActivityList();
    for ( std::vector<Activity *>::const_reverse_iterator activity = activities.rbegin(); activity != activities.rend(); ++activity ) {
        if ( activity != activities.rbegin() ) {
            aString += ' ';
            aString += aList.typeStr();
            aString += ' ';
        }
        aString += (*activity)->name();
    }
    aString += '\0';    /* null terminate */
}


const char *
ForkJoinActivityList::ForkJoinName::operator()()
{
    return aString.c_str();
}


/*
 * Return the name of the first activity on the list.  Used for print
 * out join delays.   Note: the list is backwards due to the parser.
 */

const std::string&
ForkJoinActivityList::firstName() const
{
    return _activityList.front()->name();
}


/*
 * Return the name of the first activity on the list.  Used for print
 * out join delays.
 */

const std::string&
ForkJoinActivityList::lastName() const
{
    return _activityList.back()->name();
}

/* -------------------------------------------------------------------- */
/*          Activity Lists that fork -- abstract superclass             */
/* -------------------------------------------------------------------- */

AndOrForkActivityList::AndOrForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist )
    : ForkJoinActivityList( owner, dom_activitylist ),
      prevLink(0)
{
}



/*
 * Free virtual entries.
 */

AndOrForkActivityList::~AndOrForkActivityList()
{
    for ( std::vector<Entry *>::const_iterator entry = _entryList.begin(); entry != _entryList.end(); ++entry ) {
	delete *entry;
    }
}


/*
 * Configure descendents
 */

void
AndOrForkActivityList::configure( const unsigned n )
{
    for_each( _activityList.begin(), _activityList.end(), Exec1<Activity,const unsigned>( &Activity::configure, n ) );
    for_each( _entryList.begin(), _entryList.end(), Exec1<Entry,const unsigned>( &Entry::configure, n ) );
}



AndOrForkActivityList&
AndOrForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );
    _entryList.push_back(NULL);
    return *this;
}


/*
 * Link activity.  Current path ends here.
 */

unsigned
AndOrForkActivityList::followInterlock( std::deque<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    unsigned max_depth = entryStack.size();
    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        const InterlockInfo newCalls = globalCalls * prBranch(*activity);
        max_depth = max( (*activity)->followInterlock( entryStack, newCalls, callingPhase ), max_depth );
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
AndOrForkActivityList::getInterlockedTasks( std::deque<const Entry *>& entryStack, const Entity * myServer,
                                            std::set<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    bool found = false;

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        if ( (*activity)->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase ) ) {
            found = true;
        }
    }

    return found;
}



/*
 * Common code for aggregating the activities for a branch to its psuedo entry
 */

Entry *
AndOrForkActivityList::collectToEntry( const unsigned i, std::deque<Entry *>&entryStack, Activity::Collect& data )
{
    Activity * anActivity = _activityList.at(i);
    Entry * anEntry = _entryList.at(i);
    const Entry * currEntry = entryStack.back();
    Activity::Collect branch(data);
    branch.setRate( prBranch(anActivity) );
    initEntry( anEntry, currEntry, branch );

    entryStack.push_back( anEntry );
    anActivity->collect( entryStack, branch );
    entryStack.pop_back();

    data.setPhase(branch.phase());
    return anEntry;
}

/* -------------------------------------------------------------------- */
/*                      Or Fork Activity Lists                          */
/* -------------------------------------------------------------------- */

/*
 * Add an OR-fork.  Also add a "virtual" entry for each branch of the fork.
 * Aggregation along each branch will take place into the virtual entry.
 * This class will aggregate the results from each branch into the parent.
 * See Smith[] for the variance stuff.
 */

OrForkActivityList&
OrForkActivityList::add( Activity * anActivity )
{
    AndOrForkActivityList::add( anActivity );

    Entry * anEntry = new VirtualEntry( anActivity );
    assert( anEntry->entryTypeOk(ACTIVITY_ENTRY) );
    anEntry->setStartActivity( anActivity );

    _entryList.back() = anEntry;

    return *this;
}



/*
 * Check that all items in the fork list add to one and they all have probabilities.
 */

bool
OrForkActivityList::check() const
{
    double sum = 0.;
    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
	sum += prBranch(*activity);
    }
    if ( fabs( 1.0 - sum ) > EPSILON ) {
        ForkJoinName aName( *this );
        LQIO::solution_error( LQIO::ERR_MISSING_OR_BRANCH, aName(), owner()->name().c_str(), sum );
	return false;
    }
    return true;
}


double
OrForkActivityList::prBranch( const Activity * activity ) const
{
    return getDOM()->getParameterValue(activity->getDOM());
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
OrForkActivityList::findChildren( Call::stack& callStack, const bool directPath, std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack ) const
{
    unsigned max_depth = callStack.depth();
    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        max_depth = max( (*activity)->findChildren( callStack, directPath, activityStack, forkStack ), max_depth );
    }

    return max_depth;
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

std::deque<const AndForkActivityList *>::const_iterator
OrForkActivityList::backtrack( const std::deque<const AndForkActivityList *>& forkStack ) const
{
    if ( prev() ) {
        return prev()->backtrack( forkStack );
    } else {
        return forkStack.end();
    }
}



/*
 * Recursively aggregate using 'aFunc' along all branches of the or, storing
 * the results of the aggregation in the virtual entry assigned to each branch.
 * (except if aFunc == setThroughput).  Mean and variance is needed to
 * find the variance of the aggregate.
 */

Activity::Collect&
OrForkActivityList::collect( std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    const unsigned int n = _activityList.size();
    const unsigned int submodel = data.submodel();
    Entry * currEntry = entryStack.back();
    Activity::Function f = data.collect();

    /* Now search down lists */

    if ( f == &Activity::collectWait ) {
	std::vector<Vector<Exponential> > term( n );
        Vector<Exponential> sum( currEntry->maxPhase() );

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
            Entry * anEntry = collectToEntry( i, entryStack, branch );

            term[i].resize( currEntry->maxPhase() );
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                if ( submodel == 0 ) {
		    const double s =  anEntry->_phase[p].elapsedTime();
		    if ( !isfinite( s ) ) continue;			/* Ignore bogus branches */
                    term[i][p].init( s, anEntry->_phase[p].variance() );
                    for ( unsigned j = 0; j < i; ++j ) {
                        sum[p] += varianceTerm( prBranch(_activityList[i]), term[i][p], prBranch(_activityList[j]), term[j][p] );
                    }
                } else {
                    term[i][p].mean( anEntry->_phase[p].myWait[submodel] );
                }
                sum[p] += prBranch(_activityList[i]) * term[i][p];
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->aggregate( submodel, p, sum[p] );
        }

    } else if ( f == &Activity::collectReplication ) {
        Vector< VectorMath<double> > sum(currEntry->maxPhase());
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            sum[p].resize( sum[p].size() + currEntry->_phase[p]._surrogateDelay.size() );
        }

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
            Entry * anEntry = collectToEntry( i, entryStack, branch );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                VectorMath<double>& term = anEntry->_phase[p]._surrogateDelay;
                term *= prBranch(_activityList[i]);
                sum[p] += term;
            }
        }
        currEntry->aggregateReplication( sum );

    } else if ( f == &Activity::collectServiceTime ) {
        VectorMath<double> sum( currEntry->maxPhase() );

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
            Entry * anEntry = collectToEntry( i, entryStack, branch );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] += prBranch(_activityList[i]) * anEntry->_phase[p].serviceTime();
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->addServiceTime( p, sum[p] );
        }

    } else {
        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
            collectToEntry( i, entryStack, branch );
        }
    }
    return data;
}



/*
 * Return the sum of aFunc.
 */

const Activity::Exec&
OrForkActivityList::exec( std::deque<const Activity *>& stack, Activity::Exec& data ) const
{
    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
	Activity::Exec branch(data);
	branch.setReplyAllowed(false);
	branch.setRate( data.rate() * prBranch(*activity) );
	branch = (*activity)->exec( stack, branch );		/* only want the last one. */
	data = branch.sum();
    }
    return data;
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
OrForkActivityList::callsPerform( const Entry * entry, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        (*activity)->callsPerform( entry, forkList, submodel, k, p, aFunc, rate * prBranch(*activity) );
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
OrForkActivityList::concurrentThreads( unsigned n ) const
{
    const unsigned m = n;

/* Now search down lists */

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        n = max( (*activity)->concurrentThreads( m ), n );
    }

    return n;
}

/* -------------------------------------------------------------------- */
/*                      And Fork Activity Lists                         */
/* -------------------------------------------------------------------- */


AndForkActivityList::AndForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist  )
    : AndOrForkActivityList( owner, dom_activitylist ),
      myParent(0),
      myJoinList(0),
      myJoinDelay(0.0),
      myJoinVariance(0.0)
{
    n_forks += 1;
}




/*
 * Add an AND-fork.  Also add a "thread" entry for each branch of the fork.
 * Aggregation along each branch will take place into the thread.  The
 * threads are also used as chains in the MVA models.
 */

AndForkActivityList&
AndForkActivityList::add( Activity * anActivity )
{
    AndOrForkActivityList::add( anActivity );
    
    Thread * aThread = new Thread( anActivity, this );
    assert( aThread->entryTypeOk(ACTIVITY_ENTRY) );
    aThread->setStartActivity( anActivity );
    _entryList.back() = aThread;

    Task * aTask = const_cast<Task *>(dynamic_cast<const Task *>(anActivity->owner()));    /* Downcase/unconst */

    aTask->addThread( aThread );

    return *this;
}


/*
 * Check that all items in the fork list for this join are the same.
 * If they are zero, tag this join as a synchronization point.
 */

bool
AndForkActivityList::check() const
{
    if ( myJoinList ) {
        return myJoinList->check();
    } else {
	return true;
    }
}


/*
 * Determine if this fork is a descendent of aParent.
 */

bool
AndForkActivityList::isDescendentOf( const AndForkActivityList * aParent ) const
{
    if ( !myParent ) {
        return false;
    } else if ( myParent == aParent ) {
        return true;
    } else {
        return myParent->isDescendentOf( aParent );
    }
}




/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
AndForkActivityList::findChildren( Call::stack& callStack, const bool directPath, std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack ) const
{
    unsigned max_depth = callStack.depth();
    if ( forkStack.size() ) {
        myParent = forkStack.back();
    } else {
        myParent = 0;
    }
    forkStack.push_back( this );

    /* Now search down lists */

    try {
	for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
            if ( Options::Debug::forks() ) {
                cerr << "AndFork: " << setw( activityStack.size() ) << " " << activityStack.back()->name()
                     << " -> " << (*activity)->name() << endl;
            }
            max_depth = max( (*activity)->findChildren( callStack, directPath, activityStack, forkStack ), max_depth );
        }
    } 
    catch ( const bad_internal_join& error ) {
	LQIO::solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, firstName().c_str(), owner()->name().c_str(), error.what() );
        max_depth = max( max_depth, error.depth() );
    }
    forkStack.pop_back();
    return max_depth;
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

std::deque<const AndForkActivityList *>::const_iterator
AndForkActivityList::backtrack( const std::deque<const AndForkActivityList *>& forkStack ) const
{
    std::deque<const AndForkActivityList *>::const_iterator i = std::find( forkStack.begin(), forkStack.end(), this );
    if ( i != forkStack.end() ) {
        return i;
    } else if ( prev() ) {
        return prev()->backtrack( forkStack );
    } else {
        return forkStack.end();
    }
}



/*
 * Aggregate subchains into their psuedo entries.  When all branches
 * haved joined aggregation into this entry will continue by the join.
 * We need the mean and variance of all the branches to calculate the
 * mean and/or variance of the parent.  Note that this aggregate
 * function will sequence to the NEXT activity after the join once all
 * forks have been evaluated (which is different from the behaviour of
 * the OR-fork).
 *
 * Phases cause some grief... the join term is added to the next_p
 * (next_phase).  We have to do some trickery to figure out the phase
 * 1 and 2 waits if a reply occurs on an AND branch.  When the entity
 * is acting as a server, the time spent in the "other threads" shows
 * up as the synchronization delay, so this delay has to be stored by
 * phase.
 */

Activity::Collect&
AndForkActivityList::collect( std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    const unsigned int submodel = data.submodel();
    const unsigned n = _activityList.size();
    Entry * currEntry = entryStack.back();
    const unsigned int curr_phase = data.phase();
    unsigned next_phase = curr_phase;
    Activity::Function f = data.collect();
    
    if (flags.trace_quorum) {
        cout <<"\nAndForkActivityList::collect()...the start --------------- : submodel = " << submodel <<  endl;
    }

    // Start tomari: Quorum
    //The current code assumes that every AndFork corresponds to an AndJoin.
    //So the aggregation is done only in the ForkList.

    if ( f == &Activity::collectWait ) {
        DiscreteCDFs  quorumCDFs;
        DiscreteCDFs  localCDFs;
        DiscreteCDFs  remoteCDFs;
        bool isQuorumDelayedThreadsActive = false;
        double totalParallelLocal = 0;
        double totalSequentialLocal = 0;

        /* Calculate start time */

        double time = 0.0;
        if ( submodel != 0 ) {
            time = currEntry->getStartTime();
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                time += currEntry->_phase[p].myWait[submodel]; /* Pick off time for this pass. - (since day 1!) */
            }
        } else {
            time = currEntry->getStartTimeVariance();
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                time += currEntry->_phase[p].variance();       /* total isn't known until all activities scanned. */
            }
        }

        /* Now search down lists */

        Exponential phase_one;
        const bool isThereQuorumDelayedThreads = myJoinList && myJoinList->hasQuorum();

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
            Thread * anEntry = dynamic_cast<Thread *>(collectToEntry( i, entryStack, branch ));
            next_phase = max( next_phase, branch.phase() );

            Vector<Exponential> term( currEntry->maxPhase() );

            anEntry->startTime( submodel, time );

            if ( submodel == 0 ) {

                /* Updating variance */

                DiscretePoints sumTotal;              /* BUG 583 -- we don't care about phase */
                DiscretePoints sumLocal;
                DiscretePoints sumRemote;

                anEntry->_total.myVariance = 0.0;
                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->_total.myVariance += anEntry->_phase[p].variance();
                    if (flags.trace_quorum) {
                        cout <<"\nEntry " << anEntry->name() << ", anEntry->elapsedTime(p="<<p<<")=" << anEntry->_phase[p].elapsedTime() << endl;
                        cout << "anEntry->phase[p="<<p<<"].myWait[submodel=1]=" << anEntry->_phase[p].myWait[1] << endl;
                        cout << "anEntry->Entry::variance(p="<<p<<"]="<< anEntry->_phase[p].variance() << endl;
                    }

                    term[p].init( anEntry->_phase[p].elapsedTime(), anEntry->_phase[p].variance() );
                    sumTotal += term[p];            /* BUG 583 */
                }
                // tomari: first possible update for Quorum.

                _activityList[i]->estimateQuorumJoinCDFs(sumTotal, quorumCDFs,
                                                          localCDFs, remoteCDFs,
                                                          isThereQuorumDelayedThreads,
                                                          isQuorumDelayedThreadsActive,
                                                          totalParallelLocal,
                                                          totalSequentialLocal);

            } else if ( submodel == Model::sync_submodel ) {

                /* Updating join delays */

                DiscretePoints sumTotal;              /* BUG 583 -- we don't care about phase */
                DiscretePoints sumLocal;
                DiscretePoints sumRemote;

                anEntry->_total.myWait[submodel] = 0.0;
                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->_total.myWait[submodel] += anEntry->_phase[p].myWait[submodel];
                    if (flags.trace_quorum) {
                        cout <<"\nEntry " << anEntry->name() <<", anEntry->elapsedTime(p="<<p<<")=" <<anEntry->_phase[p].elapsedTime() << endl;
//                        cout << "anEntry->phase[curr_p="<<curr_p<<"].myWait[submodel="<<2<<"]=" << anEntry->_phase[curr_p].myWait[2] << endl;
                    }

                    term[p].init( anEntry->_phase[p].elapsedTime(), anEntry->_phase[p].variance() );
                    sumTotal += term[p];            /* BUG 583 */
                }

                // tomari: second possible update for Quorum
                _activityList[i]->estimateQuorumJoinCDFs(sumTotal, quorumCDFs,
                                                          localCDFs, remoteCDFs,
                                                          isThereQuorumDelayedThreads,
                                                          isQuorumDelayedThreadsActive,
                                                          totalParallelLocal,
                                                          totalSequentialLocal);

            } else {

                /* Updating the waiting time for this submodel */

                anEntry->_total.myWait[submodel] = 0.0;
                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->_total.myWait[submodel] += anEntry->_phase[p].myWait[submodel];
                    term[p].init( anEntry->_phase[p].myWait[submodel], anEntry->_phase[p].variance() );
                }
            }

            /* Phase change for this branch, so record the time it occurs. */

            if ( next_phase != curr_phase ) {
                phase_one = term[curr_phase];
            }
        }

        //to disable accounting for sequential execution in the quorum delayed threads,
        //set probQuorumDelaySeqExecution to zero.
        //0probQuorumDelaySeqExecution = 0;
        DiscretePoints * join = calcQuorumKofN( submodel, isQuorumDelayedThreadsActive, quorumCDFs );

        /* Need to compute p1 and p2 delay components */

        if ( submodel == 0 ) {
            myJoinDelay = join->mean();
            /* Update Variance for parent. */
            if ( next_phase != curr_phase ) {
                currEntry->aggregate( submodel, curr_phase, phase_one );
                (*join) -= phase_one;
            }
            currEntry->aggregate( submodel, next_phase, *join );
        } else if ( submodel == Model::sync_submodel ) {
            if (flags.trace_quorum) {
                cout << "\nmyJoinDelay " << join->mean() << endl;
                cout << "myJoinVariance " << join->variance() << endl;
            }

            myJoinVariance = join->variance();
            if ( flags.trace_activities ) {
                cout << "Join delay aggregate to ";
                if ( dynamic_cast<VirtualEntry *>(currEntry) ) {
                    cout << " virtual entry ";
                } else {
                    cout << " actual entry ";
                }
                cout << currEntry->name() << ", submodel " << submodel << ", phase " << curr_phase << " wait: " << endl;
            }

            /* Update quorumJoin delay for parent.  Set variance for this fork/join.  */

            if ( next_phase != curr_phase ) {
                if ( flags.trace_activities ) {
                    cout << *join << ", phase " << next_phase << " wait: ";
                }

                /* we've encountered a phase change, so try to estimate the phases.  */

                currEntry->aggregate( submodel, curr_phase, phase_one );
                (*join) -= phase_one;
            }

            if ( flags.trace_activities ) {
                cout << *join << endl;
            }
            currEntry->aggregate( submodel, next_phase, *join );
        }

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
        if ( myJoinList && myJoinList->hasQuorum()
	     if (totalParallelLocal+ totalSequentialLocal > 0) {
		 double probQuorumDelaySeqExecution = totalParallelLocal/
		     (totalParallelLocal+ totalSequentialLocal);
	     }

             && submodel == Model::sync_submodel
             && !flags.disable_expanding_quorum_tree /*!pragmaQuorumDistribution.test(DISABLE_EXPANDING_QUORUM)*/
             && pragma.getQuorumDelayedCalls() == KEEP_ALL_QUORUM_DELAYED_CALLS ) {
            saveQuorumDelayedThreadsServiceTime(entryStack,*join,quorumCDFs,
                                                localCDFs,remoteCDFs,
                                                probQuorumDelaySeqExecution);
        }
#endif

        delete join;

    } else if ( f == &Activity::collectServiceTime ) {

        /* For service time, just sum everthing up. */

        VectorMath<double> sum( currEntry->maxPhase() );
        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch(data);
            Entry * anEntry = collectToEntry( i, entryStack, branch );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] += anEntry->_phase[p].serviceTime();
            }
            next_phase = max( next_phase, branch.phase() );
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->addServiceTime( p, sum[p] );
        }

    } else {

        for ( unsigned i = 0; i < n; ++i ) {
            collectToEntry( i, entryStack, data );
        }
    }
    data.setPhase( next_phase );
    
    /* Now follow the activities after the quorumJoin */

    if ( myJoinList && myJoinList->next() ) {
        myJoinList->next()->collect( entryStack, data );
    }

    if (flags.trace_quorum) {
        cout <<"AndForkActivityList::collect()...the end --------------- : submodel = " << submodel <<  endl;
    }
    return data;
}


//Get the CDF of the joint dsitribution for a K out of N quorum

DiscretePoints *
AndForkActivityList::calcQuorumKofN( const unsigned submodel,
                                     bool isQuorumDelayedThreadsActive,
                                     DiscreteCDFs & quorumCDFs ) const
{
    const unsigned n = _activityList.size();
    DiscretePoints * join;

    if (flags.trace_quorum) {
        cout << "\nAndForkActivityList::calcQuorumKofN(): submodel=" <<submodel<< endl;
    }
    if (myJoinList  ) {
        if (myJoinList->quorumCount() == 0) {
            const_cast<AndJoinActivityList *>(myJoinList)->quorumCount(n);
        }

        if ( isQuorumDelayedThreadsActive) {
            join = quorumCDFs.quorumKofN( myJoinList->quorumCount() + 1, n + 1 );
            if (flags.trace_quorum) {
                cout << "quorum (AndJoin) of " <<myJoinList->quorumCount() + 1
                     << " out of " << n + 1 << endl;
            }
        } else {
            join = quorumCDFs.quorumKofN(myJoinList->quorumCount(),n );
            if (flags.trace_quorum) {
                cout <<"quorum of " <<myJoinList->quorumCount() << " out of " << n << endl;
            }
        }

        if (flags.trace_quorum) {
            cout <<"quorumJoin mean=" <<join->mean()
                 << ", Variance=" << join->variance() << endl;
        }
    } else {
        /* BUG 327 */
        join = quorumCDFs.quorumKofN( n, n );
    }

    if ( submodel == Model::sync_submodel ) {
        for ( std::vector<Entry *>::const_iterator entry = _entryList.begin(); entry != _entryList.end(); ++entry ) {
            if ( dynamic_cast<Thread *>(*entry) ) {
                dynamic_cast<Thread *>(*entry)->joinDelay(join->mean());
            }
        }
    }

    return join;
}


#if HAVE_LIBGSL
bool
AndForkActivityList::saveQuorumDelayedThreadsServiceTime( Stack<Entry *>& entryStack,
                                                          DiscretePoints & quorumJoin,
                                                          DiscreteCDFs & quorumCDFs,
                                                          DiscreteCDFs & localCDFs,
                                                          DiscreteCDFs & remoteCDFs,
                                                          double probQuorumDelaySeqExecution )
{
    if (flags.trace_quorum) {
        cout <<"\n'''''''''''start saveQuorumDelayedThreadsServiceTime'''''''''''''''''''''''" << endl;
    }
    const unsigned n = _activityList.size();
    bool anError= false;
    Activity * localQuorumDelayActivity = NULL;
    Task * myTask = (Task *) (_activityList[0]->owner());
    Entry * currEntry = entryStack.back();

    unsigned orgSubmodel = currEntry->owner()->submodel();
    orgSubmodel++;

    DiscretePoints * localQuorumJoin = localCDFs.quorumKofN(myJoinList->quorumCount(),n );
    DiscretePoints * localAndJoin = localCDFs.quorumKofN( n, n );
    DiscretePoints localDiffJoin;
    localDiffJoin.mean( abs(localAndJoin->mean() -localQuorumJoin->mean()));
    localDiffJoin.variance(abs(localAndJoin->variance() + localQuorumJoin->variance() ));

    if (flags.trace_quorum) {
        cout << "localAndJoin->mean() = " << localAndJoin->mean() <<
            ", Variance = " << localAndJoin->variance() << endl; ;
        cout << "localQuorumJoin->mean() = " << localQuorumJoin->mean() <<
            ", Variance = " << localQuorumJoin->variance() << endl;
        cout << "localDiffJoin.mean() = " << localDiffJoin.mean() <<
            ", Variance = " << localDiffJoin.variance() << endl;
    }
    delete localAndJoin;
    delete localQuorumJoin;

    DiscretePoints * remoteQuorumJoin = remoteCDFs.quorumKofN(myJoinList->quorumCount(),n );
    DiscretePoints * remoteAndJoin = remoteCDFs.quorumKofN(n,n );
    DiscretePoints remoteDiffJoin;
    remoteDiffJoin.mean(abs(remoteAndJoin->mean() -remoteQuorumJoin->mean()));
    remoteDiffJoin.variance(abs(remoteAndJoin->variance() + remoteQuorumJoin->variance()) );

    if (flags.trace_quorum) {
        cout << "\nremoteAndJoin->mean() = " << remoteAndJoin->mean() <<
            ", Variance = " << remoteAndJoin->variance() << endl; ;
        cout << "remoteQuorumJoin->mean() = " << remoteQuorumJoin->mean() <<
            ", Variance = " << remoteQuorumJoin->variance() << endl;
        cout << "remoteDiffJoin.mean() = " << remoteDiffJoin.mean() <<
            ", Variance = " << remoteDiffJoin.variance() << endl;
    }
    delete remoteAndJoin;
    delete remoteQuorumJoin;

    DiscretePoints * quorumAndJoin = quorumCDFs.quorumKofN(n,n );
    DiscretePoints quorumDiffJoin;
    quorumDiffJoin.mean(abs(quorumAndJoin->mean() -quorumJoin.mean()));
    quorumDiffJoin.variance(abs(quorumAndJoin->variance() + quorumJoin.variance() ));

    if (flags.trace_quorum) {
        cout << "\nquorumAndJoin->mean() = " << quorumAndJoin->mean() <<
            ", Variance = " << quorumAndJoin->variance() << endl;
        cout << "quorumJoin.mean() = " << quorumJoin.mean() <<
            ", Variance = " << quorumJoin.variance() << endl;
        cout << "quorumDiffJoin.mean() = " << quorumDiffJoin.mean() <<
            ", Variance = " << quorumDiffJoin.variance() << endl;
    }
    delete quorumAndJoin;

    char localQuorumDelayActivityName[32];
    sprintf( localQuorumDelayActivityName, "localQmDelay_%d", myJoinList->quorumListNum() );
    localQuorumDelayActivity = myTask->findActivity(localQuorumDelayActivityName);

    if (localQuorumDelayActivity != NULL) {

        DeviceEntry * procEntry = dynamic_cast<DeviceEntry *>(const_cast<Entry *>(localQuorumDelayActivity->processorCall()->dstEntry()));

        if (!flags.ignore_overhanging_threads) {
	    double service_time = (1-probQuorumDelaySeqExecution) *  localDiffJoin.mean();
            localQuorumDelayActivity->setServiceTime( service_time );
	    procEntry->setServiceTime( service_time  / localQuorumDelayActivity->numberOfSlices() )
		.setCV_sqr( ((1-probQuorumDelaySeqExecution) * localDiffJoin.variance())/square(service_time) );
            procEntry->_phase[1].myWait[orgSubmodel] = (1-probQuorumDelaySeqExecution) * localDiffJoin.mean() ;
        } else {
            localQuorumDelayActivity->setServiceTime(0);
	    procEntry->setServiceTime( 0. ).setCV_sqr( 1. );
            procEntry->_phase[1].myWait[orgSubmodel] = 0.;
        }

        if (flags.trace_quorum) {
            cout <<" procEntry->_phase[1].myWait[orgSubmodel="<<orgSubmodel<<"]="<< procEntry->_phase[1].myWait[orgSubmodel] << endl;
        }

        if (!flags.ignore_overhanging_threads) {
            localQuorumDelayActivity->remoteQuorumDelay.mean(
                remoteDiffJoin.mean() + localDiffJoin.mean() * probQuorumDelaySeqExecution );
            localQuorumDelayActivity->remoteQuorumDelay.variance(
                remoteDiffJoin.variance() + localDiffJoin.variance() * probQuorumDelaySeqExecution );
        } else {
            localQuorumDelayActivity->remoteQuorumDelay.mean(0);
            localQuorumDelayActivity->remoteQuorumDelay.variance(0);
        }
        if (flags.trace_quorum) {
            cout <<"orgSubmodel="<<orgSubmodel<< endl;
            cout << "0. localQuorumDelayActivity->remoteQuorumDelay: mean =" << localQuorumDelayActivity->remoteQuorumDelay.mean()
                 << ", Variance=" << localQuorumDelayActivity->remoteQuorumDelay.variance() << endl;
            cout <<"probQuorumDelaySeqExecution=" <<probQuorumDelaySeqExecution << endl;
        }

    } else {
	throw logic_error( "AndForkActivityList::saveQuorumDelayedThreadsServiceTime" );
        anError = true;
    }

    if (flags.trace_quorum) {
        cout <<"\n'''''''''''end saveQuorumDelayedThreadsServiceTime'''''''''''''''''''''''" << endl;
    }

    return !anError;
}
#endif

/*
 * Return the sum of aFunc.
 */

const Activity::Exec&
AndForkActivityList::exec( std::deque<const Activity *>& stack, Activity::Exec& data ) const
{
    double sum = 0.0;
    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
	Activity::Exec branch( data );
	branch.setReplyAllowed(!myJoinList || !myJoinList->hasQuorum());		/* Disallow replies quorum on branches */
	branch = (*activity)->exec( stack, branch );
        sum += branch.sum();
	if ( branch.phase() > data.phase() ) {
	    data.setPhase(branch.phase());
	}
    }
    data = sum;
    return data;		/* Result of last branch */
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait.
 * Only follow the root path.  Other paths in the AND-FORK are followed
 * by their thread.
 */

void
AndForkActivityList::callsPerform( const Entry * entry, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( myJoinList && myJoinList->next() ) {
        myJoinList->next()->callsPerform( entry, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
AndForkActivityList::concurrentThreads( unsigned n ) const
{
    unsigned m = for_each( _activityList.begin(), _activityList.end(), Sum1<Activity,unsigned,unsigned>( &Activity::concurrentThreads, 1 ) ).sum();
    n = max( n, m - 1 );

    if ( myJoinList && myJoinList->next() ) {
        return myJoinList->next()->concurrentThreads( n );
    } else {
        return n;
    }
}




/*
 * Locate all joins and print delays.
 */

const AndForkActivityList&
AndForkActivityList::insertDOMResults(void) const
{
    if ( prev() && myJoinList && myJoinList->next() ) {
	/* Result is calculated at fork in lqns, but stored in the join in the DOM */
	LQIO::DOM::AndJoinActivityList * domActlist = const_cast<LQIO::DOM::AndJoinActivityList *>(dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(myJoinList->getDOM()));
	if ( domActlist ) {
	    domActlist->setResultJoinDelay(myJoinDelay)
		.setResultVarianceJoinDelay(myJoinVariance);
	}
    }
    return *this;
}

/* -------------------------------------------------------------------- */
/*                    And Or Join Activity Lists                        */
/* -------------------------------------------------------------------- */

AndOrJoinActivityList::AndOrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist )
    : ForkJoinActivityList( owner, dom_activitylist ),
      visits(0),
      nextLink(0)
{
}



/*
 * Configure descendents
 */

void
AndOrJoinActivityList::configure( const unsigned n )
{
    if ( next() ) {
        visits += 1;
        if ( visits == 1 ) {
            next()->configure( n );
        }
        if ( visits == _activityList.size() ) {
            visits = 0;
        }
    }
}



/*
 * Search backwards up activity list looking for a match on forkStack
 */

std::deque<const AndForkActivityList *>::const_iterator
AndOrJoinActivityList::backtrack( const std::deque<const AndForkActivityList *>& forkStack ) const
{
    unsigned depth = 0;
    std::deque<const AndForkActivityList *>::const_iterator i = forkStack.end();
    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
	std::deque<const AndForkActivityList *>::const_iterator j = (*activity)->backtrack( forkStack );
	if ( forkStack.end() - j > depth ) {	/* find the one furthest away. */
	    i = j;
	    depth = forkStack.end() - j;
	}
    }
    return i;
}

/* -------------------------------------------------------------------- */
/*                      Or Join Activity Lists                          */
/* -------------------------------------------------------------------- */

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
OrJoinActivityList::findChildren( Call::stack& callStack, const bool directPath, std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack ) const
{
    if ( next() ) {
        return next()->findChildren( callStack, directPath, activityStack, forkStack );
    }
    return callStack.depth();
}



/*
 * Follow the path.  We don't care about other paths.
 */

unsigned
OrJoinActivityList::followInterlock( std::deque<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    if ( next() ) {
        return next()->followInterlock( entryStack, globalCalls, callingPhase );
    } else {
        return entryStack.size();
    }
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
OrJoinActivityList::getInterlockedTasks( std::deque<const Entry *>& entryStack, const Entity * myServer,
                                         std::set<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    if ( next() ) {
        visits += 1;
        if ( visits == _activityList.size() ) {
            visits = 0;
            return next()->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase );
        }
    }
    return false;
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
OrJoinActivityList::callsPerform( const Entry * entry, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( next() ) {
        next()->callsPerform( entry, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * Follow the path backwards.  Used to set path lists for joins.
 */

Activity::Collect&
OrJoinActivityList::collect( std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    if ( next() ) {
        return next()->collect( entryStack, data );
    } else {
	return data;
    }
}



/*
 * Return the sum of aFunc.
 */

const Activity::Exec&
OrJoinActivityList::exec( std::deque<const Activity *>& stack, Activity::Exec& data )  const
{
    if ( _rateBranch.find( stack.back() ) == _rateBranch.end() ) {
	_rateBranch.insert( std::pair<const Activity *,double>( stack.back(), data.rate() ) );
    }
    if ( next() && stack.back() == _activityList.back() ) {
	double sum = std::for_each( _rateBranch.begin(), _rateBranch.end(),
				    SumDouble<std::pair<const Activity *,double> >() ).sum();
	data.setReplyAllowed(sum == 1.0);
	data.setRate(sum);
        return next()->exec( stack, data );
    } else {
        return data;
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
OrJoinActivityList::concurrentThreads( unsigned n ) const
{
    if ( next() ) {
        visits += 1;
        if ( visits == _activityList.size() ) {
            visits = 0;
            return next()->concurrentThreads( n );
        }
    }
    return n;
}

/* -------------------------------------------------------------------- */
/*                     And Join Activity Lists                          */
/* -------------------------------------------------------------------- */

AndJoinActivityList::AndJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist )
    : AndOrJoinActivityList( owner, dom_activitylist ),
      myJoinType(JOIN_NOT_DEFINED),
      myQuorumCount(dom_activitylist ? dynamic_cast<LQIO::DOM::AndJoinActivityList*>(dom_activitylist)->getQuorumCountValue() : 0),
      myQuorumListNum(0)
{
    n_joins += 1;
}



/*
 * Add and activity to this join list.
 * Grow fork list collection so that we can figure out which fork we came from.
 */

AndJoinActivityList&
AndJoinActivityList::add( Activity * anActivity )
{
    AndOrJoinActivityList::add( anActivity );
    _forkList.push_back(NULL);
    _srcList.push_back(NULL);
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


/*
 * Check that all items in the fork list for this join are the same.
 * If they are zero, tag this join as a synchronization point.
 */

bool
AndJoinActivityList::check() const
{
    for ( unsigned j = 1; j < _activityList.size(); ++j ) {
        if ( _forkList[0] != _forkList[j] ) {
            ForkJoinName aName( *this );
	    LQIO::io_vars.error_messages[LQIO::ERR_JOIN_PATH_MISMATCH].severity = LQIO::WARNING_ONLY;
            LQIO::solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, aName(), owner()->name().c_str() );
            return false;
        }
    }
    return true;
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 *
 * We backtrack along all branches that we did not originate from
 * looking for fork points.  All fork points should match.
 */

unsigned
AndJoinActivityList::findChildren( Call::stack& callStack, const bool directPath, std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack ) const
{
    /* Look for the fork on the fork stack */

    for ( unsigned i = 0; i < _activityList.size(); ++i ) {

        if ( _activityList[i] != activityStack.back() ) {
            std::deque<const AndForkActivityList *>::const_iterator j = _activityList[i]->backtrack( forkStack );
            if ( j != forkStack.end() ) {
                if ( !const_cast<AndJoinActivityList *>(this)->joinType( INTERNAL_FORK_JOIN  ) ) {
                    throw bad_internal_join( activityStack );
                } else if( _forkList.at(i) == nullptr || std::find( forkStack.begin(), forkStack.end(), _forkList[i] ) != forkStack.end() ) {
                    _forkList[i] = *j;
                    _forkList[i]->myJoinList = this;      /* Random choice :-) */
                }
                if ( Options::Debug::forks() ) {
                    cerr << "AndJoin: " << setw( activityStack.size() ) << " " << activityStack.back()->name()
                         << ": " << _activityList[i]->name();
                    for ( std::deque<const AndForkActivityList *>::const_iterator k = forkStack.begin(); k != forkStack.end(); ++k ) {
                        if ( j == k ) {
                            cerr << " [" << (*k)->prev()->firstName() << "]";
                        } else {
                            cerr << " " << (*k)->prev()->firstName();
                        }
                    }
                    cerr << endl;
                }
            } else {
                if ( !const_cast<AndJoinActivityList *>(this)->joinType( SYNCHRONIZATION_POINT ) ) {
                    throw bad_internal_join( activityStack );
                } else if ( !addToSrcList( i, activityStack.back() ) ) {
                    throw bad_internal_join( activityStack );
                } else {
                    throw bad_external_join( activityStack );
                }
            }
        }
    }

    if ( next() ) {
        return next()->findChildren( callStack, directPath, activityStack, forkStack );
    } else {
        return callStack.depth();
    }
}



/*
 * Add anActivity to the activity list provided it isn't there already
 * and the slot that it is to go in isn't already occupied.
 */

bool
AndJoinActivityList::addToSrcList( unsigned i, const Activity * anActivity ) const
{
    if ( _srcList.at(i) != nullptr && _srcList[i] != anActivity ) {
        return false;
    } else {
        _srcList[i] = anActivity;
    }

    for ( unsigned j = 0; j < _srcList.size(); ++j ) {
        if ( j != i && _srcList[j] == anActivity ) return false;
    }
    return true;
}



/*
 * Follow the path.  We don't care about other paths.
 */

unsigned
AndJoinActivityList::followInterlock( std::deque<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    if ( next() ) {
        visits += 1;
        if ( visits == _activityList.size() || isSynchPoint() ) {
            visits = 0;
            return next()->followInterlock( entryStack, globalCalls, callingPhase );
        }
    }
    return entryStack.size();
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
AndJoinActivityList::getInterlockedTasks( std::deque<const Entry *>& entryStack, const Entity * myServer,
                                          std::set<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    if ( next() ) {
        visits += 1;
        if ( visits == _activityList.size() || isSynchPoint() ) {
            visits = 0;
            return next()->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase );
        }
    }
    return false;
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
AndJoinActivityList::callsPerform( const Entry * entry, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( isSynchPoint() && next() ) {
        next()->callsPerform( entry, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.
 * Otherwise, press on.
 */

Activity::Collect&
AndJoinActivityList::collect( std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    if ( isSynchPoint() && next() ) {
        return next()->collect( entryStack, data );
    } else {
	return data;
    }
}



/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.
 * Otherwise, press on.
 */

const Activity::Exec&
AndJoinActivityList::exec( std::deque<const Activity *>& stack, Activity::Exec& data ) const
{
    if ( next() && (isSynchPoint() || stack.back() == _activityList.back()) ) {
        return next()->exec( stack, data );
    } else {
        return data;
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
AndJoinActivityList::concurrentThreads( unsigned n ) const
{
    if ( isSynchPoint() ) {
        return next()->concurrentThreads( n );
    } else {
        return n;
    }
}

/*----------------------------------------------------------------------*/
/*                           Repetition node.                           */
/*----------------------------------------------------------------------*/

RepeatActivityList::RepeatActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist )
    : ForkActivityList( owner, dom_activitylist ), prevLink(0)
{
}



RepeatActivityList::~RepeatActivityList()
{
    _activityList.clear();
    for ( std::vector<Entry *>::const_iterator entry = _entryList.begin(); entry != _entryList.end(); ++entry ) {
	delete *entry;
    }
    _entryList.clear();
}




/*
 * Configure descendents
 */

void
RepeatActivityList::configure( const unsigned n )
{
    ForkActivityList::configure( n );
    for_each( _activityList.begin(), _activityList.end(), Exec1<Activity,unsigned>( &Activity::configure, n ) );
    for_each( _entryList.begin(), _entryList.end(), Exec1<Entry,unsigned>( &Entry::configure, n ) );
}



/*
 * Add a sublist.
 */

RepeatActivityList&
RepeatActivityList::add( Activity * anActivity )
{
    const LQIO::DOM::ExternalVariable * arg = const_cast<LQIO::DOM::ActivityList *>(getDOM())->getParameter(const_cast<LQIO::DOM::Activity *>(anActivity->getDOM()));
    if ( arg ) {

        _activityList.push_back(anActivity);

	Entry * anEntry = new VirtualEntry( anActivity );
	_entryList.push_back(anEntry);
        assert( anEntry->entryTypeOk(ACTIVITY_ENTRY) );
        anEntry->setStartActivity( anActivity );

    } else {

        /* End of list */
        ForkActivityList::add( anActivity );

    } 

    return *this;
}


double 
RepeatActivityList::rateBranch( const Activity * anActivity ) const
{
    return getDOM()->getParameterValue(anActivity->getDOM());
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
RepeatActivityList::findChildren( Call::stack& callStack, const bool directPath, std::deque<const Activity *>& activityStack, std::deque<const AndForkActivityList *>& forkStack ) const
{
    unsigned max_depth = ForkActivityList::findChildren( callStack, directPath, activityStack, forkStack );

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
	std::deque<const AndForkActivityList *> branchForkStack;    // For matching forks/joins.
        max_depth = max( (*activity)->findChildren( callStack, directPath, activityStack, branchForkStack ), max_depth );
    }

    return max_depth;
}



/*
 * Link activity.
 */

unsigned
RepeatActivityList::followInterlock( std::deque<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    unsigned max_depth = ForkActivityList::followInterlock( entryStack, globalCalls, callingPhase );

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        const InterlockInfo newCalls = globalCalls * rateBranch(*activity);
        max_depth = max( (*activity)->followInterlock( entryStack, newCalls, callingPhase ), max_depth );
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
RepeatActivityList::getInterlockedTasks( std::deque<const Entry *>& entryStack, const Entity * myServer,
                                         std::set<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    bool found = ForkActivityList::getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase );

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        if ( (*activity)->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase ) ) {
            found = true;
        }
    }

    return found;
}


/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
RepeatActivityList::callsPerform( const Entry * entry, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    ForkActivityList::callsPerform( entry, forkList, submodel, k, p, aFunc, rate );

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        (*activity)->callsPerform( entry, forkList, submodel, k, p, aFunc, rate * rateBranch(*activity) );
    }
}



/*
 * Recursively aggregate using 'aFunc' along all branches of the or, storing
 * the results of the aggregation in the virtual entry assigned to each branch.
 */

Activity::Collect&
RepeatActivityList::collect( std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    ForkActivityList::collect( entryStack, data );

    const unsigned int submodel = data.submodel();
    const unsigned int n = _activityList.size();
    Entry * currEntry = entryStack.back();
    const unsigned int curr_phase = data.phase();
    Activity::Function f = data.collect();

    for ( unsigned i = 0; i < n; ++i ) {
	Activity * anActivity = _activityList.at(i);
	Activity::Collect branch(data);
	branch.setRate( data.rate() * rateBranch(anActivity) );
        initEntry( _entryList[i], currEntry, branch );

        entryStack.push_back( _entryList[i] );
        anActivity->collect( entryStack, branch );
        entryStack.pop_back();

        if ( f == &Activity::collectWait ) {
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                Exponential term;
                Exponential sum;
                if ( submodel == 0 ) {
                    term.init( _entryList[i]->_phase[p].elapsedTime(), _entryList[i]->_phase[p].variance() );
                } else {
                    term.mean( _entryList[i]->_phase[p].myWait[submodel] );
                }
                sum = rateBranch(anActivity) * term + varianceTerm( term );
                currEntry->aggregate( submodel, p, sum );
            }

        } else if ( f == &Activity::collectReplication ) {
            Vector< VectorMath<double> > sum(currEntry->maxPhase());
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] = _entryList[i]->_phase[p]._surrogateDelay;
                sum[p] *= rateBranch(anActivity);
            }
            currEntry->aggregateReplication( sum );

        } else if ( f == &Activity::collectServiceTime ) {
	    currEntry->addServiceTime( curr_phase, rateBranch(anActivity) * _entryList[i]->_phase[curr_phase].serviceTime() );
        }
    }
    return data;
}


/*
 * Return the sum of aFunc.
 * Rate is set to zero for branches so that we don't count replies.
 */

const Activity::Exec&
RepeatActivityList::exec( std::deque<const Activity *>& stack, Activity::Exec& data ) const
{
    data = ForkActivityList::exec( stack, data );

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
	Activity::Exec branch = data;
	data.setReplyAllowed(false);
	branch.setRate( data.rate() * rateBranch(*activity) );
	branch = (*activity)->exec( stack, branch );
	data += branch.sum();
    }
    return data;
}




/*
 * Get the number of concurrent threads
 */

unsigned
RepeatActivityList::concurrentThreads( unsigned n ) const
{
    n = ForkActivityList::concurrentThreads( n );

    for ( std::vector<Activity *>::const_iterator activity = _activityList.begin(); activity != _activityList.end(); ++activity ) {
        n = max( n, (*activity)->concurrentThreads( n ) );
    }
    return n;
}

/* ------------------------ Exception Handling ------------------------ */

bad_internal_join::bad_internal_join( const std::deque<const Activity *>& activityStack )
    : path_error( activityStack.size() )
{
    for ( std::deque<const Activity *>::const_reverse_iterator i = activityStack.rbegin(); i != activityStack.rend(); ++i ) {
	if ( i != activityStack.rbegin() ) myMsg += ", ";
        myMsg += (*i)->name();
    }
}



bad_external_join::bad_external_join( const std::deque<const Activity *>& activityStack )
    : path_error( activityStack.size() )
{
    for ( std::deque<const Activity *>::const_reverse_iterator i = activityStack.rbegin(); i != activityStack.rend(); ++i ) {
	if ( i != activityStack.rbegin() ) myMsg += ", ";
        myMsg += (*i)->name();
    }
}

/* ---------------------------------------------------------------------- */

/*
 * Connect the src and dst lists together.
 */

void
act_connect ( ActivityList * src, ActivityList * dst )
{
    if ( src ) {
        src->next( dst  );
    }
    if ( dst ) {
        dst->prev( src );
    }
}

void complete_activity_connections ()
{
    /* We stored all the necessary connections and resolved the list identifiers so finalize */
    std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*>::iterator iter;
    for (iter = Activity::actConnections.begin(); iter != Activity::actConnections.end(); ++iter) {
	ActivityList* src = Activity::domToNative[iter->first];
	ActivityList* dst = Activity::domToNative[iter->second];
	assert(src != NULL && dst != NULL);
	act_connect(src, dst);
    }
}
