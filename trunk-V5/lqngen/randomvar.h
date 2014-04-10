/* -*- c++ -*-
 * generate.h	-- Greg Franks
 *
 * $Id$
 * ------------------------------------------------------------------------
 */

#if !defined(RV_RANDOMVAR_H)
#define RV_RANDOMVAR_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <cstdlib>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <ostream>

#if !HAVE_DRAND48
double drand48();
void srand48( long seedval );
#endif

namespace RV {
    class RandomVariable
    {
    public:
	typedef enum { DETERMINISTIC, BOTH, CONTINUOUS, DISCREET } distribution_t;

	RandomVariable( distribution_t type ) : _type(type) {}
	virtual ~RandomVariable() {}
	virtual RandomVariable * clone() const = 0;

	virtual double operator()() const = 0;
	virtual std::ostream& operator<<( std::ostream& ) const = 0;
	virtual const char * name() const = 0;
	virtual unsigned int nArgs() const = 0;
	virtual double getArg( unsigned int ) const = 0;
	virtual RandomVariable& setArg( unsigned int, double ) = 0;
	virtual double getMean() const = 0; 
	virtual RandomVariable& setMean( double ) = 0;
	virtual distribution_t getType() const { return _type; }

    private:
	const distribution_t _type;
    };

    class Exponential : public RandomVariable
    {
    public:
	Exponential( double a ) : RandomVariable(CONTINUOUS), _a(a) {}
	virtual Exponential * clone() const { return new Exponential( getArg( 1 ) ); }

	virtual double operator()() const { return  -_a * log( drand48() ); }
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << ")"; return output; }

	virtual const char * name() const { return "exponential"; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _a; }
	virtual Exponential& setArg( unsigned int i, double arg ) { assert( i == 1 ); _a = arg; return *this; }
	virtual double getMean() const { return 1.0 / _a; } 
	virtual Exponential& setMean( double mean ) { _a = 1.0/mean; return *this; }

    private:
	double _a;
    };

    class Pareto : public RandomVariable
    {
    public:
	Pareto( double a ) : RandomVariable(CONTINUOUS), _a(a) {}
	virtual Pareto * clone() const { return new Pareto( getArg(1) ); }

	virtual double operator()() const { return 1.0 / pow( drand48(), 1.0 / _a ); }
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << ")"; return output; }

	virtual const char * name() const { return "pareto"; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _a; }
	virtual Pareto& setArg( unsigned int i, double arg ) { assert( i == 1 ); _a = arg; return *this; }
	virtual double getMean() const { return _a / (_a - 1.0); } 
	virtual Pareto& setMean( double mean ) { assert( mean > 1 ); _a = mean / ( mean - 1.0 ); return *this; }

    private:
	double _a;
    };

    class Uniform  : public RandomVariable
    {
    public:
	Uniform( double low, double high ) : RandomVariable(BOTH), _low(low), _high(high) {}
	virtual Uniform * clone() const { return new Uniform( getArg(1), getArg(2) ); }

	virtual double operator()() const { return _low + (_high - _low) * drand48(); }
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << "," << getArg(2) << ")"; return output; }

	virtual const char * name() const { return "uniform"; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _low : _high; }
	virtual Uniform& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _low : _high) = arg; return *this; }
	virtual double getMean() const { return (_high - _low) / 2.0; } 
	virtual Uniform& setMean( double mean ) { double delta = (_high - _low) / 2.0; _low = mean - delta; _high = mean + delta; return *this; }

    private:	
	double _low;
	double _high;
    };

    class LogUniform  : public RandomVariable
    {
    public: 
	LogUniform( double low, double high ) : RandomVariable(CONTINUOUS), _low(low), _high(high) {}
	virtual LogUniform * clone() const { return new LogUniform( getArg(1), getArg(2) ); }

	virtual double operator()() const 
	    { 
		const double log_low = log(_low);
		return exp( log_low + (log(_high) - log(_low)) * drand48() ); 
	    }
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << "," << getArg(2) << ")"; return output; }

	virtual const char * name() const { return "loguniform"; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _low : _high; }
	virtual LogUniform& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _low : _high) = arg; return *this; }
	virtual double getMean() const { return (_high - _low) / 2.0; } 
	virtual LogUniform& setMean( double mean ) { double delta = (_high - _low) / 2.0; _low = mean - delta; _high = mean + delta; return *this; }

    private:
	double _low;
	double _high;
    };

    class Deterministic : public RandomVariable
    {
    public:
	Deterministic( double value ) : RandomVariable(DETERMINISTIC), _value(value) {}
	virtual Deterministic * clone() const { return new Deterministic( getArg(1) ); }

	virtual double operator()() const { return _value; }
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << ")"; return output; }

	virtual const char * name() const { return "deterministic"; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _value; }
	virtual Deterministic& setArg( unsigned int i, double arg ) { assert( i == 1 ); _value = arg; return *this; }
	virtual double getMean() const { return _value; } 
	virtual Deterministic& setMean( double mean ) { _value = mean; return *this; }

    private:
	double _value;
    };


    class Poisson : public RandomVariable
    {
    public:
	Poisson( double mean ) : RandomVariable(DISCREET), _mean(mean) {}
	virtual Poisson * clone() const { return new Poisson( getArg(1) ); }

	virtual double operator()() const {
	    static Exponential exp_rv(1.0);

	    unsigned int x = 0;
	    for ( double a = 0.0; a < _mean; a += exp_rv() ) {
		x++;
	    }
	    return x - 1;
	}

	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << ")"; return output; }

	virtual const char * name() const { return "poisson"; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _mean; }
	virtual Poisson& setArg( unsigned int i, double arg ) { assert( i == 1 ); _mean = arg; return *this; }
	virtual double getMean() const { return _mean; } 
	virtual Poisson& setMean( double mean ) { _mean = mean; return *this; }

    private:
	double _mean;
    };

    class Normal : public RandomVariable
    {
    public:

	/* See Jain, Pg 494 - Convolution method with n = 12 */
	
	Normal( double mean, double stddev ) : RandomVariable(BOTH), _mean(mean), _stddev(stddev) {}
	virtual Normal * clone() const { return new Normal( getArg(1), getArg(2) ); }

	virtual double operator()() const 
	    {
		double sum = 0.0;
		for ( unsigned int i = 0; i < 12; ++i ) {
		    sum += drand48();
		}
		return _mean + _stddev * (sum - 6);
	    }
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << "," << getArg(2) << ")"; return output; }

	virtual const char * name() const { return "normal"; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _mean : _stddev; }
	virtual Normal& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _mean : _stddev) = arg; return *this; }
	virtual double getMean() const { return _mean; } 
	virtual Normal& setMean( double mean ) { _mean = mean; return *this; }

    private:
	double _mean;
	double _stddev;
    };

    class Gamma : public RandomVariable
    {
    public:
	/* Jain: pg 485 */
	Gamma( double a, double b ) : RandomVariable(CONTINUOUS), _a(a), _b(b) {}
	virtual Gamma * clone() const { return new Gamma( getArg(1), getArg(2) ); }

	virtual double operator()() const;
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << "," << getArg(2) << ")"; return output; }

	virtual const char * name() const { return "gamma"; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _a : _b; }
	virtual Gamma& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _a : _b) = arg; return *this; }
	virtual double getMean() const { return _a * _b; } 
	virtual Gamma& setMean( double mean ) { _a = mean / _b; return *this; }

    private:
	double _a;
	double _b;
    };


    class Beta : public RandomVariable
    {
    public:
	/* Jain: pg 485 */
	Beta( double a, double b ) : RandomVariable(CONTINUOUS), _a(a), _b(b) {}
	virtual Beta * clone() const { return new Beta( getArg(1), getArg(2) ); }

	virtual double operator()() const;
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << "," << getArg(2) << ")"; return output; }

	virtual const char * name() const { return "beta"; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _a : _b; }
	virtual Beta& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _a : _b) = arg; return *this; }
	virtual double getMean() const { return _a / (_a + _b); } 
	virtual Beta& setMean( double mean ) { assert( 0 < mean && mean < 1); _a = mean * _b / ( 1.0 - mean ); return *this; }

    private:
	double _a;
	double _b;
    };

    class Probability : public RandomVariable
    {
    public:
	/* Jain: pg 485 */
	Probability( double mean );
	virtual Probability * clone() const { return new Probability( getArg(1) ); }

	virtual double operator()() const { return drand48() < _mean ? 1.0 : 0.0; }
	virtual std::ostream& operator<<( std::ostream& output ) const { output << name() << "(" << getArg(1) << ")"; return output; }

	virtual const char * name() const { return "probability"; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _mean; }
	virtual Probability& setArg( unsigned int i, double arg );
	virtual double getMean() const { return _mean; } 
	virtual Probability& setMean( double mean ) { _mean = mean; return *this; }

    private:
	double _mean;
    };
}

#endif
