/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/* August 1991.								*/
/************************************************************************/

/*
 * $Id: errmsg.cc 15738 2022-07-01 01:16:27Z greg $
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

std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages =
{
    { FTL_ACTIVITY_STACK_FULL,		    { LQIO::error_severity::FATAL,  	"Activity stack for \"%s\" is full." } },
    { FTL_MSG_POOL_EMPTY,		    { LQIO::error_severity::FATAL,   	"Message pool is empty.  Sending from \"%s\" to \"%s\"." } },
    { ERR_REPLY_NOT_FOUND,		    { LQIO::error_severity::ERROR, 	"Activity \"%s\" requests reply for entry \"%s\" but none pending." } },
    { ERR_CANNOT_CREATE_X,		    { LQIO::error_severity::ERROR, 	"Cannot create %s %s." } },
    { ERR_DELAY_MULTIPLY_DEFINED,	    { LQIO::error_severity::ERROR, 	"Delay from processor \"%s\" to processor \"%s\" previously specified." } },
    { ERR_INIT_DELAY,			    { LQIO::error_severity::ERROR, 	"Initial delay of %g is too small, %d client(s) still running." } },
    { ERR_NO_QUANTUM_FOR_PS,		    { LQIO::error_severity::ERROR, 	"No quantum greater than zero is specified for PS scheduling discipline at processor \"%s\"."} },
    { ERR_QUANTUM_SPECIFIED_FOR_FIFO,	    { LQIO::error_severity::ERROR, 	"Quantum is specified for FIFO scheduling discipline at processor \"%s\"."} },
    { ERR_INITIALIZATION_FAILED,	    { LQIO::error_severity::ERROR, 	"An error occurred while initializing parasol model.  The simulation was not run." } },
    { ERR_SIGNAL_NO_WAIT,		    { LQIO::error_severity::ERROR, 	"Signal to semaphore task %s with no pending wait." } },
    { ADV_PRECISION,			    { LQIO::error_severity::ADVISORY, 	"Specified confidence interval of %4.2f%% not met after run time of %G. Actual value is %4.2f%%." } },
    { ADV_DEADLOCK,			    { LQIO::error_severity::ADVISORY, 	"Model is deadlocked." } },
    { WRN_NO_PHASE_FOR_HISTOGRAM,	    { LQIO::error_severity::WARNING,  	"Histogram requested for entry \"%s\", phase %d -- phase is not present." } },
    { WRN_NO_QUANTUM_FOR_PS,		    { LQIO::error_severity::WARNING,  	"No quantum specified for PS scheduling discipline at processor \"%s\".  FIFO used." } },
    { WRN_INVALID_PRIORITY,		    { LQIO::error_severity::WARNING,  	"Priority specified (%d) is outside of range (%d,%d). (Value has been adjusted to %d)." } },
};

/*
 * What to do based on the severity of the error.
 */

void
LQIO::severity_action (error_severity severity)
{
    switch( severity ) {
    case LQIO::error_severity::FATAL:
	exit( EXCEPTION_EXIT );
	break;

    case LQIO::error_severity::ERROR:
	LQIO::io_vars.error_count += 1;
	if  ( LQIO::io_vars.error_count >= 10 ) {
	    throw ( std::runtime_error( "Too many errors" ) );
	}
	break;
    default:;
    }
}
