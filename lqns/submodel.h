/* -*- c++ -*-
 * submodel.h	-- Greg Franks
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * June 2007
 *
 * $Id: submodel.h 13413 2018-10-23 15:03:40Z greg $
 */

#ifndef _SUBMODEL_H
#define _SUBMODEL_H


#include "dim.h"
#include <cstdlib>
#include <lqio/input.h>
#include "cltn.h"
#include "vector.h"
#include "pop.h"

class Call;
class Entity;
class Entry;
class Model;
class MVA;
class MVACount;
class Open;
class Server;
class SubModelManip;
class Submodel;
class Task;


ostream& operator<<( ostream&, const Submodel& );


/* ------- Submodel Abstract Superclass.  Subclassed as needed. ------- */
	 
class Submodel {
    friend SubModelManip print_submodel_header( const Submodel & aSubModel, const unsigned long iterations  );

public:
    Submodel( const unsigned n, const Model * anOwner ) : _submodel_number(n), myOwner(anOwner), _n_chains(0) {}
    virtual ~Submodel() {}

private:
    Submodel( const Submodel& ) { abort(); }		/* Copying is verbotten */
    Submodel& operator=( const Submodel& ) { abort(); return *this; }

public:
    int operator==( const Submodel& aSubmodel ) const { return &aSubmodel == this; }
    int operator!() const { return servers.size() + clients.size() == 0; }	/* Submodel is empty! */

    void addClient( Task * aTask ) { clients += aTask; }
    void addServer( Entity * anEntity ) { servers += anEntity; }

    unsigned number() const { return _submodel_number; }
    Submodel& number( const unsigned );
    const Model * owner() const { return myOwner; }

    unsigned nChains() const { return _n_chains; }
    unsigned customers( const unsigned i ) const { return myCustomers[i]; }
    double thinkTime( const unsigned i ) const { return myThinkTime[i]; }
    unsigned priority( const unsigned i) const { return myPriority[i]; }

    virtual void initServers( const Model& );
    virtual void reinitServers( const Model& ) {}
    virtual void reinitClients() {}
    virtual void initWait( Entity * ) const;
    virtual void initInterlock() {}
    virtual void reinitInterlock() {}
    virtual void build() {}
    virtual void rebuild() {}
		
    virtual double nrFactor( const Server *, const unsigned, const unsigned ) const { return 0; }

    virtual unsigned n_closedStns() const { return 0; }
    virtual unsigned n_openStns() const { return 0; }

    virtual Submodel& solve( long, MVACount&, const double ) = 0;
	
    virtual ostream& print( ostream& ) const = 0;

    void debug_stop( const unsigned long, const double ) const;

protected:
    void setNChains( unsigned int n ) { _n_chains = n; }

private:
    static ostream& submodel_header_str( ostream& output, const Submodel& aSubmodel, const unsigned long iterations );

protected:
    Cltn<Task *> clients;		/* Table of clients 		*/
    Cltn<Entity *> servers;		/* Table of servers 		*/

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


/* ---------------------- Standard MVA Submodel ----------------------- */


class MVASubmodel : public Submodel {
    friend class Generate;

public:
    MVASubmodel( const unsigned, const Model * );
    virtual ~MVASubmodel();
	
    virtual void initServers( const Model& );
    virtual void reinitServers( const Model& );
    virtual void reinitClients();
    virtual void initInterlock();
    virtual void reinitInterlock();
    virtual void build();
    virtual void rebuild();
		
    virtual unsigned n_closedStns() const { return closedStnNo; }
    virtual unsigned n_openStns() const { return openStnNo; }

    virtual double nrFactor( const Server *, const unsigned e, const unsigned k ) const;

    virtual MVASubmodel& solve( long, MVACount&, const double );
	
    virtual ostream& print( ostream& ) const;


private:
    unsigned makeChains();
    void initClient( Task * );
    void modifyClientServiceTime( Task * aTask );
    void initServer( Entity * );
    void setServiceTime( Entity *, const Entry *, unsigned ) const;
    void setInterlock( Entity * ) const;
    void generate() const;
	
    void saveClientResults( Task * );
    void saveServerResults( Entity * );
    void saveWait( Entry *, const Server * );
	void setThreadChain() const ;

    ostream& printClosedModel( ostream& ) const;
    ostream& printOpenModel( ostream& ) const;

private:
    bool hasReplication;
    bool hasThreads;			/* True if client has forks.	*/
    bool hasSynchs;			/* True if server has joins.	*/

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

class SubModelManip {
public:
    SubModelManip( ostream& (*ff)(ostream&, const Submodel&, const unsigned long ),
		   const Submodel& aSubmodel, const unsigned long anInt )
	: f(ff), submodel(aSubmodel), arg(anInt) {}
private:
    ostream& (*f)( ostream&, const Submodel&, const unsigned long );
    const Submodel & submodel;
    const unsigned long arg;

    friend ostream& operator<<(ostream & os, const SubModelManip& m ) { return m.f(os,m.submodel,m.arg); }
};

SubModelManip print_submodel_header( const Submodel &, const unsigned long );
#endif
