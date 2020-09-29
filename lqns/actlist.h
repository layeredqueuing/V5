/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/actlist.h $
 *
 * Everything you wanted to know about an activity, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: actlist.h 13895 2020-09-29 14:13:22Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef _ACTLIST_H
#define _ACTLIST_H

#include <config.h>
#include "dim.h"
#include <lqio/dom_activity.h>
#include <string>
#include <stack>
#include "vector.h"
#include "prob.h"
#include "activity.h"

class Entry;
class Entity;
class ActivityList;
class InterlockInfo;
class Task;
class ForkJoinActivityList;
class AndOrJoinActivityList;
class DiscretePoints;
class DiscreteCDFs;
 
template <class type> class Stack;
template <class type> class Vector;
template <class type> class VectorMath;


class bad_internal_join : public std::runtime_error
{
public:
    bad_internal_join( const ForkJoinActivityList& );
    virtual ~bad_internal_join() throw() {}
};

class bad_external_join : public std::runtime_error
{
public:
    bad_external_join( const ForkJoinActivityList& );
    virtual ~bad_external_join() throw() {}
};


/* -------------------------------------------------------------------- */
/*                             ActivityList                             */
/* -------------------------------------------------------------------- */

class ActivityList
{
    friend void act_connect ( ActivityList * src, ActivityList * dst );

public:
    typedef enum { SEQUENCE, REPEAT, AND_FORK, AND_JOIN, AND_SYNCH, OR_FORK, OR_JOIN } activity_type;

    ActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist );

private:
    ActivityList( const ActivityList& );
    ActivityList& operator=( const ActivityList& );

public:
    virtual ~ActivityList() {}
    virtual ActivityList& configure( const unsigned ) { return *this; }

    bool operator!=( const ActivityList& item ) const { return !(*this == item); }
    virtual bool operator==( const ActivityList& item ) const { return false; }

    /* Instance Variable Access */
	
    virtual activity_type myType() const = 0;
    virtual ActivityList& add( Activity * anActivity ) = 0;
    virtual bool check() const { return true; }

    const Task * owner() const { return _task; }
    const LQIO::DOM::ActivityList * getDOM() const { return _dom; }
    virtual ActivityList * next() const;	/* Link to fork list 		*/
    virtual ActivityList * prev() const;	/* Link to Join list		*/

    virtual bool isFork() const { return false; }
    virtual bool isSync() const { return false; }
    
    /* Computation */

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>& ) const = 0;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const = 0;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& ) = 0;
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const = 0;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const = 0;
    virtual void callsPerform( const Phase::CallExec& ) const = 0;
    virtual unsigned concurrentThreads( unsigned ) const = 0;

    virtual const AndOrForkActivityList * backtrack( std::deque<const AndOrForkActivityList *>& forkStack ) const = 0;

    /* Printing */
	
    virtual std::ostream& printSubmodelWait( std::ostream& output, unsigned offset ) const { return output; }
    virtual std::ostream& printJoinDelay( std::ostream& output ) const { return output; }
    virtual const ActivityList& insertDOMResults() const { return *this; }

protected:
    virtual const char * typeStr() const { return " "; }
    virtual ActivityList& next( ActivityList * );	/* Link to fork list 		*/
    virtual ActivityList& prev( ActivityList * );	/* Link to Join list		*/

    void initEntry( Entry *, const Entry *, const Activity::Collect& ) const;

private:
    const Task * _task;
    const LQIO::DOM::ActivityList * _dom;
};

/* ==================================================================== */

class SequentialActivityList : public ActivityList
{
public:
    SequentialActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : ActivityList( owner, dom), _activity(nullptr) {}
    virtual bool operator==( const ActivityList& item ) const;
    virtual SequentialActivityList& add( Activity * anActivity );

    Activity * getActivity() const { return _activity; }

protected:
    virtual const char * typeStr() const { return "+"; }

private:
    Activity * _activity;
};

/* -------------------------------------------------------------------- */

class ForkActivityList : public SequentialActivityList
{
public:
    ForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
	
    virtual activity_type myType() const { return SEQUENCE; }
    virtual ActivityList * prev() const { return _prev; }	/* Link to fork list 		*/

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>& ) const;
    virtual const AndOrForkActivityList * backtrack( std::deque<const AndOrForkActivityList *>& forkStack ) const { return prev()->backtrack( forkStack ); }
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( const Phase::CallExec& ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual ForkActivityList& prev( ActivityList * aList) { _prev = aList; return *this; }

private:
    ActivityList * _prev;
};

/* -------------------------------------------------------------------- */

class JoinActivityList : public SequentialActivityList
{
public:
    JoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
	
    virtual activity_type myType() const { return SEQUENCE; }
    virtual ActivityList * next() const { return _next; }	/* Link to Join list		*/

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>& ) const;
    virtual const AndOrForkActivityList * backtrack( std::deque<const AndOrForkActivityList *>& forkStack ) const { return getActivity()->backtrack( forkStack ); }
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( const Phase::CallExec& ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual ActivityList& next( ActivityList * aList ) { _next = aList; return *this; }	/* Link to Join list		*/

private:
    ActivityList *_next;
};

/* ==================================================================== */

class ForkJoinActivityList : public ActivityList
{
public:
    ForkJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : ActivityList( owner, dom) {}
    virtual ~ForkJoinActivityList();

    virtual ForkJoinActivityList& add( Activity * anActivity );
    
    virtual bool operator==( const ActivityList& item ) const;
    const std::vector<const Activity *>& activityList() const { return _activityList; }

    virtual std::string getName() const;

private:
    std::vector<const Activity *> _activityList;
};


/* -------------------------------------------------------------------- */

class AndOrForkActivityList : public ForkJoinActivityList
{
    friend class Task;
	
public:
    AndOrForkActivityList( Task * owner, LQIO::DOM::ActivityList * );
    virtual ~AndOrForkActivityList();

    virtual AndOrForkActivityList& configure( const unsigned );
	
    bool hasNextFork() const;
    ActivityList * getNextFork() const;

    virtual ActivityList * prev() const { return _prev; }	/* Link to join list 		*/
    virtual const AndOrJoinActivityList * joinList() const;
    bool insertIntoJoinList( const Activity *, const AndOrJoinActivityList * );

    virtual bool check() const;
    
    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>& ) const;
    virtual const AndOrForkActivityList * backtrack( std::deque<const AndOrForkActivityList *>& forkStack ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;

    bool find( const Activity * activity ) const { return _joinList.find( activity ) != _joinList.end(); }
    virtual double prBranch( const Activity * ) const = 0;

    virtual std::ostream& printSubmodelWait( std::ostream& output, unsigned offset ) const;

protected:
    virtual AndOrForkActivityList& prev( ActivityList * aList) { _prev = aList; return *this; }
    Entry * collectToEntry( const Activity *, Entry *, std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );

protected:    
    std::vector<Entry *> _entryList;
    mutable const AndForkActivityList * _parentForkList;
    
private:
    std::map<const Activity *,const AndOrJoinActivityList *> _joinList;
    ActivityList * _prev;
};


/* -------------------------------------------------------------------- */

class OrForkActivityList : public AndOrForkActivityList
{
public:
    OrForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : AndOrForkActivityList( owner, dom ) {}
	
    virtual OrForkActivityList& add( Activity * anActivity );
    virtual activity_type myType() const { return OR_FORK; }
    virtual bool check() const;

    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual void callsPerform( const Phase::CallExec& ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

    virtual double prBranch( const Activity * ) const;

protected:
    virtual const char * typeStr() const { return "|"; }
};

/* -------------------------------------------------------------------- */

class AndForkActivityList : public AndOrForkActivityList
{
public:
    AndForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
	
    virtual AndForkActivityList& add( Activity * anActivity );
    virtual activity_type myType() const { return AND_FORK; }
    virtual bool isFork() const { return true; }

    virtual double prBranch( const Activity * ) const { return 1.0; }

    virtual bool check() const;

    bool isDescendentOf( const AndForkActivityList * ) const;

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>& ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual void callsPerform( const Phase::CallExec& ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

    virtual std::ostream& printJoinDelay( std::ostream& output ) const;
    virtual const AndForkActivityList& insertDOMResults() const;

protected:
    virtual const char * typeStr() const { return "&"; }

private:
#if HAVE_LIBGSL
    bool saveQuorumDelayedThreadsServiceTime( Stack<Entry *>& entryStack,
					      DiscretePoints & quorumJoin,DiscreteCDFs & quorumCDFs,
					      DiscreteCDFs & localCDFs,DiscreteCDFs & remoteCDFs,
					      double probQuorumDelaySeqExecution) ;
#endif
    DiscretePoints * calcQuorumKofN( const unsigned submodel,
				     bool isQuorumDelayedThreadsActive, 
				     DiscreteCDFs & quorumCDFs ) const;


private:
    double _joinDelay;
    double _joinVariance;
};
 

/* -------------------------------------------------------------------- */

class AndOrJoinActivityList : public ForkJoinActivityList
{
public:
    AndOrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * );
	
    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>& ) const;
    virtual const AndOrForkActivityList * backtrack( std::deque<const AndOrForkActivityList *>& forkStack ) const;

    virtual ActivityList * next() const { return _next; }	/* Link to fork list		*/

protected:
    virtual AndOrJoinActivityList& next( ActivityList * aList ) { _next = aList; return *this; }

private:
    ActivityList *_next;
};


/* -------------------------------------------------------------------- */

class OrJoinActivityList : public AndOrJoinActivityList
{
public:
    OrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : AndOrJoinActivityList( owner, dom ) {}
	
    virtual activity_type myType() const { return OR_JOIN; }

    /* Most operations are done by the OrForkActivityList by following the next after all branches have been done */

    virtual unsigned followInterlock( std::deque<const Entry *>& entryStack, const InterlockInfo&, const unsigned ) const { return entryStack.size(); } /* NOP */
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const { return false; }	/* NOP */
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& data ) { return data; }			/* NOP */
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& data ) const { return data; }			/* NOP */
    virtual void callsPerform( const Phase::CallExec& ) const {}		/* NOP - done by fork */
    virtual unsigned concurrentThreads( unsigned n ) const { return n; }	/* NOP - done by fork */

protected:
    virtual const char * typeStr() const { return "|"; }
};

/* -------------------------------------------------------------------- */

class AndJoinActivityList : public AndOrJoinActivityList
{
public:
    typedef enum { JOIN_NOT_DEFINED, INTERNAL_FORK_JOIN, SYNCHRONIZATION_POINT } join_type;

    AndJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
    virtual activity_type myType() const { return AND_JOIN; }

    virtual bool check() const;
    
    void quorumListNum( unsigned quorumListNum) {myQuorumListNum = quorumListNum; }
    int quorumListNum() const { return myQuorumListNum; }  
    void quorumCount ( unsigned quorumCount) { myQuorumCount = quorumCount; }
    unsigned quorumCount () const { return myQuorumCount;}

    virtual bool isSync() const { return _joinType == SYNCHRONIZATION_POINT; }
    bool joinType( const join_type );
    bool hasQuorum() const { return 0 < quorumCount() && quorumCount() < activityList().size(); }
	
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual void callsPerform( const Phase::CallExec& ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual const char * typeStr() const { return "&"; }

private:
    join_type _joinType;		/* Barrier synch point.	*/
    mutable unsigned myQuorumCount;
    unsigned  myQuorumListNum;
};


/* -------------------------------------------------------------------- */

class RepeatActivityList : public ForkActivityList
{
public:
    RepeatActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
    virtual ~RepeatActivityList();
    virtual RepeatActivityList& configure( const unsigned );
    virtual RepeatActivityList& add( Activity * anActivity );
	
    virtual activity_type myType() const { return REPEAT; }

    virtual ActivityList * prev() const { return _prev; }	/* Link to join list 		*/
    const std::vector<const Activity *>& activityList() const { return _activityList; }

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>& ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( const Phase::CallExec& ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

    virtual std::ostream& printSubmodelWait( std::ostream& output, unsigned offset ) const;

protected:
    virtual const char * typeStr() const { return "*"; }
    virtual RepeatActivityList& prev( ActivityList * aList) { _prev = aList; return *this; }

private:
    Entry * collectToEntry( const Activity *, Entry *, std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    double rateBranch( const Activity * ) const;

private:
    ActivityList * _prev;
    std::vector<const Activity *> _activityList;
    std::vector<Entry *> _entryList;
};

/* Used by model.cc */

void add_reply_list ( Task* task, Activity* activity );
void add_activity_lists ( Task* task, Activity* activity );
void complete_activity_connections ();
#endif
