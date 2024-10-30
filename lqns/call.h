/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/call.h $
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * March, 2004
 *
 * $Id: call.h 17209 2024-05-13 18:16:37Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef	LQNS_CALL_H
#define LQNS_CALL_H

#include <cmath>
#include <deque>
#include <lqio/dom_call.h>
#include "interlock.h"

class Activity;
class Call;
class Entry;
class Entity;
class EntryPath;
class Path;
class PathNode;
class Phase;
class Server;
class Submodel;
class Task;
struct InterlockInfo;

typedef bool (Call::*queryFunc)() const;

/* ------------------- Arcs between entries are... -------------------- */

class Call {

public:
    class Create {
    public:
	Create( Entry * src, unsigned p ) : _src(src), _p(p) {}

	void operator()( const LQIO::DOM::Call * );

    private:
	Entry * _src;
	const unsigned _p;
    };
    
    
    struct Find {
	Find( const Call * call, const bool direct_path ) : _call(call), _direct_path(direct_path), _broken(false) {}

	bool operator()( const Call * ) const;
	
    private:
	const Call * _call;
	const bool _direct_path;
	mutable bool _broken;
    };
	
    class Perform {
    public:
	typedef void (Perform::*F)( Call& );

	Perform( F f, const Submodel& submodel, unsigned int k=0 ) : _f(f), _submodel(submodel), _e(nullptr), _k(k), _p(0), _rate(0.0) {}
	Perform( F f, const Submodel& submodel, const Entry * e, unsigned k, unsigned p, double rate ) : _f(f), _submodel(submodel), _e(e), _k(k), _p(p), _rate(rate) {}
	Perform( const Perform& src, unsigned p, double rate ) : _f(src._f), _submodel(src._submodel), _e(src._e), _k(src._k), _p(p), _rate(rate) {}		// Set rate and phase.
	Perform( const Perform& src, double scale ) : _f(src._f), _submodel(src._submodel), _e(src._e), _k(src._k), _p(src._p), _rate(src._rate * scale) {}	// Set rate.

	unsigned int submodel() const; // { return _submodel.number(); }
	Perform& setEntry( const Entry * entry ) { _e = entry; return *this; }
	const Entry * entry() const { return _e; }
	Perform& setChain( unsigned int k ) { _k = k; return *this; }
	Perform& setPhase( unsigned int p ) { _p = p; return *this; }
	Perform& setRate( double rate ) { _rate = rate; return *this; }
	double rate() const { return _rate; }

	bool hasServer( const Entity * ) const;
	void operator()( Call * call );

	/* Functions invoked from task */
	
	void setChain( Call& );
	void setVisits( Call& );
	void setLambda( Call& );
	void initWait( Call& );
	void saveWait( Call& );
	void saveOpen( Call& );

    private:
	const F f() const { return _f; };
	unsigned int k() const { return _k; }
	unsigned int p() const { return _p; }

    private:
	const F _f;
	const Submodel& _submodel;
	const Entry * _e;
	unsigned int _k;
	unsigned int _p;
	double _rate;
    };
    
    class stack : public std::deque<const Call *>
    {
    public:
	stack() : std::deque<const Call *>() {}
	unsigned depth() const;

	static std::string fold( const std::string& s, const Call * call ) { return call->getDOM() != nullptr ? s + "," + call->srcName() : s; }
    };

    class call_cycle
    {
    public:
	call_cycle() {}
	virtual ~call_cycle() throw() {}
    };

    static double add_rendezvous( double sum, const Call * call );
    static double add_rendezvous_no_fwd( double sum, const Call * call );
    static double add_forwarding( double sum, const Call * call ) { return sum + call->forward(); }		/* BUG_304 BUG_299 */
    static std::set<Task *>& add_client( std::set<Task *>&, const Call * );
    static std::set<Entity *>& add_server( std::set<Entity *>&, const Call * );

    struct find_call {
	find_call( const Entry * e, const queryFunc f ) : _e(e), _f(f) {}
	bool operator()( const Call * call ) const { return call->dstEntry() == _e && !call->isForwardedCall() && (!_f || (call->*_f)()); }
    private:
	const Entry * _e;
	const queryFunc _f;
    };
    
    struct find_fwd_call {
	find_fwd_call( const Entry * e ) : _e(e) {}
	bool operator()( const Call * call ) const { return call->dstEntry() == _e && call->isForwardedCall(); }
    private:
	const Entry * _e;
    };
    
    struct add_replicated_rendezvous {
	add_replicated_rendezvous( unsigned int submodel ) : _submodel(submodel) {}
	double operator()( double sum, const Call * call ) { return call->submodel() == _submodel ? sum + call->rendezvous() * call->fanOut() : sum; }
    private:
	const unsigned int _submodel;
    };

    struct add_rendezvous_to {
	add_rendezvous_to( const Entity * task ) : _task(task) {}
	double operator()( double sum, const Call * call ) const { return call->dstTask() == _task ? sum + call->rendezvous() * call->fanOut() : sum; }
    private:
	const Entity * _task;
    };

    struct add_submodel_rendezvous {
	add_submodel_rendezvous( unsigned int submodel ) : _submodel(submodel) {}
	double operator()( double sum, const Call * call ) const { return call->submodel() == _submodel ? sum + call->rendezvous() : sum; }
    private:
	const unsigned int _submodel;
    };

    struct add_wait {
	add_wait( unsigned int submodel ) : _submodel(submodel) {}
	double operator()( double sum, const Call * call ) const { return call->submodel() == _submodel ? sum + call->rendezvousDelay() : sum; }
    private:
	const unsigned int _submodel;
    };

public:
    Call( const Phase * fromPhase, const Entry * toEntry );

protected:
    Call( const Call& );
    virtual Call * clone( unsigned int src_replica, unsigned int dst_replica ) = 0;

public:
    virtual ~Call();

private:
    Call& operator=( const Call& ) = delete;

public:
    int operator==( const Call& item ) const;
    int operator!=( const Call& item ) const;

    Call& initCustomers( std::deque<const Task *>& stack, unsigned int customers );
    
    virtual bool check() const;

    /* Instance variable access */

    const LQIO::DOM::Call* getDOM() const { return _dom; }
    const Phase * getSource() const { return _source; }
    const Entry * dstEntry() const { return _destination; }
    
    virtual double rendezvous() const { return hasRendezvous() ? getDOMValue() : 0.0; }
    virtual Call& rendezvous( const LQIO::DOM::Call* dom ) { _dom = dom; return *this; }
    double sendNoReply() const { return hasSendNoReply() ? getDOMValue() : 0.0; }
    Call& sendNoReply( const LQIO::DOM::Call* dom ) { _dom = dom; return *this; }
    double forward() const { return hasForwarding() ? getDOMValue() : 0.0; }
    Call& forward( const LQIO::DOM::Call* dom ) { _dom = dom; return *this; }

    unsigned fanIn() const;
    unsigned fanOut() const;

    double wait() const { return _wait; }
    void setWait( double wait ) { if ( std::isnan( wait ) ) abort(); _wait = wait; }

protected:
    Call& setSource( const Phase * source ) { _source = source; return *this; }
    Call& setDestination( const Entry * destination ) { _destination = destination; return *this; }

public:
    /* Queries */

    virtual bool isForwardedCall() const { return false; }
    virtual bool isProcessorCall() const { return false; }
    bool isNotProcessorCall() const { return !isProcessorCall(); }
    bool hasRendezvous() const { return getDOM() != nullptr  && getDOM()->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS; }
    bool hasSendNoReply() const { return getDOM() != nullptr && getDOM()->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY; }
    bool hasForwarding() const { return  getDOM() != nullptr && getDOM()->getCallType() == LQIO::DOM::Call::Type::FORWARD; }
    bool hasOvertaking() const;
    bool hasNoCall() const { return !hasRendezvous() && !hasSendNoReply() && !hasForwarding(); }
    bool hasRendezvousOrNone() const { return !hasSendNoReply() && !hasForwarding(); }
    bool hasSendNoReplyOrNone() const { return !hasRendezvous() && !hasForwarding(); }
    bool hasForwardingOrNone() const { return !hasRendezvous() && !hasSendNoReply(); }
    bool hasNoForwarding() const { return dstEntry() == nullptr || hasRendezvous() || hasSendNoReply(); }		/* Special case for topological sort */
    bool hasTypeForCallInfo( LQIO::DOM::Call::Type type ) const;
#if PAN_REPLICATION
    unsigned getChain() const { return _chainNumber; } //tomari
#endif
    
    virtual const std::string& srcName() const;
    const Task * srcTask() const;

    const std::string& dstName() const;
    unsigned submodel() const;

    double rendezvousDelay() const;
#if PAN_REPLICATION
    double rendezvousDelay( const unsigned k ) const;
#endif
    double residenceTime() const;
    double queueingTime() const;
#if PAN_REPLICATION
    double nrFactor( const Submodel& aSubmodel, const unsigned k ) const;
#endif
    double dropProbability() const;
    virtual const Call& insertDOMResults() const;

    /* Computation */

    Call& expand();
    double variance() const;
    double CV_sqr() const;
    const Call& followInterlock( Interlock::CollectTable& ) const;

    /* Proxies */

    const Entity * dstTask() const;
    short index() const;
    double serviceTime() const;

private:
    double getDOMValue() const;

private:
    const LQIO::DOM::Call* _dom;	/* Input */
    const Phase* _source;		/* Calling Phase/activity.	*/
    const Entry* _destination;		/* to whom I am referring to	*/
    unsigned _chainNumber;
    double _wait;			/* Waiting time.		*/
};

/* -------------------------------------------------------------------- */

class NullCall : public Call {
public:
    NullCall() : Call(nullptr,nullptr) {}
    virtual NullCall * clone( unsigned int, unsigned int ) { abort(); return nullptr; }

    virtual const std::string& srcName() const { static const std::string null("NULL"); return null; }
};

/* -------------------------------------------------------------------- */

class FromEntry : virtual protected Call {
public:
    FromEntry( const Entry * entry ) : _entry(entry) {}
    const Entry * srcEntry() const { return _entry; }

private:
    const Entry * _entry;
};
    
class PhaseCall : virtual public Call, protected FromEntry  {
public:
    PhaseCall( const Phase *, const Entry * toEntry );
    PhaseCall( const PhaseCall&, unsigned int src_replica, unsigned int dst_replica );

    virtual PhaseCall * clone( unsigned int src_replica, unsigned int dst_replica ) { return new PhaseCall( *this, src_replica, dst_replica ); }
};


class ForwardedCall : public PhaseCall {
public:
    ForwardedCall( const Phase *, const Entry * toEntry, const Call * fwdCall );

    virtual bool check() const;

    virtual bool isForwardedCall() const { return true; }
    virtual const std::string& srcName() const;
    virtual const ForwardedCall& insertDOMResults() const;

private:
    const Call * _fwdCall;		// Original forwarded call
};

/* -------------------------------------------------------------------- */

class FromActivity : virtual protected Call {
public:
    FromActivity() {}
};

class ActivityCall : virtual public Call, protected FromActivity {
public:
    ActivityCall( const Activity * fromActivity, const Entry * toEntry );
    ActivityCall( const ActivityCall&, unsigned int src_replica, unsigned int dst_replica );

    virtual ActivityCall * clone( unsigned int src_replica, unsigned int dst_replica ) { return new ActivityCall( *this, src_replica, dst_replica ); }
};


class ActivityForwardedCall : public ActivityCall {
public:
    ActivityForwardedCall( const Activity *, const Entry * toEntry );

    virtual bool isForwardedCall() const { return true; }
};

/* -------------------------------------------------------------------- */

class ProcessorCall : virtual public Call {
public:
    ProcessorCall( const Phase *, const Entry * toEntry );

    virtual bool isProcessorCall() const { return true; }
    virtual ProcessorCall * clone( unsigned int, unsigned int ) { abort(); return nullptr; }
};


class PhaseProcessorCall : public ProcessorCall, protected FromEntry {
public:
    PhaseProcessorCall( const Phase * fromPhase, const Entry * toEntry );
};

class ActivityProcessorCall : public ProcessorCall, protected FromActivity {
public:
    ActivityProcessorCall( const Activity * fromActivity, const Entry * toEntry );
};
#endif
