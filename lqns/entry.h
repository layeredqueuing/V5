/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/entry.h $
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: entry.h 15046 2021-10-05 21:52:16Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(ENTRY_H)
#define ENTRY_H

#include <lqio/input.h>
#include <lqio/dom_entry.h>
#include <set>
#include <vector>
#include <mva/prob.h>
#include <mva/vector.h>
#include "call.h"
#include "activity.h"
#include "slice.h"

namespace LQIO {
    namespace DOM {
        class Call;
	class Phase;
    };
}

class Activity;
class Call;
class Entity;
class Entry;
class Exponential;
class MVASubmodel;
class Model;
class Processor;
class Submodel;
class Task;
typedef Vector<unsigned> ChainVector;

/* -------------------- Nodes in the graph are... --------------------- */

class Entry 
{
    friend class Interlock;		/* To access interlock */
    friend class Activity;		/* To access phase[].wait */
    friend class ActivityList;		/* To access phase[].wait */
    friend class OrForkActivityList;	/* To access phase[].wait */
    friend class AndForkActivityList;	/* To access phase[].wait */
    friend class RepeatActivityList;	/* To access phase[].wait */
    friend class CallInfo;

public:
    enum class RequestType { NOT_CALLED, RENDEZVOUS, SEND_NO_REPLY, OPEN_ARRIVAL };

    
    class CallExec {
    public:
	CallExec( callFunc f, unsigned submodel, unsigned k=0 ) : _f(f), _submodel(submodel), _k(k)  {}
	void operator()( Entry * object ) const { object->callsPerform( _f, _submodel, _k ); }
	
	const callFunc f() const { return _f; }
	unsigned submodel() const { return _submodel; }
	unsigned k() const { return _k; }

    private:
	const callFunc _f;
	const unsigned _submodel;
	const unsigned _k;
    };


    /*
     * Used to run f over all entries (and threads).  Each thread has it's own chain.
     */
    
    class CallExecWithChain {
    public:
	CallExecWithChain( callFunc f, unsigned submodel, const ChainVector& chains, unsigned int i ) : _f(f), _submodel(submodel), _chains(chains), _i(i)  {}
	void operator()( Entry * object ) const { object->callsPerform( _f, _submodel, _chains[_i] ); _i += 1; }
	
	const callFunc f() const { return _f; }
	unsigned submodel() const { return _submodel; }
	unsigned int index() const { return _i; }

    private:
	const callFunc _f;
	const unsigned _submodel;
	const ChainVector& _chains;
	mutable unsigned int _i;
    };


    struct get_clients {
	get_clients( std::set<Task *>& clients ) : _clients(clients) {}
	void operator()( const Entry * entry ) const;
    private:
	std::set<Task *>& _clients;
    };

    struct get_servers {
	get_servers( std::set<Entity *>& servers ) : _servers(servers) {}
	void operator()( const Entry * entry ) const;
    private:
	std::set<Entity *>& _servers;
    };


    /*
     * Compare two entries by their name and replica number.  The
     * default replica is one, and will only not be one if replicated
     * entries are expanded to individual entries.
     */
    
    struct equals {
	equals( const std::string& name, unsigned int replica=1 ) : _name(name), _replica(replica) {}
	bool operator()( const Entry * entry ) const;
    private:
	const std::string _name;
	const unsigned int _replica;
    };
    

protected:
    struct add_wait {
	add_wait( unsigned int submodel ) : _submodel(submodel) {}
	double operator()( double sum, const Phase& phase ) const { return sum + phase._wait[_submodel]; }
    private:
	const unsigned int _submodel;
    };


public:
    static bool joinsPresent;
    static bool deterministicPhases;
    static unsigned totalOpenArrivals;
    static unsigned max_phases;		/* maximum phase encountered.	*/
	
    int operator==( const Entry& anEntry ) const;
    static void reset();
    static Entry * find( const std::string&, unsigned int=1 );
    static Entry * create( LQIO::DOM::Entry* domEntry, unsigned int );
    static bool max_phase( const Entry * e1, const Entry * e2 ) { return e1->maxPhase() < e2->maxPhase(); }
	
protected:
    /* Instance creation */

    Entry( LQIO::DOM::Entry *, unsigned int, bool=true );
    Entry( const Entry&, unsigned int replica );

private:
    Entry& operator=( const Entry& ) = delete;

public:
    virtual ~Entry();
    virtual Entry * clone( unsigned int, const AndOrForkActivityList * fork=nullptr ) const = 0;

public:
    bool check() const;
    virtual Entry& configure( const unsigned );
    Entry& expand();
    Entry& expandCalls();
    unsigned findChildren( Call::stack&, const bool ) const;
    virtual Entry& initProcessor() = 0;
    virtual Entry& initWait() = 0;
    Entry& initThroughputBound();
    Entry& initServiceTime();
#if PAN_REPLICATION
    Entry& initReplication( const unsigned );	// REPL
#endif
    Entry& resetInterlock();
    Entry& createInterlock();
    Entry& initInterlock( Interlock::CollectTable& path );

    /* Instance Variable access */

    unsigned int index() const { return _index; }
    unsigned int entryId() const { return _entryId; }
    const Phase& getPhase( unsigned int p ) const { return _phase[p]; }
    LQIO::DOM::Phase::Type phaseTypeFlag( const unsigned p ) const { return _phase[p].phaseTypeFlag(); }
    double openArrivalRate() const;
    double CV_sqr( const unsigned p ) const { return _phase[p].CV_sqr(); }
    double computeCV_sqr( const unsigned p ) const { return _phase[p].computeCV_sqr(); }
    double computeCV_sqr() const;
    int priority() const;
    bool setIsCalledBy( const RequestType callType );
    bool isCalledUsing( const RequestType callType ) const { return callType == _calledBy; }
    bool isCalled() const { return _calledBy != RequestType::NOT_CALLED; }
    Entry& setEntryInformation( LQIO::DOM::Entry * entryInfo );
    virtual Entry& setDOM( unsigned phase, LQIO::DOM::Phase* phaseInfo );
    Entry& setForwardingInformation( Entry* toEntry, LQIO::DOM::Call *);
    Entry& addServiceTime( const unsigned, const double );
    double serviceTimeForPhase( const unsigned int p ) const { return _phase[p].serviceTime(); }
    double serviceTime() const { return _total.serviceTime(); }
    double throughput() const { return _throughput; }
    double throughputBound() const { return _throughputBound; }
    Entry& rendezvous( Entry *, const unsigned, const LQIO::DOM::Call* callDOMInfo );
    double rendezvous( const Entry * ) const;
    const Entry& rendezvous( const Entity *, VectorMath<double>& ) const;
    Entry& sendNoReply( Entry *, const unsigned, const LQIO::DOM::Call* callDOMInfo );
    double sendNoReply( const Entry * ) const;
    double sumOfSendNoReply( const unsigned p ) const;
    Entry& forward( Entry *, const LQIO::DOM::Call* callDOMInfo  );
    double forward( const Entry * anEntry ) const { return _phase[1].forward( anEntry ); }
    const Activity * getStartActivity() const { return _startActivity; }
    Entry& setStartActivity( Activity * );
    virtual double processorCalls( const unsigned ) const = 0;
    double processorCalls() const; 
    bool phaseIsPresent( const unsigned p ) const { return _phase[p].isPresent(); }
    virtual double openWait() const { return 0.; }
    LQIO::DOM::Entry* getDOM() const { return _dom; }
#if PAN_REPLICATION
    Entry& resetReplication();
#endif
    unsigned getReplicaNumber() const { return _replica_number; }
	
    void addDstCall( Call * aCall ) { _callerList.insert(aCall); }
    void removeDstCall( Call *aCall ) { _callerList.erase(aCall); }
    unsigned callerListSize() const { return _callerList.size(); }
    const std::set<Call *>& callerList() const { return _callerList; }
    const std::set<Call *>& callList(unsigned p) const { return _phase[p].callList(); }
    Call * processorCall(unsigned p) const { return _phase[p].processorCall(); }

    /* Queries */

    const std::string& name() const { return getDOM()->getName(); }
    virtual const Entity * owner() const = 0;
    virtual Entry& owner( const Entity * ) = 0;
	
    virtual bool isTaskEntry() const { return false; }
    virtual bool isVirtualEntry() const { return false; }
    virtual bool isProcessorEntry() const { return false; }
    bool isActivityEntry() const { return _entryType == LQIO::DOM::Entry::Type::ACTIVITY; }
    bool isStandardEntry() const { return _entryType == LQIO::DOM::Entry::Type::STANDARD; }
    bool isSignalEntry() const { return _semaphoreType == LQIO::DOM::Entry::Semaphore::SIGNAL; }
    bool isWaitEntry() const { return _semaphoreType == LQIO::DOM::Entry::Semaphore::WAIT; }
    bool isInterlocked( const Entry * ) const;
    bool isReferenceTaskEntry() const;
	
    bool hasDeterministicPhases() const { return getDOM()->hasDeterministicPhases(); }
    bool hasNonExponentialPhases() const { return getDOM()->hasNonExponentialPhases(); }
    bool hasThinkTime() const { return getDOM()->hasThinkTime(); }
    bool hasVariance() const;
    bool hasStartActivity() const { return _startActivity != nullptr; }
    bool hasOpenArrivals() const { return getDOM()->hasOpenArrivalRate(); }
		
    bool entryTypeOk( const LQIO::DOM::Entry::Type );
    bool entrySemaphoreTypeOk( const LQIO::DOM::Entry::Semaphore aType );
    unsigned maxPhase() const { return _phase.size(); }
    unsigned concurrentThreads() const;
    std::set<Entity *>& getServers( const std::set<Entity *>& ) const;	// Called tasks/processors

    double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
#if PAN_REPLICATION
    double waitExceptChain( const unsigned, const unsigned, const unsigned ) const; //REP N-R
#endif
//    double waitTime( unsigned int submodel ) { return _total.getWaitTimey(submodel); }
    double getProcWait( const unsigned p, int submodel )  { return _phase[p].getProcWait(submodel, 0) ;}	

    double elapsedTime() const { return _total.elapsedTime(); }			/* Found through deltaWait  */
    double elapsedTimeForPhase( const unsigned int p) const { return _phase[p].elapsedTime(); }	/* For server service times */

    double varianceForPhase( const unsigned int p ) const { return _phase[p].variance(); }
    double variance() const { return _total.variance(); }
	
    double utilization() const;
    virtual double processorUtilization() const = 0;
    virtual double queueingTime( const unsigned ) const = 0;	// Time queued for processor.
    Probability prVisit() const;

    virtual double getStartTime() const { return 0.0; }
    virtual double getStartTimeVariance() const { return 0.0; }

    /* Computation */

    void add_call( unsigned p, const LQIO::DOM::Call * dom );
    void sliceTime( const Entry& dst, Slice_Info phase_info[], double y_xj[] ) const;
    virtual Entry& computeVariance() { return *this; }
    virtual Entry& updateWait( const Submodel&, const double ) = 0;
#if PAN_REPLICATION
    virtual double updateWaitReplication( const Submodel&, unsigned& ) = 0;
#endif
    Entry& saveClientResults( const MVASubmodel& submodel, const Server& station, unsigned int k );
    virtual Entry& saveOpenWait( const double aWait ) = 0;
    Entry& saveThroughput( double );

    const Entry& followInterlock( Interlock::CollectTable& ) const;
    bool getInterlockedTasks( Interlock::CollectTasks& ) const;

    void set( const Entry * src, const Activity::Collect& );
    Entry& aggregate( const unsigned, const unsigned p, const Exponential& );
#if PAN_REPLICATION
    Entry& aggregateReplication( const Vector< VectorMath<double> >& );
#endif

    const Entry& callsPerform( callFunc aFunc, const unsigned submodel, const unsigned k ) const;

    /* Dynamic Updates / Late Finalization */
    /* In order to integrate LQX's support for model changes we need to have a way  */
    /* of re-calculating what used to be static for all dynamically editable values */
	
    Entry& recalculateDynamicValues();
    Entry& sanityCheckParameters();
	
    /* Sanity checks */

    bool checkDroppedCalls() const;

    /* XML output */
 
    void insertDOMQueueingTime(void) const;
    const Entry& insertDOMResults(double *phaseUtils) const;

    /* Printing */

    std::ostream& printCalls( std::ostream& output, unsigned int submodel=0 ) const;
    std::ostream& printSubmodelWait( std::ostream& output, unsigned offset ) const;
    static std::string fold( const std::string& s1, const Entry * e2 );

protected:
    Entry& setMaxPhase( const unsigned phase );

private:
    void setThroughput( const double throughput ) { _throughput = throughput; }

protected:
    LQIO::DOM::Entry* _dom;	
    Vector<Phase> _phase;
    NullPhase _total;
    double _nextOpenWait;			/* copy for delta computation	*/

    /* Activity Entries */
	
    Activity * _startActivity;			/* Starting activity.		*/

private:
    const unsigned _entryId;			/* Gobal entry id. (for chain)	*/
    const unsigned short _index;		/* My index (for mva)		*/
    LQIO::DOM::Entry::Type _entryType;
    LQIO::DOM::Entry::Semaphore _semaphoreType;	/* Extra type information	*/
    RequestType _calledBy;			/* true if entry referenced.	*/
    double _throughput;				/* Computed throughput.		*/
    double _throughputBound;			/* Type 1 throughput bound.	*/
	
    std::set<Call *> _callerList;		/* Who calls me.		*/

    Vector<InterlockInfo> _interlock;		/* Interlock table.		*/
    const unsigned int _replica_number;		/* > 1 if a replica		*/
};

/* --------------------------- Task Entries --------------------------- */


class TaskEntry : public Entry 
{
public:
    TaskEntry( LQIO::DOM::Entry* domEntry, unsigned int index, bool global=true );

protected:
    TaskEntry( const TaskEntry& src, unsigned int replica );
    
public:
    virtual Entry * clone( unsigned int replica, const AndOrForkActivityList * fork=nullptr ) const { return new TaskEntry( *this, replica ); }

    virtual TaskEntry& initProcessor();
    virtual TaskEntry& initWait();

    virtual TaskEntry& owner( const Entity * aTask ) { _task = aTask; return *this; }
    virtual const Entity * owner() const { return _task; }

    virtual bool isTaskEntry() const { return true; }

    virtual double processorCalls( const unsigned ) const;

    virtual double processorUtilization() const;
    virtual double queueingTime( const unsigned ) const;		// Time queued for processor.
    virtual TaskEntry& computeVariance();
    virtual TaskEntry& updateWait( const Submodel&, const double );
#if PAN_REPLICATION
    virtual double updateWaitReplication( const Submodel&, unsigned& );
#endif
    virtual TaskEntry& saveOpenWait( const double aWait ) { _nextOpenWait = aWait; return *this; }
    virtual double openWait() const { return _openWait; }
    
private:
    const Entity * _task;			/* My task.			*/
    double _openWait;				/* Computed open response time.	*/
    double _nextOpenWait;			/* copy for delta computation	*/
};

/* -------------------------- Device Entries -------------------------- */

class DeviceEntry : public Entry 
{
public:
    DeviceEntry( LQIO::DOM::Entry* domEntry, Processor * );

private:
    DeviceEntry( const DeviceEntry& src, unsigned int replica );

public:
    virtual ~DeviceEntry();
    virtual Entry * clone( unsigned int replica, const AndOrForkActivityList * fork=nullptr ) const { return new DeviceEntry( *this, replica ); }

    virtual DeviceEntry& initProcessor();
    virtual DeviceEntry& initWait();
    DeviceEntry& initVariance();

    virtual DeviceEntry& owner( const Entity * );
    virtual const Entity * owner() const { return _processor; }

    DeviceEntry& setServiceTime( const double );
    DeviceEntry& setPriority( const int );
    DeviceEntry& setCV_sqr( const double );

    virtual double processorCalls( const unsigned ) const;

    virtual bool isProcessorEntry() const { return true; }

    virtual double processorUtilization() const;
    virtual double queueingTime( const unsigned ) const;		// Time queued for processor.
    virtual DeviceEntry& updateWait( const Submodel&, const double );
    virtual DeviceEntry& saveOpenWait( const double aWait ) { return *this; }
#if PAN_REPLICATION
    virtual double updateWaitReplication( const Submodel&, unsigned& );
#endif

private:
    const Entity * _processor;
};

/* ------------------------- Virtual Entries -------------------------- */

/*
 * Used by class AndOrForkActivityList.
 */

class VirtualEntry : public TaskEntry 
{
public:
    VirtualEntry( const Activity * anActivity );

protected:
    VirtualEntry( const VirtualEntry& src, unsigned int replica ) : TaskEntry( src, replica ) {}

public:
    ~VirtualEntry();
    virtual Entry * clone( unsigned int replica, const AndOrForkActivityList * fork=nullptr ) const { return new VirtualEntry( *this, replica ); }

    virtual bool isVirtualEntry() const { return true; }
    virtual Call * processorCall( const unsigned ) const { return nullptr; }
};

void set_start_activity (Task* newTask, LQIO::DOM::Entry* targetEntry);

/* ------------------ Proxy messages for class call. ------------------ */


/*
 * Forward request to associated entry.  Defined here rather than in
 * class body due to foward reference problems.  Inlined.
 */

inline const Entity * Call::dstTask() const { return _destination->owner(); }
inline short Call::index() const { return _destination->index(); }
inline double Call::serviceTime() const { return _destination->serviceTimeForPhase(1); }
#endif
