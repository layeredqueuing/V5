/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/* August 1991.								*/
/************************************************************************/

/*
 * $id$
 */

#include <cstdio>
#include <stdlib.h>
#include <stdexcept>
#include <lqio/input.h>
#include <lqio/error.h>
#include "errmsg.h"
#include "lqsim.h"

/*
 * Error messages.
 */

struct LQIO::error_message_type local_error_messages[] =
{
    { LQIO::FATAL_ERROR,   "Activity stack for \"%s\" is full." },                                                             	/* FTL_ACTIVITY_STACK_FULL          */
    { LQIO::FATAL_ERROR,   "Message pool is empty.  Sending from \"%s\" to \"%s\"." },                                         	/* FTL_MSG_POOL_EMPTY               */
    { LQIO::RUNTIME_ERROR, "%s \"%s\": Replication not supported." },                                                          	/* ERR_REPLICATION                  */
    { LQIO::RUNTIME_ERROR, "Activity \"%s\" requests reply for entry \"%s\" but none pending." },                              	/* ERR_REPLY_NOT_FOUND              */
    { LQIO::RUNTIME_ERROR, "Cannot create %s %s." },                                                                           	/* ERR_CANNOT_CREATE_X              */
    { LQIO::RUNTIME_ERROR, "Delay from processor \"%s\" to processor \"%s\" previously specified." },                          	/* ERR_DELAY_MULTIPLY_DEFINED       */
    { LQIO::RUNTIME_ERROR, "Initial delay of %g is too small, %d client(s) still running." },                                   /* ERR_INIT_DELAY                   */
    { LQIO::RUNTIME_ERROR, "No quantum greater than zero is specified for PS scheduling discipline at processor \"%s\"."},      /* ERR_NO_QUANTUM_FOR_PS	    */
    { LQIO::RUNTIME_ERROR, "Quantum is specified for FIFO scheduling discipline at processor \"%s\"."}, 			/* ERR_QUANTUM_SPECIFIED_FOR_FIFO   */
    { LQIO::RUNTIME_ERROR, "An error occurred while initializing parasol model.  The simulation was not run." },		/* ERR_INITIALIZATION_FAILED 	    */
    { LQIO::RUNTIME_ERROR, "Signal to semaphore task %s with no pending wait." },						/* ERR_SIGNAL_NO_WAIT		    */
    { LQIO::ADVISORY_ONLY, "Specified confidence interval of %4.2f%% not met after run time of %G. Actual value is %4.2f%%." }, /* ADV_PRECISION                    */
    { LQIO::ADVISORY_ONLY, "Model is deadlocked." }, 										/* ADV_DEADLOCK			    */
    { LQIO::WARNING_ONLY,  "Histogram requested for entry \"%s\", phase %d -- phase is not present." },         		/* WRN_NO_PHASE_FOR_HISTOGRAM       */
    { LQIO::WARNING_ONLY,  "No quantum specified for PS scheduling discipline at processor \"%s\".  FIFO used." },              /* WRN_NO_QUANTUM_FOR_PS            */
    { LQIO::WARNING_ONLY,  "Priority specified (%d) is outside of range (%d,%d). (Value has been adjusted to %d)." },           /* WRN_INVALID_PRIORITY             */
    { LQIO::NO_ERROR, 0 }
};

/*
 * Copy over common error messages and set max_error.
 */

struct LQIO::error_message_type error_messages[LSTLCLERRMSG+1];

void
init_errmsg()
{
    unsigned i, j;

    for ( i = 1; i <= LQIO::LSTGBLERRMSG; ++i ) {
	error_messages[i] = LQIO::global_error_messages[i];
    }
    for ( j = 0; i <= LSTLCLERRMSG; ++i, ++j ) {
	error_messages[i] = local_error_messages[j];
    }
    LQIO::io_vars.error_messages = &error_messages[0];
    LQIO::io_vars.max_error = LSTLCLERRMSG;
}

/*
 * What to do based on the severity of the error.
 */

void
severity_action (unsigned severity)
{
    switch( severity ) {
    case LQIO::FATAL_ERROR:
	exit( EXCEPTION_EXIT );
	break;

    case LQIO::RUNTIME_ERROR:
	LQIO::io_vars.error_count += 1;
	if  ( LQIO::io_vars.error_count >= 10 ) {
	    throw ( std::runtime_error( "Too many errors" ) );
	}
	break;
    }
}
