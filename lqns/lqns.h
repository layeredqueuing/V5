/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/lqns.h $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: lqns.h 11963 2014-04-10 14:36:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(LQNS_H)
#define LQNS_H
#include <config.h>
#include <string>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <lqio/input.h>

extern char * generate_file_name;

extern struct FLAGS {
    unsigned bounds_only:1;			/* -b: Load, compute bounds then stop.	*/
    unsigned deprecate_merge:1;			/* -M: merge SNR and RNV waits.		*/
    unsigned generate:1;			/* -zgenerate=xx: Generate MVA program.	*/
    unsigned no_execute:1;			/* -n: Load, but do not solve model.	*/
    unsigned parseable_output:1;		/* -p: Generate parseable output file	*/
    unsigned reset_mva:1;			/* --reset-mva (reset mva each iter.)	*/
    unsigned rtf_output:1;			/* -r: Generate RTF output file.	*/
    unsigned verbose:1;				/* -v: Be chatty.			*/
    unsigned xml_output:1;			/* -x: Ouptut XML output		*/
    
    unsigned average_variance:1;		/* Use average variance values.		*/

    unsigned trace_activities:1;		/* Print out activity stuff.		*/
    unsigned trace_delta_wait:1;		/* Print out deltaWait computation.	*/
    unsigned trace_forks:1;			/* Print out fork stuff.		*/
    unsigned trace_idle_time:1;			/* Print out idle times.		*/
    unsigned trace_interlock:1;			/* Print out interlocking.		*/
    unsigned trace_joins:1;			/* Print out join stuff.		*/
    unsigned trace_mva:1;			/* Print out MVA solutions.		*/
    unsigned trace_overtaking:1;		/* Print out overtaking calc.		*/
    unsigned trace_intermediate:1;		/* Print out intermediate solutions.	*/
    unsigned trace_wait:1;			/* Print out wait after each major loop	*/
    unsigned trace_replication:1;		/* Print out replication stuff.		*/
    unsigned trace_variance:1;			/* Print out variance solutions.	*/
    unsigned trace_submodel:1;			/* Submodel to trace.			*/
    unsigned trace_convergence:1;		/* Print out convergence values.	*/
    unsigned trace_throughput:1;
    unsigned trace_quorum:1;
	
    unsigned print_overtaking:1;		/* Print out overtaking values.		*/

    unsigned override_iterations:1;		/* Ignore value in model file.		*/
    unsigned override_convergence:1;		/* Ignore value in model file.		*/
    unsigned override_print_interval:1;		/* Ignore value in model file.		*/
    unsigned override_underrelaxation:1;	/* Ignore value in model file.		*/
    unsigned disable_expanding_quorum_tree:1;
    unsigned ignore_overhanging_threads:1;
    unsigned full_reinitialize:1;		/* Maybe a pragma?			*/
    unsigned reload_only:1;			/* Reload lqx.				*/
    unsigned restart:1;				/* Restart reusing valid results.	*/
	
    unsigned long single_step;			/* Stop after each major iteration	*/
    unsigned int skip_submodel;			/* Don't solve this level.		*/
    unsigned int min_steps;			/* Minimum number of iterations.	*/
} flags;

extern lqio_params_stats io_vars;
extern const char opts[];
#if HAVE_GETOPT_LONG
extern const struct option longopts[];
#endif
extern const char * opthelp[];

int main(int argc, char *argv[]);
#endif
