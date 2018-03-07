/* -*- c++ -*-
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January 2003
 *
 * ------------------------------------------------------------------------
 * $Id: entry.h 13200 2018-03-05 22:48:55Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(ENTRY_H)
#define ENTRY_H

#include "lqn2ps.h"
#include <cstring>
#include "element.h"
#include "vector.h"
#include "phase.h"
#include "call.h"
#include <lqio/dom_entry.h>

class Arc;
class Activity;
class Entry;
class Label;
class Processor;
class Task;
template <class type> class Cltn;
template <class type> class Stack;
class SRVNEntryManip;

extern "C" {
    typedef void (* err_func_t)( unsigned err, ... );
}

/* */

/*
 * Printing functions.
 */

ostream& operator<<( ostream&, const Entry& );

/* -------------------- Nodes in the graph are... --------------------- */

class Entry : public Element {
    friend class Call;
    friend class Task;
    friend ostream& histogram_of_str( ostream& output, const Entry& anEntry );
    typedef SRVNEntryManip (* print_func_ptr)( const Entry& );
	
public:
    static Entry * create(LQIO::DOM::Entry* domEntry );

    static unsigned totalOpenArrivals;
    static unsigned max_phases;		/* maximum phase encounterd.	*/
    static const char * phaseTypeFlagStr [];

private:
    Entry& operator=( const Entry& );
	
public:
    /* Instance creation */

    Entry( const LQIO::DOM::DocumentObject * );
    Entry( const Entry& );
    virtual ~Entry();

    int operator==( const Entry& anEntry ) const;

    static void reset();
    Cltn<Entry *> * link( Cltn<Entry *> * aCltn = 0 );
    void addCall( const unsigned int p, LQIO::DOM::Call* domCall );
	
    void check() const;
    unsigned findChildren( CallStack&, const unsigned ) const;
    Entry& aggregate();
    const Entry& aggregateEntries( const unsigned ) const;
    unsigned setChain( unsigned curr_k, unsigned next_k, const Entity * aServer, callFunc aFunc ) const;
    unsigned referenceTasks( Cltn<const Entity *>&, Element * dst ) const;
    unsigned clients( Cltn<const Entity *>&, const callFunc = 0 ) const;
    virtual Entry& setClientClosedChain( unsigned );
    virtual Entry& setClientOpenChain( unsigned );
    virtual Entry& setServerChain( unsigned );

    /* Instance Variable access */

    const Task * owner() const { return myOwner; }
    Entry& owner( const Task * owner ) { myOwner = owner; return *this; }

    const LQIO::DOM::ExternalVariable & openArrivalRate() const;
    phase_type phaseTypeFlag( const unsigned p ) const;
    const LQIO::DOM::ExternalVariable & Cv_sqr( const unsigned p ) const;
    double Cv_sqr() const;
    bool hasPriority() const;    
    const LQIO::DOM::ExternalVariable& priority() const;

    bool hasServiceTime( const unsigned int p ) const;
    const LQIO::DOM::ExternalVariable& serviceTime( const unsigned p ) const;
    double serviceTime() const;

    bool hasThinkTime( const unsigned int p ) const;
    const LQIO::DOM::ExternalVariable& thinkTime( const unsigned p ) const;

    Entry& histogram( const unsigned p, const double min, const double max, const unsigned n_bins );
    Entry& histogramBin( const unsigned p, const double begin, const double end, const double prob, const double conf95, const double conf99 );
    double maxServiceTime( const unsigned p ) const;

    const LQIO::DOM::ExternalVariable & rendezvous( const Entry * anEntry, const unsigned p ) const;
    double rendezvous( const Entry * ) const;
    Entry& rendezvous( const Entry * anEntry, const unsigned p, const LQIO::DOM::Call * );
    const LQIO::DOM::ExternalVariable & sendNoReply( const Entry * anEntry, const unsigned p ) const;
    double sendNoReply( const Entry * ) const;
    Entry& sendNoReply( const Entry * anEntry, const unsigned p, const LQIO::DOM::Call * );
    const LQIO::DOM::ExternalVariable &  forward( const Entry * anEntry ) const;
    Entry& forward( const Entry *  anEntry, const LQIO::DOM::Call * );
    bool forwardsTo( const Task * aTask ) const;
    Call * forwardingRendezvous( Entry *, const unsigned, const double );
    unsigned fanIn( const Entry * toEntry ) const;
    unsigned fanOut( const Entry * toEntry ) const;
    Entry& setStartActivity( Activity * );
    Activity * startActivity() const { return myActivity; }
    bool phaseIsPresent( const unsigned p ) const { return isPresent[p]; }
    Entry& phaseIsPresent( const unsigned phase, const bool yesOrNo );

    /* Result queries */

    double executionTime( const unsigned p ) const;
    double executionTime() const;
    double openWait() const;
    double processorUtilization() const;
    double queueingTime( const unsigned p ) const;
    double serviceExceeded( const unsigned p ) const;
    double serviceExceeded() const;
    double throughput() const;
    double throughputBound() const;
    double utilization( const unsigned p ) const;
    double utilization() const;
    double variance( const unsigned p ) const;
    double variance() const;

    double numberSlices( const unsigned p ) const;
    double sliceTime( const unsigned p ) const;

    /* Queries */

    bool isCalled( const requesting_type callType );
    requesting_type isCalled() const { return calledFlag; }
    bool isReferenceTaskEntry() const;
    bool isSelectedIndirectly() const;

    bool hasMaxServiceTime() const;
    bool hasHistogram() const;
    bool hasRendezvous() const;
    bool hasSendNoReply() const;
    bool hasForwarding() const;
    bool hasOpenArrivalRate() const;
    bool hasForwardingLevel() const;
    bool isForwardingTarget() const;
    bool hasCalls( const callFunc aFunc ) const;
    bool hasThinkTime() const;
    bool hasDeterministicPhases() const;
    bool hasNonExponentialPhases() const;
    bool hasQueueingTime() const;

    Entry& setPhaseDOM( unsigned phase, const LQIO::DOM::Phase* phaseInfo );
    const LQIO::DOM::Phase * getPhaseDOM( unsigned phase ) const;
    void addDstCall( GenericCall * aCall ) { myCallers << aCall; }
    void removeDstCall( GenericCall * aCall) { myCallers -= aCall; }
    unsigned callerListSize() const { return myCallers.size(); }
    const Cltn<GenericCall *>& callerList() const { return myCallers; }
    const Cltn<Call *>& callList() const { return myCalls; }
    void addActivityReplyArc( Reply * aReply ) { myActivityCallers << aReply; }
    void deleteActivityReplyArc( Reply * aReply ) { myActivityCallers -= aReply; }

    bool isActivityEntry() const;
    bool isStandardEntry() const;
    bool isSignalEntry() const;
    bool isWaitEntry() const;

    bool is_r_lock_Entry() const;
    bool is_r_unlock_Entry() const;
    bool is_w_lock_Entry() const;
    bool is_w_unlock_Entry() const;

    bool entryTypeOk( const LQIO::DOM::Entry::EntryType );
    bool entrySemaphoreTypeOk( const semaphore_entry_type );
    bool entryRWLockTypeOk( const rwlock_entry_type );
    unsigned maxPhase() const { return myMaxPhase; }
    unsigned numberOfPhases() const;

    unsigned countArcs( const callFunc = 0 ) const;
    unsigned countCallers( const callFunc = 0 ) const;

    double serviceTimeForSRVNInput() const;
    double serviceTimeForSRVNInput( const unsigned p ) const;
    double serviceTimeForQueueingNetwork( const unsigned p ) const;
    double serviceTimeForQueueingNetwork() const;
    Entry& aggregateService( const Activity * anActivity, const unsigned p, const double rate );
    Entry& aggregatePhases();

    const Entry& addThptUtil( double& tput_sum, double &util_sum ) const;

    static Entry * find( const string& );
    static int compare( const void *, const void * );
    virtual double getIndex() const;
    virtual int span() const;

    Graphic::colour_type colour() const;

    /* movement */

    virtual Entry& moveTo( const double x, const double y );
    virtual Entry& label();
    const Entry& labelQueueingNetworkVisits( Label& ) const;
    const Entry& labelQueueingNetworkService( Label& ) const;
    const Entry& labelQueueingNetworkWaiting( Label& ) const;
    virtual Entry& scaleBy( const double, const double );
    virtual Entry& translateY( const double );
    virtual Entry& depth( const unsigned );

#if defined(REP2FLAT)
    static Entry * find_replica( const string&, const unsigned );

    Entry * expandEntry( int replica ) const;
    const Entry& expandCalls() const;
#endif

    /* Printing */
    
    virtual ostream& draw( ostream& ) const;
//    virtual ostream& print( ostream& ) const;

private:
    Call * findCall( const Entry * anEntry, const callFunc = 0 ) const;
    Call * findCall( const Task * aTask ) const;
    Call * findOrAddCall( const Entry * anEntry, const callFunc = 0 );
    ProxyEntryCall * findOrAddFwdCall( const Entry * anEntry );
    Call * findOrAddPseudoCall( const Entry * anEntry );		// For -Lclient

    void addSrcCall( Call * aCall ) { myCalls << aCall; }
    Entry& moveSrc();
    Entry& moveDst();

    ostream& printSRVNLine( ostream& output, char code, print_func_ptr func ) const;

public:
    bool drawLeft;
    bool drawRight;

protected:
    Vector2<Phase> phase;

private:
    const Task * myOwner;
    unsigned myIndex;
    unsigned short myMaxPhase;		/* Largest phase index.		*/
    requesting_type calledFlag;		/* true if entry referenced.	*/
    Cltn<Call *> myCalls;		/* Who I call.			*/
    Cltn<GenericCall *> myCallers;	/* Who calls me.		*/
    Vector<bool> isPresent;		/* True if phase is used.	*/
    Activity * myActivity;		/* If I have activities.	*/
    Arc *myActivityCall;		/* Arc to who I call		*/
    Cltn<Reply *> myActivityCallers;	/* Arcs from who reply to me.	*/
};

/* ------------------ Proxy messages for class call. ------------------ */

/*
 * Forward request to associated entry.  Defined here rather than in
 * class body due to foward reference problems.  Inlined.
 */

inline const Entry * Call::dstEntry() const { return destination; }
inline const Task * Call::dstTask() const { return destination->owner(); }

inline double Call::variance() const { return destination->variance(1); }
inline phase_type Call::phaseTypeFlag() const { return destination->phaseTypeFlag(1); }
inline const LQIO::DOM::ExternalVariable & Call::serviceTime() const { return destination->serviceTime(1); }
inline double Call::executionTime() const { return destination->executionTime(1); }

/* --------- Access functions called by parser to store data. --------- */

void setEntryOwner( const Cltn<Entry *> &, const Task * );

/* --------- Access functions called by parser to store data. --------- */

/*
 * Compare to tasks by their name.  Used by the set class to insert items
 */

struct ltEntry
{
    bool operator()(const Entry * e1, const Entry * e2) const { return e1->name() < e2->name(); }
};


/*
 * Compare a entry name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqEntryStr 
{
    eqEntryStr( const string & s ) : _s(s) {}
    bool operator()(const Entry * e1 ) const { return e1->name() == _s; }

private:
    const string & _s;
};

extern set<Entry *,ltEntry> entry;
extern Entry * add_entry( LQIO::DOM::Entry* entry );

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNEntryManip {
public:
    SRVNEntryManip( ostream& (*ff)(ostream&, const Entry & ), const Entry & theEntry )
	: f(ff), anEntry(theEntry) {}
private:
    ostream& (*f)( ostream&, const Entry& );
    const Entry & anEntry;

    friend ostream& operator<<(ostream & os, const SRVNEntryManip& m ) 
	{ return m.f(os,m.anEntry); }
};

SRVNEntryManip compute_service_time( const Entry & anEntry );
SRVNEntryManip print_number_slices( const Entry& anEntry );
SRVNEntryManip print_queueing_time( const Entry& anEntry );
SRVNEntryManip print_service_time( const Entry& anEntry );
SRVNEntryManip print_slice_time( const Entry& anEntry );
SRVNEntryManip print_think_time( const Entry& anEntry );
SRVNEntryManip print_variance( const Entry& anEntry );

bool map_entry_names( const char * from_entry_name, Entry * & fromEntry, const char * to_entry_name, Entry * & toEntry,  err_func_t err_func );
#endif
