/* -*- c++ -*-
 * $HeadURL$
 *
 * Pure virtual class for tasks and processors.
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

#if	!defined(ENTITY_H)
#define ENTITY_H

#include "dim.h"
#include <lqio/input.h>
#include <lqio/dom_processor.h>
#include "vector.h"
#include "cltn.h"
#include "prob.h"

class CallStack;
class Entity;
class Entry;
class Processor;
class Format;
class Interlock;
class Task;
class Model;
class Server;
class Submodel;
template <class type> class Stack;

typedef Vector<unsigned> ChainVector;

ostream& operator<<( ostream&, const Entity& );
int operator==( const Entity&, const Entity& );

#define TYPE_STR_BUFSIZ	20

/* ----------------------- Abstract Superclass ------------------------ */

class Entity {
    friend class Model;
    friend class Generate;

public:
    Entity( LQIO::DOM::Entity* domVersion, Cltn<Entry *>* entries = 0);
    virtual ~Entity();

private:
    Entity( const Entity& );		/* Copying is verbotten */
    Entity& operator=( const Entity& );

public:
    /* Initialization */

    virtual void configure( const unsigned );
    virtual unsigned findChildren( CallStack&, const bool ) const;
    virtual Entity& initWait();
    virtual Entity& initThroughputBound() { return *this; }
    virtual Entity& initPopulation() = 0;
    virtual Entity& initJoinDelay() { return *this; }
    virtual Entity& initThreads() { return *this; }
	
    /* Instance Variable Access */
	   
    virtual int priority() const { return 0; }
    virtual Entity& priority( const int ) { return *this; }
    virtual const Processor * processor() const = 0;
    virtual Entity& processor( Processor * aProcessor ) = 0;
    virtual scheduling_type scheduling() const { return domEntity->getSchedulingType(); }
    Entity& trace( const int flag ) { traceFlag = flag; return *this; }
    int trace() const { return traceFlag; }
    virtual Entity& copies( const unsigned );
    virtual unsigned copies() const;
    virtual Entity& replicas( const unsigned );
    virtual unsigned replicas() const { return domEntity->getReplicas(); }
    double population() const { return myPopulation; }
    virtual double variance() const { return myVariance; }
    unsigned submodel() const { return mySubmodel; }
    Entity& setSubmodel( const unsigned submodel ) { mySubmodel = submodel; return *this; }
    virtual double thinkTime( const unsigned = 0, const unsigned = 0 ) const { return myThinkTime; }
   virtual Entity& setOverlapFactor( const double ) { return *this; }

    virtual unsigned int fanOut( const Entity * ) const;

    double throughput() const;
    double utilization() const;

    /* Queries */

    virtual bool hasVariance() const { return attributes.variance; }
    bool hasDeterministicPhases() const { return attributes.deterministic; }
    bool hasSecondPhase() const { return (bool)(myMaxPhase > 1); }

    virtual unsigned hasClientChain( const unsigned, const unsigned ) const { return 0; }
    unsigned hasServerChain( const unsigned k ) const { return myServerChains.find(k); }
    virtual bool hasActivities() const { return false; }
    bool hasThreads() const { return nThreads() > 1 ? true : false; }
    virtual bool hasSynchs() const { return false; }
	
    bool isInOpenModel() const { return attributes.open_model ? true : false; }
    Entity& isInOpenModel( const bool yesOrNo ) { attributes.open_model = yesOrNo; return *this; }
    bool isInClosedModel() const { return attributes.closed_model ? true : false; }
    Entity& isInClosedModel( const bool yesOrNo ) { attributes.closed_model = yesOrNo; return *this; }
    Entity& isPureServer( const bool yesOrNo ) { attributes.pure_server = yesOrNo; return *this; }
    bool isPureServer() const { return (bool)attributes.pure_server; }
    Entity& isPureDelay( const bool yesOrNo ) { attributes.pure_delay = yesOrNo; return *this; }
    bool isPureDelay() const { return (bool)attributes.pure_delay; }
    Entity& initialized( const bool yesOrNo ) { attributes.initialized = yesOrNo; return *this; }
    bool initialized() const { return (bool)attributes.initialized; }
    virtual bool isUsed() const { return submodel() > 0; }

    virtual bool isTask() const          { return false; }
    virtual bool isReferenceTask() const { return false; }
    virtual bool isProcessor() const     { return false; }
    virtual bool isInfinite() const;
    bool isMultiServer() const   	 { return copies() > 1; }

    bool schedulingIsOk( const unsigned bits ) const;

    const Cltn<Entry *>& entries() const { return entryList; }
    Entity& addEntry( Entry * );
    Entity& removeEntry( Entry * );
    Cltn<Entry *> * entries( Cltn<Entry *> * entries );
    Entry * entryAt( const unsigned index ) const { return entryList[index]; }

    virtual const char * name() const { return domEntity->getName().c_str(); }
    unsigned maxPhase() const { return myMaxPhase; }

    unsigned nEntries() const { return entryList.size(); }
    virtual unsigned nClients() const = 0;
    unsigned clients( Cltn<Task *> & ) const;
    unsigned fanIn( const Task * ) const;

    Entity& addServerChain( const unsigned k ) { myServerChains.append(k); return *this; }
    const ChainVector& serverChains() const { return myServerChains; }
    Server * serverStation() const { return myServerStation; }

    bool markovOvertaking() const;

    double openArrivalRate() const;

    /* Computation */
	
    Probability prInterlock( const Task& ) const;
    virtual double prOt( const unsigned, const unsigned, const unsigned ) const { return 0.0; }
    void setIdleTime( const double , Submodel * aSubmodel);
    virtual Entity& computeVariance();
    void setOvertaking( const unsigned, const Cltn<Task *>& );
    virtual Entity& updateWait( const Submodel&, const double ) { return *this; }	/* NOP */
    virtual double updateWaitReplication( const Submodel&, unsigned& ) { return 0.0; }	/* NOP */
    double deltaUtilization() const;

    /* Dynamic Updates / Late Finalization */
    /* In order to integrate LQX's support for model changes we need to have a way  */
    /* of re-calculating what used to be static for all dynamically editable values */
	
    virtual void recalculateDynamicValues() {}

    /* Sanity Check */

    virtual void sanityCheck() const;

    /* MVA interface */

    virtual Server * makeServer( const unsigned ) = 0;

    /* Threads */
	
    virtual unsigned nThreads() const { return 1; }		/* Return the number of threads. */
    virtual unsigned concurrentThreads() const { return 1; }	/* Return the number of threads. */
    virtual void joinOverlapFactor( const Submodel&, VectorMath<double>* ) const {};	/* NOP? */
	
    /* Printing */

    virtual ostream& print( ostream& ) const = 0;
    ostream& printServerChains( ostream& output ) const;
    ostream& printOvertaking( ostream& output ) const;
    virtual ostream& printJoinDelay( ostream& output ) const { return output; }

protected:
    virtual unsigned validScheduling() const;
    virtual scheduling_type defaultScheduling() const { return SCHEDULE_FIFO; }
    double computeIdleTime( const unsigned, const double ) const;
	
private:
    void setServiceTime( Server * station, const Entry * anEntry, unsigned k ) const;

public:
    LQIO::DOM::Entity* domEntity;	/* The DOM Representation	*/
    Interlock * interlock;		/* For interlock calculation.	*/

protected:
    Cltn<Entry *> entryList;		/* Entries for this entity.	*/
    double myPopulation;		/* customers when I'm a client	*/
    double myVariance;			/* Computed variance.		*/
    double myThinkTime;			/* Think time.			*/
    Server * myServerStation;		/* Servers by submodel.		*/

    struct {
	unsigned initialized:1;		/* Task was initialized.	*/
	unsigned closed_model:1;	/* Stn in in closed model.	*/
	unsigned open_model:1;		/* Stn is in open model.	*/
	unsigned deterministic:1;	/* an entry has det. phase.	*/
	unsigned pure_delay:1;		/* Wierd task.			*/
	unsigned pure_server:1;		/* Can use FCFS schedulging.	*/
 	unsigned variance:1;		/* an entry has Cv_sqn != 1.	*/
    } attributes;

private:
    unsigned short myMaxPhase;		/* Max phase over all entries	*/
    short traceFlag;			/* trace ref to this queue.	*/
    unsigned mySubmodel;		/* My submodel, 0 == ref task.	*/
    mutable double myLastUtilization;	/* For convergence test.								*/

    /* MVA interface */

    ChainVector myServerChains;		/* Chains for this server.	*/
};



/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNEntityManip {
public:
    SRVNEntityManip( ostream& (*ff)(ostream&, const Entity & ), const Entity & theEntity ) : f(ff), anEntity(theEntity) {}
private:
    ostream& (*f)( ostream&, const Entity& );
    const Entity & anEntity;

    friend ostream& operator<<(ostream & os, const SRVNEntityManip& m ) { return m.f(os,m.anEntity); }
};

SRVNEntityManip print_server_chains( const Entity & aServer );
SRVNEntityManip print_info( const Entity& );
#endif
