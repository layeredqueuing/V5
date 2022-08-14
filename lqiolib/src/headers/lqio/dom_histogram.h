/* -*- c++ -*-
 *  $Id: dom_histogram.h 15813 2022-08-11 19:18:33Z greg $
 *
 *  Created by Martin Mroz on 07/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_HISTOGRAM__
#define __LQIO_DOM_HISTOGRAM__

#include "input.h"
#include "dom_object.h"
#include <vector>

namespace LQIO {
    namespace DOM {
	class Histogram : public DocumentObject {
	private:
	    Histogram( const Histogram& ) = delete;
	    Histogram& operator=( const Histogram& ) = delete;
	    
	public:
	    enum class Type { CONTINUOUS, DISCRETE };
	    
	    class HistogramBin {
		friend class Histogram;
	    public:
		HistogramBin( double begin=0., double end=0., double mean=0., double variance=0. )
		    : _begin(begin), _end(end), _mean(mean), _variance(variance) {}

		bool operator<( const HistogramBin& arg ) { return _begin < arg._begin; }
		void initialize( double begin, double end ) { _begin = begin; _end = end; _mean = 0.0; _variance = 0.0; }

	    private:
		double _begin;
		double _end;
		double _mean;
		double _variance;
	    };
      
	    /* Constructor for the basic histogram type */
	    Histogram(const Document *, Type type, unsigned n_bins, double min, double max );
	    virtual ~Histogram();
      
	    /* Accessors and Mutators */
	    const char * getTypeName() const { return __typeName; }

	    void capacity( unsigned int n_bins, double min, double max );
	    unsigned int getBins() const { return _n_bins; }
	    double getMin() const { return _min; }
	    double getMax() const { return _max; }
	    double getBinBegin( unsigned int ) const;
	    double getBinSize() const { return _bin_size; }
	    double getBinEnd( unsigned int ) const;
	    double getBinMean( unsigned int ) const;
	    double getBinVariance( unsigned int ) const;
	    Type getHistogramType() const { return _histogram_type; }
	    Histogram& setBinMeanVariance( unsigned int, double, double=0 );
	    void setTimeExceeded( double );		/* Must be of type isTimeExceeded */
	    double getTimeExceeded() const;		/* Must be of type isTimeExceeded */

	    /* Queries */
	    unsigned int getBinIndex( double ) const;
	    unsigned int getOverflowIndex() const { return _n_bins + 1; }

	    bool isTimeExceeded() const { return _n_bins == 0 && _min == _max; }
	    bool isHistogram() const { return _n_bins > 0; }
	    bool hasResults() const { return _has_results; }

	private:
	    unsigned int _n_bins;
	    double _min;
	    double _max;
	    double _bin_size;
	    bool _has_results;
	    const Type _histogram_type;
	    std::vector<HistogramBin> _bins;

	public:
	    static const char * __typeName;
	};
    }
}

#endif /* __LQIO_DOM_HISTOGRAM__ */
