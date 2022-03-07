/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 *
 * $Id: lqngen.cc 11731 2013-10-17 19:20:36Z greg $
 */

#include "randomvar.h"
#include <sstream>
#include <stdexcept>

namespace RV
{
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
		x = pow( drand48(), 1.0/_a );
		y = pow( drand48(), 1.0/_b );
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
		prod *= drand48();
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

#if !HAVE_DRAND48
    /* Windows doesn't have this... So stolen from Parasol drand48.c */

/*
 *	drand48, etc. pseudo-random number generator
 *	This implementation assumes unsigned short integers of at least
 *	16 bits, long integers of at least 32 bits, and ignores
 *	overflows on adding or multiplying two unsigned integers.
 *	Two's-complement representation is assumed in a few places.
 *	Some extra masking is done if unsigneds are exactly 16 bits
 *	or longs are exactly 32 bits, but so what?
 *	An assembly-language implementation would run significantly faster.
 */

#define N	16
#define MASK	((unsigned)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)	((unsigned)(x) & MASK)
#define HIGH(x)	LOW((x) >> N)
#define MUL(x, y, z)	{ long l = (long)(x) * (long)(y); (z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)	((long)(x) + (long)(y) > MASK)
#define ADDEQU(x, y, z)	(z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0	0x330E
#define X1	0xABCD
#define X2	0x1234
#define A0	0xE66D
#define A1	0xDEEC
#define A2	0x5
#define C	0xB
#define SET3(x, x0, x1, x2)	((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SETLOW(x, y, n) SET3(x, LOW((y)[n]), LOW((y)[(n)+1]), LOW((y)[(n)+2]))
#define SEED(x0, x1, x2) (SET3(x, x0, x1, x2), SET3(a, A0, A1, A2), c = C)
#define NEST(TYPE, f, F)	TYPE f( unsigned short xsubi[3] ) { \
	register TYPE v; unsigned temp[3]; \
	for (unsigned int i = 0; i < 3; i++) { temp[i] = x[i]; x[i] = LOW(xsubi[i]); }  \
	v = F(); for (unsigned int i = 0; i < 3; i++) { xsubi[i] = x[i]; x[i] = temp[i]; } return v; }
#define HI_BIT	(1L << (2 * N - 1))

static unsigned x[3] = { X0, X1, X2 }, a[3] = { A0, A1, A2 }, c = C;
static unsigned short lastx[3];
static void next();

double drand48()
{
  static double two16m = 1.0 / (1L << N);
  next();
  return (two16m * (two16m * (two16m * x[0] + x[1]) + x[2]));
}

NEST(double, erand48, drand48);

static void
next()
{
  unsigned p[2], q[2], r[2], carry0, carry1;

  MUL(a[0], x[0], p);
  ADDEQU(p[0], c, carry0);
  ADDEQU(p[1], carry0, carry1);
  MUL(a[0], x[1], q);
  ADDEQU(p[1], q[0], carry0);
  MUL(a[1], x[0], r);
  x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] + a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
  x[1] = LOW(p[1] + r[0]);
  x[0] = LOW(p[0]);
}

void
srand48( long seedval )
{
  SEED(X0, LOW(seedval), HIGH(seedval));
}

#endif
