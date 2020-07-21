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
 * $Id: task.h 13705 2020-07-20 21:46:53Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(TASK_H)
#define TASK_H

#include <config.h>
#include <cstring>
#include <lqio/dom_task.h>
#include "dim.h"
#include "entity.h"
#include "prob.h"
#include "call.h"

class Activity;
class ActivityList;
class Entry;
class Format;
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

private:
    class SRVNManip {
    public:
	SRVNManip( ostream& (*f)(ostream&, const Task & ), const Task & task ) : _f(f), _task(task) {}
    private:
	ostream& (*_f)( ostream&, const Task& );
	const Task & _task;

	friend ostream& operator<<(ostream & os, const SRVNManip& m ) { return m._f(os,m._task); }
    };

    class SRVNIntManip {
    public:
	SRVNIntManip( ostream& (*f)(ostream&, const Task &, const unsigned ),
			  const Task & task, const unsigned n ) : _f(f), _task(task), _n(n) {}
    private:
	ostream& (*_f)( ostream&, const Task&, const unsigned );
	const Task & _task;
	const unsigned _n;

	friend ostream& operator<<(ostream & os, const SRVNIntManip& m ) { return m._f(os,m._task,m._n); }
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
    virtual Task& configure( const unsigned );
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    Task& initProcessor();
    virtual Task& initWait();
    virtual Task& initPopulation();
    virtual Task& initThroughputBound();
    Task& initInterlock();
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
    bool hasJoins() const { return false; } // Fix me... 
    double  processorUtilization() const;

    virtual bool hasSynchs() const;
    virtual unsigned hasClientChain( const unsigned submodel, const unsigned k ) const;

    virtual unsigned nClients() const;		// # Calling tasks
    virtual unsigned nThreads() const;
    virtual unsigned concurrentThreads() const { return _maxThreads; }
	
    std::set<Entity *,Entity::LT> servers( const std::set<Entity *,Entity::LT>& ) const;	// Called tasks/processors
	
    Task& addClientChain( const unsigned submodel, const unsigned k ) { _clientChains[submodel].push_back(k); return *this; }
    const ChainVector& clientChains( const unsigned submodel ) const { return _clientChains[submodel]; }
    Server * clientStation( const unsigned submodel ) const { return _clientStation[submodel]; }

    /* Model Building. */

    virtual int rootLevel() const;
    Server * makeClient( const unsigned, const unsigned  );
    const Task& callsPerform( callFunc, const unsigned submodel ) const;
    const Task& openCallsPerform( callFunc, const unsigned submodel ) const;

    /* Computation */
	
    virtual Task& computeVariance();
    virtual Task& updateWait( const Submodel&, const double );
    virtual double updateWaitReplication( const Submodel&, unsigned& );
    virtual Task& recalculateDynamicValues();

    /* Threads */

    unsigned threadIndex( const unsigned submodel, const unsigned k ) const;
    void forkOverlapFactor( const Submodel&, VectorMath<double>* ) const;
    double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
    double waitExceptChain( const unsigned ix, const unsigned submodel, const unsigned k, const unsigned p ) const;

    /* Synchronization */

    virtual void joinOverlapFactor( const Submodel&, VectorMath<double>* ) const;
	
    /* Quorum */

#if HAVE_LIBGSL
    int expandQuorumGraph();
#endif

    /* Sanity Check */

    virtual const Task& sanityCheck() const;

    /* XML output */
    virtual const Task& insertDOMResults() const;

    /* Printing */
    ostream& print( ostream& output ) const;

    unsigned countCallList( unsigned ) const;
    unsigned countJoinList() const;

    ostream& printSubmodelWait( ostream& output ) const;
    ostream& printClientChains( ostream& output, const unsigned ) const;
    ostream& printOverlapTable( ostream& output, const ChainVector&, const VectorMath<double>* ) const;
    virtual ostream& printJoinDelay( ostream& ) const;
    static SRVNIntManip print_client_chains( const Task & aTask, const unsigned aSubmodel ) { return SRVNIntManip( output_client_chains, aTask, aSubmodel ); }

private:
    SRVNManip print_activities() const { return SRVNManip( output_activities, *this ); }
    SRVNManip print_entries() const { return SRVNManip( output_entries, *this ); }
    SRVNManip print_task_type() const { return SRVNManip( output_task_type, *this ); }

    static ostream& output_activities( ostream& output, const Task& );
    static ostream& output_entries( ostream& output, const Task& );
    static ostream& output_task_type( ostream& output, const Task& );
    static ostream& output_client_chains( ostream& output, const Task& aClient, const unsigned aSubmodel ) { aClient.printClientChains( output, aSubmodel ); return output; }

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
};

/* ------------------------- Reference Tasks -------------------------- */

class ReferenceTask : public Task {
public:
    ReferenceTask( LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries );

    virtual unsigned copies() const;
    
    virtual bool check() const;
    virtual ReferenceTask& recalculateDynamicValues();
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    virtual double countCallers( std::set<Task *>& reject ) const;

    virtual bool isReferenceTask() const { return true; }
    virtual bool hasVariance() const { return false; }
    virtual bool isUsed() const { return true; }
    virtual int rootLevel() const { return 0; }

    Server * makeServer( const unsigned );

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
