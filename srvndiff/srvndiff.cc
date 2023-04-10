/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* April 1992.								*/
/* September 2003.							*/
/* March 2013								*/
/************************************************************************/

/*
 * Comparison of srvn output results.
 * By Greg Franks.  August, 1991.
 *
 * $Id: srvndiff.cc 16653 2023-04-09 14:43:29Z greg $
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#define DIFFERENCE_MODE	1

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <regex>
#include <string>
#include <sstream>
#include <vector>
#if HAVE_GLOB_H
#include <glob.h>
#else
#include <dirent.h>
#endif
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>
#if HAVE_FLOAT_H
#include <float.h>
#endif
#if HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include "error.h"
#if !HAVE_GETSUBOPT
#include "getsbopt.h"
#endif
#include "getopt2.h"
#include "srvn_results.h"
#include "symtbl.h"
#if HAVE_LIBEXPAT
#include "expat_document.h"
#endif
#include "json_document.h"
#include "srvndiff.h"
#include "parseable.h"

extern "C" int resultdebug;
extern "C" int resultlineno;	/* Line number of current parse line in input file */

#define	LINE_WIDTH	1024

/* Indecies to result_str array below */

struct stats_buf
{
    friend double rms_error ( const std::vector<stats_buf>&, std::vector<double>& rms );

    stats_buf() : n_(0), sum_(0.0), sum_sqr(0.0), sum_abs(0.0), values(), all_ok(true) {}
    void resize( unsigned int n );
    void update( double value, bool = true );
    void sort();

    double mean() const;
    double mean_abs() const;
    double stddev() const;
    double min() const;
    double max() const;
    double p90th() const;
    double sum() const { return sum_; }
    double n() const { return n_; }
    double ok() const { return all_ok; }

private:
    unsigned long n_;
    double sum_;
    double sum_sqr;
    double sum_abs;
    std::vector<double> values;
    bool all_ok;
};

typedef void (*output_func_ptr)( FILE *, unsigned, double, double );
typedef double (stats_buf::*stat_func_ptr)() const;
typedef void (*entry_waiting_func)( const result_str_t result, const unsigned passes,
				    unsigned i, unsigned k, unsigned p, std::vector<stats_buf>*, ... );
typedef void (*forwarding_waiting_func)( const result_str_t result, const unsigned passes,
					 unsigned i, unsigned k, std::vector<stats_buf>*, ... );
typedef void (*activity_waiting_func)( const result_str_t result, const unsigned passes,
				       unsigned i, unsigned k, std::vector<stats_buf>*, ... );

/*
 * We use the mean, abs and rms total_error arrays for
 * accumulating the run time statistics:
 * stime == mean, utime == abs, etime == rms
 */

static const char * time_str[] = {
    "System",
    "User",
    "Elapsed"
};


/*
 * Globals needed for parsing output file.
 */

unsigned phases;			/* Number of phases in output	*/
int valid_flag;
bool compact_flag = false;
double total_util;
double total_util_conf;
double total_processor_util;
double total_tput;
double total_tput_conf;
unsigned total_copies;
double task_util;
double tput_conf;
double util_conf;
double processor_util_conf;

static bool check_act_serv( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_act_snrw( unsigned i, unsigned j, unsigned k, unsigned );
static bool check_act_wait( unsigned i, unsigned j, unsigned k, unsigned );
static bool check_cvsq( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_entp( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_fwdw( unsigned i, unsigned j, unsigned k, unsigned );
static bool check_grup( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_hold( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_join( unsigned i, unsigned j, unsigned k, unsigned );
static bool check_open( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_ovtk( unsigned i, unsigned j, unsigned k, unsigned p_i, unsigned p_j );
static bool check_proc( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_rwlk( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_serv( unsigned i, unsigned j, unsigned p, unsigned );
static bool check_snrw( unsigned i, unsigned j, unsigned k, unsigned p );
static bool check_tput( unsigned i, unsigned j, unsigned,   unsigned );
static bool check_wait( unsigned i, unsigned j, unsigned k, unsigned p );

static void get_act_drop( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_exce( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_pwat( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_serv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_snrv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_snrw( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_vari( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_wait( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_act_wvar( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned  );
static void get_cvsq( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_drop( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_enpr( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_entp( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_enut( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_exce( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_fwdv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_fwdw( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_gutl( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_iter( double value[], double junk[],       unsigned j, unsigned p, unsigned,   unsigned );
static void get_join( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_jvar( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_mvaw( double value[], double junk[],       unsigned j, unsigned p, unsigned,   unsigned );
static void get_open( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_ovtk( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p_1, unsigned p_j );
static void get_phut( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_putl( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_pwat( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_rpht( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_rput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_rpwt( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_runt( double value[], double junk[],       unsigned j, unsigned p, unsigned,   unsigned );
static void get_serv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_snrv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_snrw( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_sput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_spwt( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_tpru( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_tput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_tutl( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_util( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_vari( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_vrsn( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_wait( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );
static void get_wpht( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_wput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_wpwt( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned );
static void get_wvar( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p );

/*
 * Main table of functions to get and print data.  See result_str_t.
 * Cast functions to common type for initializing table.
 */

static const char * fmt_e	= "%-16.16s";
static const char * fmt_e_p	= "%-16.16s %2d  ";
static const char * fmt_e_e	= "%-16.15s%-16.15s     ";
static const char * fmt_e_e_p	= "%-16.15s%-16.15s %2d  ";
static const char * fmt_t_a	= "%-16.15s%-16.15s     ";
static const char * fmt_a	= "%-16.15s     ";
static const char * fmt_j	= "%-16.15s%-16.15s";
static const char * fmt_o	= "%-16.15s%-16.15s%2d%2d ";
static unsigned int width_e	= 16;
static unsigned int width_e_p	= 16+5;
static unsigned int width_e_e_p	= 16*2+5;
static unsigned int width_j	= 16*2;

/* Using pointers for entry-fmt, act-fmt and width makes it easy to change the values on the fly. */

result_str_tab_t result_str[(int)P_LIMIT] = {
    /*                            Heading,          entry-fmt   act-fmt   width,	entry-get,activity-get, entry-check,activity-check */
    /* P_CV_SQUARE,    */       { "CV Square",      &fmt_e,	nullptr,  &width_e, 	get_cvsq, nullptr,      check_cvsq, nullptr },
    /* P_DROP,         */       { "Drop Prob",      &fmt_e_e_p,	&fmt_t_a, &width_e_e_p, get_drop, get_act_drop, check_snrw, check_act_snrw },
    /* P_ENTRY_PROC,   */       { "Utilization",    &fmt_e,     nullptr,  &width_e, 	get_enpr, nullptr,      nullptr,    nullptr },
    /* P_ENTRY_TPUT,   */       { "Throughput",     &fmt_e,     nullptr,  &width_e, 	get_entp, nullptr,      check_entp, nullptr },
    /* P_ENTRY_UTIL,   */       { "Utilization",    &fmt_e,     nullptr,  &width_e, 	get_enut, nullptr,      nullptr,    nullptr },
    /* P_EXCEEDED,     */       { "Exceeded",       &fmt_e_p,   &fmt_a,   &width_e_p, 	get_exce, get_act_exce, check_serv, check_act_serv },
    /* P_FWD_WAITING   */       { "Fwd Waiting",    &fmt_e_e,   nullptr,  &width_e_e_p, get_fwdw, nullptr,      check_fwdw, nullptr },
    /* P_FWD_WAIT_VAR  */       { "Fwd Wt Var.",    &fmt_e_e,   nullptr,  &width_e_e_p, get_fwdv, nullptr,      check_fwdw, nullptr },
    /* P_GROUP_UTIL,   */       { "Group Util.",    &fmt_e,     nullptr,  &width_e, 	get_gutl, nullptr,      check_grup, nullptr },
    /* P_ITERATIONS,   */       { "Iterations",     &fmt_e,     nullptr,  &width_e, 	get_iter, nullptr,      nullptr,    nullptr },
    /* P_JOIN,         */       { "Join Delay",     nullptr,    &fmt_j,   &width_j, 	nullptr,  get_join,     nullptr,    check_join },
    /* P_JOIN_VAR,     */       { "Join Var.",      nullptr,    &fmt_j,   &width_j, 	nullptr,  get_jvar,     nullptr,    check_join },
    /* P_MVA_WAITS,    */       { "MVA waits",      &fmt_e,     nullptr,  &width_e, 	get_mvaw, nullptr,      nullptr,    nullptr },
    /* P_OPEN_WAIT,    */       { "Open Wait",      &fmt_e,     nullptr,  &width_e, 	get_open, nullptr,      check_open, nullptr },
    /* P_OVERTAKING,   */       { "Overtaking",     &fmt_o,     nullptr,  &width_e_e_p, nullptr,  nullptr,      nullptr,    nullptr },	/* extra arg, so handled specially */ 
    /* P_PHASE_UTIL    */       { "Phase Util.",    &fmt_e,     nullptr,  &width_e, 	get_phut, nullptr,      check_proc, nullptr },
    /* P_PROCESSOR_UTIL*/       { "Proc. Util.",    &fmt_e,     nullptr,  &width_e, 	get_putl, nullptr,      check_proc, nullptr },
    /* P_PROCESSOR_WAIT*/       { "Proc. Wait.",    &fmt_e_p,   &fmt_a,   &width_e_p, 	get_pwat, get_act_pwat, check_serv, check_act_serv },
    /* P_RUNTIME,      */      	{ "Runtime",        &fmt_e,     nullptr,  &width_e, 	get_runt, nullptr,      nullptr,    nullptr },
    /* P_RWLOCK_READER_HOLD,*/  { "Reader blocked", &fmt_e,     nullptr,  &width_e, 	get_rpht, nullptr,      check_rwlk, nullptr },
    /* P_RWLOCK_READER_UTIL,*/  { "Reader Util",    &fmt_e,     nullptr,  &width_e, 	get_rput, nullptr,      check_rwlk, nullptr },
    /* P_RWLOCK_READER_WAIT,*/  { "Reader Hold",    &fmt_e,     nullptr,  &width_e, 	get_rpwt, nullptr,      check_rwlk, nullptr },
    /* P_RWLOCK_WRITER_HOLD,*/  { "Writer blocked", &fmt_e,     nullptr,  &width_e, 	get_wpht, nullptr,      check_rwlk, nullptr },
    /* P_RWLOCK_WRITER_UTIL,*/  { "Writer Util",    &fmt_e,     nullptr,  &width_e, 	get_wput, nullptr,      check_rwlk, nullptr },
    /* P_RWLOCK_WRITER_WAIT,*/  { "Writer Hold",    &fmt_e,     nullptr,  &width_e, 	get_wpwt, nullptr,      check_rwlk, nullptr },
    /* P_SEMAPHORE_UTIL*/   	{ "Sema. Util.",    &fmt_e,     nullptr,  &width_e, 	get_sput, nullptr,      check_hold, nullptr },
    /* P_SEMAPHORE_WAIT*/   	{ "Sema. Wait.",    &fmt_e,     nullptr,  &width_e, 	get_spwt, nullptr,      check_hold, nullptr },
    /* P_SERVICE,      */   	{ "Service",        &fmt_e_p,   &fmt_a,   &width_e_p, 	get_serv, get_act_serv, check_serv, check_act_serv },
    /* P_SNR_WAITING,  */   	{ "SNR Waiting",    &fmt_e_e_p, &fmt_t_a, &width_e_e_p, get_snrw, get_act_snrw, check_snrw, check_act_snrw },
    /* P_SNR_WAIT_VAR, */   	{ "SNR Wt Var.",    &fmt_e_e_p, &fmt_t_a, &width_e_e_p, get_snrv, get_act_snrv, check_snrw, check_act_snrw },
    /* P_SOLVER_VERSION*/	{ "Solver",         &fmt_e,     nullptr,  &width_e, 	get_vrsn, nullptr,      nullptr,    nullptr },
    /* P_TASK_PROC_UTIL*/   	{ "Proc. Util.",    &fmt_e,     nullptr,  &width_e, 	get_tpru, nullptr,      check_proc, nullptr },
    /* P_TASK_UTIL     */   	{ "Utilization",    &fmt_e,     nullptr,  &width_e, 	get_tutl, nullptr,      check_proc, nullptr },
    /* P_THROUGHPUT,   */   	{ "Throughput",     &fmt_e,     nullptr,  &width_e, 	get_tput, nullptr,      check_tput, nullptr },
    /* P_UTILIZATION,  */   	{ "Utilization",    &fmt_e,     nullptr,  &width_e, 	get_util, nullptr,      check_tput, nullptr },
    /* P_VARIANCE,     */   	{ "Variance",       &fmt_e_p,   &fmt_a,   &width_e_p, 	get_vari, get_act_vari, check_serv, check_act_serv },
    /* P_WAITING,      */   	{ "Waiting",        &fmt_e_e_p, &fmt_t_a, &width_e_e_p, get_wait, get_act_wait, check_wait, check_act_wait },
    /* P_WAIT_VAR,     */   	{ "Wait Var.",      &fmt_e_e_p, &fmt_t_a, &width_e_e_p, get_wvar, get_act_wvar, check_wait, check_act_wait },
};


static std::string opts = "";
LQIO::CommandLine command_line( opts );
general_info general_tab[MAX_PASS];
double iteration_tab[MAX_PASS];
double mva_wait_tab[MAX_PASS];
bool confidence_intervals_present[MAX_PASS];
std::string comment_tab[MAX_PASS];
std::map<int, processor_info> processor_tab[MAX_PASS];
std::map<int, group_info> group_tab[MAX_PASS];
std::map<int, task_info> task_tab[MAX_PASS];
std::map<int, entry_info> entry_tab[MAX_PASS];
std::map<int, activity_info> activity_tab[MAX_PASS];
std::map<int, std::map<int, join_info_t> > join_tab[MAX_PASS];
std::map<int, std::map<int, ot *> > overtaking_tab[MAX_PASS];
std::map<int, std::set<int> > task_entry_tab;	/* Input mapping */
std::map<int, std::set<int> > proc_task_tab;	/* Input mapping */
std::map<int, const std::string> headings;

unsigned pass;				/* src = 0,1, dst = 2 */
static char header[LINE_WIDTH];

static FILE * output;

#if !HAVE_GLOB_H
typedef struct {
    size_t gl_pathc;
    char ** gl_pathv;
    unsigned gl_offs;
} glob_t;

#endif
static glob_t dir_list[MAX_PASS];

/*
 * Statistics.
 */

#define N_STATS	3
typedef enum { MEAN_ERROR, ABS_ERROR, RMS_ERROR } error_type;
const char * error_str[] = {
    "Mean Error",
    "Abs. Error",
    "RMS Error"
};


std::vector<stats_buf> total[N_STATS][(int)P_LIMIT];

static double relative_error( const double, const double );

#define N_STATISTICS	6

static struct {
    const char * str;
#if defined(__SUNPRO_CC)
    const stat_func_ptr func;
#else
    stat_func_ptr func;
#endif
} statistic[N_STATISTICS] = {
    { "Mean",      &stats_buf::mean   },
    { "Std. Dev.", &stats_buf::stddev },
    { "Min",       &stats_buf::min    },
    { "P90",       &stats_buf::p90th  },
    { "Max",       &stats_buf::max    },
    { "Sum",       &stats_buf::sum    }
};

#define PRINT_MEAN	(1 << 0)
#define PRINT_STDDEV	(1 << 1)
#define PRINT_MIN  	(1 << 2)
#define PRINT_P90TH	(1 << 3)
#define PRINT_MAX 	(1 << 4)
#define PRINT_SUM  	(1 << 5)	/* BUG 354 */


/*
 * Default output format strings.
 */

static const char * result_format	= "%12.7lg";
static int result_width		= 12;
static const char * confidence_format	= "%10.3lg";
static int confidence_width	= 10;
static const char * error_format 	= "%6.1lf";
static int error_width		= 6;
static const char * separator_format	= " ";
static int separator_width	= 1;
static const char * rms_format		= "%-*s %-12.11s%-12.11s";

/* See symtbl.h */
static const char * st_namespace_names[] = {
    "processor",
    "group",
    "task",
    "entry",
    "activity",
    nullptr
};

static const char * format_opts[] = {
#define	SEPARATOR	0
    "separator",
#define RESULT_COL	1
    "result",
#define CONFIDENCE_COL	2
    "confidence",
#define ERROR_COL	3
    "error",
#define	CONF_AS_PERCENT	4
    "percent-confidence",
    0
};

/*
 * Flags to control output report.
 */

static bool print_cv_square 		= false;
static bool print_exceeded		= false;
static bool print_group_util		= true;
static bool print_iterations		= false;
static bool print_join_delay 		= true;
static bool print_join_variance 	= false;
static bool print_loss_probability 	= true;
static bool print_mva_waits		= false;
static bool print_open_wait 		= true;
static bool print_overtaking		= false;
static bool print_processor_util 	= false;
static bool print_processor_waiting	= false;
static bool print_runtimes 		= false;
static bool print_rwlock_util		= false;
//static bool print_rwlock_wait		= false;
static bool print_sema_util		= false;
static bool print_sema_wait		= false;
static bool print_service		= true;
static bool print_snr_waiting		= true;
static bool print_snr_waiting_variance	= false;
static bool print_entry_throughput	= false;
static bool print_task_throughput	= true;
static bool print_task_util		= true;
static bool print_variance		= false;
static bool print_waiting		= true;
static bool print_waiting_variance	= false;

static bool print_comment		= false;
static bool print_conf_as_percent	= false;
static bool print_confidence_intervals 	= true;
static bool print_copyright 		= false;
static bool print_error_absolute_value 	= false;
static bool print_error_only 		= false;
static bool print_latex			= false;
static bool print_quiet 		= false;
static bool print_results_only 		= false;
static bool print_rms_error_only 	= false;
static bool print_solver_information	= false;
static bool print_total_rms_error 	= true;
static bool print_totals_only 		= false;

static bool ignore_invalid_result_error = false;

static bool difference_mode		= false;

static int all_flag_count = 0;

static double error_threshold = 0.0;
static bool differences_found = false;
static bool global_differences_found = false;
static unsigned file_name_width = 12;
static bool verbose_flag = false;
bool no_replication = false;

static std::string file_pattern;
static FILE * list_of_files = 0;

static struct {
    const char * name;
    const int val;
    const bool plus_or_minus;
    const int has_arg;
    const char * description;
    const bool * value;
} flag_info[] = {
    { "files",                       '@', false, required_argument, "List of files for input", nullptr },
    { "no-confidence-intervals",     'C', false, no_argument, 	    "Print confidence intervals", &print_confidence_intervals },
#if DIFFERENCE_MODE
    { "difference",                  'D', false, no_argument,       "Output absolute difference between two files in parseable output format", nullptr },
#endif
    { "errors",                      'E', true,  no_argument,       "Print errors (or results) only", &print_error_only },
    { "select-files",                'F', false, required_argument, "Select files for comparison", nullptr },
    { "help",                        'H', false, no_argument,       "Print help", nullptr },
    { "ignore-errors",               'I', false, no_argument,       "Ignore invalid input file errors", &ignore_invalid_result_error },
    { "file-label",                  'L', false, required_argument, "Set file label to <col>=<string>", nullptr },
    { "mean-absolute-errors",        'M', false, no_argument,       "Print mean absolute values errors", &print_error_absolute_value },
    { "quiet",                       'Q', false, no_argument,       "Quiet - print only if differences found", &print_quiet },
    { "rms-errors",                  'R', false, no_argument,       "Print RMS errors only", &print_rms_error_only },
    { "error-threshold",             'S', false, required_argument, "Set error threShold for output to N.n", nullptr },
    { "no-total-rms-errors",         'T', false, no_argument,       "Print total RMS errors", &print_total_rms_error },
    { "totals-only",                 'U', false, no_argument,       "Print totals only", &print_totals_only },
    { "version",                     'V', false, no_argument,       "Print tool Version", &print_copyright },
    { "exclude",                     'X', false, required_argument, "Exclude <obj> with <regex> from results.  Object can be task, processor, or entry", nullptr },
    { "include",		     'Y', false, required_argument, "Include only <obj> with <regex> from results.  Object can be task, processor, or entry", nullptr },
    { "results",                     'a', true,  no_argument,       "Print all/no results", nullptr },
    { "processor-waiting",           'b', true,  no_argument,       "Print processor waiting times", &print_processor_waiting },
    { "coefficient-of-variation",    'c', true,  no_argument,       "Pring coefficient of variation results", &print_cv_square },
    { "asynch-send-variance",        'd', true,  no_argument,       "Print send-no-reply waiting time variance", &print_snr_waiting_variance },
    { "entry-throughput", 	     'e', true,  no_argument,       "Print entry throughput", &print_entry_throughput },
    { "format",                      'f', false, required_argument, "Set the output format for <col> to <arg>. Column can be separator, result, confidence, error, or percent-confidence. <arg> is passed to printf() as a format", nullptr },
    { "group-utilization",	     'g', true,  no_argument,       "Print processor group utilizations", &print_group_util },
    { "semaphore-utilization",       'h', true,  no_argument,       "Print semaphore utilization", &print_sema_util },
    { "iterations",                  'i', true,  no_argument,       "Print solver iterations", &print_iterations },
    { "join-delay",                  'j', true,  no_argument,       "Print join delays", &print_join_delay },
    { "join-delay-variance",         'k', true,  no_argument,       "Print join join delay variances", &print_join_variance },
    { "loss-probability",            'l', true,  no_argument,       "Print message Loss probability", &print_loss_probability },
    { "mva-wait",		     'm', true,  no_argument,	    "Print the number of times the MVA wait() function was called", &print_mva_waits },
    { "rwlock-utilization",	     'n', true,  no_argument,       "Print rwlock utilization", &print_rwlock_util },
    { "output",                      'o', false, required_argument, "Redirect output to ARG", nullptr },
    { "processor-utilization",       'p', true,  no_argument,       "Print processor utilization results", &print_processor_util },
    { "queue-length",                'q', true,  no_argument,       "Print queue length for open arrival results", &print_open_wait },
    { "run-times",                   'r', true,  no_argument,       "Print solver Run times", &print_runtimes },
    { "service-times",               's', true,  no_argument,       "Print service time results", &print_service },
    { "throughputs",                 't', true,  no_argument,       "Print task Throughput results", &print_task_throughput },
    { "utilizations",                'u', true,  no_argument,       "Print task Utilzation result", &print_task_util },
    { "variances",                   'v', true,  no_argument,       "Print variance results", &print_variance },
    { "waiting-times",               'w', true,  no_argument,       "Print waiting time results", &print_waiting },
    { "service-time-exceeded",       'x', true,  no_argument,       "Print max service time exceeded", &print_exceeded },
    { "waiting-time-variances",      'y', true,  no_argument,       "Print waiting time variance results", &print_waiting_variance },
    { "asynch-send-waits",           'z', true,  no_argument,       "Print send-no-reply waiting time results", &print_snr_waiting },
    { "solver-information",          '?', true,  no_argument,	    "Print the solver used and its version", &print_solver_information },
    { "compact",		 512+'k', false, no_argument,	    "Use a more compact format for output", nullptr },
    { "comment",		 512+'c', false, no_argument,	    "Print model comment ", nullptr },
    { "latex",			 512+'l', false, no_argument,	    "Format output for LaTeX", nullptr },
    { "debug-xml",               512+'x', false, no_argument,       "Output debugging information while parsing XML input", nullptr },
    { "debug-json",              512+'j', false, no_argument,       "Output debugging information while parsing JSON input", nullptr },
    { "debug-srvn",		 512+'s', false, no_argument,	    "Output debugging information while parsing SRVN results", nullptr },
    { "no-replication",		 512+'r', false, no_argument,       "Strip replicas from \"flattend\" model from comparison", nullptr },
    { "no-warnings",		 512+'w', false, no_argument,       "Ignore warnings when parsing results", nullptr },
    { "verbose",                 512+'v', false, no_argument,       "Verbose output (direct differences to stderr)", nullptr },
    { nullptr, 0, 0, 0, nullptr, nullptr }
};

/*
 * For output munging...
 */

struct match {
    match( const char * name ) : _name(name) {}
    bool operator()( const std::regex& r ) const { return std::regex_match( _name, r ); }
private:
    const char * _name;
};
    
std::vector<std::regex> exclude[ST_SYMTBL_TYPES];
std::vector<std::regex> include[ST_SYMTBL_TYPES];

/*
 * Local functions.
 */

static void usage( const bool full_usage = true );
static void compact_format();
static bool format_ok( const char * value, int choice, int * size );
static FILE *my_fopen(const char *filename, const char *mode);
static void compare_directories(unsigned n, char * const dirs[]);
#if !HAVE_GLOB
static void build_file_list(const char *dir, glob_t *dir_list);
#endif
static void free_dir_list(glob_t *dir_list);
static void build_file_list2(const unsigned n, char * const dir[] );
static void init_counters(void);
void init_totals( const unsigned, const unsigned );
static bool process_file(const char *filename, unsigned pass_no);
static void compare_files(const unsigned n, char * const files[] );
static void print(unsigned passes, char * const names[] );
static void make_header( char * h_ptr, char * const names[], const unsigned passes, const bool print_conf, const bool only_print_error );
static void print_sub_heading( const result_str_t result, const unsigned passes, const bool, const char * file_name, const unsigned width, const char * title, const char * sub_title );
static void print_runtime( const result_str_t result, const char * file_name, const unsigned passes, const unsigned );
static void print_iteration( const result_str_t result, const char * file_name, const unsigned passes, const unsigned );
static void print_entry_waiting( const result_str_t result, const char * file_name, const unsigned passes );
static void print_forwarding_waiting( const result_str_t result, const char * file_name, const unsigned passes );
static void print_entry_overtaking ( const result_str_t result, const char * file_name, const unsigned passes, const unsigned n );
static unsigned entry_waiting( const result_str_t result, const unsigned passes, std::vector<stats_buf>&, entry_waiting_func func );
static unsigned forwarding_waiting( const result_str_t result, const unsigned passes, std::vector<stats_buf>& rms, forwarding_waiting_func func );
static unsigned activity_waiting( const result_str_t result, const unsigned passes, std::vector<stats_buf>&, activity_waiting_func func );
static unsigned entry_overtaking( const result_str_t result, const unsigned passes, std::vector<stats_buf>&, entry_waiting_func func );
static void print_activity( const result_str_t, const unsigned passes, unsigned i, unsigned k, std::vector<stats_buf>*, ... );
static void print_activity_join ( const result_str_t result, const char * file_name, const unsigned passes );
static void print_entry( const result_str_t, const unsigned passes, unsigned i, unsigned k, unsigned p, std::vector<stats_buf>*, ... );
static void print_entry_2( const result_str_t, const unsigned passes, unsigned i, unsigned k, std::vector<stats_buf>*, ... );
static void print_entry_activity( double value[], double conf_value[], const unsigned passes, const unsigned j, std::vector<stats_buf>* );
static void print_entry_result( const result_str_t result, const char * file_name, const unsigned passes );
static void print_phase_result( const result_str_t result, const char * file_name, const unsigned passes );
static void print_group( const result_str_t result, const char * file_name, const unsigned passes );
static void print_processor( const result_str_t result, const char * file_name, const unsigned passes );
static void print_task_result( const result_str_t result, const char * file_name, const unsigned passes );
static void print_information( const result_str_t result, const char * file_name, const unsigned passes );
static void print_rms_error( const char * file_name, const result_str_t result, const std::vector<stats_buf>&, unsigned passes, const bool print_conf );
static void print_error_totals( unsigned passes, char * const names[] );
static void print_runtime_totals( unsigned passes, char * const names[] );
static void print_iteration_totals ( const result_str_t result, unsigned passes, char * const names[] );
static void print_total_statistics( const std::vector<stats_buf> &results, const unsigned start, const unsigned passes, const char * cat_string, const char * type_str, output_func_ptr print_func, const unsigned bits );
static void error_print_func( FILE * output, unsigned j, double value, double delta );
static void time_print_func( FILE * output, unsigned j, double value, double delta );
static void commonize(unsigned n, char *const dirs[]);
static int results_warning( const char * fmt, ... );
static int readInResults(const char *filename);
extern "C" int mystrcmp( const void *a, const void *b);
double rms_error( const std::vector<stats_buf>&, std::vector<double>& rms );
#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern int getsubopt (char **, char * const *, char **);
#endif
static void makeopts( std::string& opts, std::vector<struct option>& longopts );
static void minimize_path_name( std::vector<std::string>& path_name );


/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main (int argc, char * const argv[])
{
    extern char *optarg;
    extern char optsign;
    extern int optind;
    char * options;
    const char * value;
    unsigned n_args;		/* Number of args after options	*/

    struct stat stat_buf;
/* 	xxdebug = 1; */

#if HAVE_GETOPT_H
    static std::vector<struct option> longopts;
    makeopts( opts, longopts );
    command_line.setLongOpts( &longopts.front() );
#else
    makeopts( opts );
#endif

    output = stdout;
    char * name = strrchr( argv[0], '/' );
    if ( name ) {
        lq_toolname = name+1;
    } else {
        lq_toolname = argv[0];
    }
    command_line = lq_toolname;

#if HAVE_FENV_H && HAVE_FEENABLEEXCEPT && defined(DEBUG)
    feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW );
#endif

    for ( ;; ) {
#if HAVE_GETOPT_LONG
	const int c = getopt2_long( argc, argv, opts.c_str(), longopts.data(), NULL );
#else
	const int c = getopt2( argc, argv, opts.c_str() );
#endif
	if ( c == EOF ) break;
	const bool enable = (optsign == '+');

	command_line.append( c, optarg );

	switch( c ) {
	case 'a':
	    if ( all_flag_count == 0 ) {
		print_loss_probability = enable;
		print_join_delay = enable;
		print_sema_util = enable;
		print_rwlock_util = enable;
		print_group_util = enable;
		print_open_wait = enable;
		print_processor_util = enable;
		print_processor_waiting = enable;
		print_service = enable;
		print_snr_waiting = enable;
		print_task_throughput = enable;
		print_task_util = enable;
		print_waiting = enable;
		if ( enable ) {
		    all_flag_count = 1;
		}
	    } else {
		print_entry_throughput = enable;
		print_cv_square = enable;
		print_join_variance = enable;
		print_snr_waiting_variance = enable;
		print_variance = enable;
		print_exceeded = enable;
		print_waiting_variance = enable;
		if ( !enable ) {
		    all_flag_count = 0;
		}
	    }
	    break;

	case 'b':
	    print_processor_waiting = enable;
	    break;

	case 'c':
	    print_cv_square = enable;
	    break;

	case 'd':
	    print_snr_waiting_variance = enable;
	    break;

#if DIFFERENCE_MODE
	case 'D':
	    difference_mode = true;
	    break;
#endif

	case 'e':
	    print_entry_throughput = enable;
	    break;
	
	case 'E':
	    if ( enable ) {
		print_error_only   = true;
		print_results_only = false;
	    } else {
		print_error_only   = false;
		print_results_only = true;
	    }
	    break;

	case 'f':
	    options = optarg;
	    while ( *options ) {
		switch( getsubopt( &options, const_cast<char **>(format_opts), const_cast<char **>(&value) )) {
		case SEPARATOR:
		    if ( value ) {
			separator_format = value;
			separator_width  = strlen( separator_format );
		    }
		    break;

		case RESULT_COL:
		    if ( format_ok( value, RESULT_COL, &result_width ) ) {
			result_format = value;
		    }
		    break;

		case CONFIDENCE_COL:
		    if ( format_ok( value, CONFIDENCE_COL, &confidence_width ) ) {
			confidence_format = value;
		    }
		    break;

		case ERROR_COL:
		    if ( format_ok( value, ERROR_COL, &error_width ) ) {
			error_format = value;
		    }
		    break;

		case CONF_AS_PERCENT:
		    print_conf_as_percent = true;
		    if ( value && format_ok( value, CONF_AS_PERCENT, &confidence_width ) ) {
			confidence_format = value;
		    } else {
			confidence_format = "%8.3lf";
			confidence_width  = 8;
		    }
		    break;

		default:
		    (void) fprintf( stderr, "%s: invalid argument to -f -- %s\n", lq_toolname, value );
		    usage( false );
		    exit( 1 );
		}
	    }
	    break;

	case 'g':
	    print_group_util = true;
	    break;
	
	case 'H':
	    usage();
	    exit( 0 );

	case 'h':				/* BUG_164 */
	    print_sema_util = enable;
	    break;

	case 'i':
	    print_iterations = enable;
	    break;

	case 'j':
	    print_join_delay = enable;
	    break;

	case 'k':
	    print_join_variance = enable;
	    break;

	case 'l':
	    print_loss_probability = enable;
	    break;

	case 'm':
	    print_mva_waits = enable;
	    break;

	case 'n':				/* rwlock */
	    print_rwlock_util = enable;
	    break;

	case 'o':
	    output = my_fopen( optarg, "w" );
	    break;

	case 'O':
	    print_overtaking = enable;
	    break;

	case 'p':
	    print_processor_util = enable;
	    break;

	case 'q':
	    print_open_wait = enable;
	    break;

	case 'r':
	    print_runtimes = enable;
	    break;

	case 's':
	    print_service = enable;
	    break;

	case 't':
	    print_task_throughput = enable;
	    break;

	case 'u':
	    print_task_util = enable;
	    break;

	case 'v':
	    print_variance = enable;
	    break;

	case 'w':
	    print_waiting = enable;
	    break;

	case 'x':
	    print_exceeded = enable;
	    break;

	case 'y':
	    print_waiting_variance = enable;
	    break;

	case 'z':
	    print_snr_waiting = enable;
	    break;

	case '@':
	    if ( strcmp( optarg, "-" ) ==0 ) {
		list_of_files = stdin;
	    } else {
		list_of_files = my_fopen( optarg, "r" );
	    }
	    break;

	case 'C':
	    print_confidence_intervals = false;
	    break;

	case 'F':
	    file_pattern = optarg;
	    break;

	case 'I':
	    ignore_invalid_result_error = true;
	    break;

	case 'L':
	    if ( optarg != nullptr ) {
		std::stringstream ss(optarg);
		std::string item;
		while (std::getline (ss, item, ',')) {
		    int column;
		    char heading[32];
		    if ( sscanf( item.c_str(), "%d=%31s", &column, &heading ) < 2 || column < 1 || MAX_PASS < column ) {
			(void) fprintf( stderr, "%s: invalid argument to -- %s\n", lq_toolname, item.c_str() );
			usage( false );
			exit( 1 );
		    }
		    headings.emplace( column-1, heading );	/* columns start from 0 internally */
		}
	    }
	    break;
	
	case 'M':
	    print_error_absolute_value = true;
	    break;

	case 'Q':
	    print_quiet = true;
	    break;

	case 'R':
	    print_rms_error_only = true;
	    break;

	case 'S':
	    error_threshold = atof( optarg );
	    if ( error_threshold <= 0 ) {
		(void) fprintf( stderr, "%s: invalid argument to -S -- %s\n", lq_toolname, optarg );
		usage( false );
		exit( 1 );
	    }
	    break;

	case 'T':
	    print_total_rms_error = false;
	    break;

	case 'U':
	    print_totals_only     = true;
	    break;

	case 'V':
	    print_copyright = true;
	    break;

	case 'X':

	    /* Exclusion.   Ignore these tasks/processors */

	    options = optarg;
	    while ( *options ) {
		int index = getsubopt( &options, const_cast<char **>(st_namespace_names), const_cast<char **>(&value) );
		if ( index != -1 ) {
		    exclude[index].emplace_back( value );
		} else {
		    (void) fprintf( stderr, "%s: invalid argument to --exclude -- %s\n", lq_toolname, value );
		    usage( false );
		    exit( 1 );
		}
	    }
	    break;

	case 'Y':

	    /* Inclusion.   Only include these tasks/processors */

	    options = optarg;
	    while ( *options ) {
		int index = getsubopt( &options, const_cast<char **>(st_namespace_names), const_cast<char **>(&value) );
		if ( index != -1 ) {
		    include[index].emplace_back( value );
		} else {
		    (void) fprintf( stderr, "%s: invalid argument to --include -- %s\n", lq_toolname, value );
		    usage( false );
		    exit( 1 );
		}
	    }
	    break;

	case (512+'k'):
	    compact_flag = true;
	    compact_format();
	    break;
	
	case (512+'c'):
	    print_comment = true;
	    break;
	
	case '?':
	    print_solver_information = true;
	    break;
	    
	case (512+'l'):
	    print_latex = true;
	    separator_format = " & ";
	    separator_width  = 3;
	    break;

	case 512+'r':
	    no_replication = true;
	    (void) fprintf( stderr, "--no-replication: Not implemented.\n" );
	    break;
	
        case 512+'s':
            resultdebug = true;
            break;

	case 512+'v':
	    verbose_flag = true;
	    break;
	
	case (512+'w'):
	    LQIO::severity_level = LQIO::ADVISORY_ONLY;		/* Ignore warnings. */
	    break;

#if HAVE_LIBEXPAT
        case (512+'x'):
	    LQIO::DOM::Expat_Document::__debugXML = true;
	    break;
#endif
	
        case (512+'j'):
	    LQIO::DOM::Json_Document::__debugJSON = true;
	    break;

	default:
	    usage( false );
	    exit( 1 );
	}
    }

    if ( print_copyright ) {
	char copyright_date[20];
	sscanf( "$Date: 2023-04-09 10:43:29 -0400 (Sun, 09 Apr 2023) $", "%*s %s %*s", copyright_date );
	(void) fprintf( stdout, "SRVN Difference, Version %s\n", VERSION );
	(void) fprintf( stdout, "  Copyright %s the Real-Time and Distributed Systems Group,\n", copyright_date );
	(void) fprintf( stdout, "  Department of Systems and Computer Engineering,\n" );
	(void) fprintf( stdout, "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6\n\n");
    }

    /* Rationalize flags */

    if ( print_quiet ) {
	print_rms_error_only  = true;
	print_quiet 	      = true;
    } else if ( print_totals_only ) {
	print_error_only      = true;
	print_results_only    = false;
	print_rms_error_only  = true;
	print_total_rms_error = true;
    } else if ( print_rms_error_only ) {
	print_error_only      = true;
	print_results_only    = false;
    }
    if ( list_of_files && !file_pattern.empty() ) {
	(void) fprintf( stderr, "%s: -F and -@ are mutually exclusive.\n", lq_toolname );
	exit( 1 );
    }

    if ( !print_cv_square
	 && !print_exceeded
	 && !print_group_util
	 && !print_iterations
	 && !print_join_delay
	 && !print_join_variance
	 && !print_loss_probability
	 && !print_mva_waits
	 && !print_open_wait
	 && !print_overtaking
	 && !print_processor_util
	 && !print_processor_waiting
	 && !print_runtimes
	 && !print_rwlock_util
	 && !print_sema_util
	 && !print_sema_wait
	 && !print_service
	 && !print_snr_waiting
	 && !print_snr_waiting_variance
	 && !print_entry_throughput
	 && !print_solver_information
	 && !print_task_throughput
	 && !print_task_util
	 && !print_variance
	 && !print_waiting
	 && !print_waiting_variance ) {
	(void) fprintf( stderr, "%s: Nothing to print.\n", lq_toolname );
	exit( 1 );
    }


    n_args = argc - optind;
    for ( int i = optind; i < argc; ++i ) {
	command_line += " ";
	command_line += argv[i];
    }
    if ( 0 < n_args && n_args <= MAX_PASS && (!difference_mode || n_args == 2) ) {
	if ( strcmp( argv[optind], "-" ) == 0 ) {
	    init_totals( n_args, 0 );
	    compare_files( n_args, &argv[optind] );
	} else if ( stat( argv[optind], &stat_buf ) < 0 ) {
	    (void) fprintf( stderr, "%s: cannot stat ", lq_toolname );
	    perror( argv[optind] );
	} else if ( S_ISDIR( stat_buf.st_mode ) ) {
	    if ( difference_mode ) {
		(void) fprintf( stderr, "%s: Cannot use --difference with directories.\n", lq_toolname );
	    } else {
		compare_directories( n_args, &argv[optind] );
	    }
	} else if ( list_of_files ) {
	    if ( difference_mode ) {
		(void) fprintf( stderr, "%s: Cannot use --difference with --files.\n", lq_toolname );
	    } else {
		(void) fprintf( stderr, "%s: list of files specified, %s must be a directory.\n", lq_toolname, argv[optind] );
	    }
	} else {
	    init_totals( n_args, 0 );
	    compare_files( n_args, &argv[optind] );
	}
    } else {
	(void) fprintf( stderr, "%s: arg count\n", lq_toolname );
	usage( false );
	exit( 1 );
    }

    /* debug */

    return global_differences_found ? 3 : 0;
}

static void
makeopts( std::string& opts, std::vector<struct option>& longopts )
{
    struct option opt;
    opt.flag = 0;
    for ( unsigned int i = 0; flag_info[i].name != 0; ++i ) {
	opt.has_arg = flag_info[i].has_arg;
	if ( flag_info[i].plus_or_minus ) {
	    opt.val = flag_info[i].val | 0x0100;		/* Denote as '+' */
	    opt.name = flag_info[i].name;
	    longopts.push_back( opt );

	    opt.val = flag_info[i].val;
	    std::string name = "no-";
	    name += flag_info[i].name;
	    opt.name = strdup( name.c_str() );			/* Make a copy */
	    longopts.push_back( opt );	
	} else {
	    opt.val = flag_info[i].val;
	    opt.name = flag_info[i].name;
	    longopts.push_back( opt );	
	}
	if ( (flag_info[i].val & 0xff00) == 0 ) {
	    opts += static_cast<char>(flag_info[i].val);
	    if ( flag_info[i].has_arg == required_argument ) {
		opts += ':';
	    }
	}
    }
    opt.name = 0;
    opt.val  = 0;
    opt.has_arg = 0;
    longopts.push_back( opt );	
}


/*
 * Print out a helpful (?) message.
 */

static void
usage( const bool full_usage )
{
    (void) fprintf( stderr, "Usage:\t%s [OPTIONS] file1 [file2 [file3... [file6]]]\n", lq_toolname);
    (void) fprintf( stderr, "\t%s [OPTIONS] directory1 [directory2 [directory3... [directory6]]]\n", lq_toolname );

    if ( !full_usage ) return;

#if HAVE_GETOPT_LONG
    (void) fprintf( stderr, "\n\nOptions:\n" );
    for ( unsigned i = 0; flag_info[i].val != '\0'; ++i ) {
	std::string s;
	if ( flag_info[i].plus_or_minus ) {
	    s =  " -";
	    s += flag_info[i].val;
	    s += ", +";
	    s += flag_info[i].val;
	    s += ", --[no-]";
	} else {
	    if ( (flag_info[i].val & 0xff00) != 0 ) {
		s += "    ";
	    } else {
		s =  " -";
		s += flag_info[i].val;
		s += ",";
	    }
	    s += "     --";
	}
	s += flag_info[i].name;
	if ( flag_info[i].has_arg ) {
	    switch ( static_cast<unsigned int>(flag_info[i].val) ) {
	    case 'a': s += "=<obj>=<regex>"; break;
	    case 'x': s += "=<obj>=<regex>"; break;
	    case 'f': s += "=<col>=<arg>"; break;
	    case 'o': s += "=filename";	 break;
	    case 'S': s += "=N.n";	 break;
	    case 512+'h': s += "=<col>=<string>"; break;
	    default:  s += "=ARG";	 break;
	    }
	}
	fprintf( stderr, "%-40s %s", s.c_str(), flag_info[i].description );
	if ( flag_info[i].plus_or_minus && flag_info[i].value ) {
	    (void) fprintf( stderr, ": %s", *flag_info[i].value ? "ON" : "OFF" );
	}
	(void) fprintf( stderr, ".\n" );
    }
#else
    (void) fprintf( stderr, "\n\nFlag description:\n" );
    for ( unsigned i = 0; flag_info[i].description; ++i ) {
	(void) fprintf( stderr, "    %c - %s", flag_info[i].val, flag_info[i].description );
	if ( flag_info[i].value ) {
	    (void) fprintf( stderr, ": %s", *flag_info[i].value ? "ON" : "OFF" );
	}
	(void) fprintf( stderr, ".\n" );
    }
#endif
}

/*
 * Return true if the format is o.k.
 */

static bool
format_ok ( const char * value, int choice, int * size )
{
    int len;
    int prec;
    char fmt[32];

    if ( !value ) {
	(void) fprintf( stderr, "%s: No format specified to %s\n", lq_toolname, format_opts[choice] );
	return false;
    } else if ( sscanf( value, "%%%d.%d%s", &len, &prec, fmt ) != 3 ) {
	(void) fprintf( stderr, "%s: Invalid format to %s -- %s\n", lq_toolname, format_opts[choice], value );
	return false;
    } else if ( len < 4 ) {
	(void) fprintf( stderr, "%s: Invalid length value to %s -- %s\n", lq_toolname, format_opts[choice], value );
	return false;
    } else if ( len < prec + 4 ) {
	(void) fprintf( stderr, "%s: Precision is too large to %s -- %s\n", lq_toolname, format_opts[choice], value );
	return false;
    } else {
	*size = len;
	return true;
    }
}


static void
compact_format()
{
    fmt_e	= "%-8.8s";
    fmt_e_p	= "%-8.8s %2d  ";
    fmt_e_e	= "%-8.7s%-8.7s     ";
    fmt_e_e_p	= "%-8.7s%-8.7s %2d  ";
    fmt_t_a	= "%-8.7s%-8.7s     ";
    fmt_a	= "%-8.7s     ";
    fmt_j	= "%-8.7s%-8.7s";
    fmt_o	= "%-8.7s%-8.7s%2d%2d ";
    result_format = "%8.3lg";
    result_width  = 8;
    confidence_format = "%7.1lg";
    confidence_width  = 7;
    rms_format	= "%-*s %-8.7s%-8.7s";
    width_e	= 8;
    width_e_p	= 8+5;
    width_e_e_p	= 8*2+5;
    width_j	= 8*2;

    result_str[P_CV_SQUARE].string = 	"CV Sqr";
    result_str[P_DROP].string = 	"Pr Drop";
    result_str[P_ENTRY_PROC].string =	"Util";
    result_str[P_ENTRY_TPUT].string = 	"TPut";
    result_str[P_ENTRY_UTIL].string =	"Util";
    result_str[P_EXCEEDED].string =	"Pr Excd";
    result_str[P_FWD_WAITING].string =	"Fwd Wt";
    result_str[P_FWD_WAIT_VAR].string =	"Fwd WV";
    result_str[P_GROUP_UTIL].string = 	"Util";
    result_str[P_ITERATIONS].string = 	"Iters";
    result_str[P_JOIN].string =		"Join Dl";
    result_str[P_JOIN_VAR].string =	"Join DV";
    result_str[P_MVA_WAITS].string = 	"MVA Wt";
    result_str[P_OPEN_WAIT].string = 	"Open Wt";
    result_str[P_OVERTAKING].string =	"Ovrtkg";
    result_str[P_PHASE_UTIL].string = 	"Util";
    result_str[P_PROCESSOR_UTIL].string="Util";
    result_str[P_PROCESSOR_WAIT].string="Proc Wt";
    result_str[P_RUNTIME].string = 	"Runtime";	
    result_str[P_RWLOCK_READER_HOLD].string = "Rdr Bl";
    result_str[P_RWLOCK_READER_UTIL].string = "Rdr Ut";
    result_str[P_RWLOCK_READER_WAIT].string = "Rdr Ho";
    result_str[P_RWLOCK_WRITER_HOLD].string = "Wtr Bl";
    result_str[P_RWLOCK_WRITER_UTIL].string = "Wtr Ut";
    result_str[P_RWLOCK_WRITER_WAIT].string = "Wtr Ho";
    result_str[P_SEMAPHORE_UTIL].string="Sema Ut";
    result_str[P_SEMAPHORE_WAIT].string="Sema Wt";
    result_str[P_SERVICE].string =	    "Service";
    result_str[P_SOLVER_VERSION].string =   "Solver";
    result_str[P_SNR_WAITING].string =	"SNR Wt";
    result_str[P_SNR_WAIT_VAR].string =	"SNR WV";
    result_str[P_TASK_PROC_UTIL].string="Util";
    result_str[P_TASK_UTILIZATION].string="Util";
    result_str[P_THROUGHPUT].string =	"Tput";
    result_str[P_UTILIZATION].string =	"Util";
    result_str[P_VARIANCE].string =	"Varianc";
    result_str[P_WAITING].string =	"RNV Wt";
    result_str[P_WAIT_VAR].string =	"RNV WV";
}

/*
 * Open a file.  Exit on error.
 */

static FILE *
my_fopen (const char *filename, const char *mode)
{
    FILE * fptr = fopen( filename, mode );

    if ( !fptr ) {
	(void) fprintf( stderr, "%s: cannot open ", lq_toolname );
	perror( filename );
	exit( 2 );
    }
    return fptr;
}

static void
minimize_path_name( std::vector<std::string>& path_name )
{
    const size_t passes = path_name.size();
    std::vector<std::string> dir_name = path_name;

    /* find directory names */

    for ( size_t j = 0; j < passes; ++j ) {
	std::string temp = dir_name[j];
	dir_name[j] = dirname( const_cast<char *>(temp.c_str()) );		/* Clobbers temp */
	if ( dir_name[j] == "." ) {
	    char cwd[MAXPATHLEN];
	    dir_name[j] = basename( getcwd( cwd, MAXPATHLEN ) );
	}
    }

    bool all_match = true;
    for ( size_t j = 1; all_match && j < passes; ++j ) {
	all_match = dir_name[0] == dir_name[j];
    }

    if ( all_match ) {
	/* If all the directories are the same, use the file name */
	for ( size_t j = 0; j < passes; ++j ) {
	    std::string temp = path_name[j];
	    path_name[j] = basename( const_cast<char *>(temp.c_str()) );
	    size_t pos = path_name[j].rfind( '.' );	/* Strip extension */
	    if ( 0 < pos && pos != std::string::npos ) {
		path_name[j] = path_name[j].substr( 0, pos );
	    }
	}
    } else {
	/* Use last part of path which is NOT the file name */
	for ( size_t j = 0; j < passes; ++j ) {
	    path_name[j] = basename( const_cast<char *>(dir_name[j].c_str()) );
	}
    }
}


/*
 * Compare all files in dir1 with dir2.  Files must end with '.p'.
 */

static void
compare_directories (unsigned n, char * const dirs[])
{
    static const std::vector<std::string> patterns = { "*.p", "*.lqxo", "*.lqjo" };

    if ( list_of_files ) {
	build_file_list2( n, dirs );
    } else {
#if HAVE_GLOB
	for ( unsigned i = 0; i < n; ++i ) {
	    int rc = 0;
	    dir_list[i].gl_pathc = 0;
	    if ( file_pattern.empty() ) {
		for ( std::vector<std::string>::const_iterator match = patterns.begin(); match != patterns.end() && dir_list[i].gl_pathc == 0 ; ++match ) {
		    const std::string path = std::string(dirs[i]) + "/" + *match;
		    rc = glob( path.c_str(), 0, NULL, &dir_list[i] );
		}
	    } else {
		const std::string path = std::string(dirs[i]) + "/" + file_pattern;
		rc = glob( path.c_str(), 0, NULL, &dir_list[i] );
	    }
	    if ( rc == GLOB_NOSPACE ) {
		(void) fprintf( stderr, "%s: no more core!\n", lq_toolname );
		exit( 2 );
	    }
	    if ( dir_list[i].gl_pathc == 0 ) {
		(void) fprintf( stderr, "%s: no matching files in %s!\n", lq_toolname, dirs[i] );
	    }
	}
#else
	for ( unsigned int i = 0; i < n; ++i ) {
	    build_file_list( dirs[i], &dir_list[i] );
	}
#endif
    }

    if ( n > 1 ) {
	commonize( n, dirs );
    }
    if ( dir_list[0].gl_pathc == 0 ) {
	(void) fprintf( stderr, "%s: No files.\n", lq_toolname );
	return;
    }

    if ( print_rms_error_only && !print_totals_only && !print_quiet) {
	make_header( header, dirs, n, print_confidence_intervals, print_error_only );
	(void) fprintf( output, "%*c%s\n", file_name_width + (compact_flag ? 16 : 24), ' ', header );
    }

    init_totals( n, dir_list[FILE1].gl_pathc );

    for ( unsigned i = FILE2; i < n; ++i ) {
	if ( dir_list[FILE1].gl_pathc != dir_list[i].gl_pathc ) {
	    (void) fprintf( stderr, "%s: Directory size mismatch between %s and %s.\n", lq_toolname,
			    dirs[FILE1], dirs[i] );
	    goto done;
	}
    }

    /* Compare... */

    for( unsigned j = 0; j < dir_list[FILE1].gl_pathc; ++j ) {
	char * pathname[MAX_PASS];
	for ( unsigned i = 0; i < n; ++i ) {
	    pathname[i] = dir_list[i].gl_pathv[j];
	}
	compare_files( n, pathname );
    }
done:

    if ( ( print_waiting
	   || print_waiting_variance
	   || print_loss_probability
	   || print_snr_waiting
	   || print_snr_waiting_variance
	   || ( print_join_delay && join_tab[FILE1].size() > 0 )
	   || print_sema_util
	   || print_rwlock_util
	   || ( print_join_variance && join_tab[FILE1].size() > 0 )
	   || print_service
	   || print_variance
	   || print_exceeded
	   || print_cv_square
	   || print_entry_throughput
	   || print_task_throughput
	   || print_task_util
	   || print_open_wait
	   || print_overtaking
	   || ( print_group_util && group_tab[FILE1].size() )
	   || print_processor_util
	   || print_processor_waiting )
	 && !print_results_only
	 && !print_quiet
	 && print_total_rms_error
	 && dir_list[FILE1].gl_pathc >= 2 ) {
	print_error_totals( n, dirs );
    }

    if ( print_runtimes && total[REAL_TIME][P_RUNTIME][0].n() > 1 ) {
	print_runtime_totals( n, dirs );
    }

    if ( print_iterations && total[1][P_ITERATIONS][0].n() > 1 ) {
	print_iteration_totals( P_ITERATIONS, n, dirs );
    }

    if ( print_mva_waits && total[1][P_MVA_WAITS][0].n() > 1 ) {
	print_iteration_totals( P_MVA_WAITS, n, dirs );
    }

    /* Free storage */

    for ( unsigned i = 0; i < n; ++i ) {
#if HAVE_GLOB
	if ( !list_of_files ) {
	    globfree( &dir_list[i] );
	} else {
	    free_dir_list( &dir_list[i] );
	}
#else
	free_dir_list( &dir_list[i] );
#endif
    }
}

#if !HAVE_GLOB
/*
 * build a file list.
 */

static void
build_file_list (const char *dir, glob_t *dir_list)
{
    char path[MAXPATHLEN];
    struct stat stat_buf;
    struct dirent *d;
    char * p;
    DIR * dptr	= opendir( dir );
    unsigned l;

    if ( !dptr ) {
	(void) fprintf( stderr, "%s: cannot open directory ", lq_toolname  );
	perror( dir );
	exit( 2 );
    }

    dir_list->gl_offs = 0;
    dir_list->gl_pathc = 0;
    while ( ( d = readdir( dptr ) ) != NULL ) {
	(void) sprintf( path, "%s/%s", dir, d->d_name );
	if ( stat( path, &stat_buf ) < 0 ) {
	    (void) fprintf( stderr, "%s: cannot stat ", lq_toolname );
	    perror( path );
	    continue;
	}
	if ( !S_ISREG( stat_buf.st_mode ) ) {
	    continue;	/* Ignore funny files. */
	}
	p = strrchr( d->d_name, '.' );
	if ( !p || (strcmp( p, ".p" ) != 0 && strcmp( p, ".lqxo" ) != 0 && strcmp( p, ".lqjo" ) != 0 ) ) {
	    continue;	/* Ignore non .p files.	*/
	}
	if ( !file_pattern.empty() ) {
	    char c = *p;
	    *p = '\0';	/* Set end of string */
	    if ( !std::regex_match( d->d_name, std::regex( file_pattern ) ) ) {
		continue;	/* check for match file list */
	    }
	    *p = c;	/* reset value */
	}
	if ( !dir_list->gl_offs ) {
	    dir_list->gl_offs = 50;
	    dir_list->gl_pathv = (char **)malloc( sizeof( char * ) * dir_list->gl_offs );
	} else if ( dir_list->gl_pathc == dir_list->gl_offs ) {
	    dir_list->gl_offs *= 2;
	    dir_list->gl_pathv = (char **)realloc( (char *)dir_list->gl_pathv,
						   sizeof( char * ) * dir_list->gl_offs );
	}
	if ( !dir_list->gl_pathv ) {
	    (void) fprintf( stderr, "%s: no more core!\n", lq_toolname );
	    exit( 2 );
	}
	dir_list->gl_pathv[dir_list->gl_pathc] = strdup( path );
	dir_list->gl_pathc += 1;

	/* Set field width */

	l = ( ( strlen( d->d_name ) + 1 ) / 8 + 1 ) * 8;
	if ( l > file_name_width ) {
	    file_name_width = l;
	}
    }
    (void) closedir( dptr );

    /*
     * Sort the list
     */

    (void) qsort( (char *)dir_list->gl_pathv, (int)dir_list->gl_pathc, sizeof( char * ), &mystrcmp );
}
#endif



/*
 * build a file list.
 */

static void
build_file_list2 ( const unsigned n,  char * const dirs[] )
{
    char filename[MAXPATHLEN];
    unsigned i;

    for ( i = FILE1; i < n; ++i ) {
	dir_list[i].gl_offs = 0;
	dir_list[i].gl_pathc = 0;
    }
    while ( fgets( filename, MAXPATHLEN, list_of_files ) != NULL )  {
	char basename[MAXPATHLEN];
	unsigned int l = strlen( filename );
	if ( l == 0 ) continue;		/* No file -- ignore */
	if ( filename[l-1] == '\n' ) {
	    filename[l-1] = '\0';
	}
	char * p = strrchr( filename, '.' );
	if ( p ) {
	    l = p - filename;	/* Strip extension */
	}
	strncpy( basename, filename, l );
	basename[l] = 0;	/* Null terminate */

	for ( i = FILE1; i < n; ++i ) {
	    char path[MAXPATHLEN];
	    struct stat stat_buf;

	    (void) snprintf( path, MAXPATHLEN, "%s/%s", dirs[i], filename );
	    if ( stat( path, &stat_buf ) < 0 ) {
		(void) snprintf( path, MAXPATHLEN, "%s/%s.p", dirs[i], basename );
		if ( stat( path, &stat_buf ) < 0 ) {
		    (void) snprintf( path, MAXPATHLEN, "%s/%s.lqxo", dirs[i], basename );
		    if ( stat( path, &stat_buf ) < 0 ) {
			(void) snprintf( path, MAXPATHLEN, "%s/%s.lqjo", dirs[i], basename );
			if ( stat( path, &stat_buf ) < 0 ) {
			    (void) fprintf( stderr, "%s: cannot stat ", lq_toolname );
			    (void) snprintf( path, MAXPATHLEN, "%s/%s", dirs[i], filename );
			    perror( path );
			    continue;
			}
		    }
		}
	    }
	    if ( !S_ISREG( stat_buf.st_mode ) ) {
		continue;	/* Ignore funny files. */
	    }

	    if ( !dir_list[i].gl_offs ) {
		dir_list[i].gl_offs = 50;
		dir_list[i].gl_pathv = (char **)malloc( sizeof( char * ) * dir_list[i].gl_offs );
	    } else if ( dir_list[i].gl_pathc == dir_list[i].gl_offs ) {
		dir_list[i].gl_offs *= 2;
		dir_list[i].gl_pathv = (char **)realloc( (char *)dir_list[i].gl_pathv,
							 sizeof( char * ) * dir_list[i].gl_offs );
	    }
	    if ( !dir_list[i].gl_pathv ) {
		(void) fprintf( stderr, "%s: no more core!\n", lq_toolname );
		exit( 2 );
	    }
	    dir_list[i].gl_pathv[dir_list[i].gl_pathc] = strdup( path );
	    dir_list[i].gl_pathc += 1;

	    /* Set field width */

	    l = ( ( strlen( filename ) + 1 ) / 8 + 1 ) * 8;
	    if ( l > file_name_width ) {
		file_name_width = l;
	    }
	}
    }

    /*
     * Sort the list
     */

    for ( i = FILE1; i < n; ++i ) {
	(void) qsort( (char *)dir_list[i].gl_pathv, (int)dir_list[i].gl_pathc, sizeof( char * ), &mystrcmp );
    }
}



/*
 * Free the list of files.
 */

static void
free_dir_list (glob_t *dir_list)
{
    unsigned i;

    for( i = 0; i < dir_list->gl_pathc; ++i ) {
	(void) free( dir_list->gl_pathv[i] );
    }
    (void) free( dir_list->gl_pathv );
    dir_list->gl_pathc = 0;
}


/*
 * Initialize...
 */


static void
init_counters (void)
{
    for ( unsigned j = 0; j < MAX_PASS; ++j ) {
	processor_tab[j].clear();
	group_tab[j].clear();
	task_tab[j].clear();
	entry_tab[j].clear();
	activity_tab[j].clear();
	join_tab[j].clear();

	confidence_intervals_present[j] = false;

	std::map<int, std::map<int, ot *> >::iterator p_e1;
	for ( p_e1 = overtaking_tab[j].begin(); p_e1 != overtaking_tab[j].end(); ++p_e1 ) {
	    std::map<int, ot *>::iterator p_e2;
	    for ( p_e2 = p_e1->second.begin(); p_e2 != p_e1->second.end(); ++p_e2 ) {
		delete p_e2->second;
	    }
	    p_e1->second.clear();
	}
	overtaking_tab[j].clear();
    }
}



/*
 * Clear totals.
 */

void
init_totals ( const unsigned passes, const unsigned n )
{
    for ( unsigned i = 0; i < (int)P_LIMIT; ++i ) {
	for ( unsigned j = 0; j < N_STATS; ++j ) {
	    total[j][i].resize( passes );
	    for ( unsigned p = 0; p < passes; ++p ) {
		total[j][i][p].resize( n );
	    }
	}
    }
}


void
set_max_phase( const unsigned p )
{
    assert ( 0 < p && p <= MAX_PHASES );
    if ( p > phases ) phases = p;
}

/*
 * Parse a file.
 */

static bool
process_file (const char *filename, unsigned pass_no)
{
    int an_error;
    extern int result_error_flag;

    /* Set globals... */

    result_error_flag = 0;
    resultlineno = 1;
    pass   = pass_no;

    /* Open file and parse. */

    an_error = readInResults(filename);

    if ( !an_error && (valid_flag == 0 && !ignore_invalid_result_error) ) {
	an_error =  1;
	fprintf( stderr, "%s: %s: Results not valid\n", lq_toolname, filename );
    }

    return static_cast<bool>(an_error || result_error_flag);
}

/*
 * Compare the contents of file1 with file2.
 */

static void
compare_files (const unsigned n, char * const files[] )
{
    bool an_error = false;
    unsigned i;

    init_symtbl();
    init_counters();
    differences_found = false;

    for ( i = 0; i < n && !an_error; ++i ) {
	an_error = an_error | process_file( files[i], i );
    }

    if ( !an_error ) {
	if ( difference_mode ) {
#if DIFFERENCE_MODE
	    print_parseable( output );
#endif
	} else {
	    print( n, files );
	}
    }
    erase_symtbl();
    if ( differences_found ) {
	global_differences_found = true;
	if ( print_quiet ) {
	    fprintf( stdout,  "%s\n", files[0] );
	}
    }
}

/*
 * Print the results of comparison.
 */

static void
print ( unsigned passes, char * const names[] )
{
    std::string temp = names[0];
    std::string file_name = basename( const_cast<char *>(temp.c_str()) );
    size_t pos = file_name.rfind( '.' );
    file_name = file_name.substr( 0, pos );

    if ( print_comment ) {
	/* Find first comment in file (not .p) and print it.  Probably broken for latex mode and other non-common features. */
	for ( unsigned j = 0; j < passes; ++j ) {
	    if ( comment_tab[j].size() > 0 ) {
		(void) fprintf( output, "%s: %s\n\n", file_name.c_str(), comment_tab[j].c_str() );
		break;
	    }
	}
    }

    if ( !print_rms_error_only ) {
	make_header( header, names, passes, print_confidence_intervals, print_error_only );
    } else if ( print_latex ) {
	make_header( header, names, passes, print_confidence_intervals, print_error_only );
	(void) fprintf( output, header, 16, 16, file_name.c_str() );
    }

    /* Run times */

    if ( print_solver_information ) {
	print_information( P_SOLVER_VERSION, file_name.c_str(), passes );
    }
    
    if ( print_runtimes ) {
	print_runtime( P_RUNTIME, file_name.c_str(), passes, 0 );
    }

    if ( print_iterations ) {
	print_iteration( P_ITERATIONS, file_name.c_str(), passes, 0 );
    }

    if ( print_mva_waits ) {
	print_iteration( P_MVA_WAITS, file_name.c_str(), passes, 0 );
    }

    /* Waiting */

    if ( print_waiting ) {
	print_entry_waiting( P_WAITING, file_name.c_str(), passes );
    }

    /* Waiting time variance */

    if ( print_waiting_variance ) {
	print_entry_waiting( P_WAIT_VAR, file_name.c_str(), passes );
    }

    /* Waiting */

    if ( print_waiting ) {
	print_forwarding_waiting( P_FWD_WAITING, file_name.c_str(), passes );
    }

    /* Waiting time variance */

    if ( print_waiting_variance ) {
	print_forwarding_waiting( P_FWD_WAIT_VAR, file_name.c_str(), passes );
    }

    /* Waiting */

    if ( print_snr_waiting ) {
	print_entry_waiting( P_SNR_WAITING, file_name.c_str(), passes );
    }

    /* Waiting time variance */

    if ( print_snr_waiting_variance ) {
	print_entry_waiting( P_SNR_WAIT_VAR, file_name.c_str(), passes );
    }

    /* Waiting */

    if ( print_loss_probability ) {
	print_entry_waiting( P_DROP, file_name.c_str(), passes );
    }

    /* Join delays */

    if ( print_join_delay ) {
	print_activity_join( P_JOIN, file_name.c_str(), passes );
    }

    /* Join delay variance */

    if ( print_join_variance ) {
	print_activity_join( P_JOIN_VAR, file_name.c_str(), passes );
    }

    /* Service */

    if ( print_service ) {
	print_phase_result( P_SERVICE, file_name.c_str(), passes );
    }

    /* Variance */

    if ( print_variance ) {
	print_phase_result( P_VARIANCE, file_name.c_str(), passes );
    }

    /* Max service time exceeded. */

    if ( print_exceeded ) {
	print_phase_result( P_EXCEEDED, file_name.c_str(), passes );
    }

    /* Coefficient of Variantion Squared (I'm Lazy) */

    if ( print_cv_square ) {
	print_phase_result( P_CV_SQUARE, file_name.c_str(), passes );
    }

    /*+ BUG_164 */
    /* Utilizations Hold time for semaphore (phase 2 only) */

    if ( print_sema_util ) {
	print_task_result( P_SEMAPHORE_UTIL, file_name.c_str(), passes );
	print_task_result( P_SEMAPHORE_WAIT, file_name.c_str(), passes );
    }

    /* Hold time variance (phase 2 only) */

    if ( print_sema_wait ) {

    }
    /*- BUG_164 */


    /*+ RWLOCK */
    /* Utilizations and Hold time for rwlock  */

    if ( print_rwlock_util ) {
	print_task_result( P_RWLOCK_READER_UTIL, file_name.c_str(), passes );
	print_task_result( P_RWLOCK_WRITER_UTIL, file_name.c_str(), passes );

	print_task_result( P_RWLOCK_READER_WAIT, file_name.c_str(), passes );
	print_task_result( P_RWLOCK_WRITER_WAIT, file_name.c_str(), passes );
	print_task_result( P_RWLOCK_READER_HOLD, file_name.c_str(), passes );
	print_task_result( P_RWLOCK_WRITER_HOLD, file_name.c_str(), passes );
    }
    /*- RWLOCK */


    /* Throughput */

    if ( print_entry_throughput ) {
	print_entry_result( P_ENTRY_TPUT, file_name.c_str(), passes );
    }

    if ( print_task_throughput ) {
	print_task_result( P_THROUGHPUT, file_name.c_str(), passes );
    }

    /* Task Utilization */

    if ( print_task_util ) {
	print_task_result( P_UTILIZATION, file_name.c_str(), passes );
    }

    /* waiting time for open chains */

    if ( print_open_wait ) {
	print_entry_result( P_OPEN_WAIT, file_name.c_str(), passes );
    }

    /* Group utilization */

    if ( print_group_util && group_tab[FILE1].size() > 0 ) {
	print_group( P_GROUP_UTIL, file_name.c_str(), passes );
    }

    /* Processor utilization */

    if ( print_processor_util ) {
	print_processor( P_PROCESSOR_UTIL, file_name.c_str(), passes );
    }

    /* Processor waiting times */

    if ( print_processor_waiting ) {
	print_phase_result( P_PROCESSOR_WAIT, file_name.c_str(), passes );
    }

    if ( print_overtaking ) {
	print_entry_overtaking( P_OVERTAKING, file_name.c_str(), passes, entry_tab[FILE1].size() );
    }

    if ( !print_rms_error_only ) {
	(void) fputc( '\n', output );
    } else if ( print_latex ) {
	(void) fprintf( output, "\\end{tabular}\n" );
    }
}


static void
make_header ( char * h_ptr, char * const names[], const unsigned passes, const bool print_conf, const bool only_print_error )
{
    char * o_ptr = h_ptr;		/* Pointer into header string.	*/

    if ( print_latex ) {
	unsigned columns = 0;
	for ( unsigned int j = 0; j < passes; ++j ) {
	    if ( only_print_error && j == 0 ) continue;			/* Skip col 0 on error reports */
	    columns += 1;
	}

	h_ptr += snprintf( h_ptr, (o_ptr+LINE_WIDTH-h_ptr), "\\begin{tabular}{|c|*{%d}{d{2}|}}\n\\hline\n%%-*.*s ", columns );
    }

    /* Now make a cute header. */
    std::vector<std::string> column_name(passes);
    for ( unsigned int j = 0; j < passes; ++j ) {
	column_name[j] = names[j];
    }
    minimize_path_name( column_name );

    /* Replace column names if necessary */
    for ( std::map<int,const std::string>::const_iterator header = headings.begin(); header != headings.end(); ++header ) {
	column_name.at(header->first) = header->second;
    }

    for ( unsigned int j = 0; j < passes && h_ptr - o_ptr < LINE_WIDTH; ++j ) {
	unsigned field_width;		/* default heading width.	*/
	unsigned l;

	if ( only_print_error ) {
	    if ( j == 0 ) continue;			/* Skip col 0 on error reports */
	    field_width = error_width;
	} else {
	    if ( print_results_only || j == 0 ) {
		field_width = result_width;
	    } else {
		field_width = result_width + error_width + separator_width;
	    }
	    if ( j != 0 && (confidence_intervals_present[0] || confidence_intervals_present[j]) ) {
		field_width += 1;			/* Allow for '*' when err < int */
	    }
	    if ( confidence_intervals_present[j] && print_conf ) {
		field_width += confidence_width;
	    }
	}

	l = column_name[j].size();

	h_ptr += snprintf( h_ptr, (o_ptr+LINE_WIDTH-h_ptr), "%*s", separator_width, separator_format );

	if ( print_latex ) {
	    h_ptr += snprintf( h_ptr, (o_ptr+LINE_WIDTH-h_ptr), "\\multicolumn{1}{c|}{%s}", column_name[j].c_str() );
	} else if ( l >= field_width ) {
	    h_ptr += snprintf( h_ptr, (o_ptr+LINE_WIDTH-h_ptr), "%*.*s",
			    field_width, field_width, column_name[j].c_str() );
	} else {
	    h_ptr += snprintf( h_ptr, (o_ptr+LINE_WIDTH-h_ptr), "%*c%*.*s%*c",
			    (field_width-l)/2, ' ',
			    l, l, column_name[j].c_str(),
			    (field_width+1-l)/2, ' ' );
	}

	if ( j > 0 && !only_print_error ) {
	    h_ptr += snprintf( h_ptr, (o_ptr+LINE_WIDTH-h_ptr), "%*s", separator_width, separator_format );
	}
    }
    if ( print_latex ) {
	h_ptr += snprintf( h_ptr, (o_ptr+LINE_WIDTH-h_ptr), " \\\\\n\\hline\n" );
    }
}


/*
 * Print out a part of the heading.
 */

static void
print_sub_heading ( const result_str_t result, const unsigned passes, const bool conf_flag, const char * file_name, const unsigned int width, const char * title, const char * sub_title )
{
    if ( print_rms_error_only || print_quiet ) return;

    if ( print_latex ) {
	(void) fprintf( output, title, width-1, width-1, file_name );
	(void) fprintf( output, "\\hline\n%-*.*s ", width-1, width-1, sub_title );
    } else {
	(void) fprintf( output, "%-*.*s %s\n%-*.*s", width-1, width-1, file_name, title, width, width, sub_title );
    }

    for ( unsigned int j = 0; j < passes; ++j ) {
	if ( !print_error_only ) {
	    (void) fprintf( output, "%s", separator_format );
	    (void) fprintf( output, "%*s", result_width, result_str[(int)result].string );
	    if ( confidence_intervals_present[j] && conf_flag ) {
		(void) fprintf( output, "%s", separator_format );
		(void) fprintf( output, "%*s", confidence_width,
				print_conf_as_percent ? "95% +/-%" : "95% +/-" );
	    }
	}
	if ( j != 0 && passes >= 2 && !print_results_only ) {
	    (void) fprintf( output, "%s", separator_format );
	    if ( result != P_RUNTIME ) {
		(void) fprintf( output, "%*s", error_width, "%error" );
	    } else {
		(void) fprintf( output, "%*s", error_width, "%diff" );
	    }
	    if ( confidence_intervals_present[0] || confidence_intervals_present[j] ) {		/* Allow for * when error is within confidence interval */
		(void) fprintf( output, " " );
	    }
	}
    }
    if ( print_latex ) {
	(void) fprintf( output, " \\\\\n\\hline" );
    }
    (void) fputc( '\n', output );
}



/*
 * Print out data for entry waiting type fields.
 */

static void
print_entry_waiting ( const result_str_t result, const char * file_name, const unsigned passes )
{
    int count = 0;
    std::vector<stats_buf> rms(passes);

    /* Entries */

    if ( entry_waiting( result, passes, rms, 0 ) ) {
	if ( compact_flag ) {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 16+5, header, "From    To Entry  Ph" );
	} else {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 32+5, header, "From Entry      To Entry       Phase" );
	}
	entry_waiting( result, passes, rms, print_entry );
	count += 1;
	if ( print_latex && !print_rms_error_only ) {
	    (void) fprintf( output, "\\hline\n" );
	}
    }

    /* Activities */

    if ( activity_waiting( result, passes, rms, 0 ) ) {
	if ( count > 0 ) {
	    if ( !print_rms_error_only ) {
		(void) fputc( '\n', output );
	    }
	}
	if ( compact_flag ) {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 16+5, header, "TaskAct To Entry     " );
	} else {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 32+5, header, "From Task:Act   To Entry             " );
	}
	activity_waiting( result, passes, rms, print_activity );
	count += 1;
    }

    if ( count > 0 ) {
	print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
    }
}



/*
 * common code to count and print number of records.
 */

static unsigned
entry_waiting( const result_str_t result, const unsigned passes, std::vector<stats_buf>& rms, entry_waiting_func func )
{
    unsigned count = 0;
    for ( std::map<int,entry_info>::const_iterator i = entry_tab[FILE1].begin(); i != entry_tab[FILE1].end(); ++i ) {
	const entry_info& ip = i->second;
	for ( unsigned p = 0; p < MAX_PHASES; ++p ) {
	    const std::map<int,call_info>& to = ip.phase[p].to;
	    for ( std::map<int,call_info>::const_iterator k = to.begin(); k != to.end(); ++k ) {
	        unsigned j;

		/* Skip entries with all zeros as not interesting */

		for ( j = 0; j < passes; ++j ) {
		    if ( (*result_str[(int)result].check_func)( i->first, j, k->first, p ) ) break;
		}
		if ( j == passes ) continue;
		count += 1;

		if ( func ) {
		    (*func)( result, passes, i->first, k->first, p, &rms,
			     find_symbol_pos( i->first, ST_ENTRY ),
			     find_symbol_pos( k->first, ST_ENTRY ),
			     p+1 );
		}
	    }
	}
    }
    return count;
}



/*
 * Common code to count and print number of records.
 */

static unsigned
activity_waiting( const result_str_t result, const unsigned passes,
		  std::vector<stats_buf>& rms, activity_waiting_func func )
{
    unsigned count = 0;

    for ( std::map<int,activity_info>::const_iterator i = activity_tab[FILE1].begin(); i != activity_tab[FILE1].end(); ++i ) {
	const activity_info& ip = i->second;
	const std::map<int,call_info>& to = ip.to;
	for ( std::map<int,call_info>::const_iterator k = to.begin(); k != to.end(); ++k ) {
	    unsigned j;
	    for ( j = 0; j < passes; ++j ) {
		if ( (*result_str[(int)result].act_check_func)( i->first, j, k->first, 0 ) ) break;
	    }
	    if ( j == passes ) continue;
	    count += 1;

	    if ( func ) {
		(*func)( result, passes, i->first, k->first, &rms,
			 find_symbol_pos( i->first, ST_ACTIVITY ),
			 find_symbol_pos( k->first, ST_ENTRY ) );
	    }
	}
    }
    return count;
}



/*
 * Print out data for entry waiting type fields.
 */

static void
print_forwarding_waiting ( const result_str_t result, const char * file_name, const unsigned passes )
{
    int count = 0;
    std::vector<stats_buf> rms(passes);

    /* Entries */

    if ( forwarding_waiting( result, passes, rms, 0 ) ) {
	if ( compact_flag ) {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 16+5, header, "From    To Entry" );
	} else {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 32+5, header, "From Entry      To Entry       " );
	} 
	forwarding_waiting( result, passes, rms, print_entry_2 );
	count += 1;
	if ( print_latex && !print_rms_error_only ) {
	    (void) fprintf( output, "\\hline\n" );
	}
    }

    if ( count > 0 ) {
	print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
    }
}



/*
 * common code to count and print number of records.
 */

static unsigned
forwarding_waiting( const result_str_t result, const unsigned passes, std::vector<stats_buf>& rms, forwarding_waiting_func func )
{
    unsigned count = 0;
    for ( std::map<int,entry_info>::const_iterator i = entry_tab[FILE1].begin(); i != entry_tab[FILE1].end(); ++i ) {
	const entry_info& ip = i->second;
	const std::map<int,call_info>& to = ip.fwd_to;
	for ( std::map<int,call_info>::const_iterator k = to.begin(); k != to.end(); ++k ) {
	    unsigned j;

	    /* Skip entries with all zeros as not interesting */

	    for ( j = 0; j < passes; ++j ) {
		if ( (*result_str[(int)result].check_func)( i->first, j, k->first, 0 ) ) break;
	    }
	    if ( j == passes ) continue;
	    count += 1;

	    if ( func ) {
		(*func)( result, passes, i->first, k->first, &rms,
			 find_symbol_pos( i->first, ST_ENTRY ),
			 find_symbol_pos( k->first, ST_ENTRY ) );
	    }
	}
    }
    return count;
}



/*
 * Print out data for entry waiting type fields.
 */

static void
print_entry_overtaking ( const result_str_t result, const char * file_name, const unsigned passes, const unsigned n )
{
    int count = 0;
    std::vector<stats_buf> rms(passes);

#if defined(DEBUG)
    std::map<int, std::map<int, ot *> >::const_iterator p_e1;
    for ( p_e1 = overtaking_tab[0].begin(); p_e1 != overtaking_tab[0].end(); ++p_e1 ) {
	std::map<int, ot *>::const_iterator p_e2;
	for ( p_e2 = p_e1->second.begin(); p_e2 != p_e1->second.end(); ++p_e2 ) {
	    for ( unsigned p_j = 0; p_j < MAX_PHASES; ++p_j ) {
		for ( unsigned p_i = 0; p_i < MAX_PHASES; ++p_i ) {
		    fprintf( stderr, "OT(%d,%d,%d)[%d][%d]=%g\n", p_e1->first, 0, p_e2->first, p_i, p_j, p_e2->second->phase[p_i][p_j] );
		}
	    }
	}
    }
#endif

    /* Entries */

    if ( entry_overtaking( result, passes, rms, 0 ) ) {
	print_sub_heading( result, passes, print_confidence_intervals, file_name, 32+5, header, "From Entry      To Entry        Phase" );
	entry_overtaking( result, passes, rms, print_entry );
	count += 1;
    }

    if ( count > 0 ) {
	print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
    }
}


/*
 * common code to count and print number of records.
 */

static unsigned
entry_overtaking( const result_str_t result, const unsigned passes,
		  std::vector<stats_buf>& rms, entry_waiting_func func )
{
    assert ( result == P_OVERTAKING );
    double value[MAX_PASS];
    double conf_value[MAX_PASS];
    unsigned count = 0;
    for ( std::map<int,entry_info>::const_iterator i = entry_tab[FILE1].begin(); i != entry_tab[FILE1].end(); ++i ) {
        for ( std::map<int,entry_info>::const_iterator k = entry_tab[FILE1].begin(); k != entry_tab[FILE1].end(); ++k ) {
	    for ( unsigned  p_j = 0; p_j < MAX_PHASES; ++p_j ) {	/* Phase of server */
		for ( unsigned  p_i = 0; p_i < MAX_PHASES; ++p_i ) {	/* Phase of client */
		    unsigned j;

		    /* Skip entries with all zeros as not interesting */

		    for ( j = 0; j < passes; ++j ) {
			check_ovtk( i->first, j, k->first, p_j, p_i );
/*			if ( (*result_str[(int)result].check_func)( i->first, j, k->first, p_j, p_i ) ) break; */
		    }
		    if ( j == passes ) continue;

		    count += 1;

		    if ( func ) {
			if ( !print_rms_error_only ) {
			    (void) fprintf( output, *result_str[(int)result].format,
					    find_symbol_pos( i->first, ST_ENTRY ),
					    find_symbol_pos( k->first, ST_ENTRY ),
					    p_j + 1,
					    p_i + 1 );
			}
			for ( j = 0; j < passes; ++j ) {
			    get_ovtk( value, conf_value, i->first, j, k->first, p_j, p_i );
/* 			    (*result_str[(int)result].func)( value, conf_value, i->first, j, k->first, p_j, p_i ); */
			    print_entry_activity( value, conf_value, passes, j, &rms );
			}
			if ( !print_rms_error_only ) {
			    (void) fputc( '\n', output );
			}
		    }
		}
	    }
	}
    }
    if ( !print_rms_error_only ) {
	(void) fputc( '\n', output );
    }
    return count;
}



/*
 * Print out data for entry waiting type fields.
 */

static void
print_activity_join ( const result_str_t result, const char * file_name, const unsigned passes )
{
    const unsigned n = join_tab[FILE1].size();
    if ( n == 0 ) return;

    std::vector<stats_buf> rms(passes);

    if ( !print_rms_error_only ) {
	(void) fputc( '\n', output );
    }
    print_sub_heading( result, passes, print_confidence_intervals, file_name, 32, header, "From Task:Act   To Task:Act     " );

    for ( std::map<int, std::map<int, join_info_t> >::const_iterator next_i = join_tab[FILE1].begin(); next_i != join_tab[FILE1].end(); ++next_i ) {
	unsigned i = next_i->first;
	for ( std::map<int, join_info_t>::const_iterator next_k = join_tab[FILE1][i].begin(); next_k != join_tab[FILE1][i].end(); ++next_k ) {
	    unsigned k = next_k->first;
	    print_activity( result, passes, i, k, &rms,
			    find_symbol_pos( i, ST_ACTIVITY ),
			    find_symbol_pos( k, ST_ACTIVITY ) );
	}
    }

    print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
}


/*
 * Print out data for entry service type fields.
 */

static void
print_phase_result ( const result_str_t result, const char * file_name, const unsigned passes )
{
    std::vector<stats_buf> rms(passes);

    if ( compact_flag ) {
	print_sub_heading( result, passes, print_confidence_intervals, file_name, 8+5, header, "Entry   Phase" );
    } else {
	print_sub_heading( result, passes, print_confidence_intervals, file_name, 16+5, header, "Entry           Phase" );
    }

    for ( std::map<int,entry_info>::const_iterator i = entry_tab[FILE1].begin(); i != entry_tab[FILE1].end(); ++i ) {
        unsigned p;
	for ( p = 0; p < MAX_PHASES; ++p ) {
	    unsigned j;

	    /* Skip entries with all zeros as not interesting */

	    for ( j = 0; j < passes; ++j ) {
		if ( (*result_str[(int)result].check_func)( i->first, j, p, 0 ) ) break;
	    }
	    if ( j == passes ) continue;

	    print_entry( result, passes, i->first, 0, p, &rms, find_symbol_pos( i->first, ST_ENTRY ), p+1 );
	}
    }
    if ( print_latex && !print_rms_error_only ) {
	(void) fprintf( output, "\\hline\n" );
    }

    /* Activities */
    if ( activity_tab[FILE1].size() > 0 && result_str[(int)result].act_check_func ) {
	if ( !print_rms_error_only ) {
	    (void) fputc( '\n', output );
	}
	if ( compact_flag ) {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 16+5, header, "TaskAct     " );
	} else {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, 16+5, header, "Task:Activity        " );
	}

	for ( std::map<int,activity_info>::const_iterator i = activity_tab[FILE1].begin(); i != activity_tab[FILE1].end(); ++i ) {
	    unsigned j;
	    /* Skip activities with all zeros as not interesting */

	    for ( j = 0; j < passes; ++j ) {
		if ( (*result_str[(int)result].act_check_func)( i->first, j, 0, 0 ) ) break;
	    }
	    if ( j == passes ) continue;

	    print_activity( result, passes, i->first, 0, &rms, find_symbol_pos( i->first, ST_ACTIVITY ) );
	}
	if ( print_latex && !print_rms_error_only ) {
	    (void) fprintf( output, "\\hline\n" );
	}
    }

    print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
}


/*
 * Print task results.
 */

static void
print_task_result ( const result_str_t result, const char * file_name, const unsigned passes )
{
    std::vector<stats_buf> rms(passes);
    bool first_record = true;			/* Suppress unused output if no records to print */

    for ( std::map<int,task_info>::const_iterator i = task_tab[FILE1].begin(); i != task_tab[FILE1].end(); ++i ) {
	unsigned j;
	for ( j = 0; j < passes; ++j ) {
	    if ( (*result_str[(int)result].check_func)( i->first, j, 0, 0 ) ) break;
	}
	if ( j == passes ) continue;
	if ( first_record ) {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, (compact_flag ? 8 : 16), header, "Task" );
	    first_record = false;
	}

	print_entry( result, passes, i->first, 0, 0, &rms, find_symbol_pos( i->first, ST_TASK ) );
    }
    if ( !first_record ) {
	print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
    }
}


/*
 * Print open arrival, cvsq results.
 */

static void
print_entry_result ( const result_str_t result, const char * file_name, const unsigned passes )
{
    std::vector<stats_buf> rms(passes);
    bool first_record = true;

    for ( std::map<int,entry_info>::const_iterator i = entry_tab[FILE1].begin(); i != entry_tab[FILE1].end(); ++i ) {
	unsigned j;
	for ( j = 0; j < passes; ++j ) {
	    if ( (result_str[(int)result]).check_func( i->first, j, 0, 0 ) ) break;
	}
	if ( j == passes ) continue;
	if ( first_record ) {
	    print_sub_heading( result, passes, print_confidence_intervals, file_name, (compact_flag ? 8 : 16), header, "Entry" );
	    first_record = false;
	}

	print_entry( result, passes, i->first, 0, 0, &rms, find_symbol_pos( i->first, ST_ENTRY ) );
    }
    if ( !first_record ) {
	print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
    }
}


/*
 * Print processor statistics.
 */

static void
print_group ( const result_str_t result, const char * file_name, const unsigned passes )
{
    std::vector<stats_buf> rms(passes);

    print_sub_heading( result, passes, print_confidence_intervals, file_name, (compact_flag ? 8 : 16), header, "Group" );

    for ( std::map<int,group_info>::const_iterator i = group_tab[FILE1].begin(); i != group_tab[FILE1].end(); ++i ) {
	print_entry( result, passes, i->first, 0, 0, &rms, find_symbol_pos( i->first, ST_GROUP ) );
    }
    print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
}


/*
 * Print processor statistics.
 */

static void
print_processor ( const result_str_t result, const char * file_name, const unsigned passes )
{
    std::vector<stats_buf> rms(passes);

    print_sub_heading( result, passes, print_confidence_intervals, file_name, (compact_flag ? 8 : 16), header, "Processor" );

    for ( std::map<int,processor_info>::const_iterator i = processor_tab[FILE1].begin(); i != processor_tab[FILE1].end(); ++i ) {
	print_entry( result, passes, i->first, 0, 0, &rms, find_symbol_pos( i->first, ST_PROCESSOR ) );
    }
    print_rms_error( file_name, result, rms, passes, print_confidence_intervals );
}


/*
 * Print out runtime information.
 */

/* ARGSUSED */
static void
print_runtime ( const result_str_t result, const char * file_name, const unsigned passes, const unsigned n )
{
    unsigned p;

    print_sub_heading( result, passes, false, file_name, (compact_flag ? 8 : 16), header, "Statistic" );

    for ( p = 0; p <= 2; ++p ) {
	unsigned j;
	double value[MAX_PASS];

	if ( !print_rms_error_only ) {
	    (void) fprintf( output, *result_str[(int)result].format, time_str[p] );
	}

	for ( j = 0; j < passes; ++j ) {
	    (*result_str[(int)result].func)( value, nullptr, j, p, 0, 0 );

	    if ( !print_error_only && !print_rms_error_only ) {
		time_print_func( output, 0, value[j], 0 );
	    }


	    if ( j > FILE1 ) {
		double diff = value[j] - value[FILE1];
		if ( value[FILE1] != 0.0 ) {
		    diff = diff  * 100.0 / value[FILE1];
		} else {
		    diff = 0.0;
		}
		if ( passes >= 2 && !print_rms_error_only && !print_results_only ) {
		    (void) fprintf( output, "%s", separator_format );
		    (void) fprintf( output, error_format, diff );
		}
	    }

	    /* Store time values in error arrays. */

	    total[p][result][j].update( value[j] );

	}


	if ( !print_rms_error_only ) {
	    (void) fputc( '\n', output );
	}
    }

    if ( !print_rms_error_only ) {
	(void) fputc( '\n', output );
    }
}



/* ARGSUSED */
static void
print_iteration ( const result_str_t result, const char * file_name, const unsigned passes, const unsigned n )
{
    unsigned j;
    double value[MAX_PASS];

    print_sub_heading( result, passes, false, file_name, (compact_flag ? 8 : 16), header, " " );
    if ( !print_rms_error_only ) {
	(void) fprintf( output, *result_str[(int)result].format, " " );
    }

    for ( j = 0; j < passes; ++j ) {
	(*result_str[(int)result].func)( value, nullptr, j, 0, 0, 0 );

	if ( !print_error_only && !print_rms_error_only ) {
	    (void) fprintf( output, "%s", separator_format );
	    (void) fprintf( output, result_format, value[j] );
	}


	if ( j > FILE1 ) {
	    double diff = value[j] - value[FILE1];
	    if ( value[FILE1] != 0.0 ) {
		diff = diff  * 100.0 / value[FILE1];
	    } else {
		diff = 0.0;
	    }
	    if ( passes >= 2 && !print_rms_error_only && !print_results_only ) {
		(void) fprintf( output, "%s", separator_format );
		(void) fprintf( output, error_format, diff );
	    }
	}

	/* Store time values in error arrays. */

 	total[1][result][j].update( value[j] );

    }

    if ( !print_rms_error_only ) {
	(void) fputc( '\n', output );
    }

    if ( !print_rms_error_only ) {
	(void) fputc( '\n', output );
    }
}



static void
print_information ( const result_str_t result, const char * file_name, const unsigned passes )
{
    int width = (compact_flag ? 8 : 16);

    if ( print_latex ) {
	(void) fprintf( output, header, width-1, width-1, file_name );
	(void) fprintf( output, "\\hline\n" );
    } else {
	(void) fprintf( output, "%-*.*s %s\n", width-1, width-1, file_name, header );
    }

    (void) fprintf( output, *result_str[(int)result].format, "Version" );
    for ( unsigned int j = 0; j < passes; ++j ) {

	size_t width = result_width;
	if ( j > FILE1 ) {
	    if ( passes >= 2 && !print_rms_error_only && !print_results_only ) {
		width += error_width;
		if ( confidence_intervals_present[0] || confidence_intervals_present[j] ) {		/* Allow for * when error is within confidence interval */
		    width += error_width + 1;
		}
	    }
	    // Might need multicolumn here.
	}
	
	(void) fprintf( output, "%s", separator_format );
	if ( general_tab[j].solver_info.empty() ) {
	    (void) fprintf( output, "%*s", width, "--" );
	} else {
	    (void) fprintf( output, "%*s", width, general_tab[j].solver_info.c_str() );
	}
    }
    if ( print_latex ) {
	(void) fprintf( output, " \\\\\n\\hline\n" );
	(void) fprintf( output, "\\end{tabular}\n" );
    } else {
	(void) fputc( '\n', output );
	(void) fputc( '\n', output );
    }
}



/*
 * Print a line.  Format information is stored in the result_str array which is
 * indexed by result.
 */

static void
print_entry ( const result_str_t result, const unsigned passes, unsigned i, unsigned k, unsigned p,
	      std::vector<stats_buf>* delta, ... )
{
    double value[MAX_PASS];
    double conf_value[MAX_PASS];
    va_list args;

    va_start( args, delta );
    if ( !print_rms_error_only ) {
	(void) vfprintf( output, *result_str[(int)result].format, args );
    }
    va_end( args );

    for ( unsigned int j = 0; j < passes; ++j ) {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
	(*result_str[(int)result].func)( value, conf_value, i, j, k, p );

	print_entry_activity( value, conf_value, passes, j, delta );
    }

    if ( !print_rms_error_only ) {
	if ( print_latex ) {
	    (void) fprintf( output, " \\\\" );
	}
	(void) fputc( '\n', output );
    }
}




/*
 * Print a line.  Format information is stored in the result_str array which is
 * indexed by result.
 */

static void
print_entry_2 ( const result_str_t result, const unsigned passes, unsigned i, unsigned k, 
	      std::vector<stats_buf>* delta, ... )
{
    unsigned j;
    double value[MAX_PASS];
    double conf_value[MAX_PASS];
    va_list args;

    va_start( args, delta );
    if ( !print_rms_error_only ) {
	(void) vfprintf( output, *result_str[(int)result].format, args );
    }
    va_end( args );

    for ( j = 0; j < passes; ++j ) {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
	(*result_str[(int)result].func)( value, conf_value, i, j, k, 0 );

	print_entry_activity( value, conf_value, passes, j, delta );
    }

    if ( !print_rms_error_only ) {
	if ( print_latex ) {
	    (void) fprintf( output, " \\\\" );
	}
	(void) fputc( '\n', output );
    }
}




/*
 * Print a line.  Format information is stored in the result_str array which is
 * indexed by result.
 */

static void
print_activity ( const result_str_t result, const unsigned passes, unsigned i, unsigned k,
		 std::vector<stats_buf>* delta, ... )
{
    unsigned j;
    double value[MAX_PASS];
    double conf_value[MAX_PASS];
    va_list args;

    va_start( args, delta );
    if ( !print_rms_error_only ) {
	(void) vfprintf( output, *result_str[(int)result].act_format, args );
    }
    va_end( args );

    for ( j = 0; j < passes; ++j ) {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
	(*result_str[(int)result].act_func)( value, conf_value, i, j, k, 0 );

	print_entry_activity( value, conf_value, passes, j, delta );
    }

    if ( !print_rms_error_only ) {
	if ( print_latex ) {
	    (void) fprintf( output, " \\\\" );
	}
	(void) fputc( '\n', output );
    }
}



/*
 * Common code for print_entry  and print_activity
 */

static void
print_entry_activity( double value[], double conf_value[], const unsigned passes, const unsigned j, std::vector<stats_buf>* delta )
{
    if ( !print_error_only && !print_rms_error_only ) {
	(void) fprintf( output, "%s", separator_format );
	(void) fprintf( output, result_format, value[j] );

	if ( confidence_intervals_present[j] && print_confidence_intervals ) {
	    (void) fprintf( output, "%s", separator_format );
	    double x = conf_value[j];
	    if ( print_conf_as_percent && value[j] ) {
		x = conf_value[j] * 100.0 / value[j];
	    }
	    (void) fprintf( output, confidence_format, x );
	}

    }

    /* Accumulate statistics */

    if ( j == FILE1 ) {
	(*delta)[j].update( value[FILE1] );
    } else {
	double error;
	if ( std::isfinite( value[j] ) && std::isfinite( value[FILE1] ) ) {
	    error = value[j] - value[FILE1];
	} else if ( !std::isfinite( value[j] ) && std::isfinite( value[FILE1] ) ) {
	    error = value[j];
	} else if ( std::isfinite( value[j] ) && !std::isfinite( value[FILE1] ) ) {
	    error = -value[FILE1];
	} else {
	    error = 0.0;
	}
	const double abserr = fabs(error);
	const bool ok = abserr <= conf_value[0] || abserr <= conf_value[j];		/* test either */
	if ( print_error_absolute_value ) {
	    error = abserr;
	}
	(*delta)[j].update( error, ok );
	if ( passes >= 2 && !print_rms_error_only && !print_results_only ) {
	    (void) fprintf( output, "%s", separator_format );
	    (void) fprintf( output, error_format, relative_error( error, value[FILE1] ) );
	    if ( confidence_intervals_present[0] || confidence_intervals_present[j] ) {
		(void) fprintf( output, "%s", ok ? "*" : " " );
	    }
	}
    }
}

/*
 * Print RMS errors.  Set global error flag.
 */

static void
print_rms_error ( const char * file_name, const result_str_t result, const std::vector<stats_buf> &stats,
		  unsigned passes, const bool print_conf )
{
    std::vector<double> rms(passes);
    double max_rms_error = rms_error( stats, rms );

    if ( max_rms_error > error_threshold ) {
	differences_found = true;	/* Ignore noise */
    }

    if ( !print_results_only
	 && !print_quiet
	 && passes >= 2
	 && stats[1].n() >= 1
	 && ( error_threshold == 0.0 || ( error_threshold != 0.0 && max_rms_error >= error_threshold ) ) ) {

	if ( !print_totals_only ) {
	    if ( print_rms_error_only ) {
		(void) fprintf( output, rms_format, file_name_width, file_name, result_str[(int)result].string, "RMS error" );
	    } else {
		(void) fprintf( output, "%-*.*s", *result_str[(int)result].format_width, *result_str[(int)result].format_width, "RMS error" );
	    }
	}

	for ( unsigned j = 0; j < passes; ++j ) {
	    if ( !(print_error_only || print_totals_only)) {
		(void) fprintf( output, "%s", separator_format );
		if ( j != 0 ) {
		    (void) fprintf( output, result_format, rms[j] );
		} else {
		    (void) fprintf( output, "%*c", result_width, ' ' );
		}

		if ( confidence_intervals_present[j] && print_conf ) {
		    (void) fprintf( output, "%s", separator_format );
		    (void) fprintf( output, "%*c", confidence_width, ' ' );
		}
	    }

	    if ( j != 0 && passes >= 2 ) {
		const bool ok = stats[j].ok();
		double rms_err = relative_error( rms[j], rms[0] );
		double denom = stats[0].mean();	/* Normalizing value */

		total[RMS_ERROR][(int)result][j].update( rms_err, ok );
		total[MEAN_ERROR][(int)result][j].update( relative_error( stats[j].mean(), denom ), ok );
		total[ABS_ERROR][(int)result][j].update( relative_error( stats[j].mean_abs(), denom ), ok );

		if ( !print_totals_only ) {
		    (void) fprintf( output, "%s", separator_format );
		    if ( rms[0] > 0.0 ) {
			(void) fprintf( output, error_format, rms_err );
		    } else {
			(void) fprintf( output, "%*c", error_width, ' ' );
		    }
		    if ( confidence_intervals_present[0] || confidence_intervals_present[j] ) {
			(void) fprintf( output, "%s", stats[j].ok() ? "*" : " " );
		    }
		}
	    }

	}
	if ( !print_totals_only ) {
	    if ( print_latex ) {
		(void) fprintf( output, " \\\\" );
	    }
	    (void) fputc( '\n', output );
	}
    }

    if ( !(print_rms_error_only || print_totals_only || print_quiet)) {
	if ( print_latex ) {
	    (void) fprintf( output, "\\hline\n\\end{tabular}" );
	}
	(void) fputc( '\n', output );
    }
}



/*
 * Print RMS errors.
 */

static void
print_error_totals ( unsigned passes, char * const names[] )
{
    int i;
    char header[132];
    make_header( header, names, passes, print_confidence_intervals, 1 );

    (void) fprintf( output, "\n%-*s%s\n", file_name_width+12+12, " ", header );

    for ( i = 1; i < (int)P_LIMIT; ++i ) {
	int j;
	if ( i == P_ITERATIONS || i == P_RUNTIME ) continue;
	if ( total[RMS_ERROR][i][1].n() == 0 ) continue;

	for ( j = 0; j < N_STATS; ++j ) {
	    print_total_statistics( total[j][i], 1, passes,
				    result_str[i].string, error_str[j],
				    error_print_func,
				    PRINT_MEAN|PRINT_STDDEV|PRINT_MIN|PRINT_P90TH|PRINT_MAX );
	}
    }
}



/*
 * Print RMS errors.
 */

static void
print_runtime_totals ( unsigned passes, char * const names[] )
{
    char header[132];
    int j;

    make_header( header, names, passes, false, 0 );

    (void) fprintf( output, "\n%-*s%s\n", file_name_width+12+12, " ", header );

    for ( j = 0 ; j < 3; ++j ) {
	print_total_statistics( total[j][P_RUNTIME], 0, passes,
				result_str[P_RUNTIME].string, time_str[j],
				time_print_func,
				PRINT_MEAN|PRINT_STDDEV|PRINT_MIN|PRINT_P90TH|PRINT_MAX|PRINT_SUM );	/* BUG 354 */
    }
}



static void
print_iteration_totals ( const result_str_t result, unsigned passes, char * const names[] )
{
    char header[132];

    make_header( header, names, passes, false, 2 );

    (void) fprintf( output, "\n%-*s%s\n", file_name_width+12+12, " ", header );

    print_total_statistics( total[1][(int)result], 0, passes,
			    result_str[(int)result].string, " ",
			    error_print_func,
			    PRINT_MEAN|PRINT_STDDEV|PRINT_MIN|PRINT_P90TH|PRINT_MAX|PRINT_SUM );
}



/*
 * Print statistics.  The statistics are mean, stddev, max, and min.
 */

static void
print_total_statistics ( const std::vector<stats_buf>& results, const unsigned start, const unsigned passes,
			 const char * cat_string, const char * type_str, output_func_ptr print_func, const unsigned print_bit )
{
    /* Sort values prior to finding min(), max() and p90() */

    for ( unsigned j = start; j < passes; ++j ) {
	const_cast<std::vector<stats_buf>&>(results)[j].sort();
    }

    for ( unsigned i = 0; i < N_STATISTICS; ++i ) {
	double result[MAX_PASS];
	if ( !((1 << i) & print_bit) ) continue;

	(void) fprintf( output, rms_format, file_name_width, statistic[i].str, cat_string, type_str );

	for ( unsigned j = start; j < passes; ++j ) {
	    double delta = 0.0;
	    result[j] = (results[j].*statistic[i].func)();
	    if ( j > 0 && start == 0 && result[0] != 0.0 ) {
		delta = (result[0] - result[j]) / result[0];
	    }
	    (*print_func)( output, j, result[j], delta );
	}
	(void) fputc( '\n', output );
    }

    (void) fputc( '\n', output );
}

/*
 * Printing functions
 */

/* ARGSUSED */
static void
error_print_func( FILE * output, unsigned j, double value, double delta )
{
    (void) fprintf( output, "%s", separator_format );
    (void) fprintf( output, error_format, value );
}


static void
time_print_func( FILE * output, unsigned j, double value, double delta )
{
    static char buf[32];

    /* Need to parse and extract time value */

    double csecs  = fmod( value * 100.0, 100.0 );
    double secs   = fmod( floor( value ), 60.0 );
    double mins   = fmod( floor( value / 60.0 ), 60.0 );
    double hrs    = floor( value / 3600.0 );

    (void) fprintf( output, "%s", separator_format );

    (void) snprintf( buf, 32, "%2.0f:%02.0f:%02.0f.%02.0f", hrs, mins, secs, csecs );
    (void) fprintf( output, "%*.*s", result_width, result_width, buf );

    if ( j > 0 ) {
	(void) fprintf( output, "%s", separator_format );
/* 	(void) fprintf( output, error_format, delta ); */
	(void) fprintf( output, "%*.*s", error_width, error_width, " " );
    }
}

/*
 * Functions that extract data from tables.
 */

/*ARGSUSED*/
static void
get_runt( double value[], double junk[], unsigned j, unsigned p, unsigned, unsigned )
{
    value[j]      = general_tab[j].value[p];
}

/*ARGSUSED*/
static void
get_iter( double value[], double junk[], unsigned j, unsigned, unsigned, unsigned )
{
    value[j]	  = iteration_tab[j];
}

/*ARGSUSED*/
static void
get_mvaw( double value[], double junk[], unsigned j, unsigned, unsigned, unsigned )
{
    value[j]	  = mva_wait_tab[j];
}

/*ARGSUSED*/
static void
get_vrsn( double value[], double junk[], unsigned j, unsigned, unsigned, unsigned )
{
    value[j]	  = 0.0;
}

static void
get_ovtk( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p_j, unsigned p_i )
{
    value[j]      = 0.0;
    conf_value[j] = 0.0;
    std::map<int, std::map<int, ot *> >::const_iterator p_e1 = overtaking_tab[j].find(i);
    if ( p_e1 != overtaking_tab[j].end() ) {
	std::map<int, ot *>::const_iterator p_e2 = p_e1->second.find(k);
	if ( p_e2 != p_e1->second.end() ) {
	    value[j] = p_e2->second->phase[p_j][p_i];
	}
    }
}


static void
get_wait( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    std::map<int,call_info>::const_iterator p_k = entry_tab[j][i].phase[p].to.find(k);
    if ( p_k != entry_tab[j][i].phase[p].to.end() ) {
	value[j]      = p_k->second.waiting;
	conf_value[j] = p_k->second.wait_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


static void
get_wvar( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    std::map<int,call_info>::const_iterator p_k = entry_tab[j][i].phase[p].to.find(k);
    if ( p_k != entry_tab[j][i].phase[p].to.end() ) {
	value[j]      = p_k->second.wait_var;
	conf_value[j] = p_k->second.wait_var_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


static void
get_drop( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    std::map<int,call_info>::const_iterator p_k = entry_tab[j][i].phase[p].to.find(k);
    if ( p_k != entry_tab[j][i].phase[p].to.end() ) {
	value[j]      = p_k->second.loss_probability;
	conf_value[j] = p_k->second.loss_prob_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


/*ARGSUSED*/
static void
get_fwdw( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,call_info>::const_iterator fwd_k = entry_tab[j][i].fwd_to.find(k);
    if ( fwd_k != entry_tab[j][i].fwd_to.end() ) {
	value[j]      = fwd_k->second.waiting;
	conf_value[j] = fwd_k->second.wait_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


/*ARGSUSED*/
static void
get_fwdv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,call_info>::const_iterator fwd_k = entry_tab[j][i].fwd_to.find(k);
    if ( fwd_k != entry_tab[j][i].fwd_to.end() ) {
	value[j]      = fwd_k->second.wait_var;
	conf_value[j] = fwd_k->second.wait_var_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}

static void
get_serv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    value[j]      = entry_tab[j][i].phase[p].service;
    conf_value[j] = entry_tab[j][i].phase[p].serv_conf;
}


static void
get_vari( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    value[j]      = entry_tab[j][i].phase[p].variance;
    conf_value[j] = entry_tab[j][i].phase[p].var_conf;
}


static void
get_exce( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    value[j]      = entry_tab[j][i].phase[p].exceeded;
    conf_value[j] = entry_tab[j][i].phase[p].exceed_conf;
}


void
get_cvsq( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned xp )
{
    double var  = 0.0;
    double mean = 0.0;
    unsigned p;

    for ( p = 0; p < phases; ++p ) {
	var  += entry_tab[j][i].phase[p].variance;
	mean += entry_tab[j][i].phase[p].service;
    }
    if ( mean ) {
	value[j] =  var / (mean * mean);
    } else {
	value[j] = 0;
    }
    conf_value[j] = 0;
}


/*+ RWLOCK */
/*ARGSUSED*/
static void
get_rput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].rwlock_reader_utilization;
    conf_value[j] = task_tab[j][i].rwlock_reader_utilization_conf;
}

/*ARGSUSED*/
static void
get_wput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].rwlock_writer_utilization;
    conf_value[j] = task_tab[j][i].rwlock_writer_utilization_conf;
}

/*ARGSUSED*/
static void
get_rpwt( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].rwlock_reader_waiting;
    conf_value[j] = task_tab[j][i].rwlock_reader_waiting_conf;
}

/*ARGSUSED*/
static void
get_wpwt( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].rwlock_writer_waiting;
    conf_value[j] = task_tab[j][i].rwlock_writer_waiting_conf;
}

/*ARGSUSED*/
static void
get_rpht( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].rwlock_reader_holding;
    conf_value[j] = task_tab[j][i].rwlock_reader_holding_conf;
}

/*ARGSUSED*/
static void
get_wpht( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].rwlock_writer_holding;
    conf_value[j] = task_tab[j][i].rwlock_writer_holding_conf;
}
/*-RWLOCK */



/*+ BUG_164 */
/*ARGSUSED*/
static void
get_sput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].semaphore_utilization;
    conf_value[j] = task_tab[j][i].semaphore_utilization_conf;
}

/*ARGSUSED*/
static void
get_spwt( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].semaphore_waiting;
    conf_value[j] = task_tab[j][i].semaphore_waiting_conf;
}
/*- BUG_164 */


/*ARGSUSED*/
static void
get_entp( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = entry_tab[j][i].throughput;
    conf_value[j] = entry_tab[j][i].throughput_conf;
}


/*ARGSUSED*/
static void
get_tput( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    value[j]      = task_tab[j][i].throughput;
    conf_value[j] = task_tab[j][i].throughput_conf;
}


/*ARGSUSED*/
static void
get_enut( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = entry_tab[j][i].utilization;
    conf_value[j] = entry_tab[j][i].utilization_conf;
}

/*ARGSUSED*/
static void
get_util( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].utilization;
    conf_value[j] = task_tab[j][i].utilization_conf;
}

static void
get_tutl( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    value[j]      = task_tab[j][i].total_utilization[p];
    conf_value[j] = task_tab[j][i].total_utilization_conf[p];
}

/*ARGSUSED*/
static void
get_tpru( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = task_tab[j][i].processor_utilization;
    conf_value[j] = task_tab[j][i].processor_utilization_conf;
}

/*ARGSUSED*/
static void
get_enpr( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = entry_tab[j][i].processor_utilization;
    conf_value[j] = entry_tab[j][i].processor_utilization_conf;
}

/*ARGSUSED*/
static void
get_open( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = entry_tab[j][i].open_waiting;
    conf_value[j] = entry_tab[j][i].open_wait_conf;
}


/*ARGSUSED*/
static void
get_putl( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,processor_info>::const_iterator p_i = processor_tab[j].find(i);
    if ( p_i != processor_tab[j].end() ) {
	value[j] = processor_tab[j][i].utilization;
	conf_value[j] = processor_tab[j][i].utilization_conf;
    }
}


/*ARGSUSED*/
static void
get_gutl( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,group_info>::const_iterator p_i = group_tab[j].find(i);
    if ( p_i != group_tab[j].end() ) {
	value[j]  = group_tab[j][i].utilization;
	conf_value[j] = group_tab[j][i].utilization_conf;
    }
}


static void
get_phut( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    value[j]      = entry_tab[j][i].phase[p].utilization;
    conf_value[j] = entry_tab[j][i].phase[p].utilization_conf;
}


void
static get_pwat( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    value[j]      = entry_tab[j][i].phase[p].processor_waiting;
    conf_value[j] = entry_tab[j][i].phase[p].processor_waiting_conf;
}


static void
get_snrw( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    std::map<int,call_info>::const_iterator p_k = entry_tab[j][i].phase[p].to.find(k);
    if ( p_k != entry_tab[j][i].phase[p].to.end() ) {
	value[j]      = p_k->second.snr_waiting;
	conf_value[j] = p_k->second.snr_wait_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


static void
get_snrv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned p )
{
    std::map<int,call_info>::const_iterator p_k = entry_tab[j][i].phase[p].to.find(k);
    if ( p_k != entry_tab[j][i].phase[p].to.end() ) {
	value[j]      = p_k->second.snr_wait_var;
	conf_value[j] = p_k->second.snr_wait_var_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


/*ARGSUSED*/
static void
get_act_wait( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = 0.0;
    conf_value[j] = 0.0;
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	std::map<int,call_info>::const_iterator p_k = p_i->second.to.find(k);
	if ( p_k != p_i->second.to.end() ) {
	    value[j]      = p_k->second.waiting;
	    conf_value[j] = p_k->second.wait_conf;
	}
    }
}


/*ARGSUSED*/
static void
get_act_drop( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = 0.0;
    conf_value[j] = 0.0;
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	std::map<int,call_info>::const_iterator p_k = p_i->second.to.find(k);
	if ( p_k != p_i->second.to.end() ) {
	    value[j]      = p_k->second.loss_probability;
	    conf_value[j] = p_k->second.loss_prob_conf;
	}
    }
}

/*ARGSUSED*/
static void
get_act_wvar( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = 0.0;
    conf_value[j] = 0.0;
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	std::map<int,call_info>::const_iterator p_k = p_i->second.to.find(k);
	if ( p_k != p_i->second.to.end() ) {
	    value[j]      = p_k->second.wait_var;
	    conf_value[j] = p_k->second.wait_var_conf;
	}
    }
}

/*ARGSUSED*/
static void
get_join( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = 0.;
    conf_value[j] = 0.;

    std::map<int, std::map<int,join_info_t> >::const_iterator p_i = join_tab[j].find(i);
    if ( p_i != join_tab[j].end() ) {
	std::map<int,join_info_t>::const_iterator p_k = p_i->second.find(k);
	if ( p_k != p_i->second.end() ) {
	    value[j]      = p_k->second.mean;
	    conf_value[j] = p_k->second.mean_conf;
	}
    }
}

/*ARGSUSED*/
static void
get_jvar( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = 0.;
    conf_value[j] = 0.;

    std::map<int, std::map<int,join_info_t> >::const_iterator p_i = join_tab[j].find(i);
    if ( p_i != join_tab[j].end() ) {
	std::map<int,join_info_t>::const_iterator p_k = p_i->second.find(k);
	if ( p_k != p_i->second.end() ) {
	    value[j]      = p_k->second.variance;
	    conf_value[j] = p_k->second.variance_conf;
	}
    }
}

/*ARGSUSED*/
static void
get_act_serv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	value[j]      = p_i->second.service;
	conf_value[j] = p_i->second.serv_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


/*ARGSUSED*/
static void
get_act_vari( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	value[j]      = p_i->second.variance;
	conf_value[j] = p_i->second.var_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


/*ARGSUSED*/
static void
get_act_exce( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	value[j]      = p_i->second.exceeded;
	conf_value[j] = p_i->second.exceed_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


/*ARGSUSED*/
static void
get_act_pwat( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	value[j]      = p_i->second.processor_waiting;
	conf_value[j] = p_i->second.processor_waiting_conf;
    } else {
	value[j]      = 0.0;
	conf_value[j] = 0.0;
    }
}


/*ARGSUSED*/
static void
get_act_snrw( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = 0.0;
    conf_value[j] = 0.0;
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	std::map<int,call_info>::const_iterator p_k = p_i->second.to.find(k);
	if ( p_k != p_i->second.to.end() ) {
	    value[j]      = p_k->second.snr_waiting;
	    conf_value[j] = p_k->second.snr_wait_conf;
	}
    }
}


/*ARGSUSED*/
static void
get_act_snrv( double value[], double conf_value[], unsigned i, unsigned j, unsigned k, unsigned )
{
    value[j]      = 0.0;
    conf_value[j] = 0.0;
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	std::map<int,call_info>::const_iterator p_k = p_i->second.to.find(k);
	if ( p_k != p_i->second.to.end() ) {
	    value[j]      = p_k->second.snr_wait_var;
	    conf_value[j] = p_k->second.snr_wait_var_conf;
	}
    }
}

/*
 * Functions that check data in tables.
 */

static bool
check_wait( unsigned i, unsigned j, unsigned k, unsigned p )
{
    std::map<int,call_info>::const_iterator p_k = entry_tab[j][i].phase[p].to.find(k);
    return p_k != entry_tab[j][i].phase[p].to.end() && p_k->second.waiting > 0.0;
}

static bool
check_fwdw( unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,call_info>::const_iterator fwd_k = entry_tab[j][i].fwd_to.find(k);
    return fwd_k != entry_tab[j][i].fwd_to.end() && fwd_k->second.waiting > 0.0;
}

static bool
check_snrw( unsigned i, unsigned j, unsigned k, unsigned p )
{
    std::map<int,call_info>::const_iterator p_k = entry_tab[j][i].phase[p].to.find(k);
    return p_k != entry_tab[j][i].phase[p].to.end() && (std::isinf(p_k->second.snr_waiting) || p_k->second.snr_waiting > 0.0);
}


static bool
check_join( unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int, std::map<int,join_info_t> >::const_iterator p_i = join_tab[j].find(i);
    if ( p_i != join_tab[j].end() ) {
	std::map<int,join_info_t>::const_iterator p_k = p_i->second.find(k);
	return p_k != p_i->second.end();
    }
    return false;
}

static bool
check_act_wait( unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	std::map<int,call_info>::const_iterator p_k = p_i->second.to.find(k);
	return p_k != p_i->second.to.end() && p_k->second.waiting > 0.0;
    } else {
	return false;
    }
}


static bool
check_act_snrw( unsigned i, unsigned j, unsigned k, unsigned )
{
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    if ( p_i != activity_tab[j].end() ) {
	std::map<int,call_info>::const_iterator p_k = p_i->second.to.find(k);
	return p_k != p_i->second.to.end() && p_k->second.snr_waiting > 0.0;
    } else {
	return false;
    }
}


/*ARGSUSED*/
static bool
check_serv( unsigned i, unsigned j, unsigned p, unsigned )
{
    return static_cast<bool>( entry_tab[j][i].phase[p].service > 0.0 );
}

/*ARGSUSED*/
static bool
check_entp( unsigned i, unsigned j, unsigned, unsigned )
{
    return entry_tab[j][i].throughput;
}

/*ARGSUSED*/
static bool
check_act_serv( unsigned i, unsigned j, unsigned, unsigned )
{
    std::map<int,activity_info>::const_iterator p_i = activity_tab[j].find(i);
    return p_i != activity_tab[j].end() && p_i->second.service > 0.0;
}

/*ARGSUSED*/
static bool
check_cvsq( unsigned i, unsigned j, unsigned, unsigned )
{
    for ( unsigned p = 0; p < phases; ++p ) {
	if ( entry_tab[j][i].phase[p].service > 0.0 ) return true;
    }
    return false;
}

/*+ BUG_164 */
/*ARGSUSED*/
static bool
check_hold( unsigned i, unsigned j, unsigned, unsigned )
{
    return static_cast<bool>( task_tab[j][i].semaphore_waiting > 0.0 );
}
/*- BUG_164 */

/*+ RWLOCK */

static bool
check_rwlk( unsigned i, unsigned j, unsigned, unsigned )
{
    return static_cast<bool>( (task_tab[j][i]. rwlock_reader_holding > 0.0 ) ||(task_tab[j][i]. rwlock_writer_holding > 0.0 ));
}
/*- RWLOCK */

static bool
check_tput( unsigned i, unsigned j, unsigned, unsigned )
{
    return task_tab[j][i].has_results;
}


static bool
check_open( unsigned i, unsigned j, unsigned, unsigned )
{
    return static_cast<bool>( entry_tab[j][i].open_arrivals );
}

/*ARGSUSED*/
static bool
check_proc( unsigned i, unsigned j, unsigned, unsigned )
{
    return true;
}


/*ARGSUSED*/
static bool
check_grup( unsigned i, unsigned j, unsigned, unsigned )
{
    return true;
}


static bool
check_ovtk( unsigned i, unsigned j, unsigned k, unsigned p_j, unsigned p_i  )
{
    std::map<int, std::map<int, ot *> >::const_iterator p_e1 = overtaking_tab[j].find(i);
    if ( p_e1 != overtaking_tab[j].end() ) {
	std::map<int, ot *>::const_iterator p_e2 = p_e1->second.find(k);
#if 0
	if ( p_e2 != p_e1->second.end() ) {
	    fprintf( stderr, "OT(%d,%d,%d)[%d][%d]=%g\n", i, j, k, p_i, p_j, p_e2->second->phase[p_j][p_i] );
	    return true;
	}
#else
	return p_e2 != p_e1->second.end() && p_e2->second->phase[p_j][p_i] != 0;
#endif
    }
    return false;
}


/*
 * Commonize.  Eliminate files not found in all directories.  Print
 * warning messages as appropriate.
 */

static void
commonize (unsigned n, char *const dirs[])
{
    unsigned i;		/* Index across directories.	*/

    for ( i = 1; i < n; ++i ) {
	unsigned j;	/* Index into file names in dir	*/
	for ( j = 0; j < dir_list[0].gl_pathc && j < dir_list[i].gl_pathc; ) {
	    char * src = basename( dir_list[0].gl_pathv[j] );
	    char * dst = basename( dir_list[i].gl_pathv[j] );
	    int result = strcmp( src, dst );

	    if ( result != 0 ) {
		/* Check for xml/p variance */
		char * p = strrchr( src, '.' );
		char * q = strrchr( dst, '.' );
		for ( ; p && q && *p != '.' && *p == *q; ++p, ++q );
		if ( p && q && *p == '.' && *q == '.'
		     && ( ( strcmp( p, ".p" ) == 0 && ( strcmp( q, ".lqxo" ) == 0 || strcmp( q, ".xml" ) == 0 || strcmp( q, ".lqjo" ) == 0 || strcmp( q, ".json" ) == 0 ) )
			  || ( strcmp( q, ".p" ) == 0 && ( strcmp( p, ".lqxo" ) == 0 || strcmp( p, ".xml" ) == 0 || strcmp( p, ".lqjo" ) == 0 || strcmp( p, ".json" ) == 0 ) ) ) ) {
		    result = 0;
		}
	    }

	    if ( result > 0 ) {
		unsigned k;		/* Index for squishing list.	*/

		/* dir2 entry not found -- shift and delete... */

		(void) fprintf( stderr, "%s: file \"%s\" not found in \"%s\"\n", lq_toolname,
				basename( dir_list[i].gl_pathv[j] ), dirs[0] );
		(void) free( dir_list[i].gl_pathv[j] );
		for ( k = j + 1; k < dir_list[i].gl_pathc; ++k ) {
		    dir_list[i].gl_pathv[k-1] = dir_list[i].gl_pathv[k];
		}
		dir_list[i].gl_pathv[k] = (char *)0;
		dir_list[i].gl_pathc -= 1;

	    } else if ( result < 0 ) {
		unsigned l;		/* Index for squishing dirs.	*/

		/* dir1 entry not found -- shift and delete all dirs up to dir2 */

		(void) fprintf( stderr, "%s: file \"%s\" not found in \"%s\"\n", lq_toolname,
				basename( dir_list[0].gl_pathv[j] ), dirs[i] );
		for ( l = 0; l < i; ++l ) {
		    unsigned k;		/* Index for squishing list.	*/
		    (void) free( dir_list[l].gl_pathv[j] );
		    for ( k = j + 1; k < dir_list[l].gl_pathc; ++k ) {
			dir_list[l].gl_pathv[k-1] = dir_list[l].gl_pathv[k];
		    }
		    dir_list[l].gl_pathv[k] = (char *)0;
		    dir_list[l].gl_pathc -= 1;
		}

	    } else {
		++j;
	    }
	}
    }
}

/*----------------------------------------------------------------------*/
/*		 	Adding Data to the Lists			*/
/*----------------------------------------------------------------------*/

static unsigned int
find_or_add( st_etypes type, const char * name )
{
    if ( std::any_of( exclude[type].begin(), exclude[type].end(), match( name ) ) 
	 || ( !include[type].empty() && std::none_of( include[type].begin(), include[type].end(), match( name ) ) ) ) return 0;

    unsigned int x = find_symbol_name( name, type );
    if ( x == 0 ) {
	if ( pass != FILE1 ) {
	    results_warning( "%s %s not found in source file", st_namespace_names[type], name );
	} else {
	    x = add_symbol( name, type );
	}
    }
    return x;
}


unsigned int find_or_add_processor( const char * processor ) { return find_or_add( ST_PROCESSOR, processor ); }
unsigned int find_or_add_group( const char * group ) { return find_or_add( ST_GROUP, group ); }
unsigned int find_or_add_task( const char * task ) { return find_or_add( ST_TASK, task ); }
unsigned int find_or_add_entry( const char * entry ) { return find_or_add( ST_ENTRY, entry ); }


unsigned int
find_or_add_activity( const char * task, const char * activity )
{

    if ( std::any_of( exclude[ST_TASK].begin(), exclude[ST_TASK].end(), match( task ) ) ) return 0;
    if ( !include[ST_TASK].empty() && std::none_of( include[ST_TASK].begin(), include[ST_TASK].end(), match( task ) ) ) return 0;

    const std::string buf = std::string(task) + ":" + std::string(activity);
    unsigned a = find_symbol_name( buf.c_str(), ST_ACTIVITY );
    if ( a == 0 ) {
	if ( pass != FILE1 ) {
	    results_warning( "Task %s, activity %s not found in source file", task, activity );
	} else {
	    a = add_symbol( buf.c_str(), ST_ACTIVITY );
	}
    }

    return a;
}

/*
 * Load results from either the existing XML input, or from the named file.  Return 0 on sucess.
 */

static int readInResults(const char *filename)
{
    unsigned error_code = 0;

    if ( strcmp( filename, "-" ) == 0 ) {
	if ( list_of_files == stdin ) {
	    error_code = 1;
	    fprintf( stderr, "%s: stdin cannot be used as an input file.\n", lq_toolname );
	} else {
	    resultin = stdin;
	    input_file_name = "stdin";
	    error_code = resultparse();
	}
    } else {

	/*
	 * If there is a DOM loaded, I am going to assume results will be parsed from DOM in memory, and not a new file.
	 */

	const char * p = strrchr( filename, '.' );
	resultlineno = 1;
	if ( p && strcasecmp( p, ".p" ) == 0 ) {
	    resultin = my_fopen( filename, "r" );
	    if ( resultin ) {
		input_file_name = filename;
		error_code = resultparse();
		fclose( resultin );
	    } else {
		error_code = errno;
	    }
	} else if ( p && (strcasecmp( p, ".json" ) == 0 || strcasecmp( p, ".lqjo" ) == 0 ) ) {
	    if ( !LQIO::DOM::Json_Document::load( filename ) ) {
		error_code = 1;
	    }
#if HAVE_LIBEXPAT
	} else {
	    if ( !LQIO::DOM::Expat_Document::load( filename ) ) {
		error_code = 1;
	    }
#endif
	}
    }
    return error_code;
}


static int
results_warning( const char * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    verrprintf( stdout, LQIO::WARNING_ONLY, input_file_name, resultlineno, 0, fmt, args );
    va_end( args );
    return 0;
}


void
results_error( const char * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    verrprintf( stdout, LQIO::WARNING_ONLY, input_file_name, resultlineno, 0, fmt, args );
    va_end( args );
}


/*
 * For quicksort (qsort).  It passes pointers to elements.
 */

int
mystrcmp ( const void *a, const void *b )
{
    char * s1 = *(char **)a;
    char * s2 = *(char **)b;

    if ( s1 && s2 ) {
	return strcmp( s1, s2 );
    } else if ( s1 == 0 && s2 == 0 ) {
	return 0;
    } else if ( s1 == 0 ) {
	return 1;
    } else {
	return -1;
    }
}



/*
 * Calculate RMS error.
 */

double
rms_error ( const std::vector<stats_buf>& stats, std::vector<double>& rms )
{
    double max = 0.0;
    std::vector<stats_buf>::const_iterator s;
    std::vector<double>::iterator r;

    for ( s = stats.begin(), r = rms.begin(); s != stats.end() && r != rms.end(); ++s, ++r ) {
	if ( s->n_ == 0 ) {
	    (*r) = 0.0;
	} else if ( std::isfinite( s->sum_sqr ) ) {
	    (*r) = sqrt( s->sum_sqr / static_cast<double>(s->n_) );
	} else {
	    (*r) = s->sum_sqr;
	}
	if ( r != rms.begin() && *rms.begin() > 0.0 ) {
	    double temp = relative_error( (*r), *rms.begin() );
	    if ( temp > max ) max = temp;
	}
    }
    return max;
}

void
stats_buf::resize ( const unsigned int n )
{
    values.resize( n, 0. );
}


void
stats_buf::update ( double value, bool ok )
{
    if ( n_ < values.size() ) {
	values[n_] = value;
    }
    n_ += 1;
    if ( std::isfinite( value ) ) {
	sum_    += value;
	sum_sqr += value * value;
	sum_abs += fabs( value );
    } else {
	sum_    = value;
	sum_sqr = value;
	sum_abs = fabs( value );
    }
    all_ok = all_ok && ok;
}


double
stats_buf::mean() const
{
    if ( n_ > 0 ) {
	return sum_ / (double)n_;
    } else {
	return 0.0;
    }
}

double
stats_buf::stddev() const
{
    if ( n_ < 2 ) {
	return 0.0;
    } else if ( std::isfinite( sum_sqr ) ) {
	double mean_sqr = ( sum_ * sum_ ) / n_;
	if ( mean_sqr > 0 ) {
	    return sqrt( ( sum_sqr - mean_sqr ) / n_ );
	} else {
	    return 0.0;
	}
    } else {
	return sum_sqr;
    }
}


double
stats_buf::mean_abs() const
{
    if ( n_ > 0 ) {
	return sum_abs / (double)n_;
    } else {
	return 0.0;
    }
}


double
stats_buf::min() const
{
    if ( values.size() > 0 ) {
	return values[0];
    } else {
	return 0.0;
    }
}


double
stats_buf::max() const
{
    if ( values.size() > 0 && n_ > 0 ) {
	return values[n_-1];
    } else {
	return 0.0;
    }
}


double
stats_buf::p90th() const
{
    if ( values.size() > 0 && n_ > 0 ) {
	return values[static_cast<int>((n_-1) * 0.9)];
    } else {
	return 0.;
    }
}


void
stats_buf::sort()
{
    std::sort( values.begin(), values.end() );
}

static double
relative_error( const double a, const double b )
{
    if ( a == b ) {
	return 0;
    } else if ( b == 0 ) {
	return std::numeric_limits<double>::infinity();
    } else if ( std::isfinite( a ) ) { 			/* BUG_171 */
	return a * 100.0 / b;
    } else {
	return a;
    }
}



extern "C" {
    int yywrap();
}

/*
 * Keep -linput happy.
 */

int
yywrap()
{
    return 1;
}
