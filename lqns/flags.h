/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/flags.h $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: flags.h 16546 2023-03-18 22:32:16Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_FLAGS_H
#define LQNS_FLAGS_H

extern char * generate_file_name;

extern struct FLAGS {
    unsigned bounds_only:1;			/* -b: Load, compute bounds then stop.	*/
    unsigned deprecate_merge:1;			/* -M: merge SNR and RNV waits.		*/
    unsigned generate:1;			/* -zgenerate=xx: Generate MVA program.	*/
    unsigned no_execute:1;			/* -n: Load, but do not solve model.	*/
    unsigned reset_mva:1;			/* --reset-mva (reset mva each iter.)	*/
    unsigned rtf_output:1;			/* -r: Generate RTF output file.	*/
    unsigned print_lqx:1;			/* --debug-spex: Ouptut LQX		*/
    
    unsigned average_variance:1;		/* Use average variance values.		*/

    unsigned trace_activities:1;		/* Print out activity stuff.		*/
    unsigned trace_convergence:1;		/* Print out convergence values.	*/
    unsigned trace_customers:1;			/* Print out the real number of customers and maximum number of customers*/
    unsigned trace_forks:1;			/* Print out fork stuff.		*/
    unsigned trace_idle_time:1;			/* Print out idle times.		*/
    unsigned trace_interlock:1;			/* Print out interlocking.		*/
    unsigned trace_intermediate:1;		/* Print out intermediate solutions.	*/
    unsigned trace_joins:1;			/* Print out join stuff.		*/
    unsigned trace_overtaking:1;		/* Print out overtaking calc.		*/
    unsigned trace_quorum:1;
    unsigned trace_replication:1;		/* Print out replication stuff.		*/
    unsigned trace_throughput:1;
    unsigned trace_variance:1;			/* Print out variance solutions.	*/
    unsigned trace_virtual_entry:1;		/* Print out wait after each major loop	*/
    unsigned trace_wait:1;			/* Print out wait after each major loop	*/
	
    unsigned print_overtaking:1;		/* Print out overtaking values.		*/

    unsigned disable_expanding_quorum_tree:1;
    unsigned ignore_overhanging_threads:1;
    unsigned full_reinitialize:1;		/* Maybe a pragma?			*/
	
    unsigned long single_step;			/* Stop after each major iteration	*/
    unsigned int min_steps;			/* Minimum number of iterations.	*/
} flags;
#endif
