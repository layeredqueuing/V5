/* -*- c++ -*-
 * Interface to the random number generator.  Subclasses generate
 * random numbers with the distribution specified.  The superclass
 * number() generates a uniformly distributed random numer between
 * [0,1).
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * Nov 2024.
 *     
 * $Id: task.h 17410 2024-10-31 13:54:12Z greg $
 */

#ifndef	LQSIM_RANDOM_H
#define LQSIM_RANDOM_H

#include <random>

class Random
{
protected:
    Random() {}

public:
    virtual ~Random() {}
    virtual double operator()() = 0;
    static double number() { return __f(__generator); }
    static void seed( unsigned int value ) { __generator.seed( value ); }

private:
    static std::random_device __random_device;
    static std::uniform_real_distribution<double> __f;	/* uniform from 0 to 1 */

public:
    static std::mt19937 __generator;
};


class Constant : public Random
{
public:
    Constant( double value ) : Random(), _value(value) {}
    virtual ~Constant() {}

    double operator()() { return _value; }

private:
    const double _value;
};


class Exponential : public Random
{
    /* The std::exponential_distribution takes a rate, not a time */
public:
    Exponential( double mean ) : Random(), _f(1.0/mean) {}
    virtual ~Exponential() {}

    double operator()() { return _f(__generator); }

private:
    std::exponential_distribution<double> _f;
};



class Gamma : public Random
{
public:
    /* alpha = k, beta = scale (1/lambda) */
    Gamma( double alpha, double beta ) : Random(), _f(alpha,beta) {}
    virtual ~Gamma() {}

    double operator()() { return _f(__generator); }

private:
    std::gamma_distribution<double> _f;
};



class HyperExponential : public Random
{
public:
    HyperExponential( double mean, double cv_sqr ) : Random(), _mean(mean), _cv_sqr(cv_sqr), _f(0.,1.) {}
    virtual ~HyperExponential() {}

    double operator()();

private:
    const double _mean;
    const double _cv_sqr;
    std::uniform_real_distribution<double> _f;
};



class Pareto : public Random
{
public:
    Pareto( double scale, double shape ) : Random(), _scale(scale), _shape(shape), _f(0.,1.) {}
    virtual ~Pareto() {}

    double operator()();
    
private:
    const double _scale;
    const double _shape;
    std::uniform_real_distribution<double> _f;
};



class Uniform : public Random
{
public:
    Uniform( double minimum, double maximum ) : Random(), _f(minimum,maximum) {}
    virtual ~Uniform() {}

    double operator()() { return _f(__generator); }

private:
    std::uniform_real_distribution<double> _f;
};
#endif
