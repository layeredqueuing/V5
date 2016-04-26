/*	-*- c++ -*-
 * $Id: confidence_intervals.cc 9747 2010-08-18 03:26:40Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * May 2010
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <string>
#include <cstdlib>
#include <cmath>
#include <assert.h>

#include "confidence_intervals.h"

namespace LQIO {

    double ConfidenceIntervals::t_values[2][34] =
    { 
        {
            12.706,  4.303,  3.182,  2.776,  2.571,  2.447,  2.365,  2.306,
             2.262,  2.228,  2.201,  2.179,  2.160,  2.145,  2.131,  2.120,
             2.110,  2.101,  2.093,  2.086,  2.080,  2.074,  2.069,  2.064,
             2.060,  2.056,  2.052,  2.048,  2.045,  2.042,  2.021,  2.000,
             1.980,  1.960
        }, {
            63.657,  9.925,  5.841,  4.604,  4.032,  3.707,  3.499,  3.355,
             3.250,  3.169,  3.106,  3.055,  3.012,  2.977,  2.947,  2.921,
             2.898,  2.878,  2.861,  2.845,  2.831,  2.819,  2.807,  2.797,
             2.787,  2.779,  2.771,  2.763,  2.756,  2.750,  2.704,  2.660,
             2.617,  2.576
        } 
    };

    /* static */ double 
    ConfidenceIntervals::get_t_value(const unsigned blocks, const confidence_level_t level)
    {
	if ( blocks < 2 ) return 0.0;
	const unsigned i = (level == CONF_95 ? 0 : 1);
	const unsigned j = blocks - 2;

	if(j < 30) {
	    return t_values[i][j];
	} else if(j < 40) {
	    return t_values[i][29] + (t_values[i][30] - t_values[i][29])*(j - 29)/10.0;
	} else if(j < 60) {
	    return t_values[i][30] + (t_values[i][31] - t_values[i][30])*(j - 39)/20.0;
	} else if(j < 120) {
	    return t_values[i][31] + (t_values[i][32] - t_values[i][31])*(j - 59)/60.0;
	} else {
	    return t_values[i][33];
	}
	
    }

    /* static */ double 
    ConfidenceIntervals::invert( const double value, const unsigned blocks, const confidence_level_t level )
    {
	return value * value / get_t_value( blocks, level );
    }


    ConfidenceIntervals::ConfidenceIntervals( const confidence_level_t level, const unsigned blocks )
	: _t_value(0), _level(level)
    {
	if ( blocks > 1 ) {
	    set_t_value( blocks );
	}
    }

    ConfidenceIntervals& 
    ConfidenceIntervals::set_t_value( const unsigned blocks )
    {
	_t_value = get_t_value( blocks, _level );
	assert( _t_value != 0.0 );
	return *this;
    }

    double 
    ConfidenceIntervals::operator()( const double arg ) const
    {
	return sqrt( arg ) * _t_value;
    }

    double
    ConfidenceIntervals::invert( const double arg ) const
    {
	return (arg * arg) / _t_value;
    }
};
