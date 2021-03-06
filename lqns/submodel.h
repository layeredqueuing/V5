/* -*- c++ -*-
 * submodel.h	-- Greg Franks
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * June 2007
 *
 * $Id: submodel.h 14863 2021-06-26 01:36:42Z greg $
 */

#ifndef _SUBMODEL_H
#define _SUBMODEL_H


#include <set>
#include <cstdlib>
#include <lqio/input.h>
#include <mva/vector.h>
#include <mva/pop.h>

class Call;
class Entry;
class Entity;
class Group;
class MVA;
class MVACount;
class Model;
class Open;
class Processor;
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

    /*
     * Remove all tasks/entites 'y' from either _clients/_servers 'x'.  Mark 'y'
     * as pruned.
     */
    
    template <class Type> struct erase_from {
	erase_from<Type>( std::set<Type>& x ) : _x(x) {}
	void operator()( Type y ) { _x.erase(y); y->setPruned(true); }
    private:
	std::set<Type>& _x;
    };

public:
    Submodel( const unsigned n ) : _submodel_number(n), _n_chains(0) {}
    virtual ~Submodel() {}

private:
    Submodel( const Submodel& ) { abort(); }		/* Copying is verbotten */
    Submodel& operator=( const Submodel& ) { abort(); return *this; }

public:
    int operator==( const Submodel& aSubmodel ) const { return &aSubmodel == this; }
    int operator!() const { return _servers.size() + _clients.size() == 0; }	/* Submodel is empty! */

    void addClient( Task * aTask ) { _clients.insert(aTask); }
    void addServer( Entity * anEntity ) { _servers.insert(anEntity); }

    const std::set<Task *>& getClients() const { return _clients; }			/* Table of clients 		*/
    virtual const char * const submodelType() const = 0;
    unsigned number() const { return _submodel_number; }
    Submodel& setSubmodelNumber( const unsigned );

    virtual Vector<double> * getOverlapFactor() const { return nullptr; } 
    unsigned nChains() const { return _n_chains; }
    unsigned customers( const unsigned i ) const { return _customers[i]; }
    double thinkTime( const unsigned i ) const { return _thinkTime[i]; }
    void setThinkTime( unsigned int i, double thinkTime ) { _thinkTime[i] = thinkTime; }
    unsigned priority( const unsigned i ) const { return _priority[i]; }

    Submodel& addClients();
    virtual Submodel& initServers( const Model& ) { return *this; }
    virtual Submodel& reinitServers( const Model& ) { return *this; }
    virtual Submodel& initInterlock() { return *this; }
    virtual Submodel& build() { return *this; }
    virtual Submodel& rebuild() { return *this; }
    virtual Submodel& optimize();


#if PAN_REPLICATION
    virtual double nrFactor( const Server *, const unsigned, const unsigned ) const { return 0; }
#endif

    virtual unsigned n_closedStns() const { return 0; }
    virtual unsigned n_openStns() const { return 0; }

    virtual Submodel& solve( long, MVACount&, const double ) = 0;

    virtual std::ostream& print( std::ostream& ) const = 0;

    void debug_stop( const unsigned long, const double ) const;

protected:
    void setNChains( unsigned int n ) { _n_chains = n; }
    SubmodelManip print_submodel_header( const Submodel& aSubModel, const unsigned long iterations  ) { return SubmodelManip( &Submodel::submodel_header_str, aSubModel, iterations ); }

private:
    void addToGroup( Task *, submodel_group_t& group ) const;
    bool replicaGroups( const std::set<Task *>&, const std::set<Task *>& ) const;
		     
    static std::ostream& submodel_header_str( std::ostream& output, const Submodel& aSubmodel, const unsigned long iterations );

protected:
    std::set<Task *> _clients;		/* Table of clients 		*/
    std::set<Entity *> _servers;	/* Table of servers 		*/

private:
    unsigned _submodel_number;		/* Submodel number.  Set once.	*/
    unsigned _n_chains;			/* Number of chains K		*/
	
protected:
    /* MVA Stuff */

    Population _customers;		/* Customers by chain k		*/
    Vector<double> _thinkTime;		/* Think time for chain k	*/
    Vector<unsigned> _priority;		/* Priority for chain k.	*/
};

inline std::ostream& operator<<( std::ostream& output, const Submodel& self) { return self.print( output ); }

/* ---------------------- Standard MVA Submodel ----------------------- */


class MVASubmodel : public Submodel {
    friend class Generate;

public:
    MVASubmodel( const unsigned );
    virtual ~MVASubmodel();
	
    const char * const submodelType() const { return "Submodel"; }

    virtual MVASubmodel& initServers( const Model& );
    virtual MVASubmodel& reinitServers( const Model& );
    virtual MVASubmodel& initInterlock();
    virtual MVASubmodel& build();
    virtual MVASubmodel& rebuild();
		
    virtual unsigned n_closedStns() const { return _closedStation.size(); }
    virtual unsigned n_openStns() const { return _openStation.size(); }
    virtual Vector<double> * getOverlapFactor() const { return _overlapFactor; } 

    bool hasClosedModel() const { return _closedModel != nullptr; }
    bool hasOpenModel() const { return _openModel != nullptr; }
    
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
    double closedModelUtilization( const Server& station, unsigned int k ) const;
    double openModelUtilization( const Server& station ) const;

    virtual std::ostream& print( std::ostream& ) const;
    
private:
    bool hasThreads() const { return _hasThreads; }
    bool hasSynchs() const { return _hasSynchs; }
    bool hasReplicas() const { return _hasReplicas; }

public:
#if PAN_REPLICATION
    bool usePanReplication() const;
#endif
    void setChains( const ChainVector& chain );

protected:
    unsigned makeChains();
    void saveWait( Entry *, const Server * );

    std::ostream& printClosedModel( std::ostream& ) const;
    std::ostream& printOpenModel( std::ostream& ) const;

private:
    bool _hasThreads;			/* True if client has forks.	*/
    bool _hasSynchs;			/* True if server has joins.	*/
    bool _hasReplicas;			/* True is submodel has replica	*/

    /* MVA Stuff */
	
    Vector<Server *> _closedStation;
    Vector<Server *> _openStation;
    MVA * _closedModel;
    Open * _openModel;

    /* Fork-Join stuff. */
	
    Vector<double> * _overlapFactor;
};

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

SubModelManip print_submodel_header( const Submodel &, const unsigned long );
#endif
