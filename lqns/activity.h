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
 * $Id: activity.h 13949 2020-10-18 16:02:42Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _ACTIVITY_H
#define _ACTIVITY_H

#include <config.h>
#include "dim.h"
#include <string>
#include <set>
#include <deque>
#include <stack>
#include <lqio/dom_activity.h>
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
class AndOrForkActivityList;

class Task;
class Format;
class ActivityCall;
class Call;
class Path;

template <class type> class Stack;

typedef void (Activity::*AggregateFunc)(Entry *,const unsigned,const unsigned);
/* -------------------------------------------------------------------- */
/*                               Activity                               */
/* -------------------------------------------------------------------- */

/*
 * Very much like a phase, but linked to other objects.
 */

class Activity : public Phase
{
    friend void add_activity_lists ( Task* task, Activity* activity );
    friend class Task;				/* To access add_... */

    /*
     * Helper class for exec.
     */

public:
    class Count_If;
    class Collect;
    
    typedef bool (Activity::*Predicate)( Count_If& ) const;
    typedef void (Activity::*Function)( Entry *, const Collect& ) const;
    
    class Count_If {
    public:
	Count_If() : _e(nullptr), _f(nullptr), _p(0), _replyAllowed(false), _rate(0.0), _sum(0.0) {}
	Count_If( const Entry* e, const Predicate f ) : _e(e), _f(f), _p(1), _replyAllowed(true), _rate(1.0), _sum(0.0) {}

	Count_If& operator=( const Count_If& src );
	Count_If& operator=( double value ) { _sum = value; return *this; }
	Count_If& operator+=( double addend ) { _sum += addend; return *this; }
	double sum() const { return _sum; }
	unsigned int phase() const { return _p; }
	void setPhase( unsigned int p ) { _p = p; }
	bool canReply() const { return _replyAllowed; }
	void setReplyAllowed( bool arg ) { _replyAllowed = arg; }
	double rate() const { return _rate; }
	void setRate( double rate ) { _rate = rate; }
	const Entry* entry() const { return _e; }
	const Predicate count_if() const { return _f; }

    private:
	const Entry* _e;
	Predicate _f;
	unsigned int _p;
	bool _replyAllowed;
	double _rate;
	double _sum;
    };

    class Collect {
    public:
	Collect() : _f(nullptr), _submodel(0), _p(0), _rate(1) {}
	Collect( unsigned int submodel, Function f ) : _f(f), _submodel(submodel), _p(1), _rate(1) {}

	Function collect() const { return _f; }
	unsigned int submodel() const { return _submodel; }
	unsigned int phase() const { return _p; }
	double rate() const { return _rate; }
	void setRate( double rate ) { _rate = rate; }
	void setPhase( unsigned int p ) { _p = p; }
	
    private:
	Function _f;
	unsigned int _submodel;
	unsigned int _p;
	double _rate;
    };

public:
    Activity( const Task * aTask, const std::string& aName );
    virtual ~Activity();

private:
    Activity( const Activity& ) { abort(); }		/* Copying is verbotten */
    Activity& operator=( const Activity& ) { abort(); return *this; }

public:
    virtual Activity& configure( const unsigned );

    /* Instance Variable access */
	
    virtual LQIO::DOM::Activity* getDOM() const { return dynamic_cast<LQIO::DOM::Activity*>(Phase::getDOM()); }
	
    virtual const std::string& name() const { return getDOM()->getName(); }
    virtual const Entity * owner() const { return _task; }

    bool activityDefined() const;
    ActivityList * prevFork( ActivityList * aList );
    ActivityList * prevFork() const { return _prevFork; }
    ActivityList * nextJoin( ActivityList * aList ); 
    ActivityList * nextJoin() const { return _nextJoin; }
    Activity& resetInputOutputLists();

    Activity& add_calls();
    Activity& add_reply_list();
    Activity& add_activity_lists();

    const std::set<const Entry *>& replyList() { return _replyList; }

    virtual Call * findOrAddCall( const Entry *, const queryFunc = 0 );
    virtual Call * findOrAddFwdCall( const Entry * anEntry );

    /*Quorum tomari*/
    bool localQuorumDelay() { return myLocalQuorumDelay;}
    void localQuorumDelay(bool localQuorumDelay ) { myLocalQuorumDelay= localQuorumDelay;}

    /* Queries */

    virtual bool check() const;

    virtual double throughput() const { return _throughput; }	/* Throughput results.		*/
    virtual bool repliesTo( const Entry * ) const;
    virtual bool isActivity() const { return true; }
    bool isReachable() const { return _reachable; }
    bool isNotReachable() const;
    Activity& isSpecified( const bool yesOrNo ) { _specified = yesOrNo; return *this; }
    bool isSpecified() const { return _specified; }
    bool isStartActivity() const { return entry() != 0; }

    /* Computation */

    bool checkReplies( Activity::Count_If& data ) const;
    void collectWait( Entry *, const Activity::Collect& ) const;
    void collectReplication( Entry *, const Activity::Collect& ) const;
    void collectServiceTime( Entry *, const Activity::Collect& ) const;
    void setThroughput( Entry *, const Activity::Collect& ) const;


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

    unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndOrForkActivityList *>&, double ) const;
    const Activity& backtrack( const std::deque<const AndOrForkActivityList *>& forkStack, std::set<const AndOrForkActivityList *>& forkSet ) const;
    virtual const Activity& followInterlock( Interlock::CollectTable& ) const;
    Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Collect& ) const;
    Count_If& count_if( std::deque<const Activity *>&, Count_If& ) const;
    virtual void callsPerform( const CallExec& ) const;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& path ) const;
    unsigned concurrentThreads( unsigned ) const;
    /* XML output */

    const Activity& insertDOMResults() const;
	
protected:
    virtual ProcessorCall * newProcessorCall( Entry * procEntry );

private:
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
    const Entity * _task;			/*				*/
    ActivityList * _prevFork;			/* Fork list which calls me	*/
    ActivityList * _nextJoin;			/* Join which I call.		*/
	
    std::set<const Entry *> _replyList;		/* Who I generate replies to.	*/
    double _rate;
    bool _specified;				/* Set if defined		*/
    mutable bool _reachable;			/* Set if activity is reachable	*/

    double _throughput;				/* My throughput.		*/
    bool myLocalQuorumDelay;
};


class PsuedoActivity : public Activity
{
public:
    PsuedoActivity( const Task * aTask, const std::string& aName ) : Activity( aTask, aName ) {}
    virtual ~PsuedoActivity() {}

    virtual bool isPseudo() const { return true; }	/* Allow Phase::initProcessor to create proc entry. */
};

class activity_cycle : public path_error 
{
public:
    activity_cycle( const Activity *, const std::deque<const Activity *>& );
    virtual ~activity_cycle() throw() {}
};

Activity * add_activity( Task* newTask, LQIO::DOM::Activity* activity );
void add_activity_calls ( Task* task, Activity* activity );
#endif
