/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/* August 1991.								*/
/************************************************************************/

/*
 * $Id: errmsg.cc 17290 2024-09-16 16:10:54Z greg $
 */

#include <cstdio>
#include <stdlib.h>
#include <stdexcept>
#include <vector>
#include <lqio/dom_document.h>
#include <lqio/error.h>
#include "errmsg.h"
#include "lqsim.h"

/*
 * Error messages.  Note:  always call LQIO::runtime_error or LQIO::input_error and never dom->runtime_error().
 */

std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages =
{
    { FTL_ACTIVITY_STACK_FULL,		    { LQIO::error_severity::FATAL,  	"Activity stack for \"%s\" is full." } },
    { ERR_CANNOT_CREATE_X,		    { LQIO::error_severity::ERROR, 	"Cannot create %s %s." } },
    { ERR_DELAY_MULTIPLY_DEFINED,	    { LQIO::error_severity::ERROR, 	"Delay from processor \"%s\" to processor \"%s\" previously specified." } },
    { ERR_INITIALIZATION_FAILED,	    { LQIO::error_severity::ERROR, 	"An error occurred while initializing parasol model.  The simulation was not run." } },
    { ERR_INIT_DELAY,			    { LQIO::error_severity::ERROR, 	"Initial delay of %g is too small, %d client(s) still running." } },
    { ERR_MSG_POOL_EMPTY,		    { LQIO::error_severity::ERROR,   	"Message pool is empty.  Sending from \"%s\" to \"%s\"." } },
    { ERR_NO_QUANTUM_FOR_PS,		    { LQIO::error_severity::ERROR, 	"No quantum greater than zero is specified for PS scheduling discipline at processor \"%s\"."} },
    { ERR_REPLY_NOT_FOUND,		    { LQIO::error_severity::ERROR, 	"Activity \"%s\" requests reply for entry \"%s\" but none pending." } },
    { ERR_SIGNAL_NO_WAIT,		    { LQIO::error_severity::ERROR, 	"Signal to semaphore task %s with no pending wait." } },
    { ADV_PRECISION,			    { LQIO::error_severity::ADVISORY, 	"Specified confidence interval of %4.2f%% not met after run time of %G. Actual value is %4.2f%%." } },
    { ADV_DEADLOCK,			    { LQIO::error_severity::ADVISORY, 	"Model is deadlocked." } },
    { WRN_NO_PHASE_FOR_HISTOGRAM,	    { LQIO::error_severity::WARNING,  	"Histogram requested for entry \"%s\", phase %d -- phase is not present." } },
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
