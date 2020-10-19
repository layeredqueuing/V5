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
 * $Id: actlist.h 13949 2020-10-18 16:02:42Z greg $
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
struct InterlockInfo;
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
	
    virtual ActivityList& add( Activity * anActivity ) = 0;
    virtual bool check() const { return true; }

    const Task * owner() const { return _task; }
    const LQIO::DOM::ActivityList * getDOM() const { return _dom; }
    virtual ActivityList * next() const;	/* Link to fork list 		*/
    virtual ActivityList * prev() const;	/* Link to Join list		*/

    virtual bool isFork() const { return false; }
    virtual bool isSync() const { return false; }
    
    /* Computation */

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const = 0;
    virtual void followInterlock( Interlock::CollectTable& ) const = 0;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& ) = 0;
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const = 0;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& ) const = 0;
    virtual void callsPerform( const Phase::CallExec& ) const = 0;
    virtual unsigned concurrentThreads( unsigned ) const = 0;

    virtual void backtrack( const std::deque<const AndOrForkActivityList *>& forkStack, std::set<const AndOrForkActivityList *>& forkList ) const = 0;

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
	
    virtual ActivityList * prev() const { return _prev; }	/* Link to fork list 		*/

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const;
    virtual void backtrack( const std::deque<const AndOrForkActivityList *>& forkStack, std::set<const AndOrForkActivityList *>& forkList ) const { prev()->backtrack( forkStack, forkList ); }
    virtual void followInterlock( Interlock::CollectTable& ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& ) const;
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
	
    virtual ActivityList * next() const { return _next; }	/* Link to Join list		*/

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const;
    virtual void backtrack( const std::deque<const AndOrForkActivityList *>& forkStack, std::set<const AndOrForkActivityList *>& forkList ) const { getActivity()->backtrack( forkStack, forkList ); }
    virtual void followInterlock( Interlock::CollectTable& ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& ) const;
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
private:
    /* Used to concatentate activity list names into a string */
    struct fold {
	fold( const std::string& op ) : _op(op) {}
	std::string operator()( const std::string& s1, const Activity * a2 ) const { return s1 + " " + _op + " " + a2->name(); }
    private:
	const std::string _op;
    };

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
    friend class AndOrJoinActivityList;

protected:
    struct Sum
    {
	Sum( const AndOrForkActivityList * self ) : _self(self), _sum(0) {}
	void operator()( const Activity * object ) { _sum += _self->prBranch(object); }
	double sum() const { return _sum; }
    private:
	const AndOrForkActivityList * _self;
	double _sum;
    };
	
public:
    AndOrForkActivityList( Task * owner, LQIO::DOM::ActivityList * );
    virtual ~AndOrForkActivityList();

    virtual AndOrForkActivityList& configure( const unsigned );
	
    bool hasNextFork() const;
    ActivityList * getNextFork() const;

    virtual ActivityList * prev() const { return _prev; }	/* Link to join list 		*/
    virtual const AndOrJoinActivityList * joinList() const { return _joinList; }

    virtual bool check() const;
    
    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const;
    virtual void backtrack( const std::deque<const AndOrForkActivityList *>& forkStack, std::set<const AndOrForkActivityList *>& forkSet ) const;
    virtual void followInterlock( Interlock::CollectTable& ) const;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& ) const;

    virtual double prBranch( const Activity * ) const = 0;

    virtual std::ostream& printSubmodelWait( std::ostream& output, unsigned offset ) const;

protected:
    virtual AndOrForkActivityList& prev( ActivityList * aList) { _prev = aList; return *this; }
    Entry * collectToEntry( const Activity *, Entry *, std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );

private:
    void setJoinList( const AndOrJoinActivityList * joinList ) { _joinList = joinList; }

protected:    
    std::vector<Entry *> _entryList;
    mutable const AndForkActivityList * _parentForkList;
    
private:
    const AndOrJoinActivityList * _joinList;
    ActivityList * _prev;
};


/* -------------------------------------------------------------------- */

class OrForkActivityList : public AndOrForkActivityList
{
public:
    OrForkActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : AndOrForkActivityList( owner, dom ) {}
	
    virtual OrForkActivityList& add( Activity * anActivity );
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
    virtual bool isFork() const { return true; }

    virtual double prBranch( const Activity * ) const { return 1.0; }

    virtual bool check() const;

    bool isDescendentOf( const AndForkActivityList * ) const;

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const;
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
	
    double getNextRate() const { return 1.0; }

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const;
    virtual void backtrack( const std::deque<const AndOrForkActivityList *>& forkStack, std::set<const AndOrForkActivityList *>& forkList ) const;

    virtual ActivityList * next() const { return _next; }	/* Link to fork list		*/
    const AndOrForkActivityList * forkList() const { return _forkList; }

protected:
    virtual AndOrJoinActivityList& next( ActivityList * aList ) { _next = aList; return *this; }
    virtual bool updateRate( const Activity *, double ) { return false; }

private:
    void setForkList( const AndOrForkActivityList * forkList ) { _forkList = forkList; }

private:
    const AndOrForkActivityList * _forkList;
    ActivityList *_next;
};


/* -------------------------------------------------------------------- */

class OrJoinActivityList : public AndOrJoinActivityList
{
private:
    struct add_rate { double operator()( const double l, const std::pair<const Activity *,double>& r ) { return l + r.second; } };
    
public:
    OrJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom ) : AndOrJoinActivityList( owner, dom ), _rateList() {}
	
    double getNextRate() const { return _rate; }

    /* Most operations are done by the OrForkActivityList by following the next after all branches have been done */

    virtual void followInterlock( Interlock::CollectTable& path ) const {} 	/* NOP */
    virtual bool getInterlockedTasks( Interlock::CollectTasks& ) const { return false; }	/* NOP */
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& data ) { return data; }			/* NOP */
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& data ) const { return data; }			/* NOP */
    virtual void callsPerform( const Phase::CallExec& ) const {}		/* NOP - done by fork */
    virtual unsigned concurrentThreads( unsigned n ) const { return n; }	/* NOP - done by fork */


protected:
    virtual const char * typeStr() const { return "|"; }
    virtual bool updateRate( const Activity *, double );

private:
    std::map<const Activity *,double> _rateList;
    double _rate;
};

/* -------------------------------------------------------------------- */

class AndJoinActivityList : public AndOrJoinActivityList
{
public:
    typedef enum { JOIN_NOT_DEFINED, INTERNAL_FORK_JOIN, SYNCHRONIZATION_POINT } join_type;

    AndJoinActivityList( Task * owner, LQIO::DOM::ActivityList * dom );

    virtual bool check() const;
    
    void quorumListNum( unsigned quorumListNum) {myQuorumListNum = quorumListNum; }
    int quorumListNum() const { return myQuorumListNum; }  
    void quorumCount ( unsigned quorumCount) { myQuorumCount = quorumCount; }
    unsigned quorumCount () const { return myQuorumCount;}

    virtual bool isSync() const { return _joinType == SYNCHRONIZATION_POINT; }
    bool joinType( const join_type );
    bool hasQuorum() const { return 0 < quorumCount() && quorumCount() < activityList().size(); }
	
    virtual void followInterlock( Interlock::CollectTable& ) const;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& ) const;
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
	
    virtual ActivityList * prev() const { return _prev; }	/* Link to join list 		*/
    const std::vector<const Activity *>& activityList() const { return _activityList; }

    virtual unsigned findChildren( Call::stack&, bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const;
    virtual void followInterlock( Interlock::CollectTable& ) const;
    virtual Activity::Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Activity::Collect& );
    virtual const Activity::Count_If& count_if( std::deque<const Activity *>&, Activity::Count_If& ) const;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& ) const;
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
