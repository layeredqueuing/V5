/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/lqns.h $
 *
 * Dimensions common to everything, plus some funky inline functions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: lqns.h 17275 2024-09-10 20:35:49Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef	LQNS_LQNS_H
#define LQNS_LQNS_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>


#define MAX_CLASSES     200                     /* Max classes (clients)        */
#define MAX_PHASES      3                       /* Number of Phases.            */
#define N_SEMAPHORE_ENTRIES     2               /* Number of semaphore entries  */

#define	BUG_270		1			/* Enable prune pragma		*/
#define PAN_REPLICATION	1			/* Use Amy Pan's replication	*/
#define BUG_299_PRUNE	1			/* Enable replica prune code	*/

const double EPSILON = 0.000001;		/* For testing against 1 or 0 */

/*
 * Return square.  C++ doesn't even have an exponentiation operator, let
 * alone a smart one.
 */

template <typename Type> inline Type square( Type a ) { return a * a; }
template <typename Type> inline void Delete( Type x ) { delete x; }

/* 
 * Common under-relaxation code.  Adapted to include newton-raphson
 * adjustment.  
 */

double under_relax( const double old_value, const double new_value, const double relax );

template <class Type> struct EqualsReplica {
    EqualsReplica( const std::string& name, unsigned int replica=1 ) : _name(name), _replica(replica) {}
    bool operator()( const Type * object ) const { return object->name() == _name && object->getReplicaNumber() == _replica; }
private:
    const std::string _name;
    const unsigned int _replica;
};
#endif
