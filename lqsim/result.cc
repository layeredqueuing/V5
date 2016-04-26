/* result.cc	-- Greg Franks Fri Jun  5 2009
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqsim/result.cc $
 *
 * ------------------------------------------------------------------------
 * $Id: result.cc 11462 2013-08-05 01:33:59Z greg $
 * ------------------------------------------------------------------------
 */


#include "lqsim.h"
#include <cstdarg>
#include <parasol.h>
#include "result.h"
#include "model.h"

/*
 * For block statistics.
 */

unsigned number_blocks;

/*
 * Calculate the t1 and t2 values used for estimating the confidence
 * intervals.
 */

static double t_values[2][34] =
{ {
	12.706,	4.303,	3.182,	2.776,	2.571,	2.447,	2.365,	2.306,
	2.262,	2.228,	2.201,	2.179,	2.160,	2.145,	2.131,	2.120,
	2.110,	2.101,	2.093,	2.086,	2.080,	2.074,	2.069,	2.064,
	2.060,	2.056,	2.052,	2.048,	2.045,	2.042,	2.021,	2.000,
	1.980,	1.960
    }, {
	63.657,	9.925,	5.841,	4.604,	4.032,	3.707,	3.499,	3.355,
	3.250,	3.169,	3.106,	3.055,	3.012,	2.977,	2.947,	2.921,
	2.898,	2.878,	2.861,	2.845,	2.831,	2.819,	2.807,	2.797,
	2.787,	2.779,	2.771,	2.763,	2.756,	2.750,	2.704,	2.660,
	2.617,	2.576
    } };



inline static double conf_value( const unsigned i, const unsigned n )
{
    const int t_index = n - 2;		/* find t values	*/
    if ( t_index < 0 ) {
	return  0.0;
    } else if(t_index < 30) {
	return t_values[i][t_index];
    } else if(t_index < 40) {
	return t_values[i][29] + (t_values[i][30] - t_values[i][29])*(t_index - 29)/10.0;
    } else if(t_index < 60) {
	return t_values[i][30] + (t_values[i][31] - t_values[i][30])*(t_index - 39)/20.0;
    } else if(t_index < 120) {
	return t_values[i][31] + (t_values[i][32] - t_values[i][31])*(t_index - 59)/60.0;
    } else {
	return t_values[i][33];
    }
}

double
result_t::conf95( const unsigned n )
{
    return conf_value( 0, n );
}


double
result_t::conf99( const unsigned n )
{	
    return conf_value( 1, n );
}

void
result_t::init( int type, const char * format, ... )
{
    va_list args;
    char buf[128];

    buf[0] = 0;

    _type = type;

    va_start( args, format );
    (void) vsprintf( buf, format, args );
    va_end( args );

    raw = ps_open_stat( buf, _type );
    clear_results();
}


void
result_t::init( const long stat_id ) 
{
    raw = stat_id;		/* We already have a stat.  just set it up. */
    clear_results();
}


/*
 * Clear result fields.
 */

void
result_t::clear_results ()
{
    _sum        = 0;
    _sum_sqr    = 0;
    _count      = 0;
    _count_sqr  = 0;
    _n          = 0;
    _avg_count  = 0;
}



void
result_t::reset()
{
    ps_reset_stat( raw );	
}


/*
 * Accumulate the counters of 'raw' into cummulative for a population
 * type counter.  Returns the value in case a variance is needed.
 */

double
result_t::accumulate()
{
    double value;
    double count;

    ps_get_stat( raw, &value, &count );

#ifdef	NOTDEF
    if ( debug_flag && stddbg ) {
	(void) fprintf( stddbg, "%-38.38s POP  %12.7g %8ld\n", raw->name, value, count );
    }
#endif
    _sum       += value;
    _count     += count;
    _n	       += 1;
    _sum_sqr   += square( value );
    _count_sqr += square( count );

    ps_reset_stat( raw );	

    return value;		/* For variance. */
}



void
result_t::accumulate_variance ( const double mean )
{
    double mean_squares;
    double count;

    ps_get_stat( raw, &mean_squares, &count );

    double value = mean_squares - square(mean);
    if ( value < 0. ) value = 0.0;

    _sum       += value;
    _count     += count;
    _n	       += 1;
    _sum_sqr   += square( value );
    _count_sqr += square( count );
    ps_reset_stat( raw );
}



/*
 * Change count in service to match cycles.  This makes service reflect
 * phase time rather than slice time.  Do this before r_cycle because
 * accumulate will reset the count field.
 */

void
result_t::accumulate_service( const result_t& r_cycle )
{
    double value;
    double n_cycles;
    double n_slices;
    double junk;

    ps_get_stat( r_cycle.raw, &junk, &n_cycles );
    ps_get_stat( raw, &value, &n_slices );

    if ( n_cycles ) {
	value *= n_slices / n_cycles;
    } else {
	value = 0.0;
    }

    _sum       += value;
    _count     += n_cycles;
    _n	       += 1;
    _sum_sqr   += square( value );
    _count_sqr += square( n_cycles );
    ps_reset_stat( raw );
}



/*
 * Accumulate utilization for the case where the processor may be
 * preempted.  We derive it from service time (and throughput).
 */


void
result_t::accumulate_utilization( const result_t& r_cycle, const double service_time )
{
    double n_cycles;
    double junk;
    ps_get_stat( r_cycle.raw, &junk, &n_cycles );
    const double utilization = service_time * (n_cycles / Model::block_period());	/* service_time * throughput. */
    _sum       += utilization;
    _sum_sqr   += square( utilization );
    _count     += 1;
    _n         += 1;
    _count_sqr += 1;
    
    ps_reset_stat( raw );
}


/*
 * Return mean.
 */

double
result_t::mean() const
{
    if ( _n > 0 ) {
	return _sum / static_cast<double>(_n);
    } else {
	return 0;
    }
}



/*
 * Return the sample standard deviation.
 */

double
result_t::variance() const
{
    if ( _n >= 2 ) {
	double temp = _sum_sqr - square(_sum) / static_cast<double>(_n);
	if ( temp > 0.0 ) {
	    return temp / static_cast<double>(_n - 1);
	} else if ( temp < -0.1 ) {
	    abort();
	}
    }
    return 0.0;
}


double
result_t::mean_count() const
{
    if ( _n > 0 ) {
	return _count / static_cast<double>(_n);
    } else {
	return 0;
    }
}

/*
 * Return the sample standard deviation.
 */

double
result_t::variance_count() const
{
    if ( _n >= 2 ) {
	const double temp = _count_sqr - square(_count) / static_cast<double>(_n);
	if ( temp > 0.0 ) {
	    return temp / static_cast<double>(_n - 1);
	} else if ( temp < -0.1 ) {
	    abort();
	}
    }
    return 0.0;
}


/*
 * Output a line of the raw report.  Fold mutilate and spindle as
 * required.
 */

FILE *
result_t::print_raw( FILE * output, const char * format,... ) const
{
    va_list args;
    char buf[128];

    va_start( args, format );
    (void) vsprintf( buf, format, args );
    va_end( args );

    (void) fprintf( output, "%-38.38s %-8s ", buf, _type == VARIABLE ? "VARIABLE" : "SAMPLE" );
    if ( number_blocks > 1 ) {
	(void) fprintf( output, "%12.7g %12.7g %12.7g %8.0f\n", mean(), conf95( number_blocks ), conf99( number_blocks ), _count / _n );
    } else {
	(void) fprintf( output, "%12.7g %8.0f\n", _sum, _count );
    }
    return output;
}
