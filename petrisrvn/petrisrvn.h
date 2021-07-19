/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/* January 2005.							*/
/************************************************************************/

#ifndef _PETRISRVN_H
#define _PETRISRVN_H

/*
 * $Id: petrisrvn.h 14910 2021-07-16 14:09:02Z greg $
 *
 * Solve LQN using petrinets.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <stdexcept>
#include <cstdio>
#include <regex.h>
#include <lqio/input.h>

/*  #define OLD_JOIN */
#define BUG_622			/* Measure service times for activities */
#define BUG_163			/* Measure sync delay.			*/
#define BUG_263			/* Quroum delay				*/

extern struct lqio_params_stats io_vars;

enum class Mode { SOLVE, RELOAD, RESTART };

/*
 * Layers in the petrinet.
 */

#define	PROC_LAYER_NUM		1
#define PRIMARY_LAYER_NUM	2
#define	SERVICE_RATE_LAYER_NUM	3
#define	CALL_RATE_LAYER_NUM	4
#define MEASUREMENT_LAYER_NUM	5
#define	FIFO_LAYER_NUM		6
#define	JOIN_LAYER_NUM		7
#define	ENTRY_LAYER_NUM		8

#define	NULL_LAYER		0
#define	PROC_LAYER		(LAYER)(1<<PROC_LAYER_NUM)
#define PRIMARY_LAYER		(LAYER)(1<<PRIMARY_LAYER_NUM)
#define	SERVICE_RATE_LAYER	(LAYER)(1<<SERVICE_RATE_LAYER_NUM)
#define	CALL_RATE_LAYER		(LAYER)(1<<CALL_RATE_LAYER_NUM)
#define	MEASUREMENT_LAYER	(LAYER)(1<<MEASUREMENT_LAYER_NUM)
#define	FIFO_LAYER		(LAYER)(1<<FIFO_LAYER_NUM)
#define	JOIN_LAYER		(LAYER)(1<<JOIN_LAYER_NUM)
#define	ENTRY_LAYER(n)		(LAYER)((1<<(ENTRY_LAYER_NUM-1+(n))))

/*----------------------------------------------------------------------*/

#define DIMI 15				/* max 15 tasks 	    	*/
#define DIME 30				/* max 30 entries	    	*/
#define DIMET 10			/* max 10 entries/task	    	*/
#define DIMPH 3				/* max 3 phases/task	    	*/
#define DIMSLICE 5			/* max 5 slices/phase.		*/
#define DIMP DIMI			/* max 20 processors	    	*/
#define	MAX_MULT 7			/* Largest multi-server (+1)	*/
#define MAX_ACT 25			/* 25 activities per task.	*/
#define MAX_BRANCH 10			/* 10 branchs per fork-join.	*/
#define MAX 1.e+12
#define MAX_STAGE 4			/* For erlang distributions.	*/
#define OPEN_MODEL_TOKENS 15		/* Number of tokens in open pl.	*/
#define N_SEMAPHORE_ENTRIES 2		/* N entries for semaphore task	*/

#define	INFINITE_SERVER	0
#define EPSILON 0.0001

extern double comm_delay[DIMP+1][DIMP+1];/* Delay in sending a message.	*/

extern regex_t * inservice_match_pattern;

extern bool debug_flag;			/* Spew out all calls to get data.	*/
extern bool keep_flag;                  /* Keep result files?          		*/
extern bool no_execute_flag;		/* Don't call solver.			*/
extern bool parse_flag;                 /* Parsable output desired?    		*/
extern bool reload_net_flag;		/* Reload result values and print.	*/
extern bool rtf_flag;                   /* Parsable output desired?    		*/
extern bool trace_flag;			/* Trace greatspn execution.		*/
extern bool uncondition_flag;		/* Uncodition pr{inservice|}		*/
extern bool verbose_flag;		/* Spew out GreatSPN noise.		*/
extern bool xml_flag;			/* XML Output desired  ?		*/

extern bool customers_flag;		/* Smash customers together.		*/
extern bool distinguish_join_customers;	/* Cust at join multi-server are unique	*/ 
extern bool simplify_network;		/* Delete single place processors	*/

#if 0
extern bool comm_delay_flag;		/* Communication delays present		*/
extern double inter_proc_delay;		/* Initial value;			*/
#endif
extern double x_scaling;		/* Auto-squish if val == 0.		*/

extern unsigned open_model_tokens;	/* Default global open queue max size	*/

extern FILE * stddbg;			/* debugging output goes here.		*/

static inline bool bit_test( unsigned flag, unsigned bits ) { return ((1 << flag) & bits ) != 0; }
static inline void throw_bad_parameter() { throw std::domain_error( "invalid parameter" ); }

#endif /* _PETRISRVN_H */
