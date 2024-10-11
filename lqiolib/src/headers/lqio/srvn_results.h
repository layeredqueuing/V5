/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1991.								*/
/************************************************************************/

/*
 * $Id: srvn_results.h 17355 2024-10-10 22:52:30Z greg $
 */

#ifndef SRVN_RESULTS_H
#define	SRVN_RESULTS_H

#if	defined(__cplusplus)
#include <cstdio>
#include <string>
extern "C" {
#else
#include <stdio.h>
#endif

/* A few global variables used by the parser. */

extern unsigned int srvn_max_phases;	/* Number of phases. */
extern int 	resultdebug;		/* If yacc'ed using -t then resultdebug = 1 => debugging output */
extern int	resultlineno;		/* Line number of current parse line in input file */
extern FILE 	*resultin;		/* File pointer for the input file to the pareser */
extern int	result_error_flag;	/* set to 1 if resulterror called.  Must be initialized! */
extern int	resultlex();		/* Lexical analysis function */

/* A few utility functions provided by the parser */

int resultparse();
int resultlex();

void results_error( const char * fmt, ... );

/* These function are used to place data into the data structures. */

void add_act_drop_probability(const char * task, const char *to, const char *from, double *delay);
void add_act_drop_probability_confidence(const char * task, const char *to, const char *from, int conf_level, double *delay);
void add_act_histogram_bin( const char * task, const char * activity, const double begin, const double end, const double prob, const double conf95, const double conf99 );
void add_act_histogram_statistics( const char * task, const char * activity, const double mean, const double stddev, const double skew, const double kurtosis );
void add_act_proc(const char * task_name, const char *activity, double utilization, double *waiting);
void add_act_proc_confidence(const char * task_name, const char *activity, int conf_level, double utilization, double *waiting);
void add_act_service(const char * task, const char *activity, double *time);
void add_act_service_confidence(const char * task, const char *activity, int conf_level, double *time);
void add_act_service_exceeded(const char * task, const char *activity, double *time);
void add_act_service_exceeded_confidence(const char * task, const char *activity, int conf_level, double *time);
void add_act_snr_wait_variance(const char * task, const char *to, const char * from, double *delay);
void add_act_snr_wait_variance_confidence(const char * task, const char *to, const char * from, int conf_level, double *delay);
void add_act_snr_waiting(const char * task, const char *to, const char *from, double *delay);
void add_act_snr_waiting_confidence(const char * task, const char *to, const char *from, int conf_level, double *delay);
void add_act_thpt_ut(const char * task, const char * activity, double throughput, double *utilization);
void add_act_thpt_ut_confidence(const char * task, const char * activity, int conf_level, double throughput, double *utilization);
void add_act_variance(const char * task, const char *activity, double *time);
void add_act_variance_confidence(const char * task, const char *activity, int conf_level, double *time);
void add_act_wait_variance(const char * task, const char *to, const char * from, double *delay);
void add_act_wait_variance_confidence(const char * task, const char *to, const char * from, int conf_level, double *delay);
void add_act_waiting(const char * task, const char *to, const char *from, double *delay);
void add_act_waiting_confidence(const char * task, const char *to, const char *from, int conf_level, double *delay);
void add_bound(const char *entry, double lower, double upper);
void add_comment( const char *);
void add_drop_probability(const char *to, const char *from, double *delay);
void add_drop_probability_confidence(const char *to, const char *from, int conf_level, double *delay);
void add_elapsed_time(double);
void add_entry_proc(const char *entry, double utilization, double *waiting);
void add_entry_proc_confidence(const char *entry, int conf_level, double utilization, double *waiting);
void add_entry_thpt_ut(const char *entry, double throughput, double *utilization, double total_util );
void add_entry_thpt_ut_confidence(const char * entry, int conf_level, double throughput, double * utilization, double total_util );
void add_fwd_wait_variance(const char *to, const char * from, double delay);
void add_fwd_wait_variance_confidence(const char *to, const char * from, int conf_level, double delay);
void add_fwd_waiting(const char *to, const char *from, double delay);
void add_fwd_waiting_confidence(const char *to, const char *from, int conf_level, double delay);
void add_group_util( const char * group_name, double utilization );
void add_group_util_conf( const char * group_name, int conf_level, double utilization );
void add_histogram_bin( const char * entry, const unsigned phase, const double begin, const double end, const double prob, const double conf95, const double conf99 );
void add_histogram_statistics( const char * entry, const unsigned phase, const double mean, const double stddev, const double skew, const double kurtosis );
void add_holding_time( const char * task, const char * acquire, const char * release, double time, double variance, double utilization );
void add_holding_time_confidence( const char * task, const char * acquire, const char * release, int conf_level, double time, double variance, double utilization );
void add_join(const char * task, const char *from, const char *to, double mean, double variance );
void add_join_confidence(const char * task, const char *from, const char *to, int conf_level, double mean, double variance);
void add_mva_solver_info( const unsigned int submodels, const unsigned long core, const double step, const double step_squared, const double wait, const double wait_squared, const unsigned int faults );
void add_open_arriv(const char *task, const char *entry, double arrival, double waiting);
void add_open_arriv_confidence(const char *task, const char *entry, int conf_level, double value);
void add_output_pragma(const char *str,int len);
void add_overtaking ( const char * e1, const char * e2, const char * e3, const char * e4, int p, double *ot );
void add_proc(const char * proc );
void add_proc_task( const char * task, unsigned int );
void add_reader_holding_time( const char * task_name, const char * lock, const char * unlock, double blocked_time, double blocked_variance, double hold_time, double hold_variance,double utilization );
void add_reader_holding_time_confidence( const char * task_name, const char * lock, const char * unlock, int level, double blocked_time, double blocked_variance, double hold_time, double hold_variance, double utilization );
void add_service(const char *entry, double *time);
void add_service_confidence(const char *entry, int conf_level, double *time);
void add_service_exceeded(const char *entry, double *time);
void add_service_exceeded_confidence(const char *entry, int conf_level, double *time);
void add_snr_wait_variance(const char *to, const char * from, double *delay);
void add_snr_wait_variance_confidence(const char *to, const char * from, int conf_level, double *delay);
void add_snr_waiting(const char *to, const char *from, double *delay);
void add_snr_waiting_confidence(const char *to, const char *from, int conf_level, double *delay);
void add_solver_info(const char *);
void add_system_info(const char *);
void add_system_time(double);
void add_max_rss(long);
void add_task_proc(const char *task, double );
void add_task_proc_confidence(const char *task, int level, double );
void add_thpt_ut(const char *task);
void add_total_proc( const char * proc, double );
void add_total_proc_confidence( const char * proc, int conf_level, double value);
void add_user_time(double);
void add_variance(const char *entry, double *time);
void add_variance_confidence(const char *entry, int conf_level, double *time);
void add_wait_variance(const char *to, const char * from, double *delay);
void add_wait_variance_confidence(const char *to, const char * from, int conf_level, double *delay);
void add_waiting(const char *to, const char *from, double *delay);
void add_waiting_confidence(const char *to, const char *from, int conf_level, double *delay);
void add_writer_holding_time( const char * task_name, const char * lock, const char * unlock, double blocked_time, double blocked_variance, double hold_time, double hold_variance,double utilization );
void add_writer_holding_time_confidence( const char * task_name, const char * lock, const char * unlock, int level, double blocked_time, double blocked_variance, double hold_time, double hold_variance, double utilization );

void set_general(int v, double c, int i, int pr, int ph);
void set_variable( const char * variable_name, double value );
void total_thpt_ut( const char * task_name, double tput, double * utilization, double tot_util );
void total_thpt_ut_confidence( const char * task_name, int conf_level, double tput, double * utilization, double tot_util );

#if	defined(__cplusplus)
}

namespace LQIO {
    namespace SRVN {
	bool loadResults( const std::filesystem::path& );
    }
}
#endif
#endif	/* SRVN_RESULTS_H */
