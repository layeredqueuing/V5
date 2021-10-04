/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* April 1992.								*/
/* September 2003.							*/
/************************************************************************/

/*
 * Comparison of srvn output results.
 * By Greg Franks.  August, 1991.
 *
 * $Id: srvndiff.cc 9689 2010-07-06 18:36:54Z greg $
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <string>
#include "srvndiff.h"
#include "srvn_results.h"
#include "symtbl.h"

/*
 * Set the appropriate local variables to the general information
 * as given.  Initialize total counters.
 */

/*ARGSUSED*/
void
set_general ( int v, double c, int i, int pr, int ph )
{
    valid_flag = v;

    total_tput = 0.0;
    total_tput_conf = 0.0;
    total_util = 0.0;
    total_util_conf = 0.0;
    total_processor_util = 0.0;    
    total_copies = 0;
    task_util  = 0.0;
    phases = ph;

    iteration_tab[pass] = static_cast<double>(i);
}


/*
 * No-operation.
 */

void
add_output_pragma( const char *str, int len )
{
}


/*
 * Add a time value to the list.
 */

/*ARGSUSED*/
void
add_elapsed_time ( double time )
{
    time_tab[pass].value[REAL_TIME] = time;
}

/*
 * Add a time value to the list.
 */

void
add_user_time ( double time )
{
    time_tab[pass].value[USER_TIME] = time;
}

/*
 * Add a time value to the list.
 */

void
add_system_time ( double time )
{
    time_tab[pass].value[SYST_TIME] = time;
}


void
add_comment( const char * comment )
{
    comment_tab[pass] = comment;
}

void 
add_mva_solver_info( const unsigned int submodels, const unsigned long core, const double step, const double step_squared, const double wait, const double wait_squared, const unsigned int faults )
{
    mva_wait_tab[pass] = wait;
}


/*
 * Add a bound datum to the list.
 */

/*ARGSUSED*/
void
add_bound (const char *entry, double lower, double upper )
{
}

/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_waiting (const char *from, const char *to, double *delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].waiting = delay[p];
	}
    }
}


void
add_waiting_confidence( const char *from, const char *to, int conf_level, double *delay )
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry && conf_level == 95 ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].wait_conf = delay[p];
	}
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_waiting( const char * task, const char *from, const char *to, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry ) {
	activity_tab[pass][from_activity].to[to_entry].waiting = delay[0];
    }
}



void
add_act_waiting_confidence( const char * task, const char *from, const char *to, int conf_level, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry && conf_level == 95 ) {
	activity_tab[pass][from_activity].to[to_entry].wait_conf = delay[0];
	confidence_intervals_present[pass] = true;
    }
}

void add_fwd_wait_variance(const char *from, const char * to, double delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	entry_tab[pass][from_entry].fwd_to[to_entry].wait_var = delay;
    }
}

void add_fwd_wait_variance_confidence(const char *from, const char * to, int conf_level, double delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry && conf_level == 95) {
	entry_tab[pass][from_entry].fwd_to[to_entry].wait_var_conf = delay;
	confidence_intervals_present[pass] = true;
    }
}

void add_fwd_waiting(const char *from, const char *to, double delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	entry_tab[pass][from_entry].fwd_to[to_entry].waiting = delay;
    }
}

void add_fwd_waiting_confidence(const char *from, const char *to, int conf_level, double delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry && conf_level == 95) {
	entry_tab[pass][from_entry].fwd_to[to_entry].wait_conf = delay;
	confidence_intervals_present[pass] = true;
    }
}

/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_snr_waiting (const char *from, const char *to, double *delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].snr_waiting = delay[p];
	}
    }
}


void
add_snr_waiting_confidence(const char *from, const char *to, int conf_level, double *delay )
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry && conf_level == 95) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].snr_wait_conf = delay[p];
	}
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_snr_waiting( const char * task, const char *from, const char *to, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry ) {
	activity_tab[pass][from_activity].to[to_entry].snr_waiting= delay[0];
    }
}


void
add_act_snr_waiting_confidence( const char * task, const char *from, const char *to, int conf_level, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry && conf_level == 95 ) {
	activity_tab[pass][from_activity].to[to_entry].snr_wait_conf = delay[0];
    }
}

/* BUG_118 */

/*
 * Add a drop probability.
 */

void
add_drop_probability (const char *from, const char *to, double *prob)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].loss_probability = prob[p];
	}
    }
}


void
add_drop_probability_confidence(const char *from, const char *to, int conf_level, double *prob )
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry && conf_level == 95 ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].loss_prob_conf = prob[p];
	}
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_drop_probability( const char * task, const char *from, const char *to, double *prob )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry ) {
	activity_tab[pass][from_activity].to[to_entry].loss_probability = prob[0];
    }
}



void
add_act_drop_probability_confidence( const char * task, const char *from, const char *to, int conf_level, double *prob )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry && conf_level == 95 ) {
	activity_tab[pass][from_activity].to[to_entry].loss_prob_conf = prob[0];
	confidence_intervals_present[pass] = true;
    }
}

/*+ BUG_164 */
/*
 * Add a holding time datum to the list of waiting times.  We only care about phase 2.
 */

void
add_holding_time( const char * task_name, const char * acquire, const char * release, double time, double variance, double utilization )
{
    int task_id  = find_or_add_task( task_name );

    if ( !task_id ) return;

    task_tab[pass][task_id].semaphore_waiting = time;
    task_tab[pass][task_id].semaphore_utilization = utilization;
}



/*
 * Add a holding time datum to the list of waiting times.
 */

void
add_holding_time_confidence( const char * task, const char * acquire, const char * release, int conf_level, double time, double variance, double utilization )
{
    int task_id  = find_or_add_task( task );

    if ( task_id && conf_level == 95 ) {
	task_tab[pass][task_id].semaphore_waiting_conf = time;
	task_tab[pass][task_id].semaphore_utilization_conf = utilization;
	confidence_intervals_present[pass] = true;
    }
}



/*- BUG_164 */

/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_wait_variance (const char *from, const char *to, double *delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].wait_var = delay[p];
	}
    }
}


void
add_wait_variance_confidence( const char *from, const char *to, int conf_level, double *delay )
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].wait_var_conf= delay[p];
	}
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_wait_variance( const char * task, const char *from, const char *to, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry ) {
	activity_tab[pass][from_activity].to[to_entry].wait_var = delay[0];
    }
}



void
add_act_wait_variance_confidence( const char * task, const char *from, const char *to, int conf_level, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry && conf_level == 95 ) {
	activity_tab[pass][from_activity].to[to_entry].wait_var_conf = delay[0];
	confidence_intervals_present[pass] = true;
    }
}



void
add_snr_wait_variance (const char *from, const char *to, double *delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].snr_wait_var = delay[p];
	}
    }
}


void
add_snr_wait_variance_confidence (const char *from, const char *to, int conf_level, double *delay)
{
    unsigned from_entry = find_or_add_entry( from );
    unsigned to_entry   = find_or_add_entry( to );

    if ( from_entry && to_entry && conf_level == 95) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][from_entry].phase[p].to[to_entry].snr_wait_var_conf = delay[p];
	}
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_snr_wait_variance( const char * task, const char *from, const char *to, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry ) {
	activity_tab[pass][from_activity].to[to_entry].snr_wait_var_conf = delay[0];
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_snr_wait_variance_confidence( const char * task, const char *from, const char *to, int conf_level, double *delay )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_entry      = find_or_add_entry( to );

    if ( from_activity && to_entry && conf_level == 95 ) {
	activity_tab[pass][from_activity].to[to_entry].snr_wait_var_conf = delay[0];
	confidence_intervals_present[pass] = true;
    }
}

/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_join (const char * task, const char *from, const char *to, double mean, double variance)
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_activity   = find_or_add_activity( task, to );

    if ( from_activity && to_activity ) {
	join_tab[pass][from_activity][to_activity].mean          = mean;
	join_tab[pass][from_activity][to_activity].variance      = variance;
    }
}


/*
 * Store confidence interval data.
 */


void
add_join_confidence(const char * task, const char *from, const char *to, int conf_level, double mean, double variance )
{
    unsigned from_activity = find_or_add_activity( task, from );
    unsigned to_activity   = find_or_add_activity( task, to );

    if ( from_activity && to_activity && conf_level == 95 ) {
	join_tab[pass][from_activity][to_activity].mean_conf     = mean;
	join_tab[pass][from_activity][to_activity].variance_conf = variance;
	confidence_intervals_present[pass] = true;
    }
}

/*
 * Add some service time information to the list.
 */

void
add_service (const char *entry, double *time)
{
    unsigned e = find_or_add_entry( entry );

    if ( e ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].service = time[p];
	}
    }
}


void
add_service_confidence( const char *entry, int conf_level, double * time )
{
    unsigned e = find_or_add_entry( entry );

    if ( e && conf_level == 95) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].serv_conf = time[p];
	}
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_service( const char * task, const char *activity, double *time )
{
    unsigned a = find_or_add_activity( task, activity );

    if ( a ) {
	activity_tab[pass][a].service   = time[0];
    }
}


void
add_act_service_confidence( const char * task, const char *activity, int conf_level, double *time )
{
    unsigned a = find_or_add_activity( task, activity );

    if ( a && conf_level == 95) {
	activity_tab[pass][a].serv_conf = time[0];
	confidence_intervals_present[pass] = true;
    }
}

/*
 * Add some variance time information to the list.
 */

void
add_variance (const char *entry, double *time)
{
    unsigned e = find_or_add_entry( entry );

    if ( e ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].variance = time[p];
	}
    }
}


void
add_variance_confidence (const char *entry, int conf_level, double *time)
{
    unsigned e = find_or_add_entry( entry );

    if ( e && conf_level == 95 ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].var_conf = time[p];
	}
	confidence_intervals_present[pass] = true;
    }
}


void
add_act_variance( const char * task, const char *activity, double *time )
{
    unsigned a = find_or_add_activity( task, activity );

    if ( a ) {
	activity_tab[pass][a].variance = time[0];
    }
}


void
add_act_variance_confidence( const char * task, const char *activity, int conf_level, double *time )
{
    unsigned a = find_or_add_activity( task, activity );

    if ( a && conf_level == 95) {
	activity_tab[pass][a].var_conf = time[0];
    }
}

/*
 * Add some service time exceeded information to the list.
 */

void
add_service_exceeded (const char *entry, double *time)
{
    unsigned e = find_or_add_entry( entry );

    if ( e ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].exceeded = time[p];
	}
    }
}


void
add_service_exceeded_confidence (const char *entry, int conf_level, double *time)
{
    unsigned e = find_or_add_entry( entry );

    if ( e && conf_level == 95 ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].exceed_conf = time[p];
	}
	confidence_intervals_present[pass] = true;
    }
}



void
add_act_service_exceeded( const char * task, const char *activity, double *time )
{
    unsigned a = find_or_add_activity( task, activity );

    if ( a ) {
	activity_tab[pass][a].exceeded = time[0];
    }
}


void
add_act_service_exceeded_confidence( const char * task, const char *activity, int conf_level, double *time )
{
    unsigned a = find_or_add_activity( task, activity );

    if ( a && conf_level == 95) {
	activity_tab[pass][a].exceed_conf = time[0];
    }
}

/*
 * Add entry related throughput, utilization to sub list.
 * ARGS_USED
 */

void
add_entry_thpt_ut ( const char *, const char *entry, double throughput, double *phase_util, double utilization )
{
    int entry_id = find_or_add_entry( entry );

    if ( !entry_id ) return;

    entry_tab[pass][entry_id].throughput  = throughput;
    entry_tab[pass][entry_id].utilization = utilization;
    entry_tab[pass][entry_id].throughput_conf  = tput_conf;
    entry_tab[pass][entry_id].utilization_conf = util_conf;
    for ( unsigned int p = 0; p < MAX_PHASES; ++p ) {
	entry_tab[pass][entry_id].phase[p].utilization = phase_util[p];
    }

    total_tput += throughput;
    total_util += utilization;
    
    total_tput_conf = tput_conf;	/* Save for task total if this is only entry.	*/
    total_util_conf = util_conf;	/* Save for task total if this is only entry.	*/
    tput_conf = 0.0;
    util_conf = 0.0;
}

/*
 * Store confidence level info for the task.
 */

void
add_entry_thpt_ut_confidence ( const char * entry, int conf_level, double throughput, double *utilization, double util )
{
    if ( conf_level == 95 ) {
	tput_conf = throughput;
	util_conf = util;
    }
    confidence_intervals_present[pass] = true;
}


/*
 * Add the utilziation for the group group_name.
 */

void add_group_util( const char * group_name, double utilization )
{
    const unsigned int g = find_or_add_group( group_name );
    if ( !g ) return;

    group_tab[pass][g].utilization = utilization;
    group_tab[pass][g].has_results = true;
}


/*
 * Store confidence interval data for the group.
 */

void add_group_util_conf( const char * group_name, int conf_level, double utilization )
{
    const unsigned int g = find_or_add_group( group_name );
    if ( !g ) return;

    if ( conf_level == 95 ) {
	group_tab[pass][g].utilization_conf = utilization;
    }
    confidence_intervals_present[pass] = true;
}

/*
 * Add the throughput and utilization per task to the list.
 */

void
add_thpt_ut (const char *task)
{
    int task_id  = find_or_add_task( task );

    if ( !task_id ) return;

    task_tab[pass][task_id].has_results	     = true;
    task_tab[pass][task_id].throughput      += total_tput;
    task_tab[pass][task_id].throughput_conf  = total_tput_conf;
    task_tab[pass][task_id].utilization_conf = total_util_conf;
    task_tab[pass][task_id].utilization     += total_util;

    total_tput = 0.0;
    total_util = 0.0;
}


/*
 * Add the throughput and utilization per task to the list.
 */

/*ARGSUSED*/
void
total_thpt_ut ( const char * task_name, double tput, double * utilization, double tot_util )
{
    const unsigned int task_id  = find_or_add_task( task_name );
    if ( task_id ) {
	for ( unsigned int p = 0; p < phases; ++p ) {
	    task_tab[pass][task_id].total_utilization[p] = utilization[p];
	}
	/* override sum */
	total_tput = tput;
	total_util = tot_util;
    }
}


/*
 * Add the throughput and utilization per task to the list.
 */

/*ARGSUSED*/
void
total_thpt_ut_confidence ( const char * task_name, int level, double tput, double * utilization, double tot_util )
{
    int task_id  = find_or_add_task( task_name );
    if (( task_id ) && ( level == 95 )) {
	for ( unsigned int p = 0; p < phases; ++p ) {
	    task_tab[pass][task_id].total_utilization_conf[p] = utilization[p];
	}
	/* override entry value */
	total_tput_conf = tput;
	total_util_conf = tot_util;
    }
}


/*
 * Add entry related throughput, utilization to sub list.
 */

void
add_act_thpt_ut (const char * task_name, const char *activity, double throughput, double *utilization)
{
/* 	total_tput += throughput; */
/* 	total_util += utilization[0]; */
}


/*
 * Add entry related throughput, utilization to sub list.
 */

void
add_act_thpt_ut_confidence (const char * task_name, const char *activity, int level, double throughput, double *utilization)
{
/* 	total_tput += throughput; */
/* 	total_util += utilization[0]; */
}


/*
 * Add the open arrival data to the list.
 */

/*ARGSUSED*/
void
add_open_arriv (const char *task, const char *entry, double arrival, double waiting)
{
    unsigned e = find_or_add_entry( entry );

    if ( e ) {
	entry_tab[pass][e].open_waiting   = waiting;
	entry_tab[pass][e].open_arrivals  = true;
    }
}

/*
 * Add confidence levels for open arrival waiting times.
 */

void
add_open_arriv_confidence( const char *task, const char *entry, int conf_level, double value )
{
    unsigned e = find_or_add_entry( entry );

    if ( e && conf_level == 95) {
	entry_tab[pass][e].open_wait_conf = value;
	entry_tab[pass][e].open_arrivals  = true;
    }
}

/*
 * Add the processor data to the processor list.
 */

void
add_proc (const char *processor)
{
    unsigned p = find_or_add_processor( processor );

    if ( p ) {
	processor_tab[pass][p].utilization 	+= total_processor_util;
	processor_tab[pass][p].n_tasks     	+= total_copies;
	processor_tab[pass][p].utilization_conf += processor_util_conf;
	processor_tab[pass][p].has_results	= true;
	total_processor_util = 0.0;
	total_copies         = 0;
	processor_util_conf  = 0.0;
    }
}


/*ARGSUSED*/
void
add_task_proc (const char * proc, const char *task, int multiplicity, double util )
{
    const unsigned int t = find_or_add_task( task );

    if ( t != 0 ) {
	task_tab[pass][t].processor_utilization = util;
    }

    total_copies += multiplicity;
    task_util = 0.0;
}


/*
 * Store the utilization confidence.  If there is a total, this value will be overwritten.
 */

void
add_task_proc_confidence (const char *task, int conf_level, double utilization )
{
    if ( conf_level == 95 ) {
	processor_util_conf = utilization;
	confidence_intervals_present[pass] = true;
    }
}


/*
 * Add the processor entry infortmation to a temporary list.
 */

void
add_entry_proc (const char *entry, double utilization, double *waiting)
{
    const unsigned e      = find_or_add_entry( entry );
    task_util            += utilization;
    total_processor_util += utilization;

    if ( e ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].processor_waiting = waiting[p];
	}
	entry_tab[pass][e].processor_utilization = utilization;
    }
}



/*
 * Add the processor entry infortmation to a temporary list.
 */

void
add_entry_proc_confidence (const char *entry, int conf_level, double utilization, double *waiting)
{
    unsigned e = find_or_add_entry( entry );

    if ( e && conf_level == 95 ) {
	for ( unsigned p = 0; p < phases; ++p ) {
	    entry_tab[pass][e].phase[p].processor_waiting_conf = waiting[p];
	}
	entry_tab[pass][e].processor_utilization_conf = utilization;
	confidence_intervals_present[pass] = true;
	processor_util_conf = utilization;		/* Will be reset if totals present */
    }
}



/*
 * Add the processor entry infortmation to a temporary list.
 * Done by the entry.
 */

/*ARGSUSED*/
void
add_act_proc (const char * task, const char *activity, double utilization, double *waiting)
{
    unsigned a;
    int task_id  = find_or_add_task( task );

    if ( !task_id ) {
	return;
    }

    a = find_or_add_activity( find_symbol_pos( task_id, ST_TASK ), activity );
    if ( a ) {
	activity_tab[pass][a].processor_waiting      = waiting[0];
    }

}

/*
 * Add the processor entry infortmation to a temporary list.
 * Done by the entry.
 */

/*ARGSUSED*/
void
add_act_proc_confidence (const char * task, const char *activity, int conf_level, double utilization, double *waiting)
{
    unsigned a;
    int task_id  = find_or_add_task( task );

    if ( !task_id ) {
	return;
    }

    a = find_or_add_activity( find_symbol_pos( task_id, ST_TASK ), activity );
    if ( a && conf_level == 95 ) {
	activity_tab[pass][a].processor_waiting_conf = waiting[0];
	confidence_intervals_present[pass] = true;
    }
}



void
add_total_proc( const char * processor_name, double utilization)
{
}


/*
 * Store confidence level info for the processor.
 */

void
add_total_proc_confidence ( const char * processor_name, int conf_level, double utilization )
{
    if ( conf_level == 95 ) {
	processor_util_conf = utilization;
    }
    confidence_intervals_present[pass] = true;
}



void
add_overtaking ( const char * e1s, const char * e2s, const char * e3s, const char * e4s, int p, double *delay )
{
    const unsigned e1 = find_or_add_entry( e1s );
    const unsigned e2 = find_or_add_entry( e2s );
    const unsigned e3 = find_or_add_entry( e3s );
    const unsigned e4 = find_or_add_entry( e4s );

    if ( e1 && e2 && e3 && e4 && e1 == e3 && e2 == e4 ) {
	if ( !overtaking_tab[pass][e1][e2] ) {
	    overtaking_tab[pass][e1][e2] = new ot;
	}
	for ( unsigned q = 0; q < phases; ++q ) {
	    overtaking_tab[pass][e1][e2]->phase[p-1][q] = delay[q];
	}
    }
}


void add_act_histogram_bin( const char * task, const char * activity, const double begin, const double end, const double prob, const double conf95, const double conf99 ) {}
void add_act_histogram_statistics( const char * task, const char * activity, const double mean, const double stddev, const double skew, const double kurtosis ) {}
void add_histogram_bin( const char * entry, const unsigned phase, const double begin, const double end, const double prob, const double conf95, const double conf99 ) {}
void add_histogram_statistics( const char * entry, const unsigned phase, const double mean, const double stddev, const double skew, const double kurtosis ) {}
