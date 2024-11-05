/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $Id: model.h 17436 2024-11-05 23:16:25Z greg $
 */

#ifndef LQSIM_MODEL_H
#define LQSIM_MODEL_H

#include "lqsim.h"
#include <regex>
#include <lqio/dom_document.h>
#include <lqio/common_io.h>
#include "result.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}

extern matherr_type matherr_disposition;    	/* What to do on math fault     */
extern FILE * stddbg;

extern std::regex processor_match_pattern;	/* Pattern to match.	    */
extern std::regex task_match_pattern;		/* Pattern to match.	    */

/*
 * Information unique to a particular instance of a task.
 */

extern bool abort_on_dropped_message;
extern bool reschedule_on_async_send;
extern bool print_lqx;

#if defined(_PARASOL)
extern "C" void ps_genesis(void *);
#endif

class Task;

class Model {
#if defined(_PARASOL)
    friend void ps_genesis(void *);
#endif

public:
    struct simulation_parameters {
	friend class Model;
	
	static const double DEFAULT_TIME;

	simulation_parameters() :
	    _seed(123456),
	    _run_time(50000),
	    _precision(0),
	    _max_blocks(1),
	    _initial_delay(0),
	    _block_period(50000)
	    {}

	void set( const std::map<std::string,std::string>&, double );

    private:
	bool set( unsigned long& parameter, const std::map<std::string,std::string>& pragmas, const std::string& value );
	bool set( double& parameter, const std::map<std::string,std::string>& pragmas, const std::string& value );

    private:
	static const unsigned long MAX_BLOCKS	    = 30;
	static const unsigned long INITIAL_LOOPS    = 100;

	unsigned long _seed;
	double _run_time;
	double _precision;
	unsigned long _max_blocks;
	double _initial_delay;
	double _block_period;
    };

    struct simulation_status {
	simulation_status() : _valid(false), _confidence(0.) {}
	bool _valid;
	double _confidence;
    };

private:
    Model( LQIO::DOM::Document* document, const std::filesystem::path&, const std::filesystem::path&, LQIO::DOM::Document::OutputFormat );
    Model( const Model& );
    Model& operator=( const Model& );

public:
    typedef bool (Model::*solve_using)();
    virtual ~Model();
    
    static int solve( solve_using, const std::filesystem::path&, LQIO::DOM::Document::InputFormat, const std::filesystem::path&, LQIO::DOM::Document::OutputFormat, const LQIO::DOM::Pragma& );

    bool operator!() const { return _document == nullptr; }

    static int genesis_task_id() { return __genesis_task_id; }
    static double block_period() { return __model->_parameters._block_period; }
    static void set_block_period( double block_period ) { __model->_parameters._block_period = block_period; }

    bool start();
    bool reload();		/* Load results from LQX */
    bool restart();
    
private:
    bool prepare();		/* Step 1 -- outside of parasol */
    bool create();		/* Step 2 -- inside of parasol	*/
#if BUG_313
    static void extend();	/* convert entry think times	*/
#endif

    void reset_stats();
    void accumulate_data();
    void insertDOMResults();

    const std::filesystem::path& getOutputFileName() const { return (!_output_file_name.empty() && _output_file_name != "-") ? _output_file_name : _input_file_name; }
    
    void print_intermediate();
    std::ostream& print( std::ostream& output ) const;
    
    bool run( int );
    static void start_task( Task * );

    static double rms_confidence();
    static double normalized_conf95( const Result& stat );

private:
    LQIO::DOM::Document* _document;
    const std::filesystem::path _input_file_name;
    const std::filesystem::path _output_file_name;
    const LQIO::DOM::Document::OutputFormat _output_format;
    LQIO::DOM::CPUTime _start_time;
    simulation_parameters _parameters;
    double _confidence;
#if defined(_PARASOL)
    static int __genesis_task_id;
#endif
    static Model * __model;
    static const std::map<const LQIO::DOM::Document::OutputFormat,const std::string> __parseable_output;

public:
    static double max_service;			/* Max service time found.	*/
    static bool __enable_print_interval;
    static unsigned int __print_interval;	/* Value set by input file.	*/
};

double square( const double arg );
#endif
