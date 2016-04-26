/* -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/actlist.h $
 *
 * Everything you wanted to know about an activity, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: actlist.h 11963 2014-04-10 14:36:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef _ACTLIST_H
#define _ACTLIST_H

#include <config.h>
#include "dim.h"
#include <lqio/dom_activity.h>
#include <string>
#include "vector.h"
#include "prob.h"

class CallStack;
class Entry;
class Entity;
class Activity;
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
template <class type> class Cltn;
template <class type> class Vector;
template <class type> class VectorMath;


class bad_internal_join : public path_error 
{
public:
    bad_internal_join( const Stack<const Activity *>& );
    virtual ~bad_internal_join() throw() {}
};

class bad_external_join : public path_error 
{
public:
    bad_external_join( const Stack<const Activity *>& );
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
    virtual void configure( const unsigned, const unsigned ) = 0;
    static void reset();

    int operator!=( const ActivityList& item ) const { return !(*this == item); }
    virtual int operator==( const ActivityList& item ) const { return 0; }

    /* Instance Variable Access */
	
    virtual activity_type myType() const = 0;
    virtual ActivityList& add( Activity * anActivity ) = 0;
    virtual void check() const {}

    const Task * owner() const { return myOwner; }
    const LQIO::DOM::ActivityList * getDOM() const { return myDOMActivityList; }
    virtual ActivityList * next() const;	/* Link to fork list 		*/
    virtual ActivityList * prev() const;	/* Link to Join list		*/
	
    /* Computation */

    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>& ) const = 0;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const = 0;
    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const = 0;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc ) = 0;
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const = 0;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const = 0;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const = 0;
    virtual unsigned concurrentThreads( unsigned ) const = 0;

    virtual const char * firstName() const;
    virtual const char * lastName() const;
	
    /* Printing */
	
    virtual void insertDOMResults(void) const {}

public:
    static unsigned n_joins;
    static unsigned n_forks;
   
protected:
    virtual const char * typeStr() const { return " "; }
    virtual ActivityList& next( ActivityList * );	/* Link to fork list 		*/
    virtual ActivityList& prev( ActivityList * );	/* Link to Join list		*/

    void initEntry( Entry *, const Entry *, const unsigned, AggregateFunc, const double = 1.0 ) const;

private:
    const Task * myOwner;
    const LQIO::DOM::ActivityList * myDOMActivityList;
};

/* ==================================================================== */

class SequentialActivityList : public ActivityList
{
public:
    SequentialActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist ) : ActivityList( owner, dom_activitylist), myActivity(0) {}
    virtual int operator==( const ActivityList& item ) const;
    virtual ActivityList& add( Activity * anActivity );
    virtual const char * firstName() const;
    virtual const char * lastName() const;

    Activity * getMyActivity() {return myActivity;} 

protected:
    virtual const char * typeStr() const { return "+"; }

protected:
    Activity * myActivity;
};

/* -------------------------------------------------------------------- */

class ForkActivityList : public SequentialActivityList
{
public:
    ForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist );
    virtual void configure( const unsigned, const unsigned );
	
    virtual activity_type myType() const { return SEQUENCE; }
    virtual ActivityList * prev() const { return prevLink; }	/* Link to fork list 		*/

    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>&  ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual ForkActivityList& prev( ActivityList * aList) { prevLink = aList; return *this; }

private:
    ActivityList * prevLink;
};

/* -------------------------------------------------------------------- */

class JoinActivityList : public SequentialActivityList
{
public:
    JoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist );
    virtual void configure( const unsigned, const unsigned );
	
    virtual activity_type myType() const { return SEQUENCE; }
    virtual ActivityList * next() const { return nextLink; }	/* Link to Join list		*/

    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>&  ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual ActivityList& next( ActivityList * aList ) { nextLink = aList; return *this; }	/* Link to Join list		*/

private:
    ActivityList *nextLink;
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
	string aString;
    };
	
    friend class ForkJoinActivityList::ForkJoinName;

public:
    ForkJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist ) : ActivityList( owner, dom_activitylist) {}
    virtual ~ForkJoinActivityList();
    virtual int operator==( const ActivityList& item ) const;
    Cltn<Activity *> & getMyActivityList(){ return myActivityList;}; 
    virtual ActivityList& add( Activity * anActivity );
    virtual const char * firstName() const;
    virtual const char * lastName() const;

protected:
    Cltn<Activity *> myActivityList;
};


/* -------------------------------------------------------------------- */

class AndOrForkActivityList : public ForkJoinActivityList
{
    friend class Task;
	
public:
    AndOrForkActivityList( Task * owner, LQIO::DOM::ActivityList *  );
    virtual ~AndOrForkActivityList();
    virtual void configure( const unsigned, const unsigned );
	
    virtual ActivityList& add( Activity * anActivity );

    virtual ActivityList * prev() const { return prevLink; }	/* Link to join list 		*/

    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;

    virtual double prBranch( const unsigned i ) const = 0;

protected:
    virtual AndOrForkActivityList& prev( ActivityList * aList) { prevLink = aList; return *this; }
    Entry * aggregateToEntry( const unsigned i, Stack<Entry *>&entryStack, const AndForkActivityList * forkList,
			      const unsigned submodel, const unsigned p, unsigned& next_p, AggregateFunc aFunc );

    
protected:
    Cltn<Entry *> myEntries;

private:
    ActivityList * prevLink;
};


/* -------------------------------------------------------------------- */

class OrForkActivityList : public AndOrForkActivityList
{
public:
    OrForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist ) : AndOrForkActivityList( owner, dom_activitylist ) {}
	
    virtual ActivityList& add( Activity * anActivity );
    virtual activity_type myType() const { return OR_FORK; }
    virtual void check() const;

    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>&  ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

    virtual double prBranch( const unsigned i ) const;

protected:
    virtual const char * typeStr() const { return "|"; }
};

/* -------------------------------------------------------------------- */

class AndForkActivityList : public AndOrForkActivityList
{
    friend class AndJoinActivityList;
    
public:
    AndForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist );
	
    virtual ActivityList& add( Activity * anActivity );
    virtual double prBranch( const unsigned ) const { return 1.0; }
    virtual activity_type myType() const { return AND_FORK; }
    virtual void check() const;

    bool isDescendentOf( const AndForkActivityList * ) const;

    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>&  ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

    virtual void insertDOMResults(void) const;

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
    AndOrJoinActivityList( Task * owner, LQIO::DOM::ActivityList *  );
    virtual void configure( const unsigned, const unsigned );
	
    virtual ActivityList * next() const { return nextLink; }	/* Link to fork list		*/
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;

protected:
    virtual AndOrJoinActivityList& next( ActivityList * aList ) { nextLink = aList; return *this; }

protected:
    mutable unsigned visits;

private:
    ActivityList *nextLink;
};


/* -------------------------------------------------------------------- */

class OrJoinActivityList : public AndOrJoinActivityList
{
public:
    OrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist ) : AndOrJoinActivityList( owner, dom_activitylist ) {}
	
    virtual activity_type myType() const { return OR_JOIN; }

    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>&  ) const;
    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual const char * typeStr() const { return "|"; }
};

/* -------------------------------------------------------------------- */

class AndJoinActivityList : public AndOrJoinActivityList
{
public:
    typedef enum { JOIN_NOT_DEFINED, INTERNAL_FORK_JOIN, SYNCHRONIZATION_POINT } join_type;

    AndJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist );
    virtual ActivityList& add( Activity * anActivity );

    virtual unsigned size() const { return myActivityList.size(); }
    void quorumListNum( unsigned quorumListNum) {myQuorumListNum = quorumListNum; }
    int quorumListNum() const { return myQuorumListNum; }  
    void quorumCount ( unsigned quorumCount) { myQuorumCount = quorumCount; }
    unsigned quorumCount () const { return myQuorumCount;}

    virtual activity_type myType() const { return AND_JOIN; }
    virtual void check() const;

    bool isSynchPoint() const { return myJoinType == SYNCHRONIZATION_POINT; }
    bool isInternalForkJoin() const { return myJoinType == INTERNAL_FORK_JOIN; }
    bool joinType( const join_type );
    bool hasQuorum() const { return 0 < quorumCount() && quorumCount() < size(); }
	
    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>&  ) const;
    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

private:
    virtual const char * typeStr() const { return "&"; }
    bool addToSrcList( unsigned i, const Activity * ) const;

private:
    mutable join_type myJoinType;		/* Barrier synch point.	*/
    mutable Cltn<const AndForkActivityList *> myForkList;
    mutable Cltn<const Activity *> mySrcList;
    mutable unsigned myQuorumCount;
    unsigned  myQuorumListNum;
};


/* -------------------------------------------------------------------- */

class RepeatActivityList : public ForkActivityList
{
public:
    RepeatActivityList( Task * owner, LQIO::DOM::ActivityList * dom_activitylist );
    virtual ~RepeatActivityList();
    virtual void configure( const unsigned, const unsigned );
    virtual ActivityList& add( Activity * anActivity );
	
    virtual activity_type myType() const { return REPEAT; }

    virtual ActivityList * prev() const { return prevLink; }	/* Link to join list 		*/

    virtual unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>&  ) const;
    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const;
    virtual void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    virtual double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, AggregateFunc2 ) const;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual unsigned concurrentThreads( unsigned ) const;

protected:
    virtual const char * typeStr() const { return "*"; }
    virtual RepeatActivityList& prev( ActivityList * aList) { prevLink = aList; return *this; }

private:
    double rateBranch( const unsigned i ) const;

private:
    ActivityList * prevLink;
    Cltn<Activity *> myActivityList;
    Cltn<Entry *> myEntries;
};

/* Used by model.cc */

void add_reply_list ( Task* task, Activity* activity );
void add_activity_lists ( Task* task, Activity* activity );
void complete_activity_connections ();
#endif
