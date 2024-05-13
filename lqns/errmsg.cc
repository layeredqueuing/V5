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
 * $Id: errmsg.cc 17209 2024-05-13 18:16:37Z greg $
 * ----------------------------------------------------------------------
 */

#include "lqns.h"
#include <lqio/dom_document.h>
#include <lqio/error.h>
#include "errmsg.h"

/*
 * Error messages.  Many are inherited from glblerr.h.
 */

std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages =
{
    {  ERR_BOGUS_COPIES,			{ LQIO::error_severity::ERROR,    "Derived population of %g for task \"%s\" is not valid." } },
    {  WRN_MULTI_PHASE_INFINITE_SERVER,		{ LQIO::error_severity::WARNING,  "Entry \"%s\" on infinite server \"%s\" has %d phases." } },
    {  ADV_BCMP_NOT_SUPPORTED,			{ LQIO::error_severity::ADVISORY, "BCMP conversion for submodel %d does not support %s." } },
    {  ADV_CONVERGENCE_VALUE,			{ LQIO::error_severity::ADVISORY, "Invalid convergence value of %g, using %g." } },
    {  ADV_EMPTY_SUBMODEL,			{ LQIO::error_severity::ADVISORY, "Submodel %d is empty." } },
    {  ADV_INFINITE_UTILIZATION,		{ LQIO::error_severity::ADVISORY, "The utilization at %s \"%s\" is infinite." } },
    {  ADV_INVALID_REF_TASK_UTILIZATION,	{ LQIO::error_severity::ADVISORY, "Model results are invalid (Reference task \"%s\" utilization of \"%g\" is not equal to copies \"%d\")." } },
    {  ADV_INVALID_UTILIZATION,			{ LQIO::error_severity::ADVISORY, "The utilization at %s \"%s\" with multiplicity %d of %f is too high." } },
    {  ADV_ITERATION_LIMIT,			{ LQIO::error_severity::ADVISORY, "Iteration limit of %d is too small, using %d." } },
    {  ADV_LARGE_CONVERGENCE_VALUE,		{ LQIO::error_severity::ADVISORY, "Convergence value of %g may be too large -- check results!" } },
    {  ADV_MANY_CLASSES,			{ LQIO::error_severity::ADVISORY, "This model has a large number of clients (%d) in submodel %d, use of '#pragma mva=schweitzer' is advised." } },
    {  ADV_MVA_FAULTS,    			{ LQIO::error_severity::ADVISORY, "The MVA solver reported %d convergence faults during solution." } },
    {  ADV_NO_OVERHANG,				{ LQIO::error_severity::ADVISORY, "Overhanging threads are ignored." } },
    {  ADV_REPLICATION_ITERATION_LIMIT,		{ LQIO::error_severity::ADVISORY, "Replicated Submodel %d failed to converge after %d iterations (convergence test is %g, limit is %g)." } },
    {  ADV_SERVICE_TIME_RANGE,			{ LQIO::error_severity::ADVISORY, "Service times for %s \"%s\" have a range of %g - %g. Results may not be valid." } },
    {  ADV_SOLVER_ITERATION_LIMIT,		{ LQIO::error_severity::ADVISORY, "Model failed to converge after %d iterations (convergence test is %g, limit is %g)." } },
    {  ADV_UNDERRELAXATION,			{ LQIO::error_severity::ADVISORY, "Under-relaxation ignored.  %g outside range [0-2), using %g." } },
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
