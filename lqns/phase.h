/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/phase.h $
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: phase.h 17428 2024-11-05 00:47:59Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_PHASE_H
#define LQNS_PHASE_H

#include <set>
#include <lqio/dom_phase.h>
#include <mva/vector.h>
#include <mva/prob.h>
#include "call.h"
#include "interlock.h"

class Activity;
class ActivityList;
class AndForkActivityList;
class DeviceEntry;
class Entity;
class Entry;
class Processor;
class Submodel;
class Task;
class TaskEntry;

namespace LQIO {
    namespace DOM {
	class Phase;
	class Call;
    }
}

/* -------------------------------------------------------------------- */

class NullPhase {

public:
    NullPhase( const std::string& name );
    virtual ~NullPhase() = default;
    int operator==( const NullPhase& aPhase ) const { return &aPhase == this; }

protected:
    NullPhase( const NullPhase& );

public:
    /* Initialialization */
	
    /* Instance variable access */
    virtual NullPhase& configure( const unsigned );
    NullPhase& setDOM(LQIO::DOM::Phase* phaseInfo);
    virtual LQIO::DOM::Phase* getDOM() const { return _dom; }
    virtual const std::string& name() const { return _name; }
    NullPhase& setName( const std::string& name ) { _name = name; return *this; }

    /* Queries */
    virtual bool isPresent() const { return getDOM() != nullptr && getDOM()->isPresent(); }
    bool hasServiceTime() const { return getDOM() != nullptr && getDOM()->hasServiceTime(); }
    bool hasThinkTime() const { return getDOM() != nullptr && getDOM()->hasThinkTime(); }
    virtual bool isActivity() const { return false; }
	
    NullPhase& setServiceTime( const double t );
    NullPhase& addServiceTime( const double t );
    double serviceTime() const;
    double thinkTime() const;
    double CV_sqr() const;

    NullPhase& setVariance( double variance ) { _variance = variance; return *this; }
    NullPhase& addVariance( double variance ) { _variance += variance; return *this; }
    virtual double variance() const { return _variance; } 		/* Computed variance.		*/
    double computeCV_sqr() const;

    virtual double waitExcept( const unsigned ) const;
    double residenceTime() const { return waitExcept( 0 ); }
    NullPhase& setWaitTime( unsigned int submodel, double value ) { _wait[submodel] = value; return *this; }
    NullPhase& addWaitTime( unsigned int submodel, double value ) { _wait[submodel] += value; return *this; }
    double getWaitTime( unsigned int submodel ) const { return _wait[submodel];}
    size_t getWaitSize() const { return _wait.size(); }

    virtual void recalculateDynamicValues() {}

    static void insertDOMHistogram( LQIO::DOM::Histogram * histogram, const double m, const double v );

private:
    LQIO::DOM::Phase* _dom;
    std::string _name;			/* Name -- computed dynamically		*/
    double _serviceTime;		/* Initial service time.		*/
    double _variance;			/* Set if this is a processor		*/

protected:
    /* Computed in class Phase */
    VectorMath<double> _wait;		/* Saved waiting time.			*/
};

/* -------------------------------------------------------------------- */

class Phase : public NullPhase {
    friend class Activity;		/* For access to mySurrogateDelay 	*/
    friend class RepeatActivityList;	/* For access to mySurrogateDelay 	*/
    friend class OrForkActivityList;	/* For access to mySurrogateDelay 	*/

public:
    struct get_servers {
	get_servers( std::set<Entity *>& servers ) : _servers(servers) {}
	void operator()( const Phase& phase ) const;
	void operator()( const Phase* phase ) const;
    private:
	std::set<Entity *>& _servers;
    };

private:
    /* Bonus entries are created on devices for each phase */
    class DeviceInfo {
	friend class Phase;
	
    public:
	enum class Type { HOST, PROCESSOR, THINK_TIME };

	DeviceInfo( const Phase&, const std::string&, Type );		/* True if this device is the phase's processor */
	~DeviceInfo();

	bool isHost() const { return _type == Type::HOST; }
	bool isProcessor() const { return _type == Type::HOST || _type == Type::PROCESSOR; }
	ProcessorCall * call() const { return _call; }
	DeviceEntry * entry() const { return _entry; }
	void recalculateDynamicValues();

    private:
	double service_time() const { return _phase.serviceTime(); }
	double think_time() const { return _phase.thinkTime(); }
	double n_calls() const { return _phase.numberOfSlices(); }
	double CV_sqr() const { return _phase.CV_sqr(); }
	double n_processor_calls() const { return service_time() > 0. ? n_calls() : 0.0; }	// zero if no service. BUG 315.

	static std::set<Entity *>& add_server( std::set<Entity *>&, const DeviceInfo * );
	
	struct add_wait {
	    add_wait( unsigned int submodel ) : _submodel(submodel) {}
	    double operator()( double sum, const DeviceInfo * call ) const;
	private:
	    const unsigned int _submodel;
	};

    private:
	const Phase& _phase;
	const std::string _name;
	const Type _type;
	DeviceEntry * _entry;
	ProcessorCall * _call;
	LQIO::DOM::Entry * _entry_dom;		/* Only for ~Phase		*/
	LQIO::DOM::Call * _call_dom;		/* Only for ~Phase		*/
    };

    struct add_weighted_wait {
	add_weighted_wait( unsigned int submodel, double total ) : _submodel(submodel), _total(total) {}
	double operator()( double sum, const Call * call ) const { return call->submodel() == _submodel ? sum + call->wait() * call->rendezvous() / _total: sum; }
    private:
	const unsigned int _submodel;
	const double _total;
    };

    struct follow_interlock {
	follow_interlock( Interlock::CollectTable& path ) : _path(path) {}
	void operator()( const Call * call ) const;
	void operator()( const DeviceInfo* device ) const;
    private:
	Interlock::CollectTable& _path;
    };
    
    struct get_interlocked_tasks {
	get_interlocked_tasks( Interlock::CollectTasks& path ) : _path(path) {}
	bool operator()( bool rc, const Call * call ) const;
	bool operator()( bool rc, const DeviceInfo* device ) const;
    private:
	Interlock::CollectTasks& _path;
    };

public:
    Phase( const std::string& = "" );
    Phase( const Phase&, unsigned int );
    Phase( const Phase& );		/* For _phase.resize() */
    virtual ~Phase();

public:
    /* Initialialization */
	
    void initialize( const std::string name, unsigned int n, Entry* entry );
    unsigned int getPhaseNumber() const { return _phase_number; }
    NullPhase& setPhaseNumber( unsigned int p ) { _phase_number = p; return *this; }
#if PAN_REPLICATION
    size_t getSurrogateDelaySize() const { return _surrogateDelay.size(); }
    Phase& setSurrogateDelaySize( size_t );
    Phase& clearSurrogateDelay();
    Phase& addSurrogateDelay( const VectorMath<double>& );
#endif

    Phase& expandCalls();
    virtual Phase& initializeProcessor();
    Phase& initCustomers( std::deque<const Task *>& stack, unsigned int customers );
    Phase& initVariance();
    
    unsigned findChildren( Call::stack&, const bool ) const;
    virtual const Phase& followInterlock( Interlock::CollectTable&  ) const;
    virtual void callsPerform( Call::Perform& ) const;
    void addSrcCall( Call * aCall ) { _calls.insert(aCall); }
    void removeSrcCall( Call *aCall ) { _calls.erase(aCall); }
    virtual unsigned int getReplicaNumber() const;

    /* Instance Variable access */
	
    LQIO::DOM::Phase::Type phaseTypeFlag() const { return getDOM() ? getDOM()->getPhaseTypeFlag() : LQIO::DOM::Phase::Type::STOCHASTIC; }
    Phase& setEntry( const Entry * entry ) { _entry = entry; return *this; }
    const Entry * entry() const { return _entry; }
    virtual const Entity * owner() const;
    Phase& setPrOvertaking( const Probability& pr_ot ) { _prOvertaking = pr_ot; return *this; }
    const Probability& prOvertaking() const { return _prOvertaking; }

    bool isDeterministic() const { return getDOM() ? getDOM()->getPhaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC : false; }
    bool isNonExponential() const { return serviceTime() > 0 && CV_sqr() != 1.0; }
    
    /* Call lists to/from entries. */
	
    double rendezvous( const Entity * ) const;
    double rendezvous( const Entry * ) const;
    Phase& rendezvous( Entry *, const LQIO::DOM::Call* callDOMInfo );
    double sendNoReply( const Entry * ) const;
    Phase& sendNoReply( Entry *, const LQIO::DOM::Call* callDOMInfo );
    Phase& forward( Entry *, const LQIO::DOM::Call* callDOMInfo );
    double forward( const Entry * ) const;

    const std::set<Call *>& callList() const { return _calls; }

    /* Calls to processors */
	
    DeviceInfo * getProcessor() const;
    ProcessorCall * processorCall() const;
    DeviceEntry * processorEntry() const;
    double processorCalls() const;
    double queueingTime() const;
    double processorWait() const;
    const std::vector<DeviceInfo *>& devices() const { return _devices; }
	
    /* Queries */

    virtual bool check() const;
    double numberOfSlices() const;
    double numberOfSubmodels() const { return _wait.size(); }
    virtual double throughput() const;
    double utilization() const;
    double processorUtilization() const;
    bool hasVariance() const;
    bool hasCalls() const { return !_calls.empty(); }
    virtual bool isPseudo() const { return false; }		// quorum
    
    /* computation */
	
    virtual double waitExcept( const unsigned ) const;
    Phase& updateWait( const Submodel&, const double );
    void computeVariance();	 			/* Computed variance.		*/
    double getProcWait( unsigned int submodel ); // tomari quorum
    double getTaskWait( unsigned int submodel );
    double getRendezvous( unsigned int submodel );
#if PAN_REPLICATION
    double waitExceptChain( const unsigned, const unsigned k );
    double updateWaitReplication( const Submodel& );
    double getReplicationProcWait( unsigned int submodel );
    double getReplicationTaskWait(); //tomari quorum
    double getReplicationRendezvous( unsigned int submodel );
#endif
    virtual bool getInterlockedTasks( Interlock::CollectTasks& path ) const;

    /* recalculation of dynamic values */
	
    virtual void recalculateDynamicValues();
    virtual const Phase& insertDOMResults() const; 

protected:
    Call * findCall( const Entry * anEntry, const queryFunc = 0 ) const;
    Call * findFwdCall( const Entry * anEntry ) const;
    virtual Call * findOrAddCall( const Entry * anEntry, const queryFunc = 0 );
    virtual Call * findOrAddFwdCall( const Entry * anEntry, const Call * fwdCall );

    double processorVariance() const;
    virtual ProcessorCall * newProcessorCall( Entry * procEntry ) const;

private:
    Phase const& addForwardingRendezvous( Call::stack& callStack ) const;
    Phase& forwardedRendezvous( const Call * fwdCall, const double value );
    double sumOfRendezvous() const;
#if PAN_REPLICATION
    double nrFactor( const Call * aCall, const Submodel& aSubmodel ) const;
#endif
    double mol_phase() const;
    double stochastic_phase() const;
    double deterministic_phase() const;
    double random_phase() const;
    double square_phase() const { return square( residenceTime() ); }
	
private:
    unsigned int _phase_number;		/* phase of entry (if phase)		*/
    const Entry * _entry;		/* Root for activity			*/
    std::set<Call *> _calls;         	/* Who I call.                          */
    std::vector<DeviceInfo *> _devices;	/* Will replace below			*/
#if PAN_REPLICATION
    VectorMath<double> _surrogateDelay;	/* Saved old surrogate delay. REP N-R	*/
#endif
    Probability _prOvertaking;
};
#endif
