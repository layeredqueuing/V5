/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/libmva/src/headers/mva/open.h $
 *
 * Open Network solver.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * $Date: 2020-12-31 09:51:49 -0500 (Thu, 31 Dec 2020) $
 *
 * $Id: open.h 14305 2020-12-31 14:51:49Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(OPEN_H)
#define	OPEN_H

#include "vector.h"
#include "pop.h"

class Open;
class Server;
class MVA;

std::ostream& operator<<( std::ostream &, Open& );

/* -------------------------------------------------------------------- */

class Open 
{
    /* The following is defined in the Open test suite and only used there. */
	
#if defined(TESTMVA)
    friend bool check( const Open& solver, const unsigned );
#endif
	
public:
    Open( Vector<Server *>& );
    virtual ~Open();

    std::ostream& print( std::ostream& output = std::cout ) const;

    void solve( const MVA& closedModel, const Population& N );	/* Mixed models.	*/
    void solve();						/* Open models.		*/
    void convert( const Population& N ) const; 			/* Switcharoo.		*/
    double throughput( const Server& ) const;
    double utilization( const Server& ) const;
    double entryThroughput( const Server&, const unsigned ) const;
    double entryUtilization( const Server&, const unsigned ) const;

protected:
    const unsigned M;			/* Number of stations.		*/
    Vector<Server *>& Q;		/* Queue type.  SS/delay.	*/
};
#endif

