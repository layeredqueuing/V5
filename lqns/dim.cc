/*  -*- c++ -*-
 * $Id: dim.cc 13676 2020-07-10 15:46:20Z greg $
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

/*
 * return factorial.  Cache results.
 */

double
factorial( unsigned n )
{
    static double a[101];
    if ( n == 1 || n == 0 ) return 1.0;
    if ( n <= 100 ) {
	if ( a[n] == 0 ) {
	    a[n] = static_cast<double>(n) * factorial(n-1);
	}
	return a[n];
    } else {
	double product;
	for ( product = 1.0; n > 1; --n ) {
	    product *= n;
	}
	return product;
    }
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
 * Exponentiation.  Handles negative exponents.
 */

double
power( double a, int b )
{
    if ( b > 5 ) {
	return pow( a, (double)b );
    } else if ( b >= 0 ) {
	double product = 1.0;
	for ( ; b > 0; --b ) product *= a;
	return product;
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
