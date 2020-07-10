/* -*- C++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/randomvar.h $
 * randomvar.h	-- Greg Franks
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January, 2005
 *
 * $Id: randomvar.h 13676 2020-07-10 15:46:20Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_AGGR_H
#define LQNS_AGGR_H

#include "vector.h"
#include "prob.h"
#include <string>
	  
class Exponential;
class DiscretePoints;

Probability Pr_A_lt_B( const Exponential&, const Exponential& );
DiscretePoints max( const DiscretePoints&, const DiscretePoints& );
DiscretePoints min( const DiscretePoints&, const DiscretePoints& );


ostream& operator<<( ostream&, const Exponential& );

struct Erlang
{
    double a;
    unsigned m;
};

class Exponential
{
    friend Exponential operator*( const double, const Exponential& );
    friend Exponential operator+( const Exponential&, const Exponential& );

    friend Exponential varianceTerm( const double, const Exponential&, const double, const Exponential& );
    friend Exponential varianceTerm( const Exponential& );
	
public:
    Exponential() : myMean(0.0), myVariance(0.0) {}
    Exponential( const double m, const double v ) : myMean(m), myVariance(v) {}
    virtual ~Exponential() {}

    Exponential& operator=( const Exponential& );
    Exponential& operator+=( const Exponential& );
    Exponential& operator+=( const double );
    Exponential& operator-=( const Exponential& );
    Exponential& operator*=( const double );
    Exponential& operator/=( const double );
    bool operator==( const Exponential& ) const;
	
    /* Return values to entries */
	
    double mean() const { return myMean; }
    double variance() const { return myVariance; }
    Exponential& max( const Exponential& );
    Exponential& min( const Exponential& );
    Exponential& mean( const double m ) { myMean = m; return *this; }
    Exponential& variance( const double v ) { myVariance = v; return *this; }
    Exponential& init( const double m, const double v ) { myMean = m; myVariance = v; return *this; }

    void  setNumber(int threadNumber) { myNumber = threadNumber;};

    int getNumber() { return myNumber; };

    Erlang erlang() const;

    /* Printing... */

    virtual ostream& print( ostream& output ) const;
	
protected:
    virtual const char * typeStr() const { return "Exponential: "; }
    double gammaCDF (double time, double mean, double variance   );

protected:
    double myMean;
    Positive myVariance;
    int myNumber; 
};

/* -------------------------------------------------------------------- */

class DiscreteCDFs
{
public:
    DiscreteCDFs();
    ~DiscreteCDFs();
    
    DiscretePoints * quorumKofN( const unsigned k, const unsigned n );
    DiscretePoints * quorumKofNRecursive(DiscretePoints*** MemoizingTable, unsigned k, unsigned n);

    DiscretePoints& addCDF( DiscretePoints&);

private:
    Vector<DiscretePoints *>  myCDFsCltn; 
};

/* -------------------------------------------------------------------- */

class DiscretePoints : public Exponential
{
    friend class DiscreteCDFs;
public:
    DiscretePoints() : Exponential(), isActive(false) {}
    DiscretePoints( const double m, const double v ) : Exponential( m, v ), isActive(false)
	{ estimateCDF(); }

    DiscretePoints( const DiscretePoints& );
    DiscretePoints& operator=( const DiscretePoints& );
    virtual ~DiscretePoints();
    
    const string& nameGet() const { return myName; }
    void nameSet( char * );
    bool active() const { return isActive; }
    void active(bool newState){isActive = newState;}

    DiscretePoints& estimateCDF();

    DiscretePoints& max( const DiscretePoints& );
    DiscretePoints& min( const DiscretePoints& );
    DiscretePoints& setCDF(VectorMath<double> & t, Vector<double> & A);
    DiscretePoints& getCDF(VectorMath<double> & to, Vector<double> & CDFo);

    DiscretePoints& meanVar();
#if HAVE_LIBGSL
    DiscretePoints& gammaMeanVar();
    DiscretePoints& calcGammaPoints( double mean, double variance, int K, int N);
    DiscretePoints& calcGammaPoints( );
    DiscretePoints& calcExpPoints();

    DiscretePoints& closedFormGeoPoints( double avgNumCallsToLowerLevelTasks, double level1Mean,
					 double level2Mean);
    double closedFormGeo(double x, double theta1, double theta2, double p);

    DiscretePoints& closedFormDetPoints(double avgNumCallsToLowerLevelTasks,double level1Mean, double level2Mean);

    double closedFormDet(double time, double avgNumCallsToLowerLevelTasks , double thetaC, double thetaS);
#endif

    /* Printing... */ 

    virtual ostream& print( ostream& output ) const;
	
protected:
    virtual const char * typeStr() const { return "DiscretePoints: "; };
#if HAVE_LIBGSL
    double gammaCDF ( double time, double mean, double variance   );
#endif
 
private:
    DiscretePoints& merge( const DiscretePoints& );

    static DiscretePoints negate( const DiscretePoints& );
    DiscretePoints& negate();
    DiscretePoints& pointByPointNegate();

    DiscretePoints& inverseMultiply( const DiscretePoints& arg );

public:  
    DiscretePoints& operator+=( const double );
    DiscretePoints& operator+=( const Exponential& arg ) { Exponential::operator+=( arg ); return *this; }
    DiscretePoints& operator*=( const DiscretePoints& );
    DiscretePoints& pointByPointAdd( const DiscretePoints& );//tomari
    DiscretePoints& pointByPointMul( const DiscretePoints& arg );// tomari
    DiscretePoints& operator*=(const double scalar); //tomari
 
protected:
    VectorMath<double> t;  //time random variable points that correspond to 
    //the probability points in vector A.
    VectorMath<double> A; // estimated probability points in the CDF for the
    //points in vector t.

private:
    bool isActive;
    string myName;
};
#endif
