/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/task.h $
 *
 * Tasks.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * May 2009.
 *
 * $Id: task.h 14140 2020-11-25 20:24:15Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(TASK_H)
#define TASK_H

#include <lqio/dom_task.h>
#include "dim.h"
#include "entity.h"
#include "call.h"

class Activity;
class ActivityList;
class Entry;
class Processor;
class Server;
class Submodel;
class Task;
class Thread;
class Group;

/* ----------------------- Abstract Superclass ------------------------ */

class Task : public Entity {

public:
    /*
     * Compare two tasks by their submodel. 
     */

    struct LT
    {
	bool operator()(const Task * t1, const Task * t2) const {
	    return (t1->submodel() < t2->submodel()) 
		|| (t1->getDOM() && (!t2->getDOM() || t1->getDOM()->getSequenceNumber() < t2->getDOM()->getSequenceNumber() ));
	}
    };

#if __cplusplus >= 201103L
    enum class root_level_t { IS_NON_REFERENCE, IS_REFERENCE, HAS_OPEN_ARRIVALS };
#else
    typedef enum { IS_NON_REFERENCE, IS_REFERENCE, HAS_OPEN_ARRIVALS } root_level_t;
#endif

private:
    struct find_max_depth {
	find_max_depth( Call::stack& callStack, bool directPath ) : _callStack(callStack), _directPath(directPath), _dstEntry(callStack.back()->dstEntry()) {}
	unsigned int operator()( unsigned int depth, const Entry * entry );
    private:
	Call::stack& _callStack;
	const bool _directPath;
	const Entry * _dstEntry;
    };
    
    class SRVNManip {
    public:
	SRVNManip( std::ostream& (*f)(std::ostream&, const Task & ), const Task & task ) : _f(f), _task(task) {}
    private:
	std::ostream& (*_f)( std::ostream&, const Task& );
	const Task & _task;

	friend std::ostream& operator<<(std::ostream & os, const SRVNManip& m ) { return m._f(os,m._task); }
    };

    class SRVNIntManip {
    public:
	SRVNIntManip( std::ostream& (*f)(std::ostream&, const Task &, const unsigned ),
			  const Task & task, const unsigned n ) : _f(f), _task(task), _n(n) {}
    private:
	std::ostream& (*_f)( std::ostream&, const Task&, const unsigned );
	const Task & _task;
	const unsigned _n;

	friend std::ostream& operator<<(std::ostream & os, const SRVNIntManip& m ) { return m._f(os,m._task,m._n); }
    };
    
public:
    static Task* create( LQIO::DOM::Task* domTask, const std::vector<Entry *>& entries );

protected:
    Task( LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries);

public:
    virtual ~Task();

    /* Initialization */

    static void reset();
    virtual bool check() const;
    bool checkReachability() const;
    virtual Task& configure( const unsigned );
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    Task& initProcessor();
    virtual Task& initWait();
    virtual Task& initPopulation();
    virtual Task& initThroughputBound();
    Task& createInterlock();
    virtual Task& initThreads();

    void findParents();
    virtual double countCallers( std::set<Task *>& reject ) const;

    /* Instance Variable Access */

    virtual LQIO::DOM::Task * getDOM() const { return dynamic_cast<LQIO::DOM::Task *>(Entity::getDOM()); }
    int priority() const;
    virtual unsigned queueLength() const { return 0; }
    const Processor * getProcessor() const { return _processor; }
    bool hasProcessor() const { return _processor != nullptr; }
    virtual const Group * group() const { return _group; }

    Activity * findActivity( const std::string& name ) const;
    Activity * findOrAddActivity( const std::string& name );
    Activity * findOrAddPsuedoActivity( const std::string& name );
    void addPrecedence( ActivityList * );
    const std::vector<ActivityList *>& precedences() const { return _precedences; }
    const std::vector<Activity *>& activities() const { return _activities; }
    virtual Entity& setOverlapFactor( const double of ) { _overlapFactor = of; return *this; }
    virtual double thinkTime( const unsigned = 0, const unsigned = 0 ) const;
    virtual unsigned int fanOut( const Entity * ) const;
    virtual unsigned int fanIn( const Task * ) const;
    void resetReplication();
    Task& addThread( Thread * aThread ) { _threads.push_back(aThread); return *this; }

    /* Queries */

    virtual bool isTask() const { return true; }
    bool isCalled() const;
    virtual bool hasInfinitePopulation() const { return false; }

    virtual bool hasActivities() const { return _activities.size() != 0 ? true : false; }
    bool hasThinkTime() const;
    bool hasForks() const;
    bool hasSyncs() const;
    double  processorUtilization() const;

    virtual unsigned hasClientChain( const unsigned submodel, const unsigned k ) const;

    virtual unsigned nClients() const;		// # Calling tasks
    virtual unsigned nThreads() const;
    virtual unsigned concurrentThreads() const { return _maxThreads; }
	
    std::set<Entity *> getServers( const std::set<Entity *>& ) const;	// Called tasks/processors
	
    Task& addClientChain( const unsigned submodel, const unsigned k ) { _clientChains[submodel].push_back(k); return *this; }
    const ChainVector& clientChains( const unsigned submodel ) const { return _clientChains[submodel]; }
    Server * clientStation( const unsigned submodel ) const { return _clientStation[submodel]; }

    /* Model Building. */

    virtual root_level_t rootLevel() const;
    Server * makeClient( const unsigned, const unsigned  );
    Task& initClientStation( Submodel& );
    Task& modifyClientServiceTime( const MVASubmodel& );
    Task& saveClientResults( const MVASubmodel& );
    const Task& callsPerform( callFunc, const unsigned submodel ) const;
    const Task& openCallsPerform( callFunc, const unsigned submodel ) const;
    const Task& setChain( const MVASubmodel& submodel ) const;

    /* Computation */
	
    virtual Task& recalculateDynamicValues();
    virtual Task& computeVariance();
    virtual Task& updateWait( const Submodel&, const double );
    virtual double updateWaitReplication( const Submodel&, unsigned& );
    Task& computeOvertaking( Entity * );

    /* Threads */

    unsigned threadIndex( const unsigned submodel, const unsigned k ) const;
    void forkOverlapFactor( const Submodel& ) const;
    double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
    double waitExceptChain( const unsigned ix, const unsigned submodel, const unsigned k, const unsigned p ) const;

    /* Synchronization */

    virtual void joinOverlapFactor( const Submodel& ) const;
	
    /* Quorum */

#if HAVE_LIBGSL
    int expandQuorumGraph();
#endif

    /* Sanity Check */

    virtual const Entity& sanityCheck() const;

    /* XML output */
    virtual const Task& insertDOMResults() const;

    /* Printing */
    std::ostream& print( std::ostream& output ) const;
    std::ostream& printSubmodelWait( std::ostream& output ) const;
    std::ostream& printClientChains( std::ostream& output, const unsigned ) const;
    std::ostream& printOverlapTable( std::ostream& output, const ChainVector&, const VectorMath<double>* ) const;
    virtual std::ostream& printJoinDelay( std::ostream& ) const;
    static SRVNIntManip print_client_chains( const Task & aTask, const unsigned aSubmodel ) { return SRVNIntManip( output_client_chains, aTask, aSubmodel ); }

private:
    SRVNManip print_activities() const { return SRVNManip( output_activities, *this ); }
    SRVNManip print_entries() const { return SRVNManip( output_entries, *this ); }
    SRVNManip print_task_type() const { return SRVNManip( output_task_type, *this ); }

    static std::ostream& output_activities( std::ostream& output, const Task& );
    static std::ostream& output_entries( std::ostream& output, const Task& );
    static std::ostream& output_task_type( std::ostream& output, const Task& );
    static std::ostream& output_client_chains( std::ostream& output, const Task& aClient, const unsigned aSubmodel ) { aClient.printClientChains( output, aSubmodel ); return output; }

protected:
    bool HOL_Scheduling() const;
    bool PPR_Scheduling() const;

private:
    Task& initReplication( const unsigned );	 	// REP N-R
    double bottleneckStrength() const;

    /* Thread stuff */

    double overlapFactor( const unsigned i, const unsigned j ) const;

    void store_activity_service_time ( const char * activity_name, const double service_time );	// quorum.

private:
    const Processor * _processor;	/* proc. allocated to task. 	*/
    const Group * _group;		/* Group allocated to task.	*/
    unsigned _maxThreads;		/* Max concurrent threads.	*/
    double _group_util;
    double _group_share;                /* share within a group         */

    std::vector<Activity *> _activities;	/* Activities for this task.	*/
    std::vector<ActivityList *> _precedences;	/* Items I own for deletion.	*/

    double _overlapFactor;		/* Aggregate input o.f.		*/

    /* MVA interface */

    Vector<Thread *> _threads;	 	/* My Threads.			*/
    Vector<ChainVector> _clientChains;	/* Client chains by submodel	*/
    Vector<Server *> _clientStation;	/* Clients by submodel.		*/

    mutable bool _has_fork;	/* Cached			*/
    mutable bool _has_sync;
    mutable bool _no_syncs;
};

/* ------------------------- Reference Tasks -------------------------- */

class ReferenceTask : public Task {
private:
    struct find_max_depth {
	find_max_depth( Call::stack& callStack ) : _callStack(callStack) {}
	unsigned int operator()( unsigned int depth, const Entry * entry );
    private:
	Call::stack& _callStack;
    };
    
    
public:
    ReferenceTask( LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries );

    virtual unsigned copies() const;
    
    virtual ReferenceTask& initClient( const Vector<Submodel *>& );
    virtual ReferenceTask& reinitClient( const Vector<Submodel *>& );

    virtual bool check() const;
    virtual ReferenceTask& recalculateDynamicValues();
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    virtual double countCallers( std::set<Task *>& reject ) const;

    virtual bool isReferenceTask() const { return true; }
    virtual bool hasVariance() const { return false; }
    virtual bool isUsed() const { return true; }
    virtual root_level_t rootLevel() const { return IS_REFERENCE; }

    Server * makeServer( const unsigned );

    virtual const Task& sanityCheck() const;
    
protected:
    virtual scheduling_type defaultScheduling() const { return SCHEDULE_CUSTOMER; }
};


/* -------------------------- Server Tasks ---------------------------- */

class ServerTask : public Task {
public:
    ServerTask(LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries)
	: Task(dom,aProc,aGroup,entries) /*myQueueLength(queue_length)*/ {}

    virtual bool check() const;
    virtual ServerTask& configure( const unsigned );
    virtual unsigned queueLength() const;

    virtual bool hasVariance() const;
    virtual bool hasInfinitePopulation() const;

    virtual Server * makeServer( const unsigned );

protected:
    virtual unsigned validScheduling() const;
    virtual scheduling_type defaultScheduling() const;
};


/* -------------------------- Server Tasks ---------------------------- */

class SemaphoreTask : public Task {
    SemaphoreTask(LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries)
	: Task(dom,aProc,aGroup,entries) /*myQueueLength(queue_length)*/ {}

    virtual bool check() const;
    virtual SemaphoreTask& configure( const unsigned );

    virtual Server * makeServer( const unsigned );
};
#endif
