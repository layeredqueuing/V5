/*  -*- c++ -*-
 * $Id: gamma.cc 14817 2021-06-15 16:51:27Z greg $
 *
 * Gamma distribution.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November 30, 2012
 *
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include <cassert>
#include <mva/fpgoop.h>
#include "gamma.h"

Gamma_Distribution::Gamma_Distribution( double shape, double scale )
    : _shape(shape), _scale(scale)
{
    setParameters( shape, scale );
}

void
Gamma_Distribution::setParameters( double shape, double scale )
{
    assert( shape >= 0 && scale > 0 );

    _c = shape * log( scale ) + logGamma( shape );
}

/**
 * This method computes the cumulative distribution function of the distribution.
 * @param x a number in the domain of the distribution
 * @return the cumulative probability at x
 */

double 
Gamma_Distribution::getCDF(double x) const
{
    return gammaCDF(x / _scale, _shape);
}

/**
 * This method computes the log of the gamma function.
 * @param x a positive number
 * @return the log of the gamma function at x
 */

double 
Gamma_Distribution::logGamma( double x )
{
    static const double coef[] = {76.18009173, -86.50532033, 24.01409822, -1.231739516, 0.00120858003, -0.00000536382};
    const double step = 2.50662827465;
    const double fpf = 5.5;
    double t = x - 1;
    double tmp = t + fpf;
    tmp = (t + 0.5) * log(tmp) - tmp;
    double ser = 1;
    for (int i = 0; i < 6; i++){
	t = t + 1;
	ser = ser + coef[i] / t;
    }
    return tmp + log(step * ser);
}


/**
 * This method computes the probability density function,
 * @param x a number &gt; 0
 * @return the probability density at x
 */

double 
Gamma_Distribution::getDensity( double x ) const
{
    if (x < 0) return 0;
    else if (x == 0 ) {
	if ( _shape < 1) return get_infinity();
	if ( _shape == 1) return exp(-_c);
	else return 0;
    } else {
	return exp(-_c + (_shape - 1) * log(x) - x / _scale);
    }
}


/**
 * This method computes the cumulative distribution function of the gamma distribution
 * with a specified shape parameter and scale parameter 1.
 * @param x a positive number
 * @param a the shape parameter
 * @return the cumulative probability at x
 */

double 
Gamma_Distribution::gammaCDF(double x, double a)
{
    if (x <= 0) return 0;
    else if (x < a + 1) return gammaSeries(x, a);
    else return 1 - gammaCF(x, a);
}

/**
 * This method computes a gamma series that is used in the gamma cumulative
 * distribution function.
 * @param x a postive number
 * @param a the shape parameter
 * @return the gamma series at x
 */
	
double
Gamma_Distribution::gammaSeries(double x, double a)
{
    //Constants
    const unsigned int maxit = 100;
    const double eps = 0.0000003;

    //Variables
    double sum = 1.0 / a;
    double ap = a;
    double gln = logGamma(a);
    double del = sum;
    for (unsigned int n = 1; n <= maxit; n++){
	ap++;
	del = del * x / ap;
	sum = sum + del;
	if (fabs(del) < fabs(sum) * eps) break;
    }
    return sum * exp(-x + a * log(x) - gln);
}

/**
 * This method computes a gamma continued fraction function function that is used in
 * the gamma cumulative distribution function.
 * @param x a positive number
 * @param a the shape parameter
 * @return the gamma continued fraction function at x
 */

double 
Gamma_Distribution::gammaCF(double x, double a)
{
    //Constants
    const unsigned int maxit = 100;
    const double eps = 0.0000003;
    //Variables
    double gln = logGamma(a);
    double g = 0;
    double gOld = 0;
    double a0 = 1;
    double a1 = x;
    double b0 = 0;
    double b1 = 1;
    double fac = 1;
    for (unsigned int n = 1; n <= maxit; n++){
	const double ana = static_cast<double>(n) - a;
	a0 = (a1 + a0 * ana) * fac;
	b0 = (b1 + b0 * ana) * fac;
	const double anf = static_cast<double>(n) * fac;
	a1 = x * a0 + anf * a1;
	b1 = x * b0 + anf * b1;
	if (a1 != 0){
	    fac = 1.0 / a1;
	    g = b1 * fac;
	    if (fabs((g - gOld) / g) < eps) break;
	    gOld = g;
	}
    }
    return exp(-x + a * log(x) - gln) * g;
}


