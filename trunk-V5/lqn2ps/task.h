/* -*- c++ -*-
 *
 * Tasks.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * April 2010.
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#if	!defined(TASK_H)
#define TASK_H

#include "lqn2ps.h"
#include <cstring>
#include "entity.h"
#include "cltn.h"
#include "actlayer.h"
#include "point.h"

class Task;
class Processor;
class Share;
class Call;
class TaskCall;
class EntityCall;
class OpenArrival;
class ProcessorCall;
class ActivityList;
template <class type> class Stack;
template <class type> class Sequence;

ostream& operator<<( ostream&, const Task& );

/* ----------------------- Abstract Superclass ------------------------ */

class Task : public Entity {
    typedef double (Task::*taskPhaseFunc)( const unsigned ) const;

public:
    static bool thinkTimePresent;
    static bool holdingTimePresent;
    static bool holdingVariancePresent;
    static void reset();
    static Task* create( const LQIO::DOM::Task* domTask, Cltn<Entry *>& entries );

protected:
    Task( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries );

public:
    virtual ~Task();
    virtual Task * clone( unsigned int, const string& aName, const Processor * aProcessor, const Share * aShare ) const = 0;

    virtual void rename();
    virtual void squishName();
    virtual Task& aggregate();

    virtual Entity& processor( const Processor * aProcessor ) { myProcessor = aProcessor; return *this; }
    virtual const Processor * processor() const { return myProcessor; }
    const Share * share() const { return myShare; }
    virtual int priority() const;

    virtual int rootLevel() const;
    virtual Task const& sort() const;
    virtual double getIndex() const;
    virtual int span() const;

    const Cltn<Entry *>& entries() const { return entryList; }
    Task& addEntry( Entry * );
    Task& removeEntry( Entry * );
    Entry * entryAt( const unsigned index ) const { return entryList[index]; }
    Activity * findActivity( const string& name ) const;
    Activity * findOrAddActivity( const LQIO::DOM::Activity * );
#if defined(REP2FLAT)
    Activity * addActivity( const Activity&, const unsigned );
    Activity * findActivity( const Activity&, const unsigned );
#endif
    Task& removeActivity( Activity * );
    void addPrecedence( ActivityList * );
    const Cltn<Activity *>& activities() const { return activityList; }
    const Cltn<ActivityList *>& precedences() const { return precedenceList; }

    unsigned nEntries() const { return entryList.size(); }
    unsigned nActivities() const { return activityList.size(); }

    virtual unsigned setChain( unsigned, callFunc aFunc ) const;
    virtual Task& setServerChain( unsigned );

    virtual bool forwardsTo( const Task * aTask ) const;
    virtual bool hasForwardingLevel() const;
    virtual bool isForwardingTarget() const;
    virtual bool isCalled( const requesting_type ) const;
    virtual bool hasThinkTime() const { return false; }

    virtual int queueLength() const { return 0; }

    double openArrivalRate() const;

    virtual double holdingTime( const unsigned p ) const { return 0.0; }
    virtual double holdingVariance( const unsigned p ) const { return 0.0; }
    double throughput() const;
    double utilization( const unsigned p ) const;
    virtual double utilization() const;
    double processorUtilization() const;

    bool hasActivities() const { return activities().size() != 0; }
    virtual bool hasCalls( const callFunc ) const;
    bool hasOpenArrivals() const;
    bool hasQueueingTime() const;
    virtual bool hasHoldingTime() const { return false; }
    virtual bool hasHoldingVariance() const { return false; }

    virtual bool isTask() const { return true; }
    virtual bool isPureServer() const;
    virtual bool isSelectedIndirectly() const;

    virtual bool canConvertToReferenceTask() const;
    virtual bool canConvertToOpenArrivals() const;

    virtual void check() const;
    virtual unsigned referenceTasks( Cltn<const Entity *>&, Element * dst ) const;
    virtual unsigned clients( Cltn<const Entity *>&, const callFunc = 0 ) const;
    virtual unsigned servers( Cltn<const Entity *>& ) const;
    unsigned maxPhase() const { return myMaxPhase; }			/* Max phase over all entries	*/

    virtual unsigned findChildren( CallStack&, const unsigned );
    unsigned countThreads() const;
    Cltn<Activity *> repliesTo( Entry * anEntry ) const;
    const Cltn<EntityCall *>& callList() const { return myCalls; }
    EntityCall * findOrAddCall( const Task *, const callFunc aFunc );
    EntityCall * findOrAddFwdCall( const Task * );
    EntityCall * findOrAddPseudoCall( const Entity * );		// For -Lclient

    virtual bool isInOpenModel( const Cltn<Entity *>& servers ) const;
    virtual bool isInClosedModel( const Cltn<Entity *>& servers  ) const;

    virtual double serviceTimeForQueueingNetwork( const unsigned k, chainTestFunc ) const;
    double sliceTimeForQueueingNetwork( const unsigned k, chainTestFunc ) const;

    /* Activities */
    
    unsigned generate();
    Task& format();
    virtual Task& reformat();
    double justify();
    double justifyByEntry();
    double alignActivities();

    /* movement */

    virtual Task& moveBy( const double dx, const double dy );
    virtual Task& moveTo( const double x, const double y );
    virtual Task& scaleBy( const double, const double );
    virtual Task& translateY( const double );
    virtual Task& depth( const unsigned );

    virtual Entity& label();

#if defined(REP2FLAT)
    virtual Task& removeReplication();
    Task * expandTask( int ext ) const;
#endif

    /* Printing */
    
    virtual ostream& draw( ostream& output ) const;
//    virtual ostream& print( ostream& output ) const;
#if defined(PMIF_OUTPUT)
    virtual ostream& printPMIFClient( ostream& output ) const;
    virtual ostream& printPMIFArcs( ostream& output ) const;
#endif
    ostream& printEntries( ostream& ) const;
    ostream& printActivities( ostream& ) const;

    virtual ostream& drawClient( ostream&, const bool is_in_open_model, const bool is_in_closed_model ) const;
#if defined(QNAP_OUTPUT)
    virtual ostream& printQNAPClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model, const bool multi_class ) const;
#endif

private:
    unsigned topologicalSort();
    const Task& labelQueueingNetwork( entryLabelFunc aFunc ) const;
    unsigned countArcs( const callFunc = 0 ) const;
    double countCalls( const callFunc2 ) const;

    EntityCall * findCall( const Entity *, const callFunc aFunc = 0 ) const;
    Task& moveDst();
    Task& moveSrc();
    Task& moveSrcBy( const double dx, const double dy );

#if defined(REP2FLAT)
    Task& expandActivities( const Task& src, int replica );

protected:
    LQIO::DOM::Task * cloneDOM( const string& aName, LQIO::DOM::Processor * dom_processor ) const;
    const Cltn<Entry *>& groupEntries( int replica, Cltn<Entry *>& newEntryList  ) const;

public:
#endif

private:
    Task& aggregateEntries();
#if defined(QNAP_OUTPUT)
    ostream& printQNAPRequests( ostream& output, const bool is_in_open_model, const bool is_in_closed_model, const bool multi_class ) const;
    ostream& printQNAPRequests( ostream& output, Sequence<EntityCall *> &nextCall, const bool multi_class, QNAP_Element_func chain_func, callFunc2 call_func ) const;
#endif
#if defined(PMIF_OUTPUT)
    ostream& printPMIFRequests( ostream& output ) const;
#endif

protected:
    Cltn<Entry *> entryList;			/* Entries for this entity.	*/
    Cltn<Activity *> activityList;		/* Activities for this entity.	*/
    Cltn<ActivityList *> precedenceList;	/* Precendences for this entity	*/

private:
    Task( const Task& );
    Task& operator=( const Task& );

private:
    static Processor * defaultProcessor;	/* Used when we need a processor */
    const Processor * myProcessor;		/* proc. allocated to task.  	*/
    const Share * myShare;			/* share for this task.		*/
    mutable unsigned short myMaxPhase;		/* Max phase over all entries	*/
    Cltn<EntityCall *> myCalls;			/* Arc calling processor	*/

    Vector2<ActivityLayer> layers;	
    unsigned maxLevel;
    double entryWidthPts;
};


/* ------------------------- Reference Tasks -------------------------- */

class ReferenceTask : public Task {
public:
    ReferenceTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& aCltn );
    virtual ReferenceTask * clone( unsigned int, const string& aName, const Processor * aProcessor, const Share * aShare ) const;

    virtual double getIndex() const { return index(); }

    virtual bool isReferenceTask() const { return true; }
    virtual bool isPureServer() const { return false; }
    virtual bool hasThinkTime() const;
    LQIO::DOM::ExternalVariable& thinkTime() const;

    virtual bool canConvertToOpenArrivals() const { return false; }

    virtual int rootLevel() const { return 1; }

    virtual Graphic::colour_type colour() const;
    virtual unsigned findChildren( CallStack&, const unsigned );
};


/* --------------------------- Server Tasks --------------------------- */

class ServerTask : public Task {
public:
    ServerTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& aCltn );
    virtual ServerTask * clone( unsigned int, const string& aName, const Processor * aProcessor, const Share * aShare ) const;

    virtual bool isServerTask() const   { return true; }
    virtual bool canConvertToReferenceTask() const;
};

/* ------------------------ Lock Server Tasks ------------------------- */

class SemaphoreTask : public Task {
public:
    SemaphoreTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries );
    virtual SemaphoreTask * clone( unsigned int, const string& aName, const Processor * aProcessor, const Share * aShare ) const;

    virtual bool isServerTask() const   { return true; }

private:
};

class RWLockTask : public Task {
public:
    RWLockTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries );
    virtual RWLockTask * clone( unsigned int, const string& aName, const Processor * aProcessor, const Share * aShare ) const;

    virtual bool isServerTask() const   { return true; }

private:
};

/*
 * Compare a task name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqTaskStr 
{
    eqTaskStr( const string& s ) : _s(s) {}
    bool operator()(const Task * p1 ) const { return p1->name() == _s; }

private:
    const string & _s;
};

extern set<Task *,ltTask> task;

#endif
