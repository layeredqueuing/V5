/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/phase.h $
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: phase.h 14081 2020-11-11 18:56:16Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PHASE_H)
#define PHASE_H

#include <string>
#include <set>
#include <lqio/input.h>
#include "vector.h"
#include "call.h"
#include "prob.h"
#include "interlock.h"

class ProcessorCall;
class Entry;
class DeviceEntry;
class Entity;
class TaskEntry;
class Task;
class Submodel;
class AndForkActivityList;
class ActivityList;
class Activity;

namespace LQIO {
    namespace DOM {
	class Phase;
	class Call;
    }
}

class NullPhase {
    friend class Entry;			/* For access to myVariance */
    friend class TaskEntry;		/* For access to myVariance */
    friend class DeviceEntry;		/* For access to myWait     */
    friend class Activity;		/* For access to myVariance */
    friend class ActivityList;		/* For access to myVariance */
    friend class RepeatActivityList;	/* For access to myVariance */
    friend class OrForkActivityList;	/* For access to myVariance */
    friend class AndForkActivityList;	/* For access to myVariance */

public:
    NullPhase()
	: _name(),
	  _variance(0.0),
	  _wait(),
	  _dom(nullptr),
	  _serviceTime(0.0)
	{}

    virtual ~NullPhase() {}

    int operator==( const NullPhase& aPhase ) const { return &aPhase == this; }

    /* Initialialization */
	
    virtual NullPhase& configure( const unsigned );
    NullPhase& setDOM(LQIO::DOM::Phase* phaseInfo);
    virtual LQIO::DOM::Phase* getDOM() const { return _dom; }
    virtual const std::string& name() const { return _name; }
    NullPhase& setName( const std::string& name ) { _name = name; return *this; }

    /* Instance variable access */
    virtual bool isPresent() const { return _dom != 0 && _dom->isPresent(); }
    bool hasThinkTime() const { return _dom && _dom->hasThinkTime(); }
    virtual bool isActivity() const { return false; }
	
    NullPhase& setServiceTime( const double t );
    NullPhase& addServiceTime( const double t );
    double serviceTime() const;
    double thinkTime() const;
    double CV_sqr() const;

    virtual double variance() const { return _variance; } 		/* Computed variance.		*/
    double computeCV_sqr() const;

    virtual double waitExcept( const unsigned ) const;
    double elapsedTime() const { return waitExcept( 0 ); }
    double waitingTime( unsigned int submodel ) const { return _wait[submodel];}

    virtual ostream& print( ostream& output ) const { return output; }
	
    virtual NullPhase& recalculateDynamicValues() { return *this; }

    static void insertDOMHistogram( LQIO::DOM::Histogram * histogram, const double m, const double v );

protected:
    std::string _name;			/* Name -- computed dynamically		*/
    double _variance;			/* Set if this is a processor		*/
    VectorMath<double> _wait;		/* Saved waiting time.			*/
    LQIO::DOM::Phase* _dom;
	
private:
    double _serviceTime;		/* Initial service time.		*/
};


/* -------------------------------------------------------------------- */

class Phase : public NullPhase {
    friend class Entry;			/* For access to mySurrogateDelay 	*/
    friend class TaskEntry;
    friend class Activity;		/* For access to mySurrogateDelay 	*/
    friend class ActivityList;		/* For access to mySurrogateDelay 	*/
    friend class RepeatActivityList;	/* For access to mySurrogateDelay 	*/
    friend class OrForkActivityList;	/* For access to mySurrogateDelay 	*/
    friend class AndForkActivityList;	/* For access to mySurrogateDelay 	*/

private:
    struct add_weighted_wait {
	add_weighted_wait( unsigned int submodel, double total ) : _submodel(submodel), _total(total) {}
	double operator()( double sum, const Call * call ) const { return call->submodel() == _submodel ? sum + call->wait() * call->rendezvous() / _total: sum; }
    private:
	const unsigned int _submodel;
	const double _total;
    };

    struct get_interlocked_tasks {
	get_interlocked_tasks( Interlock::CollectTasks& path ) : _path(path) {}
	bool operator()( bool rc, const Call * call ) const;
    private:
	Interlock::CollectTasks& _path;
    };


public:
    class CallExec {
    public:
	CallExec( const Entry * e, unsigned submodel, unsigned k, unsigned p, callFunc f, double rate ) : _e(e), _submodel(submodel), _k(k), _p(p), _f(f), _rate(rate) {}
	CallExec( const CallExec& src, double rate, unsigned p ) : _e(src._e), _submodel(src._submodel), _k(src._k), _p(p), _f(src._f), _rate(rate) {}		// Set rate and phase.
	CallExec( const CallExec& src, double scale ) : _e(src._e), _submodel(src._submodel), _k(src._k), _p(src._p), _f(src._f), _rate(src._rate * scale) {}	// Set rate.

	const Entry * entry() const { return _e; }
	double getRate() const { return _rate; }
	unsigned getPhase() const { return _p; }

	void operator()( Call * object ) const { if (object->submodel() == _submodel) (object->*_f)( _k, _p, _rate ); }

    private:
	const Entry * _e;
	const unsigned _submodel;
	const unsigned _k;
	const unsigned _p;
	const callFunc _f;
	const double _rate;
    };
    
public:
    Phase( const std::string& );
    Phase();
    virtual ~Phase();

    /* Initialialization */
	
    virtual Phase& initProcessor();
    Phase& initReplication( const unsigned );
    Phase& resetReplication();
    Phase& initWait();
    Phase& initVariance();

    unsigned findChildren( Call::stack&, const bool ) const;
    virtual const Phase& followInterlock( Interlock::CollectTable&  ) const;
    virtual void callsPerform( const CallExec& ) const;
    void setInterlockedCall(const unsigned submodel);
    void addSrcCall( Call * aCall ) { _callList.insert(aCall); }
    void removeSrcCall( Call *aCall ) { _callList.erase(aCall); }

    /* Instance Variable access */
	
    phase_type phaseTypeFlag() const { return _dom ? _dom->getPhaseTypeFlag() : PHASE_STOCHASTIC; }
    Phase& setEntry( const Entry * entry ) { _entry = entry; return *this; }
    const Entry * entry() const { return _entry; }
    virtual const Entity * owner() const;
    Phase& setPrOvertaking( const Probability& pr_ot ) { _prOvertaking = pr_ot; return *this; }
    const Probability& prOvertaking() const { return _prOvertaking; }

    bool isDeterministic() const { return _dom ? _dom->getPhaseTypeFlag() == PHASE_DETERMINISTIC : false; }
    bool isNonExponential() const { return serviceTime() > 0 && CV_sqr() != 1.0; }
    
    /* Call lists to/from entries. */
	
    double rendezvous( const Entity * ) const;
    double rendezvous( const Entry * ) const;
    Phase& rendezvous( Entry *, const LQIO::DOM::Call* callDOMInfo );
    double sendNoReply( const Entry * ) const;
    Phase& sendNoReply( Entry *, const LQIO::DOM::Call* callDOMInfo );
    Phase& forward( Entry *, const LQIO::DOM::Call* callDOMInfo );
    double forward( const Entry * ) const;

    const std::set<Call *>& callList() const { return _callList; }

    /* Calls to processors */
	
    ProcessorCall * processorCall() const { return _processorCall; }
    double processorCalls() const;
    double queueingTime() const;
    double processorWait() const;
	
    /* Queries */

    virtual bool check() const;
    double numberOfSlices() const;
    double numberOfSubmodels() const { return _wait.size(); }
    virtual double throughput() const;
    double utilization() const;
    double processorUtilization() const;
    bool isUsed() const { return _callList.size() > 0.0 || serviceTime() > 0.0; }
    bool hasVariance() const;
    virtual bool isPseudo() const { return false; }		// quorum

    /* computation */
	
    virtual double waitExcept( const unsigned ) const;
    double waitExceptChain( const unsigned, const unsigned k );
    double computeVariance();	 			/* Computed variance.		*/
    Phase& updateWait( const Submodel&, const double ); 
    double getProcWait( unsigned int submodel, const double relax ); // tomari quorum
    double getTaskWait( unsigned int submodel, const double relax );
    double getRendezvous( unsigned int submodel, const double relax );
    double updateWaitReplication( const Submodel& );
    double getReplicationProcWait( unsigned int submodel, const double relax );
    double getReplicationTaskWait( unsigned int submodel, const double relax ); //tomari quorum
    double getReplicationRendezvous( unsigned int submodel, const double relax );
    virtual bool getInterlockedTasks( Interlock::CollectTasks& path ) const;

    /* recalculation of dynamic values */
	
    virtual Phase& recalculateDynamicValues();
    virtual const Phase& insertDOMResults() const; 

protected:
    Call * findCall( const Entry * anEntry, const queryFunc = 0 ) const;
    Call * findFwdCall( const Entry * anEntry ) const;
    virtual Call * findOrAddCall( const Entry * anEntry, const queryFunc = 0 );
    virtual Call * findOrAddFwdCall( const Entry * anEntry, const Call * fwdCall );

    double processorVariance() const;
    virtual ProcessorCall * newProcessorCall( Entry * procEntry );

private:
    Phase const& addForwardingRendezvous( Call::stack& callStack ) const;
    Phase& forwardedRendezvous( const Call * fwdCall, const double value );
    double sumOfRendezvous() const;
    double nrFactor( const Call * aCall, const Submodel& aSubmodel ) const;
    double mol_phase() const;
    double stochastic_phase() const;
    double deterministic_phase() const;
    double random_phase() const;
	
protected:
    const Entry * _entry;		/* Root for activity			*/
    std::set<Call *> _callList;         /* Who I call.                          */
    ProcessorCall * _processorCall;    	/* Link to processor.                   */
    ProcessorCall * _thinkCall;		/* Link to processor.                   */

private:
    DeviceEntry * _processorEntry;      /*                                      */
    DeviceEntry * _thinkEntry;          /*                                      */

    VectorMath<double> _surrogateDelay;	/* Saved old surrogate delay. REP N-R	*/
    Probability _prOvertaking;
};

inline ostream& operator<<( ostream& output, const Phase& self ) { self.print( output ); return output; }
#endif
