/* -*- c++ -*-
 * $Id: errmsg.cc 13542 2020-05-19 15:01:32Z greg $
 *
 * Error messages.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * Novemeber 1990.
 * August 1991.
 * November, 1994
 *
 * ----------------------------------------------------------------------
 */

#include "lqn2ps.h"
#include <stdio.h>
#include <cstdlib>
#include <lqio/input.h>
#include <lqio/error.h>
#include "errmsg.h"

/*
 * Error messages.  Many are inherited from glblerr.h.
 */


struct LQIO::error_message_type local_error_messages[] = {
    { LQIO::FATAL_ERROR,   "Layer %d exceeds maximum %d" },                                                                   	/* FTL_LAYERIZATION             	*/
    { LQIO::RUNTIME_ERROR, "Activity \"%s\" is not reachable from any entry of task \"%s\"." },                                 /* LQIO::ERR_ACTIVITY_NOT_REACHABLE     */
    { LQIO::RUNTIME_ERROR, "The number of replicas (%d) for task \"%s\" is not an integer multiple of the number of replicas (%d) for processor \"%s\"." }, /* ERR_REPLICATION_PROCESSOR */
    { LQIO::RUNTIME_ERROR, "Fan-out of %d from task \"%s\" with %d replicas does not match the fan-in of %d to task \"%s\" with %d replicas." },         /* LQIO::ERR_REPLICATION                */
    { LQIO::RUNTIME_ERROR, "No calls from %s \"%s\" to entry \"%s\"." },                                                        /* LQIO::ERR_NO_CALLS_TO_ENTRY          */
    { LQIO::RUNTIME_ERROR, "No objects selected to print." },                                                                   /* LQIO::ERR_NO_OBJECTS                 */
    { LQIO::RUNTIME_ERROR, "\"%s\" -- Not implemented." },                                                                      /* LQIO::ERR_NOT_IMPLEMENTED            */
    { LQIO::RUNTIME_ERROR, "\"%s\" -- Should not implement." },                                                                 /* LQIO::ERR_SHOULD_NOT_IMPLEMENT       */
    { LQIO::WARNING_ONLY,  "Coefficient of variation is incompatible with phase type at %s \"%s\" %s \"%s\"." },                /* WRN_COEFFICIENT_OF_VARIATION         */
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
    io_vars.error_messages = error_messages;
    io_vars.max_error = LSTLCLERRMSG;

    /* Adjust priority */

    io_vars.error_messages[LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH].severity = LQIO::WARNING_ONLY;
    io_vars.error_messages[LQIO::ERR_CYCLE_IN_CALL_GRAPH].severity = LQIO::WARNING_ONLY;
    io_vars.error_messages[LQIO::ERR_MISSING_OR_BRANCH].severity = LQIO::WARNING_ONLY;
}

/*
 * What to do based on the severity of the error.
 */

void
severity_action (unsigned severity)
{
    switch( severity ) {
    case LQIO::FATAL_ERROR:
	(void) exit( 1 );
	break;

    case LQIO::RUNTIME_ERROR:
	io_vars.error_count += 1;
	if  ( io_vars.error_count >= io_vars.max_error ) {
	    throw ( runtime_error( "Too many errors" ) );
	}
	break;
    }
}
