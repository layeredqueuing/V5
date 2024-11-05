/* -*- c++ -*-
 * generate.h	-- Greg Franks
 *
 * $Id: randomvar.h 17431 2024-11-05 10:08:21Z greg $
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
#include <random>

namespace RV {
    class RandomVariable
    {
    public:
	typedef enum { CONSTANT, BOTH, CONTINUOUS, DISCREET, PROBABILITY } distribution_t;

	RandomVariable( distribution_t type ) : _type(type) {}
	virtual ~RandomVariable() {}
	virtual RandomVariable * clone() const = 0;

	virtual double operator()() const = 0;
	std::ostream& print( std::ostream& ) const;
	virtual const char * name() const = 0;
	virtual unsigned int nArgs() const = 0;
	virtual double getArg( unsigned int ) const = 0;
	virtual RandomVariable& setArg( unsigned int, double ) = 0;
	virtual RandomVariable& setArg( unsigned int, const std::string& );
	virtual double getMean() const = 0; 
	virtual RandomVariable& setMean( double ) = 0;
	RandomVariable& setMean( const std::string& );
	virtual distribution_t getType() const { return _type; }

	static double number() { return __f(__generator); }
	static void seed( unsigned int value ) { __generator.seed( value ); }

    private:
	const distribution_t _type;

	static std::random_device __random_device;
	static std::uniform_real_distribution<double> __f;	/* uniform from 0 to 1 */
	static std::mt19937 __generator;
    };

    class Exponential : public RandomVariable
    {
    public:
	Exponential( double a ) : RandomVariable(CONTINUOUS), _a(a) {}
	virtual Exponential * clone() const { return new Exponential( getArg( 1 ) ); }

	virtual double operator()() const { return  -_a * log( number() ); }

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _a; }
	virtual Exponential& setArg( unsigned int i, double arg ) { assert( i == 1 ); _a = arg; return *this; }
	virtual double getMean() const { return 1.0 / _a; } 
	virtual Exponential& setMean( double mean ) { _a = 1.0/mean; return *this; }

    public:
	static const char * const __name;

    private:
	double _a;
    };

    class Pareto : public RandomVariable
    {
    public:
	Pareto( double a ) : RandomVariable(CONTINUOUS), _a(a) {}
	virtual Pareto * clone() const { return new Pareto( _a ); }

	virtual double operator()() const { return 1.0 / pow( number(), 1.0 / _a ); }

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _a; }
	virtual Pareto& setArg( unsigned int i, double arg ) { assert( i == 1 ); _a = arg; return *this; }
	virtual double getMean() const { return _a / (_a - 1.0); } 
	virtual Pareto& setMean( double mean ); 

    public:
	static const char * const __name;

    private:
	double _a;
    };

    class Uniform : public RandomVariable
    {
    public:
	Uniform( double low, double high ) : RandomVariable(BOTH), _low(low), _high(high) {}
	virtual Uniform * clone() const { return new Uniform( _low, _high ); }

	virtual double operator()() const { return _low + (_high - _low) * number(); }

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _low : _high; }
	virtual Uniform& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _low : _high) = arg; return *this; }
	virtual double getMean() const { return static_cast<double>(_high - _low) / 2.0; } 
	virtual Uniform& setMean( double mean ) { _high = (mean - _low) * 2.0 + _low; return *this; }

    public:
	static const char * const __name;

    private:	
	double _low;
	double _high;
    };

    class LogUniform  : public RandomVariable
    {
    public: 
	LogUniform( double low, double high ) : RandomVariable(CONTINUOUS), _low(low), _high(high) {}
	virtual LogUniform * clone() const { return new LogUniform( _low, _high ); }

	virtual double operator()() const 
	    { 
		const double log_low = log(_low);
		return exp( log_low + (log(_high) - log(_low)) * number() ); 
	    }

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _low : _high; }
	virtual LogUniform& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _low : _high) = arg; return *this; }
	virtual double getMean() const { return (_high - _low) / 2.0; } 
	virtual LogUniform& setMean( double mean ) { _high = (mean - _low) * 2.0 + _low; return *this; }

    public:
	static const char * const __name;

    private:
	double _low;
	double _high;
    };

    class Constant : public RandomVariable
    {
    public:
	Constant( double value ) : RandomVariable(CONSTANT), _value(value) {}
	virtual Constant * clone() const { return new Constant( _value ); }

	virtual double operator()() const { return _value; }

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _value; }
	virtual Constant& setArg( unsigned int i, double arg ) { assert( i == 1 ); _value = arg; return *this; }
	virtual double getMean() const { return _value; } 
	virtual Constant& setMean( double mean ) { _value = mean; return *this; }

    public:
	static const char * const __name;

    private:
	double _value;
    };


    class Normal : public RandomVariable
    {
    public:

	/* See Jain, Pg 494 - Convolution method with n = 12 */
	
	Normal( double mean, double stddev ) : RandomVariable(CONTINUOUS), _mean(mean), _stddev(stddev) {}
	virtual Normal * clone() const { return new Normal( _mean, _stddev ); }

	virtual double operator()() const 
	    {
		double sum = 0.0;
		for ( unsigned int i = 0; i < 12; ++i ) {
		    sum += number();
		}
		return _mean + _stddev * (sum - 6);
	    }

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _mean : _stddev; }
	virtual Normal& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _mean : _stddev) = arg; return *this; }
	virtual double getMean() const { return _mean; } 
	virtual Normal& setMean( double mean ) { _mean = mean; return *this; }

    public:
	static const char * const __name;

    private:
	double _mean;
	double _stddev;
    };

    class Gamma : public RandomVariable
    {
    public:
	/* Jain: pg 485 */
	Gamma( double a, double b ) : RandomVariable(CONTINUOUS), _a(a), _b(b) {}	/* Scale,shape*/
	virtual Gamma * clone() const { return new Gamma( _a, _b ); }

	virtual double operator()() const;

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _a : _b; }
	virtual Gamma& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _a : _b) = arg; return *this; }
	virtual double getMean() const { return _a * _b; } 
	virtual Gamma& setMean( double mean ) { _a = mean / _b; return *this; }

    public:
	static const char * const __name;

    private:
	double _a;
	double _b;
    };


    class Beta : public RandomVariable
    {
    public:
	/* Jain: pg 485 */
	Beta( double a, double b ) : RandomVariable(CONTINUOUS), _a(a), _b(b) {}
	Beta( double mean ) : RandomVariable(CONTINUOUS), _a(mean), _b(mean) {}
	Beta& operator=( double mean ) { return setMean( mean ); }
	virtual Beta * clone() const { return new Beta( _a, _b ); }

	virtual double operator()() const;

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return i == 1 ? _a : _b; }
	virtual Beta& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _a : _b) = arg; return *this; }
	virtual double getMean() const { return _a / (_a + _b); } 
	virtual Beta& setMean( double mean );

    public:
	static const char * const __name;

    private:
	double _a;
	double _b;
    };

    class Poisson : public RandomVariable
    {
    public:
	Poisson( double mean, double offset ) : RandomVariable(DISCREET), _mean(mean), _offset(offset) {}
	virtual Poisson * clone() const { return new Poisson( _mean, _offset ); }

	virtual double operator()() const {
	    unsigned int x = _offset;
#if 0
	    static Exponential exp_rv(1.0);

	    for ( double a = 0.0; a < _mean; a += exp_rv() ) {
		x++;
	    }
#else
	    const double L = exp( -_mean );
	    double p = 1.0;
	    do {
		x += 1;
		p *= number();
	    } while ( p > L );
#endif
	    return x - 1;
	}

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return (i == 1 ? _mean : _offset); }
	virtual Poisson& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _mean : _offset) = arg; return *this; }
	virtual double getMean() const { return _mean; } 
	virtual Poisson& setMean( double mean ) { _mean = mean - _offset; return *this; }

    public:
	static const char * const __name;

    private:
	double _mean;
	double _offset;
    };

    class Binomial : public RandomVariable
    {
    public:
	Binomial( double low, double high ) : RandomVariable(DISCREET), _low(low), _high(high) {}
	virtual Binomial * clone() const { return new Binomial( _low, _high ); }

	virtual double operator()() const {
	    static Exponential exp_rv(1.0);

	    unsigned int x = 0;
	    for ( unsigned int i = 0; i < _high; i += 1 ) {
		if ( number() < 0.5 ) x++;
	    }
	    return x + _low;
	}

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 2; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 || i == 2 ); return (i == 1 ? _low : _high); }
	virtual Binomial& setArg( unsigned int i, double arg ) { assert( i == 1 || i == 2 ); (i == 1 ? _low : _high) = arg; return *this; }
	virtual double getMean() const { return (_high - _low) / 2.0; } 
	virtual Binomial& setMean( double mean ) { _high = ceil( (mean - _low) * 2 ) + _low; return *this; }

    public:
	static const char * const __name;

    private:
	double _low;
	double _high;
    };

    class Probability : public RandomVariable
    {
    public:
	/* Jain: pg 485 */
	Probability( double mean ) : RandomVariable(PROBABILITY), _mean(mean) {}
	Probability& operator=( double prob ) { return setMean( prob ); }
	virtual Probability * clone() const { return new Probability( _mean ); }

	virtual double operator()() const { return number() < _mean ? 1.0 : 0.0; }

	virtual const char * name() const { return __name; }
	virtual unsigned int nArgs() const { return 1; }
	virtual double getArg( unsigned int i ) const { assert( i == 1 ); return _mean; }
	virtual Probability& setArg( unsigned int i, double arg ) { assert ( i == 1 ); _mean = arg; return *this; }
	virtual double getMean() const { return _mean; } 
	virtual Probability& setMean( double mean );  // { _mean = mean; return *this; }

    public:
	static const char * const __name;

    private:
	double _mean;
    };

    inline std::ostream& operator<<( std::ostream& output, const RandomVariable& self ) { return self.print( output ); }
}

#endif
