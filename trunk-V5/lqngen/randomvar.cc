/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 *
 * $Id: lqngen.cc 11731 2013-10-17 19:20:36Z greg $
 */

#include "randomvar.h"
#include <sstream>

namespace RV {
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

    Probability::Probability( double mean ) : RandomVariable(CONTINUOUS), _mean(0) 
    { 
	setArg( 1, mean );
    }

    Probability& Probability::setArg( unsigned int i, double arg )
    {
	assert ( i == 1 );
	if ( arg < 0.0 || 1.0 < arg ) {
	    std::ostringstream err;
	    err << "Invalid probability: " << arg;
	    throw std::domain_error( err.str() );
	} else {
	    _mean = arg;
	}
	return *this;
    }
}


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
