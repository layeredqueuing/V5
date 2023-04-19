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
 * $Id: actlist.cc 16676 2023-04-19 11:56:50Z greg $
 * ------------------------------------------------------------------------
 */


#include "lqns.h"
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <lqio/error.h>
#include <lqio/dom_actlist.h>
#include "activity.h"
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "entrythread.h"
#include "errmsg.h"
#include "flags.h"
#include "model.h"
#include "option.h"
#include "pragma.h"
#include "processor.h"
#include "submodel.h"
#include "task.h"

/* -------------------------------------------------------------------- */
/*               Activity Lists -- Abstract Superclass                  */
/* -------------------------------------------------------------------- */


ActivityList::ActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : _task(owner),
      _dom(dom)
{
    owner->addPrecedence(this);
}


ActivityList::ActivityList( const ActivityList& src, const Task * owner, unsigned int )
    : _task(owner),	// This will be the replica.
      _dom(src._dom)
{
}

/*
 * Return the next link.
 */

ActivityList*
ActivityList::next() const
{
    throw LQIO::should_not_implement( "ActivityList::next" );
    return nullptr;
}


/*
 * Return the previous link.
 */

ActivityList*
ActivityList::prev() const
{
    throw LQIO::should_not_implement( "ActivityList::prev" );
    return nullptr;
}


/*
 * Set the next link.
 */

ActivityList&
ActivityList::next( ActivityList * )
{
    throw LQIO::should_not_implement( "ActivityList::next" );
    return *this;
}


/*
 * Set the prev link.
 */

ActivityList&
ActivityList::prev( ActivityList * )
{
    throw LQIO::should_not_implement( "ActivityList::addSubList" );
    return *this;
}


/*
 * Connect the src and dst lists together.
 */

/* static */ void
ActivityList::connect( ActivityList * src, ActivityList * dst )
{
    if ( src ) {
        src->next( dst  );
    }
    if ( dst ) {
        dst->prev( src );
    }
}

/* -------------------------------------------------------------------- */
/*                       Simple Activity Lists                          */
/* -------------------------------------------------------------------- */

SequentialActivityList::SequentialActivityList( const SequentialActivityList& src, const Task * task, unsigned int replica )
    : ActivityList( src, task, replica ),
      _activity( task->findActivity( src._activity->name() ) )	/* Link to replica */
{
}

bool
SequentialActivityList::operator==( const ActivityList& operand ) const
{
    const SequentialActivityList * anOperand = dynamic_cast<const SequentialActivityList *>(&operand);
    if ( anOperand ) {
        return anOperand->_activity == _activity;
    }
    return false;
}


SequentialActivityList&
SequentialActivityList::add( Activity * anActivity )
{
    _activity = anActivity;
    return *this;
}

/* -------------------------------------------------------------------- */
/*                       Simple Forks (rvalues)                         */
/* -------------------------------------------------------------------- */

ForkActivityList::ForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : SequentialActivityList( owner, dom ),
      _prev(nullptr)
{
}


ForkActivityList::ForkActivityList( const ForkActivityList& src, const Task* task, unsigned int replica )
    : SequentialActivityList( src, task, replica ),
      _prev(nullptr)
{
    getActivity()->prevFork( this );	/* Link Activity to list 	*/
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
ForkActivityList::findChildren( Activity::Children& path ) const
{
    if ( getActivity() ) {
        return getActivity()->findChildren( path );
    } else {
        return path.depth();
    }
}



/*
 * Generate interlocking table.
 */

void
ForkActivityList::followInterlock( Interlock::CollectTable& path ) const
{
    if ( getActivity() ) getActivity()->followInterlock( path );
}



/*
 * Follow the path backwards.  Used to set path lists for joins.
 */

Activity::Collect&
ForkActivityList::collect( std::deque<const Activity *>& activityStack, std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    if ( getActivity() ) {
        return getActivity()->collect( activityStack, entryStack, data );
    } else {
	return data;
    }
}


/*
 * Return the sum of aFunc.
 */

const Activity::Count_If&
ForkActivityList::count_if( std::deque<const Activity *>& stack, Activity::Count_If& data ) const
{
    if ( getActivity() ) {
        return getActivity()->count_if( stack, data );
    } else {
        return data;
    }
}


/*
 * Collect the calls by phase for the interlocker.
 */

CallInfo::Item::collect_calls&
ForkActivityList::collect_calls( std::deque<const Activity *>& stack, CallInfo::Item::collect_calls& data ) const
{
    if ( getActivity() ) {
        return getActivity()->collect_calls( stack, data );
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
ForkActivityList::getInterlockedTasks( Interlock::CollectTasks& path ) const
{
    return getActivity() != nullptr && getActivity()->getInterlockedTasks( path );
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
ForkActivityList::callsPerform( Call::Perform& operation ) const
{
    if ( getActivity() ) getActivity()->callsPerform( operation );
}



/*
 * Get the number of concurrent threads
 */

unsigned
ForkActivityList::concurrentThreads( unsigned n ) const
{
    return getActivity() ? getActivity()->concurrentThreads( n ) : n;
}

/* -------------------------------------------------------------------- */
/*                      Simple Joins (lvalues)                          */
/* -------------------------------------------------------------------- */

JoinActivityList::JoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : SequentialActivityList( owner, dom ),
      _next(nullptr)
{
}


JoinActivityList::JoinActivityList( const JoinActivityList& src, const Task* task, unsigned int replica )
    : SequentialActivityList( src, task, replica ),
      _next(nullptr)
{
    getActivity()->nextJoin( this );	/* Link Activity to list 	*/
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
JoinActivityList::findChildren( Activity::Children& path ) const
{
    return  next() != nullptr ? next()->findChildren( path ) : path.depth();
}



/*
 * Link activity.
 */

void
JoinActivityList::followInterlock( Interlock::CollectTable& path ) const
{
    if ( next() != nullptr ) return next()->followInterlock( path );
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
JoinActivityList::getInterlockedTasks( Interlock::CollectTasks& path ) const
{
    return next() != nullptr && next()->getInterlockedTasks( path );
}


/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
JoinActivityList::callsPerform( Call::Perform& operation ) const
{
    if ( next() != nullptr ) next()->callsPerform( operation );
}



/*
 * Get the number of concurrent threads
 */

unsigned
JoinActivityList::concurrentThreads( unsigned n ) const
{
    return next() != nullptr ? next()->concurrentThreads( n ) : n;
}




/*
 * Follow the path backwards.  Used to set path lists for joins.
 */

Activity::Collect&
JoinActivityList::collect( std::deque<const Activity *>& activityStack, std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    return next() != nullptr ? next()->collect( activityStack, entryStack, data ) : data;
}


/*
 * Return the sum of aFunc.
 */

const Activity::Count_If&
JoinActivityList::count_if( std::deque<const Activity *>& stack, Activity::Count_If& data ) const
{
    return next() != nullptr ? next()->count_if( stack, data ) : data;
}


/*
 * Collect the calls by phase for the interlocker.
 */

CallInfo::Item::collect_calls&
JoinActivityList::collect_calls( std::deque<const Activity *>& stack, CallInfo::Item::collect_calls& data ) const
{
    return next() != nullptr ? next()->collect_calls( stack, data ) : data;
}

/*----------------------------------------------------------------------*/
/*                  Activity lists that fork or join.                   */
/*----------------------------------------------------------------------*/

ForkJoinActivityList::ForkJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : ActivityList( owner, dom ),
      _activities()
{
}

/*
 * Clone the activity List
 */

ForkJoinActivityList::ForkJoinActivityList( const ForkJoinActivityList& src, const Task * task, unsigned int replica )
    : ActivityList( src, task, replica ),
      _activities()
{
    for ( std::vector<const Activity *>::const_iterator activity = src.activities().begin(); activity != src.activities().end(); ++activity ) {
	_activities.push_back( task->findActivity( (*activity)->name() ) );
    }
}


// Check cltns for equivalence.

bool
ForkJoinActivityList::operator==( const ActivityList& operand ) const
{
    const ForkJoinActivityList * anOperand = dynamic_cast<const ForkJoinActivityList *>(&operand);
    return anOperand && anOperand->_activities == _activities;
}

/*
 * Add an item to the activity list.
 */

ForkJoinActivityList&
ForkJoinActivityList::add( Activity * anActivity )
{
    _activities.push_back( anActivity );
    return *this;
}

/* -------------------------------------------------------------------- */
/*          Activity Lists that fork -- abstract superclass             */
/* -------------------------------------------------------------------- */

AndOrForkActivityList::AndOrForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : ForkJoinActivityList( owner, dom ),
      _parentForkList(nullptr),
      _joins(nullptr),
      _prev(nullptr)
{
}



AndOrForkActivityList::AndOrForkActivityList( const AndOrForkActivityList& src, const Task * owner, unsigned int replica )
    : ForkJoinActivityList( src, owner, replica ),
      _parentForkList(nullptr),
      _joins(nullptr),
      _prev(nullptr)
{
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	const_cast<Activity *>(*activity)->prevFork( this );	/* Link activity to this list	*/
    }
}



/*
 * Clone the virtual entries.
 */

VirtualEntry *
AndOrForkActivityList::cloneVirtualEntry( const Entry * src, const Task * owner, unsigned int replica ) const
{
    VirtualEntry * dst = dynamic_cast<VirtualEntry *>(src->clone( replica, this ));
    dst->owner( owner );
    if ( src->hasStartActivity() ) {
	Activity * activity = owner->findActivity( src->getStartActivity()->name() );
	dst->setStartActivity( activity );
	activity->setEntry( dst );
    }
    return dst;
}



/*
 * Free virtual entries.
 */

AndOrForkActivityList::~AndOrForkActivityList()
{
    std::for_each( entries().begin(), entries().end(), Delete<Entry *> );
}



/*
 * Configure descendents
 */

AndOrForkActivityList&
AndOrForkActivityList::configure( const unsigned n )
{
    std::for_each( entries().begin(), entries().end(), Exec1<Entry,const unsigned>( &Entry::configure, n ) );
    return *this;
}


#if PAN_REPLICATION
ActivityList&
AndOrForkActivityList::setSurrogateDelaySize( size_t size )
{
    std::for_each( entries().begin(), entries().end(), Exec1<Entry,size_t>( &Entry::setSurrogateDelaySize, size ) );
    return *this;
}
#endif


ActivityList *
AndOrForkActivityList::getNextFork() const
{
    return joins()->next();
}



bool
AndOrForkActivityList::hasNextFork() const
{
    return joins() != nullptr && joins()->next() != nullptr;
}


/*
 * Check that all items in the fork list for this join are the same.
 * If they are zero, tag this join as a synchronization point.
 */

bool
AndOrForkActivityList::check() const
{
    return true;
}



unsigned
AndOrForkActivityList::findChildren( Activity::Children& path ) const
{
    path.push_fork( this );

    /* Now search down lists */

    unsigned int max_depth = 0;
    try {
	max_depth = std::accumulate( activities().begin(), activities().end(), max_depth, find_children( *this, path ) );
    }
    catch ( const bad_internal_join& error ) {
	getDOM()->runtime_error( LQIO::ERR_FORK_JOIN_MISMATCH, error.getDOM()->getListTypeName().c_str(), error.what(), error.getDOM()->getLineNumber() );
    }
    path.pop_fork();
    return max_depth;
}



/*
 * Link activity.  Current path ends here.
 */

void
AndOrForkActivityList::followInterlock( Interlock::CollectTable& path ) const
{
    std::for_each( activities().begin(), activities().end(), follow_interlock( *this, path ) );
    if ( hasNextFork() ) getNextFork()->followInterlock( path );
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
AndOrForkActivityList::getInterlockedTasks( Interlock::CollectTasks& path ) const
{
    bool found = std::count_if( activities().begin(), activities().end(), Predicate1<Activity,Interlock::CollectTasks&>( &Activity::getInterlockedTasks, path ) ) > 0;
    if ( hasNextFork() && getNextFork()->getInterlockedTasks( path ) ) found = true;

    return found;
}



/*
 * Common code for aggregating the activities for a branch to its psuedo entry
 */

VirtualEntry *
AndOrForkActivityList::collectToEntry( const Activity * activity, VirtualEntry * entry, std::deque<const Activity *>& activityStack, std::deque<Entry *>&entryStack, Activity::Collect& branch )
{
    branch.setRate( branch.rate() * prBranch(activity) );

    entry->set( entryStack.back(), branch );

    entryStack.push_back( entry );
    activity->collect( activityStack, entryStack, branch );
    entryStack.pop_back();

    return entry;
}



std::ostream&
AndOrForkActivityList::printSubmodelWait( std::ostream& output, unsigned offset ) const
{
    std::for_each( entries().begin(), entries().end(), ConstPrint1<Entry,unsigned>( &Entry::printSubmodelWait, output, offset ) );
    return output;
}



unsigned
AndOrForkActivityList::find_children::operator()( unsigned arg1, const Activity * arg2 ) const
{
    if ( Options::Debug::forks() ) {
	if ( dynamic_cast<const AndForkActivityList *>(&_self) ) std::cerr << "And Fork: ";
	else if ( dynamic_cast<const OrForkActivityList *>(&_self) ) std::cerr << "Or Fork:  ";
	else abort();
	const std::deque<const Activity *>& activityStack = _path.getActivityStack();
	std::cerr << std::setw( activityStack.size() ) << " " << activityStack.back()->name()
		  << " -> " << arg2->name() << std::endl;
    }
    Activity::Children path( _path );
    if ( dynamic_cast<const OrForkActivityList *>(&_self) != nullptr ) {
	try {
	    path.setRate(dynamic_cast<const OrForkActivityList *>(&_self)->prBranch( arg2 ) );
	}
	catch ( const std::domain_error& e ) {
	    _self.getDOM()->runtime_error( LQIO::ERR_INVALID_PARAMETER, e.what() );
	}
    }
    return std::max( arg1, arg2->findChildren(path) );
}

/* -------------------------------------------------------------------- */
/*                      Or Fork Activity Lists                          */
/* -------------------------------------------------------------------- */

OrForkActivityList::OrForkActivityList( const OrForkActivityList& src, const Task* owner, unsigned int replica )
    : AndOrForkActivityList( src, owner, replica )
{
    /* Must be done here, rather in super class because constructor will not call subclass */
    for ( std::vector<VirtualEntry *>::const_iterator entry = src.entries().begin(); entry != src.entries().end(); ++entry ) {
	_entries.push_back( cloneVirtualEntry( *entry, owner, replica ) );
    }
}



/*
 * Add an OR-fork.  Also add a "virtual" entry for each branch of the fork.
 * Aggregation along each branch will take place into the virtual entry.
 * This class will aggregate the results from each branch into the parent.
 * See Smith[] for the variance stuff.
 */

OrForkActivityList&
OrForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );

    VirtualEntry * anEntry = new VirtualEntry( anActivity );
    assert( anEntry->entryTypeOk(LQIO::DOM::Entry::Type::ACTIVITY) );
    anEntry->setStartActivity( anActivity );
    _entries.push_back( anEntry );
    assert( _entries.size() == activities().size() );

    return *this;
}



/*
 * Check that all items in the fork list add to one and they all have probabilities.
 */

bool
OrForkActivityList::check() const
{
    AndOrForkActivityList::check();

    const double sum = std::accumulate( activities().begin(), activities().end(), 0.0, add_prBranch( this ) );
    if ( sum < 1.0 - EPSILON || 1.0 + EPSILON < sum ) {
        getDOM()->runtime_error( LQIO::ERR_OR_BRANCH_PROBABILITIES, sum );
	return false;
    }
    return true;
}


Probability
OrForkActivityList::prBranch( const Activity * activity ) const
{
    return getDOM()->getParameterValue(activity->getDOM());
}


/*
 * Recursively aggregate using 'aFunc' along all branches of the or, storing
 * the results of the aggregation in the virtual entry assigned to each branch.
 * (except if aFunc == setThroughput).  Mean and variance is needed to
 * find the variance of the aggregate.
 */

Activity::Collect&
OrForkActivityList::collect( std::deque<const Activity *>& activityStack, std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    const unsigned int n = activities().size();
    const unsigned int submodel = data.submodel();
    Entry * currEntry = entryStack.back();
    unsigned phase = data.phase();
    Activity::Collect::Function f = data.collect();

    /* Now search down lists */

    if ( f == &Activity::collectWait ) {
	std::vector<Vector<Exponential> > term( n );
        Vector<Exponential> sum( currEntry->maxPhase() );

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
	    const Activity * activity = activities().at(i);
            VirtualEntry * anEntry = collectToEntry( activity, entries()[i], activityStack, entryStack, branch );
            phase = std::max( phase, branch.phase() );

            term[i].resize( currEntry->maxPhase() );
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                if ( submodel == 0 ) {
		    const double s =  anEntry->_phase[p].elapsedTime();
		    if ( !std::isfinite( s ) ) continue;			/* Ignore bogus branches */
                    term[i][p].init( s, anEntry->_phase[p].variance() );
                    for ( unsigned j = 0; j < i; ++j ) {
                        sum[p] += varianceTerm( prBranch(activity), term[i][p], prBranch(activities().at(j)), term[j][p] );
                    }
                } else {
                    term[i][p].mean( anEntry->_phase[p].getWaitTime(submodel) );
                }
                sum[p] += prBranch(activity) * term[i][p];
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->aggregate( submodel, p, sum[p] );
        }

#if PAN_REPLICATION
    } else if ( f == &Activity::collectReplication ) {
        Vector< VectorMath<double> > sum(currEntry->maxPhase());
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            sum[p].resize( sum[p].size() + currEntry->_phase[p].getSurrogateDelaySize() );
        }

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
	    const Activity * activity = activities().at(i);
            VirtualEntry * anEntry = collectToEntry( activity, entries()[i], activityStack, entryStack, branch );
            phase = std::max( phase, branch.phase() );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                VectorMath<double>& term = anEntry->_phase[p]._surrogateDelay;
                term *= prBranch(activity);
                sum[p] += term;
            }
        }
        currEntry->aggregateReplication( sum );
#endif

    } else if ( f == &Activity::collectServiceTime ) {
        VectorMath<double> sum( currEntry->maxPhase() );

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
	    const Activity * activity = activities().at(i);
            VirtualEntry * anEntry = collectToEntry( activity, entries()[i], activityStack, entryStack, branch );
            phase = std::max( phase, branch.phase() );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] += prBranch(activity) * anEntry->_phase[p].serviceTime();
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->addServiceTime( p, sum[p] );
        }

    } else {
        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
	    const Activity * activity = activities().at(i);
            collectToEntry( activity, entries()[i], activityStack, entryStack, branch );
            phase = std::max( phase, branch.phase() );
        }
    }

    /* Now follow the activities after the join */

//    assert( phase == data.phase() );		/* Phase change should not occur on branch */

    if ( hasNextFork() ) {
	data.setRate( data.rate() * joins()->getNextRate() );	/* May not be 1. */
        getNextFork()->collect( activityStack, entryStack, data );
    }

    return data;
}



/*
 * Return the sum of aFunc.
 */

const Activity::Count_If&
OrForkActivityList::count_if( std::deque<const Activity *>& stack, Activity::Count_If& data ) const
{
    double sum = 0.0;
    unsigned phase = data.phase();
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	Activity::Count_If branch(data, prBranch(*activity));
	branch = (*activity)->count_if( stack, branch );		/* only want the last one. */
	sum += branch.sum() - data.sum();				/* only accumulate difference */
	phase = std::max( phase, branch.phase() );
    }
//    assert( phase == data.phase() );

    /* Now follow the activities after the join */

    data = sum;
    data.setPhase( phase );
    if ( hasNextFork() ) {
	data.setRate( data.rate() * joins()->getNextRate() );	/* May not be 1. */
	getNextFork()->count_if( stack, data );
    }
    return data;
}



/*
 * Collect the calls by phase for the interlocker.
 */

CallInfo::Item::collect_calls&
OrForkActivityList::collect_calls( std::deque<const Activity *>& stack, CallInfo::Item::collect_calls& data ) const
{
    unsigned phase = data.phase();
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	CallInfo::Item::collect_calls branch(data);
	(*activity)->collect_calls( stack, data );		/* will update data._calls on since _calls is a reference */
	phase = std::max( phase, branch.phase() );
    }
    data.setPhase( phase );
    if ( hasNextFork() ) {
	getNextFork()->collect_calls( stack, data );
    }
    return data;
}


/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
OrForkActivityList::callsPerform( Call::Perform& operation ) const
{
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	Call::Perform g( operation, prBranch(*activity) );
	(*activity)->callsPerform( g );
    }

    if ( hasNextFork() ) {
        getNextFork()->callsPerform( operation );
    }
}



/*
 * Get the number of concurrent threads
 */

unsigned
OrForkActivityList::concurrentThreads( unsigned n ) const
{
    n = std::accumulate( activities().begin(), activities().end(), n, max_using_arg<Activity,const unsigned int>( &Activity::concurrentThreads, n ) );
    return hasNextFork() ? getNextFork()->concurrentThreads( n ) : n;
}

/* -------------------------------------------------------------------- */
/*                      And Fork Activity Lists                         */
/* -------------------------------------------------------------------- */


AndForkActivityList::AndForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom  )
    : AndOrForkActivityList( owner, dom ),
      _joinDelay(0.0),
      _joinVariance(0.0)
{
}


AndForkActivityList::AndForkActivityList( const AndForkActivityList& src, const Task* owner, unsigned int replica )
    : AndOrForkActivityList( src, owner, replica ),
      _joinDelay(0.0),
      _joinVariance(0.0)
{
    /* Must be done here, rather in super class because constructor will not call subclass */
    for ( std::vector<VirtualEntry *>::const_iterator entry = src.entries().begin(); entry != src.entries().end(); ++entry ) {
	_entries.push_back( cloneVirtualEntry( *entry, owner, replica ) );
    }
}



/*
 * Clone the virtual entry (thread) and add the thread to the owner.
 */

VirtualEntry *
AndForkActivityList::cloneVirtualEntry( const Entry * src, const Task * owner, unsigned int replica ) const
{
    Thread * dst = dynamic_cast<Thread *>(AndOrForkActivityList::cloneVirtualEntry( src, owner, replica ));
    const_cast<Task *>(owner)->addThread( dst );
    return dst;
}



/*
 * Add an AND-fork.  Also add a "thread" entry for each branch of the fork.
 * Aggregation along each branch will take place into the thread.  The
 * threads are also used as chains in the MVA models.
 */

AndForkActivityList&
AndForkActivityList::add( Activity * anActivity )
{
    ForkJoinActivityList::add( anActivity );

    Thread * thread = new Thread( anActivity, this );
    assert( thread->entryTypeOk(LQIO::DOM::Entry::Type::ACTIVITY) );
    thread->setStartActivity( anActivity );
    _entries.push_back( thread );
    assert( _entries.size() == activities().size() );

    Task * task = const_cast<Task *>(dynamic_cast<const Task *>(anActivity->owner()));    /* Downcase/unconst */
    task->addThread( thread );

    return *this;
}


/*
 * Check that all items in the fork list for this join are the same.
 * If they are zero, tag this join as a synchronization point.
 */

bool
AndForkActivityList::check() const
{
    return AndOrForkActivityList::check();
}


/*
 * Determine if this fork is a descendent of aParent.
 */

bool
AndForkActivityList::isDescendentOf( const AndForkActivityList * aParent ) const
{
    return _parentForkList == aParent || (_parentForkList != nullptr && _parentForkList->isDescendentOf( aParent ));
}




/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
AndForkActivityList::findChildren( Activity::Children& path ) const
{
    _parentForkList = nullptr;
    const std::deque<const AndOrForkActivityList *>& forkStack = path.getForkStack();
    for ( std::deque<const AndOrForkActivityList *>::const_reverse_iterator forkList = forkStack.rbegin(); forkList != forkStack.rend() && !_parentForkList; ++forkList ) {
    	if ( dynamic_cast<const AndForkActivityList *>(*forkList) != nullptr ) _parentForkList = dynamic_cast<const AndForkActivityList *>(*forkList);
    }

    return AndOrForkActivityList::findChildren( path );
}


/*
 * Check my list for a match on activity.  If so, insert it into the list and keep going up.
 */

void
AndOrForkActivityList::backtrack( Activity::Backtrack& data ) const
{
    data.insert_fork( this );
    prev()->backtrack( data );
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
AndForkActivityList::collect( std::deque<const Activity *>& activityStack, std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    const unsigned int submodel = data.submodel();
    const unsigned n = activities().size();
    Entry * currEntry = entryStack.back();
    unsigned phase = data.phase();
    Activity::Collect::Function f = data.collect();

    if (flags.trace_quorum) {
        std::cout <<"\nAndForkActivityList::collect()...the start --------------- : submodel = " << submodel <<  std::endl;
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
                time += currEntry->_phase[p].getWaitTime(submodel); /* Pick off time for this pass. - (since day 1!) */
            }
        } else {
            time = currEntry->getStartTimeVariance();
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                time += currEntry->_phase[p].variance();       /* total isn't known until all activities scanned. */
            }
        }

        /* Now search down lists */

        Exponential phase_one;
	const AndJoinActivityList * joins = dynamic_cast<const AndJoinActivityList *>(this->joins());
        const bool isThereQuorumDelayedThreads = joins && joins->hasQuorum();

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch( data );
	    const Activity * activity = activities().at(i);
            Thread * anEntry = dynamic_cast<Thread *>(collectToEntry( activity, entries()[i], activityStack, entryStack, branch ));
            phase = std::max( phase, branch.phase() );

            Vector<Exponential> term( currEntry->maxPhase() );

            anEntry->startTime( submodel, time );

            if ( submodel == 0 ) {

                /* Updating variance */

                DiscretePoints sumTotal;              /* BUG 583 -- we don't care about phase */
                DiscretePoints sumLocal;
                DiscretePoints sumRemote;

                anEntry->_total.setVariance( 0.0 );
                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->_total.addVariance( anEntry->_phase[p].variance() );
                    if (flags.trace_quorum) {
                        std::cout <<"\nEntry " << anEntry->name() << ", anEntry->elapsedTime(p="<<p<<")=" << anEntry->_phase[p].elapsedTime() << std::endl;
                        std::cout << "anEntry->phase[p="<<p<<"]._wait[submodel=1]=" << anEntry->_phase[p].getWaitTime(1) << std::endl;
                        std::cout << "anEntry->Entry::variance(p="<<p<<"]="<< anEntry->_phase[p].variance() << std::endl;
                    }

                    term[p].init( anEntry->_phase[p].elapsedTime(), anEntry->_phase[p].variance() );
                    sumTotal += term[p];            /* BUG 583 */
                }
                // tomari: first possible update for Quorum.

                const_cast<Activity *>(activity)->estimateQuorumJoinCDFs(sumTotal, quorumCDFs,
									 localCDFs, remoteCDFs,
									 isThereQuorumDelayedThreads,
									 isQuorumDelayedThreadsActive,
									 totalParallelLocal,
									 totalSequentialLocal);

            } else if ( submodel == Model::syncSubmodel() ) {

                /* Updating join delays */

                DiscretePoints sumTotal;              /* BUG 583 -- we don't care about phase */
                DiscretePoints sumLocal;
                DiscretePoints sumRemote;

                anEntry->_total.setWaitTime( submodel,0.0 );
                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->_total.addWaitTime( submodel, anEntry->_phase[p].getWaitTime( submodel ) );
                    if (flags.trace_quorum) {
                        std::cout <<"\nEntry " << anEntry->name() <<", anEntry->elapsedTime(p="<<p<<")=" <<anEntry->_phase[p].elapsedTime() << std::endl;
//                        std::cout << "anEntry->phase[curr_p="<<curr_p<<"]._wait[submodel="<<2<<"]=" << anEntry->_phase[curr_p]._wait[2] << std::endl;
                    }

                    term[p].init( anEntry->_phase[p].elapsedTime(), anEntry->_phase[p].variance() );
                    sumTotal += term[p];            /* BUG 583 */
                }

                // tomari: second possible update for Quorum
                const_cast<Activity *>(activity)->estimateQuorumJoinCDFs(sumTotal, quorumCDFs,
									 localCDFs, remoteCDFs,
									 isThereQuorumDelayedThreads,
									 isQuorumDelayedThreadsActive,
									 totalParallelLocal,
									 totalSequentialLocal);

            } else {

                /* Updating the waiting time for this submodel */

		    anEntry->_total.setWaitTime(submodel, 0.0 );
                for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                    anEntry->_total.addWaitTime( submodel, anEntry->_phase[p].getWaitTime(submodel) );
                    term[p].init( anEntry->_phase[p].getWaitTime(submodel), anEntry->_phase[p].variance() );
                }
            }

            /* Phase change for this branch, so record the time it occurs. */

            if ( phase != data.phase() ) {
		if ( currEntry->maxPhase() == 1 && phase != data.phase() ) {
		    phase = 1;		/* kick it back because there is no second phase BUG 259 */
		} else {
		    phase_one = term[data.phase()];
		}
	    }
	}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
	double probQuorumDelaySeqExecution = 0;
	if (totalParallelLocal+ totalSequentialLocal > 0) {
	    probQuorumDelaySeqExecution = totalParallelLocal/
		(totalParallelLocal+ totalSequentialLocal);
	}
#endif

	//to disable accounting for sequential execution in the quorum delayed threads,
        //set probQuorumDelaySeqExecution to zero.
        //0probQuorumDelaySeqExecution = 0;
        DiscretePoints * join = calcQuorumKofN( submodel, isQuorumDelayedThreadsActive, quorumCDFs );

        /* Need to compute p1 and p2 delay components */

        if ( submodel == 0 ) {
            _joinDelay = join->mean();
            /* Update Variance for parent. */
            if ( phase != data.phase() ) {
                currEntry->aggregate( submodel, data.phase(), phase_one );
                (*join) -= phase_one;
            }
            currEntry->aggregate( submodel, phase, *join );
        } else if ( submodel == Model::syncSubmodel() ) {
            if (flags.trace_quorum) {
                std::cout << "\n_joinDelay " << join->mean() << std::endl;
                std::cout << "_joinVariance " << join->variance() << std::endl;
            }

            _joinVariance = join->variance();
            if ( flags.trace_activities ) {
                std::cout << "Join delay aggregate to ";
                if ( dynamic_cast<VirtualEntry *>(currEntry) ) {
                    std::cout << " virtual entry ";
                } else {
                    std::cout << " actual entry ";
                }
                std::cout << currEntry->name() << ", submodel " << submodel << ", phase " << data.phase() << " wait: " << std::endl;
            }

            /* Update quorumJoin delay for parent.  Set variance for this fork/join.  */

            if ( phase != data.phase() ) {
                if ( flags.trace_activities ) {
                    std::cout << *join << ", phase " << phase << " wait: ";
                }

                /* we've encountered a phase change, so try to estimate the phases.  */

                currEntry->aggregate( submodel, data.phase(), phase_one );
                (*join) -= phase_one;
            }

            if ( flags.trace_activities ) {
                std::cout << *join << std::endl;
            }

	    currEntry->aggregate( submodel, phase, *join );
        }

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
        if ( dynamic_cast<const AndJoinActivityList *>(joins()) && dynamic_cast<const AndJoinActivityList *>(joins)->hasQuorum()
             && submodel == Model::syncSubmodel()
             && !flags.disable_expanding_quorum_tree /*!pragmaQuorumDistribution.test(DISABLE_EXPANDING_QUORUM)*/
             && Pragma::getQuorumDelayedCalls() == Pragma::KEEP_ALL_QUORUM_DELAYED_CALLS ) {
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
	    const Activity * activity = activities().at(i);
            Entry * anEntry = collectToEntry( activity, entries()[i], activityStack, entryStack, branch );
            phase = std::max( phase, branch.phase() );

            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] += anEntry->_phase[p].serviceTime();
            }
        }
        for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
            currEntry->addServiceTime( p, sum[p] );
        }

    } else {

        for ( unsigned i = 0; i < n; ++i ) {
	    Activity::Collect branch(data);
	    const Activity * activity = activities().at(i);
            collectToEntry( activity, entries()[i], activityStack, entryStack, branch );
            phase = std::max( phase, branch.phase() );
        }
    }

    /* Now follow the activities after the join */

    data.setPhase( phase );
    if ( hasNextFork() ) {
        getNextFork()->collect( activityStack, entryStack, data );
    }

    if (flags.trace_quorum) {
        std::cout <<"AndForkActivityList::collect()...the end --------------- : submodel = " << submodel << std::endl;
    }
    return data;
}


//Get the CDF of the joint distribution for a K out of N quorum

DiscretePoints *
AndForkActivityList::calcQuorumKofN( const unsigned submodel,
                                     bool isQuorumDelayedThreadsActive,
                                     DiscreteCDFs & quorumCDFs ) const
{
    const unsigned n = activities().size();
    DiscretePoints * join;

    if (flags.trace_quorum) {
        std::cout << "\nAndForkActivityList::calcQuorumKofN(): submodel=" <<submodel<< std::endl;
    }
    const AndJoinActivityList * joins = dynamic_cast<const AndJoinActivityList *>(this->joins());
    if ( joins ) {
        if (joins->quorumCount() == 0) {
            const_cast<AndJoinActivityList *>(joins)->quorumCount(n);
        }

        if ( isQuorumDelayedThreadsActive) {
            join = quorumCDFs.quorumKofN( joins->quorumCount() + 1, n + 1 );
            if (flags.trace_quorum) {
                std::cout << "quorum (AndJoin) of " <<joins->quorumCount() + 1
                     << " out of " << n + 1 << std::endl;
            }
        } else {
            join = quorumCDFs.quorumKofN(joins->quorumCount(),n );
            if (flags.trace_quorum) {
                std::cout <<"quorum of " <<joins->quorumCount() << " out of " << n << std::endl;
            }
        }

        if (flags.trace_quorum) {
            std::cout <<"quorumJoin mean=" <<join->mean()
                 << ", Variance=" << join->variance() << std::endl;
        }
    } else {
        /* BUG 327 */
        join = quorumCDFs.quorumKofN( n, n );
    }

    if ( submodel == Model::syncSubmodel() ) {
        for ( std::vector<VirtualEntry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
            if ( dynamic_cast<Thread *>(*entry) ) {
                dynamic_cast<Thread *>(*entry)->joinDelay(join->mean());
            }
        }
    }

    return join;
}


#if HAVE_LIBGSL
bool
AndForkActivityList::saveQuorumDelayedThreadsServiceTime( std::deque<Entry *>& entryStack,
                                                          DiscretePoints & quorumJoin,
                                                          DiscreteCDFs & quorumCDFs,
                                                          DiscreteCDFs & localCDFs,
                                                          DiscreteCDFs & remoteCDFs,
                                                          double probQuorumDelaySeqExecution )
{
    if (flags.trace_quorum) {
        std::cout <<"\n'''''''''''start saveQuorumDelayedThreadsServiceTime'''''''''''''''''''''''" << std::endl;
    }
    const unsigned n = activities().size();
    bool anError= false;
    Activity * localQuorumDelayActivity = nullptr;
    Entry * currEntry = entryStack.back();

    unsigned orgSubmodel = currEntry->owner()->submodel();
    orgSubmodel++;

    DiscretePoints * localQuorumJoin = localCDFs.quorumKofN(dynamic_cast<const AndJoinActivityList *>(joins())->quorumCount(),n );
    DiscretePoints * localAndJoin = localCDFs.quorumKofN( n, n );
    DiscretePoints localDiffJoin;
    localDiffJoin.mean( abs(localAndJoin->mean() -localQuorumJoin->mean()));
    localDiffJoin.variance(abs(localAndJoin->variance() + localQuorumJoin->variance() ));

    if (flags.trace_quorum) {
        std::cout << "localAndJoin->mean() = " << localAndJoin->mean() <<
            ", Variance = " << localAndJoin->variance() << std::endl; ;
        std::cout << "localQuorumJoin->mean() = " << localQuorumJoin->mean() <<
            ", Variance = " << localQuorumJoin->variance() << std::endl;
        std::cout << "localDiffJoin.mean() = " << localDiffJoin.mean() <<
            ", Variance = " << localDiffJoin.variance() << std::endl;
    }
    delete localAndJoin;
    delete localQuorumJoin;

    DiscretePoints * remoteQuorumJoin = remoteCDFs.quorumKofN(dynamic_cast<const AndJoinActivityList *>(joins())->quorumCount(),n );
    DiscretePoints * remoteAndJoin = remoteCDFs.quorumKofN(n,n );
    DiscretePoints remoteDiffJoin;
    remoteDiffJoin.mean(abs(remoteAndJoin->mean() -remoteQuorumJoin->mean()));
    remoteDiffJoin.variance(abs(remoteAndJoin->variance() + remoteQuorumJoin->variance()) );

    if (flags.trace_quorum) {
        std::cout << "\nremoteAndJoin->mean() = " << remoteAndJoin->mean() <<
            ", Variance = " << remoteAndJoin->variance() << std::endl; ;
        std::cout << "remoteQuorumJoin->mean() = " << remoteQuorumJoin->mean() <<
            ", Variance = " << remoteQuorumJoin->variance() << std::endl;
        std::cout << "remoteDiffJoin.mean() = " << remoteDiffJoin.mean() <<
            ", Variance = " << remoteDiffJoin.variance() << std::endl;
    }
    delete remoteAndJoin;
    delete remoteQuorumJoin;

    DiscretePoints * quorumAndJoin = quorumCDFs.quorumKofN(n,n );
    DiscretePoints quorumDiffJoin;
    quorumDiffJoin.mean(abs(quorumAndJoin->mean() -quorumJoin.mean()));
    quorumDiffJoin.variance(abs(quorumAndJoin->variance() + quorumJoin.variance() ));

    if (flags.trace_quorum) {
        std::cout << "\nquorumAndJoin->mean() = " << quorumAndJoin->mean() <<
            ", Variance = " << quorumAndJoin->variance() << std::endl;
        std::cout << "quorumJoin.mean() = " << quorumJoin.mean() <<
            ", Variance = " << quorumJoin.variance() << std::endl;
        std::cout << "quorumDiffJoin.mean() = " << quorumDiffJoin.mean() <<
            ", Variance = " << quorumDiffJoin.variance() << std::endl;
    }
    delete quorumAndJoin;

    char localQuorumDelayActivityName[32];
    sprintf( localQuorumDelayActivityName, "localQmDelay_%d", dynamic_cast<const AndJoinActivityList *>(joins())->quorumListNum() );
    localQuorumDelayActivity = owner()->findActivity(localQuorumDelayActivityName);

    if (localQuorumDelayActivity != nullptr) {

        DeviceEntry * procEntry = dynamic_cast<DeviceEntry *>(const_cast<Entry *>(localQuorumDelayActivity->processorCall()->dstEntry()));

        if (!flags.ignore_overhanging_threads) {
	    double service_time = (1-probQuorumDelaySeqExecution) *  localDiffJoin.mean();
            localQuorumDelayActivity->setServiceTime( service_time );
	    procEntry->setServiceTime( service_time  / localQuorumDelayActivity->numberOfSlices() )
		.setCV_sqr( ((1-probQuorumDelaySeqExecution) * localDiffJoin.variance())/square(service_time) );
            procEntry->_phase[1]._wait[orgSubmodel] = (1-probQuorumDelaySeqExecution) * localDiffJoin.mean() ;
        } else {
            localQuorumDelayActivity->setServiceTime(0);
	    procEntry->setServiceTime( 0. ).setCV_sqr( 1. );
            procEntry->_phase[1]._wait[orgSubmodel] = 0.;
        }

        if (flags.trace_quorum) {
            std::cout <<" procEntry->_phase[1]._wait[orgSubmodel="<<orgSubmodel<<"]="<< procEntry->_phase[1]._wait[orgSubmodel] << std::endl;
        }

        if (!flags.ignore_overhanging_threads) {
            localQuorumDelayActivity->_remote_quorum_delay.mean(
                remoteDiffJoin.mean() + localDiffJoin.mean() * probQuorumDelaySeqExecution );
            localQuorumDelayActivity->_remote_quorum_delay.variance(
                remoteDiffJoin.variance() + localDiffJoin.variance() * probQuorumDelaySeqExecution );
        } else {
            localQuorumDelayActivity->_remote_quorum_delay.mean(0);
            localQuorumDelayActivity->_remote_quorum_delay.variance(0);
        }
        if (flags.trace_quorum) {
            std::cout <<"orgSubmodel="<<orgSubmodel<< std::endl;
            std::cout << "0. localQuorumDelayActivity->_remote_quorum_delay: mean =" << localQuorumDelayActivity->_remote_quorum_delay.mean()
                 << ", Variance=" << localQuorumDelayActivity->_remote_quorum_delay.variance() << std::endl;
            std::cout <<"probQuorumDelaySeqExecution=" <<probQuorumDelaySeqExecution << std::endl;
        }

    } else {
	throw std::logic_error( "AndForkActivityList::saveQuorumDelayedThreadsServiceTime" );
        anError = true;
    }

    if (flags.trace_quorum) {
        std::cout <<"\n'''''''''''end saveQuorumDelayedThreadsServiceTime'''''''''''''''''''''''" << std::endl;
    }

    return !anError;
}
#endif

/*
 * Return the sum of aFunc.
 */

const Activity::Count_If&
AndForkActivityList::count_if( std::deque<const Activity *>& stack, Activity::Count_If& data ) const
{
    double sum = 0.0;
    unsigned phase = data.phase();
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	Activity::Count_If branch( data, 1.0 );
	const AndJoinActivityList * joins = dynamic_cast<const AndJoinActivityList *>(this->joins());
	branch.setReplyAllowed(!joins || !joins->hasQuorum());	/* Disallow replies quorum on branches */
	branch = (*activity)->count_if( stack, branch );
        sum += branch.sum() - data.sum();				/* only accumulate difference */
	phase = std::max( phase, branch.phase() );
    }

    /* Now follow the activities after the join */

    data = sum;
    data.setPhase( phase );
    if ( hasNextFork() ) {
	getNextFork()->count_if( stack, data );
    }
    return data;		/* Result of last branch */
}



/*
 * Collect the calls by phase for the interlocker.  We don't care about rates.
 */

CallInfo::Item::collect_calls&
AndForkActivityList::collect_calls( std::deque<const Activity *>& stack, CallInfo::Item::collect_calls& data ) const
{
    unsigned phase = data.phase();
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	CallInfo::Item::collect_calls branch( data );
	(*activity)->collect_calls( stack, branch );
	phase = std::max( phase, branch.phase() );
    }

    data.setPhase( phase );
    if ( hasNextFork() ) {
	getNextFork()->collect_calls( stack, data );
    }
    return data;
}


/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait.
 * Only follow the root path.  Other paths in the AND-FORK are followed
 * by their thread.
 */

void
AndForkActivityList::callsPerform( Call::Perform& operation ) const
{
    if ( hasNextFork() ) getNextFork()->callsPerform( operation );
}



/*
 * Get the number of concurrent threads
 */

unsigned
AndForkActivityList::concurrentThreads( unsigned n ) const
{
    const unsigned m = std::accumulate( activities().begin(), activities().end(), 0, add_threads( &Activity::concurrentThreads, 1 ) );
    n = std::max( n, m - 1 );

    return hasNextFork() ? getNextFork()->concurrentThreads( n ) : n;
}




/*
 * Locate all joins and print delays.
 */

const AndForkActivityList&
AndForkActivityList::insertDOMResults(void) const
{
//    if ( getReplicaNumber() != 1 ) return *this;		/* NOP */

    if ( joins() == nullptr ) return *this;
    LQIO::DOM::AndJoinActivityList * dom = const_cast<LQIO::DOM::AndJoinActivityList *>(dynamic_cast<const LQIO::DOM::AndJoinActivityList *>(joins()->getDOM()));
    if ( dom == nullptr ) return *this;
    dom->setResultJoinDelay(_joinDelay)
	.setResultVarianceJoinDelay(_joinVariance);
    return *this;
}


std::ostream&
AndForkActivityList::printJoinDelay( std::ostream& output ) const
{
    output << "   " << owner()->name()
	   << ", Fork: " << getDOM()->getListName();
    if ( joins() != nullptr ) {
	output << " -> Join: " << joins()->getDOM()->getListName();
    } else {
	output << " -> unterminated";
    }
    output << ", join delay=" << _joinDelay << ", variance=" << _joinVariance << std::endl;
    return output;
}

/* -------------------------------------------------------------------- */
/*                    And Or Join Activity Lists                        */
/* -------------------------------------------------------------------- */

AndOrJoinActivityList::AndOrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : ForkJoinActivityList( owner, dom ),
      _forkList(nullptr),
      _next(nullptr)
{
}



AndOrJoinActivityList::AndOrJoinActivityList( const AndOrJoinActivityList& src, const Task * owner, unsigned int replica )
    : ForkJoinActivityList( src, owner, replica ),
      _forkList(nullptr),
      _next(nullptr)
{
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	const_cast<Activity *>(*activity)->nextJoin( this );	/* Link activity to this list	*/
    }
    /* Subclasses clone the virtual entry */
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.  This version attempts to match the join with its
 * fork.
 */

unsigned
AndOrJoinActivityList::findChildren( Activity::Children& path ) const
{
    const_cast<AndOrJoinActivityList *>(this)->updateRate( path.top_activity(), path.getRate() );

    if ( forkList() == nullptr ) {
	std::deque<const AndOrForkActivityList *>& forkStack = path.getForkStack();
	std::set<const AndOrForkActivityList *> resultSet(forkStack.begin(),forkStack.end());

	/* Go up all of the branches looking for forks found on forkStack */

	for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	    if ( *activity == path.top_activity() ) continue;		/* No need -- this is resultSet */

	    /* Find all forks from this activity that match anything in forkStack */

	    std::set<const AndOrForkActivityList *> branchSet;
	    Activity::Backtrack data( path.getActivityStack(), forkStack, branchSet );
	    (*activity)->backtrack( data );			/* find fork lists on this branch */


	    /* Find intersection of branches */

	    std::set<const AndOrForkActivityList *> intersection;
	    std::set_intersection( branchSet.begin(), branchSet.end(),
				   resultSet.begin(), resultSet.end(),
				   std::inserter( intersection, intersection.end() ) );
	    resultSet = intersection;
	}

	/* Result should be all forks that match on all branches.  Take the one closest to top-of-stack */

	const AndJoinActivityList * and_join_list = dynamic_cast<const AndJoinActivityList *>(this);
	if ( !resultSet.empty() ) {

	    for ( std::deque<const AndOrForkActivityList *>::const_reverse_iterator fork_list = forkStack.rbegin(); fork_list != forkStack.rend() && forkList() == nullptr; ++fork_list ) {
		/* See if we can find a match on the fork stack in the result set of the right type */
		if ( (resultSet.find( *fork_list ) == resultSet.end())
		     || (dynamic_cast<const AndForkActivityList *>(*fork_list) && dynamic_cast<const AndJoinActivityList *>(this) == nullptr )
		     || (dynamic_cast<const OrForkActivityList *>(*fork_list) && dynamic_cast<const OrJoinActivityList *>(this) == nullptr ) ) continue;

		/* Set type for join */

		if ( and_join_list != nullptr && !const_cast<AndJoinActivityList *>(and_join_list)->joinType( AndJoinActivityList::JoinType::INTERNAL_FORK_JOIN ) ) {
		    throw bad_internal_join( getDOM() );
		}

		/* Set the links */

		if ( Options::Debug::forks() ) {
		    if ( dynamic_cast<const AndJoinActivityList *>(this) ) std::cerr << "And ";
		    else if ( dynamic_cast<const OrJoinActivityList *>(this) ) std::cerr << "Or ";
		    else abort();
		    std::cerr << std::setw( path.getActivityStack().size() ) << "Join: " << getDOM()->getListName()
			      << " -> Fork: " << (*fork_list)->getDOM()->getListName() << std::endl;
		}
		const_cast<AndOrForkActivityList *>(*fork_list)->setJoinList( this );
		const_cast<AndOrJoinActivityList *>(this)->setForkList( *fork_list );	       	/* Will break loop */
	    }

	} else if ( (and_join_list != nullptr && !const_cast<AndJoinActivityList *>(and_join_list)->joinType( AndJoinActivityList::JoinType::SYNCHRONIZATION_POINT ))
		    || dynamic_cast<const OrJoinActivityList *>(this) ) {
	    throw bad_external_join( getDOM() );
	}
    }

    /* Carry on */

    if ( next() ) {
        return next()->findChildren( path );
    }
    return path.depth();
}



/*
 * return the fork list closest to the bottom of the stack.
 */

void
AndOrJoinActivityList::backtrack( Activity::Backtrack& data ) const
{
    if ( data.find_join( this ) ) {
	const std::deque<const Activity *>& activityStack = data.getActivityStack();
	throw activity_cycle( activityStack.back(), activityStack );
    }
    data.insert_join( this );
    std::for_each ( activities().begin(), activities().end(), ConstExec1<Activity,Activity::Backtrack&>( &Activity::backtrack, data ) );
}

/* -------------------------------------------------------------------- */
/*                      Or Join Activity Lists                          */
/* -------------------------------------------------------------------- */

OrJoinActivityList::OrJoinActivityList( const OrJoinActivityList& src, const Task* task, unsigned int replica )
    : AndOrJoinActivityList( src, task, replica )
{
}

bool
OrJoinActivityList::updateRate( const Activity * activity, double rate )
{
    _rateList.insert( std::pair<const Activity *,double>( activity, rate ) );
    _rate = std::accumulate( _rateList.begin(), _rateList.end(), 0., add_rate() );
    return true;
}

/* -------------------------------------------------------------------- */
/*                     And Join Activity Lists                          */
/* -------------------------------------------------------------------- */

AndJoinActivityList::AndJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : AndOrJoinActivityList( owner, dom ),
      _joinType(AndJoinActivityList::JoinType::NOT_DEFINED),
      _quorumCount(dom ? dynamic_cast<LQIO::DOM::AndJoinActivityList*>(dom)->getQuorumCountValue() : 0),
      _quorumListNum(0)
{
}


AndJoinActivityList::AndJoinActivityList( const AndJoinActivityList& src, const Task* task, unsigned int replica )
    : AndOrJoinActivityList( src, task, replica ),
      _joinType(src._joinType),
      _quorumCount(src._quorumCount),
      _quorumListNum(0)
{
}



bool
AndJoinActivityList::joinType( AndJoinActivityList::JoinType aType )
{
    if ( _joinType == AndJoinActivityList::JoinType::NOT_DEFINED ) {
        _joinType = aType;
        return true;
    } else {
        return aType == _joinType;
    }
}


/*
 * If I am a synchronization point, backtrack and abort.
 */

bool
AndJoinActivityList::check() const
{
    bool rc = true;
    if ( isSync() ) {
	getDOM()->runtime_error( LQIO::ERR_NOT_SUPPORTED, "External join" ); // 
	rc = false;
    }
    return AndOrJoinActivityList::check() && rc;
}


/*
 * disallow on or branches and repeat branches
 */

unsigned
AndJoinActivityList::findChildren( Activity::Children& path ) const
{
    if ( !path.canReply() ) {
	if ( path.top_fork() != nullptr ) {
	    const LQIO::DOM::ActivityList * fork_list = path.top_fork()->getDOM();
	    getDOM()->runtime_error( LQIO::ERR_FORK_JOIN_MISMATCH, fork_list->getListTypeName().c_str(), fork_list->getListName().c_str(), fork_list->getLineNumber() );
	} else {
	    getDOM()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, path.top_activity()->name().c_str() );
	}
	return path.depth();
    } else {
	return AndOrJoinActivityList::findChildren( path );
    }
}

/*
 * Follow the path.  We don't care about other paths.
 */

void
AndJoinActivityList::followInterlock( Interlock::CollectTable& path ) const
{
    if ( next() && isSync() ) next()->followInterlock( path );
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
AndJoinActivityList::getInterlockedTasks( Interlock::CollectTasks& path ) const
{
    return next() != nullptr && isSync() && next()->getInterlockedTasks( path );
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
AndJoinActivityList::callsPerform( Call::Perform& operation ) const
{
    if ( isSync() && next() ) next()->callsPerform( operation );
}



/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.
 * Otherwise, press on.
 */

Activity::Collect&
AndJoinActivityList::collect( std::deque<const Activity *>& activityStack, std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    return ( isSync() && next() ) ? next()->collect( activityStack, entryStack, data ) : data;
}



/*
 * If this join is the match for forkList, then stop aggregating as this thread is now done.
 * Otherwise, press on.
 */

const Activity::Count_If&
AndJoinActivityList::count_if( std::deque<const Activity *>& stack, Activity::Count_If& data ) const
{
    return ( isSync() && next() ) ? next()->count_if( stack, data ) : data;
}


/*
 * Collect the calls by phase for the interlocker.
 */

CallInfo::Item::collect_calls&
AndJoinActivityList::collect_calls( std::deque<const Activity *>& stack, CallInfo::Item::collect_calls& data ) const
{
    return ( isSync() && next() ) ? next()->collect_calls( stack, data ) : data;
}



/*
 * Get the number of concurrent threads
 */

unsigned
AndJoinActivityList::concurrentThreads( unsigned n ) const
{
    return isSync() ? next()->concurrentThreads( n ) : n;
}

/*----------------------------------------------------------------------*/
/*                           Repetition node.                           */
/*----------------------------------------------------------------------*/

RepeatActivityList::RepeatActivityList( Task * owner, LQIO::DOM::ActivityList * dom )
    : ForkActivityList( owner, dom ),
      _prev(nullptr),
      _activities(),
      _entries()
{
}


/*
 * Clone the repeat list.  _activities contains the repeat items
 * (done here).  The super class will handle the end of list.
 */

RepeatActivityList::RepeatActivityList( const RepeatActivityList& src, const Task* owner, unsigned int replica )
    : ForkActivityList( src, owner, replica ),
      _prev(nullptr),
      _activities(),
      _entries()
{
    for ( std::vector<const Activity *>::const_iterator activity = src.activities().begin(); activity != src.activities().end(); ++activity ) {
	_activities.push_back( owner->findActivity( (*activity)->name() ) );
    }
    for ( std::vector<VirtualEntry *>::const_iterator entry = src.entries().begin(); entry != src.entries().end(); ++entry ) {
	_entries.push_back( cloneVirtualEntry( *entry, owner, replica ) );
    }
}



RepeatActivityList::~RepeatActivityList()
{
    std::for_each( entries().begin(), entries().end(), Delete<Entry *> );
}




/*
 * Clone the virtual entries.
 */

VirtualEntry *
RepeatActivityList::cloneVirtualEntry( const Entry * src, const Task * owner, unsigned int replica ) const
{
    VirtualEntry * dst = dynamic_cast<VirtualEntry *>(src->clone( replica, nullptr ));
    dst->owner( owner );
    if ( src->hasStartActivity() ) {
	Activity * activity = owner->findActivity( src->getStartActivity()->name() );
	dst->setStartActivity( activity );
	activity->setEntry( dst );
    }
    return dst;
}



/*
 * Add a sublist.
 */

RepeatActivityList&
RepeatActivityList::add( Activity * activity )
{
    const LQIO::DOM::ExternalVariable * arg = getDOM()->getParameter(const_cast<LQIO::DOM::Activity *>(activity->getDOM()));
    if ( arg ) {
        _activities.push_back( activity );

	VirtualEntry * entry = new VirtualEntry( activity );
	_entries.push_back( entry );
        assert( entry->entryTypeOk(LQIO::DOM::Entry::Type::ACTIVITY) );
        entry->setStartActivity( activity );
	activity->setEntry( entry );
    } else {
        /* End of list */
        ForkActivityList::add( activity );
    }
    return *this;
}


/*
 * Configure descendents
 */

RepeatActivityList&
RepeatActivityList::configure( const unsigned n )
{
    std::for_each( entries().begin(), entries().end(), Exec1<Entry,unsigned>( &Entry::configure, n ) );
    return *this;
}



#if PAN_REPLICATION
ActivityList&
RepeatActivityList::setSurrogateDelaySize( size_t size )
{
    std::for_each( entries().begin(), entries().end(), Exec1<Entry,size_t>( &Entry::setSurrogateDelaySize, size ) );
    return *this;
}
#endif


double
RepeatActivityList::rateBranch( const Activity * anActivity ) const
{
    return getDOM()->getParameterValue(anActivity->getDOM());
}


VirtualEntry *
RepeatActivityList::collectToEntry( const Activity * activity, VirtualEntry * entry, std::deque<const Activity *>& activityStack, std::deque<Entry *>&entryStack, Activity::Collect& branch )
{
    branch.setRate( branch.rate() * rateBranch(activity) );

    entry->set( entryStack.back(), branch );

    entryStack.push_back( entry );
    activity->collect( activityStack, entryStack, branch );
    entryStack.pop_back();
    return entry;
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
RepeatActivityList::findChildren( Activity::Children& path ) const
{
    return std::accumulate( activities().begin(), activities().end(), ForkActivityList::findChildren( path ), find_children( *this, path ) );
}


/*
 * Link activity.
 */

void
RepeatActivityList::followInterlock( Interlock::CollectTable& path ) const
{
    std::for_each( activities().begin(), activities().end(), follow_interlock( *this, path ) );
    ForkActivityList::followInterlock( path );
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
RepeatActivityList::getInterlockedTasks( Interlock::CollectTasks& path ) const
{
    bool found = std::count_if( activities().begin(), activities().end(), Predicate1<Activity,Interlock::CollectTasks&>( &Activity::getInterlockedTasks, path ) ) > 0;
    if ( ForkActivityList::getInterlockedTasks( path ) ) found = true;

    return found;
}


/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
RepeatActivityList::callsPerform( Call::Perform& operation ) const
{
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	Call::Perform g( operation, rateBranch(*activity) );
        (*activity)->callsPerform( g );
    }

    ForkActivityList::callsPerform( operation );
}


/*
 * Recursively aggregate using 'aFunc' along all branches of the or, storing
 * the results of the aggregation in the virtual entry assigned to each branch.
 */

Activity::Collect&
RepeatActivityList::collect( std::deque<const Activity *>& activityStack, std::deque<Entry *>& entryStack, Activity::Collect& data )
{
    const unsigned int submodel = data.submodel();
    const unsigned int n = activities().size();
    Entry * currEntry = entryStack.back();
    Activity::Collect::Function f = data.collect();

    for ( unsigned i = 0; i < n; ++i ) {
	const Activity * anActivity = activities().at(i);
	Activity::Collect branch(data);
	VirtualEntry * anEntry = collectToEntry( anActivity, entries()[i], activityStack, entryStack, branch );

        if ( f == &Activity::collectWait ) {
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                Exponential term;
                Exponential sum;
                if ( submodel == 0 ) {
                    term.init( anEntry->_phase[p].elapsedTime(), anEntry->_phase[p].variance() );
                } else {
                    term.mean( anEntry->_phase[p].getWaitTime(submodel) );
                }
                sum = rateBranch(anActivity) * term + varianceTerm( term );
                currEntry->aggregate( submodel, p, sum );
            }

#if PAN_REPLICATION
        } else if ( f == &Activity::collectReplication ) {
            Vector< VectorMath<double> > sum(currEntry->maxPhase());
            for ( unsigned p = 1; p <= currEntry->maxPhase(); ++p ) {
                sum[p] = anEntry->_phase[p]._surrogateDelay;
                sum[p] *= rateBranch(anActivity);
            }
            currEntry->aggregateReplication( sum );
#endif

        } else if ( f == &Activity::collectServiceTime ) {
	    currEntry->addServiceTime( data.phase(), rateBranch(anActivity) * anEntry->_phase[data.phase()].serviceTime() );
        }
    }

    /* Do after as there may be a phase change. */

    ForkActivityList::collect( activityStack, entryStack, data );

    return data;
}


/*
 * Return the sum of aFunc.
 * Rate is set to zero for branches so that we don't count replies.
 */

const Activity::Count_If&
RepeatActivityList::count_if( std::deque<const Activity *>& stack, Activity::Count_If& data ) const
{
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	Activity::Count_If branch( data, rateBranch(*activity) );
	branch.setReplyAllowed( false );
	branch = (*activity)->count_if( stack, branch );
	data += branch.sum() - data.sum();				/* only accumulate difference */
    }
    return ForkActivityList::count_if( stack, data );
}



/*
 * Collect the calls by phase for the interlocker.
 */

CallInfo::Item::collect_calls&
RepeatActivityList::collect_calls( std::deque<const Activity *>& stack, CallInfo::Item::collect_calls& data ) const
{
    for ( std::vector<const Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	CallInfo::Item::collect_calls branch( data );
	(*activity)->collect_calls( stack, branch );
    }
    return ForkActivityList::collect_calls( stack, data );
}



/*
 * Get the number of concurrent threads
 */

unsigned
RepeatActivityList::concurrentThreads( unsigned n ) const
{
    return ForkActivityList::concurrentThreads( std::accumulate( activities().begin(), activities().end(), n, max_using_arg<Activity,const unsigned int>( &Activity::concurrentThreads, n ) ) );
}



std::ostream&
RepeatActivityList::printSubmodelWait( std::ostream& output, unsigned offset ) const
{
    std::for_each( entries().begin(), entries().end(), ConstPrint1<Entry,unsigned>( &Entry::printSubmodelWait, output, offset ) );
    return output;
}


unsigned
RepeatActivityList::find_children::operator()( unsigned arg1, const Activity * arg2 ) const
{
    std::deque<const AndOrForkActivityList *> forkStack;    // For matching forks/joins.
    Activity::Children path( _path, forkStack, _self.rateBranch( arg2 ) );
    path.setReplyAllowed(false);		// Bug 427
    return std::max( arg1, arg2->findChildren(path) );
}

/* ------------------------ Exception Handling ------------------------ */

bad_internal_join::bad_internal_join( const LQIO::DOM::ActivityList * list )
    : std::runtime_error( list->getListName() ), _list(list)
{
}


bad_external_join::bad_external_join( const LQIO::DOM::ActivityList * list )
    : std::runtime_error( list->getListName() ), _list(list)
{
}
