/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/ph2serv.h $
 *
 * Servers for MVA solver.  Subclass as needed.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: ph2serv.h 13676 2020-07-10 15:46:20Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PHASED_SERVER_H)
#define	PHASED_SERVER_H

#include "server.h"
#include "prob.h"
#include "dim.h"

typedef enum { SIMPLE_PHASE2, COMPLEX_PHASE2 } PHASE2_CORRECTION;

extern PHASE2_CORRECTION phase2_correction;

/* ---------------- Multi-phase, Single Entry Server ------------------ */

class Phased_Server : virtual public Server {
public:
    Phased_Server() : Server(1,1,MAX_PHASES) {}
    Phased_Server( const unsigned p ) : Server(1,1,p) {}
    Phased_Server( const unsigned k, const unsigned p ) : Server( 1,k,p ) {}
    Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server( e,k,p ) {}
    virtual ~Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const = 0;
    virtual void openWait() const;

protected:
    double residual( const unsigned e, const unsigned k, const unsigned p ) const;

    Positive MGplusG1() const;					/* Open model.	*/
};


/* ---------------- Multi-phase, Single Entry Server ------------------ */

class Rolia_Phased_Server : virtual public Server, public Phased_Server {
public:
    Rolia_Phased_Server() : Server(1,1,MAX_PHASES), Phased_Server(1,1,MAX_PHASES) { initialize(); }
    Rolia_Phased_Server( const unsigned p ) : Server(1,1,p), Phased_Server(1,1,p)  { initialize(); }
    Rolia_Phased_Server( const unsigned k, const unsigned p ) : Server( 1,k,p ), Phased_Server( 1,k,p ) { initialize(); }
    Rolia_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server( e,k,p ), Phased_Server( e,k,p ) { initialize(); }
    virtual ~Rolia_Phased_Server();

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual void initStep( const MVA& );
    virtual double prOt( const unsigned e, const unsigned k, const unsigned ) const { return Gamma[e][k]; }

    virtual ostream& printOutput( ostream& output, const unsigned = 0 ) const;

    virtual const char * typeStr() const { return "Rolia_Phased_Server"; }

protected:
    Positive overtaking( const unsigned k ) const;
	
private:
    virtual void initialize();

private:
    double **Gamma;
};



/* --------- Multi-phase, Single Entry Server with Priorities --------- */

class HOL_Rolia_Phased_Server : virtual public Server, public Rolia_Phased_Server {
public:
    HOL_Rolia_Phased_Server() : Server(1,1,MAX_PHASES), Rolia_Phased_Server(1,1,MAX_PHASES) {}
    HOL_Rolia_Phased_Server( const unsigned p ) : Server(1,1,p), Rolia_Phased_Server(1,1,p) {}
    HOL_Rolia_Phased_Server( const unsigned k, const unsigned p ) : Server( 1,k,p ), Rolia_Phased_Server(1,k,p) {}
    HOL_Rolia_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server( e,k,p ), Rolia_Phased_Server(e,k,p) {}
    virtual ~HOL_Rolia_Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_Rolia_Phased_Server"; }
};



/* --------- Multi-phase, Single Entry Server with Priorities --------- */

class PR_Rolia_Phased_Server : virtual public Server, public Rolia_Phased_Server {
public:
    PR_Rolia_Phased_Server() : Server(1,1,MAX_PHASES), Rolia_Phased_Server(1,1,MAX_PHASES) {}
    PR_Rolia_Phased_Server( const unsigned p ) : Server(1,1,p), Rolia_Phased_Server(1,1,p) {}
    PR_Rolia_Phased_Server( const unsigned k, const unsigned p ) : Server( 1,k,p ), Rolia_Phased_Server(1,k,p) {}
    PR_Rolia_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server( e,k,p ), Rolia_Phased_Server(e,k,p) {}
    virtual ~PR_Rolia_Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_Rolia_Phased_Server"; }
};



/* -------------- Multiple-phase, Multiple Entry Server --------------- */

class HVFCFS_Rolia_Phased_Server : virtual public Server, public HVFCFS_Server, public Rolia_Phased_Server {
public:
    HVFCFS_Rolia_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Server(1,1), Rolia_Phased_Server(1,1,MAX_PHASES) {}
    HVFCFS_Rolia_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Server(1,1), Rolia_Phased_Server(1,1,p) {}
    HVFCFS_Rolia_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Server(1,k), Rolia_Phased_Server(1,k,p) {}
    HVFCFS_Rolia_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Server(e,k), Rolia_Phased_Server(e,k,p) {}
    virtual ~HVFCFS_Rolia_Phased_Server() {}
	
    virtual Server& setVariance( const unsigned e, const unsigned k, const unsigned p, const double value ) { return HVFCFS_Server::setVariance(e,k,p,value); }
    virtual double r( const unsigned e, const unsigned k, const unsigned p ) const { return HVFCFS_Server::r(e,k,p); }
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const { HVFCFS_Server::openWait(); }

    virtual const char * typeStr() const { return "HVFCFS_Rolia_Phased_Server"; }

protected:
    virtual ostream& printInput( ostream& output, const unsigned e, const unsigned k ) const { return HVFCFS_Server::printInput( output, e, k ); }
};



/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

class HOL_HVFCFS_Rolia_Phased_Server : virtual public Server, public HVFCFS_Rolia_Phased_Server {
public:
    HOL_HVFCFS_Rolia_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Rolia_Phased_Server(1,1) {}
    HOL_HVFCFS_Rolia_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Rolia_Phased_Server(1,1) {}
    HOL_HVFCFS_Rolia_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Rolia_Phased_Server(1,k) {}
    HOL_HVFCFS_Rolia_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Rolia_Phased_Server(e,k) {}
    virtual ~HOL_HVFCFS_Rolia_Phased_Server() {}
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_HVFCFS_Rolia_Phased_Server"; }
};



/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

class PR_HVFCFS_Rolia_Phased_Server : virtual public Server, public HVFCFS_Rolia_Phased_Server {
public:
    PR_HVFCFS_Rolia_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Rolia_Phased_Server(1,1) {}
    PR_HVFCFS_Rolia_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Rolia_Phased_Server(1,1) {}
    PR_HVFCFS_Rolia_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Rolia_Phased_Server(1,k) {}
    PR_HVFCFS_Rolia_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Rolia_Phased_Server(e,k) {}
    virtual ~PR_HVFCFS_Rolia_Phased_Server() {}
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_HVFCFS_Rolia_Phased_Server"; }
};



/* ---------------- Multi-phase, Single Entry Server ------------------ */

class Simple_Phased_Server : virtual public Server, public Rolia_Phased_Server {
public:
    Simple_Phased_Server() : Server(1,1,MAX_PHASES), Rolia_Phased_Server(1,1,MAX_PHASES) {}
    Simple_Phased_Server( const unsigned p ) : Server(1,1,p), Rolia_Phased_Server(1,1,p) {}
    Simple_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), Rolia_Phased_Server( 1,k,p ) {}
    Simple_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), Rolia_Phased_Server(e,k,p) {}
    virtual ~Simple_Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual ostream& printOutput( ostream& output, const unsigned = 0 ) const;

    virtual const char * typeStr() const { return "Simple_Phased_Server"; }

protected:
    Positive sumOf_S2U( const MVA& solver, const Population& N, const unsigned k ) const;
};



/* --------- Multi-phase, Single Entry Server with Priorities --------- */

class HOL_Simple_Phased_Server : virtual public Server, public Simple_Phased_Server {
public:
    HOL_Simple_Phased_Server() : Server(1,1,MAX_PHASES), Simple_Phased_Server(1,1,MAX_PHASES) {}
    HOL_Simple_Phased_Server( const unsigned p ) : Server(1,1,p), Simple_Phased_Server(1,1,p) {}
    HOL_Simple_Phased_Server( const unsigned k, const unsigned p ) : Server( 1,k,p ), Simple_Phased_Server(1,k,p) {}
    HOL_Simple_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server( e,k,p ), Simple_Phased_Server(e,k,p) {}
    virtual ~HOL_Simple_Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_Simple_Phased_Server"; }
};



/* --------- Multi-phase, Single Entry Server with Priorities --------- */

class PR_Simple_Phased_Server : virtual public Server, public Simple_Phased_Server {
public:
    PR_Simple_Phased_Server() : Server(1,1,MAX_PHASES), Simple_Phased_Server(1,1,MAX_PHASES) {}
    PR_Simple_Phased_Server( const unsigned p ) : Server(1,1,p), Simple_Phased_Server(1,1,p) {}
    PR_Simple_Phased_Server( const unsigned k, const unsigned p ) : Server( 1,k,p ), Simple_Phased_Server(1,k,p) {}
    PR_Simple_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server( e,k,p ), Simple_Phased_Server(e,k,p) {}
    virtual ~PR_Simple_Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_Simple_Phased_Server"; }
};



/* -------------- Multiple-phase, Multiple Entry Server --------------- */

class HVFCFS_Simple_Phased_Server : virtual public Server, public HVFCFS_Server, virtual public Simple_Phased_Server {
public:
    HVFCFS_Simple_Phased_Server() : Server(1,1,MAX_PHASES), Simple_Phased_Server(1,1,MAX_PHASES), HVFCFS_Server(1,1) {}
    HVFCFS_Simple_Phased_Server( const unsigned p ) : Server(1,1,p), Simple_Phased_Server(1,1,p), HVFCFS_Server(1,1) {}
    HVFCFS_Simple_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), Simple_Phased_Server(1,k,p), HVFCFS_Server(1,k) {}
    HVFCFS_Simple_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), Simple_Phased_Server(e,k,p), HVFCFS_Server(e,k) {}
    virtual ~HVFCFS_Simple_Phased_Server() {}
	
    virtual Server& setVariance( const unsigned e, const unsigned k, const unsigned p, const double value ) { return HVFCFS_Server::setVariance(e,k,p,value); }
    virtual double r( const unsigned e, const unsigned k, const unsigned p ) const { return HVFCFS_Server::r( e, k, p ); }

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const { HVFCFS_Server::openWait(); }

    virtual const char * typeStr() const { return "HVFCFS_Simple_Phased_Server"; }

protected:
    virtual ostream& printInput( ostream& output, const unsigned e, const unsigned k ) const { return HVFCFS_Server::printInput( output, e, k ); } 
};



/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

class HOL_HVFCFS_Simple_Phased_Server : virtual public Server, public HVFCFS_Simple_Phased_Server {
public:
    HOL_HVFCFS_Simple_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Simple_Phased_Server(1,1) {}
    HOL_HVFCFS_Simple_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Simple_Phased_Server(1,1) {}
    HOL_HVFCFS_Simple_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Simple_Phased_Server(1,k) {}
    HOL_HVFCFS_Simple_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Simple_Phased_Server(e,k) {}
    virtual ~HOL_HVFCFS_Simple_Phased_Server() {}
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_HVFCFS_Simple_Phased_Server"; }
};

/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

class PR_HVFCFS_Simple_Phased_Server : virtual public Server, public HVFCFS_Simple_Phased_Server {
public:
    PR_HVFCFS_Simple_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Simple_Phased_Server(1,1) {}
    PR_HVFCFS_Simple_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Simple_Phased_Server(1,1) {}
    PR_HVFCFS_Simple_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Simple_Phased_Server(1,k) {}
    PR_HVFCFS_Simple_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Simple_Phased_Server(e,k) {}
    virtual ~PR_HVFCFS_Simple_Phased_Server() {}
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_HVFCFS_Simple_Phased_Server"; }
};

/* ------------------- Markov based Phased Server  -------------------- */

class Markov_Phased_Server : virtual public Server, public Phased_Server {

public:
    Markov_Phased_Server() : Server(1,1,MAX_PHASES), Phased_Server(1,1,MAX_PHASES) { initialize(); }
    Markov_Phased_Server( const unsigned p ) : Server(1,1,p), Phased_Server(1,1,p) { initialize(); }
    Markov_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), Phased_Server(1,k,p) { initialize(); }
    Markov_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), Phased_Server(e,k,p) { initialize(); }
    virtual ~Markov_Phased_Server();
    virtual void clear();

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual Probability *** getPrOt( const unsigned e ) const;
    virtual double prOt( const unsigned e, const unsigned k, const unsigned p ) const { return prOvertake[e][k][0][p]; }
    virtual const char * typeStr() const { return "Markov_Phased_Server"; }

protected:
    Positive overtaking( const unsigned k, const unsigned p_i ) const;
    Probability PrOT( const unsigned k, const unsigned p_i ) const;
    Positive sumOf_S2U( const MVA& solver, const unsigned p_i, const Population& N, const unsigned k ) const;

    virtual ostream& printInput( ostream&, const unsigned, const unsigned ) const;
    ostream& printOvertaking( ostream&, const unsigned, const unsigned ) const;

private:
    void initialize();

protected:
    Probability ****prOvertake;

};


/* ------------ Markov based Phased Server with Priorities ------------ */

class HOL_Markov_Phased_Server : virtual public Server, public Markov_Phased_Server {

public:
    HOL_Markov_Phased_Server() : Server(1,1,MAX_PHASES), Markov_Phased_Server(1,1,MAX_PHASES)  {}
    HOL_Markov_Phased_Server( const unsigned p ) : Server(1,1,p), Markov_Phased_Server(1,1,p) {}
    HOL_Markov_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), Markov_Phased_Server(1,k,p) {}
    HOL_Markov_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), Markov_Phased_Server(e,k,p) {}
    virtual ~HOL_Markov_Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_Markov_Phased_Server"; }
};


/* ------------ Markov based Phased Server with Priorities ------------ */

class PR_Markov_Phased_Server : virtual public Server, public Markov_Phased_Server {

public:
    PR_Markov_Phased_Server() : Server(1,1,MAX_PHASES), Markov_Phased_Server(1,1,MAX_PHASES)  {}
    PR_Markov_Phased_Server( const unsigned p ) : Server(1,1,p), Markov_Phased_Server(1,1,p) {}
    PR_Markov_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), Markov_Phased_Server(1,k,p) {}
    PR_Markov_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), Markov_Phased_Server(e,k,p) {}
    virtual ~PR_Markov_Phased_Server() {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_Markov_Phased_Server"; }
};


/* -------------- Multiple-phase, Multiple Entry Server --------------- */

class HVFCFS_Markov_Phased_Server : virtual public Server, public HVFCFS_Server, public Markov_Phased_Server {
public:
    HVFCFS_Markov_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Server(1,1), Markov_Phased_Server(1,1,MAX_PHASES) {}
    HVFCFS_Markov_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Server(1,1), Markov_Phased_Server(1,1,p) {}
    HVFCFS_Markov_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Server(1,k), Markov_Phased_Server(1,1,p) {}
    HVFCFS_Markov_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Server(e,k), Markov_Phased_Server(e,k,p) {}
    virtual ~HVFCFS_Markov_Phased_Server() {}
    virtual void clear();

	
    virtual Server& setVariance( const unsigned e, const unsigned k, const unsigned p, const double value ) { return HVFCFS_Server::setVariance(e,k,p,value); }
    virtual double r( const unsigned e, const unsigned k, const unsigned p ) const { return HVFCFS_Server::r( e, k, p ); }

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void openWait() const { HVFCFS_Server::openWait(); }
	
    virtual const char * typeStr() const { return "HVFCFS_Markov_Phased_Server"; }

protected:
    virtual ostream& printInput( ostream&, const unsigned, const unsigned ) const;
};


/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

class HOL_HVFCFS_Markov_Phased_Server : virtual public Server, public HVFCFS_Markov_Phased_Server {
public:
    HOL_HVFCFS_Markov_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Markov_Phased_Server(1,1,MAX_PHASES) {}
    HOL_HVFCFS_Markov_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Markov_Phased_Server(1,1,p) {}
    HOL_HVFCFS_Markov_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Markov_Phased_Server(1,1,p) {}
    HOL_HVFCFS_Markov_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Markov_Phased_Server(e,k,p) {}
    virtual ~HOL_HVFCFS_Markov_Phased_Server() {}
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_HVFCFS_Markov_Phased_Server"; }
};

/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

class PR_HVFCFS_Markov_Phased_Server : virtual public Server, public HVFCFS_Markov_Phased_Server {
public:
    PR_HVFCFS_Markov_Phased_Server() : Server(1,1,MAX_PHASES), HVFCFS_Markov_Phased_Server(1,1,MAX_PHASES) {}
    PR_HVFCFS_Markov_Phased_Server( const unsigned p ) : Server(1,1,p), HVFCFS_Markov_Phased_Server(1,1,p) {}
    PR_HVFCFS_Markov_Phased_Server( const unsigned k, const unsigned p ) : Server(1,k,p), HVFCFS_Markov_Phased_Server(1,1,p) {}
    PR_HVFCFS_Markov_Phased_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p), HVFCFS_Markov_Phased_Server(e,k,p) {}
    virtual ~PR_HVFCFS_Markov_Phased_Server() {}
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "PR_HVFCFS_Markov_Phased_Server"; }
};
#endif
