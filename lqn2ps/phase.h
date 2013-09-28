/* -*- c++ -*-
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PHASE_H)
#define PHASE_H

#include "cltn.h"
#include "vector.h"
#include <lqio/input.h>

class Call;
class Entry;
class Entity;
class Task;
class PhaseManip;

class Phase {

public:
    class Histogram {
    public:
	class Bin {
	public:
	    double myBegin;    
	    double myEnd;
	    double myProb;
	    double myConf95;
	    double myConf99;
	};

	Histogram();
	Phase::Histogram& set( const double min, const double max, const unsigned n_bins );
	Phase::Histogram& moments( const double, const double, const double, const double );
	Phase::Histogram& addBin( const double begin, const double end, const double prob, const double conf95, const double conf99 );
	bool hasHistogram() const { return myNumBins > 2 && myMax > 0; }
	bool maxServiceTime() const { return myNumBins == 2 && myMax > 0; }

    private:
	unsigned myNumBins;		/* Number of bins */
	double myMin;                   /* Lower range limit on the histogram */
	double myMax;                   /* Upper range limit on the histogram */

	double myMean;			/* Distribution results. */
	double myStdDev;
	double mySkew;
	double myKurtosis;

	Vector2<Phase::Histogram::Bin> bins;	/* */
    };

private:
    Phase( const Phase& );		/* Copying is verbotten */ 

public:
    Phase();
    virtual ~Phase();
    Phase& operator=( const Phase& );
	
    int operator==( const Phase& aPhase ) const { return &aPhase == this; }

    /* Initialialization */
	
    void initCv_sqr( const double );
    void initialize( Entry * src, const unsigned p );
    virtual void check() const;
    const Phase& setChain( const unsigned curr_k, const Entity * aServer, callFunc aFunc ) const;

    /* Instance variable access */
	
    Phase& setDOM( const LQIO::DOM::Phase * dom ) { _documentObject = dom; return *this; }
    virtual const LQIO::DOM::Phase * getDOM() const { return _documentObject; }
    int phase() const { return myPhase; }
    const Entry * entry() const { return myEntry; }
    virtual const Task * owner() const;

    virtual const string& name() const;

    Phase& phaseTypeFlag( phase_type aType );
    phase_type phaseTypeFlag() const;

    bool hasServiceTime() const;
    const LQIO::DOM::ExternalVariable& serviceTime() const;

    bool hasThinkTime() const;
    const LQIO::DOM::ExternalVariable& thinkTime() const;

    bool hasCV_sqr() const;
    const LQIO::DOM::ExternalVariable& Cv_sqr() const;

    double executionTime() const;
    double variance() const;
    double serviceExceeded() const;
    double queueingTime() const;
    double utilization() const;

    bool hasQueueingTime() const;
    bool isNonExponential() const;

    bool hasHistogram() const { return myHistogram.hasHistogram(); }
    Phase& histogram( const double min, const double max, const unsigned n_bins );
    Phase& moments( const double, const double, const double, const double );
    Phase& histogramBin( const double begin, const double end, const double prob, const double conf95, const double conf99 );
    double maxServiceTime() const;

    virtual const Cltn<Call *>& callList() const;
    virtual const Entry * rootEntry() const { return 0; }

    virtual bool hasCallsFor( unsigned p ) const;

    virtual double serviceTimeForSRVNInput() const;
    virtual double serviceTimeForQueueingNetwork() const;

    const Phase& addThptUtil( double &util_sum ) const;
	
protected:
    Phase& recomputeCv_sqr( const Phase * );

protected:
    Histogram myHistogram;		/* Histogram information	*/

private:
    const LQIO::DOM::Phase * _documentObject;
    Entry *myEntry;
    unsigned myPhase;
};

ostream& operator<<( ostream&, const Phase& );
ostream& operator<<( ostream&, const Phase::Histogram::Bin& );

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class PhaseManip {
public:
    PhaseManip( ostream& (*ff)(ostream&, const Phase * ), const Phase * thePhase )
	: f(ff), anPhase(thePhase) {}
private:
    ostream& (*f)( ostream&, const Phase* );
    const Phase * anPhase;

    friend ostream& operator<<(ostream & os, const PhaseManip& m ) 
	{ return m.f(os,m.anPhase); }
};
#endif
