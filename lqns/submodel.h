/* -*- c++ -*-
 * submodel.h	-- Greg Franks
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * June 2007
 *
 * $Id: submodel.h 13676 2020-07-10 15:46:20Z greg $
 */

#ifndef _SUBMODEL_H
#define _SUBMODEL_H


#include "dim.h"
#include <set>
#include <cstdlib>
#include <lqio/input.h>
#include "vector.h"
#include "pop.h"
#include "task.h"

class Call;
class Processor;
class Entry;
class Model;
class MVA;
class MVACount;
class Open;
class Server;
class SubModelManip;
class Group;

/* ------- Submodel Abstract Superclass.  Subclassed as needed. ------- */
	 
class Submodel {
    class SubmodelManip {
    public:
	SubmodelManip( ostream& (*ff)(ostream&, const Submodel&, const unsigned long ),
		       const Submodel& aSubmodel, const unsigned long anInt )
	    : f(ff), submodel(aSubmodel), arg(anInt) {}
    private:
	ostream& (*f)( ostream&, const Submodel&, const unsigned long );
	const Submodel& submodel;
	const unsigned long arg;

	friend ostream& operator<<(ostream & os, const SubmodelManip& m ) { return m.f(os,m.submodel,m.arg); }
    };

public:
    Submodel( const unsigned n, const Model * anOwner ) : _submodel_number(n), myOwner(anOwner), _n_chains(0) {}
    virtual ~Submodel() {}

private:
    Submodel( const Submodel& ) { abort(); }		/* Copying is verbotten */
    Submodel& operator=( const Submodel& ) { abort(); return *this; }

public:
    int operator==( const Submodel& aSubmodel ) const { return &aSubmodel == this; }
    int operator!() const { return _servers.size() + _clients.size() == 0; }	/* Submodel is empty! */

    void addClient( Task * aTask ) { _clients.insert(aTask); }
    void addServer( Entity * anEntity ) { _servers.insert(anEntity); }

    virtual const char * const submodelType() const = 0;
    unsigned number() const { return _submodel_number; }
    Submodel& number( const unsigned );
    const Model * owner() const { return myOwner; }

    unsigned nChains() const { return _n_chains; }
    unsigned customers( const unsigned i ) const { return myCustomers[i]; }
    double thinkTime( const unsigned i ) const { return myThinkTime[i]; }
    unsigned priority( const unsigned i) const { return myPriority[i]; }

    virtual Submodel& initServers( const Model& );
    virtual Submodel& reinitServers( const Model& ) { return *this; }
    virtual Submodel& reinitClients() { return *this; }
    virtual Submodel& initInterlock() { return *this; }
    virtual Submodel& reinitInterlock() { return *this; }
    virtual Submodel& build() { return *this; }
    virtual Submodel& rebuild() { return *this; }

    virtual double nrFactor( const Server *, const unsigned, const unsigned ) const { return 0; }

    virtual unsigned n_closedStns() const { return 0; }
    virtual unsigned n_openStns() const { return 0; }

    virtual Submodel& solve( long, MVACount&, const double ) = 0;
	
    virtual ostream& print( ostream& ) const = 0;

    void debug_stop( const unsigned long, const double ) const;

protected:
    void setNChains( unsigned int n ) { _n_chains = n; }
    SubmodelManip print_submodel_header( const Submodel& aSubModel, const unsigned long iterations  ) { return SubmodelManip( &Submodel::submodel_header_str, aSubModel, iterations ); }

private:
    static ostream& submodel_header_str( ostream& output, const Submodel& aSubmodel, const unsigned long iterations );

protected:
    std::set<Task *> _clients;			/* Table of clients 		*/
    std::set<Entity *,Entity::LT> _servers;	/* Table of servers 		*/

private:
    unsigned _submodel_number;		/* Submodel number.  Set once.	*/
    const Model * myOwner;		/* Pointer to layerizer.	*/
    unsigned _n_chains;			/* Number of chains K		*/
	
protected:
    /* MVA Stuff */

    Population myCustomers;		/* Customers by chain k		*/
    VectorMath<double> myThinkTime;	/* Think time for chain k	*/
    Vector<unsigned> myPriority;	/* Priority for chain k.	*/
};

inline ostream& operator<<( ostream& output, const Submodel& self) { return self.print( output ); }

/* ---------------------- Standard MVA Submodel ----------------------- */


class MVASubmodel : public Submodel {
    friend class Generate;

public:
    MVASubmodel( const unsigned, const Model * );
    virtual ~MVASubmodel();
	
    const char * const submodelType() const { return "Submodel"; }

    virtual MVASubmodel& initServers( const Model& );
    virtual MVASubmodel& reinitServers( const Model& );
    virtual MVASubmodel& reinitClients();
    virtual MVASubmodel& initInterlock();
    virtual MVASubmodel& reinitInterlock();
    virtual MVASubmodel& build();
    virtual MVASubmodel& rebuild();
		
    virtual unsigned n_closedStns() const { return closedStnNo; }
    virtual unsigned n_openStns() const { return openStnNo; }

    virtual double nrFactor( const Server *, const unsigned e, const unsigned k ) const;

    virtual MVASubmodel& solve( long, MVACount&, const double );
	
    virtual ostream& print( ostream& ) const;

private:
    bool hasReplication() const { return _hasReplication; }
    bool hasThreads() const { return _hasThreads; }
    bool hasSynchs() const { return _hasSynchs; }

protected:
    unsigned makeChains();
    void initClient( Task * );
    void modifyClientServiceTime( Task * aTask );
    void initServer( Entity * );
    void setServiceTime( Entity *, const Entry *, unsigned ) const;
    void setInterlock( Entity * ) const;
    void generate() const;
    void saveClientResults( Task * );
    virtual void saveServerResults( Entity * );
    void saveWait( Entry *, const Server * );
    void setThreadChain() const;

    ostream& printClosedModel( ostream& ) const;
    ostream& printOpenModel( ostream& ) const;

private:
    bool _hasReplication;		/* True if replication present.	*/
    bool _hasThreads;			/* True if client has forks.	*/
    bool _hasSynchs;			/* True if server has joins.	*/

    /* MVA Stuff */
	
    unsigned closedStnNo;
    unsigned openStnNo;
    Vector<Server *> closedStation;
    Vector<Server *> openStation;
    MVA * closedModel;
    Open * openModel;

    /* Fork-Join stuff. */
	
    VectorMath<double> * overlapFactor;
};

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

SubModelManip print_submodel_header( const Submodel &, const unsigned long );
#endif
