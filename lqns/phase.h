/* -*- c++ -*-
 * $HeadURL$
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PHASE_H)
#define PHASE_H

#include <string>
#include <lqio/input.h>
#include <lqio/dom_call.h>
#include <lqio/dom_phase.h>
#include <lqio/dom_extvar.h>
#include "cltn.h"
#include "vector.h"
#include "prob.h"

class Call;
class ProcessorCall;
class CallStack;
class Entry;
class DeviceEntry;
class Entity;
class Model;
class TaskEntry;
class Task;
class Submodel;
class AndForkActivityList;
class ActivityList;
class Activity;
class InterlockInfo;
template <class type> class Stack;

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
	: myVariance(0.0),
	  myDOMPhase(NULL),
	  myServiceTime(0.0)
	{}
	
    /* Results */

public:
    NullPhase( const NullPhase& );
    NullPhase& operator=( const NullPhase& );
    virtual ~NullPhase() {}

    int operator==( const NullPhase& aPhase ) const { return &aPhase == this; }

    /* Initialialization */
	
    virtual void configure( const unsigned, const unsigned = 0 );
    NullPhase& setDOM(LQIO::DOM::Phase* phaseInfo);
    virtual LQIO::DOM::Phase* getDOM() const { return myDOMPhase; }

    /* Instance variable access */
    virtual bool isPresent() const { return myDOMPhase != 0 && myDOMPhase->isPresent(); }
    bool hasThinkTime() const { return myDOMPhase && myDOMPhase->hasThinkTime(); }
    virtual bool isActivity() const { return false; }
	
    NullPhase& setServiceTime( const double t );
    NullPhase& addServiceTime( const double t );
    double serviceTime() const;
    double thinkTime() const { return (hasThinkTime()) ? myDOMPhase->getThinkTimeValue() : 0; }

    virtual double variance() const { return myVariance; } 		/* Computed variance.		*/
    double CV_sqr() const { return (myDOMPhase && myDOMPhase->hasCoeffOfVariationSquared()) ? 
	    myDOMPhase->getCoeffOfVariationSquaredValue() : 1.0; }
    double computeCV_sqr() const;

    double waitExcept( const unsigned ) const;
    double elapsedTime() const { return waitExcept( 0 ); }
    double waitTime(int submodel) { return myWait[submodel];}

    virtual ostream& print( ostream& output ) const { return output; }
	
    virtual void recalculateDynamicValues() {}

    static void insertDOMHistogram( LQIO::DOM::Histogram * histogram, const double m, const double v );

protected:
    double myVariance;			/* Set if this is a processor	*/
    VectorMath<double> myWait;		/* Saved waiting time.		*/
    LQIO::DOM::Phase* myDOMPhase;
	
private:
    double myServiceTime;		/* Initial service time.	*/
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

public:
    Phase( const char * aName=0 );
    virtual ~Phase();

    /* Initialialization */
	
    virtual void initProcessor();
    void initReplication( const unsigned );
    void initWait();
    void initVariance();

    unsigned findChildren( CallStack&, const bool ) const;
    virtual unsigned followInterlock( Stack<const Entry *> &, const InterlockInfo&, const unsigned  );
    virtual void callsPerform( Stack<const Entry *>&, const AndForkActivityList *, const unsigned, const unsigned, const unsigned, callFunc, const double ) const;
	
    void addSrcCall( Call * aCall ) { myCalls << aCall; }
    void removeSrcCall( Call *aCall ) { myCalls -= aCall; }

    double processorCalls() const;

    /* Instance Variable access */
	
    phase_type phaseTypeFlag() const { return myDOMPhase ? myDOMPhase->getPhaseTypeFlag() : PHASE_STOCHASTIC; }
    void resetReplication();
    virtual const Entry * entry() const = 0;
    virtual const Entity * owner() const = 0;
    virtual const char * name() const { return myName.c_str(); }

    /* Call lists to/from entries. */
	
    double rendezvous( const Entity * ) const;
    double rendezvous( const Entry * ) const;
    Phase& rendezvous( Entry *, LQIO::DOM::Call* callDOMInfo );
    double sendNoReply( const Entry * ) const;
    Phase& sendNoReply( Entry *, LQIO::DOM::Call* callDOMInfo );
    Phase& forward( Entry *, LQIO::DOM::Call* callDOMInfo );
    double forward( const Entry * ) const;

    const Cltn<Call *>& callList() const { return myCalls; }
    ProcessorCall * processorCall() const { return myProcessorCall; }

    /* Calls to processors */
	
    double queueingTime() const;
    double processorWait() const;
	
    /* Queries */

    void check( const unsigned=0 ) const;
    double numberOfSlices() const;
    virtual double throughput() const = 0;	
    double utilization() const;
    double processorUtilization() const;
    bool isUsed() const { return myCalls.size() > 0.0 || serviceTime() > 0.0; }
    bool hasVariance() const;
    virtual bool isPseudo() const { return false; }		// quorum

    /* computation */
	
    double computeVariance();	 			/* Computed variance.		*/
    double waitExceptChain( const unsigned, const unsigned k );
    Phase& updateWait( const Submodel&, const double ); 
    double getProcWait( unsigned int submodel, const double relax ); // tomari quorum
    double getTaskWait( unsigned int submodel, const double relax );
    double getRendezvous( unsigned int submodel, const double relax );
    double updateWaitReplication( const Submodel& );
    double getReplicationProcWait( unsigned int submodel, const double relax );
    double getReplicationTaskWait( unsigned int submodel, const double relax ); //tomari quorum
    double getReplicationRendezvous( unsigned int submodel, const double relax );
    virtual bool getInterlockedTasks( Stack<const Entry *>&, const Entity *, Cltn<const Entity *>&, const unsigned ) const;
	
    /* recalculation of dynamic values */
	
    virtual void recalculateDynamicValues();
	
    virtual void insertDOMResults() const; 

protected:
    Call * findCall( const Entry * anEntry, const queryFunc = 0 ) const;
    Call * findFwdCall( const Entry * anEntry ) const;
    virtual Call * findOrAddCall( const Entry * anEntry, const queryFunc = 0 );
    virtual Call * findOrAddFwdCall( const Entry * anEntry, const Call * fwdCall );

    double processorVariance() const;
    virtual ProcessorCall * newProcessorCall( Entry * procEntry ) = 0;

private:
    Phase const& addForwardingRendezvous( CallStack& callStack ) const;
    Phase& forwardedRendezvous( const Call * fwdCall, const double value );
    double sumOfRendezvous() const;
    double nrFactor( const Call * aCall, const Submodel& aSubmodel ) const;
    double mol_phase() const;
    double stochastic_phase() const;
    double deterministic_phase() const;
    double random_phase() const;
	
protected:
    string myName;			/* Name -- computed dynamically		*/	
    Cltn<Call *> myCalls;               /* Who I call.                          */
    ProcessorCall * myProcessorCall;    /* Link to processor.                   */
    ProcessorCall * myThinkCall;	/* Link to processor.                   */

private:
    DeviceEntry * myProcessorEntry;     /*                                      */
    DeviceEntry * myThinkEntry;         /*                                      */

private:
    VectorMath<double> mySurrogateDelay;/* Saved old surrogate delay. REP N-R	*/
    bool iWasChanged;			/* True if reinit is required		*/
};



class GenericPhase : public Phase {

public:
    GenericPhase();

    virtual const Entry * entry() const { return myEntry; }
    virtual const Entity * owner() const;

    void initialize( Entry *, const int n );
    virtual double throughput() const;

protected:
    virtual ProcessorCall * newProcessorCall( Entry * procEntry );

private:
    Entry * myEntry;
};

ostream& operator<<( ostream&, const Phase& );
#endif
