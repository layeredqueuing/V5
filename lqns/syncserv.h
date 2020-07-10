/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/syncserv.h $
 *
 * Servers for MVA solver.  Subclass as needed.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: syncserv.h 13676 2020-07-10 15:46:20Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(SYNCH_SERVER_H)
#define	SYNCH_SERVER_H

#include "dim.h"
#include "server.h"
#include "ph2serv.h"

/* -------------------------------------------------------------------- */
/* 		       Simple (Reiser Product Form)			*/
/* -------------------------------------------------------------------- */

class Synch_Server : public virtual Server 
{
public:
    Synch_Server() : Server() { throw should_not_implement( "Synch_Server::Synch_Server", __FILE__, __LINE__ ); }
    Synch_Server( const unsigned k ) : Server(k) { throw should_not_implement( "Synch_Server::Synch_Server", __FILE__, __LINE__ ); }
    Synch_Server( const unsigned e, const unsigned k ) : Server(e,k) { initialize(); }
    Synch_Server( const unsigned e, const unsigned k, const unsigned p ) : Server(e,k,p) { initialize(); }
	
    virtual Server& setVariance( const unsigned e, const unsigned k, const unsigned p, const double value ) { return *this; } /* NOP */

    virtual void wait( const MVA& solver, const unsigned k, const Population & N ) const;
    virtual void mixedWait( const MVA& solver, const Population& N ) const;
    virtual void openWait() const;

    virtual const char * typeStr() const { return "Synch_Server"; }
    virtual ostream& printHeading( ostream& output = cout ) const;

public:
    virtual Server& setClientChain( const unsigned e, const unsigned k );
	
private:
    void initialize();
    void shouldNotImplement();

    virtual short updateD() const { return 0; }			/* For synch server.  	*/
	
//	double synchDelay( const MVA&, const unsigned, const Population &N ) const;
    Positive max( const double, const double ) const;
    Positive Upsilon( const MVA&, const unsigned e, const Population &N, const unsigned k ) const;
    double alpha( const MVA&, const unsigned, const Population &N ) const;
    Positive gamma( const MVA&, const unsigned e, const Population &N, const unsigned k ) const;

#ifdef NOTDEF
    void addTerm( double&, const double, const double, const unsigned, const double rate[] ) const;
#endif
private:
    unsigned chainForEntry[3];
};
#endif
