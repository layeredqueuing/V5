/* -*- c++ -*-
 * Interface to the random number generator.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * Nov 2024.
 *     
 * $Id: task.h 17410 2024-10-31 13:54:12Z greg $
 */


#include "random.h"
#include <cmath>

std::random_device Random::__random_device;
std::mt19937 Random::__generator(Random::__random_device());
std::uniform_real_distribution<double> Random::__f(0.,1.);

double HyperExponential::operator()()
{
#if 0
    if ( drand48() <= 0.5 / (_cv_sqr - 0.5) ) {
	return _f(_generator())( _mean * _cv_sqr );
    } else {
	return _f(_generator())( _mean / 2.0 );
    }
#else
    const double prob = 0.5 * (1.0 - (std::sqrt((_cv_sqr-1.0)/(_cv_sqr+1.0))));
    const double temp = _f(__generator)>prob ? (_mean/(1.0-prob)) : (_mean/prob);
    return -0.5 * temp * log(_f(__generator));
#endif
}



double Pareto::operator()()
{
    return _scale * pow( 1.0 - _f(__generator), -1.0 / _shape );
}
