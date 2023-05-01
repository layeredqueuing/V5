/* -*- c++ -*-
< * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/task.h $
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
 * $Id: task.h 16700 2023-04-24 11:12:07Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_TASK_H
#define LQNS_TASK_H

#include <lqio/dom_task.h>
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
    enum class root_level_t { IS_NON_REFERENCE, IS_REFERENCE, HAS_OPEN_ARRIVALS };
    friend class Interlock;		// BUG_425 deprecate

    struct sum {
	typedef double (Task::*funcPtr)() const;
	sum( funcPtr f ) : _f(f) {}
	double operator()( double l, const Task* r ) const { return l + (r->*_f)(); }
	double operator()( double l, const Task& r ) const { return l + (r.*_f)(); }
    private:
	const funcPtr _f;
    };

private:
    struct find_max_depth {
	find_max_depth( Call::stack& callStack, bool directPath ) : _callStack(callStack), _directPath(directPath), _dstEntry(callStack.back()->dstEntry()) {}
	unsigned int operator()( unsigned int depth, const Entry * entry );
    private:
	Call::stack& _callStack;
	const bool _directPath;
	const Entry * _dstEntry;
    };
    
    struct add_customers {
	unsigned int operator()( unsigned int addend, const std::pair<const Task *,unsigned int>& augend ) const;
	unsigned int operator()( unsigned int addend, const Entity * augend ) const;	// BUG_425 deprecate
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
    static Task * find( const std::string&, unsigned int=1 );

protected:
    Task( LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries);
    Task( const Task&, unsigned int );
    virtual Task * clone( unsigned int ) = 0;

private:
    Task( const Task& ) = delete;
    Task& operator=( const Task& ) = delete;
    void cloneActivities( const Task& src, unsigned int replica );
    
public:
    virtual ~Task();
    
public:
    /* Initialization */

    Task& linkForkToJoin();
    virtual bool check() const;
    virtual Task& configure( const unsigned );
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    Task& initProcessor();
    virtual void initializeClient();
    virtual void reinitializeClient();
    Task& initCustomers( std::deque<const Task *>& stack, unsigned int customers );
    void initializeWait( const Submodel& submodel );
    Task& createInterlock();
    virtual Task& initThreads();

    void findParents();

    /* Instance Variable Access */

    virtual LQIO::DOM::Task * getDOM() const { return dynamic_cast<LQIO::DOM::Task *>(Entity::getDOM()); }
    int priority() const;
    virtual unsigned queueLength() const { return 0; }
    const Processor * getProcessor() const { return _processor; }
    bool hasProcessor() const { return _processor != nullptr; }
    const Group * getGroup() const { return _group; }
    virtual unsigned int population() const;

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
#if PAN_REPLICATION
    void clearSurrogateDelay();
#endif
    Task& addThread( Thread * aThread ) { _threads.push_back(aThread); return *this; }

    /* Queries */

    virtual bool isTask() const { return true; }
    bool isCalled() const;
    virtual bool hasInfinitePopulation() const { return false; }

    virtual bool hasActivities() const { return _activities.size() != 0 ? true : false; }
    bool hasThinkTime() const;
    bool hasForks() const { return _has_forks; }
    bool hasSyncs() const { return _has_syncs; }
    bool hasQuorum() const { return _has_quorum; }
    double  processorUtilization() const;

    virtual unsigned nClients() const;		// # Calling tasks
    virtual unsigned nThreads() const;
    virtual unsigned concurrentThreads() const { return _maxThreads; }
	
    std::set<Entity *> getServers( const std::set<Entity *>& ) const;	// Called tasks/processors
	
    Task& addClientChain( const unsigned submodel, const unsigned k ) { _clientChains[submodel].push_back(k); return *this; }
    const ChainVector& clientChains( const unsigned submodel ) const { return _clientChains[submodel]; }
    Server* clientStation( const unsigned submodel ) const { return _clientStation[submodel]; }
    virtual Task* mapToReplica( size_t ) const;

    /* Model Building. */

    virtual root_level_t rootLevel() const;

    Task& expand();
    Task& expandCalls();

    Server * makeClient( const unsigned, const unsigned  );
    void saveClientResults( const MVASubmodel&, const Server&, unsigned int chain );
    const Task& closedCallsPerform( Call::Perform ) const;	// Copy arg.
    const Task& openCallsPerform( Call::Perform ) const;	// Copy arg.

    /* Computation */
	
    virtual void recalculateDynamicValues();

    void computeThroughputBound();
    virtual Task& computeVariance();
    Task& updateWait( const Submodel&, const double );
#if PAN_REPLICATION
    virtual double updateWaitReplication( const Submodel&, unsigned& );
#endif

    /* Threads */

    const Vector<Thread *>& threads() const { return _threads; }	 	/* My Threads.			*/
    unsigned threadIndex( const unsigned submodel, const unsigned k ) const;
    void forkOverlapFactor( const Submodel& ) const;
    double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
#if PAN_REPLICATION
    double waitExceptChain( const unsigned ix, const unsigned submodel, const unsigned k, const unsigned p ) const;
#endif

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
    std::ostream& printOverlapTable( std::ostream& output, const ChainVector&, const Vector<double>* ) const;
    virtual std::ostream& printJoinDelay( std::ostream& ) const;
    static SRVNIntManip print_client_chains( const Task & aTask, const unsigned aSubmodel ) { return SRVNIntManip( output_client_chains, aTask, aSubmodel ); }

private:
    SRVNManip print_activities() const { return SRVNManip( output_activities, *this ); }

    static std::ostream& output_activities( std::ostream& output, const Task& );
    static std::ostream& output_client_chains( std::ostream& output, const Task& aClient, const unsigned aSubmodel ) { aClient.printClientChains( output, aSubmodel ); return output; }

protected:
    bool HOL_Scheduling() const;
    bool PPR_Scheduling() const;

private:
#if PAN_REPLICATION
    Task& setSurrogateDelaySize( size_t );
#endif
    double bottleneckStrength() const;
    void store_activity_service_time ( const char * activity_name, const double service_time );	// quorum.

    /* Thread stuff */

    double overlapFactor( const unsigned i, const unsigned j ) const;


private:
    const Processor * _processor;	/* proc. allocated to task. 	*/
    const Group * _group;		/* Group allocated to task.	*/
    std::vector<Activity *> _activities;	/* Activities for this task.	*/
    std::vector<ActivityList *> _precedences;	/* Items I own for deletion.	*/

    unsigned _maxThreads;		/* Max concurrent threads.	*/
    double _overlapFactor;		/* Aggregate input o.f.		*/

    
    /* MVA interface */

    Vector<Thread *> _threads;	 	/* My Threads.			*/
    std::map<const Task *,unsigned int> _customers;
    Vector<ChainVector> _clientChains;	/* Client chains by submodel	*/
    Vector<Server *> _clientStation;	/* Clients by submodel.		*/

    bool _has_forks;
    bool _has_syncs;
    bool _has_quorum;
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
    virtual ~ReferenceTask() = default;

protected:
    ReferenceTask( const ReferenceTask& task, unsigned int replica ) : Task( task, replica ) {}
    virtual Task * clone( unsigned int );

public:
    virtual unsigned copies() const;
    
    virtual void initializeClient();
    virtual void reinitializeClient();

    virtual bool check() const;
    virtual void recalculateDynamicValues();
    virtual unsigned findChildren( Call::stack&, const bool ) const;

    virtual bool isReferenceTask() const { return true; }
    virtual bool hasVariance() const { return false; }
    virtual bool isUsed() const { return true; }
    virtual root_level_t rootLevel() const { return root_level_t::IS_REFERENCE; }

    Server * makeServer( const unsigned );

    virtual const Task& sanityCheck() const;
    
protected:
    virtual scheduling_type defaultScheduling() const { return SCHEDULE_CUSTOMER; }
    virtual bool schedulingIsOK() const { return scheduling() == SCHEDULE_CUSTOMER; }
};


/* -------------------------- Server Tasks ---------------------------- */

class ServerTask : public Task {
public:
    ServerTask(LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries)
	: Task(dom,aProc,aGroup,entries) {}
    virtual ~ServerTask() = default;

protected:
    ServerTask( const ServerTask& task, unsigned int replica ) : Task( task, replica ) {}
    virtual Task * clone( unsigned int );

public:
    virtual bool check() const;
    virtual unsigned queueLength() const;

    virtual bool hasVariance() const;
    virtual bool hasInfinitePopulation() const;

    virtual Server * makeServer( const unsigned );

protected:
    virtual bool schedulingIsOK() const;
    virtual scheduling_type defaultScheduling() const;
};


/* ------------------------ Semaphore Tasks --------------------------- */

class SemaphoreTask : public Task {
    SemaphoreTask(LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries)
	: Task(dom,aProc,aGroup,entries) {}
    virtual ~SemaphoreTask() = default;

protected:
    SemaphoreTask( const SemaphoreTask& task, unsigned int replica ) : Task( task, replica ) {}
    virtual Task * clone( unsigned int );

    virtual bool schedulingIsOK() const { return scheduling() == SCHEDULE_SEMAPHORE; }

public:
    virtual bool check() const;
    virtual SemaphoreTask& configure( const unsigned );

    virtual Server * makeServer( const unsigned );
};
#endif
