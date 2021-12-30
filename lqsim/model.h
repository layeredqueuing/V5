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
 * $Id: model.h 15299 2021-12-30 21:36:22Z greg $
 */

#ifndef LQSIM_MODEL_H
#define LQSIM_MODEL_H

#include <lqsim.h>
#include <lqio/dom_document.h>
#include <lqio/common_io.h>
#include <regex>
#include "result.h"
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#include <time.h>

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
extern bool messages_lost;

extern "C" void ps_genesis(void *);

class Task;

class Model {
    friend void ps_genesis(void *);

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
	bool set( unsigned long& parameter, const std::map<std::string,std::string>& pragmas, const char * value );
	bool set( double& parameter, const std::map<std::string,std::string>& pragmas, const char * value );

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
    template <typename Type> inline static void Delete( Type x ) { delete x; }

    Model( LQIO::DOM::Document* document, const std::string&, const std::string& );
    Model( const Model& );
    Model& operator=( const Model& );

public:
    virtual ~Model();
    
    static int solve( const std::string&, LQIO::DOM::Document::InputFormat, const std::string&, const LQIO::DOM::Pragma& pragmas );

    bool operator!() const { return _document == nullptr; }

    static int genesis_task_id() { return __genesis_task_id; }
    static double block_period() { return __model->_parameters._block_period; }
    static void set_block_period( double block_period ) { __model->_parameters._block_period = block_period; }

private:
    bool prepare();		/* Step 1 -- outside of parasol */
    bool create();		/* Step 2 -- inside of parasol	*/

    bool start();
    bool reload();		/* Load results from LQX */
    bool restart();
    
    void reset_stats();
    void accumulate_data();
    void insertDOMResults();

    bool hasOutputFileName() const { return _output_file_name.size() > 0 && _output_file_name != "-"; }
    
    void print();
    void print_intermediate();
    void print_raw_stats( FILE * output ) const;
    
    bool run( int );
    static void start_task( Task * );

    static double rms_confidence();
    static double normalized_conf95( const result_t& stat );

private:
    LQIO::DOM::Document* _document;
    std::string _input_file_name;
    std::string _output_file_name;
    LQIO::DOM::CPUTime _start_time;
    simulation_parameters _parameters;
    double _confidence;
    static int __genesis_task_id;
    static Model * __model;

public:
    static double max_service;	/* Max service time found.	*/
};

double square( const double arg );
#endif
