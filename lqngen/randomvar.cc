/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 *
 * $Id: lqngen.cc 11731 2013-10-17 19:20:36Z greg $
 */

#include "randomvar.h"
#include <sstream>
#include <stdexcept>

namespace RV
{
    std::random_device RandomVariable::__random_device;
    std::mt19937 RandomVariable::__generator(RandomVariable::__random_device());
    std::uniform_real_distribution<double> RandomVariable::__f(0.,1.);

    RandomVariable&
    RandomVariable::setMean( const std::string& str )
    {
	char * endptr = 0;
	setMean( strtod( str.c_str(), &endptr ) );
	if ( endptr != 0 && *endptr != '\0' ) throw std::invalid_argument( "Invalid numeric argument" );
	return *this;
    }


    RandomVariable&
    RandomVariable::setArg( unsigned int i, const std::string& arg )
    {
	char * endptr = 0;
	setArg( i, strtod( arg.c_str(), &endptr ) );
	if ( endptr != 0 && *endptr != '\0' ) throw std::invalid_argument( "Invalid numeric argument" );
	return *this;
    }


    std::ostream& RandomVariable::print( std::ostream& output ) const
    {
	output << name() << "(" << getArg(1);
	if ( nArgs() == 2 ) {
	    output << "," << getArg(2);
	}
	output << ")"; return output;
    }

    double Beta::operator()() const
    {
	double x = 0;
	double y = 0;
	if ( _a < 1 && _b < 1 ) {
	    do { 
		x = pow( number(), 1.0/_a );
		y = pow( number(), 1.0/_b );
	    } while ( x + y > 1 );
	} else {
	    x = Gamma( 1, _a )();
	    y = Gamma( 1, _b )();
	}
	return x / ( x + y );
    }


    Beta& Beta::setMean( double mean )
    {
	if ( mean <= 0 || 1 <= mean ) throw std::invalid_argument( "mean <= 0 || 1 <= mean" );
	_a = mean * _b / ( 1.0 - mean );
	return *this;
    }
    
    double Gamma::operator()() const 
    {
	if ( _b < 1 ) {
	    return _a * Beta( _b, 1.0 - _b )() * Exponential( 1.0 )();
	} else if ( _b == floor( _b ) ) {
	    double prod = 1;
	    for ( unsigned int i = 0; i < _b; ++i ) {
		prod *= number();
	    }
	    return -_a * log( prod );
	} else {
	    return Gamma(_a, floor(_b))() + Gamma( _a, _b - floor(_b) )();
	}
    }

    Pareto& Pareto::setMean( double mean )
    {
	if ( mean <= 1.0 ) throw std::invalid_argument( "mean <= 1" );
	_a = mean / ( mean - 1.0 );
	return *this;
    }

    Probability& Probability::setMean( double mean )
    {
	if ( mean < 0 || 1 < mean ) throw std::invalid_argument( "mean < 0 || 1 < mean" );
	_mean = mean;
	return *this;
    }
}

const char * const RV::Exponential::__name  = "exponential";
const char * const RV::Pareto::__name       = "pareto";
const char * const RV::Uniform::__name 	    = "uniform";
const char * const RV::Constant::__name     = "constant";
const char * const RV::Normal::__name       = "normal";
const char * const RV::LogUniform::__name   = "loguniform";
const char * const RV::Gamma::__name        = "gamma";
const char * const RV::Beta::__name         = "beta";
const char * const RV::Poisson::__name      = "poisson";
const char * const RV::Binomial::__name     = "binomial";
const char * const RV::Probability::__name  = "probability";
