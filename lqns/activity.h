/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/activity.h $
 *
 * Everything you wanted to know about an activity, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * July 2007
 *
 * ------------------------------------------------------------------------
 * $Id: activity.h 11963 2014-04-10 14:36:42Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _ACTIVITY_H
#define _ACTIVITY_H

#include <config.h>
#include "dim.h"
#include <string>
#include <lqio/dom_activity.h>
#include "vector.h"
#include "phase.h"
#if HAVE_LIBGSL
#include "randomvar.h"
#else
class DiscretePoints;
class DiscreteCDFs;
#endif

class Entry;
class ActivityList;
class AndForkActivityList;

class Task;
class Format;
class ActivityCall;
class Call;
class Path;

template <class type> class Stack;
template <class type> class Cltn;

/* -------------------------------------------------------------------- */
/*                               Activity                               */
/* -------------------------------------------------------------------- */

/*
 * Very much like a phase, but linked to other objects.
 */

class Activity : public Phase
{
    friend class Entry;				/* To access aggregate functions */
    friend class TaskEntry;			/* To access aggregate functions */
    friend class ActivityList;			/* To access aggregate functions */
    friend class OrForkActivityList;		/* To access aggregate functions */
    friend class AndForkActivityList;		/* To access aggregate functions */
    friend class RepeatActivityList;		/* To access aggregate functions */
    friend class ForkActivityList;
    friend class JoinActivityList;
    friend class AndOrForkActivityList;
    friend class AndOrJoinActivityList;
    friend void add_activity_lists ( Task* task, Activity* activity );
    friend class Task;				/* To access add_... */

public:
    Activity( const Task * aTask, const char * aName );
    virtual ~Activity();

private:
    Activity( const Activity& ) { abort(); }		/* Copying is verbotten */
    Activity& operator=( const Activity& ) { abort(); return *this; }

public:
    virtual void configure( const unsigned, const unsigned );

    /* Instance Variable access */
	
    virtual LQIO::DOM::Activity* getDOM() const { return dynamic_cast<LQIO::DOM::Activity*>(Phase::getDOM()); }
	
    virtual const Entry * entry() const { return 0; }
    virtual const char * name() const { return getDOM()->getName().c_str(); }

    bool activityDefined() const;
    ActivityList * inputFrom( ActivityList * aList );
    ActivityList * outputTo( ActivityList * aList ); 
    ActivityList * outputTo() { return outputToList;}
    Activity& resetInputOutputLists();

    Activity& add_calls();
    Activity& add_reply_list();
    Activity& add_activity_lists();

    Activity& replyList( Cltn<Entry *> * );
    Cltn<Entry *> * replyList() { return myReplyList; }

    virtual Call * findOrAddCall( const Entry *, const queryFunc = 0 );
    virtual Call * findOrAddFwdCall( const Entry * anEntry );

    Activity& setRootEntry( const Entry * anEntry ) { myRootEntry = anEntry; return *this; }

    /*Quorum tomari*/
    bool localQuorumDelay() { return myLocalQuorumDelay;}
    void localQuorumDelay(bool localQuorumDelay ) { myLocalQuorumDelay= localQuorumDelay;}

    /* Queries */

    virtual const Entity * owner() const;

    void clearThroughput() { myThroughput = 0.0; }
    virtual double throughput() const { return myThroughput; }	/* Throughput results.		*/
    virtual bool repliesTo( const Entry * ) const;
    virtual bool isActivity() const { return true; }
    bool isReachable() const { return iAmReachable; }
    Activity& isSpecified( const bool yesOrNo ) { iAmSpecified = yesOrNo; return *this; }
    bool isSpecified() const { return iAmSpecified; }
    bool isStartActivity() const { return myRootEntry != 0; }

    /* Computation */

    unsigned countCallList( unsigned ) const;

    bool estimateQuorumJoinCDFs (DiscretePoints & sumTotal,
				 DiscreteCDFs & quorumCDFs,DiscreteCDFs & localCDFs,
				 DiscreteCDFs & remoteCDFs, 
				 const bool isThereQuorumDelayedThreads, bool & isQuorumDelayedThreadsActive,
				 double &totalParallelLocal,double &totalSequentialLocal);
    bool estimateThreepointQuorumCDF(double level1Mean, 
				     double level2Mean,  double avgNumCallsToLevel2Tasks,
				     DiscretePoints & sumTotal, DiscretePoints & sumLocal, 
				     DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs, 
				     DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,  
				     const bool isThereQuorumDelayedThreads, 
				     bool & isQuorumDelayedThreadsActive);
#if HAVE_LIBGSL
    bool estimateGammaQuorumCDF(phase_type phaseTypeFlag, double level1Mean, 
				double level2Mean,  double avgNumCallsToLevel2Tasks,
				DiscretePoints & sumTotal, DiscretePoints & sumLocal, 
				DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs, 
				DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,  
				const bool isThereQuorumDelayedThreads, 
				bool & isQuorumDelayedThreadsActive);
    bool estimateClosedFormGeoQuorumCDF(double level1Mean, 
					double level2Mean,  double avgNumCallsToLevel2Tasks,
					DiscretePoints & sumTotal, DiscretePoints & sumLocal, 
					DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs, 
					DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,  
					const bool isThereQuorumDelayedThreads, 
					bool & isQuorumDelayedThreadsActive);
    bool estimateClosedFormDetQuorumCDF(double level1Mean, 
					double level2Mean,  double avgNumCallsToLevel2Tasks,
					DiscretePoints & sumTotal, DiscretePoints & sumLocal, 
					DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs, 
					DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,  
					const bool isThereQuorumDelayedThreads, 
					bool & isQuorumDelayedThreadsActive);
#endif
    bool getLevelMeansAndNumberOfCalls(double & level1Mean, double & level2Mean,
				       double &  avgNumCallsToLevel2Tasks);

    /* Thread manipulation */

    unsigned findChildren( CallStack&, const bool, Stack<const Activity *>&, Stack<const AndForkActivityList *>& ) const;
    unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned );
    void aggregate( Stack<Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, unsigned&, AggregateFunc );
    double aggregate2( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, const AggregateFunc2 ) const;
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;
    unsigned concurrentThreads( unsigned ) const;

    /* XML output */

    void insertDOMResults() const;
	
protected:
    virtual ProcessorCall * newProcessorCall( Entry * procEntry );

private:
    void aggregateWait( Entry *, const unsigned submodel, const unsigned p );
    void aggregateReplication( Entry *, const unsigned submodel, const unsigned p );
    void setThroughput( Entry *, const unsigned submodel, const unsigned p );
    void aggregateServiceTime( Entry *, const unsigned submodel, const unsigned p );
    double aggregateReplies( const Entry * anEntry, const unsigned p, const double rate ) const;

    ActivityList * act_join_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_fork_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_loop_list( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist );

public:
    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> domToNative;

    //tomari, make it public for now.
#if HAVE_LIBGSL
    DiscretePoints remoteQuorumDelay;   	//tomari quorum
#endif
	 
private:
    string myName;				/* Duh...			*/
    const Task * myTask;			/*				*/
    ActivityList * inputFromList;		/* Node which calls me		*/
    ActivityList * outputToList;		/* Node which I call.		*/
	
    Cltn<Entry *> * myReplyList;		/* Who I generate replies to.	*/
    double myThroughput;			/* My throughput.		*/
    const Entry * myRootEntry;			/* Set if root activity.	*/
    bool iAmSpecified;				/* Set if defined		*/
    mutable bool iAmReachable;			/* Set if activity is reachable	*/
	
    bool myLocalQuorumDelay;
};


class PsuedoActivity : public Activity
{
public:
    PsuedoActivity( const Task * aTask, const char * aName ) : Activity( aTask, aName ) {}
    virtual ~PsuedoActivity() {}

    virtual bool isPseudo() const { return true; }	/* Allow Phase::initProcessor to create proc entry. */
};

class activity_cycle : public path_error 
{
public:
    activity_cycle( const Activity *, Stack<const Activity *>& );
    virtual ~activity_cycle() throw() {}
};

Activity * add_activity( Task* newTask, LQIO::DOM::Activity* activity );
void add_activity_calls ( Task* task, Activity* activity );
#endif
