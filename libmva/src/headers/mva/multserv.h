/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/libmva/src/headers/mva/multserv.h $
 *
 * Servers for MVA solver.  Subclass as needed.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * November, 2021
 *
 * $Id: multserv.h 15107 2021-11-17 16:45:05Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(MULTI_SERVER_H)
#define	MULTI_SERVER_H

#include "server.h"
#include "ph2serv.h"

/* -------------------------------------------------------------------- */
/* 		           Reiser (Product Form)			*/
/* -------------------------------------------------------------------- */

class Reiser_Multi_Server : public virtual Server {
public:
    Reiser_Multi_Server( const unsigned copies )
	: Server(), J(copies) { initialize(); }
    Reiser_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k), J(copies) { initialize(); }
    Reiser_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k), J(copies) { initialize(); }
    Reiser_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p), J(copies) { initialize(); }
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const;
    virtual void openWait() const;

    virtual double mu() const { return static_cast<double>(J); }
    virtual double mu( const unsigned n ) const { return static_cast<double>(std::min( n, J )); }
    virtual double alpha( const unsigned n ) const;

    virtual const char * typeStr() const { return "Reiser_Multi_Server"; }
    virtual std::ostream& printHeading( std::ostream& output = std::cout ) const;

    virtual unsigned int copies() const { return J; }
    virtual unsigned int marginalProbabilitiesSize() const { return J; }

protected:
    double sumOf_rho( const unsigned n ) const;
    virtual Positive sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const;

private:
    void initialize();
    double A() const;

private:
    const unsigned int J;				/* Number of servers	*/
};

/* --------------------- Multi Server with Phases ----------------------*/

class Phased_Reiser_Multi_Server : public virtual Server,
				   public Reiser_Multi_Server {
public:
    Phased_Reiser_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Reiser_Multi_Server(copies,1,1,MAX_PHASES) {}
    Phased_Reiser_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Reiser_Multi_Server(copies,1,1,p) {}
    Phased_Reiser_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Reiser_Multi_Server(copies,1,k,p) {}
    Phased_Reiser_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Phased_Reiser_Multi_Server"; }

protected:
    virtual Positive sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const;
};


/* ------------------ Multi Server with Markov Phases ----------------- */

class Markov_Phased_Reiser_Multi_Server : public virtual Server,
					  public Reiser_Multi_Server,
					  public Markov_Phased_Server {
public:
    Markov_Phased_Reiser_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Reiser_Multi_Server(copies,1,1,MAX_PHASES),
	  Markov_Phased_Server(1,1,MAX_PHASES) {}
    Markov_Phased_Reiser_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Reiser_Multi_Server(copies,1,1,p),
	  Markov_Phased_Server(1,1,p) {}
    Markov_Phased_Reiser_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Reiser_Multi_Server(copies,1,k,p),
	  Markov_Phased_Server(1,k,p) {}
    Markov_Phased_Reiser_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p),
	  Markov_Phased_Server(e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Markov_Phased_Reiser_Multi_Server"; }
};


/* ----------------- Processor Sharing Multi Server ------------------- */

class Reiser_PS_Multi_Server : public virtual Server,
			       public Reiser_Multi_Server {
public:
    Reiser_PS_Multi_Server( const unsigned copies )
	: Server(),
	  Reiser_Multi_Server(copies) {}
    Reiser_PS_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Reiser_Multi_Server(copies,k) {}
    Reiser_PS_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Reiser_Multi_Server(copies,e,k) {}
    Reiser_PS_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Reiser_PS_Multi_Server"; }

protected:
    virtual Positive sumOf_L( const MVA& solver, const Population& N, const unsigned k ) const;
};

/*----------------------------------------------------------------------*/
/*                         Conway Multi-Server				*/
/*----------------------------------------------------------------------*/

class Conway_Multi_Server : public virtual Server,
			    public Reiser_Multi_Server {

#if defined(TESTMVA)
    friend int main ( int argc, char * argv[] );
#endif
    
private:
    class B_Iterator : public Population::Iterator
    {
    public:
	B_Iterator( const Server& aServer, const Population& N, const unsigned k );

	virtual int operator()( Population& n );

    protected:
	virtual int step( Population& N, const unsigned i, const unsigned n );

    private:
	void initialize( const unsigned );

    protected:
	const unsigned J;			/* Max number.			*/
	const unsigned K;			/* Number of classes		*/
	unsigned index;			/* for iterator.		*/
    };

    class A_Iterator : public B_Iterator
    {
    public:
	A_Iterator( const Server& aServer, const unsigned i, const Population& N, const unsigned k ): B_Iterator(aServer,N,k), class_i(i) {}

	virtual int operator()( Population& n );

    private:
	const unsigned class_i;			/* index of class with at least 1 cust.	*/
    };

public:
    Conway_Multi_Server( const unsigned copies )
	: Server(),
	  Reiser_Multi_Server(copies) {}
    Conway_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Reiser_Multi_Server(copies,k) {}
    Conway_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Reiser_Multi_Server(copies,e,k) {}
    Conway_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p) {}
	
    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Conway_Multi_Server"; }

protected:
    double effectiveBacklog( const MVA& solver, const Population& N, const unsigned k ) const;
    double departureTime( const MVA& solver, const Population& N, const unsigned k ) const;

private:
    double sumOf_PS_k( const MVA& solver, const Population& N, const unsigned k, Population::Iterator& next ) const;
    double meanMinimumService( const Population& N ) const;
    double A( const MVA& solver, const Population& n, const Population& N, const unsigned k ) const;

#if	DEBUG_MVA
    std::ostream& printXE( std::ostream&, const unsigned int i, const Population& N, const unsigned int k, const double xe, const double q ) const;
    std::ostream& printXR( std::ostream&, const Population& N, const unsigned int k, const double xr, const double PB ) const;

public:
    static bool debug_XE;
#endif

};

/* ------------------- Phased Conway Multi Server   ------------------- */

/* --------------------- Phased Conway Multiserver -------------------- */

class Phased_Conway_Multi_Server : public virtual Server,
				   public Conway_Multi_Server {
public:
    Phased_Conway_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Conway_Multi_Server(copies,1,1,MAX_PHASES) {}
    Phased_Conway_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Conway_Multi_Server(copies,1,1,p) {}
    Phased_Conway_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Conway_Multi_Server(copies,1,k,p) {}
    Phased_Conway_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Conway_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Phased_Conway_Multi_Server"; }
};

/* -------------- Conway Multiserver with Markov Phases --------------- */

class Markov_Phased_Conway_Multi_Server : public virtual Server,
					  public Conway_Multi_Server,
					  public Markov_Phased_Server {
public:
    Markov_Phased_Conway_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Conway_Multi_Server(copies,1,1,MAX_PHASES),
	  Markov_Phased_Server(1,1,MAX_PHASES) {}
    Markov_Phased_Conway_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Conway_Multi_Server(copies,1,1,p),
	  Markov_Phased_Server(1,1,p) {}
    Markov_Phased_Conway_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Conway_Multi_Server(copies,1,k,p),
	  Markov_Phased_Server(1,k,p) {}
    Markov_Phased_Conway_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Conway_Multi_Server(copies,e,k,p),
	  Markov_Phased_Server(e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Markov_Phased_Conway_Multi_Server"; }

protected:
    virtual Probability PBusy( const MVA& solver, const Population& N, const unsigned k ) const;

private:
    Positive meanMinimumOvertaking( const MVA& solver, const Population& N, const unsigned k, const unsigned p ) const;

private:
    virtual std::ostream& printInput( std::ostream& output, const unsigned e, const unsigned k ) const { return Markov_Phased_Server::printInput( output, e, k ); }
};

/*----------------------------------------------------------------------*/
/* 			    Rolia Multi Server				*/
/*----------------------------------------------------------------------*/

class Rolia_Multi_Server : public virtual Server,
			   public Reiser_Multi_Server {
public:
    Rolia_Multi_Server( const unsigned copies )
	: Server(),
	  Reiser_Multi_Server(copies) {}
    Rolia_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Reiser_Multi_Server(copies,k) {}
    Rolia_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Reiser_Multi_Server(copies,e,k) {}
    Rolia_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual unsigned int marginalProbabilitiesSize() const { return 0; }	/* No need for marginals	*/

    virtual const char * typeStr() const { return "Rolia_Multi_Server"; }

protected:
    double filter( const MVA&, const double, const unsigned, const unsigned, const unsigned ) const;
    virtual Positive sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const;
};

class Rolia_PS_Multi_Server : public virtual Server,
			      public Rolia_Multi_Server {
public:
    Rolia_PS_Multi_Server( const unsigned copies )
	: Server(),
	  Rolia_Multi_Server(copies) {}
    Rolia_PS_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Rolia_Multi_Server(copies,k) {}
    Rolia_PS_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Rolia_Multi_Server(copies,e,k) {}
    Rolia_PS_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Rolia_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Rolia_PS_Multi_Server"; }

protected:
    virtual Positive sumOf_L( const MVA& solver, const Population& N, const unsigned k ) const;
};

/* ---------------------- Phased Rolia Multiserver -------------------- */

class Phased_Rolia_Multi_Server : public virtual Server,
				  public Rolia_Multi_Server {
public:
    Phased_Rolia_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Rolia_Multi_Server(copies,1,1,MAX_PHASES) {}
    Phased_Rolia_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Rolia_Multi_Server(copies,1,1,p) {}
    Phased_Rolia_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Rolia_Multi_Server(copies,1,k,p) {}
    Phased_Rolia_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Rolia_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Phased_Rolia_Multi_Server"; }

protected:
    virtual Positive sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const;
};

/* ---------------- Rolia Multiserver with Markov Phases -------------- */

class Markov_Phased_Rolia_Multi_Server : public virtual Server,
					 public Phased_Rolia_Multi_Server,
					 public Markov_Phased_Server {
public:
    Markov_Phased_Rolia_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Phased_Rolia_Multi_Server(copies,1,1,MAX_PHASES),
	  Markov_Phased_Server(1,1,MAX_PHASES) {}
    Markov_Phased_Rolia_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Phased_Rolia_Multi_Server(copies,1,1,p),
	  Markov_Phased_Server(1,1,p) {}
    Markov_Phased_Rolia_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Phased_Rolia_Multi_Server(copies,1,k,p),
	  Markov_Phased_Server(1,k,p) {}
    Markov_Phased_Rolia_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Phased_Rolia_Multi_Server(copies,e,k,p),
	  Markov_Phased_Server(e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Markov_Phased_Rolia_Multi_Server"; }
};

/* ---------------------- Phased Rolia Multiserver -------------------- */

class Phased_Rolia_PS_Multi_Server : public virtual Server,
				     public Rolia_PS_Multi_Server {
public:
    Phased_Rolia_PS_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Rolia_PS_Multi_Server(copies,1,1,MAX_PHASES) {}
    Phased_Rolia_PS_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Rolia_PS_Multi_Server(copies,1,1,p) {}
    Phased_Rolia_PS_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Rolia_PS_Multi_Server(copies,1,k,p) {}
    Phased_Rolia_PS_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Rolia_PS_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Phased_Rolia_PS_Multi_Server"; }

protected:
    virtual Positive sumOf_L( const MVA& solver, const Population& N, const unsigned k ) const;
};

/* ---------------------- Phased Rolia Multiserver -------------------- */

class Markov_Phased_Rolia_PS_Multi_Server : public virtual Server,
					    public Rolia_PS_Multi_Server,
					    public Markov_Phased_Server {
public:
    Markov_Phased_Rolia_PS_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Rolia_PS_Multi_Server(copies,1,1,MAX_PHASES),
	  Markov_Phased_Server(1,1,MAX_PHASES) {}
    Markov_Phased_Rolia_PS_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Rolia_PS_Multi_Server(copies,1,1,p),
	  Markov_Phased_Server(1,1,p) {}
    Markov_Phased_Rolia_PS_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Rolia_PS_Multi_Server(copies,1,k,p),
	  Markov_Phased_Server(1,k,p) {}
    Markov_Phased_Rolia_PS_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Rolia_PS_Multi_Server(copies,e,k,p),
	  Markov_Phased_Server(e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Phased_Rolia_PS_Multi_Server"; }
};

/*----------------------------------------------------------------------*/
/*			   Zhou Multi-Server				*/
/*----------------------------------------------------------------------*/

class Zhou_Multi_Server : public virtual Server,
			  public Reiser_Multi_Server {
public:
    Zhou_Multi_Server( const unsigned copies )
	: Server(),
	  Reiser_Multi_Server(copies) {}
    Zhou_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Reiser_Multi_Server(copies,k) {}
    Zhou_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Reiser_Multi_Server(copies,e,k) {}
    Zhou_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual unsigned int marginalProbabilitiesSize() const { return 0; }	/* No need for marginals	*/

    virtual const char * typeStr() const { return "Zhou_Multi_Server"; }

private:
    virtual Positive sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const;

    double S_mean( const MVA& solver, const Population& N, const unsigned k ) const;
    Probability P_mean( const MVA& solver, const Population& N, const unsigned k ) const;
};

/* ------------------- Zhou Multiserver with Phases ------------------- */

class Phased_Zhou_Multi_Server : public virtual Server,
				 public Zhou_Multi_Server {
public:
    Phased_Zhou_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Zhou_Multi_Server(copies,1,1,MAX_PHASES) {}
    Phased_Zhou_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Zhou_Multi_Server(copies,1,1,p) {}
    Phased_Zhou_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Zhou_Multi_Server(copies,1,k,p) {}
    Phased_Zhou_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Zhou_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Zhou_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Zhou_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Phased_Zhou_Multi_Server"; }
};

/* ---------------- Zhou Multiserver with Markov Phases --------------- */

class Markov_Phased_Zhou_Multi_Server : public virtual Server,
					public Phased_Zhou_Multi_Server,
					public Markov_Phased_Server {
public:
    Markov_Phased_Zhou_Multi_Server( const unsigned copies )
	: Server(1,1,MAX_PHASES),
	  Phased_Zhou_Multi_Server(copies,1,1,MAX_PHASES),
	  Markov_Phased_Server(1,1,MAX_PHASES) {}
    Markov_Phased_Zhou_Multi_Server( const unsigned copies, const unsigned p )
	: Server(1,1,p),
	  Phased_Zhou_Multi_Server(copies,1,1,p),
	  Markov_Phased_Server(1,1,p) {}
    Markov_Phased_Zhou_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Server(1,k,p),
	  Phased_Zhou_Multi_Server(copies,1,k,p),
	  Markov_Phased_Server(1,k,p) {}
    Markov_Phased_Zhou_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Phased_Zhou_Multi_Server(copies,e,k,p),
	  Markov_Phased_Server(e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Zhou_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Zhou_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Markov_Phased_Zhou_Multi_Server"; }
};

/*----------------------------------------------------------------------*/
/*                        Bruell Multi-Server				*/
/*----------------------------------------------------------------------*/

class Bruell_Multi_Server : public virtual Server,
			    public Reiser_Multi_Server {
public:
    Bruell_Multi_Server( const unsigned copies )
	: Server(),
	  Reiser_Multi_Server(copies) {}
    Bruell_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Reiser_Multi_Server(copies,k) {}
    Bruell_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Reiser_Multi_Server(copies,e,k) {}
    Bruell_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p) {}
	
    virtual void setMarginalProbabilitiesSize( const Population &N );
    virtual unsigned int marginalProbabilitiesSize() const { return marginalSize; }
    virtual int vectorProbabilities() const { return 1; }

    virtual double muS( const Population& N, const unsigned k ) const;

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Bruell_Multi_Server"; }

protected:
    unsigned marginalSize;
};

/*----------------------------------------------------------------------*/
/*			  Schmidt Multi-Server				*/
/*----------------------------------------------------------------------*/

class Schmidt_Multi_Server : public virtual Server,
			     public Bruell_Multi_Server {
public:
    Schmidt_Multi_Server( const unsigned copies )
	: Server(),
	  Bruell_Multi_Server(copies) {}
    Schmidt_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Bruell_Multi_Server(copies,k) {}
    Schmidt_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Bruell_Multi_Server(copies,e,k) {}
    Schmidt_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Bruell_Multi_Server(copies,e,k,p) {}
	
    virtual double muS( const Population& N, const unsigned k ) const;

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual const char * typeStr() const { return "Schmidt_Multi_Server"; }
};

/* -------------------------------------------------------------------- */
/*                         Suri Multi Server                          	*/
/* -------------------------------------------------------------------- */

class Suri_Multi_Server : public Server {
public:
    Suri_Multi_Server( const unsigned copies )
	: Server(), J(copies) {}
    Suri_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k), J(copies) {}
    Suri_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k), J(copies) {}
    Suri_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p), J(copies) {}

    virtual double mu() const { return static_cast<double>(J); }

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const;
    virtual void openWait() const;

    virtual unsigned int marginalProbabilitiesSize() const { return 0; }

    virtual const char * typeStr() const { return "Suri_Multi_Server"; }

private:
    const unsigned int J;			/* Number of servers */

protected:
    static const double alpha;		// Eqn (17), Suri et. al.
    static const double beta;
};


class Markov_Phased_Suri_Multi_Server : public Suri_Multi_Server {
public:
    Markov_Phased_Suri_Multi_Server( const unsigned copies )
	: Suri_Multi_Server(1,1,MAX_PHASES) {}
    Markov_Phased_Suri_Multi_Server( const unsigned copies, const unsigned p )
	: Suri_Multi_Server(1,1,p) {}
    Markov_Phased_Suri_Multi_Server( const unsigned copies, const unsigned k, const unsigned p )
	: Suri_Multi_Server(1,k,p) {}
    Markov_Phased_Suri_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Suri_Multi_Server(e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Suri_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Suri_Multi_Server::openWait(); }

protected:
    virtual const char * typeStr() const { return "Markov_Phased_Suri_Multi_Server"; }
};


#if 0
/* ------------------ Multi Server with Priorities -------------------- */

class HOL_Reiser_Multi_Server : public virtual Server,
				public Reiser_Multi_Server {
public:
    HOL_Reiser_Multi_Server( const unsigned copies )
	: Server(),
	  Reiser_Multi_Server(copies) {}
    HOL_Reiser_Multi_Server( const unsigned copies, const unsigned k )
	: Server(k),
	  Reiser_Multi_Server(copies,k) {}
    HOL_Reiser_Multi_Server( const unsigned copies, const unsigned e, const unsigned k )
	: Server(e,k),
	  Reiser_Multi_Server(copies,e,k) {}
    HOL_Reiser_Multi_Server( const unsigned copies, const unsigned e, const unsigned k, const unsigned p )
	: Server(e,k,p),
	  Reiser_Multi_Server(copies,e,k,p) {}

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const { return Reiser_Multi_Server::mixedWait( solver, N ); }
    virtual void openWait() const { return Reiser_Multi_Server::openWait(); }

    virtual int priorityServer() const { return 1; }
    virtual const char * typeStr() const { return "HOL_Reiser_Multi_Server"; }
};
#endif
#endif
