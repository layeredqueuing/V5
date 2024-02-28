/* -*- C++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 1991.								*/
/* September 2003.							*/
/* March 2013								*/
/************************************************************************/

/*
 * $Id: srvndiff.h 17071 2024-02-28 12:00:35Z greg $
 */

#if	!defined(SRVNDIFF_H)
#define	SRVNDIFF_H

#include <set>
#include <map>
#include <string>
#include <vector>

#define	FILE1	0
#define	FILE2	1
#define	FILE3	2
#define	FILE4	3
#define FILE5   4
#define FILE6   5
#define FILE7   6
#define FILE8   7

#define	MAX_PASS	8		/* See FILE */
#define MAX_PHASES	3

typedef enum { SYST_TIME, USER_TIME, REAL_TIME } time_values;

/* See result_str table in srvndiff.cc */
typedef enum {
    P_BOUND,
    P_CV_SQUARE,
    P_DROP,
    P_ENTRY_PROC,
    P_ENTRY_TPUT,
    P_ENTRY_UTIL,
    P_EXCEEDED,
    P_FWD_WAITING,
    P_FWD_WAIT_VAR,
    P_GROUP_UTIL,
    P_ITERATIONS,
    P_JOIN,
    P_JOIN_VAR,
    P_MVA_WAITS,
    P_OPEN_WAIT,
    P_OVERTAKING,
    P_PHASE_UTIL,
    P_PROCESSOR_UTIL,
    P_PROCESSOR_WAIT,
    P_RUNTIME,
    P_RWLOCK_READER_HOLD,
    P_RWLOCK_READER_UTIL,
    P_RWLOCK_READER_WAIT,
    P_RWLOCK_WRITER_HOLD,
    P_RWLOCK_WRITER_UTIL,
    P_RWLOCK_WRITER_WAIT,
    P_SEMAPHORE_UTIL,
    P_SEMAPHORE_WAIT,
    P_SERVICE,
    P_SNR_WAITING,
    P_SNR_WAIT_VAR,
    P_SOLVER_VERSION,
    P_TASK_PROC_UTIL,
    P_TASK_UTILIZATION,
    P_THROUGHPUT,
    P_UTILIZATION,
    P_VARIANCE,
    P_WAITING,
    P_WAIT_VAR,
    P_LIMIT
} result_str_t;

typedef void (*func_ptr)( double[], double[], unsigned, unsigned, unsigned, unsigned );
typedef bool (*check_func_ptr)( unsigned, unsigned, unsigned, unsigned );

typedef struct {
    const char * string;
    const char ** format;		/* row format for entries. */
    const char ** act_format;		/* row format for activities (same width as format). */
    unsigned *format_width;		/* width of formatted string. */
    func_ptr func;			/* statistic fetching function */
    func_ptr act_func;
    check_func_ptr check_func;		/* Row present function */
    check_func_ptr act_check_func;
} result_str_tab_t;

extern result_str_tab_t result_str[];

struct ot {
    double phase[MAX_PHASES][MAX_PHASES];
};


/*
 * Primary data structures for storing results.
 */

struct general_info {
    double value[3];
    const char * sysname;
    const char * nodename;
    std::string solver_info;
};

struct call_info
{
    call_info() : loss_prob_conf(0.), loss_probability(0.), 
		  snr_wait_conf(0.), snr_wait_var(0.), snr_wait_var_conf(0.), snr_waiting(0.),
		  wait_conf(0.), wait_var(0.), wait_var_conf(0.), waiting(0.) {}

    double loss_prob_conf;
    double loss_probability;
    double snr_wait_conf;
    double snr_wait_var;
    double snr_wait_var_conf;
    double snr_waiting;
    double wait_conf;
    double wait_var;
    double wait_var_conf;
    double waiting;
};

struct activity_info
{
    activity_info() :  exceeded(0.), exceed_conf(0.),
		       processor_waiting(0.), processor_waiting_conf(0.),
		       service(0.), serv_conf(0.), 
		       variance(0.), var_conf(0.),
		       utilization(0.), utilization_conf(0.),
		       processor_utilization(0.), processor_utilization_conf(0.) {}

    double exceeded;
    double exceed_conf;
    double processor_waiting;
    double processor_waiting_conf;
    double service;
    double serv_conf;
    double variance;
    double var_conf;
    double utilization;
    double utilization_conf;
    double processor_utilization;
    double processor_utilization_conf;
    std::map<int,call_info> to;
};

struct entry_info
{
    entry_info() : open_arrivals(false), bounds(false), drop_probability(0.0), drop_prob_conf(0.0), open_waiting(0.0), open_wait_conf(0.0),
		   fwd_to(), phase(MAX_PHASES),
		   throughput(0.0), throughput_conf(0.0), throughput_bound(0.0), utilization(0.0), utilization_conf(0.0),
		   processor_utilization(0.0), processor_utilization_conf(0.0)
	{
	}

    bool open_arrivals;
    bool bounds;
    double drop_probability;
    double drop_prob_conf;
    double open_waiting;
    double open_wait_conf;
    std::map<int,call_info> fwd_to;
    std::vector<activity_info> phase;
    double throughput;
    double throughput_conf;
    double throughput_bound;
    double utilization;
    double utilization_conf;
    double processor_utilization;
    double processor_utilization_conf;

    typedef enum EntryType {
	ENTRY_NOT_DEFINED,
	ENTRY_STANDARD,
	ENTRY_ACTIVITY,
	ENTRY_DEVICE
    } EntryType;
};

struct join_info_t
{
    join_info_t() : mean(0), mean_conf(0), variance(0), variance_conf(0) {}

    double mean;
    double mean_conf;
    double variance;
    double variance_conf;
};

struct task_info
{
    task_info() : semaphore_waiting(0.), semaphore_waiting_conf(0.), semaphore_utilization(0.), semaphore_utilization_conf(0.),
		  rwlock_reader_waiting(0.), rwlock_reader_holding(0.), rwlock_reader_utilization(0.), rwlock_writer_waiting(0.),
		  rwlock_writer_holding(0.), rwlock_writer_utilization(0.), rwlock_reader_waiting_conf(0.), rwlock_reader_holding_conf(0.),
		  rwlock_reader_utilization_conf(0.), rwlock_writer_waiting_conf(0.), rwlock_writer_holding_conf(0.), rwlock_writer_utilization_conf(0.),
		  throughput(0.), throughput_conf(0.), utilization(0.), utilization_conf(0.),
		  processor_utilization(0.), processor_utilization_conf(0.),
		  has_results(false)
	{
	    for ( unsigned p = 0; p < MAX_PHASES; ++p ) {
		total_utilization[p] = 0.0;
		total_utilization_conf[p] = 0.0;
	    }
	}
    
    double semaphore_waiting;
    double semaphore_waiting_conf;
    double semaphore_utilization;
    double semaphore_utilization_conf;

    double rwlock_reader_waiting;
    double rwlock_reader_holding;
    double rwlock_reader_utilization;
    double rwlock_writer_waiting;
    double rwlock_writer_holding;
    double rwlock_writer_utilization;
    double rwlock_reader_waiting_conf;
    double rwlock_reader_holding_conf;
    double rwlock_reader_utilization_conf;
    double rwlock_writer_waiting_conf;
    double rwlock_writer_holding_conf;
    double rwlock_writer_utilization_conf;

    double throughput;
    double throughput_conf;
    double utilization;
    double utilization_conf;
    double total_utilization[MAX_PHASES];
    double total_utilization_conf[MAX_PHASES];
    double processor_utilization;
    double processor_utilization_conf;
    bool has_results;
};

struct processor_info
{
    processor_info() : utilization(0.), utilization_conf(0.), n_tasks(0.), has_results(false) {}

    double utilization;
    double utilization_conf;
    unsigned n_tasks;
    bool has_results;
};


struct group_info
{
    group_info() : utilization(0.), utilization_conf(0.), n_tasks(0.), has_results(false) {}

    double utilization;
    double utilization_conf;
    unsigned n_tasks;
    bool has_results;
};


extern unsigned phases;			/* Number of phases in output	*/
extern int valid_flag;
extern double total_util;
extern double total_util_conf;
extern double total_processor_util;
extern double total_tput;
extern double total_tput_conf;
extern unsigned total_copies;
extern double task_util;
extern double tput_conf;
extern double util_conf;
extern double processor_util_conf;
extern bool open_arrivals;


extern general_info general_tab[MAX_PASS];
extern double iteration_tab[MAX_PASS];
extern double mva_wait_tab[MAX_PASS];
extern bool confidence_intervals_present[MAX_PASS];
extern std::string comment_tab[MAX_PASS];
extern std::map<int, processor_info> processor_tab[MAX_PASS];
extern std::map<int, group_info> group_tab[MAX_PASS];
extern std::map<int, task_info> task_tab[MAX_PASS];
extern std::map<int, entry_info> entry_tab[MAX_PASS];
extern std::map<int, activity_info> activity_tab[MAX_PASS];
extern std::map<int, std::map<int, join_info_t> > join_tab[MAX_PASS];
extern std::map<int, std::map<int, ot *> > overtaking_tab[MAX_PASS];
extern std::map<int, std::set<int> > task_entry_tab;		/* Input mapping */
extern std::map<int, std::set<int> > proc_task_tab;		/* Input mapping */

extern unsigned pass;				/* src = 0,1, dst = 2 */

void set_max_phase( const unsigned p );
unsigned int find_or_add_processor( const char * processor, const char * task );
unsigned int find_or_add_processor( const char * processor );
unsigned int find_or_add_group( const char * group );
unsigned int find_or_add_task( const char * task );
unsigned int find_or_add_entry( const char * task, const char * entry );
unsigned int find_or_add_entry( const char * name );
unsigned int find_or_add_activity( const char * task, const char * name );
#endif
