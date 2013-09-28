/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May  1996.								*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $Id$
 */

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <cstdio>
#include <lqio/input.h>
#include <lqio/dom_histogram.h>

class Histogram
{
private:
    Histogram( const Histogram& );
    Histogram& operator=( const Histogram& );

    class hist_bin {
    public:
	hist_bin() : bin(0), sum(0), sum_sqr(0) {}
	void reset() { bin = 0.; sum = 0.; sum_sqr = 0.; }

	double bin;
	double sum;
	double sum_sqr;
    };

public:
    Histogram( const LQIO::DOM::Histogram * );
    void reset();

    void accumulate_data();
    void insert(const double value);

    void insertDOMResults();
    
private:
    unsigned int overflow_bin() const { return _n_bins + 1; }

    double mean( const unsigned i ) const;
    double variance( const unsigned i ) const;

private:
    LQIO::DOM::Histogram * _histogram;	/* */
    const unsigned int _n_bins;		/* Number of bins */
    const double _bin_size;             /* Size of each bin */
    const double _min;                  /* Lower range limit on the histogram */
    const double _max;                  /* Upper range limit on the histogram */
    unsigned _n;			/* Number of blocks. */
    double _count;			/* Total count in all bins */
    vector<hist_bin> _hist;
};
#endif

