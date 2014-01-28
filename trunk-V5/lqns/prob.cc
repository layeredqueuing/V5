/*  -*- c++ -*-
 * $Id$
 *
 * This class only allows double precision values in the range of 0.0 to
 * 1.0.  Attempts to set instances to values outside this range will
 * cause a core dump (not elegant, but what the hey!)
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <cmath>
#include "prob.h"
#include "fpgoop.h"

/* ----------------------- Helper Functions --------------------------- */

istream& 
operator<<( istream &input, Probability& arg )
{
    double temp;
    input >> temp;
    arg = temp;
    return input;
}

istream& 
operator<<( istream &input, Positive& arg )
{
    double temp;
    input >> temp;
    arg = temp;
    return input;
}

/*
 * Funky assignment operators.
 */

Probability& 
Probability::operator+=( const double arg )
{
    value = check( value + arg );
    return *this;
}

Probability& 
Probability::operator-=( const double arg )
{
    value = check( value - arg );
    return *this;
}

Probability& 
Probability::operator*=( const double arg )
{
    value = check( value * arg );
    return *this;
}

Probability& 
Probability::operator/=( const double arg )
{
    value = check( value / arg );
    return *this;
}

/*
 * Private member functions.
 */

double 
Probability::check( const double arg ) const
{
    if ( !isfinite(arg) || arg < 0.0 || (1.0 + NOISE) < arg ) {
	throw domain_error( "Probability::check" );
    } else if ( 1.0 < arg ) {
	return 1.0;    /* A little noise -- truncate. */
    }
    return arg;
}

/*
 * Funky assignment operators.
 */

Positive& 
Positive::operator+=( const double arg )
{
    value = check( value + arg );
    return *this;
}

Positive& 
Positive::operator-=( const double arg )
{
    value = check( value - arg );
    return *this;
}

Positive& 
Positive::operator*=( const double arg )
{
    value = check( value * arg );
    return *this;
}

Positive& 
Positive::operator/=( const double arg )
{
    value = check( value / arg );
    return *this;
}

/*
 * Private member functions.
 */

double 
Positive::check( const double arg ) const
{
    if ( !isfinite(arg) || arg < 0.0 ) {
	throw domain_error( "Positive::check" );
    }
    return arg;
}
