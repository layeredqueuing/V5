/*   -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/prob.h $
 *
 * Probabilty type.  Checks for valid probabilities.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: prob.h 11963 2014-04-10 14:36:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PROBABILITY_H)
#define PROBABILITY_H

#define NOISE 0.00001			/* For probability fixing.	*/

#include "dim.h"

class Probability {
    friend class Positive;
	
public:
    Probability( const double arg=0.0 ) { value = check( arg ); }
    Probability( const Probability& arg ) { value = arg.value; }
    Probability& operator=( const Probability& arg ) { value = arg.value; return *this; }
    Probability& operator=( const double arg ) { value = check( arg ); return *this; }

    operator double() const { return value; }
	
    Probability& operator+=( const double );
    Probability& operator-=( const double );
    Probability& operator*=( const double );
    Probability& operator/=( const double );
    friend ostream& operator<<( ostream &output, const Probability& arg ) { output << arg.value; return output; }
    friend istream& operator<<( istream &input, Probability& arg );

private:
    double value;
    double check( const double ) const;
};


class Positive {
public:
    Positive( const double arg=0.0 ) { value = check( arg ); }
    Positive( const Positive& arg ) { value = arg.value; }
    Positive( const Probability& arg ) { value = arg.value; }
    Positive& operator=( const Positive& arg ) { value = arg.value; return *this; }
    Positive& operator=( const Probability& arg ) { value = arg.value; return *this; }
    Positive& operator=( const double arg ) { value = check( arg ); return *this; }

    operator double() const { return value; }

    Positive& operator+=( const double );
    Positive& operator-=( const double );
    Positive& operator*=( const double );
    Positive& operator/=( const double );
    friend ostream& operator<<( ostream &output, const Positive& arg ) { output << arg.value; return output; }
    friend istream& operator<<( istream &input, Positive& arg );

private:
    double value;
    double check( const double ) const;
};

#endif
