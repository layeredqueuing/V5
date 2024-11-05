/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/activity.h $
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
 * $Id: activity.h 17435 2024-11-05 22:11:46Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _ACTIVITY_H
#define _ACTIVITY_H

#include <set>
#include <map>
#include <deque>
#include <stack>
#include <numeric>
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
class AndOrJoinActivityList;

class Task;
class ActivityCall;
class Call;

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
    class State;
    
    typedef bool (Activity::*Predicate)( Count_If& ) const;
    
    /*
     * Support for backtracking up the precedence graph to find forks associated with joins.
     */
    
    class Backtrack {
    public:
	class State
	{
	public:
	    State( const std::deque<const Activity *>& activityStack, const std::deque<const AndOrForkActivityList *>& forkStack, std::set<const AndOrForkActivityList *>& forkSet ) :
		_activityStack(activityStack), _forkStack(forkStack), _forkSet(forkSet), _joinSet() {}
	    State( const State& src ) : _activityStack(src._activityStack), _forkStack(src._forkStack), _forkSet(src._forkSet), _joinSet(src._joinSet) {}
	private:
	    State& operator=( const State& src ) = delete;
	public:
	    const std::deque<const Activity *>& getActivityStack() const { return _activityStack; }	// For error handling only.
	    bool find_fork( const AndOrForkActivityList * fork ) const { return _forkSet.find( fork ) != _forkSet.end(); }
	    bool find_join( const AndOrJoinActivityList * join ) const { return _joinSet.find( join ) != _joinSet.end(); }
	    void insert_fork( const AndOrForkActivityList * fork ) { if ( std::find( _forkStack.begin(), _forkStack.end(), fork ) != _forkStack.end() ) _forkSet.insert( fork ); }
	    void insert_join( const AndOrJoinActivityList * join ) { _joinSet.insert( join ); }
	private:
	    const std::deque<const Activity *>& _activityStack;
	    const std::deque<const AndOrForkActivityList *>& _forkStack;
	    std::set<const AndOrForkActivityList *>& _forkSet;
	    std::set<const AndOrJoinActivityList *> _joinSet;
	};
    public:
	Backtrack( const State& state ) : _state(state) {}
	Backtrack( const Backtrack& src ) : _state(src._state) {}
    private:
	Backtrack& operator=( const Backtrack& ) = delete;
    public:
	void operator()( const Activity* activity ) { activity->backtrack(_state); }
	void operator()( const Activity& activity ) { activity.backtrack(_state); }
    private:
	State _state;
    };
    
    /*
     * Support for collecting stuff (like waiting times) when traversing the precedence graph.
     */
    
    class Collect {
    public:
	typedef void (Activity::*Function)( Entry *, const Collect& ) const;
	
	Collect() : _f(nullptr), _submodel(0), _p(0), _rate(1), _taskStack(nullptr), _customers(0) {}
	Collect( Function f, unsigned int submodel=0 ) : _f(f), _submodel(submodel), _p(1), _rate(1), _taskStack(nullptr), _customers(0) {}
	Collect( Function f, std::deque<const Task *>& stack, unsigned int customers ) : _f(f), _submodel(0), _p(0), _rate(1), _taskStack(&stack), _customers(customers) {}
	Collect( const Collect& ) = default;

    private:
	Collect& operator=( const Collect& ) = delete;
	
    public:
	Function collect() const { return _f; }
	unsigned int submodel() const { return _submodel; }
	unsigned int phase() const { return _p; }
	double rate() const { return _rate; }
	void setRate( double rate ) { _rate = rate; }
	void setPhase( unsigned int p ) { _p = p; }
	std::deque<const Task *>& taskStack() { return *_taskStack; }
	unsigned int customers() const { return _customers; }
	
    private:
	Function _f;
	unsigned int _submodel;
	unsigned int _p;
	double _rate;
	std::deque<const Task *>* _taskStack;	/* BUG_425 */
	unsigned int _customers;		/* BUG_425 */
    };

    /*
     * Add to count if the predicate returns true
     */
    
    class Count_If {
    public:
	Count_If() : _e(nullptr), _f(nullptr), _p(0), _replyAllowed(true), _rate(0.0), _sum(0.0) {}
	Count_If( const Entry* e, const Predicate f ) : _e(e), _f(f), _p(1), _replyAllowed(true), _rate(1.0), _sum(0.0) {}
	Count_If( const Count_If& src, double rate ) : _e(src._e), _f(src._f), _p(src._p), _replyAllowed(true), _rate(src._rate*rate), _sum(src._sum) {}

    private:
	Count_If( const Count_If& ) = delete;
	Count_If& operator()( const Count_If& ) = delete;

    public:
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

    class Ancestors {
    public:
	Ancestors( Call::stack& callStack, bool directPath, bool followCalls ) : _callStack(callStack), _directPath(directPath), _follow_calls(followCalls), _activityStack(), _forkStack(), _rate(1.), _replyAllowed(true) {}
	Ancestors( const Ancestors& src ) : _callStack(src._callStack), _directPath(src._directPath), _follow_calls(src._follow_calls), _activityStack(src._activityStack), _forkStack(src._forkStack), _rate(src._rate), _replyAllowed(src._replyAllowed) {}
	Ancestors( const Ancestors& src, double rate ) : _callStack(src._callStack), _directPath(src._directPath), _follow_calls(src._follow_calls), _activityStack(src._activityStack), _forkStack(src._forkStack), _rate(src._rate * rate), _replyAllowed(false) {}
	Ancestors( const Ancestors& src, std::deque<const AndOrForkActivityList *>& forkStack ) : _callStack(src._callStack), _directPath(src._directPath), _follow_calls(src._follow_calls), _activityStack(src._activityStack), _forkStack(forkStack), _rate(src._rate), _replyAllowed(src._replyAllowed) {}
	Ancestors( const Ancestors& src, std::deque<const AndOrForkActivityList *>& forkStack, double rate ) : _callStack(src._callStack), _directPath(src._directPath), _follow_calls(src._follow_calls), _activityStack(src._activityStack), _forkStack(forkStack), _rate(src._rate * rate), _replyAllowed(false) {}

	Call::stack& getCallStack() { return _callStack; }
	unsigned depth() const { return _callStack.depth(); }
	
	bool isDirectPath() const { return _directPath; }
	bool followCalls() const { return _follow_calls; }
	const std::deque<const Activity *>& getActivityStack() const { return _activityStack; }
	std::deque<const AndOrForkActivityList *>& getForkStack() { return _forkStack; }

	double getRate() const { return _rate; }
	void setRate( double rate ) { _rate = rate; }
	bool canReply() const { return _replyAllowed; }
	void setReplyAllowed( bool arg ) { _replyAllowed = arg; }

	bool find( const Activity * activity ) const { return std::find( _activityStack.begin(), _activityStack.end(), activity ) != _activityStack.end(); }
	void push_activity( const Activity * activity ) { _activityStack.push_back( activity ); }
	void pop_activity() { _activityStack.pop_back(); }
	const Activity * top_activity() { return _activityStack.empty() ? nullptr : _activityStack.back(); }
	void push_fork( const AndOrForkActivityList * fork_list ) { _forkStack.push_back( fork_list ); }
	void pop_fork() { _forkStack.pop_back(); }
	const AndOrForkActivityList * top_fork() { return _forkStack.empty() ? nullptr : _forkStack.back(); }

    private:
	Call::stack& _callStack;
	const bool _directPath;
	const bool _follow_calls;
	std::deque<const Activity *> _activityStack;		// For checking for cycles.
	std::deque<const AndOrForkActivityList *> _forkStack; 	// For matching forks/joins.
	double _rate;
	bool _replyAllowed;
    };

/* ------------------------------------------------------------------------ */
public:
    Activity( const Task *, const std::string& );
    Activity( const Activity&, const Task *, unsigned int replica );
    virtual ~Activity() = default;

    Activity * clone( const Task* task, unsigned int replica ) { return new Activity( *this, task, replica ); }

private:
    Activity& operator=( const Activity& ) = delete;

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

    const std::set<const Entry *>& replyList() const { return _replyList; }
    virtual unsigned int getReplicaNumber() const { return _replica_number; }

    virtual Call * findOrAddCall( const Entry *, const queryFunc = 0 );
    virtual Call * findOrAddFwdCall( const Entry * anEntry );

#if HAVE_LIBGSL
    /*Quorum tomari*/
    bool localQuorumDelay() const { return _local_quorum_delay; }
    void localQuorumDelay( bool local_quorum_delay ) { _local_quorum_delay = localQuorumDelay; }
#endif

    /* Queries */

    virtual bool check() const;

    virtual double throughput() const { return _throughput; }	/* Throughput results.		*/
    virtual bool repliesTo( const Entry * ) const;
    virtual bool isActivity() const { return true; }
    bool isReachable() const { return _reachable; }
    Activity& isSpecified( const bool yesOrNo ) { _specified = yesOrNo; return *this; }
    bool isSpecified() const { return _specified; }
    bool isStartActivity() const { return entry() != nullptr; }

    /* Computation */

    bool checkReplies( Activity::Count_If& data ) const;
    void collectWait( Entry *, const Activity::Collect& ) const;
#if PAN_REPLICATION
    void collectReplication( Entry *, const Activity::Collect& ) const;
#endif
    void collectServiceTime( Entry *, const Activity::Collect& ) const;
    void collectCustomers( Entry *, const Activity::Collect& ) const;
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
    bool estimateGammaQuorumCDF(LQIO::DOM::Phase::Type phaseTypeFlag, double level1Mean, 
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
    unsigned findChildren( Ancestors& ) const;
    const Activity& backtrack( Backtrack::State& data ) const;
    virtual const Activity& followInterlock( Interlock::CollectTable& ) const;
    Collect& collect( std::deque<const Activity *>&, std::deque<Entry *>&, Collect& ) const;
    Count_If& count_if( std::deque<const Activity *>&, Count_If& ) const;
    CallInfo::Item::collect_calls& collect_calls( std::deque<const Activity *>&, CallInfo::Item::collect_calls& ) const;
    virtual void callsPerform( Call::Perform& ) const;
    virtual bool getInterlockedTasks( Interlock::CollectTasks& path ) const;
    unsigned concurrentThreads( unsigned ) const;

    /* XML output */
    const Activity& insertDOMResults() const;

protected:
    virtual ProcessorCall * newProcessorCall( Entry * procEntry ) const;

private:
    ActivityList * act_join_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_fork_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_loop_list( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist );

public:
    static void completeConnections();
    static void clearConnectionMaps();
    static std::string fold( const std::string& s1, const Activity * a2 ) { return s1 + "," + a2->name(); }

private:
    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> __actConnections;
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> __domToNative;

private:
    const Entity * _task;			/*				*/
    ActivityList * _prevFork;			/* Fork list which calls me	*/
    ActivityList * _nextJoin;			/* Join which I call.		*/
	
    std::set<const Entry *> _replyList;		/* Who I generate replies to.	*/
    bool _specified;				/* Set if defined		*/
    mutable bool _reachable;			/* Set if activity is reachable	*/
    const unsigned int _replica_number;		/*				*/
    
#if HAVE_LIBGSL
public:
    DiscretePoints _remote_quorum_delay;   	//tomari quorum
private:
    bool _local_quorum_delay;
#endif

private:
    double _throughput;				/* My throughput.		*/
};


class PsuedoActivity : public Activity
{
public:
    PsuedoActivity( const Task * aTask, const std::string& aName ) : Activity( aTask, aName ) {}
    virtual ~PsuedoActivity() {}

    virtual bool isPseudo() const { return true; }	/* Allow Phase::initializeProcessor to create proc entry. */
};

class activity_cycle : public std::runtime_error
{
public:
    activity_cycle( const Activity *, const std::deque<const Activity *>& );
    virtual ~activity_cycle() throw() {}
};

Activity * add_activity( Task* newTask, LQIO::DOM::Activity* activity );
void add_activity_calls ( Task* task, Activity* activity );
#endif
