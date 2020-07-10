/* -*- c++ -*-
 * $Id: errmsg.h 13676 2020-07-10 15:46:20Z greg $
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

#include "dim.h"
#include <lqio/glblerr.h>

extern "C" {
    extern void severity_action(unsigned severity);
}

/*
 * See glblerr.h for entries 1-49.
 */

enum {
    ERR_BOGUS_COPIES=LQIO::LSTGBLERRMSG+1,
    ERR_EXTERNAL_SYNC,
    ERR_REPLICATION_PROCESSOR,
    ERR_REPLICATION,
    ERR_INVALID_FANINOUT_PARAMETER,
    ERR_INFINITE_THROUGHPUT,
    ERR_NO_CALLS_TO_ENTRY,
    ERR_ARRIVAL_RATE,
    ADV_CONVERGENCE_VALUE,
    ADV_LARGE_CONVERGENCE_VALUE,
    ADV_ITERATION_LIMIT,
    ADV_SOLVER_ITERATION_LIMIT,
    ADV_NO_OVERHANG,
    ADV_REPLICATION_ITERATION_LIMIT,
    ADV_SERVICE_TIME_RANGE,
    ADV_EMPTY_SUBMODEL,
    ADV_INVALID_UTILIZATION,
    ADV_UNDERRELAXATION,
    ADV_MANY_CLASSES,
    WRN_COEFFICIENT_OF_VARIATION,
    WRN_INVALID_INT_VALUE,
    WRN_MULTI_PHASE_INFINITE_SERVER,
    WRN_NO_REQUESTS_MADE,
    LSTLCLERRMSG=WRN_NO_REQUESTS_MADE
};
#endif
