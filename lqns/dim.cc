/*  -*- c++ -*-
 * $Id: dim.cc 12547 2016-04-05 18:32:45Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#if !defined(TESTMVA)
#include "lqns.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "fpgoop.h"

extern double convergence_value;		/* value to converge to.	*/
using namespace std;

double
factorial( unsigned n )
{
    double product;
    for ( product = 1.0; n > 1; --n ) {
	product *= n;
    }
    return product;
}


/*
 * return ln of factorial of n.  Cache results.
 */

double
log_factorial( const unsigned n )
{
    static double a[101];	/* Automatically initialized to zero! */
	
    if ( n == 0 ) throw domain_error( "log_factorial(0)" );
    if ( n == 1 ) return 0.0;
    if ( n <= 100 ) {
	if ( a[n] == 0 ) {
#if HAVE_LGAMMA	    
	    a[n] = lgamma( n + 1.0 );
#else
	    a[n] = ::log( n ) + log_factorial( n - 1 );
#endif
	}
	return a[n];
    } else {
#if HAVE_LGAMMA	    
	return lgamma( n + 1.0 );
#else
	return ::log( n ) + log_factorial( n - 1 );
#endif
    }
}



/*
 * Returns the binomial coefficient
 */

double
binomial_coef( const unsigned n, const unsigned k )
{
    return floor( 0.5 + exp( log_factorial( n ) - log_factorial( k ) - log_factorial( n - k ) ) );
}



/*
 * Compute and return power.  We don't use pow() because we know that b is always an integer.
 */

static inline double
Power( const double a, unsigned b )
{
    double product = 1.0;

    while ( b > 0 ) {
	product *= a;
	b -= 1;
    }
    return product;
}



/*
 * Exponentiation.  Handles negative exponents.
 */

double
power( const double a, const int b )
{
    if ( b > 5 ) {
	return pow( a, (double)b );
    } else if ( b >= 0 ) {
	return Power( a, b );
    } else {
	return 1.0 / power( a, -b );
    }
}


/*
 * Choose.
 * i! / ( j! * (i-j)! )
 */

double
choose( unsigned i, unsigned j )
{
    assert ( i >= j );
    if ( i == 0 ) return 0;
    if ( j == 0 || i == j ) return 1;
	
    double product;
    const unsigned int a = max( j, i - j );
    const unsigned int b = min( j, i - j );
	
    for ( product = 1.0; i > a; --i ) {
	product *= i;
    }
	
    return product / factorial( b );
}


#if !defined(TESTMVA)
/*
 * Common underrelaxation code.  
 */

void
under_relax( double& old_value, const double new_value, const double relax ) 
{
    if ( isfinite( new_value ) && isfinite( old_value ) ) {
	old_value = new_value * relax + old_value * (1.0 - relax);
	if ( flags.trace_idle_time ) {
	cout <<"under_relax() .. relax=" << relax << endl;
	}

    } else {
	old_value = new_value;
    }
}
#endif


/*
 * Print out the error message.
 */

class_error::class_error( const string& aStr, const char * file, const unsigned line, const char * anError )
    : exception()
{
    char temp[10];
    sprintf( temp, "%d", line );

    myMsg = aStr;
    myMsg += ": ";
    myMsg += file;
    myMsg += " ";
    myMsg += temp;
    myMsg += ": ";
    myMsg += anError;
}


class_error::~class_error() throw()
{
}

const char * 
class_error::what() const throw()
{
    return myMsg.c_str();
}


path_error::~path_error() throw()
{
}

const char * 
path_error::what() const throw()
{
    return myMsg.c_str();
}


const char * 
exception_handled::what() const throw()
{
    return myMsg.c_str();
}


#if defined(__GNUC__) || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 700))
#include "prob.h"
#include "server.h"
#if !defined(TESTMVA) && !defined(TESTDIST)
#include "randomvar.h"
#include "activity.h"
#include "phase.h"
#include "call.h"
#include "entity.h"
#include "entry.h"
#include "processor.h"
#include "task.h"
#include "group.h"
#include "report.h"
#include "phase.h"
#include "randomvar.h"
#include "submodel.h"
#include "interlock.h"
#include "actlist.h"
#include "entrythread.h"
#include "stack.h"
#include "stack.cc"
#endif
#if !defined(TESTMVA) || defined(TESTDIST)
#include "randomvar.h"
#include "cltn.h"
#include "cltn.cc"
#endif
#include "vector.h"
#include "vector.cc"

#if	!defined(TESTMVA) && !defined(TESTDIST)
template class BackwardsSequence<Activity *>;
template class BackwardsSequence<Submodel *>;
template class Cltn<Activity *>;
template class Cltn<AndForkActivityList *>;
template class Cltn<AndJoinActivityList *>;
template class Cltn<AndForkActivityList const *>;
template class Cltn<Call *>;
template class Cltn<Group *>;
template class Cltn<CallInfoItem *>;
template class Cltn<Entity *>;
template class Cltn<Entity const *>;
template class Cltn<Entry *>;
template class Cltn<Entry const *>;
template class Cltn<Phase *>;
template class Cltn<Server *>;
template class Cltn<Submodel *>;
template class Cltn<Task *>;
template class Cltn<Task const *>;
template class Cltn<Thread *>;
template class Cltn<const Activity *>;
template class Cltn<ActivityList *>;
template class Sequence<Activity *>;
template class Sequence<ActivityList *>;
template class Sequence<AndJoinActivityList *>;
template class Sequence<Call *>;
template class Sequence<CallInfoItem *>;
template class Sequence<Entity *>;
template class Sequence<Entry *>;
template class Sequence<Submodel *>;
template class Sequence<Task *>;
template class Sequence<const Entity *>;
template class Sequence<const Entry *>;
template class Sequence<const Task *>;
template class Sequence<unsigned int>;
template class Stack<Entry *>;
template class Stack<Phase *>;
template class Stack<const Call *>;
template class Stack<const Activity *>;
template class Stack<const AndForkActivityList *>;
template class Stack<const Entity *>;
template class Stack<const Entry *>;
template class Vector<Exponential>;
template class Vector<GenericPhase>;
template class Vector<InterlockInfo>;
template class Vector<MVACount>;
#if _WIN64
template class Vector<unsigned long long>;
#endif
template class Vector<Vector<Exponential> >;
template class Vector<Vector<unsigned> >;
template class Vector<VectorMath<double> >;
template class Vector<VectorMath<unsigned> >;
template class Vector<unsigned short>;
template class Sequence<DiscretePoints *>;
#endif
#if !defined(TESTMVA) || defined(TESTDIST)
template class Cltn<DiscretePoints *>;
#endif
template class Vector<double>;
template class Vector<unsigned int>;
template class Vector<unsigned long>;
template class Vector<Server *>;
template class Vector<Probability>;
template class VectorMath<unsigned int>;
template class VectorMath<double>;
template class VectorMath<Probability>;

template ostream& operator<< ( ostream& output, const Vector<unsigned int>& self );
template ostream& operator<< ( ostream& output, const VectorMath<unsigned int>& self );
template ostream& operator<< ( ostream& output, const VectorMath<Probability>& self );
template ostream& operator<< ( ostream& output, const VectorMath<double>& self );

#if !defined(TESTMVA)
template ostream& operator<< ( ostream& output, const Vector<Exponential>& self );
template ostream& operator<< ( ostream& output, const Cltn<Activity *> & self );
#endif
#endif
