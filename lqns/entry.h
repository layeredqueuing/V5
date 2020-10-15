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
 * $Id: entry.h 13930 2020-10-15 19:20:12Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(ENTRY_H)
#define ENTRY_H

#include <config.h>
#include "dim.h"
#include <cstdlib>
#include <lqio/input.h>
#include <lqio/dom_entry.h>
#include <set>
#include <vector>
#include <deque>
#include "prob.h"
#include "call.h"
#include "vector.h"
#include "phase.h"
#include "activity.h"
#include "interlock.h"

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
class Format;
class Model;
class Processor;
class ProcessorEntry;
class Slice_Info;
class Submodel;
class Task;

template <class type> class Stack;

typedef enum { ENTRY_NOT_DEFINED, STANDARD_ENTRY, ACTIVITY_ENTRY, DEVICE_ENTRY } entry_type;
typedef enum { NOT_CALLED, RENDEZVOUS_REQUEST, SEND_NO_REPLY_REQUEST, OPEN_ARRIVAL_REQUEST } requesting_type;

/* */

/*
 * Interface to parser.
 */


class CallInfoItem {
public:
    CallInfoItem( const Entry * src, const Entry * dst );
	
    bool hasRendezvous() const;
    bool hasSendNoReply() const;
    bool hasForwarding() const;
		
    bool isTaskCall() const;
    bool isProcessorCall() const;
	
    const Entry * srcEntry() const { return source; }
    const Entry * dstEntry() const { return destination; }

public:
    const Call * phase[MAX_PHASES+1];

private:
    const Entry * source; 
    const Entry * destination; 
};


class CallInfo {
    struct compare
    {
	compare( const Entry* dstEntry ) : _dstEntry(dstEntry) {}
	bool operator()( CallInfoItem& i ) { return i.dstEntry() == _dstEntry; }
    private:
	const Entry * _dstEntry;
    };
public:
    CallInfo( const Entry& anEntry, const unsigned );
    CallInfo( const CallInfo& ) { abort(); }					/* Copying is verbotten */
    CallInfo& operator=( const CallInfo& ) { abort(); return *this; }		/* Copying is verbotten */
	
    std::vector<CallInfoItem>::const_iterator begin() const { return _calls.begin(); }
    std::vector<CallInfoItem>::const_iterator end() const { return _calls.end(); }
    unsigned size() const { return _calls.size(); }

private:
    std::vector<CallInfoItem> _calls;
};


/* -------------------- Nodes in the graph are... --------------------- */

class Entry 
{
    friend class Interlock;		/* To access interlock */
    friend class Activity;		/* To access phase[].wait */
    friend class ActivityList;		/* To access phase[].wait */
    friend class OrForkActivityList;	/* To access phase[].wait */
    friend class AndForkActivityList;	/* To access phase[].wait */
    friend class RepeatActivityList;	/* To access phase[].wait */

public:
    static bool joinsPresent;
    static bool deterministicPhases;
    static unsigned totalOpenArrivals;
    static unsigned max_phases;		/* maximum phase encountered.	*/
    static const char * phaseTypeFlagStr [];
	
    int operator==( const Entry& anEntry ) const;
    static void reset();
    static Entry * find( const string& entry_name );
    static Entry * create( LQIO::DOM::Entry* domEntry, unsigned int );
	
protected:
    /* Instance creation */

    Entry( LQIO::DOM::Entry* aDomEntry, const unsigned, const unsigned );

public:
    virtual ~Entry();

private:
    Entry( const Entry& );
    Entry& operator=( const Entry& );

public:
    bool check() const;
    virtual Entry& configure( const unsigned );
    unsigned findChildren( Call::stack&, const bool ) const;
    virtual Entry& initProcessor() = 0;
    virtual Entry& initWait() = 0;
    Entry& initThroughputBound();
    Entry& initServiceTime();
    Entry& initReplication( const unsigned );	// REPL
    Entry& resetInterlock();
    unsigned initInterlock( std::deque<const Entry *>& stack, const InterlockInfo& globalCalls );

    /* Instance Variable access */

    unsigned int index() const { return _index; }
    unsigned int entryId() const { return _entryId; }
    phase_type phaseTypeFlag( const unsigned p ) const { return _phase[p].phaseTypeFlag(); }
    double openArrivalRate() const;
    double CV_sqr( const unsigned p ) const { return _phase[p].CV_sqr(); }
    double computeCV_sqr( const unsigned p ) const { return _phase[p].computeCV_sqr(); }
    double computeCV_sqr() const;
    int priority() const;
    bool setIsCalledBy( const requesting_type callType );
    bool isCalledUsing( const requesting_type callType ) const { return callType == _calledBy; }
    bool isCalled() const { return _calledBy != NOT_CALLED; }
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
    virtual Entry& setStartActivity( Activity * );
    virtual double processorCalls( const unsigned ) const = 0;
    double processorCalls() const; 
    bool phaseIsPresent( const unsigned p ) const { return _phase[p].isPresent(); }
    virtual double openWait() const { return 0.; }
    LQIO::DOM::Entry* getDOM() const { return _entryDOM; }
    Entry& resetReplication();
	
    void addDstCall( Call * aCall ) { _callerList.insert(aCall); }
    void removeDstCall( Call *aCall ) { _callerList.erase(aCall); }
    unsigned callerListSize() const { return _callerList.size(); }
    const std::set<Call *>& callerList() const { return _callerList; }
    const std::set<Call *>& callList(unsigned p) const { return _phase[p].callList(); }
    Call * processorCall(unsigned p) const { return _phase[p].processorCall(); }

    /* Queries */

    const std::string& name() const { return _entryDOM->getName(); }
    virtual const Entity * owner() const = 0;
    virtual Entry& owner( const Entity * ) = 0;
	
    virtual bool isTaskEntry() const { return false; }
    virtual bool isVirtualEntry() const { return false; }
    virtual bool isProcessorEntry() const { return false; }
    bool isActivityEntry() const { return _entryType == ACTIVITY_ENTRY; }
    bool isStandardEntry() const { return _entryType == STANDARD_ENTRY; }
    bool isSignalEntry() const { return _semaphoreType == SEMAPHORE_SIGNAL; }
    bool isWaitEntry() const { return _semaphoreType == SEMAPHORE_WAIT; }
    bool isInterlocked( const Entry * ) const;
    bool isReferenceTaskEntry() const;
	
    bool hasDeterministicPhases() const { return _entryDOM->hasDeterministicPhases(); }
    bool hasNonExponentialPhases() const { return _entryDOM->hasNonExponentialPhases(); }
    bool hasThinkTime() const { return _entryDOM->hasThinkTime(); }
    bool hasVariance() const;
    bool hasStartActivity() const { return _startActivity != 0; }
    bool hasOpenArrivals() const { return _entryDOM->hasOpenArrivalRate(); }
		
    bool entryTypeOk( const entry_type );
    bool entrySemaphoreTypeOk( const semaphore_entry_type aType );
    unsigned maxPhase() const { return _phase.size(); }
    unsigned concurrentThreads() const;

    double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
    double waitExceptChain( const unsigned, const unsigned, const unsigned ) const; //REP N-R
    double waitTime( int submodel )  { return _total.waitTime(submodel); }
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

    void add_call( const unsigned p, const LQIO::DOM::Call* domCall );
    void sliceTime( const Entry& dst, Slice_Info phase_info[], double y_xj[] ) const;
    virtual Entry& computeVariance() { return *this; }
    virtual Entry& updateWait( const Submodel&, const double ) = 0;
    virtual double updateWaitReplication( const Submodel&, unsigned& ) = 0;
    virtual Entry& saveOpenWait( const double aWait ) = 0;
    void saveThroughput( double );

    unsigned followInterlock( std::deque<const Entry *>&, const InterlockInfo& );
    void followForwarding( Phase *, const Entry *, const double, Stack<const Entity *>& ) const;
    bool getInterlockedTasks( Interlock::Collect& ) const;

    void set( const Entry * src, const Activity::Collect& );
    Entry& aggregate( const unsigned, const unsigned p, const Exponential& );
    Entry& aggregateReplication( const Vector< VectorMath<double> >& );

    const Entry& callsPerform( callFunc, const unsigned, const unsigned k = 0 ) const;

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

    std::ostream& printSubmodelWait( std::ostream& output, unsigned offset ) const;

protected:
    Entry& setMaxPhase( const unsigned phase );

private:
    void setThroughput( const double throughput ) { _throughput = throughput; }


protected:
    LQIO::DOM::Entry* _entryDOM;	
    Vector<Phase> _phase;
    NullPhase _total;
    double _nextOpenWait;			/* copy for delta computation	*/

    /* Activity Entries */
	
    Activity * _startActivity;			/* Starting activity.		*/

private:
    const unsigned _entryId;			/* Gobal entry id. (for chain)	*/
    const unsigned short _index;		/* My index (for mva)		*/
    entry_type _entryType;
    semaphore_entry_type _semaphoreType;	/* Extra type information	*/
    requesting_type _calledBy;			/* true if entry referenced.	*/
    double _throughput;				/* Computed throughput.		*/
    double _throughputBound;			/* Type 1 throughput bound.	*/
	
    std::set<Call *> _callerList;		/* Who calls me.		*/

    Vector<InterlockInfo> _interlock;		/* Interlock table.		*/
};

/* --------------------------- Task Entries --------------------------- */


class TaskEntry : public Entry 
{
public:
    TaskEntry( LQIO::DOM::Entry* domEntry, const unsigned id, const unsigned int index ) : Entry(domEntry,id,index), _task(0), _openWait(0.), _nextOpenWait(0.) {}

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
    virtual double updateWaitReplication( const Submodel&, unsigned& );
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
    DeviceEntry( LQIO::DOM::Entry* domEntry, const unsigned, Processor * );
    virtual ~DeviceEntry();

    virtual DeviceEntry& initProcessor();
    virtual DeviceEntry& initWait();
    DeviceEntry& initVariance();

    virtual DeviceEntry& owner( const Entity * aProcessor );
    virtual const Entity * owner() const { return myProcessor; }

    DeviceEntry& setServiceTime( const double );
    DeviceEntry& setPriority( const int );
    DeviceEntry& setCV_sqr( const double );

    virtual double processorCalls( const unsigned ) const;

    virtual bool isProcessorEntry() const { return true; }

    virtual double processorUtilization() const;
    virtual double queueingTime( const unsigned ) const;		// Time queued for processor.
    virtual DeviceEntry& updateWait( const Submodel&, const double );
    virtual DeviceEntry& saveOpenWait( const double aWait ) { return *this; }
    virtual double updateWaitReplication( const Submodel&, unsigned& );

private:
    const Entity * myProcessor;
};

/* ------------------------- Virtual Entries -------------------------- */

class VirtualEntry : public TaskEntry 
{
public:
    VirtualEntry( const Activity * anActivity );
    ~VirtualEntry();

    virtual bool isVirtualEntry() const { return true; }
    virtual Entry& setStartActivity( Activity * );

    virtual Call * processorCall( const unsigned ) const { return 0; }
};

void set_start_activity (Task* newTask, LQIO::DOM::Entry* targetEntry);

/* ------------------ Proxy messages for class call. ------------------ */


/*
 * Forward request to associated entry.  Defined here rather than in
 * class body due to foward reference problems.  Inlined.
 */

inline const Entity * Call::dstTask() const { return destination->owner(); }
inline short Call::index() const { return destination->index(); }
inline double Call::serviceTime() const { return destination->serviceTimeForPhase(1); }
#endif
