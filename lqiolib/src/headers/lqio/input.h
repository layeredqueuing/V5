/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 1991.                                                         */
/* March 2010.								*/
/************************************************************************/

/*
 * $Id$
 */

#if	!defined(LQIO_INPUT_H)
#define	LQIO_INPUT_H

/* Don't forget to fix labels.c */
/* Add a new scheduler type 'CFS' 2008*/
typedef enum {
    SCHEDULE_CUSTOMER,
    SCHEDULE_DELAY,
    SCHEDULE_FIFO, 
    SCHEDULE_HOL,  
    SCHEDULE_PPR,  
    SCHEDULE_RAND, 
    SCHEDULE_PS,   
    SCHEDULE_PS_HOL,
    SCHEDULE_PS_PPR,
    SCHEDULE_POLL, 
    SCHEDULE_BURST,
    SCHEDULE_UNIFORM,
    SCHEDULE_SEMAPHORE,
    SCHEDULE_CFS,
    SCHEDULE_RWLOCK
} scheduling_type;	      
#define	N_SCHEDULING_TYPES    15
			      
#define SCHED_CUSTOMER_BIT   (1 << SCHEDULE_CUSTOMER)
#define SCHED_DELAY_BIT	     (1 << SCHEDULE_DELAY)
#define SCHED_FIFO_BIT	     (1 << SCHEDULE_FIFO)
#define SCHED_HOL_BIT	     (1 << SCHEDULE_HOL)
#define SCHED_PPR_BIT        (1 << SCHEDULE_PPR)
#define SCHED_RAND_BIT	     (1 << SCHEDULE_RAND)
#define SCHED_PS_BIT         (1 << SCHEDULE_PS)
#define SCHED_PS_HOL_BIT     (1 << SCHEDULE_PS_HOL)
#define SCHED_PS_PPR_BIT     (1 << SCHEDULE_PS_PPR)
#define SCHED_POLL_BIT	     (1 << SCHEDULE_POLL)
#define SCHED_BURST_BIT      (1 << SCHEDULE_BURST)
#define SCHED_UNIFORM_BIT    (1 << SCHEDULE_UNIFORM)
#define SCHED_SEMAPHORE_BIT  (1 << SCHEDULE_SEMAPHORE)
#define SCHED_CFS_BIT  	     (1 << SCHEDULE_CFS)
#define SCHED_RWLOCK_BIT     (1 << SCHEDULE_RWLOCK)

typedef enum { PHASE_STOCHASTIC, PHASE_DETERMINISTIC } phase_type;
typedef enum { SEMAPHORE_NONE, SEMAPHORE_SIGNAL, SEMAPHORE_WAIT } semaphore_entry_type;
typedef enum { DEFAULT_MATHERR, IGNORE_MATHERR, REPORT_MATHERR, ABORT_MATHERR } matherr_type;
typedef enum { RWLOCK_NONE, RWLOCK_R_UNLOCK, RWLOCK_R_LOCK,RWLOCK_W_UNLOCK,RWLOCK_W_LOCK } rwlock_entry_type;

/* Exit codes -- can be ored */   
#define NORMAL_TERMINATION      (0)
#define INVALID_OUTPUT          (1 << 0)
#define INVALID_INPUT           (1 << 1)
#define INVALID_ARGUMENT        (1 << 2)
#define FILEIO_ERROR            (1 << 3)
#define EXCEPTION_EXIT	        0xff
	
#if defined(__cplusplus)
#include "error.h"
#include <string>

namespace LQIO { struct error_message_type; };
typedef struct LQIO::error_message_type ErrorMessageType;
	
typedef struct lqio_params_stats {
    unsigned n_entries;                         /* O:total number of entries      */
    unsigned n_tasks;                           /* O:total number of tasks        */
    unsigned n_processors;	                /* O:number of processors         */
    unsigned n_groups;                          /* O:total number of groups       */
    const char* lq_toolname;                    /* I:Name of tool for messages    */
    const char* lq_version;			/* I: version number		  */
    const char* lq_command_line;		/* I:Command line		  */
    void (*severity_action)(unsigned severity); /* I:Severity action              */
	
    unsigned max_error;				/* I:Maximum error ID number      */
    unsigned error_count;			/* IO:Number of errors            */
    LQIO::severity_t severity_level;            /* I:Messages < severity_level ignored. */
	
    ErrorMessageType* error_messages;		/* IO:Error Messages */
	
    bool anError;

} lqio_params_stats;

extern int LQIO_lineno;				/* Input line number -- can't use namespace because it's used with C */

extern const char * scheduling_type_str[N_SCHEDULING_TYPES+1];
extern const char * schedulingTypeXMLString[N_SCHEDULING_TYPES+1];   
#endif
#endif	/* LQIO_INPUT_H */
