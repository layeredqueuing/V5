/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/errmsg.cc $
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
 * $Id: errmsg.cc 15718 2022-06-27 12:52:14Z greg $
 * ----------------------------------------------------------------------
 */


#include "lqns.h"
#include <lqio/input.h>
#include <lqio/error.h>
#include "flags.h"
#include "errmsg.h"

/*
 * Error messages.  Many are inherited from glblerr.h.
 */

struct LQIO::error_message_type local_error_messages[] =
{
    { LQIO::error_severity::ERROR,    "Arrival rate of %g to entry \"%s\" exceeds service rate of %g." },		                                /* ERR_ARRIVAL_RATE                     */
    { LQIO::error_severity::ERROR,    "Derived population of %g for task \"%s\" is not valid." },                                                  	/* ERR_BOGUS_COPIES                     */
    { LQIO::error_severity::ERROR,    "External synchronization at join \"%s\" for task \"%s\"." },							/* ERR_EXTERNAL_SYNC                    */
    { LQIO::error_severity::ERROR,    "Fan-out of %d from task \"%s\" with %d replicas does not match the fan-in of %d to task \"%s\" with %d replicas." }, /* ERR_REPLICATION             */
    { LQIO::error_severity::ERROR,    "Invalid %s for task \"%s\" to task \"%s\": %s." },				                        	/* ERR_INVALID_FANINOUT_PARAMETER	*/
    { LQIO::error_severity::ERROR,    "No calls from %s \"%s\" to entry \"%s\"." },                                                                	/* ERR_NO_CALLS_TO_ENTRY                */
    { LQIO::error_severity::ERROR,    "The number of replicas (%d) for task \"%s\" is not an integer multiple of the number of replicas (%d) for processor \"%s\"." },	/* ERR_REPLICATION_PROCESSOR 	*/
    { LQIO::error_severity::ERROR,    "The number of replicas (%d) for task \"%s\" is not an integer multiple of the number of replicas (%d) for processor \"%s\"." },	/* ERR_BAD_ 	*/
    { LQIO::error_severity::WARNING,  "Coefficient of variation is incompatible with phase type at %s \"%s\" %s \"%s\"." },                        	/* WRN_COEFFICIENT_OF_VARIATION         */
    { LQIO::error_severity::WARNING,  "Entry \"%s\" on infinite server \"%s\" has %d phases." },							/* WRN_MULTI_PHASE_INFINITE_SERVER	*/
    { LQIO::error_severity::WARNING,  "No requests made from \"%s\" to \"%s\"." },                                                                 	/* WRN_NO_REQUESTS_MADE                 */
    { LQIO::error_severity::ADVISORY, "Invalid convergence value of %g, using %g." },                                                              	/* ADV_CONVERGENCE_VALUE                */
    { LQIO::error_severity::ADVISORY, "Convergence value of %g may be too large -- check results!" },                                              	/* ADV_LARGE_CONVERGENCE_VALUE          */
    { LQIO::error_severity::ADVISORY, "Iteration limit of %d is too small, using %d." },                                                           	/* ADV_ITERATION_LIMIT                  */
    { LQIO::error_severity::ADVISORY, "Model failed to converge after %d iterations (convergence test is %g, limit is %g)." },                     	/* ADV_SOLVER_ITERATION_LIMIT           */
    { LQIO::error_severity::ADVISORY, "Model results are invalid (Reference task \"%s\" utilization of \"%g\" is not equal to copies \"%d\")." },	/* ADV_INVALID_REF_TASK_UTILIZATION 	*/
    { LQIO::error_severity::ADVISORY, "Overhanging threads are ignored." },                                                                        	/* ADV_NO_OVERHANG                      */
    { LQIO::error_severity::ADVISORY, "Replicated Submodel %d failed to converge after %d iterations (convergence test is %g, limit is %g)." },    	/* ADV_REPLICATION_ITERATION_LIMIT      */
    { LQIO::error_severity::ADVISORY, "Service times for %s \"%s\" have a range of %g - %g. Results may not be valid." },                          	/* ADV_SERVICE_TIME_RANGE               */
    { LQIO::error_severity::ADVISORY, "Submodel %d is empty." },                                                                                   	/* ADV_EMPTY_SUBMODEL                   */
    { LQIO::error_severity::ADVISORY, "The utilization at %s \"%s\" with multiplicity %d of %f is too high." },                                    	/* ADV_INVALID_UTILIZATION              */
    { LQIO::error_severity::ADVISORY, "The utilization at %s \"%s\" is infinite." },	                                          			/* ADV_INFINITE_UTILIZATION             */
    { LQIO::error_severity::ADVISORY, "Under-relaxation ignored.  %g outside range [0-2), using %g." },                                            	/* ADV_UNDERRELAXATION                  */
    { LQIO::error_severity::ADVISORY, "This model has a large number of clients (%d) in submodel %d, use of '#pragma mva=schweitzer' is advised." },	/*ADV_MANY_CLASSES			*/
    { LQIO::error_severity::ADVISORY, "The MVA solver reported %d convergence faults during solution." },						/* ADV_MVA_FAULTS			*/
    { LQIO::error_severity::ALL, nullptr }
};

/*
 * What to do based on the severity of the error.
 */

void
LQIO::severity_action (LQIO::error_severity severity)
{
    switch( severity ) {
    case LQIO::error_severity::FATAL:
	(void) abort();
	break;

    case LQIO::error_severity::ERROR:
	LQIO::io_vars.error_count += 1;
	if  ( LQIO::io_vars.error_count >= LQIO::io_vars.max_error ) {
	    throw std::runtime_error( "Too many errors" );
	}
	break;
    default:;
    }
}
