/* -*- c++ -*-
 *
 * ------------------------------------------------------------------------
 * $Id: result.h 17479 2024-11-15 21:03:38Z greg $
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
namespace Instance {
    class Instance;
}

class Result {
    friend class Instance::Instance; 		// Old interface for processors.

protected:
    Result( const std::string& name, LQIO::DOM::DocumentObject * dom ) :  _dom(dom), _name(getName(name)), _sum(0.), _sum_sqr(0.), _count(0.), _count_sqr(0.), _avg_count(0.), _n(0), _resid(0.) {}

public:
    virtual ~Result() {}
    typedef LQIO::DOM::DocumentObject& (LQIO::DOM::DocumentObject::*set_fn)( const double );

    virtual void record( double ) = 0;	/* record a sample.		*/

    double accumulate();
    void accumulate_variance ( const double );
    void accumulate_service( const Result& r_cycle );
    void accumulate_utilization( const Result& r_cycle, const double service_time );
    
    virtual void reset();		/* Result the raw counter	*/     
    void clear_results();		/* Clear everything.		*/
    bool has_results() const { return _count > 0.; }

    double mean() const;
    double variance() const;
    double mean_count() const;
    double variance_count() const;
    static double conf95( const unsigned );
    static double conf99( const unsigned );

    const Result& insertDOMResults( set_fn, set_fn=nullptr ) const;
    std::ostream& print( std::ostream& ) const;

protected:
    virtual double getMean() const = 0;
    virtual double getOther() const = 0;
    virtual const std::string& getTypeName() const = 0;
    double now() const;
    
private:
    std::string getName( const std::string& ) const;

private:
    LQIO::DOM::DocumentObject * _dom;
    const std::string _name;

    double _sum;			/* Sum of values.		*/
    double _sum_sqr;			/* Sum of squares		*/
    double _count;			/* Number of items.		*/
    double _count_sqr;			/* Number of items.		*/
    double _avg_count;			/* Average or count.		*/
    unsigned _n;			/* Number of hits in sample	*/

protected:
    double _resid;			/* non-rounding resid 		*/
};

class SampleResult : public Result
{
public:
    SampleResult( const std::string& name, LQIO::DOM::DocumentObject * dom ) : Result( name, dom ), _count(0), _sum(0.) {}

    virtual void record( double );	/* record a sample.		*/
    void add( double );			/* Add preemption time		*/
    virtual void reset();

protected:
    virtual double getMean() const { return _count > 0 ? _sum / static_cast<double>(_count) : 0.; }
    virtual double getOther() const { return static_cast<double>(_count); }
    virtual const std::string& getTypeName() const { return __type_name; }

private:
    static std::string __type_name;

    long _count;			/* sample count			*/
    double _sum;			/* sample sum			*/
};

class VariableResult : public Result
{
public:
    VariableResult( const std::string& name, LQIO::DOM::DocumentObject * dom ) : Result( name, dom ), _start(0.), _old_value(0.), _old_time(0.), _integral(0.) {}

    virtual void record( double );	/* record a sample.		*/
    void record_offset( double, double );
    virtual void reset();

protected:
    virtual double getMean() const;
    virtual double getOther() const;
    virtual const std::string& getTypeName() const { return __type_name; }

private:
    static std::string __type_name;

    double _start;			/* start time			*/
    double _old_value;			/* previous value		*/
    mutable double _old_time;		/* previous time		*/
    mutable double _integral;		/* variable integral		*/
};

#if HAVE_PARASOL
class ParasolResult : public VariableResult
{
public:
    ParasolResult( const std::string& name, LQIO::DOM::DocumentObject * dom ) : VariableResult( name, dom ), _raw(-1) {}
    
    void init( const long id );		// Old Parasol interface for processors
    int getRawID() const { return _raw; }
    virtual void reset();

protected:
    virtual double getMean() const;
    virtual double getOther() const;
    
private:
    long _raw;				/* index to raw value.		*/
};
#endif

inline std::ostream& operator<<( std::ostream& output, const Result& self ){ return self.print( output ); }
#endif
