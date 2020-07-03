/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/randomvar.cc $
 *
 * Random variable manipulation functions.  There are two types, plain
 * old exponentials, and those represented by a discreet distribution
 * function.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January 2005.
 *
 * $Id: randomvar.cc 11963 2014-04-10 14:36:42Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <cmath>
#include <cstdlib>
#include "randomvar.h"
#if !defined(TESTDIST)
#include "lqns.h"
#include "pragma.h"
#endif
#if HAVE_GSL_GSL_MATH_H
#include <gsl/gsl_math.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#if HAVE_GSL_GSL_CDF_H
#include <gsl/gsl_cdf.h>
#endif
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_sf_exp.h>
#include <gsl/gsl_sf_gamma.h>
#endif

static double ab_term( const Erlang& a, const Erlang& b, const int offset );


/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Print out service time of entry in standard output format.
 */

ostream& 
operator<<( ostream& output, const Exponential& self )
{
    self.print( output );
    return output;
}



/*
 * Return a new aggregation record which is the product of the scalar
 * multiplier and the Exponential multiplicand.  Keep Cv2 constant.
 */

Exponential
operator*( const double multiplier, const Exponential& multiplicand )
{
    return Exponential( multiplicand.myMean     * multiplier,
			multiplicand.myVariance * square( multiplier ) );
}


/*
 * Add two records.
 */

Exponential
operator+( const Exponential& augend, const Exponential& addend )
{
    return Exponential( augend.myMean     + addend.myMean, 
			augend.myVariance + addend.myVariance );
}

/* -------------------- Global External functions --------------------- */

/*
 * Compute the probability that A is less than B.  See Mak. (8).
 * Only works for Erlang/Exponential type servers.
 */

Probability
Pr_A_lt_B( const Exponential& A, const Exponential& B )
{
    if ( A.mean() == 0.0 ) return 1.0;
    if ( B.mean() == 0.0 ) return 0.0;

    const Erlang a = A.erlang();
    const Erlang b = B.erlang();

    return ab_term( a, b, -1 );
}



/*
 * Return max of A and B.  See [Mak] pg, 264.
 */

DiscretePoints
max( const DiscretePoints& A, const DiscretePoints& B )
{
    DiscretePoints a_max = A;
    return a_max.max( B );
}



/*
 * Return max of A and B.  See [Mak] pg, 264.
 */

DiscretePoints
min( const DiscretePoints& A, const DiscretePoints& B )
{
    DiscretePoints a_min = A;
    return a_min.min( B );
}



/*
 * Second half of OR-FORK variance term.  See smith, pg 178.  
 */

Exponential
varianceTerm( const double p1, const Exponential& minuend, const double p2, const Exponential& subtrahend )
{
    Exponential delta;
    double difference = minuend.myMean - subtrahend.myMean;

    delta.myVariance += square( difference ) * (p1 * p2);

    return delta;
}



/*
 * Second half of REPEAT variance term.  See smith, pg 178.  Repeat
 * loops are geometrically distributed, hence n_{v} = 1.
 */

Exponential
varianceTerm( const Exponential& multiplicand )
{
    Exponential product;

    product.myVariance = square( multiplicand.myMean );  /* * variance of repeats */

    return product;
}

/* ---------------------- Static local functions ---------------------- */

/*
 * Common expression for max and Pr_A_lt_B functions.
 */

static double
ab_term( const Erlang& a, const Erlang& b, const int offset ) 
{
    const double x_A = 1.0 / a.a;
    const double x_B = 1.0 / b.a;
    double sum  = factorial( a.m + offset ) / factorial( a.m - 1 );
    double prod = 1.0;
    for ( unsigned k = 1; k < b.m; ++k ) {
	prod *= x_B / ( x_A + x_B );
	sum  += prod * factorial( a.m + k + offset ) / ( factorial( a.m - 1 ) * factorial( k ) );
    }	
    return pow( x_A / ( x_A + x_B ), static_cast<double>(a.m) ) / pow( x_A + x_B, static_cast<double>(offset+1) ) * sum;
}

/*----------------------------------------------------------------------*/
/*   Exponential record -- accumulates activity service and variance.   */
/*----------------------------------------------------------------------*/

/*
 * Assignement.
 */

Exponential&
Exponential::operator=( const Exponential& arg )
{
    myMean     = arg.myMean;
    myVariance = arg.myVariance;
    return *this;
}



/*
 * Scale a record.  See Repetition reduction rule from Smith, pg 178.
 */

Exponential&
Exponential::operator*=( const double multiplier )
{
    myVariance  *= square( multiplier );
    myMean      *= multiplier;

    return *this;
}



/*
 * Scale a record.  See Repetition reduction rule from Smith, pg 178.
 */

Exponential&
Exponential::operator/=( const double divisor )
{
    myVariance  /= square( divisor );
    myMean      /= divisor;

    return *this;
}



/*
 * For Or-Join.  Note that addVarianceTerm must also be called to
 * compute the correct variance.
 */

Exponential&
Exponential::operator+=( const Exponential& addend )
{
    myMean     += addend.myMean;
    myVariance += addend.myVariance;
    return *this;
}



Exponential& 
Exponential::operator+=( const double addend )
{
    myMean     += addend;
    return *this;
}


/*
 * For And-Join phase calculation. 
 */

Exponential&
Exponential::operator-=( const Exponential& subtrahend )
{
    myMean     -= subtrahend.myMean;
//  myVariance -= subtrahend.myVariance;
    // tomari, I believe if they are independent, the variance should be:
    //myVariance += subtrahend.myVariance; 
    // Yes -- but we have the total variance, and we're trying to compute some..

    return *this;
}



bool 
Exponential::operator==( const Exponential& arg ) const
{
    return myMean == arg.myMean && myVariance == arg.myVariance;
}

/*
 * Return max of A and B.  See [Mak] pg, 264.
 */

Exponential&
Exponential::max( const Exponential& arg ) 
{
    const Erlang a = erlang();
    const Erlang b = arg.erlang();

    const double m = mean() + arg.mean() - ab_term( a, b, 0 ) - ab_term( b, a, 0 );
    const double v = square( mean() ) + square( arg.mean() ) + variance() + arg.variance() 
	- square( m ) - ab_term( a, b, 1 ) - ab_term( b, a, 1 );

    myMean     = m;
    myVariance = v;
    return *this;
}


/*
 * Return min of A and B.  See [Mak] (9).
 */

Exponential&
Exponential::min( const Exponential& arg ) 
{
    myMean = 1.0 / (1.0 / mean() + 1.0 / arg.mean() );			/* [mak] (9) */
    myVariance = square( mean() );			/* !!! PUNT !!! */
    return *this;
}

/*
 * Return the scale parameter (a) and shape (m) to fit an erlang
 * distribution to the receiver's mean and variance.  Use exponential
 * if hyper-exponential.
 */

Erlang
Exponential::erlang() const
{
    static const double max_stages = 20.0;

    Erlang e;
    const double x2 = square( mean() );

    if ( x2 == 0.0 ) {
	e.a = 0.0;
	e.m = 1;
    } else {
	const double cv = variance() / x2;
	double m;
	if ( cv > 1.0 ) {
	    m = 1.0;		/* Punt -- exponential. */
	} else if ( cv < 1.0 / max_stages ) {
	    m = max_stages;		/* Too many stages - truncate. */
	} else {
	    m = floor( 1.0 / cv + 0.5 );
	}
	e.a = mean() / m;
	e.m = static_cast<int>(m);
    }

    return e;
}



/*
 * Print Mean and variances.
 */

ostream&
Exponential::print( ostream& output ) const
{
    output << typeStr() 
	   << " Mean:     " << mean()
	   << " Variance: " << variance();
    return output;
}

/*----------------------------------------------------------------------*/
/*                    AndJoin Exponential record                        */
/*----------------------------------------------------------------------*/

/*
 * Copy a record.
 */

DiscretePoints::DiscretePoints( const DiscretePoints& arg )
{
    t          = arg.t;
    A          = arg.A;

    myMean     = arg.myMean;
    myVariance = arg.myVariance;
    myName     = arg.myName;
}

DiscretePoints&
DiscretePoints::operator=( const DiscretePoints& arg )
{
    if ( this == &arg ) return *this;

    t          = arg.t;
    A          = arg.A;

    myMean     = arg.myMean;
    myVariance = arg.myVariance;
    myName     = arg.myName;

    return *this;
}


DiscretePoints::~DiscretePoints()
{
}


DiscretePoints& 
DiscretePoints::operator*=(const double scalar) 
{
    for (unsigned int i=1; i <= A.size();i++) {
	A[i] *= scalar;
    }
    return *this;
}


DiscretePoints& 
DiscretePoints::operator+=( const double addend )
{
    for (unsigned int i=1; i <= t.size();i++) {
	t[i] += addend;
    }
    return *this;
}


/*
 * Take product of arg.A and A.
 */

DiscretePoints&
DiscretePoints::operator*=( const DiscretePoints& arg )
{
    merge( arg );	/* make number of points in me match those in arg */

    const unsigned arg_n = arg.t.size();
    const unsigned n = t.size();

    for ( unsigned j = 1; j <= n; ++j ) {

	if ( arg.t[1] > t[j] ) {
	    A[j] = 0; 			/* arg.A[0] is implicitly 0 */
	} else {
	    for ( unsigned i = 2; i <= arg_n; ++i ) {
		if ( arg.t[i] > t[j] ) {
		    A[j] *= arg.A[i-1];
		    goto next;
		}
	    }
	    A[j] *= arg.A[arg_n];
	}
    next: ;
    }

    return *this;
}



void 
DiscretePoints::nameSet(char * newName)
{
    myName = newName;
}


/*
 * Find the three-point cummulative distribution function.
 */

DiscretePoints&
DiscretePoints::estimateCDF()
{
    double a[4];
    const double std_dev = sqrt( variance() );
    const double x = mean();

    if (x == 0 ) {
	return *this;
    }
    if ( std_dev == 0.0 ) {

	/* Special case -- deterministic... */

	if ( t.size() < 1 ) {
	    t.grow(1 - t.size());
	    A.grow(1 - A.size());
	}
	t[1] = x;
	A[1] = 1.0;
	return *this;
    }

    if ( t.size() < 3 ) {
	t.grow(3 - t.size());
	A.grow(3 - A.size());
    }

    /* Equation 4 */

    if ( x > std_dev ) {
	t[1] = x - std_dev;
    } else {
	t[1] = 0;
    }
    t[2] = x;
    if ( std_dev >= x ) {
	t[3] = x + 2 * variance() / x;
    } else {
	t[3] = x + 2 * std_dev;
    }

    /* Equation 5 */

    const double temp = variance() + square( x );
    const double delta = square( t[1] ) * (t[3] - t[2]) + square( t[2] ) * (t[1] - t[3])
	+ square( t[3] ) * (t[2] - t[1]);

    a[1] = ( temp * (t[3] - t[2]) + square( t[2] ) * (x  - t[3])
	     + square( t[3] ) * (t[2] - x) ) / delta;

#ifdef NOTDEF	// This term is not needed.
    a[2] = ( square( t[1] ) * (t[3] - x) + temp * (t[1] - t[3])
	     + square( t[3] ) * (x  - t[1]) ) / delta;
#endif

    a[3] = ( square( t[1] ) * (x  - t[2]) + square( t[2] ) * (t[1] - x)
	     + temp * (t[2] - t[1]) )  / delta;

    /* Equation 6 */

    /* A0 = 0.0 */
    A[1] = a[1];
    A[2] = 1.0 - a[3];
    A[3] = 1.0;

    // tomari quorum: to check the quality of the ThreePoint approximation.
    double tempMean = mean();
    double tempVariance = variance();
    meanVar();
    double errorInMean = 100 * (tempMean - mean() ) / tempMean;
    double errorInVariance = 100 * (tempVariance - variance() ) / tempVariance;

    if (errorInMean > 4.0 || errorInVariance > 0.4) {
	cout << "\nDiscretePoints::estimateCDF(): using THEEEPOINT ESTIMATION: BIG WARNING:" << endl;
	cout << "Perecntage of error in the mean of the estimated threads =" <<
	    errorInMean << "%." << endl;
	cout << "Perecntage of error in the variance of the estimated threads =" 
	     <<errorInVariance << "%." << endl;
	cout <<"This might lead to a significant error in the final results" << endl;
    }

    ////end tomari quorum

    return *this;
}



/*
 * The receiver will be the max of itself and arg by merging the CDF
 * in arg with the receiver.  Duplicates are rejected.  It is not a
 * good idea to mix max() and min() functions for the same receiver
 * object.
 */
// tomari: the max function really computes the product of the CDFs, and then it calculates
//the mean and variance. If the path have expoenential distribution then we use a closed formula
// otherwise we estimate it. 
DiscretePoints&
DiscretePoints::max( const DiscretePoints& arg )
{
    if ( t.size() == 0 ) {
	*this = arg;		/* First argument.  It is, by definition, the max. */
#if !defined(TESTDIST)
    } else if ( pragma.exponential_paths() ) {
	Exponential::max( arg );
#endif
    } else {
	(*this) *= arg;
	meanVar();
    }

    return *this;
}


//tomari qourum: added setCDF to fit a gamma distribution.
DiscretePoints&
DiscretePoints::setCDF(VectorMath<double> & ti, Vector<double> & Ai)
{
    assert(ti.size() == Ai.size());
    t.clear();
    A.clear();
    for (unsigned int i = 1; i <= ti.size() ; i++ ) {
	t.insert(i, ti[i]);
	A.insert(i, Ai[i]);
    }

    return *this;
}
DiscretePoints&
DiscretePoints::getCDF(VectorMath<double> & to, Vector<double> & CDFo)
{

    for (unsigned int i = 1; i <= t.size() ; i++ ) {
	to.insert(i, t[i]);
	CDFo.insert(i, A[i]);
    }

    return *this;
}


DiscretePoints&
DiscretePoints::pointByPointMul( const DiscretePoints& arg )
{
    const unsigned arg_n = arg.t.size();
    const unsigned n = t.size();
    unsigned i = 1, //index for this
	j = 1,      //index for arg
	k = 1;      //index for the result
    DiscretePoints result;
    if (arg_n == 0 || n == 0) {
	*this = result;
	return *this;
    }

    while (i <= n && j <= arg_n) { //cross product

	if (t[i] == arg.t[j]) {
	    result.A.insert(k, A[i]*arg.A[j]);
	    result.t.insert(k, t[i]);
	    i++; j++;
	}
	else if ((t[i] < arg.t[j])) {
	    result.A.insert(k, A[i]*((j > 1)?arg.A[j-1]:0));
	    result.t.insert(k, t[i]);
	    i++;
	}
	else {
	    result.A.insert(k, ((i > 1)?A[i-1]:0)*arg.A[j]);
	    result.t.insert(k, arg.t[j]);
	    j++;
	}
	k++;
    }

    while (i <= n) {
	result.A.insert(k, A[i] * arg.A[arg_n]);
	result.t.insert(k, t[i]);
	k++; i++;
    }

    while (j <= arg_n) {
	result.A.insert(k, A[n] * arg.A[j]);
	result.t.insert(k, arg.t[j]);
	k++;j++;
    }

    *this = result;

    return *this;
}


/*
 * The receiver will be the min of itself and arg by merging the CDF
 * in arg with the receiver.  Duplicates are rejected.
 */

DiscretePoints&
DiscretePoints::min( const DiscretePoints& arg )
{
    if ( t.size() == 0 ) {
	*this = arg;		/* First argument.  It is, by definition, the min. */
#if !defined(TESTDIST)    
    } else if ( pragma.exponential_paths() ) {
	Exponential::min( arg );
#endif
    } else {
	inverseMultiply( arg );
	meanVar();
    }

    return *this;
}

//tomari: Result = A + arg.A
DiscretePoints&
DiscretePoints::pointByPointAdd( const DiscretePoints& arg )
{

    const unsigned arg_n = arg.t.size();
    const unsigned n = t.size();
    unsigned i = 1, //index for this
	j = 1,      //index for arg
	k = 1;      //index for the result
    DiscretePoints result;
    if (arg_n == 0)
	return *this;
    else if (n == 0) {
	*this = arg;
	return *this;
    }

    while (i <= n && j <= arg_n) { //cross product

	if (t[i] == arg.t[j]) {
	    result.A.insert(k, A[i]+arg.A[j]);
	    result.t.insert(k, t[i]);
	    i++; j++;
	}
	else if ((t[i] < arg.t[j])) {
	    result.A.insert(k, A[i]+((j > 1)?arg.A[j-1]:0));
	    result.t.insert(k, t[i]);
	    i++;
	}
	else {
	    result.A.insert(k, ((i > 1)?A[i-1]:0)+arg.A[j]);
	    result.t.insert(k, arg.t[j]);
	    j++;
	}
	k++;
    }

    while (i <= n) {
	result.A.insert(k, A[i] + arg.A[arg_n]);
	result.t.insert(k, t[i]);
	k++; i++;
    }

    while (j <= arg_n) {
	result.A.insert(k, A[n] + arg.A[j]);
	result.t.insert(k, arg.t[j]);
	k++;j++;
    }

    *this = result;
    return *this;
}


DiscretePoints&
DiscretePoints::inverseMultiply( const DiscretePoints& arg )
{
    merge( arg );	/* make number of points in me match those in arg */

    negate();

    const unsigned arg_n = arg.t.size();
    const unsigned n = t.size();

    for ( unsigned j = 1; j <= n; ++j ) {

	if ( arg.t[1] > t[j] ) {
	    A[j] *= 1.0;		/* arg.A[0] is implicityly 1 after negate */
	} else {
	    for ( unsigned i = 2; i <= arg_n; ++i ) {
		if ( arg.t[i] > t[j] ) {
		    A[j] *= (1.0 - arg.A[i-1]);
		    goto next;
		}
	    }
	    A[j] *= (1.0 - arg.A[arg_n]);
	}
    next: ;
    }

    negate();
    return *this;
}



/*
 * Merge sort old and new `t' arrays.
 */
//tomari: this will give more t values. The new t values will have a cumulative
// probability depending on there locations on the CDF. 
// The number of differnt A values stays the same because we are fitting the arg.A 
//values in the current A. But the number of A values will be the same as the new number of t values.
DiscretePoints&
DiscretePoints::merge( const DiscretePoints& arg )
{
    const unsigned nPts = arg.t.size();

    unsigned j = 1;
    double lastA = 0.0;
    if ( t[1] == 0 ) {
	lastA = A[1];
    }

    for ( unsigned i = 1; i <= nPts; ++i ) {
	while ( j <= t.size() ) {		// Re-evaluate t.size each loop.
	    if ( arg.t[i] < t[j] ) {
		t.insert( j, arg.t[i] );
		A.insert( j, lastA );		// Copy term from last point to new point.
		goto nextA;			// points >= j shifted right.
	    } else if ( arg.t[i] == t[j] ) {
		//		lastA = A[j];			// Move up one in list.
		goto nextA;			// Duplicate -- ignore.
	    } else {
		lastA = A[j];			// Move up one in list.
		j += 1;
	    }
	}

	/* Fell off the end ... insert term anyway. */

	t.insert( j, arg.t[i] );
	A.insert( j, lastA );

    nextA: ;
    }
    return *this;
}



/*
 * Compute mean and variance.  We don't care about phases
 * so only phase 1 is set.  All other phases are cleared.
 */

DiscretePoints&
DiscretePoints::meanVar()
{
    const unsigned n = A.size();

    double lastA = 0.0;
    myMean = 0.0;		/* Clear everything. */

    for ( unsigned k = 1; k <= n; ++k ) {
	const Positive a_k = A[k] - lastA;		/* Weight */
	myMean += a_k * t[k];
	lastA = A[k];
    }

    lastA = 0;
    myVariance = 0.0;
    for ( unsigned k = 1; k <= n; ++k ) {
	const double a_k = A[k] - lastA;
	myVariance += a_k * square(t[k] - myMean);
	lastA = A[k];
    }

    return *this;
}

#if HAVE_LIBGSL
//closed form formula for threads with deterministic calls.
DiscretePoints& 
DiscretePoints::closedFormDetPoints(double avgNumCallsToLowerLevelTasks,double level1Mean, double level2Mean)
{
#if !defined(TESTDIST)    
    if (flags.trace_quorum) { 
	cout <<"\nOriginals for closedFormDetPoints: mean= " << mean() 
	     << ", variance=" << variance() << endl;
	cout <<" level1Mean= " << level1Mean << ", level2Mean=" << 
	    level2Mean << ", avgNumCallsToLowerLevelTasks="<<avgNumCallsToLowerLevelTasks<<endl;
    }
#endif

    VectorMath<double> ti;
    VectorMath<double> Ai;

    double time = 0;
    double tempMean =0;
    double tempSum  = 0;
    int index =0;
    double cdfValue =0;
    double previousCdfValue = 0;

    //Chebyshev's Inequality: page 219. "An introduction to Probability
    //Theory and its Applications", voulme 1, Wiiliam Feller, second
    //edition 1966.
    //Theorem: Let X be a random variable with mean mu=E(X) and variance
    // segma**2 = Var(X). Then for any t > 0:
    // P{abs(X- mu) >= t } <= segma**2 / t**2.

    double chebyshevProb = 0.999; //0.999;
    //The random variables are independent. The variance is the sum of their variances.
    //each distribution is a gamma
    double calcVariance=  level1Mean * level1Mean /(avgNumCallsToLowerLevelTasks +1)
	+ avgNumCallsToLowerLevelTasks * level2Mean * level2Mean;

    double maxSamplingTime =  sqrt(calcVariance)/ (1- chebyshevProb);
    //double maxSamplingTime = sqrt(variance()/ (1- chebyshevProb));

    double calcMean = level1Mean + avgNumCallsToLowerLevelTasks * level2Mean;
    double stepSize  = calcMean / 100;//100;  
    //double stepSize  =  mean()/ 100;  

    double threshold = calcMean * (1 - chebyshevProb) ;
    //double threshold = level2Mean * (1 - chebyshevProb) ;
    //double threshold = mean()  * (1 - chebyshevProb) ;

    //double p = 1.0 / ( avgNumCallsToLowerLevelTasks + 1.0 );

    for (time = 0.0 ,  index =1;  time <= maxSamplingTime 
	     /*&& cdfValue <= chebyshevProb */; time = time + stepSize) {

	// cout <<"Threshold = " << threshold << endl;
	cdfValue =   closedFormDet(time, avgNumCallsToLowerLevelTasks, 
				   level1Mean /(avgNumCallsToLowerLevelTasks +1), level2Mean);

	if (cdfValue > 1 || cdfValue < 0) {
	    cout <<"ERROR............time="<<time<<", cdfValue is =" <<cdfValue << endl;
	    continue;
	}

	if (((cdfValue - previousCdfValue) * time)  > threshold) {

	    tempMean = tempMean + (cdfValue - previousCdfValue) * (time);
	    tempSum = tempSum + (cdfValue - previousCdfValue) * (time * time);

	    // cout << "\nThread#" << i<< " ,N=" << N1 <<" ,K=" << K1 << " ,Time = " << time ;
	    // cout << "time="<< time<<", cdfValue=" << cdfValue << endl;

	    ti.insert(index, time); 
	    Ai.insert(index, cdfValue );

	    previousCdfValue = cdfValue ;    
	    index++;
	}
    }

    double errorInMean = 100 * (tempMean - calcMean ) / calcMean;

#if !defined(TESTDIST)    
    if (flags.trace_quorum) {
	cout <<"Maximum number of sampling points before  = " << maxSamplingTime / stepSize;
	cout <<", Actual number of sampling points= " << index << endl;

	//Calculate the variance.
	double tempVariance = tempSum - tempMean * tempMean;
	cout <<"Evaluated, after sampling, tempMean for Thread# " <<getNumber()<< " = " 
	     << tempMean << endl;
	cout <<"Percentage of error in mean value =" << errorInMean <<"%." <<endl;
	cout <<"Evaluated, after sampling, tempVariance for Thread# " << getNumber()
	     << " = " << tempVariance << endl;
    }
#endif

    if (fabs(errorInMean) > 4.0) {
	cout << "\nDiscretePoints::closedFormDetPoints(): BIG WARNING:" << endl;
	cout << "Perecntage of error in the mean of the estimated threads =" 
	     <<errorInMean << "%." << endl;
	cout <<"This might lead to a significant error in the final results" << endl;
	cout <<"You might need to increase chebyshevProb or decrease stepSize" << endl;
    }

    setCDF(ti, Ai);

#if !defined(TESTDIST)    
    if (flags.trace_quorum) {
	meanVar();
    }
#endif

    return *this;
}


//closed form points for threads with geometric calls.
DiscretePoints& 
DiscretePoints::closedFormGeoPoints( double avgNumCallsToLowerLevelTasks,double level1Mean, double level2Mean)
{
#if !defined(TESTDIST)    
    if (flags.trace_quorum) { 
	cout <<"\nOriginals for closedFormGeoPoints: mean= " << mean() 
	     << ", variance=" << variance() << endl;
	cout <<" level1Mean= " << level1Mean << ", level2Mean=" << 
	    level2Mean << ", avgNumCallsToLowerLevelTasks="<<avgNumCallsToLowerLevelTasks<<endl;
    }
#endif
    VectorMath<double> ti;
    VectorMath<double> Ai;

    //special case, there is no lower layer calls, then the service time is exponentially distributed.
    if (avgNumCallsToLowerLevelTasks == 0.0 || level2Mean==0.0 ) {
	mean(level1Mean);
	calcExpPoints();

#if !defined(TESTDIST)    
	if (flags.trace_quorum) {
	    cout <<"\nDiscretePoints::closedFormGeoPoints(): ERROR REPORTED:" << endl;
	    cout <<"avgNumCallsToLowerLevelTasks == 0.0" << endl;
	}  
#endif
	return *this; 
    }

    double time = 0;
    double tempMean =0;
    double tempSum  = 0;
    int index =0;
    double cdfValue =0;
    double previousCdfValue = 0;

    //Chebyshev's Inequality: page 219. "An introduction to Probability
    //Theory and its Applications", voulme 1, Wiiliam Feller, second
    //edition 1966.
    //Theorem: Let X be a random variable with mean mu=E(X) and variance
    // segma**2 = Var(X). Then for any t > 0:
    // P{abs(X- mu) >= t } <= segma**2 / t**2.

    double chebyshevProb = 0.999; //0.999;
    //exact formula for the variance of a closedform geometric.
    double calcVariance=  level1Mean * level1Mean 
	+ ((avgNumCallsToLowerLevelTasks+ 1) * (avgNumCallsToLowerLevelTasks+ 1) -1)
	* level2Mean * level2Mean;

    double maxSamplingTime = sqrt(calcVariance)/ (1- chebyshevProb);
    //double maxSamplingTime = sqrt(variance()/ (1- chebyshevProb));

    double calcMean = level1Mean + avgNumCallsToLowerLevelTasks * level2Mean;
    double stepSize  = calcMean / 100;//100;  
    //double stepSize  =  mean()/ 100;  

    double threshold = level2Mean * (1 - chebyshevProb) ;
    //double threshold = mean()  * (1 - chebyshevProb) ;

    double p = 1.0 / ( avgNumCallsToLowerLevelTasks + 1.0 );

    for (time = 0.0 ,  index =1;  time <= maxSamplingTime && cdfValue <= chebyshevProb
	     ; time = time + stepSize) {

	cdfValue = closedFormGeo(time, level1Mean *p, level2Mean, p);


	if (((cdfValue - previousCdfValue) * time)  > threshold) {

	    tempMean = tempMean + (cdfValue - previousCdfValue) * (time);
	    tempSum = tempSum + (cdfValue - previousCdfValue) * (time * time);

	    //  cout << "\nThread#" << i<< " ,N=" << N1 <<" ,K=" << K1 << " ,Time = " << time ;
	    // cout << " ,cdfValue=" << cdfValue;

	    ti.insert(index, time); 
	    Ai.insert(index, cdfValue );

	    previousCdfValue = cdfValue ;    
	    index++;
	}
    }

    double errorInMean = 100 * (tempMean - calcMean ) / calcMean;

#if !defined(TESTDIST)    
    if (flags.trace_quorum) {
	cout <<"Maximum number of sampling points before  = " << maxSamplingTime / stepSize;
	cout <<", Actual number of sampling points= " << index << endl;

	//Calculate the variance.
	double tempVariance = tempSum - tempMean * tempMean;
	cout <<"Evaluated, after sampling, tempMean for Thread# " <<getNumber()<< " = " 
	     << tempMean << endl;
	cout <<"Percentage of error in mean value =" << errorInMean <<"%." <<endl;
	cout <<"Evaluated, after sampling, tempVariance for Thread# " << getNumber()
	     << " = " << tempVariance << endl;
    }
#endif

    if (fabs(errorInMean) > 4.0) {
	cout << "\nDiscretePoints::closedFormGeoPoints(): BIG WARNING:" << endl;
	cout << "Perecntage of error in the mean of the estimated threads =" 
	     <<errorInMean << "%." << endl;
	cout <<"This might lead to a significant error in the final results" << endl;
	cout <<"You might need to increase chebyshevProb or decrease stepSize" << endl;
    }

    setCDF(ti, Ai);
#if !defined(TESTDIST)    
    if (flags.trace_quorum) {
	meanVar();
    }
#endif
    return *this;
}


////////Fitting Mean and Variance to a Gamma CDF distribution/////////////
//Gamma distribution is not defined for time = zero.
//This function is for backward compatability with other code.
DiscretePoints& 
DiscretePoints::calcGammaPoints( double mean1, double variance1,int K, int N)
{
    mean(mean1);
    variance(variance1);
    calcGammaPoints();
    return *this;
}

DiscretePoints& 
DiscretePoints::calcExpPoints()
{
    variance(mean() * mean());
    calcGammaPoints();
    return *this;
}


////////Fitting Mean and Variance to a Gamma CDF distribution/////////////
//Gamma distribution is not defined for time = zero.
DiscretePoints& 
DiscretePoints::calcGammaPoints()
{
#if !defined(TESTDIST)    
    if (flags.trace_quorum) {
	cout <<"Originals for gamma: mean= " << mean() << ", variance=" << variance() << endl;
    }
#endif
    assert(mean() >= 0  && variance() >=0 );

    VectorMath<double> ti;
    VectorMath<double> Ai;


    if ( variance() == 0.0 ) {
	ti.insert(1, mean()); 
	Ai.insert(1, 1 );
	setCDF(ti, Ai);

#if !defined(TESTDIST)    
	if (flags.trace_quorum) {
	    meanVar();
	    cout <<"after calling meanVar: mean=" << mean() 
		 <<" , variance = " << variance() << endl;
	}
#endif
	return *this; 
    } else if (mean() ==0) {
	//for submodel==0, especially in initialization the mean will be zero and variance
	//will be greater than zero.
	//  cout <<"\nDiscretePoints::calcGammaPoints(): ERROR REPORTED: variance=" << variance()
	//  <<" , mean = " << mean() << endl;
	//  cout <<"mean cannot be zero while variance is not zero" << endl;
	return *this; 
	//assert(0);
    }

    double time = 0;
    double tempMean =0;
    double tempSum  = 0;
    int index =0;
    double cdfValue=0;
    double previousCdfValue = 0;

    //Chebyshev's Inequality: page 219. "An introduction to Probability
    //Theory and its Applications", voulme 1, Wiiliam Feller, second
    //edition 1966.
    //Theorem: Let X be a random variable with mean mu=E(X) and variance
    // segma**2 = Var(X). Then for any t > 0:
    // P{abs(X- mu) >= t } <= segma**2 / t**2.

    double chebyshevProb = 0.999;
    double maxSamplingTime = sqrt(variance()/ (1- chebyshevProb));

    double stepSize  =  mean()/ 100;  
    double threshold = mean()  * (1 - chebyshevProb) ;


    for (time = 0 ,  index =1;  time <= maxSamplingTime && cdfValue <= chebyshevProb ; time = time + stepSize) {

	cdfValue = gammaCDF ( time, mean(), variance()   );  	  

	if (((cdfValue - previousCdfValue) * time)  > threshold) {

	    tempMean = tempMean + (cdfValue - previousCdfValue) * (time);
	    tempSum = tempSum + (cdfValue - previousCdfValue) * (time * time);

	    //  cout << "\nThread#" << i<< " ,N=" << N1 <<" ,K=" << K1 << " ,Time = " << time ;
	    // cout << " ,cdfValue=" << cdfValue;

	    ti.insert(index, time); 
	    Ai.insert(index, cdfValue );

	    previousCdfValue = cdfValue ;  
	    index++;
	}
    }


    //Calculate the variance.
    double tempVariance = tempSum - tempMean * tempMean;
    double errorInMean = 100 * (tempMean - mean() ) / mean();

#if !defined(TESTDIST)    
    if (flags.trace_quorum) {
	cout <<"Maximum number of Gamma sampling points before = " << maxSamplingTime / stepSize;
	cout <<", Actual number of sampling points =" << index << endl;
	cout <<"Evaluated, after sampling, tempMean for Thread# " <<getNumber()<< " = " 
	     << tempMean << endl;
	cout <<"Percentage of error in mean value =" << errorInMean <<"%." <<endl;
	cout <<"Evaluated, after sampling, tempVariance for Thread# " << getNumber()<< " = " 
	     << tempVariance << endl;
	cout <<"final: cdfValue - previousCdfValue= " <<  cdfValue - previousCdfValue << endl;
    }
#endif
    if (fabs(errorInMean) > 4.0) {
	cout << "\nDiscretePoints::calcGammaPoints(): BIG WARNING:" << endl;
	cout << "Perecntage of error in the mean of the estimated threads =" 
	     <<errorInMean << "%." << endl;
	cout <<"This might lead to a significant error in the final results" << endl;
	cout <<"You might need to increase chebyshevProb or decrease stepSize" << endl;
    }

    setCDF(ti, Ai);


    return *this;
}


double 
DiscretePoints::closedFormDet(double time, double avgNumCallsToLowerLevelTasks , 
			      double thetaC, double thetaS) 
{
    int k = (int) avgNumCallsToLowerLevelTasks;
    double b= 2.0 * thetaC * thetaS / (fabs(thetaC -thetaS));
    double c = (thetaC+thetaS) / fabs(thetaC-thetaS);
    double cdfValue=0;
    double constant = 0; 
    double A1plusA2 = 0;
    double A1minusA2 = 0;


    if (thetaC==0 && thetaS > 0) {
	//thetaC==thetaS, this is a gamma or Erlang distribution.

	// cout <<"thetaC==0 && thetaS > 	0"<< endl;
	double shape = k;   //shape is also called alpha
	//(using Tao's notation) or k (using Wikipedia's notation)
	double scale = thetaS;//scale is 1/beta or theta (in Wikipedia's notation)

	return gsl_cdf_gamma_P(time, shape, scale);
    } else if (thetaS==0 && thetaC > 0) {
	//thetaC==thetaS, this is a gamma or Erlang distribution.
	// cout << "thetaS==0 && thetaC > 0" << endl;
	double shape = k +1;   //shape is also called alpha
	//(using Tao's notation) or k (using Wikipedia's notation)
	double scale = thetaC;//scale is 1/beta or theta (in Wikipedia's notation)

	return gsl_cdf_gamma_P(time, shape, scale);
    }
    if (thetaC==0 && thetaS == 0) {
	//thetaC==thetaS, this is a gamma or Erlang distribution.
	// cout << "thetaC==0 && thetaS == 0" << endl;
	//may be at initialization
	return 1.0;
    }


    if (thetaC > thetaS) {
	constant =   pow((c*c -1 ), static_cast<double>(k)) * b /
	    (factorial(k) * pow(2.0,static_cast<double>(k+1)) * thetaC);

	//cout <<"thetaC > thetaS, time=" <<time<< endl;
	A1plusA2 = 0;
	double part1 =0;
	double part3 =0;
	double part4 =0;


	for (int i=0; i<=k; i++) {
	    double part2 = 0;
	    for (int j=0; j<=k-i; j++) {
		part2 +=  (j%2?-1:1) * pow((time/b), static_cast<double>(k-i-j)) /
		    (((j+1)%2?-1:1) * pow((c-1), static_cast<double>(j+1)) *factorial(k-i-j));
		//   cout <<"inside: part2=" << part2 << endl;
	    }
	    part1 += part2 * ((i%2?-1:1) * factorial(k-1+i)) /
		(factorial(i) * pow(2.0, static_cast<double>(i)));
	    //  cout <<"inside: part1=" << part1 << endl;
	}

	part1 = part1 * 2* k* exp((time/b)*(1-c));

	//cout <<"1. part1=" << part1 <<", part2=" << part2 << ", time="<< time<<
	//  ", k="<< k<< endl;

	part3 = 0;
	for (int i=1; i<=k; i++) {
	    part4 =0;
	    for (int j=0; j<=k-i; j++) {
		part4 += pow(time/b, static_cast<double>(k-i-j)) /
		    (pow((1+c), static_cast<double>(j+1)) *factorial(k-i-j));
		//   cout <<"inside: part4=" << part4 << endl;
	    }
	    part3 += part4 * factorial(k-1+i) /
		(factorial(i-1) * pow(2.0, static_cast<double>(i)) );
	    // cout <<"inside: part3=" << part3 << endl;
	}

	part3 = part3 * (k%2?-1:1) * 2* exp(-1.0*(time/b)*(1+c));


	//part5 and part6 are used to "manually" compoute capitalC.
	/*double part5 = 0;
	
	  for (int i = 0; i <=k; i++) {
	  part5 += (factorial(k+i-1) /
	  ( factorial(i) * pow(2.0,i) * ((k-i+1)%2?-1:1) * pow(c-1, k-i+1)));
	  }
	
	  part5 *= ((k+1)%2?-1:1)*2*k;
	
	  double part6 = 0;
	  for (int i = 1; i <= k; i++) {
	  part6 += factorial(k+i-1)/
	  (factorial(i-1)*pow(2.0,i) *pow(1+c,k-i+1));
	  }
	
	  part6 *=((k+1)%2?-1:1)* 2.0;
	
	  // double capitalC= (part5 + part6 )   ;
	  cout << "capitalC= (part5 + part6 )=" << (part5 + part6 ) << endl;
	*/


	double capitalC= 1.0/constant;

	//cout << "capitalC= 1/constant=" << 1.0/constant << endl;

	A1plusA2 = part1 + part3;
	cdfValue = constant * (A1plusA2 + capitalC);

	//  cout <<"time=" << time << ", cdfValue=" << cdfValue << endl;

	if (cdfValue < 0 || cdfValue > 1) {
	    cout <<"DETAILED ERROR: cdfValue=" << cdfValue << ", time=" << time <<
		", constant=" << constant<< endl;
	    cout <<"3. part1=" << part1 << ", part3=" << part3
		 <<", A1plusA2=" << A1plusA2 << ", capitalC=" << capitalC << endl;
	}
    } else if (thetaC < thetaS) {
	constant =   pow((c*c -1 ), static_cast<double>(k)) * b /
	    (factorial(k) * pow(2.0,static_cast<double>(k+1)) * thetaC);

	//  cout <<"DiscretePoints::closedFormDet(): thetaC < thetaS, time=" <<time<< endl;
	A1minusA2 = 0;
	double part1 =0;
	double part3 =0;
	double part4 =0;

	for (int i=1; i<=k; i++) {
	    double part2 = 0;
	    for (int j=0; j<=k-i; j++) {
		part2 +=  (j%2?-1:1) * pow((time/b), static_cast<double>(k-i-j)) /
		    (((j+1)%2?-1:1) * pow((c-1), static_cast<double>(j+1)) *factorial(k-i-j));
		//   cout <<"inside: part2=" << part2 << endl;
	    }
	    part1 += part2 * ((i%2?-1:1) * factorial(k-1+i)) /
		(factorial(i-1) * pow(2.0, static_cast<double>(i)));
	    //  cout <<"inside: part1=" << part1 << endl;
	}

	part1 = part1 * -1 * 2* exp((time/b)*(1-c));

	//cout <<"1. part1=" << part1 <<", part2=" << part2 << ", time="<< time<<
	//  ", k="<< k<< endl;

	part3 = 0;
	for (int i=0; i<=k; i++) {
	    part4 =0;
	    for (int j=0; j<=k-i; j++) {
		part4 += pow(time/b, static_cast<double>(k-i-j)) /
		    (pow((1+c), static_cast<double>(j+1)) *factorial(k-i-j));
		//   cout <<"inside: part4=" << part4 << endl;
	    }
	    part3 += part4 * factorial(k-1+i) /
		(factorial(i) * pow(2.0, static_cast<double>(i)) );
	    // cout <<"inside: part3=" << part3 << endl;
	}

	part3 = part3 * (k%2?-1:1) * 2* k* exp(-1.0*(time/b)*(1+c));

	double capitalC =0;

	//part5 and part6 are used to manuallay compute capitalC.

	/*double part5 =0;
	  double part6=0;
	
	  for (int i=1; i<= k; i++) {
	  part5 += factorial(k+i-1) /(factorial(i-1)*pow(2.0,i) 
	  * pow(1-c, k-i+1));
	  }
	
	  part5= part5 * pow(-1.0,k) * 2;
	
	  for (int i=0; i<=k; i++) {
	  part6 += factorial(k+i-1) /(factorial(i)*pow(2.0,i) 
	  * pow(1+c, k-i+1));
	  }
	
	  part6= part6 * pow(-1.0,k) * 2* k;
	
	  capitalC=part5 + part6;
	  //cout << "capitalC=part5 + part6=" << part5 + part6 << endl;
	  */

	///////////////////////////
	capitalC= 1.0/constant;
	//cout <<"capitalC= 1.0/constant= " << 1.0/constant<< endl;

	A1minusA2 = part1 - part3;
	cdfValue = constant * (A1minusA2 + capitalC);

	//  cout <<"\ntime=" << time << ", cdfValue=" << cdfValue <<  
	//	", constant=" << constant<< endl;
	//	cout <<"3. part1=" << part1 << ", part3=" << part3 << endl;
	//   cout << "A1minusA2=" << A1minusA2 <<", capitalC=" << capitalC << endl;

	if (cdfValue < 0 || cdfValue > 1) {
	    cout <<"DETAILED ERROR: cdfValue=" << cdfValue << ", time=" << time <<
		", constant=" << constant<< endl;
	    cout <<"3. part1=" << part1 << ", part3=" << part3
		 << ", A1minusA2=" << A1minusA2 <<", capitalC=" << capitalC << endl;
	}
    } else {
	//thetaC==thetaS, this is a gamma or an Erlang distribution.
	double shape = 2*k +1;   //shape is also called alpha
	//(using Tao's notation) or k (using Wikipedia's notation)
	double scale = thetaC;//scale is 1/beta or theta (in Wikipedia's notation)

	return gsl_cdf_gamma_P(time, shape, scale);
    }

    return cdfValue;
}



double 
DiscretePoints::closedFormGeo(double x, double theta1, double theta2, double p) {
    double diff = fabs(theta1-theta2);
    double sum = theta1+theta2;
    double prod = theta1*theta2;

    if (theta1 != 0.0) {

	double alpha = sqrt(sum*sum - 4*p*prod)/diff;
	double lambda1 = 1/theta1;
	double constant = p*theta2/(alpha*diff);
	double gamma = lambda1 - (sum/(2*prod));
	double beta = sqrt(sum*sum - 4*p*prod)/(2*prod);

	return constant*
	    ( (gamma+beta)*exp((gamma-lambda1-beta)*x)/(gamma-lambda1-beta)
	      - (gamma-beta)*exp((gamma-lambda1+beta)*x)/(gamma-lambda1+beta) )
	    - constant*
	    ( (gamma+beta)/(gamma-lambda1-beta)- (gamma-beta)/(gamma-lambda1+beta));

    } else {
	// level1mean ==0
	return (p + (1-p) * (1- exp(-p*x/theta2)));
    }
}


double 
DiscretePoints::gammaCDF ( double time, double mean, double variance   )
{
    const double shape = (mean * mean) /variance;   //shape is also called alpha
    const double scale = variance/mean;//scale is 1/beta or theta (in Wikipedia's notation)

    //using the gsl library.

    return gsl_cdf_gamma_P(time, shape, scale);
}


DiscretePoints&
DiscretePoints::gammaMeanVar()
{
    cout <<"\n  DiscretePoints::gammaMeanVar(): Error " <<  "Implement me before calling me" << endl;

    return *this;
}
#endif

/*
 * Perform the operation 1 - A.
 */

DiscretePoints&
DiscretePoints::pointByPointNegate()
{
    if (t[1] > 0) {
	A.grow(1); 
	t.grow(1);

	for ( unsigned j = t.size(); j > 1 ; j-- ) {
	    A[j] = A[j-1];
	    t[j] = t[j-1];
	}

	t[1] = 0;
	A[1] = 0;
    }

    const  unsigned  n = t.size();

    for ( unsigned j = 1; j <= n; ++j ) {
	A[j] = 1.0 - A[j];
    }

    return *this;
}

DiscretePoints&
DiscretePoints::negate()
{
    const unsigned n = t.size();
    for ( unsigned j = 1; j <= n; ++j ) {
	A[j] = 1.0 - A[j];
    }
    return *this;
}



DiscretePoints
DiscretePoints::negate( const DiscretePoints& arg ) 
{
    DiscretePoints rv = arg;
    rv.negate();
    return rv;
}


ostream&
DiscretePoints::print( ostream& output ) const
{
    output << "t: " << t << endl
	   << "A: " << A << endl;
    return Exponential::print( output );
}

/***********************************************************************/
//tomari: Quorum

DiscreteCDFs::DiscreteCDFs()
{
}

DiscretePoints* DiscreteCDFs::quorumKofNRecursive(DiscretePoints*** MemoizingTable, unsigned k, unsigned n) 
{
    if (!MemoizingTable[k][n]) {
	MemoizingTable[k][n] = new DiscretePoints;
	if (k==1) {
	    if (n==1) {
		*MemoizingTable[k][n] = *myCDFsCltn[1];
	    }
	    else {
		*MemoizingTable[k][n] = *quorumKofNRecursive(MemoizingTable, 1, n-1);
		MemoizingTable[k][n]->pointByPointAdd(*myCDFsCltn[n]);
	    }
	}
	else if (k == n) {
	    *MemoizingTable[k][n] = *quorumKofNRecursive(MemoizingTable, k-1, n-1);
	    MemoizingTable[k][n]->pointByPointMul(*myCDFsCltn[n]);
	}
	else if (n > k) {
	    *MemoizingTable[k][n] = *quorumKofNRecursive(MemoizingTable, k-1, n-1);
	    MemoizingTable[k][n]->pointByPointMul(*myCDFsCltn[n]);
	    MemoizingTable[k][n]->pointByPointAdd(*quorumKofNRecursive(MemoizingTable, k, n-1));
	}
    }
    return MemoizingTable[k][n];
}

//Return the joint CDF for K out of N distributions.
DiscretePoints * 
DiscreteCDFs::quorumKofN( const unsigned K, const unsigned N )
{
#if !defined(TESTDIST)    
    if (pragma.exponential_paths()) {
	throw not_implemented( "DiscreteCDFs::quorumKofN", __FILE__, __LINE__ );
    }
#endif

    DiscretePoints * totalCDF = new DiscretePoints();  

    DiscretePoints*** MemoizingTable = new DiscretePoints**[N+1];
    for (unsigned int i=0; i <= N; i++) { //initialization.
	MemoizingTable[i] = new DiscretePoints*[N+1];
	for (unsigned int j=0; j <= N; j++) {
	    MemoizingTable[i][j] = NULL;
	}
    }
    if ( myCDFsCltn.size() == N ) {

	for (unsigned int h = 1 ; h <= myCDFsCltn.size(); h++) {

#if !defined(TESTDIST)    
	    if (flags.trace_quorum) {
		cout << "\nDiscreteCDFs::quorumKofN(): myCDFsCltn.size() =" 
		     << myCDFsCltn.size() << endl;
		cout <<"myCDFsCltn["<<h<<"]->mean()=" << myCDFsCltn[h]->mean()
		     <<", Variance=" <<myCDFsCltn[h]->variance() << endl; 

	    }
#endif

	    VectorMath<double> ti;
	    VectorMath<double> Ai;
	    myCDFsCltn[h]->getCDF(ti,Ai);


	    if (ti.size() == 0 && Ai.size() == 0) {
		ti.insert(1, 0); 
		Ai.insert(1, 1 );
		myCDFsCltn[h]->setCDF(ti, Ai);
	    }
	}


	for (unsigned int i =K ; i <= N; i++ ) {
	    DiscretePoints term = *quorumKofNRecursive(MemoizingTable, i, N);
	    double fact = factorial(i -1 ) / (factorial(K-1) * factorial(i - K )) ;
	    term *= (((i-K) % 2)?-1:1) * fact; 
	    *totalCDF = totalCDF->pointByPointAdd(term);
	}
	totalCDF->meanVar();
    }

    for (unsigned int i=0; i <= N; i++) { //initialization.
	for (unsigned int j=0; j <= N; j++) {
	    delete  MemoizingTable[i][j];
	}
	delete[]  MemoizingTable[i];
    }
    delete[]  MemoizingTable;

    return totalCDF;
}



void 
DiscreteCDFs::delCDFs()
{
    myCDFsCltn.deleteContents();
}

DiscretePoints& 
DiscreteCDFs::addCDF( DiscretePoints& aCDF)
{
    DiscretePoints *  newCDF = new DiscretePoints();

    *newCDF = aCDF;

    myCDFsCltn << newCDF; 

    return aCDF;

}


DiscreteCDFs::~DiscreteCDFs()
{
    myCDFsCltn.deleteContents();
}

/****************************************************************/
