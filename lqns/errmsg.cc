/* -*- c++ -*-
 * $HeadURL$
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
 * $Id$
 * ----------------------------------------------------------------------
 */


#include "dim.h"
#include <stdexcept>
#include <cstdlib>
#include <lqio/input.h>
#include <lqio/error.h>
#include "lqns.h"
#include "errmsg.h"

/*
 * Error messages.  Many are inherited from glblerr.h.
 */

struct LQIO::error_message_type local_error_messages[] =
{
    { LQIO::RUNTIME_ERROR, "Derived population of %g for task \"%s\" is not valid." },                                                  /* ERR_BOGUS_COPIES                     */
    { LQIO::RUNTIME_ERROR, "External synchronization from entry \"%s\" not supported for task \"%s\": backtrace is \"%s\"." },          /* ERR_EXTERNAL_SYNC                    */
    { LQIO::RUNTIME_ERROR, "Fan-in from task \"%s\" to task \"%s\" are not identical for all calls." },                                 /* ERR_FANIN_MISMATCH                   */
    { LQIO::RUNTIME_ERROR, "Fan-out from %s \"%s\" (%d * %d replicas) does not match fan-in to %s \"%s\" (%d * %d)." },                 /* ERR_REPLICATION                      */
    { LQIO::RUNTIME_ERROR, "Infinite throughput for task \"%s\".  Model specification error." },                                        /* ERR_INFINITE_THROUGHPUT              */
    { LQIO::RUNTIME_ERROR, "Invalid fan-in of %d: source task \"%s\" is not replicated." },                                             /* ERR_INVALID_FANIN                    */
    { LQIO::RUNTIME_ERROR, "Invalid fan-out of %d: destination task \"%s\" has only %d replicas." },                                    /* ERR_INVALID_FANOUT                   */
    { LQIO::RUNTIME_ERROR, "No calls from %s \"%s\" to entry \"%s\"." },                                                                /* ERR_NO_CALLS_TO_ENTRY                */
    { LQIO::RUNTIME_ERROR, "Open arrival rate of %g to task \"%s\" is too high.  Service time is %g." },                                /* ERR_ARRIVAL_RATE                     */
    { LQIO::ADVISORY_ONLY, "Invalid convergence value of %g, using %g." },                                                              /* ADV_CONVERGENCE_VALUE                */
    { LQIO::ADVISORY_ONLY, "Convergence value of %g may be too large -- check results!" },                                              /* ADV_LARGE_CONVERGENCE_VALUE          */
    { LQIO::ADVISORY_ONLY, "Iteration limit of %d is too small, using %d." },                                                           /* ADV_ITERATION_LIMIT                  */
    { LQIO::ADVISORY_ONLY, "Model failed to converge after %d iterations (convergence test is %g, limit is %g)." },                     /* ADV_SOLVER_ITERATION_LIMIT           */
    { LQIO::ADVISORY_ONLY, "Overhanging threads are ignored." },                                                                        /* ADV_NO_OVERHANG                      */
    { LQIO::ADVISORY_ONLY, "Replicated Submodel %d failed to converge after %d iterations (convergence test is %g, limit is %g)." },    /* ADV_REPLICATION_ITERATION_LIMIT      */
    { LQIO::ADVISORY_ONLY, "Service times for %s \"%s\" have a range of %g - %g. Results may not be valid." },                          /* ADV_SERVICE_TIME_RANGE               */
    { LQIO::ADVISORY_ONLY, "Submodel %d is empty." },                                                                                   /* ADV_EMPTY_SUBMODEL                   */
    { LQIO::ADVISORY_ONLY, "The utilization of %f at %s \"%s\" with multiplicity %d is too high." },                                    /* ADV_INVALID_UTILIZATION              */
    { LQIO::ADVISORY_ONLY, "Under-relaxation ignored.  %g outside range [0-2), using %g." },                                            /* ADV_UNDERRELAXATION                  */
    { LQIO::WARNING_ONLY,  "Coefficient of variation is incompatible with phase type at %s \"%s\" %s \"%s\"." },                        /* WRN_COEFFICIENT_OF_VARIATION         */
    { LQIO::WARNING_ONLY,  "Value specified for %s, %d, is invalid." },                                                                 /* WRN_INVALID_INT_VALUE                */
    { LQIO::WARNING_ONLY,  "Entry \"%s\" on inisfinite server \"%s\" has %d phases." },							/* WRN_MULTI_PHASE_INFINITE_SERVER	*/
    { LQIO::WARNING_ONLY,  "No requests made from \"%s\" to \"%s\"." },                                                                 /* WRN_NO_REQUESTS_MADE                 */
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
    io_vars.error_messages = &error_messages[0];
    io_vars.max_error = LSTLCLERRMSG;
}

/*
 * What to do based on the severity of the error.
 */

void
severity_action (unsigned severity)
{
    switch( severity ) {
	case LQIO::FATAL_ERROR:
	(void) abort();
	break;

    case LQIO::RUNTIME_ERROR:
	io_vars.anError = true;
	io_vars.error_count += 1;
	if  ( io_vars.error_count >= 10 ) {
	    throw runtime_error( "Too many errors" );
	}
	break;
    }
}
