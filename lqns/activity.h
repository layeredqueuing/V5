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
 * $Id: activity.h 13705 2020-07-20 21:46:53Z greg $
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

    /*
     * Helper class for exec.
     */

public:
    class Exec;
    class Collect;
    
    typedef bool (Activity::*Predicate)( Exec& ) const;
    typedef void (Activity::*Function)( Entry *, const Collect& );
    
    class Exec {
    public:
	Exec() : _e(nullptr), _f(nullptr), _p(0), _replyAllowed(false), _rate(0.0), _sum(0.0) {}
	Exec( const Entry* e, const Predicate f ) : _e(e), _f(f), _p(1), _replyAllowed(true), _rate(1.0), _sum(0.0) {}

	Exec& operator=( const Exec& src );
	Exec& operator=( double value ) { _sum = value; return *this; }
	Exec& operator+=( double addend ) { _sum += addend; return *this; }
	double sum() const { return _sum; }
	double phase() const { return _p; }
	void setPhase( unsigned int p ) { _p = p; }
	bool canReply() const { return _replyAllowed; }
	void setReplyAllowed( bool arg ) { _replyAllowed = arg; }
	double rate() const { return _rate; }
	void setRate( double rate ) { _rate = rate; }
	const Entry* entry() const { return _e; }
	const Predicate exec() const { return _f; }

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
    virtual const Entity * owner() const { return myTask; }

    bool activityDefined() const;
    ActivityList * inputFrom( ActivityList * aList );
    ActivityList * outputTo( ActivityList * aList ); 
    ActivityList * outputTo() { return outputToList;}
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

    virtual double throughput() const { return myThroughput; }	/* Throughput results.		*/
    virtual bool repliesTo( const Entry * ) const;
    virtual bool isActivity() const { return true; }
    bool isReachable() const { return iAmReachable; }
    Activity& isSpecified( const bool yesOrNo ) { iAmSpecified = yesOrNo; return *this; }
    bool isSpecified() const { return iAmSpecified; }
    bool isStartActivity() const { return entry() != 0; }

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

    unsigned findChildren( Call::stack&, const bool, std::deque<const Activity *>&, std::deque<const AndForkActivityList *>& ) const;
    std::deque<const AndForkActivityList *>::const_iterator backtrack( const std::deque<const AndForkActivityList *>& ) const;
    virtual unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo&, const unsigned );
    Collect& collect( std::deque<Entry *>&, Collect& );
    Exec& exec( std::deque<const Activity *>&, Exec& ) const;
    virtual void callsPerform( const Entry *, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
    virtual bool getInterlockedTasks( std::deque<const Entry *>&, const Entity *, std::set<const Entity *>&, const unsigned ) const;
    unsigned concurrentThreads( unsigned ) const;
    /* XML output */

    const Activity& insertDOMResults() const;
	
protected:
    virtual ProcessorCall * newProcessorCall( Entry * procEntry );

private:
    bool checkReplies( Activity::Exec& data ) const;
    void collectWait( Entry *, const Activity::Collect& );
    void collectReplication( Entry *, const Activity::Collect& );
    void collectServiceTime( Entry *, const Activity::Collect& );
    void setThroughput( Entry *, const Activity::Collect& );

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
    const Entity * myTask;			/*				*/
    ActivityList * inputFromList;		/* Node which calls me		*/
    ActivityList * outputToList;		/* Node which I call.		*/
	
    std::set<const Entry *> _replyList;		/* Who I generate replies to.	*/
    double myThroughput;			/* My throughput.		*/
    bool iAmSpecified;				/* Set if defined		*/
    mutable bool iAmReachable;			/* Set if activity is reachable	*/
	
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
