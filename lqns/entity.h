/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/entity.h $
 *
 * Pure virtual class for tasks and processors.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: entity.h 16687 2023-04-21 01:04:31Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_ENTITY_H
#define LQNS_ENTITY_H

#include <vector>
#include <set>
#include <map>
#include <lqio/dom_entity.h>
#include <mva/prob.h>
#include <mva/vector.h>
#include "call.h"

class Entity;
class Entry;
class Processor;
class Interlock;
class Task;
class ReferenceTask;
class Model;
class Server;
class Submodel;
class MVASubmodel;

typedef Vector<unsigned> ChainVector;

std::ostream& operator<<( std::ostream&, const Entity& );
int operator==( const Entity&, const Entity& );

/* ----------------------- Abstract Superclass ------------------------ */

class Entity {
    friend class Generate;

    struct Attributes {
	Attributes() : closed_server(false), closed_client(false), open_server(false), pruned(false), deterministic(false), variance(false) {}

	bool closed_server;	/* Stn is server in closed model.	*/
	bool closed_client;	/* Stn is client in closed model.	*/
	bool open_server;	/* Stn is server in open model.		*/
	bool pruned;		/* Stn can be pruned			*/
	bool deterministic;	/* an entry has det. phase.		*/
 	bool variance;		/* an entry has Cv_sqn != 1.		*/
    };

protected:
    /*
     * Compare two entities by their name and replica number.  The
     * default replica is one, and will only not be one if replicas
     * are expanded to individual tasks.
     */
    
    struct equals {
	equals( const std::string& name, unsigned int replica=1 ) : _name(name), _replica(replica) {}
	bool operator()( const Entity * entity ) const { return entity->name() == _name && entity->getReplicaNumber() == _replica; }
    private:
	const std::string _name;
	const unsigned int _replica;
    };

public:
    struct sum_square {
	typedef double (Entity::*funcPtr)() const;
	sum_square( funcPtr f ) : _f(f) {}
	double operator()( double addend, const Entity* object ) { return addend + square( (object->*_f)() ); }
	double operator()( double addend, const Entity& object ) { return addend + square( (object.*_f)() ); }
    private:
	const funcPtr _f;
    };

    /*
     * Compare two entities by their name, but not replica number
     * except that entity must be a replica.
     */

    struct matches {
	matches( const std::string& name ) : _name(name) {}
	bool operator()( const Entity * entity ) const { return entity->name() == _name && entity->getReplicaNumber() > 1; }
    private:
	const std::string _name;
    };

    static std::set<Task *>& add_clients( std::set<Task *>& clients, const Entity * entity ) { return entity->getClients( clients ); }

private:
    class SRVNManip {
    public:
	SRVNManip( std::ostream& (*f)( std::ostream&, const Entity& ), const Entity& entity ) : _f(f), _entity(entity) {}
    private:
	std::ostream& (*_f)( std::ostream&, const Entity& );
	const Entity & _entity;

	friend std::ostream& operator<<( std::ostream& os, const SRVNManip& m ) { return m._f(os,m._entity); }
    };

public:
    Entity( LQIO::DOM::Entity*, const std::vector<Entry *>& );
    virtual ~Entity();

protected:
    Entity( const Entity&, unsigned int );
    virtual Entity * clone( unsigned int ) = 0;

private:
    Entity( const Entity& ) = delete;
    Entity& operator=( const Entity& ) = delete;

public:
    /* Initialization */

    virtual Entity& configure( const unsigned );
    virtual bool check() const;
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    void initializeInterlock();
    virtual Entity& initJoinDelay() { return *this; }
    virtual Entity& initThreads() { return *this; }
    virtual void initializeServer();
    void reinitializeServer();
	
    /* Instance Variable Access */
	   
    virtual LQIO::DOM::Entity * getDOM() const { return _dom; }
    virtual int priority() const { return 0; }
    virtual Entity& priority( const int ) { return *this; }
    virtual scheduling_type scheduling() const { return getDOM()->getSchedulingType(); }
    virtual unsigned int copies() const;
    virtual unsigned int replicas() const;
    virtual unsigned int population() const { return copies(); }
    virtual double variance() const { return _variance; }
    virtual const Processor * getProcessor() const { return nullptr; }
    unsigned submodel() const { return _submodel; }
    Entity& setSubmodel( const unsigned submodel ) { _submodel = submodel; return *this; }
    virtual double thinkTime( const unsigned = 0, const unsigned = 0 ) const { return _thinkTime; }
    virtual Entity& setOverlapFactor( const double ) { return *this; }
    unsigned getReplicaNumber() const { return _replica_number; }
    
    virtual unsigned int fanOut( const Entity * ) const = 0;
    virtual unsigned int fanIn( const Task * ) const = 0;

    double throughput() const;
    double utilization() const { return _utilization; }
    double saturation() const;

    /* Queries */

    bool hasSecondPhase() const { return (bool)(_maxPhase > 1); }
    bool hasOpenArrivals() const;
    
    bool hasServerChain( const unsigned k ) const;
    virtual bool hasActivities() const { return false; }
    bool hasThreads() const { return nThreads() > 1; }
    virtual bool hasSynchs() const { return false; }

    virtual bool hasVariance() const { return _attributes.variance; }
    bool hasDeterministicPhases() const { return _attributes.deterministic; }
    bool isClosedModelClient() const { return _attributes.closed_client; }
    bool isClosedModelServer() const { return _attributes.closed_server; }
    bool isOpenModelServer() const   { return _attributes.open_server; }
    Entity& setClosedModelClient( const bool yesOrNo ) { _attributes.closed_client = yesOrNo; return *this; }
    Entity& setClosedModelServer( const bool yesOrNo ) { _attributes.closed_server = yesOrNo; return *this; }
    Entity& setDeterministicPhases( const bool yesOrNo ) { _attributes.deterministic = yesOrNo; return *this; }
    Entity& setOpenModelServer( const bool yesOrNo )   { _attributes.open_server = yesOrNo; return *this; }
    Entity& setVarianceAttribute( const bool yesOrNo ) { _attributes.variance = yesOrNo; return *this; }

    virtual bool isTask() const          { return false; }
    virtual bool isReferenceTask() const { return false; }
    virtual bool isProcessor() const     { return false; }
    virtual bool isInfinite() const;
    bool isCalledBy( const Task* ) const;
    bool isMultiServer() const   	{ return copies() > 1; }
    bool isReplicated() const		{ return replicas() > 1; }

    virtual bool isUsed() const { return submodel() > 0; }
    bool isReplica() const;
    
    const std::vector<Entry *>& entries() const { return _entries; }
    Entity& addEntry( Entry * );
    Entry * entryAt( const unsigned index ) const { return _entries.at(index); }

    Entity& addTask( const Task * aTask ) { _tasks.insert( aTask ); return *this; }
    Entity& removeTask( const Task * aTask ) { _tasks.erase( aTask ); return *this; }
    const std::set<const Task *>& tasks() const { return _tasks; }

    virtual const std::string& name() const { return getDOM()->getName(); }
    unsigned maxPhase() const { return _maxPhase; }

    unsigned nEntries() const { return _entries.size(); }
    virtual unsigned nClients() const { return _tasks.size(); }
    std::set<Task *>& getClients( std::set<Task *>& ) const;
    const std::set<const Entry *>& commonEntries() const { return _interlock.commonEntries(); }
    
    Entity& addServerChain( const unsigned k ) { _serverChains.push_back(k); return *this; }
    const ChainVector& serverChains() const { return _serverChains; }
    Server* serverStation() const { return _station; }
    virtual Entity* mapToReplica( size_t i ) const = 0;

    bool markovOvertaking() const;

    /* Computation */
	
    Probability prInterlock( const Task& ) const;
    void setInterlock( Submodel& ) const;

    virtual double prOt( const unsigned, const unsigned, const unsigned ) const { return 0.0; }

    void saveServerResults( const MVASubmodel&, const Server&, double );
    Entity& updateAllWaits( const Vector<Submodel *>& );
    void setIdleTime( const double );
    virtual Entity& computeVariance();
    virtual double updateWaitReplication( const Submodel&, unsigned& ) { return 0.0; }	/* NOP */
    double deltaUtilization() const;

    /* Sanity Check */

    virtual const Entity& sanityCheck() const;
    virtual bool openModelInfinity() const;

    /* MVA interface */

    Entity& clear();
    virtual Server * makeServer( const unsigned ) = 0;

    /* Threads */
	
    virtual unsigned nThreads() const { return 1; }		/* Return the number of threads. */
    virtual unsigned concurrentThreads() const { return 1; }	/* Return the number of threads. */
    virtual void joinOverlapFactor( const Submodel& ) const {};	/* NOP? */
	
protected:
    Entity& setUtilization( double );
    virtual bool schedulingIsOK() const = 0;
    virtual scheduling_type defaultScheduling() const { return SCHEDULE_FIFO; }
    virtual double computeUtilization( const MVASubmodel&, const Server& );
	
private:
    double computeIdleTime( const unsigned, const double ) const;

public:
    virtual const Entity& insertDOMResults() const;

    /* Printing */

    virtual std::ostream& print( std::ostream& ) const = 0;
    virtual std::ostream& printJoinDelay( std::ostream& output ) const { return output; }
    static SRVNManip print_server_chains( const Entity& entity ) { return SRVNManip( output_server_chains, entity ); }
    SRVNManip print_name() const { return SRVNManip( output_name, *this ); }
    SRVNManip print_info() const { return SRVNManip( output_info, *this ); }
    SRVNManip print_type() const { return SRVNManip( output_type, *this ); }
    SRVNManip print_entries() const { return SRVNManip( output_entries, *this ); }
    static std::string fold( const std::string& s1, const Entity* );

private:
    static std::ostream& output_name( std::ostream& output, const Entity& );
    static std::ostream& output_type( std::ostream& output, const Entity& );
    static std::ostream& output_server_chains( std::ostream& output, const Entity& );
    static std::ostream& output_info( std::ostream& output, const Entity& );
    static std::ostream& output_entries( std::ostream& output, const Entity& );
    
private:
    LQIO::DOM::Entity* _dom;		/* The DOM Representation	*/

protected:
    std::vector<Entry *> _entries;	/* Entries for this entity.	*/
    std::set<const Task *> _tasks;	/* Clients of this entity	*/

    double _variance;			/* Computed variance.		*/
    double _thinkTime;			/* Think time.			*/
    Server * _station;			/* Servers by submodel.		*/

private:
    Attributes _attributes;
    Interlock _interlock;		/* For interlock calculation.	*/
    unsigned _submodel;			/* My submodel, 0 == ref task.	*/
    unsigned _maxPhase;			/* Largest phase.		*/
    double _utilization;		/* Utilization			*/
    mutable double _lastUtilization;	/* For convergence test.	*/
#if BUG_393
    std::vector<double> _marginalQueueProbabilities;
#endif

    /* MVA interface */

    ChainVector _serverChains;		/* Chains for this server.	*/
    const unsigned int _replica_number;	/* > 1 if a replica		*/

};
#endif
