/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
<<<<<<< .working
 * $Id$
 *
=======
 * $Id$
 * ------------------------------------------------------------------------
>>>>>>> .merge-right.r11941
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
#include <vector>
#include <set>
#include <stdexcept>
#include <lqio/input.h>
#include "randomvar.h"
#if HAVE_REGEX_H
#include <regex.h>
#endif


extern lqio_params_stats io_vars;

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

typedef enum
{
    BOTH,
    LQNGEN_ONLY,
    LQN2LQX_ONLY
} opt_values;

typedef struct 
{
    int c;
    const char * name;
    union {
	void * null;
	RV::RandomVariable * f;
	const char ** s;
    } opts;
    const opt_values program;
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
    static struct observations_t {
	bool utilization;
	bool throughput;
	bool residence_time;
	bool mva_waits;
	bool iterations;
	bool user_time;
	bool system_time;
    } observe;
    static double sensitivity;
    static unsigned int number_of_runs;
    static unsigned int number_of_models;
};

bool common_arg( const int c, char * optarg, char ** endptr );

#if HAVE_GETOPT_H
void makeopts( std::string& opts, std::vector<struct option>& longopts );
#else
void makeopts( std::string& opts );
#endif
void usage();
#endif /* LQNGEN_H */
