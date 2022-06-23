/* -*- c++ -*-
 * $Id: errmsg.h 15694 2022-06-22 23:27:00Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November 1990.
 * August 1991.
 * March 1993.
 * December 1995.
 *----------------------------------------------------------------------
 */

#if	!defined(ERRMSG_H)
#define	ERRMSG_H

#include <lqio/glblerr.h>

/*
 * See glblerr.h for entries 1-49.
 */

enum {
    ERR_ARRIVAL_RATE=LQIO::LSTGBLERRMSG+1,
    ERR_BOGUS_COPIES,
    ERR_EXTERNAL_SYNC,
    ERR_REPLICATION,
    ERR_INVALID_FANINOUT_PARAMETER,
    ERR_NO_CALLS_TO_ENTRY,
    ERR_REPLICATION_PROCESSOR,
    WRN_COEFFICIENT_OF_VARIATION,
    WRN_MULTI_PHASE_INFINITE_SERVER,
    WRN_NO_REQUESTS_MADE,
    ADV_CONVERGENCE_VALUE,
    ADV_LARGE_CONVERGENCE_VALUE,
    ADV_ITERATION_LIMIT,
    ADV_SOLVER_ITERATION_LIMIT,
    ADV_INVALID_REF_TASK_UTILIZATION,
    ADV_NO_OVERHANG,
    ADV_REPLICATION_ITERATION_LIMIT,
    ADV_SERVICE_TIME_RANGE,
    ADV_EMPTY_SUBMODEL,
    ADV_INVALID_UTILIZATION,
    ADV_INFINITE_UTILIZATION,
    ADV_UNDERRELAXATION,
    ADV_MANY_CLASSES,
    ADV_MVA_FAULTS,
    LSTLCLERRMSG=ADV_MVA_FAULTS
};

extern struct LQIO::error_message_type local_error_messages[];
#endif
