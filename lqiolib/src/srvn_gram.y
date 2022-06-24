/*
 * $id: srvn_gram.y 14381 2021-01-19 18:52:02Z greg $ 
 */

%{
#define YYDEBUG 1
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "input.h"
#include "srvn_input.h"
#include "srvn_spex.h"

static char * make_name( int i );
static void * curr_task = 0;
static void * curr_proc = 0;
static void * curr_group = 0;
static void * curr_entry = 0;
static void * dest_entry = 0;
static void * curr_activity = 0;
static bool constant_expression = true;
extern int LQIO_lex();
%}

%token <aString>	TEXT END_LIST SYMBOL VARIABLE RANGE_ERR SOLVER
%token <anInt>		INTEGER
%token <aFloat>		FLOAT CONST_INFINITY
%token			TRANSITION

/*+ spex */
%token <anInt>		KEY_UTILIZATION KEY_THROUGHPUT KEY_PROCESSOR_UTILIZATION KEY_SERVICE_TIME KEY_VARIANCE KEY_THROUGHPUT_BOUND KEY_PROCESSOR_WAITING KEY_WAITING KEY_WAITING_VARIANCE KEY_ITERATIONS KEY_ELAPSED_TIME KEY_USER_TIME KEY_SYSTEM_TIME KEY_EXCEEDED_TIME
%token			TOK_LESS_EQUAL TOK_LOGIC_NOT TOK_LESS_THAN TOK_NOT_EQUALS TOK_GREATER_EQUAL TOK_EQUALS TOK_LOGIC_AND TOK_GREATER_THAN TOK_LOGIC_OR TOK_POWER
%token			SRVN_INPUT SPEX_PARAMETER SPEX_RESULT SPEX_CONVERGENCE SPEX_EXPRESSION
/*- spex */

%union {
    int anInt;
    double aFloat;
    void * aVariable;
    void * domObject;
    char * aString;
    void * entryList;
    void * activityList;
    scheduling_type schedulingFlag;
    void * aParseTreeNode;
    void * aParseTreeList;
}

%type <aVariable>	act_prob act_count group_share conv_val it_limit print_int underrelax_coeff
%type <aVariable>	real integer quorum_count rvalue
%type <domObject>	entry_ref dest_ref activity_def activity_ref task_ref
%type <schedulingFlag>	proc_sched_flag proc_sched_quantum task_sched_flag
%type <entryList>	entry_list act_entry_list
%type <activityList>	join_list fork_list and_join_list and_fork_list or_join_list or_fork_list loop_list
%type <anInt>		cap_flag hist_bins 
%type <aFloat>		constant 
%type <aParseTreeNode>  forall_expr ternary_expr assignment or_expr and_expr compare_expr expression term power prefix arrayref factor 
%type <aParseTreeNode>  opt_report_info r_decl c_decl 
%type <aParseTreeList>	parameter_list expression_list r_decl_list c_decl_list opt_convergence_info

/*
 *			   *** WARNING ***
 *	Each of these non-terminals returns a mallocated string/new Operator *
 *	Don't forget to free() it iff not used.
 */

%type <aString>		symbol proc_id entry_id task_id group_id activity_id reply_activity comment 

%%
/*----------------------------------------------------------------------*/
/* Special code for running parts of the parser.			*/
/*----------------------------------------------------------------------*/

start			: SRVN_INPUT SRVN_input_file
/*+ json */
			| SPEX_PARAMETER parameter_list { spex_set_parameter_list( $2 ); }
			| SPEX_RESULT r_decl_list 	{ spex_set_result_list( $2 ); }
			| SPEX_CONVERGENCE c_decl_list	{ spex_set_convergence_list( $2 ); }
			| SPEX_EXPRESSION expression  	{ spex_set_variable( spex_inline_expression( $2 ) ) ; }
/*- json */
			;

/*----------------------------------------------------------------------*/
/* Input file grammar.							*/
/*----------------------------------------------------------------------*/

SRVN_input_file		: parameter_list srvn_spec opt_report_info opt_convergence_info			/* spex */
				{ spex_set_program( $1, $3, $4 ); }
    			;

srvn_spec		: general_info processor_info group_info task_info entry_info activity_info_list
    			;

/* --------------------- General information -------------------------- */

general_info		: 'G' comment conv_val it_limit print_int underrelax_coeff END_LIST
				{ srvn_set_model_parameters( $2, $3, $4, $5, $6 ); free( $2 ); }
				general_obs
    			| 'G' comment conv_val it_limit underrelax_coeff END_LIST
				{ srvn_set_model_parameters( $2, $3, $4, 0, $5 ); free( $2 ); }
				general_obs
    			| 'G' comment conv_val it_limit END_LIST
				{ srvn_set_model_parameters( $2, $3, $4, 0, 0 ); free( $2 ); }
				general_obs
    			| 'G' comment END_LIST
				{ srvn_set_model_parameters( $2, 0, 0, 0, 0 ); free( $2 ); }
				general_obs
			|
				{ srvn_set_model_parameters( "", 0, 0, 0, 0 ); }
			;

comment			: TEXT 			/* Comment on the model			*/
			;

conv_val		: real 			/* convergence value			*/
    			;

it_limit		: integer 		/* iteration limit			*/
    			;

print_int		: integer 		/*  intermediate result print interval  */
    			;

underrelax_coeff  	: real			/*  under-relaxation coefficient	*/
    			;

/*+ spex */
general_obs		: general_obs_info general_obs
			|
			;


general_obs_info	: KEY_WAITING VARIABLE				{ spex_document_observation( KEY_WAITING, $2 ); }		/* %w waits */
			| KEY_SERVICE_TIME VARIABLE			{ spex_document_observation( KEY_SERVICE_TIME, $2 ); }		/* %s steps */
			| KEY_ITERATIONS VARIABLE			{ spex_document_observation( KEY_ITERATIONS, $2 ); }
			| KEY_ELAPSED_TIME VARIABLE			{ spex_document_observation( KEY_ELAPSED_TIME, $2 ); }
			| KEY_USER_TIME VARIABLE			{ spex_document_observation( KEY_USER_TIME, $2 ); }
			| KEY_SYSTEM_TIME VARIABLE			{ spex_document_observation( KEY_SYSTEM_TIME, $2 ); }
			;


/*- spex */

/* -------------------------- Parameter list -------------------------- */
/*+ spex */
parameter_list		: 						{ $$ = NULL; }
			| parameter_list forall_expr			{ $$ = spex_list( $1, $2 ); }
			;

forall_expr		: assignment					{ $$ = $1; }
			| VARIABLE ',' VARIABLE '=' ternary_expr	{ $$ = spex_forall( $1, $3, $5 ); }
			;

assignment		: VARIABLE '=' ternary_expr			{ $$ = spex_assignment_statement( $1, $3, constant_expression ); constant_expression = true; }
			| VARIABLE '=' '[' expression_list ']'		{ $$ = spex_array_assignment( $1, $4, constant_expression ); constant_expression = true; }
			| VARIABLE '=' '[' constant ':' constant ',' constant ']'
									{ $$ = spex_array_comprehension( $1, $4, $6, $8 ); constant_expression = true; }
			| VARIABLE '=' TEXT				{ $$ = spex_assignment_statement( $1, spex_get_string( $3 ), true ); free( $3 ); }
			| SOLVER					{ srvnwarning( "Spex control variable \"%s\" is not supported.", $1 ); $$ = NULL; }		/* Silently ignore $solver */
			;

ternary_expr		: or_expr '?' or_expr ':' or_expr 		{ $$ = spex_ternary( $1, $3, $5 ); }
			| or_expr					{ $$ = $1; }
			;

or_expr			: or_expr TOK_LOGIC_OR and_expr			{ $$ = spex_or( $1, $3 ); }
			| and_expr					{ $$ = $1; }

and_expr		: and_expr TOK_LOGIC_AND compare_expr		{ $$ = spex_and( $1, $3 ); }
			| compare_expr					{ $$ = $1; }
			;

compare_expr		: compare_expr TOK_EQUALS expression		{ $$ = spex_equals( $1, $3 ); }
			| compare_expr TOK_NOT_EQUALS expression	{ $$ = spex_not_equals( $1, $3 ); }
			| compare_expr TOK_LESS_THAN expression 	{ $$ = spex_less_than( $1, $3 ); }
			| compare_expr TOK_LESS_EQUAL expression 	{ $$ = spex_less_than_or_equals( $1, $3 ); }
			| compare_expr TOK_GREATER_THAN expression 	{ $$ = spex_greater_than( $1, $3 ); }
			| compare_expr TOK_GREATER_EQUAL expression	{ $$ = spex_greater_than_or_equals( $1, $3 ); }
			| expression					{ $$ = $1; }
			;

expression		: expression '+' term				{ $$ = spex_add( $1, $3 ); }
			| expression '-' term				{ $$ = spex_subtract( $1, $3 ); }
			| term						{ $$ = $1; }
			;

term			: term '*' power				{ $$ = spex_multiply( $1, $3 ); }
			| term '/' power				{ $$ = spex_divide( $1, $3 ); }
			| term '%' power				{ $$ = spex_modulus( $1, $3 ); }
			| power						{ $$ = $1; }
			;

power			: prefix TOK_POWER power			{ $$ = spex_power( $1, $3 ); }
			| prefix					{ $$ = $1; }
			;

prefix			: TOK_LOGIC_NOT arrayref			{ $$ = spex_not( $2 ); }
			| arrayref					{ $$ = $1; }
			;

arrayref		: arrayref '[' expression ']'			{ $$ = spex_array_reference( $1, $3 ); }
			| factor
			;

factor			: '(' expression ')'				{ $$ = $2; }			/* See Parser_pre.ypp: basic_stmt(X) */
			| rvalue '(' ')'				{ $$ = spex_invoke_function( $1, NULL ); }
			| rvalue '(' expression_list ')'		{ $$ = spex_invoke_function( $1, $3 ); }
			| VARIABLE					{ $$ = spex_get_symbol( $1 ); constant_expression = false; }
			| constant					{ $$ = spex_get_real( $1 ); }
			;

expression_list		: expression					{ $$ = spex_list( NULL, $1 ); }
			| expression_list ',' expression		{ $$ = spex_list( $1, $3 ); }
			;

/*- spex */
/* ----------------------- Processor information ---------------------- */

processor_info  	: 'P' np p_decl_list END_LIST
    			;

np  			: INTEGER 		/*  total number of processors		*/
			;

p_decl_list		: p_decl
			| p_decl_list p_decl
    			;

proc_id			: symbol
			;

			/* 1  2       3               4       5                 6                7 */
p_decl			: 'p' proc_id proc_sched_flag 
				{ curr_proc = srvn_add_processor( $2, $3, NULL ); (void) free( $2 );	}
				proc_opts proc_obs
			| 'p' proc_id proc_sched_quantum real
				{ curr_proc = srvn_add_processor( $2, $3, $4 ); (void) free( $2 );	}
				proc_opts proc_obs
			| 'd' proc_id proc_id real
				{ srvn_add_communication_delay( $2, $3, $4 ); }
    			;

proc_sched_flag 	: 'f'	{ $$ = SCHEDULE_FIFO; }			/* First come first served.		*/
			| 'i'	{ $$ = SCHEDULE_DELAY; }		/* Infinite Server			*/
			| 'r'	{ $$ = SCHEDULE_RAND; }			/* Random scheduling (!?!)		*/
    			| 'h'	{ $$ = SCHEDULE_HOL; }			/* Head of line.			*/
    			| 'p'	{ $$ = SCHEDULE_PPR; }			/* Priority, preemptive resume		*/
    			;

proc_sched_quantum 	: 'c'	{ $$ = SCHEDULE_CFS; }			/* Completely fair share		*/
    			| 's'	{ $$ = SCHEDULE_PS; }			/* Processor Sharing.			*/
			| 'H'	{ $$ = SCHEDULE_PS_HOL; }		/* Processor Sharing.			*/
			| 'P'	{ $$ = SCHEDULE_PS_PPR; }		/* Processor Sharing.			*/
    			;

proc_opts		: proc_flags proc_opts
			|
			;

proc_flags		: 'i'		{ srvn_set_proc_multiplicity( curr_proc, srvn_real_constant( srvn_get_infinity() ) ); }
			| 'm' integer  	{ srvn_set_proc_multiplicity( curr_proc, $2 ); }	/* Multi-server (common queue).		*/
			| 'r' integer	{ srvn_set_proc_replicas( curr_proc, $2 ); }		/* Replicated-server.			*/
			| 'R' real	{ srvn_set_proc_rate( curr_proc, $2 ); }
			;

/*+ spex */
proc_obs		: KEY_UTILIZATION VARIABLE			{ spex_processor_observation( curr_proc, KEY_UTILIZATION, 0, $2, NULL ); }
			| KEY_UTILIZATION INTEGER VARIABLE VARIABLE	{ spex_processor_observation( curr_proc, KEY_UTILIZATION, $2, $3, $4 ); }
			|
			;

/*- spex */

/* ----------------------- group information --------------------------- */

/*
* The symbol table so far contains only processor labels.  These labels
 */

group_info		: 'U' ng g_decl_list END_LIST
			|
    			;

ng			: INTEGER 					/*  total number of groups		*/
			;

g_decl_list		: g_decl_list g_decl
    			| g_decl
    			;

/*			   1  2        3           4        5    */
g_decl			: 'g' group_id group_share cap_flag proc_id
			{
			    curr_group = srvn_add_group( $2, $3, $5, $4 );
			    (void) free($2);
			    (void) free($5);
			} group_obs
			;

group_id		: symbol					/*  group identifier			*/
			;


group_share		: real		{ $$=$1;}
			;

cap_flag 		: 'c'		{ $$ = 1; }			/* with cap   */
			| 		{ $$ = 0;}
			;

/*+ spex */
group_obs		: KEY_UTILIZATION VARIABLE			{ spex_group_observation( curr_group, KEY_UTILIZATION, 0, $2, NULL ); }
			| KEY_UTILIZATION INTEGER VARIABLE VARIABLE	{ spex_group_observation( curr_group, KEY_UTILIZATION, $2, $3, $4 ); }
			|
			;

/*- spex */

/* ----------------------- task information --------------------------- */

/*
 * The symbol table so far contains only processor labels.  These labels
 * will need to be examined in the next sections.  Therefore, the symbol
 * table nust be sorted by name.
 */

task_info		: 'T' nt t_decl_list END_LIST
    			;

nt			: INTEGER 		/*  total number of tasks		*/
			;

t_decl_list		: t_decl_list t_decl
    			| t_decl
    			;

/*			   1  2       3               4          5        6        */
t_decl			: 't' task_id task_sched_flag entry_list END_LIST proc_id 
				{
				    curr_task = srvn_add_task( $2, $3, $4, $6 );
				    (void) free($2);
				    (void) free($6);
				}
				task_opts task_obs
			| 'I' task_id task_id integer
			{
			    srvn_store_fanin( $2, $3, $4 );
			    free( $2 ); free( $3 );
			}
			| 'O' task_id task_id integer
			{
			    srvn_store_fanout( $2, $3, $4 );
			    free( $2 ); free( $3 );
			}
			;

task_id			: symbol		/*  task identifier			*/
			;


task_sched_flag		: 'P'	{ $$ = SCHEDULE_POLL; }			/* Polled scheduling at entries.	*/
			| 'S'	{ $$ = SCHEDULE_SEMAPHORE; }		/* Semaphore task.			*/
			| 'b'	{ $$ = SCHEDULE_BURST; }		/* Bursty Reference task		*/
			| 'f'	{ $$ = SCHEDULE_FIFO; }			/* FIFO Scheduling.			*/
			| 'h'	{ $$ = SCHEDULE_HOL; }			/* Head of line.			*/
			| 'i'	{ $$ = SCHEDULE_DELAY; }		/* Infinite Server			*/
			| 'n'	{ $$ = SCHEDULE_FIFO; }			/* NON Reference task, FIFO Scheduling	*/
			| 'p'	{ $$ = SCHEDULE_PPR; }			/* Priority scheduling at entries.	*/
			| 'r'	{ $$ = SCHEDULE_CUSTOMER; }		/* Reference task (customer)		*/
			| 'u'	{ $$ = SCHEDULE_UNIFORM; }		/* Reference task uniform distrib.	*/
			| 'W'	{ $$ = SCHEDULE_RWLOCK; }		/* Reader_Writer Lock task      	*/
			;

entry_list		: entry_id 		{ $$ = srvn_add_entry( $1, NULL ); (void) free( $1 ); }
			| entry_list entry_id 	{ $$ = srvn_add_entry( $2, $1 ); (void) free( $2 ); }
    			;

entry_id		: symbol					/*  entry identifier			*/
			;


task_opts		: task_flags task_opts
			|
			;

task_flags		: integer		{ srvn_set_task_priority( curr_task, $1 ) ; }		/*  task priority (optional)		*/
			| 'T' INTEGER		{ srvn_set_task_tokens( curr_task, $2 ); }
			| 'g' group_id 		{ srvn_set_task_group( curr_task, $2 ); free( $2 ); }	/* Group				*/
			| 'i'			{ srvn_set_task_multiplicity( curr_task, srvn_real_constant( srvn_get_infinity() ) ); }
			| 'm' integer		{ srvn_set_task_multiplicity( curr_task, $2 ); }	/* task multiplicity (optional)		*/
			| 'q' integer		{ srvn_set_task_queue_length( curr_task, $2 ); }	/* Queue length (optional).		*/
			| 'r' integer		{ srvn_set_task_replicas( curr_task, $2 ); }   		/* task replicas			*/
			| 'z' real 		{ srvn_set_task_think_time( curr_task, $2 ); }		/* Think time for a task (optional).	*/
			;

/*+ spex */
task_obs		: task_obs_info task_obs
			|
			;

/*													 obj,       key                        ph  cf, v1, v2 */
task_obs_info		: KEY_THROUGHPUT VARIABLE				{ spex_task_observation( curr_task, KEY_THROUGHPUT,  	       0,  0,  $2, NULL  ); }
			| KEY_THROUGHPUT INTEGER VARIABLE VARIABLE		{ spex_task_observation( curr_task, KEY_THROUGHPUT,  	       0,  $2, $3, $4 ); }			/* conf */
			| KEY_UTILIZATION VARIABLE				{ spex_task_observation( curr_task, KEY_UTILIZATION, 	       $1, 0,  $2, NULL  ); }
			| KEY_UTILIZATION INTEGER VARIABLE VARIABLE		{ spex_task_observation( curr_task, KEY_UTILIZATION, 	       $1, $2, $3, $4 ); }			/* conf */
			| KEY_PROCESSOR_UTILIZATION VARIABLE			{ spex_task_observation( curr_task, KEY_PROCESSOR_UTILIZATION, 0,  0,  $2, NULL  ); }
			| KEY_PROCESSOR_UTILIZATION INTEGER VARIABLE VARIABLE	{ spex_task_observation( curr_task, KEY_PROCESSOR_UTILIZATION, 0,  $2, $3, $4 ); }			/* conf */
			;
/*- spex */

/* ---------------------- entry information --------------------------- */

/*
 * By this point in the parsing process, all of the symbols have been
 * met.  Since no new symbols are forthcoming, the symbol table must be
 * sorted so that searches by name can be done to check are subsequent
 * instances of symbols.
 */

entry_info		: 'E' ne entry_decl_list END_LIST
    			;

ne			:  INTEGER					/*  total number of entries		*/
			;

entry_decl_list		: entry_decl
    			| entry_decl_list entry_decl
			;

entry_decl		: 'a' entry_ref arrival_rate entry_obs
			| 'A' entry_ref symbol				{ srvn_set_start_activity( $2, $3 ); free( $3 ); }
			| 'c' entry_ref coeff_of_variation entry_obs
			| 'f' entry_ref ph_type_flag
			| 'F' entry_ref dest_ref p_forward fwd_obs
			| 'H' entry_ref histogram
			| 'M' entry_ref max_ph_serv_time entry_obs
			| 'p' entry_ref priority
			| 'R' entry_ref					{ srvn_set_rwlock_flag( $2, 'R' ); }
			| 'P' entry_ref					{ srvn_set_semaphore_flag( $2, 'P' ); }
			| 's' entry_ref ph_serv_time entry_obs
			| 'U' entry_ref					{ srvn_set_rwlock_flag( $2, 'U' ); }
			| 'V' entry_ref					{ srvn_set_semaphore_flag( $2, 'V' ); }
			| 'W' entry_ref					{ srvn_set_rwlock_flag( $2, 'W' ); }
			| 'X' entry_ref					{ srvn_set_rwlock_flag( $2, 'X' ); }
			| 'y' entry_ref dest_ref ph_RNV_nb call_obs
			| 'z' entry_ref dest_ref ph_SNR_nb call_obs
			| 'Z' entry_ref ph_think_time entry_obs
			;

entry_ref		: entry_id	{ void * entry = srvn_get_entry( $1 ); curr_entry = entry; $$ = entry; free( $1 ); }
			;

dest_ref		: entry_id	{ void * entry = srvn_get_entry( $1 ); dest_entry = entry; $$ = entry; free( $1 ); }
			;

/*
 * For most records, number of phases is hard wired at 3.  We also cheat
 * and look into the value stack ($<anInt>0).
 */

arrival_rate 		: real					{ srvn_store_open_arrival_rate( $<domObject>0, $1 );  }  		/* arrival rate to entry  	*/
			;

coeff_of_variation  	: real END_LIST				{ srvn_store_coeff_of_variation( $<domObject>0, 1, $1 ); }	/*  serv.time coeff. of variation  	*/
			| real real END_LIST			{ srvn_store_coeff_of_variation( $<domObject>0, 2, $1, $2 ); }
			| real real real END_LIST		{ srvn_store_coeff_of_variation( $<domObject>0, 3, $1, $2, $3 ); }
			;

ph_type_flag  		: INTEGER END_LIST			{ srvn_set_phase_type_flag( $<domObject>0, 1, $1 ); }		/*  phase type: stoch. or determ.  	*/
			| INTEGER INTEGER END_LIST		{ srvn_set_phase_type_flag( $<domObject>0, 2, $1, $2 ); }
			| INTEGER INTEGER INTEGER END_LIST	{ srvn_set_phase_type_flag( $<domObject>0, 3, $1, $2, $3 ); }
			;

max_ph_serv_time 	: constant END_LIST			/*  mean phase service time  		*/
				{
				    srvn_set_histogram( $<domObject>0, 1, $1, $1, 0 );	/* Ph 1 */
				}
			| constant constant END_LIST
				{
				    srvn_set_histogram( $<domObject>0, 1, $1, $1, 0 );	/* Ph 1 */
				    srvn_set_histogram( $<domObject>0, 2, $2, $2, 0 );	/* Ph 2 */
				}
			| constant constant constant END_LIST
				{
				    srvn_set_histogram( $<domObject>0, 1, $1, $1, 0 );	/* Ph 1 */
				    srvn_set_histogram( $<domObject>0, 2, $2, $2, 0 );	/* Ph 2 */
				    srvn_set_histogram( $<domObject>0, 3, $3, $3, 0 );	/* Ph 3 */
				}
			;

histogram		: INTEGER constant ':' constant hist_bins
				{
				    srvn_set_histogram( $<domObject>0, $1, $2, $4, $5 );
				}
			;

priority		: INTEGER				{ srvn_store_entry_priority( $<domObject>0, $1 ); }
			;

ph_serv_time  		: real END_LIST				{ srvn_store_phase_service_time( $<domObject>0, 1, $1 ); }		/*  mean phase service time  		*/
			| real real END_LIST			{ srvn_store_phase_service_time( $<domObject>0, 2, $1, $2 ); }
			| real real real END_LIST		{ srvn_store_phase_service_time( $<domObject>0, 3, $1, $2, $3 ); }
			;

ph_RNV_nb  		: real END_LIST				{ srvn_store_rnv_data( $<domObject>-1, $<domObject>0, 1, $1 ); } 	/*  mean number of RNVs/ phase 		*/
			| real real END_LIST			{ srvn_store_rnv_data( $<domObject>-1, $<domObject>0, 2, $1, $2 ); }
			| real real real END_LIST		{ srvn_store_rnv_data( $<domObject>-1, $<domObject>0, 3, $1, $2, $3 ); }
			;

ph_SNR_nb  		: real END_LIST				{ srvn_store_snr_data( $<domObject>-1, $<domObject>0, 1, $1 ); }	/*  mean nb.of non-blck.sends/ph  	*/
			| real real END_LIST			{ srvn_store_snr_data( $<domObject>-1, $<domObject>0, 2, $1, $2 ); }
			| real real real END_LIST		{ srvn_store_snr_data( $<domObject>-1, $<domObject>0, 3, $1, $2, $3 ); }
			;

ph_think_time  		: real END_LIST				{ srvn_store_phase_think_time( $<domObject>0, 1, $1 ); }		/*  mean phase think time  		*/
			| real real END_LIST			{ srvn_store_phase_think_time( $<domObject>0, 2, $1, $2 ); }
			| real real real END_LIST		{ srvn_store_phase_think_time( $<domObject>0, 3, $1, $2, $3 ); }
			;

p_forward		: real END_LIST				{ srvn_store_prob_forward_data( $<domObject>-1, $<domObject>0, $1 ); }	/* Probability of forwarding.		*/
			;

hist_bins		: INTEGER	{ $$ = $1; }
			|		{ $$ = 20; }		/* Default is 20 bins */
			;


/*+ spex */
entry_obs		: entry_obs_info entry_obs
			|
			;

										/* Note $1 will be the phase as an integer 1, 2, or 3, if present, otherwise 0 */
entry_obs_info		: KEY_THROUGHPUT VARIABLE				{ spex_entry_observation( curr_entry, KEY_THROUGHPUT, $1, 0, $2, NULL ); }
			| KEY_THROUGHPUT INTEGER VARIABLE VARIABLE		{ spex_entry_observation( curr_entry, KEY_THROUGHPUT, $1, $2, $3, $4 ); }
			| KEY_THROUGHPUT_BOUND VARIABLE				{ spex_entry_observation( curr_entry, KEY_THROUGHPUT_BOUND, $1,  0,  $2, NULL  ); }
			| KEY_UTILIZATION VARIABLE				{ spex_entry_observation( curr_entry, KEY_UTILIZATION, $1, 0, $2, NULL ); }
			| KEY_UTILIZATION INTEGER VARIABLE VARIABLE		{ spex_entry_observation( curr_entry, KEY_UTILIZATION, $1, $2, $3, $4 ); }
			| KEY_PROCESSOR_UTILIZATION VARIABLE			{ spex_entry_observation( curr_entry, KEY_PROCESSOR_UTILIZATION, $1, 0, $2, NULL ); }
			| KEY_PROCESSOR_UTILIZATION INTEGER VARIABLE VARIABLE	{ spex_entry_observation( curr_entry, KEY_PROCESSOR_UTILIZATION, $1, $2, $3, $4 ); }
			| KEY_PROCESSOR_WAITING VARIABLE			{ spex_entry_observation( curr_entry, KEY_PROCESSOR_WAITING, $1, 0, $2, NULL ); }
			| KEY_PROCESSOR_WAITING INTEGER VARIABLE VARIABLE	{ spex_entry_observation( curr_entry, KEY_PROCESSOR_WAITING, $1, $2, $3, $4 ); }
			| KEY_SERVICE_TIME VARIABLE				{ spex_entry_observation( curr_entry, KEY_SERVICE_TIME, $1, 0, $2, NULL ); }
			| KEY_SERVICE_TIME INTEGER VARIABLE VARIABLE		{ spex_entry_observation( curr_entry, KEY_SERVICE_TIME, $1, $2, $3, $4 ); }
			| KEY_VARIANCE VARIABLE					{ spex_entry_observation( curr_entry, KEY_VARIANCE, $1, 0, $2, NULL ); }
			| KEY_VARIANCE INTEGER VARIABLE VARIABLE		{ spex_entry_observation( curr_entry, KEY_VARIANCE, $1, $2, $3, $4 ); }
			| KEY_WAITING VARIABLE					{ spex_entry_observation( curr_entry, KEY_WAITING, $1, 0, $2, NULL ); }
			| KEY_WAITING INTEGER VARIABLE VARIABLE			{ spex_entry_observation( curr_entry, KEY_WAITING, $1, $2, $3, $4 ); }
			| KEY_EXCEEDED_TIME VARIABLE				{ spex_entry_observation( curr_entry, KEY_EXCEEDED_TIME, $1, 0, $2, NULL ); }
			;

call_obs		: call_obs_info call_obs
			|
			;

call_obs_info		: KEY_WAITING VARIABLE					{ spex_call_observation( curr_entry, KEY_WAITING, $1, dest_entry, 0, $2, NULL ); }
			| KEY_WAITING INTEGER VARIABLE VARIABLE			{ spex_call_observation( curr_entry, KEY_WAITING, $1, dest_entry, $2, $3, $4 ); }
			| KEY_WAITING_VARIANCE VARIABLE				{ spex_call_observation( curr_entry, KEY_WAITING_VARIANCE, $1, dest_entry, 0, $2, NULL ); }
			| KEY_WAITING_VARIANCE INTEGER VARIABLE VARIABLE	{ spex_call_observation( curr_entry, KEY_WAITING_VARIANCE, $1, dest_entry, $2, $3, $4 ); }
			;

fwd_obs			: fwd_obs_info fwd_obs
			|
			;

fwd_obs_info		: KEY_WAITING VARIABLE					{ spex_fwd_observation( curr_entry, KEY_WAITING, $1, dest_entry, 0, $2, NULL ); }
			| KEY_WAITING INTEGER VARIABLE VARIABLE			{ spex_fwd_observation( curr_entry, KEY_WAITING, $1, dest_entry, $2, $3, $4 ); }
			| KEY_WAITING_VARIANCE VARIABLE				{ spex_fwd_observation( curr_entry, KEY_WAITING_VARIANCE, $1, dest_entry, 0, $2, NULL ); }
			| KEY_WAITING_VARIANCE INTEGER VARIABLE VARIABLE	{ spex_fwd_observation( curr_entry, KEY_WAITING_VARIANCE, $1, dest_entry, $2, $3, $4 ); }
			;
/*- spex */
/* ----------------------- activity information ----------------------- */

activity_info_list	:
			| activity_info_list activity_info
			;

activity_info		: 'A' task_ref activity_defn_list END_LIST
			| 'A' task_ref activity_defn_list ':' activity_conn_list END_LIST
			;

task_ref		: task_id					{ curr_task = srvn_find_task( $1 ); free( $1 ); }		/* Rule exists simply to set global var. */
			;

/* Service time etc. */

activity_defn_list	: activity_defn
			| activity_defn_list activity_defn
			;

activity_defn		: 's' activity_def real activity_obs		{ srvn_store_activity_service_time( $2, $3 ); }
			| 'c' activity_def real activity_obs		{ srvn_store_activity_coeff_of_variation( $2, $3 );  }
			| 'f' activity_def INTEGER			{ srvn_set_activity_phase_type_flag( $2, $3 ); }
			| 'H' activity_def constant ':' constant hist_bins 	{ srvn_set_activity_histogram( $2, $3, $5, $6 ); }
			| 'M' activity_def constant			{ srvn_set_activity_histogram( $2, $3, $3, 0 );  }
			/* srvn_store_rvn_data creates the call, so observe variables have to be afterwards */
			| 'y' activity_def dest_ref real		{ srvn_set_activity_call_name( curr_task, $2, $3, srvn_store_activity_rnv_data( $2, $3, $4 ) ); } act_call_obs
			| 'z' activity_def dest_ref real		{ srvn_set_activity_call_name( curr_task, $2, $3, srvn_store_activity_snr_data( $2, $3, $4 ) ); } act_call_obs
			| 'Z' activity_def real	activity_obs		{ srvn_store_activity_think_time( $2, $3 ); }
			;

/* Node connection */

activity_conn_list	: activity_conn
			| activity_conn ';' activity_conn_list
			;

activity_conn		: join_list					{ srvn_act_connect( curr_task, $1, NULL ); }
			| join_list TRANSITION fork_list		{ srvn_act_connect( curr_task, $1, $3 ); }
			;

join_list		: reply_activity				{ $$ = srvn_act_join_item( curr_task, $1 ); }
			| and_join_list '&' reply_activity quorum_count	{ $$ = srvn_act_and_join_list( curr_task, $3, $1, $4 ); }
			| or_join_list '+' reply_activity 		{ $$ = srvn_act_or_join_list( curr_task, $3, $1 ); }
			;

quorum_count 		:  '(' integer ')' 				{ $$ = $2; }
			|						{ $$ = 0; }
			;

and_join_list		: reply_activity				{ $$ = srvn_act_and_join_list( curr_task, $1, NULL, NULL ); }
			| and_join_list '&' reply_activity 		{ $$ = srvn_act_and_join_list( curr_task, $3, $1, NULL ); }
			;

or_join_list		: reply_activity				{ $$ = srvn_act_or_join_list( curr_task, $1, NULL ); }
			| or_join_list '+' reply_activity 		{ $$ = srvn_act_or_join_list( curr_task, $3, $1 ); }
			;

reply_activity		: activity_ref					{ $$ = $1; }
			| activity_ref '[' act_entry_list ']'		{ srvn_act_add_reply_list( curr_task, $1, $3 ); $$ = $1; }
			;

act_entry_list		: entry_ref					{ $$ = srvn_act_add_reply( curr_task, $1, NULL );  }
			| act_entry_list ',' entry_ref 			{ $$ = srvn_act_add_reply( curr_task, $3, $1 );  }
			;

fork_list		: activity_ref					{ $$ = srvn_act_fork_item( curr_task, $1 ); }
			| act_count activity_ref			{ $$ = srvn_act_loop_list( curr_task, $1, $2, NULL ); }
			| act_count activity_ref ',' loop_list		{ $$ = srvn_act_loop_list( curr_task, $1, $2, $4 ); }
			| and_fork_list '&' activity_ref 		{ $$ = srvn_act_and_fork_list( curr_task, $3, $1 ); }
			| act_prob activity_ref '+' or_fork_list	{ $$ = srvn_act_or_fork_list( curr_task, $1, $2, $4 ); }
			;

and_fork_list		: activity_ref					{ $$ = srvn_act_and_fork_list( curr_task, $1, NULL ); }
			| and_fork_list '&' activity_ref 		{ $$ = srvn_act_and_fork_list( curr_task, $3, $1 ); }
			;

or_fork_list		: act_prob activity_ref				{ $$ = srvn_act_or_fork_list( curr_task, $1, $2, NULL ); }
			| or_fork_list '+' act_prob activity_ref 	{ $$ = srvn_act_or_fork_list( curr_task, $3, $4, $1 ); }
			;

loop_list		: activity_ref					{ $$ = srvn_act_loop_list( curr_task, NULL, $1, NULL ); }	/* Terminator */
			| loop_list ',' act_count activity_ref 		{ $$ = srvn_act_loop_list( curr_task, $3, $4, $1 ); }
			;

act_prob 		: '(' real ')'					{ $$ = $2; }
			;

act_count		: real '*'					{ $$ = $1; }
			;

activity_id		: symbol		/*  activity identifier			*/
			;

activity_def		: activity_id					{ void * activity = srvn_get_activity( curr_task, $1 ); curr_activity = activity; $$ = activity; free( $1 ); }
			;

activity_ref		: activity_id					{ void * activity = srvn_find_activity( curr_task, $1 ); curr_activity = activity; $$ = activity; free( $1 ); }
			;

/*+ spex */
activity_obs		: activity_obs_info activity_obs
			|
			;

activity_obs_info	: KEY_THROUGHPUT VARIABLE				{ spex_activity_observation( curr_task, curr_activity, KEY_THROUGHPUT, 0, $2, NULL ); }
			| KEY_THROUGHPUT INTEGER VARIABLE VARIABLE		{ spex_activity_observation( curr_task, curr_activity, KEY_THROUGHPUT, $2, $3, $4 ); }
			| KEY_UTILIZATION VARIABLE	  			{ spex_activity_observation( curr_task, curr_activity, KEY_UTILIZATION, 0, $2, NULL ); }
			| KEY_UTILIZATION INTEGER VARIABLE VARIABLE		{ spex_activity_observation( curr_task, curr_activity, KEY_UTILIZATION, $2, $3, $4 ); }
			| KEY_PROCESSOR_UTILIZATION VARIABLE			{ spex_activity_observation( curr_task, curr_activity, KEY_PROCESSOR_UTILIZATION, 0, $2, NULL ); }
			| KEY_PROCESSOR_UTILIZATION INTEGER VARIABLE VARIABLE	{ spex_activity_observation( curr_task, curr_activity, KEY_PROCESSOR_UTILIZATION, $2, $3, $4 ); }
			| KEY_PROCESSOR_WAITING VARIABLE    	     		{ spex_activity_observation( curr_task, curr_activity, KEY_PROCESSOR_WAITING, 0, $2, NULL ); }
			| KEY_PROCESSOR_WAITING INTEGER VARIABLE VARIABLE	{ spex_activity_observation( curr_task, curr_activity, KEY_PROCESSOR_WAITING, $2, $3, $4 ); }
			| KEY_SERVICE_TIME VARIABLE		 		{ spex_activity_observation( curr_task, curr_activity, KEY_SERVICE_TIME, 0, $2, NULL ); }
			| KEY_SERVICE_TIME INTEGER VARIABLE VARIABLE		{ spex_activity_observation( curr_task, curr_activity, KEY_SERVICE_TIME, $2, $3, $4 ); }
			| KEY_VARIANCE VARIABLE		    			{ spex_activity_observation( curr_task, curr_activity, KEY_VARIANCE, 0, $2, NULL ); }
			| KEY_VARIANCE INTEGER VARIABLE VARIABLE		{ spex_activity_observation( curr_task, curr_activity, KEY_VARIANCE, $2, $3, $4 ); }
			| KEY_EXCEEDED_TIME VARIABLE		 		{ spex_activity_observation( curr_task, curr_activity, KEY_EXCEEDED_TIME, 0, $2, NULL ); }
			;

act_call_obs		: act_call_obs_info act_call_obs
			|
			;
			
act_call_obs_info	: KEY_WAITING VARIABLE					{ spex_activity_call_observation( curr_task, curr_activity, KEY_WAITING, dest_entry, 0, $2, NULL ); }
			| KEY_WAITING INTEGER VARIABLE VARIABLE			{ spex_activity_call_observation( curr_task, curr_activity, KEY_WAITING, dest_entry, $2, $3, $4 ); }
			| KEY_WAITING_VARIANCE VARIABLE				{ spex_activity_call_observation( curr_task, curr_activity, KEY_WAITING_VARIANCE, dest_entry, 0, $2, NULL ); }
			| KEY_WAITING_VARIANCE INTEGER VARIABLE VARIABLE	{ spex_activity_call_observation( curr_task, curr_activity, KEY_WAITING_VARIANCE, dest_entry, $2, $3, $4 ); }
			;
/*- spex */
/* -------------------------- Report Section -------------------------- */
/*+ spex */
opt_report_info		: 'R' INTEGER r_decl_list  END_LIST	{ $$ = $3; }
			| 'R' INTEGER rvalue '(' expression_list ')' END_LIST	{ $$ = spex_result_function( $3, $5 ); }
			|					{ $$ = NULL; }
			;

r_decl_list		: r_decl				{ $$ = spex_list( NULL, $1 ); }
			| r_decl_list r_decl 			{ $$ = spex_list( $1, $2 ); }
			;

r_decl			: VARIABLE '=' ternary_expr		{ $$ = spex_result_assignment_statement( $1, $3 ); }
			| expression				{ $$ = spex_result_assignment_statement( NULL, $1 ); }
			;
/*- Spex */


/* ------------------------ Convergence Section ----------------------- */
/*+ spex */
opt_convergence_info	: 'C' INTEGER c_decl_list END_LIST	{ $$ = $3; }
			|					{ $$ = NULL; }
			;

c_decl_list		: c_decl				{ $$ = spex_list( NULL, $1 ); }
			| c_decl_list c_decl 			{ $$ = spex_list( $1, $2 ); }
			;

c_decl			: VARIABLE '=' ternary_expr		{ $$ = spex_convergence_assignment_statement( $1, $3 ); }
			;
/*- spex */

/* -------------------------------------------------------------------- */

real			: FLOAT					{ $$ = srvn_real_constant( $1 ); }
			| INTEGER				{ $$ = srvn_int_constant( $1 ); }
			| VARIABLE				{ $$ = srvn_variable( $1 ); }
/*+ spex */
			| '{' expression '}'			{ $$ = spex_inline_expression( $2 ); }	/* Need to assign to variable, then run deferred assignment */
/*- spex */
    			;

constant		: FLOAT					{ $$ = $1; }
			| INTEGER				{ $$ = (double)( $1 ); }
			| CONST_INFINITY			{ $$ = srvn_get_infinity(); }
    			;

integer			: INTEGER				{ $$ = srvn_int_constant( $1 ); }
			| VARIABLE				{ $$ = srvn_variable( $1 ); }
/*+ spex */
			| '{' expression '}'			{ $$ = spex_inline_expression( $2 ); }	/* Need to assign to variable, then run deferred assignment */
/*- spex */
			;

symbol			: SYMBOL				{ $$ = $1; }
    			| INTEGER				{ $$ = make_name( $1 ); }
			;

/*+ spex */
rvalue			: SYMBOL				{ $$ = $1; }
			;
/*- spex */

%%

/*
 * Convert an integer into a malloced string.
 */

static char *
make_name( int i )
{
    char * buf = (char *)malloc( 10 );
    if ( buf ) {
	if ( snprintf( buf, 10, "%d", i ) >= 10 ) {
	}
    }
    return buf;
}
