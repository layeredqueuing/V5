/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/* August 1991.								*/
/************************************************************************/

/*
 * $Id: errmsg.cc 13727 2020-08-04 14:06:18Z greg $
 */

#include "petrisrvn.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <lqio/error.h>
#include <lqio/input.h>
#include "errmsg.h"

/*
 * Error messages.
 */

struct LQIO::error_message_type local_error_messages[] =
{
    { LQIO::FATAL_ERROR,   "Tag hash table overflow."},                                                                                           /* FTL_TAG_TABLE_FULL                   */
    { LQIO::RUNTIME_ERROR, "Delay from processor \"%s\" to processor \"%s\" previously specified."},                                              /* ERR_DELAY_MULTIPLY_DEFINED           */
    { LQIO::RUNTIME_ERROR, "%s \"%s\": Replication not supported."},                                                                              /* ERR_REPLICATION                      */
    { LQIO::RUNTIME_ERROR, "Send-no-reply from \"%s\" to \"%s\" is not supported."},                                                              /* ERR_SEND_NO_REPLIES_PROHIBITED       */
    { LQIO::RUNTIME_ERROR, "Entry \"%s\" for reference task \"%s\" must have service time, think time, or deterministic phases."},                /* ERR_BOGUS_REFERENCE_TASK             */
    { LQIO::RUNTIME_ERROR, "Task \"%s\" provides external synchronization: it cannot be a multiserver."},                                         /* ERR_MULTI_SYNC_SERVER                */
    { LQIO::WARNING_ONLY,  "Convergence problems for \"%s\"; precision is %g."},                                                                  /* WRN_CONVERGENCE                      */
    { LQIO::WARNING_ONLY,  "Premptive scheduling for processor \"%s\" cannot be used with non-unity coefficient of variation at entry \"%s\"."},  /* WRN_PREEMPTIVE_SCHEDULING            */
    { LQIO::ADVISORY_ONLY, "Open-class messages are dropped at task \"%s\" with probability %g."},                                                /* ADV_MESSAGES_LOST                    */
    { LQIO::ADVISORY_ONLY, "Throughput %g does not match open arrival rate %g at Entry \"%s\"."},                                                 /* ADV_OPEN_ARRIVALS_DONT_MATCH         */
    { LQIO::ADVISORY_ONLY, "Using Erlang %d distribution for Entry \"%s\"."},                                                                     /* ADV_ERLANG_N                         */
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
	if  ( LQIO::io_vars.error_count >= LQIO::io_vars.max_error ) {
	    throw std::runtime_error( "Too many errors" );
	}
	break;
    }
}
