/* result.cc	-- Greg Franks Fri Jun  5 2009
 *
 * ------------------------------------------------------------------------
 * $Id: result.cc 17466 2024-11-13 14:17:16Z greg $
 * ------------------------------------------------------------------------
 */


#include "lqsim.h"
#include <cstdarg>
#include <sstream>
#include <iomanip>
#include "result.h"
#include "model.h"
#include "instance.h"

/*
 * For block statistics.
 */

unsigned long number_blocks;

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
Result::conf95( const unsigned n )
{
    return conf_value( 0, n );
}


double
Result::conf99( const unsigned n )
{	
    return conf_value( 1, n );
}

std::string Result::getName( const std::string& name ) const
{
    if ( _dom == nullptr ) return name;

    std::ostringstream s;
    s << std::left << std::setw( 6 ) << _dom->getTypeName() << " " << std::left << std::setw( 11 ) << _dom->getName() << " - " << std::left << std::setw( 18 ) << name;
    return s.str();
}


const Result& Result::insertDOMResults( Result::set_fn set_mean, Result::set_fn set_variance ) const
{
    (_dom->*set_mean)( mean() );
    if ( number_blocks > 1 && set_variance != nullptr ) {
	(_dom->*set_variance)( variance() );
    }
    return *this;
}


std::ostream& Result::print( std::ostream& output ) const
{
    char buf[1024];
    (void) snprintf( buf, 1024, "%-38.38s %-8s ", _name.c_str(), getTypeName().c_str() );
    output << buf;
    if ( number_blocks > 2 ) {
	(void) snprintf( buf, 1024, "%12.7g %12.7g %12.7g %8.0f\n", mean(), conf95( number_blocks ), conf99( number_blocks ), mean_count() );
    } else {
	(void) snprintf( buf, 1024, "%12.7g %8.0f\n", mean(), mean_count() );	// _n == 1
    }
    output << buf;
    return output;
}


/*
 * Clear result fields.
 */

void
Result::clear_results()
{
    _sum        = 0;
    _sum_sqr    = 0;
    _count      = 0;
    _count_sqr  = 0;
    _n          = 0;
    _avg_count  = 0;
}



void
Result::reset()
{
    _resid = 0.0;
}


/*
 * Accumulate the counters of 'raw' into cummulative for a population
 * type counter.  Returns the value in case a variance is needed.
 */

double
Result::accumulate()
{
    const double value = getMean();
    const double count = getOther();

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

    reset();	

    return value;		/* For variance. */
}



void
Result::accumulate_variance ( const double mean )
{
    const double mean_squares = getMean();
    const double count = getOther();
    double value = mean_squares - square(mean);
    
    if ( value < 0. ) value = 0.0;

    _sum       += value;
    _count     += count;
    _n	       += 1;
    _sum_sqr   += square( value );
    _count_sqr += square( count );

    reset();
}



/*
 * Change count in service to match cycles.  This makes service reflect
 * phase time rather than slice time.  Do this before r_cycle because
 * accumulate will reset the count field.
 */

void
Result::accumulate_service( const Result& r_cycle )
{
    double value = getMean();
    const double n_cycles = getOther();
    const double n_slices = r_cycle.getOther();

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
    reset();
}



/*
 * Accumulate utilization for the case where the processor may be
 * preempted.  We derive it from service time (and throughput).
 */


void
Result::accumulate_utilization( const Result& r_cycle, const double service_time )
{
    double n_cycles = r_cycle.getOther();
    const double utilization = service_time * (n_cycles / Model::block_period());	/* service_time * throughput. */
    _sum       += utilization;
    _sum_sqr   += square( utilization );
    _count     += 1;
    _n         += 1;
    _count_sqr += 1;

    reset();
}


/*
 * Return mean.
 */

double
Result::mean() const
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
Result::variance() const
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
Result::mean_count() const
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
Result::variance_count() const
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

std::string SampleResult::__type_name( "SAMPLE" );

void SampleResult::record( double value )
{
    _count++;
    _resid += value;
    double temp = _sum + _resid;
    _resid += _sum - temp;
    _sum = temp;
}


/* Adds a number to a statistic sample 	*/
/* It is only used to add preemption time to the waiting time. Tao	*/

void SampleResult::add( double value )
{
    _resid += value;
    double temp = _sum + _resid;
    _resid += _sum - temp;
    _sum = temp;
}


void SampleResult::reset()
{
    Result::reset();
    _count = 0;
    _sum = 0.0;
}

std::string VariableResult::__type_name( "VARIABLE" );

void VariableResult::record( double value )
{

    double delta = Instance::now() - _old_time;
    if ( delta == 0.0 ) {
	_old_value = value;
    } else {
	_resid += delta * _old_value;
	double temp = _integral + _resid;
	_resid += (_integral - temp);
	_integral = temp; 
	_old_time = Instance::now();
	_old_value = value;
    }
}

/*
 * same as record, but uses offset instead of ps_now.  Likely can be refactored.
 */

void
VariableResult::record_offset( double value, double start )
{
    double delta = start - _old_time;
    if ( delta == 0.0 ) {
	_old_value = value;
    } else {
	_resid += (delta * _old_value);
	double temp = _integral + _resid;
	_resid += (_integral - temp);
	_integral = temp; 
	_old_time = start;
	_old_value = value;
    }
}

void VariableResult::reset()
{
    Result::reset();
    _start = Instance::now();
    _old_time = Instance::now();
    _integral = 0.0;
    _resid = 0.0;
}

double VariableResult::getMean() const
{
    double period = Instance::now() - _start;
    if ( period > 0.0 ) {
	_integral += (Instance::now() - _old_time) * _old_value;
	_old_time = Instance::now();
	return _integral / period;
    } else {
	return 0.;
    }
}

double VariableResult::getOther() const
{
    return Instance::now() - _start;
}

#if !BUG_289
void ParasolResult::init( const long id )
{
    _raw = id;		/* We already have a stat.  just set it up. */
    clear_results();
}

void
ParasolResult::reset()
{
    ps_reset_stat( _raw );	
}

double ParasolResult::getMean() const
{
    double value;
    double other;
    ps_get_stat( _raw, &value, &other );
    return value;
}

double ParasolResult::getOther() const
{
    double value;
    double other;
    ps_get_stat( _raw, &value, &other );
    return other;
}
#endif
