/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
 * $Id$
 *
 */

#ifndef _LQNGEN_H
#define _LQNGEN_H
#define XML_OUTPUT

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <stdexcept>
#include <lqio/input.h>
extern lqio_params_stats io_vars;
extern std::string command_line;

#if HAVE_REGEX_H
#include <regex.h>
#endif

struct Options
{
    static const char * io[];
    static const char * integer[];
    static const char * real[];
    static const char * string[];
};

const unsigned int MAX_PHASES	    = 3;	/* Number of Phases.		*/

extern std::string command_line;
extern const char * output_file_name;

typedef enum {
    FORMAT_SRVN,
#if defined(XML_OUTPUT)
    FORMAT_XML,
    FORMAT_LQX,
#endif
} output_format;

typedef double (*variate_func)( double, double );

typedef enum
{
    ARRIVALS_PROB          ,
    NUMBER_OF_CUSTOMERS    ,
    THINK_TIME             ,
    NUMBER_OF_ENTRIES      ,
    FORWARDING_REQUESTS    ,
    GENERAL_PARAMETERS     ,
    NUMBER_OF_LAYERS       ,
    OUTPUT_FORMAT2         ,
    NUMBER_OF_PROCESSORS   ,
    SEED                   ,
    NUMBER_OF_TASKS        ,
    ARRIVAL_RATE           ,
    CUSTOMERS              ,
    DETERMINISTIC          ,
    FORWARDING_PROB        ,
    HELP                   ,
    INFINITE_SERVER        ,
    NUMBER		   ,
    MULTI_SERVER           ,
    PROCESSOR_MULTIPLICITY ,
    PHASE2_PROBABILITY     ,
    SERVICE_TIME           ,
    TASK_MULTIPLICITY      ,
    VERBOSE		   ,
    RNV_PROBABILITY        ,
    RNV_REQUESTS           ,
    SNR_REQUESTS           ,
    INDIRECT               
} opt_values;

typedef enum 
{
    ITERATION_LIMIT,
    PRINT_INTERVAL,
    OVERTAKING,
    CONVERGENCE_VALUE,
    UNDERRELAXATION,
    MODEL_COMMENT
} general_options;

typedef enum
{
    DEFAULT_STRUCTURE,
    FUNNEL,
    PYRAMID,
    PRODUCT_FORM
} model_structure;

typedef struct 
{
    int c;
    const char * name;
    union {
	void * null;
	variate_func func;
	const char ** s;
    } opts;
    double floor;
    double mean;
    const bool lqngen_only;			/* Only valid for lqngen output */
    const char * msg;
} option_type;

struct Flags
{
    static bool annotate_input;
    static bool verbose;
    static bool xml_output;
    static bool lqx_output;
    static bool spex_output;
    static bool deterministic_only;
    static bool lqn2lqx;
    static model_structure structure;
    static unsigned long number_of_runs;
};

/*
 * Return square.  C++ doesn't even have an exponentiation operator, let
 * alone a smart one.
 */

static inline double square( const double x )
{
    return x * x;
}


static inline unsigned long min( const unsigned long a1, const unsigned long a2 )
{
    return ( a1 < a2 ) ? a1 : a2;
}
#endif
