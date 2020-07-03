/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
 * $Id: lqngen.h 13477 2020-02-08 23:14:37Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _LQNGEN_H
#define _LQNGEN_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <lqio/input.h>
#include <lqio/dom_document.h>
#include "randomvar.h"


extern lqio_params_stats io_vars;

const unsigned int MAX_PHASES	    = 3;	/* Number of Phases.		*/

extern std::string command_line;

typedef enum
{
    BOTH,
    LQNGEN_ONLY,
    LQN2LQX_ONLY
} opt_values;

struct option_type
{
    int c;
    const char * name;
    int has_arg;
    const opt_values program;
    const char * msg;
};

extern option_type options[];

struct Flags
{
    enum { PARAMETERS, UTILIZATION, THROUGHPUT, RESIDENCE_TIME, QUEUEING_TIME, MVA_STEPS, MVA_WAITS, ITERATIONS, ELAPSED_TIME, USER_TIME, SYSTEM_TIME, N_OBSERVATIONS };
    enum { CUSTOMERS, PROCESSOR_MULTIPLICITY, SERVICE_TIME, TASK_MULTIPLICITY, REQUEST_RATE, THINK_TIME, N_PARAMETERS };
    static bool annotate_input;
    static bool verbose;
    static bool long_names;
    static bool lqn2lqx;
    static bool spex_output;
    static bool lqx_output;		/* Output as LQX */
    static bool reset_pragmas;
    static LQIO::DOM::Document::input_format output_format; 	/* We output input files.  Confusing? */
    static std::vector<bool> observe;
    static std::vector<bool> convert;
    static std::vector<bool> override;
    static double sensitivity;
    static unsigned int number_of_runs;
    static unsigned int number_of_models;

    static bool has_any_observation() { return find_if( Flags::observe.begin(), Flags::observe.end(), is_true ) != Flags::observe.end(); }
    static bool set_true( bool ) { return true; }
    static bool set_false( bool ) { return false; }

private:
    static bool is_true( bool b ) { return b; }
};

extern RV::RandomVariable * continuous_default;
extern RV::RandomVariable * discreet_default;
extern RV::RandomVariable * constant_default;

#endif /* LQNGEN_H */
