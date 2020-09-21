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
 * $Id: actlist.h 13840 2020-09-21 14:44:18Z greg $
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
class Format;
class ForkJoinActivityList;
class AndForkActivityList;
class AndJoinActivityList;
class DiscretePoints;
class DiscreteCDFs;
 
template <class type> class Stack;
template <class type> class Vector;
template <class type> class VectorMath;


class bad_internal_join : public path_error 
{
public:
    bad_internal_join( const std::deque<const Activity *>& );
    virtual ~bad_internal_join() throw() {}
};

class bad_external_join : public path_error 
{
public:
    bad_external_join( const std::deque<const Activity *>& );
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
    virtual void configure( const unsigned ) = 0;
    static void reset();

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
	
    /* Computation */

    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const = 0;
    virtual std::deque<const AndForkActivityList *>::const_iterator backtrack( const std::deque<const AndForkActivityList *>& ) const = 0;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const = 0;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& ) = 0;
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const = 0;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const = 0;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const = 0;
    virtual unsigned concurrentThreads( unsigned ) const = 0;

    virtual const std::string& firstName() const;
    virtual const std::string& lastName() const;
	
    /* Printing */
	
    virtual const ActivityList& insertDOMResults() const { return *this; }

public:
    static unsigned n_joins;
    static unsigned n_forks;
   
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
    virtual const std::string& firstName() const;
    virtual const std::string& lastName() const;

    Activity * getMyActivity() {return _activity;} 

protected:
    virtual const char * typeStr() const { return "+"; }

protected:
    Activity * _activity;
};

/* -------------------------------------------------------------------- */

class ForkActivityList : public SequentialActivityList
{
public:
    ForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
    virtual void configure( const unsigned );
	
    virtual activity_type myType() const { return SEQUENCE; }
    virtual ActivityList * prev() const { return _prev; }	/* Link to fork list 		*/

    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    virtual std::deque<const AndForkActivityList *>::const_iterator backtrack( const std::deque<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
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
    virtual void configure( const unsigned );
	
    virtual activity_type myType() const { return SEQUENCE; }
    virtual ActivityList * next() const { return _next; }	/* Link to Join list		*/

    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    virtual std::deque<const AndForkActivityList *>::const_iterator backtrack( const std::deque<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
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
    class ForkJoinName
    {
    public:
	ForkJoinName( const ForkJoinActivityList& );
	const char * operator()();

    private:
	std::string aString;
    };
	
    friend class ForkJoinActivityList::ForkJoinName;

public:
    ForkJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : ActivityList( owner, dom) {}
    virtual ~ForkJoinActivityList();
    virtual bool operator==( const ActivityList& item ) const;
    const std::vector<Activity *>& getMyActivityList() const { return _activityList; }
    virtual ForkJoinActivityList& add( Activity * anActivity );
    virtual const std::string& firstName() const;
    virtual const std::string& lastName() const;

protected:
    std::vector<Activity *> _activityList;
};


/* -------------------------------------------------------------------- */

class AndOrForkActivityList : public ForkJoinActivityList
{
    friend class Task;
	
public:
    AndOrForkActivityList( Task * owner, LQIO::DOM::ActivityList * );
    virtual ~AndOrForkActivityList();
    virtual void configure( const unsigned );
	
    virtual AndOrForkActivityList& add( Activity * anActivity );

    virtual ActivityList * prev() const { return _prev; }	/* Link to join list 		*/

    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;

    virtual double prBranch( const Activity * ) const = 0;

protected:
    virtual AndOrForkActivityList& prev( ActivityList * aList) { _prev = aList; return *this; }
    Entry * collectToEntry( const Activity *, Entry *, std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );

protected:    
    std::vector<Entry *> _entryList;
    
private:
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

    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    virtual std::deque<const AndForkActivityList *>::const_iterator backtrack( const std::deque<const AndForkActivityList *>& ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

    virtual double prBranch( const Activity * ) const;

protected:
    virtual const char * typeStr() const { return "|"; }
};

/* -------------------------------------------------------------------- */

class AndForkActivityList : public AndOrForkActivityList
{
    friend class AndJoinActivityList;
    
public:
    AndForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
	
    virtual AndForkActivityList& add( Activity * anActivity );
    virtual double prBranch( const Activity * ) const { return 1.0; }
    virtual activity_type myType() const { return AND_FORK; }
    virtual bool check() const;

    bool isDescendentOf( const AndForkActivityList * ) const;

    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    virtual std::deque<const AndForkActivityList *>::const_iterator backtrack( const std::deque<const AndForkActivityList *>& ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

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
    mutable const AndForkActivityList * myParent;
    mutable const AndJoinActivityList * myJoinList;
    double myJoinDelay;
    double myJoinVariance;
};
 

/* -------------------------------------------------------------------- */

class AndOrJoinActivityList : public ForkJoinActivityList
{
public:
    AndOrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * );
    virtual void configure( const unsigned );
	
    virtual ActivityList * next() const { return _next; }	/* Link to fork list		*/
    virtual std::deque<const AndForkActivityList *>::const_iterator backtrack( const std::deque<const AndForkActivityList *>& ) const;

protected:
    virtual AndOrJoinActivityList& next( ActivityList * aList ) { _next = aList; return *this; }

protected:
    mutable unsigned visits;

private:
    ActivityList *_next;
};


/* -------------------------------------------------------------------- */

class OrJoinActivityList : public AndOrJoinActivityList
{
public:
    OrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : AndOrJoinActivityList( owner, dom ) {}
	
    virtual activity_type myType() const { return OR_JOIN; }

    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual const char * typeStr() const { return "|"; }

private:
    mutable std::map<const Activity *,double> _rateBranch;
};

/* -------------------------------------------------------------------- */

class AndJoinActivityList : public AndOrJoinActivityList
{
public:
    typedef enum { JOIN_NOT_DEFINED, INTERNAL_FORK_JOIN, SYNCHRONIZATION_POINT } join_type;

    AndJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
    virtual AndJoinActivityList& add( Activity * anActivity );

    virtual unsigned size() const { return _activityList.size(); }
    void quorumListNum( unsigned quorumListNum) {myQuorumListNum = quorumListNum; }
    int quorumListNum() const { return myQuorumListNum; }  
    void quorumCount ( unsigned quorumCount) { myQuorumCount = quorumCount; }
    unsigned quorumCount () const { return myQuorumCount;}

    virtual activity_type myType() const { return AND_JOIN; }
    virtual bool check() const;

    bool isSynchPoint() const { return myJoinType == SYNCHRONIZATION_POINT; }
    bool isInternalForkJoin() const { return myJoinType == INTERNAL_FORK_JOIN; }
    bool joinType( const join_type );
    bool hasQuorum() const { return 0 < quorumCount() && quorumCount() < size(); }
	
    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

private:
    virtual const char * typeStr() const { return "&"; }
    bool addToSrcList( unsigned i, const Activity * ) const;

private:
    mutable join_type myJoinType;		/* Barrier synch point.	*/
    mutable std::vector<const AndForkActivityList *> _forkList;
    mutable std::vector<const Activity *> _srcList;
    mutable unsigned myQuorumCount;
    unsigned  myQuorumListNum;
};


/* -------------------------------------------------------------------- */

class RepeatActivityList : public ForkActivityList
{
public:
    RepeatActivityList( Task * owner, LQIO::DOM::ActivityList * dom );
    virtual ~RepeatActivityList();
    virtual void configure( const unsigned );
    virtual RepeatActivityList& add( Activity * anActivity );
	
    virtual activity_type myType() const { return REPEAT; }

    virtual ActivityList * prev() const { return _prev; }	/* Link to join list 		*/

    virtual unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Test& test( std::deque<const Activity *>&, Activity::Test& ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual const char * typeStr() const { return "*"; }
    virtual RepeatActivityList& prev( ActivityList * aList) { _prev = aList; return *this; }

private:
    Entry * collectToEntry( const Activity *, Entry *, std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    double rateBranch( const Activity * ) const;

private:
    ActivityList * _prev;
    std::vector<Activity *> _activityList;
    std::vector<Entry *> _entryList;
};

/* Used by model.cc */

void add_reply_list ( Task* task, Activity* activity );
void add_activity_lists ( Task* task, Activity* activity );
void complete_activity_connections ();
#endif
