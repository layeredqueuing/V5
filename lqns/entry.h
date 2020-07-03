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
 * $Id: entry.h 13547 2020-05-21 02:22:16Z greg $
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
#include "prob.h"
#include "call.h"
#include "vector.h"
#include "phase.h"
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
template <class type> class Cltn;

typedef enum { ENTRY_NOT_DEFINED, STANDARD_ENTRY, ACTIVITY_ENTRY, DEVICE_ENTRY } entry_type;
typedef enum { NOT_CALLED, RENDEZVOUS_REQUEST, SEND_NO_REPLY_REQUEST, OPEN_ARRIVAL_REQUEST } requesting_type;

/* */

/*
 * Interface to parser.
 */


class CallInfoItem {
public:
    CallInfoItem( const Entry * src, const Entry * dst );
    virtual ~CallInfoItem();
	
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
public:
    CallInfo( const Entry * anEntry, const unsigned );
    CallInfo( const CallInfo& ) { abort(); }					/* Copying is verbotten */
    CallInfo& operator=( const CallInfo& ) { abort(); return *this; }		/* Copying is verbotten */
    virtual ~CallInfo();
	
    CallInfoItem* operator()();
    unsigned size() const { return itemCltn.size(); }

private:
    Cltn<CallInfoItem *> itemCltn;
    unsigned index;
};


/* -------------------- Nodes in the graph are... --------------------- */

class Entry 
{
    friend class Interlock;		/* To access interlock */
    friend class Activity;		/* To access phase[].wait */
    friend class ActivityList;		/* To access myThroughput/index */
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
    void check() const;
    virtual void configure( const unsigned, const unsigned = MAX_PHASES );
    unsigned findChildren( CallStack&, const bool ) const;
    virtual void initProcessor() = 0;
    virtual void initWait() = 0;
    void initThroughputBound();
    void initReplication( const unsigned );	// REPL
    Entry& resetInterlock();
    unsigned initInterlock( Stack<const Entry *>& stack, const InterlockInfo& globalCalls );

    /* Instance Variable access */

    unsigned int index() const { return _index; }
    unsigned int entryId() const { return _entryId; }
    phase_type phaseTypeFlag( const unsigned p ) const { return phase[p].phaseTypeFlag(); }
    double openArrivalRate() const;
    double CV_sqr( const unsigned p ) const { return phase[p].CV_sqr(); }
    double computeCV_sqr( const unsigned p ) const { return phase[p].computeCV_sqr(); }
    double computeCV_sqr() const;
    int priority() const;
    bool isCalled( const requesting_type callType );
    requesting_type isCalled() const { return calledFlag; }
    Entry& setEntryInformation( LQIO::DOM::Entry * entryInfo );
    virtual Entry& setDOM( unsigned phase, LQIO::DOM::Phase* phaseInfo );
    Entry& setForwardingInformation( Entry* toEntry, LQIO::DOM::Call *);
    Entry& addServiceTime( const unsigned, const double );
    double serviceTime( const unsigned p ) const { return phase[p].serviceTime(); }
    double serviceTime() const { return total.serviceTime(); }
    double thinkTime( const unsigned p ) const { return phase[p].thinkTime(); }
    double throughput() const { return myThroughput; }
    Entry& throughput( const double );
    double throughputBound() const { return myThroughputBound; }
    Entry& rendezvous( Entry *, const unsigned, LQIO::DOM::Call* callDOMInfo );
    double rendezvous( const Entry * anEntry, const unsigned p ) const { return phase[p].rendezvous( anEntry ); }
    double rendezvous( const Entry * ) const;
    void rendezvous( const Entity *, VectorMath<double>& ) const;
    Entry& sendNoReply( Entry *, const unsigned, LQIO::DOM::Call* callDOMInfo );
    double sendNoReply( const Entry * anEntry, const unsigned p ) const { return phase[p].sendNoReply( anEntry ); }
    double sendNoReply( const Entry * ) const;
    double sumOfSendNoReply( const unsigned p ) const;
    Entry& forward( Entry *, LQIO::DOM::Call* callDOMInfo  );
    double forward( const Entry * anEntry ) const { return phase[1].forward( anEntry ); }
    virtual Entry& setStartActivity( Activity * );
    virtual double processorCalls( const unsigned ) const = 0;
    double processorCalls() const; 
    virtual Call * processorCall( const unsigned p ) const = 0; //REP N-R
    bool phaseIsPresent( const unsigned p ) const { return phase[p].isPresent(); }
    Entry& saveOpenWait( const double aWait ) { nextOpenWait = aWait; return *this; }
    LQIO::DOM::Entry* getDOM() const { return myDOMEntry; }

    void resetReplication();
	
    void addDstCall( Call * aCall ) { myCallers << aCall; }
    void removeDstCall( Call *aCall ) { myCallers -= aCall; }
    unsigned callerListSize() const { return myCallers.size(); }
    const Cltn<Call *>& callerList() const { return myCallers; }
    const Cltn<Call *>& callList(unsigned p) const { return phase[p].callList(); }

    /* Queries */

    const char * name() const { return myDOMEntry->getName().c_str(); }
    virtual const Entity * owner() const = 0;
    virtual Entry& owner( const Entity * ) = 0;
	
    virtual bool isTaskEntry() const { return false; }
    virtual bool isVirtualEntry() const { return false; }
    virtual bool isProcessorEntry() const { return false; }
    bool isActivityEntry() const { return myType == ACTIVITY_ENTRY; }
    bool isStandardEntry() const { return myType == STANDARD_ENTRY; }
    bool isSignalEntry() const { return mySemaphoreType == SEMAPHORE_SIGNAL; }
    bool isWaitEntry() const { return mySemaphoreType == SEMAPHORE_WAIT; }
    bool isInterlocked( const Entry * ) const;
    bool isReferenceTaskEntry() const;
	
    bool hasDeterministicPhases() const;
    bool hasNonExponentialPhases() const;
    bool hasThinkTime() const;
    bool hasVariance() const;
    bool hasStartActivity() const { return myActivity != 0; }
    bool hasOpenArrivals() const { return myDOMEntry->hasOpenArrivalRate(); }

    bool entryTypeOk( const entry_type );
    bool entrySemaphoreTypeOk( const semaphore_entry_type aType );
    unsigned maxPhase() const { return myMaxPhase; }
    unsigned concurrentThreads() const;

    double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
    double waitExceptChain( const unsigned, const unsigned, const unsigned ) const; //REP N-R
    double elapsedTime( const unsigned p ) const { return phase[p].elapsedTime(); }	/* For server service times */
    double waitTime( const unsigned p, int submodel ) const { return phase[p].waitTime(submodel); }
    double getProcWait( const unsigned p, int submodel )  { return phase[p].getProcWait(submodel, 0) ;}	

    double elapsedTime() const { return total.elapsedTime(); }			/* Found through deltaWait  */

    double variance( const unsigned p ) const { return phase[p].variance(); }
    double variance() const { return total.variance(); }
	
    double utilization( const unsigned p ) const { return phase[p].utilization(); }
    double utilization() const;
    virtual double processorUtilization() const = 0;
    virtual double queueingTime( const unsigned ) const = 0;	// Time queued for processor.
    Probability prVisit() const;

    virtual double getStartTime() const { return 0.0; }
    virtual double getStartTimeVariance() const { return 0.0; }

    /* Computation */

    void add_call( const unsigned p, LQIO::DOM::Call* domCall );
    void sliceTime( const Entry& dst, Slice_Info phase_info[], double y_xj[] ) const;
    virtual void computeVariance() {}
    virtual Entry& updateWait( const Submodel&, const double ) = 0;
    virtual double updateWaitReplication( const Submodel&, unsigned& ) = 0;
    unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo& );
    void followForwarding( Phase *, const Entry *, const double, Stack<const Entity *>& ) const;
    bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>& ) const;
    Entry& aggregate( const unsigned, const unsigned p, const Exponential& );
    Entry& aggregateReplication( const Vector< VectorMath<double> >& );

    void callsPerform( callFunc, const unsigned, const unsigned k = 0 ) const;

    /* Dynamic Updates / Late Finalization */
    /* In order to integrate LQX's support for model changes we need to have a way  */
    /* of re-calculating what used to be static for all dynamically editable values */
	
    void recalculateDynamicValues();
    void sanityCheckParameters();
	

    /* Sanity checks */

    bool checkDroppedCalls() const;

    /* XML output */
 
    void insertDOMQueueingTime(void) const;
    void insertDOMResults(double *phaseUtils) const;

    /* Printing */

    ostream& printSubmodelWait( ostream& output, const unsigned offset ) const;
	
protected:
    Entry& setMaxPhase( const unsigned phase );

public:
    double openWait;				/* Computed open response time.	*/

protected:
    LQIO::DOM::Entry* myDOMEntry;	
    NullPhase total;
	
    Vector<GenericPhase> phase;
    double nextOpenWait;			/* copy for delta computation	*/

    /* Activity Entries */
	
    Activity * myActivity;			/* Starting activity.		*/
    unsigned int myMaxPhase;			/* Largest phase index.		*/

private:
    const unsigned _entryId;			/* Gobal entry id. (for chain)	*/
    const unsigned short _index;		/* My index (for mva)		*/
    entry_type myType;
    semaphore_entry_type mySemaphoreType;	/* Extra type information	*/
    requesting_type calledFlag;			/* true if entry referenced.	*/
    double myReplies;				/* For activities.		*/
    double myThroughput;			/* Computed throughput.		*/
    double myThroughputBound;			/* Type 1 throughput bound.	*/
	
    Cltn<Call *> myCallers;			/* Who calls me.		*/

    vector<InterlockInfo> _interlock;		/* Interlock table.		*/
};

/* --------------------------- Task Entries --------------------------- */


class TaskEntry : public Entry 
{
public:
    TaskEntry( LQIO::DOM::Entry* domEntry, const unsigned id, const unsigned int index ) : Entry(domEntry,id,index), myTask(0) {}

    virtual void initProcessor();
    virtual void initWait();

    virtual Entry& owner( const Entity * aTask );
    virtual const Entity * owner() const { return myTask; }

    virtual bool isTaskEntry() const { return true; }

    virtual double processorCalls( const unsigned ) const;
    virtual Call * processorCall( const unsigned p ) const { return phase[p].processorCall(); }

    virtual double processorUtilization() const;
    virtual double queueingTime( const unsigned ) const;		// Time queued for processor.
    virtual void computeVariance();
    virtual TaskEntry& updateWait( const Submodel&, const double );
    virtual double updateWaitReplication( const Submodel&, unsigned& );

private:
    const Entity * myTask;		/* My task.			*/
};

/* -------------------------- Device Entries -------------------------- */

class DeviceEntry : public Entry 
{
public:
    DeviceEntry( LQIO::DOM::Entry* domEntry, const unsigned, Processor * );
    virtual ~DeviceEntry();

    virtual void initProcessor();
    virtual void initWait();
    void initVariance();

    virtual Entry& owner( const Entity * aProcessor );
    virtual const Entity * owner() const { return myProcessor; }

    DeviceEntry& setServiceTime( const double );
    DeviceEntry& setPriority( const int );
    DeviceEntry& setCV_sqr( const double );

    virtual double processorCalls( const unsigned ) const;
    virtual Call * processorCall( const unsigned ) const { return 0; }

    virtual bool isProcessorEntry() const { return true; }

    virtual double processorUtilization() const;
    virtual double queueingTime( const unsigned ) const;		// Time queued for processor.
    virtual DeviceEntry& updateWait( const Submodel&, const double );
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

    virtual double processorCalls( const unsigned ) const  { return 0.0; }
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
inline double Call::serviceTime() const { return destination->serviceTime(1); }


/*
 * Compare to tasks by their name.  Used by the set class to insert items
 */

struct ltEntry
{
    bool operator()(const Entry * e1, const Entry * e2) const { return strcmp( e1->name(), e2->name() ) < 0; }
};


/*
 * Compare a entry name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqEntryStr 
{
    eqEntryStr( const string& s ) : _s(s) {}
    bool operator()(const Entry * e1 ) const { return _s == e1->name(); }

private:
    const string& _s;
};

extern set<Entry *, ltEntry> entry;

class SRVNEntryListManip {
public:
    SRVNEntryListManip( ostream& (*ff)(ostream&, const Cltn<Entry *> & ),
			const Cltn<Entry *> &theEntryList  )
	: f(ff), entryList(theEntryList) {}
private:
    ostream& (*f)( ostream&, const Cltn<Entry *> & );
    const Cltn<Entry *> & entryList;

    friend ostream& operator<<(ostream & os, const SRVNEntryListManip& m )
	{ return m.f(os,m.entryList); }
};

SRVNEntryListManip print_entries( const Cltn<Entry *> & entryList );
#endif
