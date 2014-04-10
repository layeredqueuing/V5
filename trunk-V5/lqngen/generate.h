/* -*- c++ -*-
 * generate.h	-- Greg Franks
 *
 * $Id$
 *
 */

#if !defined(GENERATE_H)
#define GENERATE_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <lqio/dom_document.h>
#include <lqio/dom_extvar.h>
#include "randomvar.h"

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


    class ModelVariable {
	friend class Generate;

    private:
//	ModelVariable( const ModelVariable& );
//	ModelVariable& operator=( const ModelVariable& );

    protected:
	typedef void (ModelVariable::*variableValueFunc)( const LQIO::DOM::ExternalVariable&, const RV::RandomVariable * ) const;

	ModelVariable( Generate& model, variableValueFunc f ) : _model(model), _f(f) {}

	void lqx_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void lqx_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void lqx_function( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void lqx_sensitivity( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const;
	void lqx_index( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void spex_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void spex_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void spex_sensitivity( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const;

#if __cplusplus >= 201103L
    private:
#endif
	Generate& _model;

    protected:
	const variableValueFunc _f;
    };

    class MakeProcessorVariable : public ModelVariable {

    public: 
	MakeProcessorVariable( Generate& model, variableValueFunc f, RV::RandomVariable * multiplicity ) : ModelVariable( model, f ), _multiplicity(multiplicity) {}
	void operator()( LQIO::DOM::Processor * ) const;

    private:
	RV::RandomVariable * _multiplicity;
    };
	
    class SetProcessorVariable : public ModelVariable {

    public: 
	SetProcessorVariable( Generate& model, variableValueFunc f, const RV::RandomVariable * multiplicity ) : ModelVariable( model, f ), _multiplicity(multiplicity) {}
	void operator()( const LQIO::DOM::Processor * ) const;

    private:
	const RV::RandomVariable * _multiplicity;
    };
	
    class MakeTaskVariable : public ModelVariable {
    public:
	MakeTaskVariable( Generate& model, variableValueFunc f, const RV::RandomVariable * customers, const RV::RandomVariable * think_time, const RV::RandomVariable * multiplicity ) 
	    : ModelVariable( model, f ), _customers(customers), _think_time(think_time), _multiplicity(multiplicity) {}
	void operator()( LQIO::DOM::Task * ) const;

    private:
	const RV::RandomVariable * _customers;
	const RV::RandomVariable * _think_time;
	const RV::RandomVariable * _multiplicity;
    };
	
    class SetTaskVariable : public ModelVariable {
    public:
	SetTaskVariable( Generate& model, variableValueFunc f, const RV::RandomVariable * customers, const RV::RandomVariable * think_time, const RV::RandomVariable * multiplicity ) 
	    : ModelVariable( model, f ), _customers(customers), _think_time(think_time), _multiplicity(multiplicity) {}
	void operator()( const LQIO::DOM::Task * ) const;

    private:
	const RV::RandomVariable * _customers;
	const RV::RandomVariable * _think_time;
	const RV::RandomVariable * _multiplicity;
    };
	
    class MakeEntryVariable : public ModelVariable {
    public:
	MakeEntryVariable( Generate& model, variableValueFunc f, const RV::RandomVariable * service_time  ) : ModelVariable( model, f ), _service_time(service_time) {}
	void operator()( LQIO::DOM::Entry * ) const;
    private:
	const RV::RandomVariable * _service_time;
    };
	
    class SetEntryVariable : public ModelVariable {
    public:
	SetEntryVariable( Generate& model, variableValueFunc f, const RV::RandomVariable * service_time ) : ModelVariable( model, f ), _service_time(service_time) {}
	void operator()( const LQIO::DOM::Entry * ) const;
    private:
	const RV::RandomVariable * _service_time;
    };
	
    class MakeCallVariable : public ModelVariable {
    public:
	MakeCallVariable( Generate& model, variableValueFunc f, const RV::RandomVariable * rnv_rate, const RV::RandomVariable * snr_rate, const RV::RandomVariable * forwarding_rate ) 
	    : ModelVariable( model, f ), _rnv_rate( rnv_rate ), _snr_rate( snr_rate ), _forwarding_rate( forwarding_rate ) {}
	void operator()( LQIO::DOM::Call * ) const;


    private:
	const RV::RandomVariable * _rnv_rate;
	const RV::RandomVariable * _snr_rate;
	const RV::RandomVariable * _forwarding_rate;
    };
	
    class SetCallVariable : public ModelVariable {
    public:
	SetCallVariable( Generate& model, variableValueFunc f, const RV::RandomVariable * rnv_rate, const RV::RandomVariable * snr_rate, const RV::RandomVariable * forwarding_rate ) 
	    : ModelVariable( model, f ), _rnv_rate( rnv_rate ), _snr_rate( snr_rate ), _forwarding_rate( forwarding_rate ) {}
	void operator()( const LQIO::DOM::Call * ) const;

    private:
	const RV::RandomVariable * _rnv_rate;
	const RV::RandomVariable * _snr_rate;
	const RV::RandomVariable * _forwarding_rate;
    };
	
    typedef void (Generate::*get_set_var_fptr)( const ModelVariable::variableValueFunc );

public:
    typedef enum { RANDOM_LAYERING, DETERMINISTIC_LAYERING, UNIFORM_LAYERING, PYRAMID_LAYERING, FUNNEL_LAYERING, FAT_LAYERING } layering_t;

public:
    Generate( LQIO::DOM::Document * doc, const unsigned runs );
    Generate( const unsigned layers, const unsigned runs );
    virtual ~Generate();

    unsigned long getNumberOfRuns() const { return _runs; }

    Generate& operator()();		// generate.
    void reparameterize();

    ostream& print( ostream& ) const;
    ostream& verbose( ostream& ) const;

    static option_type opt[];
    static const char * model_opts[];

protected:
    LQIO::DOM::ExternalVariable * get_rv( const std::string&, const std::string& name, const RV::RandomVariable * ) const;

private:
    void makeLayerCDF();
    unsigned int getLayer( const unsigned int ) const;
    LQIO::DOM::Processor * addProcessor( const string&, const scheduling_type sched_flag );
    LQIO::DOM::Task * addTask( const string& name, const scheduling_type sched_flag, const vector<LQIO::DOM::Entry *>& entries, LQIO::DOM::Processor * aProcessor);
    LQIO::DOM::Entry * addEntry( const string& name, const RV::RandomVariable * rv=0 );
    LQIO::DOM::Call * addCall( LQIO::DOM::Entry *, LQIO::DOM::Entry *, const RV::RandomVariable * rv=0 );

    void addSpex( get_set_var_fptr f, const ModelVariable::variableValueFunc g );
    void addLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g );
    void addSensitivityLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g );
    void forEach( std::map<std::string,LQIO::DOM::Entry*>::const_iterator e, const std::map<std::string,LQIO::DOM::Entry*>::const_iterator& end, const unsigned int );
    void addSensitivitySPEX( get_set_var_fptr f, const ModelVariable::variableValueFunc g );

    static bool isReferenceTask( const LQIO::DOM::Task * );
    static bool isInterestingProcessor( const LQIO::DOM::Processor * );

    void set_variables( const ModelVariable::variableValueFunc f );
    void make_variables( const ModelVariable::variableValueFunc f );
    void sensitivity_variables( const ModelVariable::variableValueFunc f );

    static std::ostream& printIndent( std::ostream& output, const int i );
    static std::ostream& printHeader( std::ostream& output, const LQIO::DOM::Document& d, const int i, const string&  );
    static std::ostream& printResults( std::ostream& output, const LQIO::DOM::Document& d, const int i, const string&  );
    void insertSPEXObservations();

    static IntegerManip indent( const int i ) { return IntegerManip( &printIndent, i ); }
    static ProgramManip print_header( const LQIO::DOM::Document& d, const int i, const string& s="" ) { return ProgramManip( &printHeader, d, i, s ); }
    static ProgramManip print_results( const LQIO::DOM::Document& d, const int i, const string& s="" ) { return ProgramManip( &printResults, d, i, s ); }

public:
    static unsigned int __number_of_clients;
    static unsigned int __number_of_processors;
    static unsigned int __number_of_tasks;
    static unsigned int __number_of_layers;
    static layering_t __layering_type;

    static RV::RandomVariable * __service_time;
    static RV::RandomVariable * __think_time;
    static RV::RandomVariable * __forwarding_probability;
    static RV::RandomVariable * __rendezvous_rate;
    static RV::RandomVariable * __send_no_reply_rate;
    static RV::RandomVariable * __customers_per_client;
    static RV::RandomVariable * __task_multiplicity;
    static RV::RandomVariable * __processor_multiplicity;
    static RV::RandomVariable * __probability_second_phase;
    static RV::RandomVariable * __probability_infinite_server;
    static RV::RandomVariable * __number_of_entries;
    
    static const char * __comment;
    static unsigned __iteration_limit;
    static unsigned __print_interval;
    static double __convergence_value;
    static double __underrelaxation;

private:
    static LQIO::DOM::ConstantExternalVariable * ONE;

    LQIO::DOM::Document * _document;
    const unsigned int _runs;
    const unsigned int _number_of_layers;

    vector<double> _layer_CDF;

    vector<vector<LQIO::DOM::Task *> > _task;
    vector<vector<LQIO::DOM::Entry *> > _entry;
    vector<LQIO::DOM::Processor *> _processor;
    vector<LQIO::DOM::Call *> _call;

    ostringstream _program;
};


inline ostream& operator<<( ostream& output, const Generate& self ) { return self.print( output ); }
#endif
