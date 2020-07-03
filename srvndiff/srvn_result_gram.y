%{
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/************************************************************************/

/*
 * $Id: srvn_result_gram.y 13491 2020-02-12 00:35:17Z greg $
 * ----------------------------------------------------------------------
 *
 * This file has been modified such that it uses result rather than yy on its
 * global variables.  The parser generated should be run through a sed command
 * of the form "sed -e /s/y/xx/g" before compilation.  Same goes for the
 * lexical analyser.
 */
	
#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#if HAVE_FLOAT_H
#include <float.h>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif
#include "srvn_results.h"
#include "srvn_result_gram.h"
 
static unsigned 	i;	/* Index into phase list array */
static unsigned		np;	/* Number of phases in phase list */
static double		*fl;	/* Phase list array (float list) */
static char 		*proc_name = 0;		/* Current processor name */
static char 		*task_name = 0;		/* Current task name */
static char		*group_name = 0;	/* Current group name */
static char		*from_name = 0;
static char 		*to_name = 0;
static char		*entry_name = 0; 
static char		*activity_name = 0;
static int		entry_phase = 0; 

static char * make_name( int i );
static void free_from_and_to_name();
static void free_task_name();
static void free_entry_name();
static void free_activity_name();
static double resultinfinity();
static void resulterror( const char * fmt );

%}

%token 		        VALIDITY_FLAG CONV_FLAG ITERATION_FLAG PROC_COUNT_FLAG PHASE_COUNT_FLAG BOUND_FLAG DROP_PROBABILITY_FLAG SERVICE_EXCEEDED_FLAG DISTRIBUTION_FLAG
%token  		WAITING_FLAG WAITING_VARIANCE_FLAG SNR_WAITING_FLAG SNR_WAITING_VARIANCE_FLAG JOIN_FLAG HOLD_TIME_FLAG RWLOCK_HOLD_TIME_FLAG
%token                  SERVICE_FLAG VARIANCE_FLAG THPT_UT_FLAG OPEN_ARRIV_FLAG PROC_FLAG GROUP_FLAG OVERTAKING_FLAG ENDLIST
%token			REAL_TIME USER_TIME SYST_TIME MAX_RSS SOLVER
%token <anInt> 		INTEGER 
%token <aFloat>		FLOAT TIME 
%token <aString> 	SYMBOL TEXT INFTY COMMENT
%token <aChar>		CHAR 
%token			CONF_INT_FLAG TASK_ENTRY_FLAG

%type  <aString>	identifier proc_identifier group_identifier task_identifier from_identifier to_identifier activity_identifier entry_identifier 

%type  <aFloatList>	float_phase_list
%type  <anInt>		validity_rep iteration_rep proc_count_rep phase_count_rep proc_task_info phase_identifier
%type  <aFloat>		real conv_rep opt_proc_task_total opt_bin_conf

%union {
	int anInt;
	double aFloat;
	char *aString;
	char aChar;
	double *aFloatList;
}

%%	/* Beginning of the rules section */

/*
 * The SRVN program output parser.
 *
 * The srvn program MUST have been run using the -p option  for this
 * parser to work.  See the man page on srvn for more information.
 *
 * The parser need not check for errors in most cases since the srvn output file
 * is unlikely to be in error.  Also, the lexical analyser will not return
 * bad integers, doubles, etc.  However, wherever a disaster could occure,
 * checking is done.
 *
 * The following explains some of the convertions used in this yacc specification.
 * The variables i, j, and k are used as indeces into the srvndata data structures.
 * The variable i is used for the most external data structure while k is used
 * for the most internal.  Due to the structure of the rules involving these
 * variables, the values assigned to i, k and k start at -1 and ascend.  The
 * variable n and q are used to temporarily store values retrieved from the
 * input file.  These values are limits used to limit the number of tasks, entries,
 * etc.  The variable p is used to index the phase lists: list of values by phase.
 * The phase list rules generate the values in the reverse order and so the p variable
 * always counts downwards, in the opposite direction to i, j, and k.  The
 * variable totals is used as a flag indicating to the parser that the data beeing
 * read pertains to a set of totals rather than a specific entry in a table.
 *
 * The resultmalloc() function calls malloc() and checks to see that some memory
 * was indeed returned.  If an error occured, resultmalloc() idicates this through
 * resulterror();
 */
	
SRVN_output_file	: general opt_runtime opt_bound opt_waiting opt_waitvar opt_snr_waiting opt_snr_waitvar opt_drop_prob opt_join opt_service opt_variance opt_service_exceeded opt_distribution opt_hold_time opt_rwlock_hold_time opt_thpt_ut opt_open_arriv opt_proc opt_overtaking
			;

opt_runtime		: runtime
			|
			;

opt_bound		: bound
    			|
    			;

opt_waiting		: waiting
    			|
    			;

opt_waitvar		: waitvar
    			|
    			;

opt_snr_waiting		: snr_waiting
    			|
    			;

opt_snr_waitvar		: snr_waitvar
    			|
    			;

opt_drop_prob 		: drop_prob
			|
			;

opt_join		: join
			|
			;

opt_service		: service
    			|
    			;

opt_variance		: variance
			|
			;

opt_service_exceeded	: service_exceeded
    			|
    			;

opt_distribution	: opt_distribution distribution
			|
			;

opt_hold_time		: hold_time
			|
			;

opt_rwlock_hold_time	: rwlock_hold_time
			|
			;

opt_thpt_ut		: thpt_ut
			|
			;

opt_open_arriv		: open_arriv
    			|
    			;

opt_proc		: proc
    			|
    			;

opt_overtaking		: overtaking
			|
			;

/*
 * General information section.
 *
 * These rules retrieve inportant information for use by the end user
 * and by the actions of the rules to follow.
 */

general			: validity_rep conv_rep iteration_rep proc_count_rep phase_count_rep
				{ set_general( $1, $2, $3, $4, $5 ); }
			;

validity_rep		: VALIDITY_FLAG CHAR
				{
					if ( ( $2 == 'y' ) || ( $2 == 'Y' ) ) {
						$$ = 1;
					} else if ( ( $2 == 'n' ) || ( $2 == 'N' ) ) {
						$$ = 0;
					} else {
						results_error("Bad validity indicator character: %c\n", $2);
						YYERROR;
					}
				}
			;

conv_rep		: CONV_FLAG real
				{ $$ = $2; }
			;

iteration_rep		: ITERATION_FLAG INTEGER
				{ $$ = $2; }
			;

proc_count_rep		: PROC_COUNT_FLAG INTEGER
				{
					$$ = $2;
					if ( $2 <= 0 ) {
						results_error("Number of processors, %d, should be greater than zero", $2);
						YYERROR;
					}
				}
			;

phase_count_rep		: PHASE_COUNT_FLAG INTEGER
				{
					$$ = $2;
					if ( $2 <= 0 ) {
						results_error("Number of phases, %d, should be greater than zero", $2);
						YYERROR;
					}
				}
			;

/*
 * Runtime information.
 */

runtime			: runtime_tbl_entry 
			| runtime_tbl_entry runtime
			;

runtime_tbl_entry	: REAL_TIME TIME
			    { add_elapsed_time( $2 ); }
			| USER_TIME TIME
			    { add_user_time( $2 ); }
			| SYST_TIME TIME
			    { add_system_time( $2 ); }
			| COMMENT
			    { add_comment( $1 ); }
			| SOLVER INTEGER INTEGER real real real real INTEGER
			    { add_mva_solver_info( $2, $3, $4, $5, $6, $7, $8 ); }
			| MAX_RSS INTEGER
			| SOLVER INTEGER INTEGER real real real real  real real real  TIME TIME TIME
			;

/*
 * Throughput bounds.
 *
 * These rules read the output file's throughput bounds table.
 * The data is inserted into the "bound" data structure.
 */

bound			: BOUND_FLAG INTEGER bound_tbl
			;

bound_tbl		: bound_tbl_entry ENDLIST
			| bound_tbl_entry bound_tbl
			;

bound_tbl_entry		: identifier real
			    { add_bound( $1, 0.0, $2 ); free( $1 ); }
			| identifier TASK_ENTRY_FLAG identifier real
			    { add_bound( $3, 0.0, $4 ); free( $1 ); free( $3 ); }
/* 			| identifier real real */
/* 			    { add_bound( $1, $2, $3 ); free( $1 ); } */
/* 			| identifier TASK_ENTRY_FLAG identifier real real */
/* 			    { add_bound( $3, $4, $5 ); free( $1 ); free( $3 ); } */
			;

/*
 * Waiting times.
 *
 * These rules read the waiting times table from the srvn output.
 * The values are stored in the "waiting" data structure.
 */

waiting			: WAITING_FLAG INTEGER waiting_fmt
			;

waiting_fmt		: waiting_t_tbl		/* New file format. */
			| waiting_e_tbl		/* Old file format. */
			| ENDLIST		/* No entries.      */
			;

waiting_t_tbl		: waiting_t_entry ENDLIST
			| waiting_t_entry waiting_t_tbl
			;

waiting_t_entry		: task_identifier TASK_ENTRY_FLAG waiting_t_choice
			    {
				free( task_name );
				task_name = 0;
			    }
			;

waiting_t_choice	: ENDLIST TASK_ENTRY_FLAG waiting_a_tbl
			| waiting_e_tbl TASK_ENTRY_FLAG waiting_a_tbl
			| waiting_e_tbl
			;

waiting_e_tbl		: waiting_e_entry waiting_e_tbl
			| waiting_e_entry ENDLIST
			;

waiting_e_entry		: from_identifier to_identifier float_phase_list waiting_e_conf_tbl
			    {
				add_waiting( $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

waiting_e_conf_tbl	: waiting_e_conf_entry waiting_e_conf_tbl
    			|
    			;

waiting_e_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_waiting_confidence( from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

waiting_a_tbl		: waiting_a_entry ENDLIST
			| waiting_a_entry waiting_a_tbl
			;


waiting_a_entry		: from_identifier to_identifier float_phase_list waiting_a_conf_tbl
			    {
				add_act_waiting( task_name, $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

waiting_a_conf_tbl	: waiting_a_conf_entry waiting_a_conf_tbl
    			|
    			;

waiting_a_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_waiting_confidence( task_name, from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

/*
 * Variance
 *
 * These rules read the variance from the table in the srvn output file.
 * The values are stored in the "variance" data structure.
 */

waitvar			: WAITING_VARIANCE_FLAG INTEGER waitvar_fmt
			;

waitvar_fmt		: waitvar_t_tbl		/* new file format */
			| waitvar_e_tbl		/* old file format */
			| ENDLIST		/* no data.        */
			;

waitvar_t_tbl		: waitvar_t_entry ENDLIST
			| waitvar_t_entry waitvar_t_tbl
			;

waitvar_t_entry		: task_identifier TASK_ENTRY_FLAG waitvar_t_choice
			    {
				free_task_name();
			    }
			;

waitvar_t_choice	: ENDLIST TASK_ENTRY_FLAG waitvar_a_tbl
			| waitvar_e_tbl TASK_ENTRY_FLAG waitvar_a_tbl
			| waitvar_e_tbl
			;

waitvar_e_tbl		: waitvar_e_entry ENDLIST
			| waitvar_e_entry waitvar_e_tbl
			;

waitvar_e_entry		: from_identifier to_identifier float_phase_list waitvar_e_conf_tbl
			    {
				add_wait_variance( $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

waitvar_e_conf_tbl	: waitvar_e_conf_entry waitvar_e_conf_tbl
    			|
    			;

waitvar_e_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_wait_variance_confidence( from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

waitvar_a_tbl		: waitvar_a_entry ENDLIST
			| waitvar_a_entry waitvar_a_tbl
			;

waitvar_a_entry		: from_identifier to_identifier float_phase_list waitvar_a_conf_tbl
			    {
				add_act_wait_variance( task_name, $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

waitvar_a_conf_tbl	: waitvar_a_conf_entry waitvar_a_conf_tbl
    			|
    			;

waitvar_a_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_wait_variance_confidence( task_name, from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

/*
 * Snr_waiting times.
 *
 * These rules read the waiting times table from the srvn output.
 * The values are stored in the "waiting" data structure.
 */

snr_waiting		: SNR_WAITING_FLAG INTEGER snr_waiting_fmt
			;

snr_waiting_fmt		: snr_waiting_t_tbl	/* New file format. */
			| snr_waiting_e_tbl	/* Old file format. */
			| ENDLIST		/* No entries.      */
			;

snr_waiting_t_tbl	: snr_waiting_t_entry ENDLIST
			| snr_waiting_t_entry snr_waiting_t_tbl
			;

snr_waiting_t_entry	: task_identifier TASK_ENTRY_FLAG snr_waiting_t_choice
			    {
				free_task_name();
			    }
			;

snr_waiting_t_choice	: ENDLIST TASK_ENTRY_FLAG snr_waiting_a_tbl
			| snr_waiting_e_tbl TASK_ENTRY_FLAG snr_waiting_a_tbl
			| snr_waiting_e_tbl
			;

snr_waiting_e_tbl	: snr_waiting_e_entry snr_waiting_e_tbl
			| snr_waiting_e_entry ENDLIST
			;

snr_waiting_e_entry	: from_identifier to_identifier float_phase_list snr_waiting_e_conf_tbl
			    {
				add_snr_waiting( $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

snr_waiting_e_conf_tbl	: snr_waiting_e_conf_entry snr_waiting_e_conf_tbl
    			|
    			;

snr_waiting_e_conf_entry : CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_snr_waiting_confidence( from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

snr_waiting_a_tbl	: snr_waiting_a_entry ENDLIST
			| snr_waiting_a_entry snr_waiting_a_tbl
			;

snr_waiting_a_entry	: from_identifier to_identifier float_phase_list snr_waiting_a_conf_tbl
			    {
				add_act_snr_waiting( task_name, $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

snr_waiting_a_conf_tbl	: snr_waiting_a_conf_entry snr_waiting_a_conf_tbl
    			|
    			;

snr_waiting_a_conf_entry : CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_snr_waiting_confidence( task_name, from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

/*
 * Variance
 *
 * These rules read the variance from the table in the srvn output file.
 * The values are stored in the "variance" data structure.
 */

snr_waitvar		: SNR_WAITING_VARIANCE_FLAG INTEGER snr_waitvar_fmt
			;

snr_waitvar_fmt		: snr_waitvar_t_tbl		/* New file format */
			| snr_waitvar_e_tbl		/* Old file format */
			| ENDLIST		/* No data.        */
			;

snr_waitvar_t_tbl	: snr_waitvar_t_entry ENDLIST
			| snr_waitvar_t_entry snr_waitvar_t_tbl
			;

snr_waitvar_t_entry	: task_identifier TASK_ENTRY_FLAG snr_waitvar_t_choice
			    {
				free_task_name();
			    }
			;

snr_waitvar_t_choice	: ENDLIST TASK_ENTRY_FLAG snr_waitvar_a_tbl
			| snr_waitvar_e_tbl TASK_ENTRY_FLAG snr_waitvar_a_tbl
			| snr_waitvar_e_tbl
			;

snr_waitvar_e_tbl	: snr_waitvar_e_entry ENDLIST
			| snr_waitvar_e_entry snr_waitvar_e_tbl
			;

snr_waitvar_e_entry	: from_identifier to_identifier float_phase_list snr_waitvar_e_conf_tbl
			    {
				add_snr_wait_variance( $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

snr_waitvar_e_conf_tbl	: snr_waitvar_e_conf_entry snr_waitvar_e_conf_tbl
    			|
    			;

snr_waitvar_e_conf_entry : CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_snr_wait_variance_confidence( from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

snr_waitvar_a_tbl	: snr_waitvar_a_entry ENDLIST
			| snr_waitvar_a_entry snr_waitvar_a_tbl
			;

snr_waitvar_a_entry	: from_identifier to_identifier float_phase_list snr_waitvar_a_conf_tbl
			    {
				add_act_snr_wait_variance( task_name, $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

snr_waitvar_a_conf_tbl	: snr_waitvar_a_conf_entry snr_waitvar_a_conf_tbl
    			|
    			;

snr_waitvar_a_conf_entry : CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_snr_wait_variance_confidence( task_name, from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

/*
 * Drop_probability times.
 *
 * These rules read the waiting times table from the srvn output.
 * The values are stored in the "waiting" data structure.
 */

drop_prob		: DROP_PROBABILITY_FLAG INTEGER drop_prob_fmt
			;

drop_prob_fmt		: drop_prob_t_tbl	/* New file format. */
			| drop_prob_e_tbl	/* Old file format */
			| ENDLIST		/* No entries.      */
			;

drop_prob_t_tbl		: drop_prob_t_entry ENDLIST
			| drop_prob_t_entry drop_prob_t_tbl
			;

drop_prob_t_entry 	: task_identifier TASK_ENTRY_FLAG drop_prob_t_choice
			    {
				free_task_name();
			    }
			;

drop_prob_t_choice 	: ENDLIST TASK_ENTRY_FLAG drop_prob_a_tbl
			| drop_prob_e_tbl TASK_ENTRY_FLAG drop_prob_a_tbl
			| drop_prob_e_tbl
			;

drop_prob_e_tbl		: drop_prob_e_entry drop_prob_e_tbl
			| drop_prob_e_entry ENDLIST
			;

drop_prob_e_entry 	: from_identifier to_identifier float_phase_list drop_prob_e_conf_tbl
			    {
				add_drop_probability( $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;

drop_prob_e_conf_tbl 	: drop_prob_e_conf_entry drop_prob_e_conf_tbl
    			|
    			;

drop_prob_e_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_drop_probability_confidence( from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

drop_prob_a_tbl		: drop_prob_a_entry ENDLIST
			| drop_prob_a_entry drop_prob_a_tbl
			;

drop_prob_a_entry 	: from_identifier to_identifier float_phase_list drop_prob_a_conf_tbl
			    {
				add_act_drop_probability( task_name, $1, $2, $3 );
				free_from_and_to_name();
				free( $3 );
			    }
			;
drop_prob_a_conf_tbl 	: drop_prob_a_conf_entry drop_prob_a_conf_tbl
    			|
    			;

drop_prob_a_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_drop_probability_confidence( task_name, from_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;


/*
 * Join times.
 *
 * These rules read the join times table from the srvn output.
 * The values are stored in the "join" data structure.
 */

join			: JOIN_FLAG INTEGER join_fmt
			;

join_fmt		: join_t_tbl		/* New file format. */
			| ENDLIST		/* No entries.      */
			;

join_t_tbl		: join_t_entry ENDLIST
			| join_t_entry join_t_tbl
			;

join_t_entry		: task_identifier TASK_ENTRY_FLAG join_e_tbl
			    {
				free_task_name();
			    }
			;

join_e_tbl		: join_e_entry join_e_tbl
			| join_e_entry ENDLIST
			;

join_e_entry		: from_identifier to_identifier real real join_conf_tbl
			    {
				add_join( task_name, $1, $2, $3, $4 );
				free_from_and_to_name();
			    }
			;

join_conf_tbl		: join_conf_entry join_conf_tbl
    			|
    			;

join_conf_entry 	: CONF_INT_FLAG INTEGER real real
			    {
				add_join_confidence( task_name, from_name, to_name, $2, $3, $4 );
			    }
    			;

/*
 * Service times.
 *
 * These rules read the service times from the table in the srvn output file.
 * The values are stored in the "service" data structure.
 */

service			: SERVICE_FLAG INTEGER service_fmt
			;

service_fmt		: service_t_tbl		/* New file format */
			| service_e_tbl		/* Old file format */
			;

service_t_tbl		: service_t_entry ENDLIST
			| service_t_entry service_t_tbl
			;

service_t_entry		: task_identifier TASK_ENTRY_FLAG service_t_choice
			    {
				free_task_name();
			    }
			;

service_t_choice	: service_e_tbl TASK_ENTRY_FLAG service_a_tbl
			| service_e_tbl 
			;

service_e_tbl		: service_e_entry ENDLIST
			| service_e_entry service_e_tbl
			;

service_e_entry		: to_identifier float_phase_list service_e_conf_tbl
			    {
				add_service( $1, $2 );
				free( to_name );
				to_name = 0;
				free( $2 );
			    }
			;

service_e_conf_tbl	: service_e_conf_entry service_e_conf_tbl
    			|
    			;

service_e_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_service_confidence( to_name, $2, $3 );
				free( $3 );
			    }
    			;

service_a_tbl		: service_a_entry ENDLIST
			| service_a_entry service_a_tbl
			;

service_a_entry		: to_identifier float_phase_list service_a_conf_tbl
			    {
				add_act_service( task_name, $1, $2 );
				free( to_name );
				to_name = 0;
				free( $2 );
			    }
			;

service_a_conf_tbl	: service_a_conf_entry service_a_conf_tbl
    			|
    			;

service_a_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_service_confidence( task_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

/*
 * Variance
 *
 * These rules read the variance from the table in the srvn output file.
 * The values are stored in the "variance" data structure.
 */

variance		: VARIANCE_FLAG INTEGER variance_fmt
			;

variance_fmt		: variance_t_tbl		/* New file format */
			| variance_e_tbl		/* Old file format */
			;

variance_t_tbl		: variance_t_entry ENDLIST
			| variance_t_entry variance_t_tbl
			;

variance_t_entry	: task_identifier TASK_ENTRY_FLAG variance_t_choice
			    {
				free_task_name();
			    }
			;

variance_t_choice	: variance_e_tbl TASK_ENTRY_FLAG variance_a_tbl
			| variance_e_tbl 
			;

variance_e_tbl		: variance_e_entry ENDLIST
			| variance_e_entry variance_e_tbl
			;

variance_e_entry	: to_identifier float_phase_list variance_e_conf_tbl
			    {
				add_variance( $1, $2 );
				free( to_name );
				to_name = 0;
				free( $2 );
			    }
			;

variance_e_conf_tbl	: variance_e_conf_entry variance_e_conf_tbl
    			|
    			;

variance_e_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_variance_confidence( to_name, $2, $3 );
				free( $3 );
			    }
    			;

variance_a_tbl		: variance_a_entry ENDLIST
			| variance_a_entry variance_a_tbl
			;

variance_a_entry	: to_identifier float_phase_list variance_a_conf_tbl
			    {
				add_act_variance( task_name, $1, $2 );
				free( to_name );
				to_name = 0;
				free( $2 );
			    }
			;

variance_a_conf_tbl	: variance_a_conf_entry variance_a_conf_tbl
    			|
    			;

variance_a_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_variance_confidence( task_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

/*
 * Service_exceeded
 *
 * These rules read the service_exceeded from the table in the srvn output file.
 * The values are stored in the "service_exceeded" data structure.
 */

service_exceeded	: SERVICE_EXCEEDED_FLAG INTEGER service_exceeded_fmt
			;

service_exceeded_fmt	: service_exceeded_t_tbl		/* New file format */
			| service_exceeded_e_tbl		/* Old file format */
			;

service_exceeded_t_tbl	: service_exceeded_t_entry ENDLIST
			| service_exceeded_t_entry service_exceeded_t_tbl
			;

service_exceeded_t_entry : task_identifier TASK_ENTRY_FLAG service_exceeded_t_choice
			    {
				free_task_name();
			    }
			;

service_exceeded_t_choice : service_exceeded_e_tbl TASK_ENTRY_FLAG service_exceeded_a_tbl
			| service_exceeded_e_tbl 
			;

service_exceeded_e_tbl	: service_exceeded_e_entry ENDLIST
			| service_exceeded_e_entry service_exceeded_e_tbl
			;

service_exceeded_e_entry : to_identifier float_phase_list service_exceeded_e_conf_tbl
			    {
				add_service_exceeded( $1, $2 );
				free( to_name );
				to_name = 0;
				free( $2 );
			    }
			;

service_exceeded_e_conf_tbl : service_exceeded_e_conf_entry service_exceeded_e_conf_tbl
    			|
    			;

service_exceeded_e_conf_entry : CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_service_exceeded_confidence( to_name, $2, $3 );
				free( $3 );
			    }
    			;

service_exceeded_a_tbl	: service_exceeded_a_entry ENDLIST
			| service_exceeded_a_entry service_exceeded_a_tbl
			;

service_exceeded_a_entry : to_identifier float_phase_list service_exceeded_a_conf_tbl
			    {
				add_act_service_exceeded( task_name, $1, $2 );
				free( to_name );
				to_name = 0;
				free( $2 );
			    }
			;

service_exceeded_a_conf_tbl : service_exceeded_a_conf_entry service_exceeded_a_conf_tbl
    			|
    			;

service_exceeded_a_conf_entry 	: CONF_INT_FLAG INTEGER float_phase_list
			    {
				add_act_service_exceeded_confidence( task_name, to_name, $2, $3 );
				free( $3 );
			    }
    			;

/*
 * Service time distributions.
 */

distribution		: DISTRIBUTION_FLAG entry_identifier phase_identifier real real real real opt_bin_list 
				{
				    add_histogram_statistics( entry_name, $3, $4, $5, $6, $7 );		/* Entry/phase */
				    free_entry_name();
				    entry_phase = 0;
				}
			| DISTRIBUTION_FLAG task_identifier activity_identifier real real real real opt_act_bin_list 
				{
				    add_act_histogram_statistics( task_name, activity_name, $4, $5, $6, $7 );	/* Task/activity */
				    free_task_name();
				    free_activity_name();
				}
			;

opt_bin_list		: bin_list opt_bin_list 
			| ENDLIST
			;

bin_list		: real real real opt_bin_conf opt_bin_conf
				{
				    add_histogram_bin( entry_name, entry_phase, $1, $2, $3, $4, $5 );		/* Entry/phase */
				}
			;

opt_act_bin_list	: act_bin_list opt_act_bin_list 
			| ENDLIST
			;

act_bin_list		: real real real opt_bin_conf opt_bin_conf
				{
				    add_act_histogram_bin( task_name, activity_name, $1, $2, $3, $4, $5 );		/* Entry/phase */
				}
			;

opt_bin_conf		: CONF_INT_FLAG INTEGER real
				{
				    $$= $3;
				}
			|
				{
				    $$ = 0.0;
				}
			;


/*
 * Hold times.
 *
 * These rules read the hold_time table from the srvn output.
 * The values are stored in the "hold_time" data structure.
 */

hold_time		: HOLD_TIME_FLAG INTEGER hold_time_t_tbl 
			;

hold_time_t_tbl		: hold_time_t_entry ENDLIST
			| hold_time_t_entry hold_time_t_tbl
			;

hold_time_t_entry	: task_identifier TASK_ENTRY_FLAG from_identifier to_identifier real real real hold_time_t_conf_tbl
			    {
				add_holding_time( $1, $3, $4, $5, $6, $7 );
				free_task_name();
				free_from_and_to_name();
			    }
			;

hold_time_t_conf_tbl	: hold_time_t_conf_entry hold_time_t_conf_tbl
			|
			;

hold_time_t_conf_entry 	: CONF_INT_FLAG INTEGER real real real 
			    {
				add_holding_time_confidence( task_name, from_name, to_name, $2, $3, $4, $5 );
			    }
			;
			

/*
 * RWLock_Hold times.
 *
 * These rules read the hold_time table from the srvn output.
 * The values are stored in the "rwlock_hold_time" data structure.
 */

rwlock_hold_time	: RWLOCK_HOLD_TIME_FLAG INTEGER rwlock_hold_time_t_tbl 
			;

rwlock_hold_time_t_tbl	: rwlock_hold_time_t_entry ENDLIST
			| rwlock_hold_time_t_entry  rwlock_hold_time_t_tbl
			;

rwlock_hold_time_t_entry : reader_hold_time_t_entry writer_hold_time_t_entry
			;

reader_hold_time_t_entry: task_identifier TASK_ENTRY_FLAG from_identifier to_identifier real real real real real reader_hold_time_t_conf_tbl
			    {
//				add_reader_holding_time( $1, $3, $4, $5, $6, $7, $8, $9);
			    }
			;

writer_hold_time_t_entry: from_identifier to_identifier real real real real real writer_hold_time_t_conf_tbl
			    {
//				add_writer_holding_time( task_name, $2, $3, $4, $5, $6, $7, $8 );
				free_task_name();
				free_from_and_to_name();
			    }
			;

reader_hold_time_t_conf_tbl	: reader_hold_time_t_conf_entry reader_hold_time_t_conf_tbl
			|
			;

reader_hold_time_t_conf_entry 	: CONF_INT_FLAG INTEGER real real real real real
			    {
//				add_reader_holding_time_confidence( task_name, from_name, to_name, $2, $3, $4, $5, $6, $7 );
			    }
			;

writer_hold_time_t_conf_tbl	: writer_hold_time_t_conf_entry writer_hold_time_t_conf_tbl
			|
			;

writer_hold_time_t_conf_entry 	: CONF_INT_FLAG INTEGER real real real real real
			    {
//				add_writer_holding_time_confidence( task_name, from_name, to_name, $2, $3, $4, $5, $6, $7 );
			    }
			;

/*
 * Throughputs and utilizations.
 *
 * This section deals with the throughputs and utilizations.  The storage for
 * these data values is provided by the thpt_ut data structure.
 */

thpt_ut			: THPT_UT_FLAG INTEGER thpt_ut_t_tbl ENDLIST
			;

thpt_ut_t_tbl		: thpt_ut_t_tbl thpt_ut_t_entry 
			| thpt_ut_t_entry 
			;

thpt_ut_t_entry		: task_identifier TASK_ENTRY_FLAG thpt_ut_e_tbl opt_thpt_ut_a_tbl opt_thpt_total 
			    {
				add_thpt_ut( $1 );
				free_task_name();
			    }
			;

opt_thpt_ut_a_tbl 	: TASK_ENTRY_FLAG thpt_ut_a_tbl 
    			|
			;

opt_thpt_total		: real float_phase_list real thpt_ut_t_conf_tbl	/* Totals for task.		*/
			    {
				total_thpt_ut( task_name, $1, $2, $3 );
				free( $2 );
			    }
    			|
			;

thpt_ut_t_conf_tbl	: thpt_ut_t_conf_entry thpt_ut_t_conf_tbl
    			|
    			;

thpt_ut_t_conf_entry 	: CONF_INT_FLAG INTEGER real float_phase_list real
			    {
				total_thpt_ut_confidence( task_name, $2, $3, $4, $5 );
				free( $4 );
			    }
    			;

thpt_ut_e_tbl		: thpt_ut_e_entry ENDLIST
			| thpt_ut_e_entry thpt_ut_e_tbl
			;

thpt_ut_e_entry		: to_identifier real float_phase_list real thpt_ut_e_conf_tbl
			    {
				add_entry_thpt_ut( task_name, $1, $2, $3, $4 );
				free( to_name );
				to_name = 0;
				free( $3 );
			    }
			;

thpt_ut_e_conf_tbl	: thpt_ut_e_conf_entry thpt_ut_e_conf_tbl
    			|
    			;

thpt_ut_e_conf_entry 	: CONF_INT_FLAG INTEGER real float_phase_list real
			    {
				add_entry_thpt_ut_confidence( to_name, $2, $3, $4, $5 );
				free( $4 );
			    }
    			;

thpt_ut_a_tbl		: thpt_ut_a_entry ENDLIST
			| thpt_ut_a_entry thpt_ut_a_tbl
			;

thpt_ut_a_entry		: to_identifier real float_phase_list thpt_ut_act_conf_tbl
			    {
				add_act_thpt_ut( task_name, to_name, $2, $3 );
				free( to_name );
				to_name = 0;
				free( $3 );
			    }
			;

thpt_ut_act_conf_tbl	: thpt_ut_act_conf_entry thpt_ut_act_conf_tbl
    			|
    			;

thpt_ut_act_conf_entry 	: CONF_INT_FLAG INTEGER real float_phase_list
			    {
				add_act_thpt_ut_confidence( task_name, to_name, $2, $3, $4 );
				free( $4 );
			    }
    			;

/*
 * Arrival rates and waiting times.
 *
 * These rules fill in the "open_arriv" data structure using the values
 * obtained from the srvn output file.
 */

open_arriv		: OPEN_ARRIV_FLAG INTEGER open_arriv_tbl
			;

open_arriv_tbl		: open_arriv_entry ENDLIST
			| open_arriv_entry open_arriv_tbl
			;

open_arriv_entry	: task_identifier to_identifier real real open_arriv_conf_tbl
			    {
				add_open_arriv( $1, $2, $3, $4 );
				free_task_name();
				free( to_name );
				to_name = 0;
			    }
			| task_identifier TASK_ENTRY_FLAG to_identifier real real open_arriv_conf_tbl
			    {
				add_open_arriv( $1, $3, $4, $5 );
				free_task_name();
				free( to_name );
				to_name = 0;
			    }
			;

open_arriv_conf_tbl	: open_arriv_conf_entry open_arriv_conf_tbl
			|
			;

open_arriv_conf_entry	: CONF_INT_FLAG INTEGER real
			    {
				add_open_arriv_confidence( task_name, to_name, $2, $3 );
			    }
			;

/*
 * Processor waiting and utilization times.
 *
 * These rules extract the processor information from the srvn output file.
 * The data is deposited in the "proc" data structure.
 */

proc			: processor proc
    			| ENDLIST
			;

processor		: PROC_FLAG proc_identifier INTEGER proc_tbl opt_processor_total 
			    {
				add_proc( proc_name );
				free( proc_name );
				proc_name = 0;
			    }
			;

opt_processor_total 	: real processor_conf_tbl ENDLIST
			    {
				add_total_proc( proc_name, $1 );
			    }
			|
			;

proc_tbl		: proc_tbl_entry proc_tbl
    			| ENDLIST 
			;

proc_tbl_entry		: task_identifier proc_task_info proc_task opt_proc_task_total opt_group_util
			    {
				add_task_proc( proc_name, task_name, $2, $4 );
				free_task_name();
			    }
			;

proc_task		: proc_entry_tbl
			| proc_entry_tbl TASK_ENTRY_FLAG proc_activity_tbl 
			;

proc_task_info		: INTEGER INTEGER INTEGER			/* n_entries, priority, {multiplicity} */
			    {
				$$ = $3;
			    }
			| INTEGER INTEGER 
			    {
				$$ = 1;
			    }
			;

proc_entry_tbl		: proc_entry ENDLIST
			| proc_entry proc_entry_tbl
			;

proc_entry		: to_identifier real float_phase_list proc_entry_conf_tbl
			    {
				add_entry_proc( $1, $2, $3 );
				free( to_name );
				to_name = 0;
				free( $3 );
			    }
			;

proc_entry_conf_tbl	: proc_entry_conf proc_entry_conf_tbl
    			|
    			;

proc_entry_conf    	: CONF_INT_FLAG INTEGER real float_phase_list
			    {
				add_entry_proc_confidence( to_name, $2, $3, $4 );
				free( $4 );
			    }
    			;

proc_activity_tbl	: proc_activity ENDLIST
			| proc_activity proc_activity_tbl
			;

proc_activity		: to_identifier real float_phase_list proc_activity_conf_tbl
			    {
				add_act_proc( task_name, to_name, $2, $3 );
				free( to_name );
				to_name = 0;
				free( $3 );
			    }

			;

proc_activity_conf_tbl	: proc_activity_conf proc_activity_conf_tbl
    			|
    			;

proc_activity_conf    : CONF_INT_FLAG INTEGER real float_phase_list
			    {
				add_act_proc_confidence( task_name, to_name, $2, $3, $4 );
				free( $4 );
			    }
    			;

proc_task_conf_tbl	: proc_task_conf_entry proc_task_conf_tbl
    			|
    			;

proc_task_conf_entry	: CONF_INT_FLAG INTEGER real 
			    {
				add_task_proc_confidence( task_name, $2, $3 );
			    }
    			;

opt_proc_task_total 	: real proc_task_conf_tbl
			    {
				$$ = $1;
			    }
			|
			    {
				$$ = 0;
			    }
			;

opt_group_util		: GROUP_FLAG group_identifier real opt_group_util_conf
			    {
				add_group_util( group_name, $3 );
			    }
			|
			;


opt_group_util_conf	: group_util_conf_entry opt_group_util_conf
			|
			;

group_util_conf_entry	: CONF_INT_FLAG INTEGER real
			    {
				add_group_util_conf( group_name, $2, $3 );
			    }
			;

processor_conf_tbl	: processor_conf_entry processor_conf_tbl
    			|
    			;

processor_conf_entry	: CONF_INT_FLAG INTEGER real 
			    {
				add_total_proc_confidence( proc_name, $2, $3 );
			    }
    			;

/*
 * The following rules are for overtaking.
 */

overtaking 		: OVERTAKING_FLAG overtaking_e_tbl
			;

overtaking_e_tbl	: overtaking_entry ENDLIST
			| overtaking_entry overtaking_e_tbl
			;

overtaking_entry	: from_identifier to_identifier identifier identifier INTEGER float_phase_list
			    {
				add_overtaking( $1, $2, $3, $4, $5, $6 );
				free_from_and_to_name();
				free( $3 );
				free( $4 );
				free( $6 );
			    }
			;

/*
 * The following rule is used to generate strings for all
 * the SYMBOLS read in.
 */

identifier		: SYMBOL
				{ $$ = strdup( $1 ); }
			| INTEGER
				{ $$ = make_name( $1 ); }
			;

proc_identifier		: identifier
				{ proc_name = $1; $$ = $1; } 
			;

task_identifier		: identifier
				{ task_name = $1; $$ = $1; } 
			;

group_identifier	: identifier
				{ group_name = $1; $$ = $1; } 
			;

from_identifier		: identifier
				{ from_name = $1; $$ = $1; } 
			;

to_identifier		: identifier
				{ to_name = $1; $$ = $1; } 
			;

activity_identifier	: identifier
				{ activity_name = $1; $$ = $1; } 
			;

entry_identifier	: identifier
				{ entry_name = $1; $$ = $1; } 
			;

phase_identifier	: INTEGER
				{ entry_phase = $1; $$ = $1; } 
			;

/*
 * All the phase list that are comprised of floating point values
 * are handled by these rules.
 */

float_phase_list	: 	{ np = 0; }
			  float_list
				{ $$ = fl; }
			;

float_list		: real ENDLIST
				{ 	/* We're on the last list element */
					i = np;
					np += 1;
					fl = (double *)malloc( (size_t)(np * sizeof( double )) );
					fl[i] = $1;
				}
			| real
			  	{ np += 1; }
			  float_list
				{	/* We're on all but the last list element. */
					i -= 1;
					fl[i] = $1;
				}
			| error ENDLIST
				{ 	/* OOPS -- NaN or Inf or whatever... Barf. */
					i = np;
					np += 1;
					fl = (double *)malloc( (size_t)(np * sizeof( double )) );
					fl[i] = 0.0;
				}
			;

/*----------------------------------------------------------------------*/

real			: FLOAT
    			| INTEGER
				{ $$ = (double)$1; }
			| INFTY
				{ $$ = resultinfinity(); }
    			;


%%	/* Beginning of the code section */

/*
 * Convert an integer into a malloced string.
 */
    
static char *
make_name( int i )
{
	char * buf = (char *)malloc( 10 );
	if ( buf ) {
		(void) sprintf( buf, "%d", i );
	}
	return buf;
}


int result_error_flag;

static void
free_from_and_to_name()
{
    free( from_name );
    from_name = 0;
    free( to_name );
    to_name = 0;
}



static void
free_task_name()
{
    free( task_name );
    task_name = 0;
}



static void
free_entry_name()
{
    free( entry_name );
    entry_name = 0;
}



static void
free_activity_name()
{
    free( activity_name );
    activity_name = 0;
}



static void
resulterror( const char * fmt )
{
    results_error( fmt );
}

static double
resultinfinity()
{
#if defined(INFINITY)
    return INFINITY;
#else
    union {
	unsigned char c[8];
	double f;
    } x;

#if defined(WORDS_BIGENDIAN)
    x.c[0] = 0x7f;
    x.c[1] = 0xf0;
    x.c[2] = 0x00;
    x.c[3] = 0x00;
    x.c[4] = 0x00;
    x.c[5] = 0x00;
    x.c[6] = 0x00;
    x.c[7] = 0x00;
#else
    x.c[7] = 0x7f;
    x.c[6] = 0xf0;
    x.c[5] = 0x00;
    x.c[4] = 0x00;
    x.c[3] = 0x00;
    x.c[2] = 0x00;
    x.c[1] = 0x00;
    x.c[0] = 0x00;
#endif
    return x.f;
#endif
}

