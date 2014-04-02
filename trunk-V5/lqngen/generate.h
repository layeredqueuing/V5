/* -*- c++ -*-
 * generate.h	-- Greg Franks
 *
 * $Id$
 *
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <lqio/dom_document.h>
#include <lqio/dom_extvar.h>

using namespace std;

namespace LQIO {
    namespace DOM {
	class Document;
	class ActivityList;
	class Activity;
	class Phase;
	class Entity;
	class Processor;
	class Group;
	class Task;    
	class Entry;
	class Call;
    }
}

#if !HAVE_DRAND48
static double drand48() 
{
    return static_cast<double>(rand())/static_cast<double>(RAND_MAX);
}
#endif



class Generate 
{
private:
    class IntegerManip {
    public:
	IntegerManip( std::ostream& (*f)(std::ostream&, const int ), const int i ) : _f(f), _i(i) {}
    private:
	std::ostream& (*_f)( std::ostream&, const int );
	const int _i;

	friend std::ostream& operator<<(std::ostream & os, const IntegerManip& m ) { return m._f(os,m._i); }
    };


    class ProgramManip {
    public:
	ProgramManip( std::ostream& (*f)(std::ostream&, const LQIO::DOM::Document& d, const int, const string& s ), const LQIO::DOM::Document& d, const int i, const string& s ) : _f(f), _d(d), _i(i), _s(s) {}
    private:
	std::ostream& (*_f)( std::ostream&, const LQIO::DOM::Document&, const int, const string& );
	const LQIO::DOM::Document& _d;
	const int _i;
	const string _s;

	friend std::ostream& operator<<(std::ostream & os, const ProgramManip& m ) { return m._f(os,m._d,m._i,m._s); }
    };


    typedef LQIO::DOM::ExternalVariable * (Generate::*variateFunc)( const opt_values, const string& ) const;

    class ModelVariable {
	friend class Generate;

    private:
//	ModelVariable( const ModelVariable& );
//	ModelVariable& operator=( const ModelVariable& );

    protected:
	typedef void (ModelVariable::*variableValueFunc)( const LQIO::DOM::ExternalVariable&, const opt_values ) const;

	ModelVariable( Generate& model, variableValueFunc f ) : _model(model), _f(f) {}

	void scalar( const LQIO::DOM::ExternalVariable& var, const opt_values index ) const;
	void vector( const LQIO::DOM::ExternalVariable& var, const opt_values index ) const;
	void index( const LQIO::DOM::ExternalVariable& var, const opt_values index ) const;

	Generate& _model;
	const variableValueFunc _f;
    };

    class GetProcessorVariable : public ModelVariable {

    public: 
	GetProcessorVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Processor * );
    };
	
    class SetProcessorVariable : public ModelVariable {

    public: 
	SetProcessorVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Processor * );
    };
	
    class GetTaskVariable : public ModelVariable {
    public:
	GetTaskVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Task * );
    };
	
    class SetTaskVariable : public ModelVariable {
    public:
	SetTaskVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Task * );
    };
	
    class GetEntryVariable : public ModelVariable {
    public:
	GetEntryVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Entry * );
    };
	
    class SetEntryVariable : public ModelVariable {
    public:
	SetEntryVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Entry * );
    };
	
    class GetCallVariable : public ModelVariable {
    public:
	GetCallVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Call * );
    };
	
    class SetCallVariable : public ModelVariable {
    public:
	SetCallVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const LQIO::DOM::Call * );
    };
	
    typedef void (Generate::*parameterize_fptr)( const ModelVariable::variableValueFunc );

public:
    Generate( const unsigned runs );
    Generate( const unsigned runs, LQIO::DOM::Document * );
    virtual ~Generate();

    unsigned long getNumberOfRuns() const { return _runs; }

    void fixed( variateFunc );
    void random( variateFunc );
    void reparameterize();

    ostream& print( ostream& ) const;

    static bool getGeneralArgs( char * options );

    static option_type opt[];
    static const char * model_opts[];
    static const char * comment;

    static double exponential( double offset, double mean ) { return offset - (mean - offset) * log( drand48() ); }
    static double pareto( double offset, double mean ) { return offset + pow( drand48(), -mean / ( mean - 1.0 ) ); }
    static double uniform( double low, double high ) { return low + (high - low) * drand48(); }
    static double loguniform( double low, double high ) { return exp( uniform( log(low), log(high) ) ); }
    static double deterministic( double, double value ) { return value; }
    static double probability( double, double mean ) { return mean; }

    LQIO::DOM::ExternalVariable * get_constant( const opt_values, const string& ) const;
    LQIO::DOM::ExternalVariable * get_variable( const opt_values, const string& ) const;
    static double get_value( const opt_values );

private:

    LQIO::DOM::Processor * addProcessor( const string&, const scheduling_type sched_flag, LQIO::DOM::ExternalVariable * n_copies );
    LQIO::DOM::Task * addTask( const string& name, const scheduling_type sched_flag, LQIO::DOM::ExternalVariable * n_copies, 
			       const vector<LQIO::DOM::Entry *>& entries, LQIO::DOM::Processor * aProcessor, LQIO::DOM::ExternalVariable * think_time=0 );
    LQIO::DOM::Entry * addEntry( const string& name, unsigned n_phases, variateFunc );
    LQIO::DOM::Call * addCall( LQIO::DOM::Entry *, LQIO::DOM::Entry *, variateFunc  );

    void addLQX( parameterize_fptr f, const ModelVariable::variableValueFunc g );
    void serialize( const ModelVariable::variableValueFunc f );
    void serialize2( const ModelVariable::variableValueFunc f );

    template <class Type> void shuffle( vector<Type>& );

    static std::ostream& printIndent( std::ostream& output, const int i );
    static std::ostream& printHeader( std::ostream& output, const LQIO::DOM::Document& d, const int i, const string&  );
    static std::ostream& printResults( std::ostream& output, const LQIO::DOM::Document& d, const int i, const string&  );
    static IntegerManip indent( const int i ) { return IntegerManip( &printIndent, i ); }
    static ProgramManip print_header( const LQIO::DOM::Document& d, const int i, const string& s="" ) { return ProgramManip( &printHeader, d, i, s ); }
    static ProgramManip print_results( const LQIO::DOM::Document& d, const int i, const string& s="" ) { return ProgramManip( &printResults, d, i, s ); }

    /* default values */
    static unsigned iteration_limit;
    static unsigned print_interval;
    static double convergence_value;
    static double underrelaxation;

    static LQIO::DOM::ConstantExternalVariable * ONE;
    static LQIO::DOM::ConstantExternalVariable * ZERO;

    static double double_rv( opt_values );
    static unsigned unsigned_rv( unsigned );
    static bool choose( unsigned );

    LQIO::DOM::Document * _document;
    const unsigned _runs;

    vector<vector<LQIO::DOM::Task *> > _task;
    vector<vector<LQIO::DOM::Entry *> > _entry;
    vector<LQIO::DOM::Processor *> _processor;
    vector<LQIO::DOM::Call *> _call;

    ostringstream _program;
};


inline ostream& operator<<( ostream& output, const Generate& self ) { return self.print( output ); }
