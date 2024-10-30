/* -*- c++ -*-
 *
 * ------------------------------------------------------------------------
 * $Id: result.h 17396 2024-10-28 14:18:18Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef LQSIM_RESULT_H
#define LQSIM_RESULT_H

extern unsigned long number_blocks;	/* For block statistics. 	*/
static inline double square( const double arg ) { return arg * arg; }

namespace LQIO {
    namespace DOM {
	class DocumentObject;
    }
}

class Result {
    friend class Instance; 		// Old interface for processors.

protected:
    Result( int type, const std::string& name, LQIO::DOM::DocumentObject * dom );

public:
    virtual ~Result() {}
    typedef LQIO::DOM::DocumentObject& (LQIO::DOM::DocumentObject::*set_fn)( const double );

    void init();
    void init( const long id );			// Old Parasol interface for processors
    virtual void record( double ) = 0;		/* record a sample.		*/

    double accumulate();
    void accumulate_variance ( const double );
    void accumulate_service( const Result& r_cycle );
    void accumulate_utilization( const Result& r_cycle, const double service_time );
    
    void reset();		/* Result the raw counter	*/     
    void clear_results();	/* Clear everything.		*/
    bool has_results() const { return _count > 0.; }

    double mean() const;
    double variance() const;
    double mean_count() const;
    double variance_count() const;
    static double conf95( const unsigned );
    static double conf99( const unsigned );

    const Result& insertDOMResults( set_fn, set_fn=nullptr ) const;
    std::ostream& print( std::ostream& ) const;

private:
    std::string getName( const std::string& ) const;

protected:
    long _raw;			/* index to raw value.		*/
    int _type;

private:
    LQIO::DOM::DocumentObject * _dom;
    const std::string _name;

    double _sum;		/* Sum of values.		*/
    double _sum_sqr;		/* Sum of squares		*/
    double _count;		/* Number of items.		*/
    double _count_sqr;		/* Number of items.		*/
    double _avg_count;		/* Average or count.		*/
    unsigned _n;		/* Number of hits in sample	*/
};

class SampleResult : public Result
{
public:
    SampleResult( const std::string& name, LQIO::DOM::DocumentObject * dom ) : Result( SAMPLE, name, dom ) {}

    virtual void record( double );		/* record a sample.		*/
    void add( double );				/* Add preemption time		*/
};

class VariableResult : public Result
{
public:
    VariableResult( const std::string& name, LQIO::DOM::DocumentObject * dom ) : Result( VARIABLE, name, dom ) {}

    virtual void record( double );		/* record a sample.		*/
};

inline std::ostream& operator<<( std::ostream& output, const Result& self ){ return self.print( output ); }
#endif
