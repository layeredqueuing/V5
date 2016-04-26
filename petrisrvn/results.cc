/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/* June 2007.								*/
/************************************************************************/

/*
 * $Id: results.cc 11065 2012-07-06 16:18:06Z greg $
 *
 * Store the results.
 */

#include <cstdlib>
#include <vector>
#include "petrisrvn.h"
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <lqio/input.h>
#include <wspnlib/wspn.h>
#include <wspnlib/global.h>
#include "errmsg.h"
#include "makeobj.h"
#include "task.h"
#include "results.h"

using namespace std;

/* define	UNCONDITIONAL_PROBS */
/* define DERIVE_UTIL */


/*----------------------------------------------------------------------*/
/* Output processing.							*/
/*----------------------------------------------------------------------*/

/*
 * Get the mean number of tokens of the object name specified.
 */

double
get_pmmean( const char * fmt, ...)
{
    va_list args;
    char object_name[BUFSIZ];	/* Place/transition name.	*/
    double value;

    va_start( args, fmt );
    (void) vsprintf( object_name, fmt, args );
    va_end( args );
    hash_name( object_name );
    value = value_pmmean( object_name );
    if ( value < 0.0 ) {
	abort();
    }

    if ( debug_flag ) {
	(void) fprintf( stddbg, "E[%s]=%g\n", object_name, value );
    }

    return value;
}


/*
 * Get the mean number of tokens of the object name specified.
 */

double
get_pmvariance( const char * fmt, ...)
{
    va_list args;
    char object_name[BUFSIZ];	/* Place/transition name.	*/
    double value;

    va_start( args, fmt );
    (void) vsprintf( object_name, fmt, args );
    va_end( args );
    hash_name( object_name );
    value = value_pmvariance( object_name );

    if ( debug_flag ) {
	(void) fprintf( stddbg, "E[%s]=%g\n", object_name, value );
    }

    return value;
}


/*
 * Get the probability of m tokens.
 */

double
get_prob( int m, const char * fmt, ...)
{
    va_list args;
    char object_name[BUFSIZ];	/* Place/transition name.	*/
    double value;

    va_start( args, fmt );
    (void) vsprintf( object_name, fmt, args );
    va_end( args );
    hash_name( object_name );
    value = value_prob( object_name, m );
    if ( value < 0.0 ) {
	abort();
    }

    if ( debug_flag ) {
	(void) fprintf( stddbg, "E[%s]=%g\n", object_name, value );
    }

    return value;
}


/*
 * Get the throughput of the object specified.
 */

double
get_tput( short kind, const char * fmt, ... )
{
    va_list args;
    char object_name[BUFSIZ];	/* Place/transition name.	*/
    double value;

    va_start( args, fmt );
    (void) vsprintf( object_name, fmt, args );
    va_end( args );
    hash_name( object_name );
    if ( kind == IMMEDIATE ) {
	value = value_itput( object_name );
    } else {
	value = value_tput( object_name );
    }

    if ( debug_flag ) {
	(void) fprintf( stddbg, "lambda(%s)=%g\n", object_name, value );
    }

    return value;
}

std::ostream& solution_stats_t::print( std::ostream& output ) const
{
    output << "    Tangible:  " << tangible << endl
	   << "    Vanishing: " << vanishing << endl
	   << "    Precision: " << precision << endl;
    return output;
}


