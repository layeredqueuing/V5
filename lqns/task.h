/* -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/task.h $
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
 * $Id: task.h 11963 2014-04-10 14:36:42Z greg $
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
#include "cltn.h"

class Activity;
class ActivityList;
class Call;
class CallStack;
class Entry;
class Format;
class Processor;
class Server;
class Submodel;
class Task;
class Thread;
class Group;

/* -------------- A Sequence of all calls from this task  ------------- */

class TaskCallList {
public:
    TaskCallList( const Task * );
    virtual ~TaskCallList();
	
    Call * operator()();
    unsigned size() const { return callCltn.size(); }

private:
    Cltn<Call *> callCltn;
    unsigned index;
};


/* ----------------------- Abstract Superclass ------------------------ */

class Task : public Entity {
public:
    static Task* create( LQIO::DOM::Task* domTask, Cltn<Entry *> * entries );

protected:
    Task( LQIO::DOM::Task* domVersion, const Processor * aProc, const Group * aGroup, Cltn<Entry *>* entries);

public:
    virtual ~Task();

    /* Initialization */

    static void reset();
    virtual void check() const;
    virtual void configure( const unsigned );
    virtual unsigned findChildren( CallStack&, const bool ) const;
    void initProcessor();
    virtual Task& initWait();
    virtual Task& initPopulation();
    virtual Task& initThroughputBound();
    Task& initInterlock();
    virtual Task& initThreads();

    void findParents();
    virtual double countCallers( Cltn<const Task *> &reject );

    /* Instance Variable Access */

    Entity& priority( const int anInt ) { myDOMTask->setPriority(anInt); return *this; }
    int priority() const { return myDOMTask->getPriority(); }
    virtual unsigned queueLength() const { return 0; }
    virtual Entity& processor( Processor * aProcessor );
    virtual const Processor * processor() const { return myProcessor; }
    Activity * findActivity( const char * name ) const;
    Activity * findOrAddActivity( const char * name );
    Activity * findOrAddPsuedoActivity( const char * name );
    void addPrecedence( ActivityList * );
    const Cltn<Activity *>& activities() const { return myActivityList; }
    virtual Entity& setOverlapFactor( const double of ) { myOverlapFactor = of; return *this; }
    virtual double thinkTime( const unsigned = 0, const unsigned = 0 ) const;
    void resetReplication();

    Task& addThread( Thread * aThread ) { myThreads << aThread; return *this; }

    /* Queries */

    virtual bool isTask() const { return true; }
    bool isCalled() const;
    virtual bool hasInfinitePopulation() const { return false; }

    virtual bool hasActivities() const { return myActivityList.size() != 0 ? true : false; }
    bool hasThinkTime() const;
    bool hasForks() const;
    bool hasJoins() const { return false; } // Fix me... 

    virtual bool hasSynchs() const;
    virtual unsigned hasClientChain( const unsigned submodel, const unsigned k ) const { return myClientChains[submodel].find(k); }

    virtual unsigned nClients() const;		// # Calling tasks
    unsigned nServers() const;			// # Called tasks/processors
    virtual unsigned nThreads() const;
    virtual unsigned concurrentThreads() const { return maxThreads; }
	
    unsigned servers( Cltn<Entity *> & ) const;	// Called tasks/processors
    unsigned servers( Cltn<Entity *> &, const Cltn<Entity *> & ) const;	// Called tasks/processors
	
    Task& addClientChain( const unsigned submodel, const unsigned k ) { myClientChains[submodel].append(k); return *this; }
    const ChainVector& clientChains( const unsigned submodel ) const { return myClientChains[submodel]; }
    Server * clientStation( const unsigned submodel ) const { return myClientStation[submodel]; }

    /* Model Building. */

    virtual int rootLevel() const;
    Server * makeClient( const unsigned, const unsigned  );
    const Task& callsPerform( callFunc, const unsigned submodel ) const;
    const Task& openCallsPerform( callFunc, const unsigned submodel ) const;

    /* Computation */
	
    virtual Task& computeVariance();
    virtual Task& updateWait( const Submodel&, const double );
    virtual double updateWaitReplication( const Submodel&, unsigned& );
    virtual void recalculateDynamicValues();

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

    virtual void sanityCheck() const;

    /* XML output */
    void insertDOMResults(void) const;

    /* Printing */
    ostream& print( ostream& output ) const;

    unsigned countCallList( unsigned ) const;
    unsigned countJoinList() const;

    ostream& printSubmodelWait( ostream& output ) const;
    ostream& printClientChains( ostream& output, const unsigned ) const;
    ostream& printOverlapTable( ostream& output, const ChainVector&, const VectorMath<double>* ) const;
    virtual ostream& printJoinDelay( ostream& ) const;

protected:
    bool HOL_Scheduling() const;
    bool PPR_Scheduling() const;

private:
    Task& initReplication( const unsigned );	 	// REP N-R
    void accumulateCallers( Cltn<const Task *> & sources ) const;
    unsigned countMultiServer( const Cltn<const Task *> & sources ) const;
    void findOrAddServer( const Cltn<Call *> &, Cltn<Entity *> &, const Cltn<Entity *> * includeOnly = 0) const;
    double bottleneckStrength() const;

    /* Thread stuff */

    double overlapFactor( const unsigned i, const unsigned j ) const;

    void store_activity_service_time ( const char * activity_name, const double service_time ) ;	// quorum.

protected:
    LQIO::DOM::Task* myDOMTask;		/* Stores all of the data.      */
	
private:
    const Processor * myProcessor;	/* proc. allocated to task. 	*/
    const Group * myGroup;		/* Group allocated to task.	*/
    double myOverlapFactor;		/* Aggregate input o.f.		*/
    unsigned maxThreads;		/* Max concurrent threads.	*/

    Cltn<Activity *> myActivityList;	/* Activities for this task.	*/
    Cltn<ActivityList *> myPrecedence;	/* Items I own for deletion.	*/

    /* MVA interface */

    Cltn<Thread *> myThreads;	 	/* My Threads.			*/
    Vector<ChainVector> myClientChains;	/* Client chains by submodel	*/
    Cltn<Server *> myClientStation;	/* Clients by submodel.		*/
};

/* ------------------------- Reference Tasks -------------------------- */

class ReferenceTask : public Task {
public:
    ReferenceTask( LQIO::DOM::Task* domVersion,
		   const Processor * aProc, const Group * aGroup, Cltn<Entry *> *aCltn );

    virtual unsigned copies() const;
    
    virtual void check() const;
    virtual void recalculateDynamicValues();
    virtual unsigned findChildren( CallStack&, const bool ) const;
    virtual double countCallers( Cltn<const Task *> &reject );

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
    ServerTask(LQIO::DOM::Task* domVersion,
	       const Processor * aProc, const Group * aGroup, Cltn<Entry *> *aCltn)
	: Task(domVersion,aProc,aGroup,aCltn) /*myQueueLength(queue_length)*/ {}

    virtual void check() const;
    virtual void configure( const unsigned );
    virtual unsigned queueLength() const { return myDOMTask->getQueueLengthValue(); }

    virtual bool hasVariance() const;
    virtual bool hasInfinitePopulation() const;

    virtual Server * makeServer( const unsigned );

protected:
    virtual unsigned validScheduling() const;
    virtual scheduling_type defaultScheduling() const;
};


/* -------------------------- Server Tasks ---------------------------- */

class SemaphoreTask : public Task {
    SemaphoreTask(LQIO::DOM::Task* domVersion,
		  const Processor * aProc, const Group * aGroup, Cltn<Entry *> *aCltn)
	: Task(domVersion,aProc,aGroup,aCltn) /*myQueueLength(queue_length)*/ {}

    virtual void check() const;
    virtual void configure( const unsigned );

    virtual Server * makeServer( const unsigned );

};


/*
 * Compare a task name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eq_task_name
{
    eq_task_name( const char * s ) : _s(s) {}
    bool operator()(const Task * t ) const;

private:
    const char * _s;
};


extern set<Task *,ltTask> task;


/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNTaskManip {
public:
    SRVNTaskManip( ostream& (*ff)(ostream&, const Task & ), const Task & theTask ) : f(ff), aTask(theTask) {}
private:
    ostream& (*f)( ostream&, const Task& );
    const Task & aTask;

    friend ostream& operator<<(ostream & os, const SRVNTaskManip& m ) { return m.f(os,m.aTask); }
};

class SRVNTaskIntManip {
public:
    SRVNTaskIntManip( ostream& (*ff)(ostream&, const Task &, const unsigned ),
		      const Task & theTask, const unsigned theInt ) : f(ff), aTask(theTask), anInt(theInt) {}
private:
    ostream& (*f)( ostream&, const Task&, const unsigned );
    const Task & aTask;
    const unsigned anInt;

    friend ostream& operator<<(ostream & os, const SRVNTaskIntManip& m ) { return m.f(os,m.aTask,m.anInt); }
};

SRVNTaskManip print_activities( const Task & aTask );
SRVNTaskIntManip print_client_chains( const Task & aClient, const unsigned aSubmodel );
SRVNTaskManip task_type( const Task& aTask );
#endif
