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
 * $Id: entity.h 15956 2022-10-07 13:54:20Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(ENTITY_H)
#define ENTITY_H

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

    enum class Attributes {
	initialized,		/* Task was initialized.		*/
	closed_server,		/* Stn is server in closed model.	*/
	closed_client,		/* Stn is client in closed model.	*/
	open_server,		/* Stn is server in open model.		*/
	deterministic,		/* an entry has det. phase.		*/
 	variance		/* an entry has Cv_sqn != 1.		*/
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

    /*
     * Update waiting times.
     */
    
    struct update_wait {
	update_wait( Entity& entity ) : _entity(entity) {}
	void operator()( const Submodel* submodel ) { _entity.updateWait( *submodel, 1.0 ); }
    private:
	Entity& _entity;
    };

    static std::set<Task *>& add_clients( std::set<Task *>& clients, const Entity * entity ) { return entity->getClients( clients ); }

    /*
     * For SRVN layering.  Increment the submodel number
     */

    struct increment_submodel {
	increment_submodel() : _submodel(0) {}
	void operator()( Entity* server )
	    {
		_submodel += 1;
		server->setSubmodel( _submodel );
	    }
	size_t count() const { return _submodel; }
    private:
	size_t _submodel;
    };

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
    Entity& operator=( const Entity& ) = delete;

public:
    /* Initialization */

    virtual Entity& configure( const unsigned );
    virtual bool check() const;
    virtual unsigned findChildren( Call::stack&, const bool ) const;
    virtual Entity& initWait();
    Entity& initInterlock();
    virtual Entity& initThroughputBound() { return *this; }
    virtual Entity& initPopulation() = 0;
    virtual Entity& initJoinDelay() { return *this; }
    virtual Entity& initThreads() { return *this; }
    virtual Entity& initClient( const Vector<Submodel *>& ) { return *this; }
    virtual Entity& reinitClient( const Vector<Submodel *>& ) { return *this; }
    virtual Entity& initServer( const Vector<Submodel *>& );
    virtual Entity& reinitServer( const Vector<Submodel *>& );
	
    /* Instance Variable Access */
	   
    virtual LQIO::DOM::Entity * getDOM() const { return _dom; }
    virtual int priority() const { return 0; }
    virtual Entity& priority( const int ) { return *this; }
    virtual scheduling_type scheduling() const { return getDOM()->getSchedulingType(); }
    virtual unsigned copies() const;
    virtual unsigned replicas() const;
    double population() const { return _population; }
    virtual double variance() const { return _variance; }
    virtual const Processor * getProcessor() const { return nullptr; }
    unsigned submodel() const { return _submodel; }
    Entity& setSubmodel( const unsigned submodel ) { _submodel = submodel; return *this; }
    virtual double thinkTime( const unsigned = 0, const unsigned = 0 ) const { return _thinkTime; }
    virtual Entity& setOverlapFactor( const double ) { return *this; }
    unsigned getReplicaNumber() const { return _replica_number; }
    Entity& setPruned( bool pruned ) { _pruned = pruned; return *this; }
    
    virtual unsigned int fanOut( const Entity * ) const = 0;
    virtual unsigned int fanIn( const Task * ) const = 0;

    double throughput() const;
    double utilization() const { return _utilization; }
    double saturation() const;

    /* Queries */

    virtual bool hasVariance() const { return _attributes.at(Attributes::variance); }
    Entity& setVarianceAttribute( const bool yesOrNo ) { _attributes.at(Attributes::variance) = yesOrNo; return *this; }
    bool hasDeterministicPhases() const { return _attributes.at(Attributes::deterministic); }
    bool hasSecondPhase() const { return (bool)(_maxPhase > 1); }
    bool hasOpenArrivals() const;
    
    virtual bool hasClientChain( const unsigned, const unsigned ) const { return false; }
    bool hasServerChain( const unsigned k ) const;
    virtual bool hasActivities() const { return false; }
    bool hasThreads() const { return nThreads() > 1; }
    virtual bool hasSynchs() const { return false; }

    bool isOpenModelServer() const   { return _attributes.at(Attributes::open_server); }
    bool isClosedModelServer() const { return _attributes.at(Attributes::closed_server); }
    bool isClosedModelClient() const { return _attributes.at(Attributes::closed_client); }
    Entity& setOpenModelServer( const bool yesOrNo )   { _attributes.at(Attributes::open_server) = yesOrNo; return *this; }
    Entity& setClosedModelServer( const bool yesOrNo ) { _attributes.at(Attributes::closed_server) = yesOrNo; return *this; }
    Entity& setClosedModelClient( const bool yesOrNo ) { _attributes.at(Attributes::closed_client) = yesOrNo; return *this; }
    Entity& initialized( const bool yesOrNo ) { _attributes.at(Attributes::initialized) = yesOrNo; return *this; }
    bool initialized() const { return _attributes.at(Attributes::initialized); }
    virtual bool isUsed() const { return submodel() > 0; }

    virtual bool isTask() const          { return false; }
    virtual bool isReferenceTask() const { return false; }
    virtual bool isProcessor() const     { return false; }
    virtual bool isInfinite() const;

    bool isCalledBy( const Task* ) const;
    bool isMultiServer() const   	{ return copies() > 1; }
    bool isReplicated() const		{ return replicas() > 1; }
    bool isPruned() const		{ return _pruned; }

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
    double nCustomers( ) const;
    const std::set<const Entry *>& commonEntries() const { return _interlock.commonEntries(); }
    
    Entity& addServerChain( const unsigned k ) { _serverChains.push_back(k); return *this; }
    const ChainVector& serverChains() const { return _serverChains; }
    Server * serverStation() const { return _station; }

    bool markovOvertaking() const;

    /* Computation */
	
    Probability prInterlock( const Task& ) const;

    virtual double prOt( const unsigned, const unsigned, const unsigned ) const { return 0.0; }

    Entity& updateAllWaits( const Vector<Submodel *>& );
    void setIdleTime( const double );
    virtual Entity& computeVariance();
    virtual Entity& updateWait( const Submodel&, const double ) { return *this; }	/* NOP */
    virtual double updateWaitReplication( const Submodel&, unsigned& ) { return 0.0; }	/* NOP */
    double deltaUtilization() const;

    /* Sanity Check */

    virtual const Entity& sanityCheck() const;
    virtual bool openModelInfinity() const;

    /* MVA interface */

    Entity& clear();
    virtual Server * makeServer( const unsigned ) = 0;
    Entity& initServerStation( Submodel& );
    virtual Entity& saveServerResults( const MVASubmodel&, double );

    /* Threads */
	
    virtual unsigned nThreads() const { return 1; }		/* Return the number of threads. */
    virtual unsigned concurrentThreads() const { return 1; }	/* Return the number of threads. */
    virtual void joinOverlapFactor( const Submodel& ) const {};	/* NOP? */
	
protected:
    Entity& setUtilization( double );
    virtual bool schedulingIsOK() const = 0;
    virtual scheduling_type defaultScheduling() const { return SCHEDULE_FIFO; }
    virtual double computeUtilization( const MVASubmodel& );
	
private:
    void setServiceTime( const Entry * anEntry, unsigned k ) const;
    void setInterlock( Submodel& ) const;
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
    
    double _population;			/* customers when I'm a client	*/
    double _variance;			/* Computed variance.		*/
    double _thinkTime;			/* Think time.			*/
    Server * _station;			/* Servers by submodel.		*/

private:
    std::map<Attributes,bool> _attributes;
    Interlock _interlock;		/* For interlock calculation.	*/
    unsigned _submodel;			/* My submodel, 0 == ref task.	*/
    unsigned _maxPhase;			/* Largest phase.		*/
    double _utilization;		/* Utilization			*/
    mutable double _lastUtilization;	/* For convergence test.	*/
#if defined(BUG_393)
    std::vector<double> _marginalQueueProbabilities;
#endif

    /* MVA interface */

    ChainVector _serverChains;		/* Chains for this server.	*/
    const unsigned int _replica_number;	/* > 1 if a replica		*/
    bool _pruned;			/* True if pruned		*/
};
#endif
