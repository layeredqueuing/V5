/* -*- c++ -*-
 * generate.h	-- Greg Franks
 *
 * $Id: generate.h 14381 2021-01-19 18:52:02Z greg $
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
#include "lqngen.h"
#include "randomvar.h"

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
	ProgramManip( std::ostream& (*f)(std::ostream&, const LQIO::DOM::Document& d, const int, const std::string& s ), const LQIO::DOM::Document& d, const int i, const std::string& s ) : _f(f), _d(d), _i(i), _s(s) {}
    private:
	std::ostream& (*_f)( std::ostream&, const LQIO::DOM::Document&, const int, const std::string& );
	const LQIO::DOM::Document& _d;
	const int _i;
	const std::string _s;

	friend std::ostream& operator<<(std::ostream & os, const ProgramManip& m ) { return m._f(os,m._d,m._i,m._s); }
    };


    class Groupize {
    public:
	Groupize( Generate& model ) : _model(model) {}
	void operator()( const std::pair<std::string,LQIO::DOM::Processor *>& p ) const { _model.groupize( p.second ); }

    private:
	Generate& _model;
    };

    class ModelVariable {
	friend class Generate;

    protected:
	typedef void (ModelVariable::*variableValueFunc)( const LQIO::DOM::ExternalVariable&, const RV::RandomVariable * ) const;

	ModelVariable( Generate& model, variableValueFunc f ) : _model(model), _f(f) {}

	void lqx_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void lqx_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void lqx_loop_body( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void lqx_sensitivity( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const;
	void spex_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void spex_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * ) const;
	void spex_sensitivity( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const;
	LQIO::DOM::ExternalVariable * get_rv( const std::string&, const std::string& name, const RV::RandomVariable * ) const;

    protected:
	Generate& _model;
	const variableValueFunc _f;		/* One of spex_random, spex_scalar, lqx_random, lqx_scalar */
    };

    class ProcessorVariable : public ModelVariable {
    public:
	ProcessorVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const std::pair<std::string,LQIO::DOM::Processor *>& ) const;
    };
	
    class TaskVariable : public ModelVariable {
    public:
	TaskVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const std::pair<std::string,LQIO::DOM::Task *>& ) const;
    };
	
    class EntryVariable : public ModelVariable {
    public:
	EntryVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const std::pair<std::string,LQIO::DOM::Entry *>& ) const;
    };
	
    class PhaseVariable : public ModelVariable {
    public:
	PhaseVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( const std::pair<unsigned, LQIO::DOM::Phase *>& p ) const { transform( p.second ); }

    protected:
	void transform( LQIO::DOM::Phase* ) const;
    };

    class ActivityVariable : public PhaseVariable {
    public:
	ActivityVariable( Generate& model, variableValueFunc f ) : PhaseVariable( model, f ) {}
	void operator()( const std::pair<std::string, LQIO::DOM::Activity *>& a ) const { transform( a.second ); }
    };

    class CallVariable : public ModelVariable {
    public:
	CallVariable( Generate& model, variableValueFunc f ) : ModelVariable( model, f ) {}
	void operator()( LQIO::DOM::Call * ) const;
    };

/* ------------------------------------------------------------------------ */

    class LQXOutput {
    public:
	LQXOutput( std::ostream& output, int i ) : _output(output), _i(i) {}
    protected:
	std::ostream& _output;
	const int _i;
    };

    class ParameterHeading : public LQXOutput {
    public:
	ParameterHeading( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( std::string& s ) const;
    };

    class DocumentHeading : public LQXOutput {
    public:
	DocumentHeading( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( struct document_observation& obs ) const;
    };

    class EntityHeading : public LQXOutput {
    public:
	EntityHeading( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& ) const;
    };

    class EntryHeading : public LQXOutput {
    public:
	EntryHeading( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const;
    };

    class PhaseHeading : public LQXOutput {
    public:
	PhaseHeading( std::ostream& output, int i, const LQIO::DOM::Entry * entry ) : LQXOutput( output, i ), _entry(entry) {}
	void operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const;
    private:
	const LQIO::DOM::Entry * _entry;
    };
	
    class CallHeading : public LQXOutput {
    public:
	CallHeading( std::ostream& output, int i, const LQIO::DOM::Entry * entry, int phase ) :  LQXOutput( output, i ), _entry(entry), _phase(phase) {}
	void operator()( const LQIO::DOM::Call * call ) const;
    private:
	const LQIO::DOM::Entry * _entry;
	const int _phase;
    };

    class ParameterResult : public LQXOutput {
    public:
	ParameterResult( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( std::string& s ) const;
    };

    class DocumentResult : public LQXOutput {
    public:
	DocumentResult( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( struct document_observation& obs ) const;
    };

    class EntityResult : public LQXOutput {
    public:
	EntityResult( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& ) const;
    };

    class EntryResult : public LQXOutput {
    public:
	EntryResult( std::ostream& output, int i ) : LQXOutput( output, i ) {}
	void operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const;
    };

    class PhaseResult : public LQXOutput {
    public:
	PhaseResult( std::ostream& output, int i, const LQIO::DOM::Entry * entry ) : LQXOutput( output, i ), _entry(entry) {}
	void operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const;
    private:
	const LQIO::DOM::Entry * _entry;
    };
	
    class CallResult : public LQXOutput {
    public:
	CallResult( std::ostream& output, int i, const LQIO::DOM::Entry * entry, int phase ) :  LQXOutput( output, i ), _entry(entry), _phase(phase) {}
	void operator()( const LQIO::DOM::Call * call ) const;
    private:
	const LQIO::DOM::Entry * _entry;
	const int _phase;
    };

    struct EntityObservation {
	void operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& ) const;
    };

    struct EntryObservation {
	void operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const;
    };

    struct PhaseObservation {
	PhaseObservation( const LQIO::DOM::Entry * entry ) : _entry(entry) {}
	void operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const;
    private:
	const LQIO::DOM::Entry * _entry;
    };
	
    struct PhaseCallObservation {
	PhaseCallObservation( const LQIO::DOM::Entry * entry ) : _entry(entry) {}
	void operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const;
    private:
	const LQIO::DOM::Entry * _entry;
    };
	
    struct CallObservation {
	CallObservation( const LQIO::DOM::Entry * entry, int phase ) : _entry(entry), _phase(phase) {}
	void operator()( const LQIO::DOM::Call * call ) const;
    private:
	const LQIO::DOM::Entry * _entry;
	const int _phase;
    };

/* ------------------------------------------------------------------------ */

    class Accumulate {
    public:
	Accumulate() : _sum(0.), _sum_squared(0.), _n(0) {}
	double mean() const;
	double stddev() const;
	unsigned int count() const { return _n; }
	
    protected:
	Accumulate& operator+=( double );
	Accumulate& operator+=( const LQIO::DOM::ExternalVariable * );
	Accumulate& operator+=( const Accumulate& );
	
    private:
	double _sum;
	double _sum_squared;
	unsigned int _n;
    };
	
    class AccumulateCustomers : public Accumulate {
    public:
	AccumulateCustomers() : Accumulate() {}
	void operator()( const LQIO::DOM::Task * );
    };

    class AccumulateMultiplicity : public Accumulate {
    public:
	AccumulateMultiplicity() : Accumulate() {}
	void operator()( const LQIO::DOM::Task * );
	void operator()( const std::vector<LQIO::DOM::Task *>& );
	void operator()( const LQIO::DOM::Processor * );
    };

    class AccumulateDelayServer : public Accumulate {
    public:
	AccumulateDelayServer() : Accumulate() {}
	void operator()( const std::vector<LQIO::DOM::Task *>& );
	void operator()( const LQIO::DOM::Task * );
    };

    class AccumulateServiceTime : public Accumulate {
    public:
	AccumulateServiceTime() : Accumulate() {}
	void operator()( const LQIO::DOM::Entry * );
	void operator()( const std::pair<unsigned,LQIO::DOM::Phase *>& );
    };

    class AccumulateThinkTime : public Accumulate {
    public:
	AccumulateThinkTime() : Accumulate() {}
	void operator()( const LQIO::DOM::Entry * );
	void operator()( const std::pair<unsigned,LQIO::DOM::Phase *>& );
    };

    class AccumulateRequests : public Accumulate {
    public:
	AccumulateRequests() : Accumulate() {}
	void operator()( const LQIO::DOM::Call * );
    };

/* ------------------------------------------------------------------------ */

    typedef void (Generate::*get_set_var_fptr)( const ModelVariable::variableValueFunc );

public:
    typedef enum { DETERMINISTIC_LAYERING, PYRAMID_LAYERING, FUNNEL_LAYERING, FAT_LAYERING, HOUR_GLASS_LAYERING, DEPTH_FIRST_LAYERING, BREADTH_FIRST_LAYERING, RANDOM_LAYERING, UNIFORM_LAYERING } layering_t;

public:
    Generate( LQIO::DOM::Document * doc, const LQIO::DOM::Document::input_format output_format, const unsigned runs, const unsigned customers );
    Generate( const LQIO::DOM::Document::input_format output_format, const unsigned runs, const unsigned layers, const unsigned customers, const unsigned processors, const unsigned clients, const unsigned tasks );
    virtual ~Generate();

    unsigned long getNumberOfRuns() const { return _runs; }

    Generate& operator()();		// generate.
    Generate& groupize();
    Generate& reparameterize();

    std::ostream& print( std::ostream& ) const;
    std::ostream& printStatistics( std::ostream& ) const;

private:
    LQIO::DOM::Document& getDOM() const { return *_document; }

    void populateLayers();
    Generate& generate();
    LQIO::DOM::Processor * addProcessor( const std::string&, const scheduling_type sched_flag );
    LQIO::DOM::Group * addGroup( LQIO::DOM::Processor * processor, double share );
    LQIO::DOM::Task * addTask( const std::string& name, const scheduling_type sched_flag, const std::vector<LQIO::DOM::Entry *>& entries, LQIO::DOM::Processor * processor );
    LQIO::DOM::Entry * addEntry( const std::string& name, const RV::Probability& rv );
    LQIO::DOM::Call * addCall( LQIO::DOM::Entry *, LQIO::DOM::Entry *, const RV::Probability& );

    static bool isReferenceTask( const LQIO::DOM::Entity * );
    static bool isReferenceTaskPhase( const LQIO::DOM::Phase * );
    static bool isInterestingProcessor( const LQIO::DOM::Entity * );
    static bool isServerTask( const LQIO::DOM::Entity * );

    void groupize( LQIO::DOM::Processor* );
    void addSpex( get_set_var_fptr f, const ModelVariable::variableValueFunc g );
    void addLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g );
    void addSensitivityLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g );
    void forEach( std::map<std::string,LQIO::DOM::Entry*>::const_iterator e, const std::map<std::string,LQIO::DOM::Entry*>::const_iterator& end, const unsigned int );
    void makeVariables( const ModelVariable::variableValueFunc f );
    void populate();

    static std::ostream& printIndent( std::ostream& output, const int i );
    static std::ostream& printHeader( std::ostream& output, const LQIO::DOM::Document& d, const int i, const std::string&  );
    static std::ostream& printResults( std::ostream& output, const LQIO::DOM::Document& d, const int i, const std::string&  );
    static void documentObservation( struct document_observation& obs );

    static IntegerManip indent( const int i ) { return IntegerManip( &printIndent, i ); }
    static ProgramManip print_header( const LQIO::DOM::Document& d, const int i, const std::string& s="" ) { return ProgramManip( &printHeader, d, i, s ); }
    static ProgramManip print_results( const LQIO::DOM::Document& d, const int i, const std::string& s="" ) { return ProgramManip( &printResults, d, i, s ); }

public:
    static layering_t __task_layering;
    static layering_t __processor_layering;
    
    static RV::RandomVariable * __customers_per_client;
    static RV::RandomVariable * __forwarding_probability;
    static RV::RandomVariable * __number_of_entries;
    static RV::RandomVariable * __outgoing_requests;
    static RV::RandomVariable * __processor_multiplicity;
    static RV::RandomVariable * __rendezvous_rate;
    static RV::RandomVariable * __send_no_reply_rate;
    static RV::RandomVariable * __service_time;
    static RV::RandomVariable * __task_multiplicity;
    static RV::RandomVariable * __think_time;
    static RV::Probability	__probability_cfs_processor;
    static RV::Probability      __probability_delay_server;
    static RV::Probability      __probability_infinite_server;
    static RV::Probability      __probability_second_phase;
    static RV::Beta             __group_share;

    static std::string __comment;
    static unsigned __iteration_limit;
    static unsigned __print_interval;
    static double __convergence_value;
    static double __underrelaxation;
    static std::map<std::string,std::string> __pragma;

protected:
    static std::vector<std::string> __random_variables;		/* LQX variable names */

private:
    LQIO::DOM::Document * _document;
    const LQIO::DOM::Document::input_format _output_format;
    const unsigned int _runs;
    const unsigned int _number_of_customers;
    const unsigned int _number_of_layers;		/* Number of layers for one model */
    const unsigned int _number_of_processors;
    const unsigned int _number_of_clients;
    const unsigned int _number_of_tasks;

    std::vector<unsigned int> _number_of_tasks_for_layer;	/* set by populateLayers() */

    std::string _comment;
    std::vector<std::vector<LQIO::DOM::Task *> > _task;
    std::vector<LQIO::DOM::Entry *> _entry;
    std::vector<LQIO::DOM::Processor *> _processor;
    std::vector<LQIO::DOM::Call *> _call;

    std::ostringstream _program;
};


inline std::ostream& operator<<( std::ostream& output, const Generate& self ) { return self.print( output ); }
#endif
