/* -*- c++ -*-
 * $Id$
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
    { LQIO::FATAL_ERROR,   "Layer %d exceeds maximum %d" },                                                                                 /* FTL_LAYERIZATION             */
    { LQIO::RUNTIME_ERROR, "Activity \"%s\" is not reachable from any entry of task \"%s\"." },                                             /* LQIO::ERR_ACTIVITY_NOT_REACHABLE   */
    { LQIO::RUNTIME_ERROR, "Fan-in from task \"%s\" to task \"%s\" are not identical for all calls." },                                     /* LQIO::ERR_FANIN_MISMATCH           */
    { LQIO::RUNTIME_ERROR, "Invalid fan-in of %d: source task \"%s\" is not replicated." },                                                 /* LQIO::ERR_INVALID_FANIN            */
    { LQIO::RUNTIME_ERROR, "Invalid fan-out of %d: destination task \"%s\" has only %d replicas." },                                        /* LQIO::ERR_INVALID_FANOUT           */
    { LQIO::RUNTIME_ERROR, "Fan-out from %s \"%s\" (%d * %d replicas) does not match fan-in to %s \"%s\" (%d * %d)." },                     /* LQIO::ERR_REPLICATION              */
    { LQIO::RUNTIME_ERROR, "No calls from %s \"%s\" to entry \"%s\"." },                                                                    /* LQIO::ERR_NO_CALLS_TO_ENTRY        */
    { LQIO::RUNTIME_ERROR, "No objects selected to print." },                                                                               /* LQIO::ERR_NO_OBJECTS               */
    { LQIO::RUNTIME_ERROR, "\"%s\" -- Not implemented." },                                                                                  /* LQIO::ERR_NOT_IMPLEMENTED          */
    { LQIO::RUNTIME_ERROR, "\"%s\" -- Should not implement." },                                                                             /* LQIO::ERR_SHOULD_NOT_IMPLEMENT     */
    { LQIO::WARNING_ONLY,  "Coefficient of variation is incompatible with phase type at task \"%s\"." },                                    /* LQIO::WRN_COEFFICIENT_OF_VARIATION */
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
	io_vars.anError = true;
	io_vars.error_count += 1;
	if  ( io_vars.error_count >= 10 ) {
	    throw ( runtime_error( "Too many errors" ) );
	}
	break;
    }
}
