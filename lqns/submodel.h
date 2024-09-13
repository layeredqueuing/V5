/* -*- c++ -*-
 * submodel.h	-- Greg Franks
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * June 2007
 *
 * $Id: submodel.h 17275 2024-09-10 20:35:49Z greg $
 */

#ifndef _SUBMODEL_H
#define _SUBMODEL_H


#include <set>
#include <mva/vector.h>
#include <mva/pop.h>

class Entry;
class Entity;
class Group;
class MVA;
class MVACount;
class Model;
class Open;
class Server;
class SubModelManip;
class Task;

typedef Vector<unsigned> ChainVector;

/* ------- Submodel Abstract Superclass.  Subclassed as needed. ------- */
	
class Submodel {
protected:
    typedef std::pair< std::set<Task *>, std::set<Entity*> > submodel_group_t;

private:
    class SubmodelManip {
    public:
	SubmodelManip( std::ostream& (*ff)(std::ostream&, const Submodel&, const unsigned long ),
		       const Submodel& aSubmodel, const unsigned long anInt )
	    : f(ff), submodel(aSubmodel), arg(anInt) {}
    private:
	std::ostream& (*f)( std::ostream&, const Submodel&, const unsigned long );
	const Submodel& submodel;
	const unsigned long arg;

	friend std::ostream& operator<<(std::ostream & os, const SubmodelManip& m ) { return m.f(os,m.submodel,m.arg); }
    };

    class SubmodelTraceManip {
    public:
	SubmodelTraceManip( std::ostream& (*ff)(std::ostream&, const std::string& ),
			    const std::string& s )
	    : f(ff), str(s) {}
    private:
	std::ostream& (*f)( std::ostream&, const std::string& );
	const std::string& str;

	friend std::ostream& operator<<(std::ostream & os, const SubmodelTraceManip& m ) { return m.f(os,m.str); }
    };

    /*
     * Remove all tasks/entites 'y' from either _clients/_servers 'x'.  Mark 'y'
     * as pruned.
     */

    template <class Type> struct erase_from {
	erase_from( std::set<Type>& x ) : _x(x) {}
	void operator()( Type y ) { _x.erase(y); }
    private:
	std::set<Type>& _x;
    };

public:
    Submodel( const unsigned n );
    virtual ~Submodel() {}

private:
    Submodel( const Submodel& ) { abort(); }		/* Copying is verbotten */
    Submodel& operator=( const Submodel& ) { abort(); return *this; }

public:
    int operator==( const Submodel& aSubmodel ) const { return &aSubmodel == this; }
    int operator!() const { return _servers.size() + _clients.size() == 0; }	/* Submodel is empty! */

    void addClient( Task * task ) { _clients.insert(task); }
    void addServer( Entity * server ) { _servers.insert(server); }
    bool hasClient( const Task * client ) const { return _clients.find(const_cast<Task *>(client)) != _clients.end(); }
    bool hasServer( const Entity * server ) const { return _servers.find(const_cast<Entity *>(server)) != _servers.end(); }

    const std::set<Task *>& getClients() const { return _clients; }		/* Table of clients 		*/
    virtual const char * const submodelType() const = 0;
    unsigned number() const { return _submodel_number; }

    virtual Vector<double> * getOverlapFactor() const { return nullptr; }

    Submodel& addClients();
    void initializeSubmodel();
    void reinitializeSubmodel();
    virtual void initializeInterlock() {}
    virtual Submodel& build() { return *this; }
    virtual Submodel& rebuild() { return *this; }
    virtual Submodel& partition();


#if PAN_REPLICATION
    virtual double nrFactor( const Server *, const unsigned, const unsigned ) const { return 0; }
#endif

    virtual unsigned nClosedStns() const { return 0; }
    virtual unsigned nCopenStns() const { return 0; }

    virtual Submodel& solve( long, MVACount&, const double ) = 0;

    virtual std::ostream& print( std::ostream& ) const = 0;

    void debug_stop( const unsigned long, const double ) const;

protected:
    void setNChains( unsigned int n ) { _n_chains = n; }
    SubmodelManip print_submodel_header( const Submodel& aSubModel, const unsigned long iterations  ) { return SubmodelManip( &Submodel::submodel_header_str, aSubModel, iterations ); }
    SubmodelTraceManip print_trace_header( const std::string& str ) { return SubmodelTraceManip( &Submodel::submodel_trace_header_str, str ); }

private:
    void initializeWait( Task * ) const;

    void addToGroup( Task *, submodel_group_t& group ) const;
    bool replicaGroups( const std::set<Task *>&, const std::set<Task *>& ) const;
		
    static std::ostream& submodel_header_str( std::ostream& output, const Submodel& aSubmodel, const unsigned long iterations );
    static std::ostream& submodel_trace_header_str( std::ostream& output, const std::string& );

protected:
    std::set<Task *> _clients;		/* Table of clients 		*/
    std::set<Entity *> _servers;	/* Table of servers 		*/

private:
    unsigned _submodel_number;		/* Submodel number.  Set once.	*/
    unsigned _n_chains;			/* Number of chains K		*/
	
private:
    /* MVA Stuff */

};

inline std::ostream& operator<<( std::ostream& output, const Submodel& self) { return self.print( output ); }

/* ---------------------- Standard MVA Submodel ----------------------- */


class MVASubmodel : public Submodel {
    friend class Generate;

    struct InitializeClientStation {
	InitializeClientStation( MVASubmodel& submodel ) : _submodel(submodel) {}
	void operator()( Task* task );
    private:
	MVASubmodel& _submodel;
    };

#if PAN_REPLICATION
    struct ModifyClientServiceTime {
	ModifyClientServiceTime( MVASubmodel& submodel ) : _submodel(submodel) {}
	void operator()( Task* task );
    private:
	MVASubmodel& _submodel;
    };
#endif

    struct InitializeServerStation {
	InitializeServerStation( MVASubmodel& submodel ) : _submodel(submodel) {}
	void operator()( Entity* entity );
    private:
	struct ComputeOvertaking  {
	    ComputeOvertaking( Entity * server ) : _server(server) {}
	    void operator()( Task * client );
	private:
	    Entity * _server;
	};

	void setServiceTimeAndVariance( Server * station, const Entry * entry, unsigned k ) const;
    private:
	MVASubmodel& _submodel;
    };

    class SaveClientResults {
    public:
	SaveClientResults( const MVASubmodel& submodel ) : _submodel(submodel) {}
	void operator()( Task * client ) const;
    private:
	const MVASubmodel& _submodel;
    };

    class SaveServerResults {
    public:
	SaveServerResults( const MVASubmodel& submodel , double relaxation ) : _submodel(submodel), _relaxation(relaxation) {}
	void operator()( Entity * entity ) const;
    private:
	const MVASubmodel& _submodel;
	const double _relaxation;
    };

    struct PrintServer {
	PrintServer( std::ostream& output, bool (Entity::*predicate)() const ) : _output(output), _predicate(predicate) {}
	void operator()( const Entity * ) const;
    private:
	std::ostream& _output;
	bool (Entity::*_predicate)() const;
    };


public:
    MVASubmodel( const unsigned );
    virtual ~MVASubmodel();
	
    const char * const submodelType() const { return "Submodel"; }

    virtual void initializeInterlock();
    virtual MVASubmodel& build();
    virtual MVASubmodel& rebuild();
		
    unsigned customers( const unsigned k ) const { return _customers[k]; }
    double thinkTime( const unsigned k ) const { return _thinkTime[k]; }
    void setThinkTime( unsigned int k, double thinkTime ) { _thinkTime[k] = thinkTime; }
    unsigned priority( const unsigned k ) const { return _priority[k]; }

    unsigned nChains() const { return _customers.size(); }
    virtual unsigned nClosedStns() const { return _closedStation.size(); }
    virtual unsigned nOpenStns() const { return _openStation.size(); }
    virtual Vector<double> * getOverlapFactor() const { return _overlapFactor; }

#if PAN_REPLICATION
    virtual double nrFactor( const Server *, const unsigned e, const unsigned k ) const;
#endif

    virtual MVASubmodel& solve( long, MVACount&, const double );
	
    double openModelThroughput( const Server& station, unsigned int e ) const;
    double closedModelThroughput( const Server& station, unsigned int e ) const;
    double closedModelThroughput( const Server& station, unsigned int e, const unsigned int k ) const;
#if PAN_REPLICATION
    double closedModelNormalizedThroughput( const Server& station, unsigned int e, const unsigned int k ) const;
#endif
    double closedModelUtilization( const Server& station ) const;
    double closedModelUtilization( const Server& station, unsigned int k ) const;		// CFS
    double openModelUtilization( const Server& station ) const;
#if BUG_393
    double closedModelMarginalQueueProbability( const Server& station, unsigned int i ) const;
#endif

    virtual std::ostream& print( std::ostream& ) const;

private:
    bool hasThreads() const { return _hasThreads; }
    bool hasSynchs() const { return _hasSynchs; }
    bool hasReplicas() const { return _hasReplicas; }

    void initializeChains( Task* client ) const;

public:
#if PAN_REPLICATION
    bool usePanReplication() const;
#endif
    void setChains( const ChainVector& chain ) const;

protected:
    unsigned makeChains();
    unsigned remakeChains();
    void saveWait( Entry *, const Server * );

    std::ostream& printClosedModel( std::ostream& ) const;
    std::ostream& printOpenModel( std::ostream& ) const;

private:
    bool _hasThreads;			/* True if client has forks.	*/
    bool _hasSynchs;			/* True if server has joins.	*/
    bool _hasReplicas;			/* True is submodel has replica	*/

    /* MVA Stuff */
	
    Population _customers;		/* Customers by chain k		*/
    Vector<double> _thinkTime;		/* Think time for chain k	*/
    Vector<unsigned int> _priority;	/* Priority for chain k.	*/

    Vector<Server *> _closedStation;
    Vector<Server *> _openStation;
    MVA * _closedModel;
    Open * _openModel;

    /* Fork-Join stuff. */
	
    Vector<double> * _overlapFactor;
};
#endif
