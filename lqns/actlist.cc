/* actlist.cc   -- Greg Franks Thu Feb 20 1997
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/actlist.cc $
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
 * $Id: actlist.cc 11977 2014-04-14 00:46:01Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cmath>
#include <lqio/input.h>
#include <lqio/error.h>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include "errmsg.h"
#include "lqns.h"
#include "cltn.h"
#include "stack.h"
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

const char *
ActivityList::lastName() const
{
    throw should_not_implement( "ActivityList::lastName", __FILE__, __LINE__ );
    return 0;
}




/*
 * Return the name of the next item on the list.  For forks/joins, it's the first item
 * in the collection.
 */

const char *
ActivityList::firstName() const
{
    throw should_not_implement( "ActivityList::lastName", __FILE__, __LINE__ );
    return 0;
}




/*
 * Common code for aggregation: reset entry information.
 */

void
ActivityList::initEntry( Entry *dstEntry, const Entry * srcEntry, const unsigned submodel, AggregateFunc aFunc, const double rate ) const
{
    if ( aFunc == &Activity::aggregateServiceTime ) {
        dstEntry->myMaxPhase = max( dstEntry->maxPhase(), srcEntry->maxPhase() );
    } else if ( aFunc == &Activity::setThroughput ) {
        dstEntry->myThroughput = srcEntry->myThroughput * rate;
    } else if ( aFunc == &Activity::aggregateWait ) {
        for ( unsigned p = 1; p <= srcEntry->maxPhase(); ++p ) {
            if ( submodel == 0 ) {
                dstEntry->phase[p].myVariance = 0.0;
            } else {
                dstEntry->phase[p].myWait[submodel] = 0.0;
            }
        }
    } else if ( aFunc == &Activity::aggregateReplication ) {
        for ( unsigned p = 1; p <= srcEntry->maxPhase(); ++p ) {
            dstEntry->phase[p].mySurrogateDelay.size( srcEntry->phase[p].mySurrogateDelay.size() );
            dstEntry->phase[p].mySurrogateDelay = 0.0;
        }
    }
}

/* -------------------------------------------------------------------- */
/*                       Simple Activity Lists                          */
/* -------------------------------------------------------------------- */

int
SequentialActivityList::operator==( const ActivityList& operand ) const
{
    const SequentialActivityList * anOperand = dynamic_cast<const SequentialActivityList *>(&operand);
    if ( anOperand ) {
        return anOperand->myActivity == myActivity;
    }
    return 0;
}


ActivityList&
SequentialActivityList::add( Activity * anActivity )
{
    myActivity = anActivity;
    return *this;
}





/*
 * return the previous name in the list.
 */

const char *
SequentialActivityList::firstName() const
{
    return myActivity->name();
}


/*
 * Print out the next name in the list.
 */

const char *
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
ForkActivityList::configure( const unsigned n, const unsigned p )
{
    if ( myActivity ) {
        myActivity->configure( n, p );
    }
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
ForkActivityList::findChildren( CallStack& callStack, const bool directPath, Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack ) const
{
    if ( myActivity ) {
        return myActivity->findChildren( callStack, directPath, activityStack, forkStack );
    } else {
        return callStack.size();
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
 * Generate interlocking table.
 */

unsigned
ForkActivityList::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
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

void
ForkActivityList::aggregate( Stack<Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned curr_p, unsigned& next_p, AggregateFunc aFunc )
{
    if ( myActivity ) {
        myActivity->aggregate( entryStack, forkList, submodel, curr_p, next_p, aFunc );
    }
}


/*
 * Return the sum of aFunc.
 */

double
ForkActivityList::aggregate2( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, AggregateFunc2 aFunc ) const
{
    if ( myActivity ) {
        return myActivity->aggregate2( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
        return 0.0;
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
ForkActivityList::getInterlockedTasks( Stack<const Entry *>& entryStack, const Entity * myServer,
                                       Cltn<const Entity *>& interlockedTasks, const unsigned last_phase ) const
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
ForkActivityList::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( myActivity ) {
        myActivity->callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate );
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
JoinActivityList::configure( const unsigned n, const unsigned p )
{
    if ( next() ) {
        next()->configure( n, p );
    }
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
JoinActivityList::findChildren( CallStack& callStack, const bool directPath, Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack ) const
{
    if ( next() ) {
        return next()->findChildren( callStack, directPath, activityStack, forkStack );
    } else {
        return callStack.size();
    }
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
 * Link activity.
 */

unsigned
JoinActivityList::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
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
JoinActivityList::getInterlockedTasks( Stack<const Entry *>& entryStack, const Entity * myServer,
                                       Cltn<const Entity *>& interlockedTasks, const unsigned last_phase ) const
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
JoinActivityList::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( next() ) {
        next()->callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate );
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

void
JoinActivityList::aggregate( Stack<Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned curr_p, unsigned& next_p, AggregateFunc aFunc )
{
    if ( next() ) {
        next()->aggregate( entryStack, forkList, submodel, curr_p, next_p, aFunc );
    }
}


/*
 * Return the sum of aFunc.
 */

double
JoinActivityList::aggregate2( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, AggregateFunc2 aFunc ) const
{
    if ( next() ) {
        return next()->aggregate2( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
        return 0.0;
    }
}

/*----------------------------------------------------------------------*/
/*                  Activity lists that fork or join.                   */
/*----------------------------------------------------------------------*/

ForkJoinActivityList::~ForkJoinActivityList()
{
    myActivityList.clearContents();
}


int
ForkJoinActivityList::operator==( const ActivityList& operand ) const
{
    const ForkJoinActivityList * anOperand = dynamic_cast<const ForkJoinActivityList *>(&operand);
    if ( anOperand ) {
        // Check cltns for equivalence.
        return anOperand->myActivityList == myActivityList;
    }
    return 0;
}

/*
 * Add an item to the activity list.
 */

ActivityList&
ForkJoinActivityList::add( Activity * anActivity )
{
    myActivityList << anActivity;
    return *this;
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

const char *
ForkJoinActivityList::firstName() const
{
    return myActivityList[1]->name();
}


/*
 * Return the name of the first activity on the list.  Used for print
 * out join delays.
 */

const char *
ForkJoinActivityList::lastName() const
{
    return myActivityList[myActivityList.size()]->name();
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
    myEntries.deleteContents();
}


/*
 * Configure descendents
 */

void
AndOrForkActivityList::configure( const unsigned n, const unsigned p )
{
    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
        anActivity->configure( n, p );
    }

    Sequence<Entry *> nextEntry( myEntries );
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
        anEntry->configure( n, p );
    }
}



ActivityList&
AndOrForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );
    myEntries.grow(1);

    return *this;
}


/*
 * Link activity.  Current path ends here.
 */

unsigned
AndOrForkActivityList::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    unsigned max_depth = entryStack.size();

/* Now search down lists */

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
        const InterlockInfo newCalls = globalCalls * prBranch(i);
        max_depth = max( anActivity->followInterlock( entryStack, newCalls, callingPhase ), max_depth );
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
AndOrForkActivityList::getInterlockedTasks( Stack<const Entry *>& entryStack, const Entity * myServer,
                                            Cltn<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    bool found = false;

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
        if ( anActivity->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase ) ) {
            found = true;
        }
    }

    return found;
}



/*
 * Common code for aggregating the activities for a branch to its psuedo entry
 */

Entry *
AndOrForkActivityList::aggregateToEntry( const unsigned i, Stack<Entry *>&entryStack, const AndForkActivityList * forkList,
                                         const unsigned submodel, const unsigned curr_p, unsigned& next_p, AggregateFunc aFunc )
{
    Activity * anActivity = myActivityList[i];
    Entry * anEntry = myEntries[i];
    const Entry * currEntry = entryStack.top();

    initEntry( anEntry, currEntry, submodel, aFunc, prBranch(i) );

    entryStack.push( anEntry );
    anActivity->aggregate( entryStack, forkList, submodel, curr_p, next_p, aFunc );
    entryStack.pop();

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

ActivityList&
OrForkActivityList::add( Activity * anActivity )
{
    AndOrForkActivityList::add( anActivity );
    const unsigned n = getMyActivityList().size();

    Entry * anEntry = new VirtualEntry( anActivity );
    assert( anEntry->entryTypeOk(ACTIVITY_ENTRY) );
    anEntry->setStartActivity( anActivity );

    myEntries[n] = anEntry;

    return *this;
}



/*
 * Check that all items in the fork list add to one and they all have probabilities.
 */

void
OrForkActivityList::check() const
{
    double sum = 0.0;
    for ( unsigned j = 1; j <= myActivityList.size(); ++j ) {
	sum += prBranch(j);
    }
    if ( fabs( 1.0 - sum ) > EPSILON ) {
        ForkJoinName aName( *this );
        LQIO::solution_error( LQIO::ERR_MISSING_OR_BRANCH, aName(), owner()->name(), sum );
    }
}


double
OrForkActivityList::prBranch( const unsigned int i ) const
{
    return getDOM()->getParameterValue(dynamic_cast<const LQIO::DOM::Activity *>(myActivityList[i]->getDOM()));
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
OrForkActivityList::findChildren( CallStack& callStack, const bool directPath, Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack ) const
{
    unsigned max_depth = callStack.size();

    /* Now search down lists */

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
        max_depth = max( anActivity->findChildren( callStack, directPath, activityStack, forkStack ), max_depth );
    }

    return max_depth;
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
 * Recursively aggregate using 'aFunc' along all branches of the or, storing
 * the results of the aggregation in the virtual entry assigned to each branch.
 * (except if aFunc == setThroughput).  Mean and variance is needed to
 * find the variance of the aggregate.
 */

void
OrForkActivityList::aggregate( Stack<Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned curr_p, unsigned& next_p, AggregateFunc aFunc )
{
    const unsigned n = myActivityList.size();
    Entry * currEntry = entryStack.top();
    unsigned branch_p;

    /* Now search down lists */

    if ( aFunc == &Activity::aggregateWait ) {
        Vector<Vector<Exponential> > branch( n );
        Vector<Exponential> sum( currEntry->maxPhase() );

        for ( unsigned i = 1; i <= n; ++i ) {
            branch_p = curr_p;
            Entry * anEntry = aggregateToEntry( i, entryStack, forkList, submodel, curr_p, branch_p, aFunc );

            branch[i].grow( currEntry->maxPhase() );
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                if ( submodel == 0 ) {
		    const double s =  anEntry->elapsedTime(p);
		    if ( !isfinite( s ) ) continue;			/* Ignore bogus branches */
                    branch[i][p].init( s, anEntry->variance(p) );
                    for ( unsigned j = 1; j <= i; ++j ) {
                        sum[p] += varianceTerm( prBranch(i), branch[i][p], prBranch(j), branch[j][p] );
                    }
                } else {
                    branch[i][p].mean( anEntry->phase[p].myWait[submodel] );
                }
                sum[p] += prBranch(i) * branch[i][p];
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->aggregate( submodel, p, sum[p] );
        }

    } else if ( aFunc == &Activity::aggregateReplication ) {
        Vector< VectorMath<double> > sum(currEntry->maxPhase());
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            sum[p].grow( currEntry->phase[p].mySurrogateDelay.size() );
        }

        for ( unsigned i = 1; i <= n; ++i ) {
            branch_p = curr_p;
            Entry * anEntry = aggregateToEntry( i, entryStack, forkList, submodel, curr_p, branch_p, aFunc );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                VectorMath<double> branch = anEntry->phase[p].mySurrogateDelay;
                branch *= prBranch(i);
                sum[p] += branch;
            }
        }
        currEntry->aggregateReplication( sum );

    } else if ( aFunc == &Activity::aggregateServiceTime ) {
        VectorMath<double> sum( currEntry->maxPhase() );

        for ( unsigned i = 1; i <= n; ++i ) {
            branch_p = curr_p;
            Entry * anEntry = aggregateToEntry( i, entryStack, forkList, submodel, curr_p, branch_p, aFunc );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] += prBranch(i) * anEntry->serviceTime(p);
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->addServiceTime( p, sum[p] );
        }

    } else {
        for ( unsigned i = 1; i <= n; ++i ) {
            branch_p = curr_p;
            aggregateToEntry( i, entryStack, forkList, submodel, curr_p, branch_p, aFunc );
        }
    }
}



/*
 * Return the sum of aFunc.
 */

double
OrForkActivityList::aggregate2( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, AggregateFunc2 aFunc ) const
{
    double sum = 0.0;

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    next_p = curr_p;
    for ( unsigned i = 1; anActivity = nextActivity(); ++i  ) {
        unsigned branch_p = curr_p;
        sum += anActivity->aggregate2( anEntry, curr_p, branch_p, rate * prBranch(i), activityStack, aFunc );
        next_p = max( next_p, branch_p );
    }
    return sum;
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
OrForkActivityList::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
        anActivity->callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate * prBranch(i) );
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

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
        n = max( anActivity->concurrentThreads( m ), n );
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

ActivityList&
AndForkActivityList::add( Activity * anActivity )
{
    AndOrForkActivityList::add( anActivity );
    
    const unsigned n = myEntries.size();
    Thread * aThread = new Thread( anActivity, this );
    assert( aThread->entryTypeOk(ACTIVITY_ENTRY) );
    aThread->setStartActivity( anActivity );
    myEntries[n] = aThread;

    Task * aTask = const_cast<Task *>(dynamic_cast<const Task *>(anActivity->owner()));    /* Downcase/unconst */

    aTask->addThread( aThread );

    return *this;
}


/*
 * Check that all items in the fork list for this join are the same.
 * If they are zero, tag this join as a synchronization point.
 */

void
AndForkActivityList::check() const
{
    if ( myJoinList ) {
        myJoinList->check();
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
AndForkActivityList::findChildren( CallStack& callStack, const bool directPath, Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack ) const
{
    unsigned max_depth = callStack.size();
    if ( forkStack.size() ) {
        myParent = forkStack.top();
    } else {
        myParent = 0;
    }
    forkStack.push( this );

    /* Now search down lists */

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    try {
        while ( anActivity = nextActivity() ) {

            if ( Options::Debug::forks() ) {
                cerr << "AndFork: " << setw( activityStack.size() ) << " " << activityStack.top()->name()
                     << " -> " << anActivity->name() << endl;
            }

            max_depth = max( anActivity->findChildren( callStack, directPath, activityStack, forkStack ), max_depth );
        }
    } 
    catch ( bad_internal_join& error ) {
	LQIO::solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, firstName(), owner()->name(), error.what() );
        max_depth = max( max_depth, error.depth() );
    }
    forkStack.pop();
    return max_depth;
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

void
AndForkActivityList::aggregate( Stack<Entry *>& entryStack, const AndForkActivityList * forkList,
                                const unsigned submodel, const unsigned curr_p, unsigned& next_p,
                                AggregateFunc aFunc )
{
    if (flags.trace_quorum) {
        cout <<"\nAndForkActivityList::aggregate()...the start --------------- : submodel = " << submodel <<  endl;
    }

    // Start tomari: Quorum
    //The current code assumes that every AndFork corresponds to an AndJoin.
    //So the aggregation is done only in the ForkList.

    const unsigned n = myActivityList.size();
    Entry * currEntry = entryStack.top();
    unsigned branch_p;

    if ( aFunc == &Activity::aggregateWait ) {
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
                time += currEntry->phase[curr_p].myWait[submodel]; /* Pick off time for this pass. */
            }
        } else {
            time = currEntry->getStartTimeVariance();
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                time += currEntry->phase[curr_p].variance();       /* total isn't known until all activities scanned. */
            }
        }

        /* Now search down lists */

        Exponential phase_one;
        const bool isThereQuorumDelayedThreads = myJoinList && myJoinList->hasQuorum();

        for ( unsigned i = 1; i <= n; ++i ) {
            branch_p = curr_p;
            Thread * anEntry = dynamic_cast<Thread *>(aggregateToEntry( i, entryStack, forkList, submodel, curr_p, branch_p, aFunc ));
            next_p = max( next_p, branch_p );

            Vector<Exponential> branch( currEntry->maxPhase() );

            anEntry->startTime( submodel, time );

            if ( submodel == 0 ) {

                /* Updating variance */

                anEntry->total.myVariance = 0.0;
                DiscretePoints sumTotal;              /* BUG 583 -- we don't care about phase */
                DiscretePoints sumLocal;
                DiscretePoints sumRemote;

                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->total.myVariance += anEntry->Entry::variance(p);
                    if (flags.trace_quorum) {
                        cout <<"\nEntry " << anEntry->name() << ", anEntry->elapsedTime(p="<<p<<")=" << anEntry->elapsedTime(p) << endl;
                        cout << "anEntry->phase[p="<<p<<"].myWait[submodel=1]=" << anEntry->phase[p].myWait[1] << endl;

                        cout << "anEntry->Entry::variance(p="<<p<<"]="<< anEntry->Entry::variance(p) << endl;
                    }

                    branch[p].init( anEntry->elapsedTime(p), anEntry->Entry::variance(p) );
                    sumTotal += branch[p];            /* BUG 583 */
                }
                // tomari: first possible update for Quorum.

                myActivityList[i]->estimateQuorumJoinCDFs(sumTotal, quorumCDFs,
                                                          localCDFs, remoteCDFs,
                                                          isThereQuorumDelayedThreads,
                                                          isQuorumDelayedThreadsActive,
                                                          totalParallelLocal,
                                                          totalSequentialLocal);

            } else if ( submodel == Model::sync_submodel ) {

                /* Updating join delays */

                anEntry->total.myWait[submodel] = 0.0;
                DiscretePoints sumTotal;              /* BUG 583 -- we don't care about phase */
                DiscretePoints sumLocal;
                DiscretePoints sumRemote;

                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->total.myWait[submodel] += anEntry->phase[curr_p].myWait[submodel];
                    if (flags.trace_quorum) {
                        cout <<"\nEntry " << anEntry->name() <<", anEntry->elapsedTime(p="<<p<<")=" <<anEntry->elapsedTime(p) << endl;
                        cout << "anEntry->phase[curr_p="<<curr_p<<"].myWait[submodel="<<2<<"]=" << anEntry->phase[curr_p].myWait[2] << endl;

                    }

                    branch[p].init( anEntry->elapsedTime(p), anEntry->Entry::variance(p) );
                    sumTotal += branch[p];            /* BUG 583 */
                }

                // tomari: second possible update for Quorum
                myActivityList[i]->estimateQuorumJoinCDFs(sumTotal, quorumCDFs,
                                                          localCDFs, remoteCDFs,
                                                          isThereQuorumDelayedThreads,
                                                          isQuorumDelayedThreadsActive,
                                                          totalParallelLocal,
                                                          totalSequentialLocal);

            } else {

                /* Updating the waiting time for this submodel */

                anEntry->total.myWait[submodel] = 0.0;
                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->total.myWait[submodel] += anEntry->phase[curr_p].myWait[submodel];
                    branch[p].init( anEntry->phase[curr_p].myWait[submodel], anEntry->Entry::variance(p) );
                }
            }

            /* Phase change for this branch, so record the time it occurs. */

            if ( branch_p != curr_p ) {
                phase_one = branch[curr_p];
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
            if ( next_p != curr_p ) {
                currEntry->aggregate( submodel, curr_p, phase_one );
                (*join) -= phase_one;
            }
            currEntry->aggregate( submodel, next_p, *join );
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
                cout << currEntry->name() << ", submodel " << submodel << ", phase " << curr_p << " wait: " << endl;
            }

            /* Update quorumJoin delay for parent.  Set variance for this fork/join.  */

            if ( next_p != curr_p ) {
                if ( flags.trace_activities ) {
                    cout << *join << ", phase " << next_p << " wait: ";
                }

                /* we've encountered a phase change, so try to estimate the phases.  */

                currEntry->aggregate( submodel, curr_p, phase_one );
                (*join) -= phase_one;
            }

            if ( flags.trace_activities ) {
                cout << *join << endl;
            }
            currEntry->aggregate( submodel, next_p, *join );
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

    } else if ( aFunc == &Activity::aggregateServiceTime ) {

        /* For service time, just sum everthing up. */

        VectorMath<double> sum( currEntry->maxPhase() );
        for ( unsigned i = 1; i <= n; ++i ) {
            branch_p = curr_p;
            Entry * anEntry = aggregateToEntry( i, entryStack, forkList, submodel, curr_p, branch_p, aFunc );
            next_p = max( next_p, branch_p );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] += anEntry->serviceTime(p);
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->addServiceTime( p, sum[p] );
        }

    } else {

        for ( unsigned i = 1; i <= n; ++i ) {
            branch_p = curr_p;
            aggregateToEntry( i, entryStack, forkList, submodel, curr_p, branch_p, aFunc );
            next_p = max( next_p, branch_p );
        }
    }

    /* Now follow the activities after the quorumJoin */

    if ( myJoinList && myJoinList->next() ) {
        myJoinList->next()->aggregate( entryStack, forkList, submodel, next_p, next_p, aFunc );
    }

    if (flags.trace_quorum) {
        cout <<"AndForkActivityList::aggregate()...the end --------------- : submodel = " << submodel <<  endl;
    }
}


//Get the CDF of the joint dsitribution for a K out of N quorum

DiscretePoints *
AndForkActivityList::calcQuorumKofN( const unsigned submodel,
                                     bool isQuorumDelayedThreadsActive,
                                     DiscreteCDFs & quorumCDFs ) const
{
    const unsigned n = myActivityList.size();
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
        for ( unsigned i = 1; i <= n; ++i ) {
            Entry * anEntry = myEntries[i];
            if ( dynamic_cast<Thread *>(anEntry) ) {
                dynamic_cast<Thread *>(anEntry)->joinDelay(join->mean());
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
    const unsigned n = myActivityList.size();
    bool anError= false;
    Activity * localQuorumDelayActivity = NULL;
    Task * myTask = (Task *) (myActivityList[1]->owner());
    Entry * currEntry = entryStack.top();

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
            procEntry->phase[1].myWait[orgSubmodel] = (1-probQuorumDelaySeqExecution) * localDiffJoin.mean() ;
        } else {
            localQuorumDelayActivity->setServiceTime(0);
	    procEntry->setServiceTime( 0. ).setCV_sqr( 1. );
            procEntry->phase[1].myWait[orgSubmodel] = 0.;
        }

        if (flags.trace_quorum) {
            cout <<" procEntry->phase[1].myWait[orgSubmodel="<<orgSubmodel<<"]="<< procEntry->phase[1].myWait[orgSubmodel] << endl;
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

double
AndForkActivityList::aggregate2( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, AggregateFunc2 aFunc ) const
{
    double sum = 0.0;
    const double branch_rate = (myJoinList && myJoinList->hasQuorum()) ? 0.0 : rate;    /* Disallow replies on quorum branches */

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    for ( unsigned i = 1; anActivity = nextActivity(); ++i  ) {
        unsigned branch_p = curr_p;
        sum += anActivity->aggregate2( anEntry, curr_p, branch_p, branch_rate, activityStack, aFunc );
	next_p = max( next_p, branch_p );
    }

    /* Now follow the activities after the quorumJoin */

    if ( myJoinList && myJoinList->next() ) {
        sum += myJoinList->next()->aggregate2( anEntry, next_p, next_p, rate, activityStack, aFunc );
    } else {
        const_cast<Entry *>(anEntry)->setMaxPhase( next_p  );   /* Flushing */
    }
    return sum;
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait.
 * Only follow the root path.  Other paths in the AND-FORK are followed
 * by their thread.
 */

void
AndForkActivityList::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( myJoinList && myJoinList->next() ) {
        myJoinList->next()->callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
AndForkActivityList::concurrentThreads( unsigned n ) const
{
    unsigned m = 0;

/* Now search down lists */

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
        m += anActivity->concurrentThreads( 1 );
    }
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

void
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
AndOrJoinActivityList::configure( const unsigned n, const unsigned p )
{
    if ( next() ) {
        visits += 1;
        if ( visits == 1 ) {
            next()->configure( n, p );
        }
        if ( visits == myActivityList.size() ) {
            visits = 0;
        }
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

/* -------------------------------------------------------------------- */
/*                      Or Join Activity Lists                          */
/* -------------------------------------------------------------------- */

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
OrJoinActivityList::findChildren( CallStack& callStack, const bool directPath, Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack ) const
{
    if ( next() ) {
        return next()->findChildren( callStack, directPath, activityStack, forkStack );
    }
    return callStack.size();
}



/*
 * Follow the path.  We don't care about other paths.
 */

unsigned
OrJoinActivityList::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
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
OrJoinActivityList::getInterlockedTasks( Stack<const Entry *>& entryStack, const Entity * myServer,
                                         Cltn<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    if ( next() ) {
        visits += 1;
        if ( visits == myActivityList.size() ) {
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
OrJoinActivityList::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( next() ) {
        next()->callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * Follow the path backwards.  Used to set path lists for joins.
 */

void
OrJoinActivityList::aggregate( Stack<Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned curr_p, unsigned& next_p, AggregateFunc aFunc )
{
    if ( next() ) {
        next()->aggregate( entryStack, forkList, submodel, curr_p, next_p, aFunc );
    }
}



/*
 * Return the sum of aFunc.
 */

double
OrJoinActivityList::aggregate2( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, AggregateFunc2 aFunc )  const
{
    if ( next() ) {
        return next()->aggregate2( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
        return 0.0;
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
        if ( visits == myActivityList.size() ) {
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

ActivityList&
AndJoinActivityList::add( Activity * anActivity )
{
    AndOrJoinActivityList::add( anActivity );
    myForkList.grow(1);
    myForkList[myForkList.size()] = 0;
    mySrcList.grow(1);
    mySrcList[mySrcList.size()] = 0;
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

void
AndJoinActivityList::check() const
{
    for ( unsigned j = 2; j <= myActivityList.size(); ++j ) {
        if ( myForkList[1] != myForkList[j] ) {
            ForkJoinName aName( *this );
            LQIO::solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, aName(), owner()->name() );
            break;
        }
    }
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
AndJoinActivityList::findChildren( CallStack& callStack, const bool directPath, Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack ) const
{
    /* Look for the fork on the fork stack */

    for ( unsigned i = 1; i <= myActivityList.size(); ++i ) {

        if ( myActivityList[i] != activityStack.top() ) {
            unsigned j = myActivityList[i]->backtrack( forkStack );
            if ( j ) {
                if ( !const_cast<AndJoinActivityList *>(this)->joinType( INTERNAL_FORK_JOIN  ) ) {
                    throw bad_internal_join( activityStack );
                } else if ( !myForkList[i] || forkStack.find( myForkList[i] ) ) {
                    myForkList[i] = forkStack[j];
                    myForkList[i]->myJoinList = this;      /* Random choice :-) */
                }
                if ( Options::Debug::forks() ) {
                    cerr << "AndJoin: " << setw( activityStack.size() ) << " " << activityStack.top()->name()
                         << ": " << myActivityList[i]->name();
                    for ( unsigned k = 1; k <= forkStack.size(); ++k ) {
                        if ( j == k ) {
                            cerr << " [" << forkStack[k]->prev()->firstName() << "]";
                        } else {
                            cerr << " " << forkStack[k]->prev()->firstName();
                        }
                    }
                    cerr << endl;
                }
            } else {
                if ( !const_cast<AndJoinActivityList *>(this)->joinType( SYNCHRONIZATION_POINT ) ) {
                    throw bad_internal_join( activityStack );
                } else if ( !addToSrcList( i, activityStack.top() ) ) {
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
        return callStack.size();
    }
}



/*
 * Add anActivity to the activity list provided it isn't there already
 * and the slot that it is to go in isn't already occupied.
 */

bool
AndJoinActivityList::addToSrcList( unsigned i, const Activity * anActivity ) const
{
    if ( mySrcList[i] != 0 && mySrcList[i] != anActivity ) {
        return false;
    } else {
        mySrcList[i] = anActivity;
    }

    for ( unsigned j = 1; j <= mySrcList.size(); ++j ) {
        if ( j != i && mySrcList[j] == anActivity ) return false;
    }
    return true;
}



/*
 * Follow the path.  We don't care about other paths.
 */

unsigned
AndJoinActivityList::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    if ( next() ) {
        visits += 1;
        if ( visits == myActivityList.size() || isSynchPoint() ) {
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
AndJoinActivityList::getInterlockedTasks( Stack<const Entry *>& entryStack, const Entity * myServer,
                                          Cltn<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    if ( next() ) {
        visits += 1;
        if ( visits == myActivityList.size() || isSynchPoint() ) {
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
AndJoinActivityList::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    if ( isSynchPoint() && next() ) {
        next()->callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate );
    }
}



/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.
 * Otherwise, press on.
 */

void
AndJoinActivityList::aggregate( Stack<Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned curr_p, unsigned& next_p, AggregateFunc aFunc )
{
    if ( isSynchPoint() && next() ) {
        next()->aggregate( entryStack, forkList, submodel, curr_p, next_p, aFunc );
    }
}



/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.
 * Otherwise, press on.
 */

double
AndJoinActivityList::aggregate2( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, AggregateFunc2 aFunc ) const
{
    if ( isSynchPoint() && next() ) {
        return next()->aggregate2( anEntry, curr_p, next_p, rate, activityStack, aFunc );
    } else {
        return 0.0;
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
    myActivityList.clearContents();
    myEntries.deleteContents();
}




/*
 * Configure descendents
 */

void
RepeatActivityList::configure( const unsigned n, const unsigned p )
{
    ForkActivityList::configure( n, p );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
        anActivity->configure( n, p );
    }

    Sequence<Entry *> nextEntry( myEntries );
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
        anEntry->configure( n, p );
    }
}



/*
 * Add a sublist.
 */

ActivityList&
RepeatActivityList::add( Activity * anActivity )
{
    const LQIO::DOM::ExternalVariable * arg = const_cast<LQIO::DOM::ActivityList *>(getDOM())->getParameter(const_cast<LQIO::DOM::Activity *>(anActivity->getDOM()));
    if ( arg ) {

	myEntries.grow(1);
        myActivityList << anActivity;

	Entry * anEntry = new VirtualEntry( anActivity );
        assert( anEntry->entryTypeOk(ACTIVITY_ENTRY) );
        anEntry->setStartActivity( anActivity );
	myEntries[myEntries.size()] = anEntry;

    } else {

        /* End of list */
        ForkActivityList::add( anActivity );

    } 

    return *this;
}


double 
RepeatActivityList::rateBranch( const unsigned int i ) const
{
    return getDOM()->getParameterValue(dynamic_cast<const LQIO::DOM::Activity *>(myActivityList[i]->getDOM()));
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
RepeatActivityList::findChildren( CallStack& callStack, const bool directPath, Stack<const Activity *>& activityStack, Stack<const AndForkActivityList *>& forkStack ) const
{
    unsigned max_depth = ForkActivityList::findChildren( callStack, directPath, activityStack, forkStack );
    const unsigned size = owner()->activities().size();

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
        Stack<const AndForkActivityList *> branchForkStack( size );    // For matching forks/joins.
        max_depth = max( anActivity->findChildren( callStack, directPath, activityStack, branchForkStack ), max_depth );
    }

    return max_depth;
}



/*
 * Link activity.
 */

unsigned
RepeatActivityList::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls, const unsigned callingPhase ) const
{
    unsigned max_depth = ForkActivityList::followInterlock( entryStack, globalCalls, callingPhase );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
        const InterlockInfo newCalls = globalCalls * rateBranch(i);
        max_depth = max( anActivity->followInterlock( entryStack, newCalls, callingPhase ), max_depth );
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
RepeatActivityList::getInterlockedTasks( Stack<const Entry *>& entryStack, const Entity * myServer,
                                         Cltn<const Entity *>& interlockedTasks, const unsigned last_phase ) const
{
    bool found = ForkActivityList::getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
        if ( anActivity->getInterlockedTasks( entryStack, myServer, interlockedTasks, last_phase ) ) {
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
RepeatActivityList::callsPerform( Stack<const Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned k, const unsigned p, callFunc aFunc, const double rate ) const
{
    ForkActivityList::callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;

    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
        anActivity->callsPerform( entryStack, forkList, submodel, k, p, aFunc, rate * rateBranch(i) );
    }
}



/*
 * Recursively aggregate using 'aFunc' along all branches of the or, storing
 * the results of the aggregation in the virtual entry assigned to each branch.
 */

void
RepeatActivityList::aggregate( Stack<Entry *>& entryStack, const AndForkActivityList * forkList, const unsigned submodel, const unsigned curr_p, unsigned& next_p, AggregateFunc aFunc )
{
    ForkActivityList::aggregate( entryStack, forkList, submodel, curr_p, next_p, aFunc );
    Entry * currEntry = entryStack.top();

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
        initEntry( myEntries[i], currEntry, submodel, aFunc, rateBranch(i) );
        unsigned branch_p;

        entryStack.push( myEntries[i] );
        anActivity->aggregate( entryStack, forkList, submodel, curr_p, branch_p, aFunc );
        entryStack.pop();

        if ( aFunc == &Activity::aggregateWait ) {
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                Exponential branch;
                Exponential sum;
                if ( submodel == 0 ) {
                    branch.init( myEntries[i]->elapsedTime(p), myEntries[i]->variance(p) );
                } else {
                    branch.mean( myEntries[i]->phase[p].myWait[submodel] );
                }
                sum = rateBranch(i) * branch + varianceTerm( branch );
                currEntry->aggregate( submodel, p, sum );
            }

        } else if ( aFunc == &Activity::aggregateReplication ) {
            Vector< VectorMath<double> > sum(currEntry->maxPhase());
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] = myEntries[i]->phase[p].mySurrogateDelay;
                sum[p] *= rateBranch(i);
            }
            currEntry->aggregateReplication( sum );

        } else if ( aFunc == &Activity::aggregateServiceTime ) {
	    currEntry->addServiceTime( curr_p, rateBranch(i) * myEntries[i]->serviceTime(curr_p) );
        }
    }
}


/*
 * Return the sum of aFunc.
 * Rate is set to zero for branches so that we don't count replies.
 */

double
RepeatActivityList::aggregate2( const Entry * anEntry, const unsigned curr_p, unsigned& next_p, const double rate, Stack<const Activity *>& activityStack, AggregateFunc2 aFunc ) const
{
    double sum = ForkActivityList::aggregate2( anEntry, curr_p, next_p, rate, activityStack, aFunc );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
        unsigned branch_p = curr_p;
        sum += anActivity->aggregate2( anEntry, curr_p, branch_p, 0.0, activityStack, aFunc );
    }
    return sum;
}




/*
 * Get the number of concurrent threads
 */

unsigned
RepeatActivityList::concurrentThreads( unsigned n ) const
{
    n = ForkActivityList::concurrentThreads( n );

    Sequence<Activity *> nextActivity(myActivityList);
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
        n = max( n, anActivity->concurrentThreads( n ) );
    }
    return n;
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



bad_external_join::bad_external_join( const Stack<const Activity *>& activityStack )
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
