/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/server.h $
 *
 * Servers for MVA solver.  Subclass as needed.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: server.h 14140 2020-11-25 20:24:15Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(SERVER_H)
#define	SERVER_H

#include "vector.h"
#include "pop.h"
#include "prob.h"
#include "dim.h"

class MVA;
class Server;
class Chain;

std::ostream& operator<<( std::ostream&, const Server& );

/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Y, U, L, W and s are all variables that are defined in the Chandy
 * Neuse paper.  Dimensions are [entry][class][phase] on all arrays.
 * Dimension phase == 0 is total over all phases for object.  All arrays
 * are auto-dimensioned by constructors.
 */

class Server
{
public:
    /* Initialization */

    Server() : openIndex(0), closedIndex(0), E(1), K(1), P(1) { initialize(); }
    Server( const unsigned k ) : openIndex(0), closedIndex(0), E(1), K(k), P(1) { initialize(); }
    Server( const unsigned e, const unsigned k ) : openIndex(0), closedIndex(0), E(e), K(k), P(1) { initialize(); }
    Server( const unsigned e, const unsigned k, const unsigned p ) : openIndex(0), closedIndex(0), E(e), K(k), P(p) { initialize(); }
    virtual ~Server();

private:
    Server( const Server& );
    Server& operator=( const Server& );

public:
    virtual void initStep( const MVA& );
    virtual void clear();

    /* Instance Variable Access */

    Server& setService( const unsigned e, const unsigned k, const unsigned p, const double s );
    Server& setService( const unsigned e, const unsigned k, const double _s )  { return setService( e, k, 1, _s ); }
    Server& setService( const unsigned k, const double _s ) { return setService( 1, k, 1, _s ); }
    Server& setVisits( const unsigned e, const unsigned k, const unsigned p, const double v );
    Server& setVisits( const unsigned e, const unsigned k, const double _v ) { return setVisits( e, k, 1, _v ); }
    Server& setVisits( const unsigned k, const double _v ) { return setVisits( 1, k, 1, _v ); }
    Server& addVisits( const unsigned e, const unsigned k, const unsigned p, const double );
    virtual Server& setVariance( const unsigned, const unsigned, const unsigned, const double );
    virtual Server& setClientChain( const unsigned e, const unsigned k );

    virtual Probability *** getPrOt( const unsigned ) const;
    Server& setInterlock( const unsigned e, const unsigned k, const Probability& prFlow );

    /* Queries */

    unsigned nEntries() const { return E; }			/* Number of entries 	*/
    virtual double mu() const { return 1.0; }			/* Capacity function.	*/
    virtual double mu( const unsigned ) const { return 1.0; }	/* Capacity function.	*/
    virtual short updateD() const { return 1; }			/* For synch server.  	*/
    virtual bool hasTau() const { return false; }

    double S() const;
    double S( const MVA& solver, const Population& ) const;
    double S( const unsigned k ) const;
    double S( const unsigned e, const unsigned k ) const { return s[e][k][0]; }
    double S( const unsigned e, const unsigned k, const unsigned p ) const { return s[e][k][p]; }
    double S_2() const;						/* Phase 2 service time	*/
    double S_2( const unsigned k ) const;
    double S_2( const unsigned e, const unsigned k ) const { return s[e][k][0] - s[e][k][1]; }
    double eta( const unsigned e, const unsigned k ) const;
    double etaS( const unsigned e, const unsigned k ) const;
    virtual double muS( const Population&, const unsigned ) const;	/* Schmidt multiserver */
    double V() const;
    double V( const unsigned k ) const { return v[0][k][0]; }
    double V( const unsigned e, const unsigned k ) const { return v[e][k][0]; }
    double V( const unsigned e, const unsigned k, const unsigned p ) const { return v[e][k][p]; }

    double R() const;
    double R( const unsigned k ) const;
    double R( const unsigned e, const unsigned k ) const;
    double R( const unsigned e, const unsigned k, const unsigned p ) const;

    virtual double r( const unsigned e, const unsigned k, const unsigned p=0 ) const { return S(e,k,p); }
	
    virtual double prOt( const unsigned, const unsigned, const unsigned ) const { return 0.0; }

    virtual unsigned int marginalProbabilitiesSize() const { return 0; }
    virtual void setMarginalProbabilitiesSize( const Population& ) { return; }
    virtual int vectorProbabilities() const { return 0; }
    virtual int infiniteServer() const { return 0; }
    virtual int priorityServer() const { return 0; }

    virtual const char * typeStr() const = 0;

    /* Computation -- closed */

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const = 0;
    double interlock( const unsigned e, const unsigned k, const double lambda ) const { return ( 1.0 - IL[e][k] ) * lambda; }

    /* Computation -- open */
	
    virtual double alpha( const unsigned n ) const;
    Server& operator*=( const double );
    Server& operator/=( const double );
    Server& operator=( const double );
    virtual void mixedWait( const MVA& solver, const Population& N ) const;
    virtual void openWait() const = 0;
    virtual double Rho() const;							// 3.284
	
    /* Printing */

    virtual std::ostream& print( std::ostream& output = std::cout ) const;
    virtual std::ostream& printHeading( std::ostream& output = std::cout ) const { return output; }
    virtual std::ostream& printOutput( std::ostream& output, const unsigned = 0 ) const { return output; }

protected:
    void setAndTotal( double *item, const unsigned phase, const double value );
    void totalVisits( const unsigned e, const unsigned k );

    Probability rho() const;
    double priorityInflation( const MVA&, const Population &, const unsigned ) const;

    virtual std::ostream& printInput( std::ostream&, const unsigned, const unsigned ) const;

private:
    void initialize();

public:
    double ***W;		/* Waiting time per visit.	*/
    unsigned openIndex;		/* Not used locally.		*/
    unsigned closedIndex;	/* Not used locally.		*/

protected:
    const unsigned E;		/* Number of entries.		*/
    const unsigned K;		/* Number of chains.		*/
    const unsigned P;		/* Number of phases.		*/

private:
    double ***s;		/* Service Time per phase.	*/
    double ***v;		/* Visit ratios per phase.	*/
    Probability ** IL;		/* Interlocking probability.	*/
};



/* --------------------- Generic Infinite Server ---------------------- */

class Infinite_Server : public Server {
public:
    Infinite_Server( const unsigned k ) : Server(k) {}
    Infinite_Server( const unsigned e, const unsigned k ) : Server(e,k) {}
    Infinite_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p) {}
    virtual ~Infinite_Server() {}

    virtual double mu() const;					/* Capacity function.	*/
    virtual double mu( const unsigned ) const { return mu(); }	/* Capacity function.	*/

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& , const Population& ) const;
    virtual void openWait() const;
	
    virtual double alpha( const unsigned ) const;

    virtual int infiniteServer() const { return 1; }

    virtual const char * typeStr() const { return "Infinite_Server"; }
};



/* ---------------------- Generic Delay Server ------------------------ */

class Client : public Infinite_Server {
public:
    Client( const unsigned k ) : Infinite_Server(k) {}
    Client( const unsigned e, const unsigned k ) : Infinite_Server(e,k) {}
    Client( const unsigned e, const unsigned k, const unsigned p ) : Infinite_Server(e,k,p) {}
    virtual ~Client() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual const char * typeStr() const { return "Client"; }
};



/* -------------------- Processor Sharing Server ---------------------- */

class PS_Server : public virtual Server {
public:
    PS_Server( ) : Server() {}
    PS_Server( const unsigned k ) : Server(k) {}
    PS_Server( const unsigned e, const unsigned k ) : Server(e,k) {}
    PS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p) {}
    virtual ~PS_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const;

    virtual const char * typeStr() const { return "PS_Server"; }
};



/* ------------------- PS Server with Priorities. ------------------- */

class PR_PS_Server : public virtual Server, public PS_Server {
public:
    PR_PS_Server( ) : Server(), PS_Server() {}
    PR_PS_Server( const unsigned k ) : Server(k), PS_Server(k) {}
    PR_PS_Server( const unsigned e, const unsigned k ) : Server(e,k), PS_Server(e,k) {}
    PR_PS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), PS_Server(e,k,p) {}
    virtual ~PR_PS_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_PS_Server"; }
};


/* ------------------- PS Server with priorities. ------------------- */

class HOL_PS_Server : public virtual Server, public PS_Server {
public:
    HOL_PS_Server( ) : Server(), PS_Server() {}
    HOL_PS_Server( const unsigned k ) : Server(k), PS_Server(k) {}
    HOL_PS_Server( const unsigned e, const unsigned k ) : Server(e,k), PS_Server(e,k) {}
    HOL_PS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), PS_Server(e,k,p) {}
    virtual ~HOL_PS_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_PS_Server"; }
};


/* ---------------------- Generic FIFO Server ------------------------- */

class FCFS_Server : public virtual Server {
public:
    FCFS_Server( ) : Server() {}
    FCFS_Server( const unsigned k ) : Server(k) {}
    FCFS_Server( const unsigned e, const unsigned k ) : Server(e,k) {}
    FCFS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p) {}
    virtual ~FCFS_Server() {}

    virtual bool hasTau() const { return true; }

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const;

    virtual const char * typeStr() const { return "FCFS_Server"; }
};



/* ------------------- FCFS Server with Priorities. ------------------- */

class PR_FCFS_Server : public virtual Server, public FCFS_Server {
public:
    PR_FCFS_Server( ) : Server(), FCFS_Server() {}
    PR_FCFS_Server( const unsigned k ) : Server(k), FCFS_Server(k) {}
    PR_FCFS_Server( const unsigned e, const unsigned k ) : Server(e,k), FCFS_Server(e,k) {}
    PR_FCFS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), FCFS_Server(e,k,p) {}
    virtual ~PR_FCFS_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_FCFS_Server"; }
};


/* ------------------- FCFS Server with priorities. ------------------- */

class HOL_FCFS_Server : public virtual Server, public FCFS_Server {
public:
    HOL_FCFS_Server( ) : Server(), FCFS_Server() {}
    HOL_FCFS_Server( const unsigned k ) : Server(k), FCFS_Server(k) {}
    HOL_FCFS_Server( const unsigned e, const unsigned k ) : Server(e,k), FCFS_Server(e,k) {}
    HOL_FCFS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), FCFS_Server(e,k,p) {}
    virtual ~HOL_FCFS_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_FCFS_Server"; }
};


/* ------------------- High Variation FIFO Server --------------------- */

class HVFCFS_Server : public virtual Server, public FCFS_Server {
public:
    HVFCFS_Server() : Server(), FCFS_Server() { initialize(); }
    HVFCFS_Server( const unsigned k ) : Server(k), FCFS_Server(k) { initialize(); }
    HVFCFS_Server( const unsigned e, const unsigned k ) : Server(e,k), FCFS_Server(e,k) { initialize(); }
    HVFCFS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), FCFS_Server(e,k,p) { initialize(); }
    virtual ~HVFCFS_Server();
    virtual void clear();

    virtual Server& setVariance( const unsigned, const unsigned, const unsigned, const double );

    virtual double r( const unsigned, const unsigned, const unsigned=0 ) const;

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const;

    virtual const char * typeStr() const { return "HVFCFS_Server"; }

protected:
    virtual std::ostream& printInput( std::ostream&, const unsigned, const unsigned ) const;

private:
    void initialize();
    Positive MG1( const unsigned ) const;

private:
    double *** myVariance;
};




/* -------------- HOL Priority High Variation FIFO Server ------------- */

class PR_HVFCFS_Server : public virtual Server, public HVFCFS_Server {
public:
    PR_HVFCFS_Server() : Server(), HVFCFS_Server(1,1) {}
    PR_HVFCFS_Server( const unsigned k ) : Server(k), HVFCFS_Server(k) {}
    PR_HVFCFS_Server( const unsigned e, const unsigned k ) : Server(e,k), HVFCFS_Server(e,k) {}
    PR_HVFCFS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Server(e,k,p) {}
    virtual ~PR_HVFCFS_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_HVFCFS_Server"; }

};



/* -------------- HOL Priority High Variation FIFO Server ------------- */

class HOL_HVFCFS_Server : public virtual Server, public HVFCFS_Server {
public:
    HOL_HVFCFS_Server() : Server(), HVFCFS_Server(1,1) {}
    HOL_HVFCFS_Server( const unsigned k ) : Server(k), HVFCFS_Server(k) {}
    HOL_HVFCFS_Server( const unsigned e, const unsigned k ) : Server(e,k), HVFCFS_Server(e,k) {}
    HOL_HVFCFS_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Server(e,k,p) {}
    virtual ~HOL_HVFCFS_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_HVFCFS_Server"; }

};
#endif
