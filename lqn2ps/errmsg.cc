/* -*- c++ -*-
 * $Id: errmsg.cc 15694 2022-06-22 23:27:00Z greg $
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
    { LQIO::error_severity::FATAL,    "Layer %d exceeds maximum %d" },										/* FTL_LAYERIZATION			*/
    { LQIO::error_severity::ERROR,    "Activity \"%s\" is not reachable from any entry of task \"%s\"." },					/* LQIO::ERR_ACTIVITY_NOT_REACHABLE	*/
    { LQIO::error_severity::ERROR,    "The number of replicas (%d) for task \"%s\" is not an integer multiple of the number of replicas (%d) for processor \"%s\"." }, /* ERR_REPLICATION_PROCESSOR */
    { LQIO::error_severity::ERROR,    "Fan-out of %d from task \"%s\" with %d replicas does not match the fan-in of %d to task \"%s\" with %d replicas." },	    /* LQIO::ERR_REPLICATION		    */
    { LQIO::error_severity::ERROR,    "Replicas must be a constant for rep2flat:  %s." },							/* ERR_REPLICATION_NOT_SET		*/
    { LQIO::error_severity::ERROR,    "No calls from %s \"%s\" to entry \"%s\"." },								/* LQIO::ERR_NO_CALLS_TO_ENTRY		*/
    { LQIO::error_severity::ERROR,    "No objects selected to print." },									/* LQIO::ERR_NO_OBJECTS			*/
    { LQIO::error_severity::ERROR,    "\"%s\" -- Not implemented." },										/* LQIO::ERR_NOT_IMPLEMENTED		*/
    { LQIO::error_severity::ERROR,    "\"%s\" -- Should not implement." },									/* LQIO::ERR_SHOULD_NOT_IMPLEMENT	*/
    { LQIO::error_severity::ERROR,    "The model has unassigned variables." },									/* LQIO::ERR_UNASSIGNED_VARIABLES	*/
    { LQIO::error_severity::ERROR,    "BCMP model trasformation failed; the following tasks were not pruned: %s." },				/* LQIO::ERR_BCMP_CONVERSION_FAILURE	*/
    { LQIO::error_severity::WARNING,  "Coefficient of variation is incompatible with phase type at %s \"%s\" %s \"%s\"." },			/* WRN_COEFFICIENT_OF_VARIATION		*/
    { LQIO::error_severity::WARNING,  "Mixed phase type detected during merge: source %s \"%s\", destination %s \"%s\"." },			/* WRN_MIXED_PHASE_TYPE			*/
};

/*
 * What to do based on the severity of the error.
 */

void
LQIO::severity_action (error_severity severity)
{
    switch( severity ) {
    case LQIO::error_severity::FATAL:
	(void) exit( 1 );
	break;

    case LQIO::error_severity::ERROR:
	LQIO::io_vars.error_count += 1;
	if  ( LQIO::io_vars.error_count >= LQIO::io_vars.max_error ) {
	    throw ( std::runtime_error( "Too many errors" ) );
	}
	break;
    }
}
