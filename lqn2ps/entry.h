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
 * $Id: entry.h 14226 2020-12-16 14:00:48Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(ENTRY_H)
#define ENTRY_H

#include "lqn2ps.h"
#include <cstring>
#include <vector>
#include <set>
#include <numeric>
#include <lqio/dom_entry.h>
#include "demand.h"
#include "element.h"
#include "phase.h"
#include "call.h"

class Arc;
class Activity;
class Entry;
class Label;
class Processor;
class Task;
class SRVNEntryManip;

extern "C" {
    typedef void (* err_func_t)( unsigned err, ... );
}

/* -------------------- Nodes in the graph are... --------------------- */

class Entry : public Element {
    friend class Call;
    friend class Task;
    friend std::ostream& histogram_of_str( std::ostream& output, const Entry& anEntry );
    typedef SRVNEntryManip (* print_func_ptr)( const Entry& );

public:
    struct count_callers {
	count_callers( const callPredicate predicate ) : _predicate(predicate) {}
	unsigned int operator()( unsigned int, const Entry * entry ) const;
	
    private:
	const callPredicate _predicate;
    };
	
public:
    static Entry * create(LQIO::DOM::Entry* domEntry );

    static unsigned totalOpenArrivals;
    static unsigned max_phases;		/* maximum phase encounterd.	*/
    static const char * phaseTypeFlagStr [];
    static unsigned int max_phase( unsigned int a, const std::pair<unsigned,Phase>& p ) { return std::max( a, p.first ); }

private:
    Entry& operator=( const Entry& );
	
public:
    /* Instance creation */

    Entry( const LQIO::DOM::DocumentObject * );
    Entry( const Entry& );
    virtual ~Entry();
    Entry * clone( unsigned int ) const;
    
    int operator==( const Entry& anEntry ) const;

    static void reset();

    const Phase& getPhase( unsigned p ) const;
    Phase& getPhase( unsigned p );

    void addCall( const unsigned int p, LQIO::DOM::Call* domCall );
	
    bool check() const;
    size_t findChildren( CallStack&, const unsigned ) const;
    Entry& aggregate();
    const Entry& aggregateEntries( const unsigned ) const;
    unsigned setChain( unsigned curr_k, unsigned next_k, const Entity * aServer, callPredicate aFunc );
    unsigned referenceTasks( std::vector<Entity *>&, Element * dst ) const;
    unsigned clients( std::vector<Entity *>&, const callPredicate = 0 ) const;
    virtual Entry& setClientClosedChain( unsigned );
    virtual Entry& setClientOpenChain( unsigned );
    virtual Entry& setServerChain( unsigned );

    /* Instance Variable access */

    const Task * owner() const { return _owner; }
    Entry& owner( const Task * owner ) { _owner = owner; return *this; }

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
    Activity * startActivity() const { return _startActivity; }
    bool phaseIsPresent( const unsigned p ) const { return _phases.find(p) != _phases.end(); }

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
    bool isCalled( const requesting_type callType ) const { return _isCalled == callType; }
    requesting_type isCalled() const { return _isCalled; }
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
    bool hasCalls( const callPredicate aFunc ) const;
    bool hasThinkTime() const;
    bool hasDeterministicPhases() const;
    bool hasNonExponentialPhases() const;
    bool hasQueueingTime() const;

    Entry& setPhaseDOM( unsigned phase, const LQIO::DOM::Phase* phaseInfo );
    const LQIO::DOM::Phase * getPhaseDOM( unsigned phase ) const;

    void addSrcCall( Call * aCall ) { _calls.push_back(aCall); }
    void removeSrcCall( Call * aCall );
    void addDstCall( GenericCall * aCall ) { _callers.push_back( aCall ); }
    void removeDstCall( GenericCall * aCall );
    const std::vector<GenericCall *>& callers() const { return _callers; }
    const std::vector<Call *>& calls() const { return _calls; }
    void addActivityReplyArc( Reply * aReply ) { _activityCallers.push_back(aReply); }
    void deleteActivityReplyArc( Reply * aReply );

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
    unsigned maxPhase() const { return std::accumulate( _phases.begin(), _phases.end(), 1, &max_phase ); }

    unsigned countArcs( const callPredicate = nullptr ) const;
    unsigned countCallers( const callPredicate = nullptr ) const;

    double serviceTimeForSRVNInput() const;
    double serviceTimeForSRVNInput( const unsigned p ) const;
    Entry& aggregateService( const Activity * anActivity, const unsigned p, const double rate );
    Entry& aggregatePhases();
    static Demand accumulate_demand( const Demand&, const Entry * );

    static Entry * find( const std::string& );
    static bool compare( const Entry *, const Entry * );
    virtual double getIndex() const;
    virtual int span() const;

    Graphic::colour_type colour() const;

    /* movement */

    virtual Entry& moveTo( const double x, const double y );
    virtual Entry& label();
    Entry& labelQueueingNetworkVisits( Label& );
    Entry& labelQueueingNetworkService( Label& );
    Entry& labelQueueingNetworkWaiting( Label& );
    virtual Entry& scaleBy( const double, const double );
    virtual Entry& translateY( const double );
    virtual Entry& depth( const unsigned );

    virtual Entry& rename();

#if defined(BUG_270)
    Entry& linkToClients( const std::vector<EntityCall *>& );
    Entry& unlinkFromServers();
#endif
#if defined(REP2FLAT)
    static Entry * find_replica( const std::string&, const unsigned );

    Entry& expandEntry();
    Entry& expandCall();
    Entry& replicateCall();
    Entry& replicateEntry( LQIO::DOM::DocumentObject ** );
#endif

    /* Printing */
    
    virtual const Entry& draw( std::ostream& ) const;

private:
    Call * findCall( const Entry * anEntry, const callPredicate = 0 ) const;
    Call * findCall( const Task * aTask ) const;
    Call * findOrAddCall( const Entry * anEntry, const callPredicate = 0 );
    ProxyEntryCall * findOrAddFwdCall( const Entry * anEntry );
    Call * findOrAddPseudoCall( const Entry * anEntry );		// For -Lclient

    Entry& moveSrc();
    Entry& moveDst();

#if defined(BUG_270)
    static void remove_from_dst( Call * call );
#endif

    std::ostream& printSRVNLine( std::ostream& output, char code, print_func_ptr func ) const;

public:
    static std::set<Entry *,LT<Entry> > __entries;

    bool drawLeft;
    bool drawRight;

protected:
    std::map<unsigned,Phase> _phases;

private:
    const Task * _owner;
    requesting_type _isCalled;			/* true if entry referenced.	*/
    std::vector<Call *> _calls;			/* Who I call.			*/
    std::vector<GenericCall *> _callers;	/* Who calls me.		*/
    Activity * _startActivity;			/* If I have activities.	*/
    Arc *_activityCall;				/* Arc to who I call		*/
    std::vector<Reply *> _activityCallers;	/* Arcs from who reply to me.	*/
};

/*
 * Printing functions.
 */

inline std::ostream& operator<<( std::ostream& output, const Entry& self ) { self.draw( output ); return output; }

/* ------------------ Proxy messages for class call. ------------------ */

/*
 * Forward request to associated entry.  Defined here rather than in
 * class body due to foward reference problems.  Inlined.
 */

inline const Entry * Call::dstEntry() const { return _destination; }
inline const Task * Call::dstTask() const { return _destination->owner(); }

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNEntryManip {
public:
    SRVNEntryManip( std::ostream& (*ff)(std::ostream&, const Entry & ), const Entry & theEntry )
	: f(ff), anEntry(theEntry) {}
private:
    std::ostream& (*f)( std::ostream&, const Entry& );
    const Entry & anEntry;

    friend std::ostream& operator<<(std::ostream & os, const SRVNEntryManip& m ) 
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
