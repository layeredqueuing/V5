/*
 *  $Id: dom_histogram.cpp 11090 2012-07-11 12:12:57Z greg $
 *
 *  Created by Martin Mroz on 07/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 * Histograms.  The are n_bins + 2 bins.  The 0 and n_bins+1 bins are
 * for underflow and overflow respectively.  
 */

#include "dom_histogram.h"
#include "dom_document.h"

namespace LQIO {
    namespace DOM {

	Histogram::Histogram( const Document * document, histogram_t type, unsigned n_bins, double min, double max, const void * histogram_element ) 
	    : DocumentObject( document, "", histogram_element ),
	      _n_bins(n_bins), _min(min), _max(max), _bin_size(0),
	      _bins(), _histogram_type(type), _has_results(false)
	{
	    capacity( n_bins, min, max );
	}
    
	Histogram::~Histogram()
	{
	}

	/* 
	 * 
	 */

	void Histogram::capacity( unsigned int n_bins, double min, double max )
	{
	    _n_bins = n_bins;
	    _min = min;
	    _max = max;
	    _bins.resize(n_bins+2);
	    _bin_size = n_bins > 0 ? (max - min)/static_cast<double>(n_bins) : 0.0;

	    _bins[0].initialize( 0, min );		// underflow
	    for ( unsigned i = 0; i < n_bins; ++i ) {
		const double x1 = min + static_cast<double>(i) * getBinSize();
		_bins[i+1].initialize( x1, x1 + getBinSize() );
	    }
	    _bins[getOverflowIndex()].initialize( max, 0 );	// overflow
	}

	/*
	 * Return the bin associated with x.  Overflow and underflow are signalled by values out of range.
	 */

	unsigned int Histogram::getBinIndex( double x ) const 
	{
	    if ( x < getMin() ) {
		return 0;
	    } else if ( getMax() <= x ) {
		return getOverflowIndex();
	    } else {
		return static_cast<unsigned int>((x - getMin()) / getBinSize()) + 1;
	    }
	}

	double Histogram::getBinBegin( unsigned int ix ) const
	{	
	    if ( getOverflowIndex() < ix ) {
		throw std::range_error( "Histogram::getBinBegin" );
	    }
	    return _bins[ix]._begin;
	}

	double Histogram::getBinEnd( unsigned int ix ) const
	{	
	    if ( getOverflowIndex() < ix ) {
		throw std::range_error( "Histogram::getBinEnd" );
	    }
	    return _bins[ix]._end;
	}

	double Histogram::getBinMean( unsigned int ix ) const
	{	
	    if ( getOverflowIndex() < ix ) {
		throw std::range_error( "Histogram::getBinMean" );
	    }
	    return _bins[ix]._mean;
	}

	double Histogram::getBinVariance( unsigned int ix ) const
	{	
	    if ( getOverflowIndex() < ix ) {
		throw std::range_error( "Histogram::getBinVariance" );
	    }
	    return _bins[ix]._variance;
	}


	Histogram& Histogram::setBinMeanVariance( unsigned int ix, double mean, double variance ) 
	{
	    _has_results = true;
	    if ( isMaxServiceTime() ) {
		const_cast<Document *>(getDocument())->setHasMaxServiceTime(true);
	    } else {
		const_cast<Document *>(getDocument())->setHasHistogram(true);
	    }

	    if ( getOverflowIndex() < ix ) {
		throw std::range_error( "Histogram::setBinMeanVariance" );
	    } 
	    _bins[ix]._mean = mean;
	    _bins[ix]._variance = variance;
	    return *this;
	}
    }
}
