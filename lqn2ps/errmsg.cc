/* -*- c++ -*-
 * $Id: errmsg.cc 15957 2022-10-07 17:14:47Z greg $
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
#include <lqio/error.h>
#include "errmsg.h"

/*
 * Error messages.  Many are inherited from glblerr.h.
 */

std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages = {
    { FTL_LAYERIZATION,		    { LQIO::error_severity::FATAL,    "Layer %d exceeds maximum %d" } },
    { ERR_REPLICATION_NOT_SET,	    { LQIO::error_severity::ERROR,    "Replicas must be a constant for rep2flat:  %s." } },
    { ERR_NO_OBJECTS,		    { LQIO::error_severity::ERROR,    "No objects selected to print." } },
    { ERR_NOT_IMPLEMENTED,	    { LQIO::error_severity::ERROR,    "\"%s\" -- Not implemented." } },
    { ERR_SHOULD_NOT_IMPLEMENT,	    { LQIO::error_severity::ERROR,    "\"%s\" -- Should not implement." } },
    { ERR_UNASSIGNED_VARIABLES,	    { LQIO::error_severity::ERROR,    "The model has unassigned variables." } },
    { ERR_BCMP_CONVERSION_FAILURE,  { LQIO::error_severity::ERROR,    "BCMP model trasformation failed; the following tasks were not pruned: %s." } },
    { WRN_COEFFICIENT_OF_VARIATION, { LQIO::error_severity::WARNING,  "Coefficient of variation is incompatible with phase type at %s \"%s\" %s \"%s\"." } },
    { WRN_MIXED_PHASE_TYPE,	    { LQIO::error_severity::WARNING,  "Mixed phase type detected during merge: source %s \"%s\", destination %s \"%s\"." } }
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
