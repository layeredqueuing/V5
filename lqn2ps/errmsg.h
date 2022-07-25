/* -*- c++ -*-
 * $Id: errmsg.h 15755 2022-07-24 10:34:56Z greg $
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

/* #include <values.h> */
#include <lqio/glblerr.h>
#include <vector>

enum {
    FTL_LAYERIZATION=LQIO::LSTGBLERRMSG+1,
    ERR_ACTIVITY_NOT_REACHABLE,
    ERR_REPLICATION_NOT_SET,
    ERR_NO_CALLS_TO_ENTRY,
    ERR_NO_OBJECTS,
    ERR_NOT_IMPLEMENTED,
    ERR_SHOULD_NOT_IMPLEMENT,
    ERR_UNASSIGNED_VARIABLES,
    ERR_BCMP_CONVERSION_FAILURE,
    WRN_COEFFICIENT_OF_VARIATION,
    WRN_MIXED_PHASE_TYPE
};

extern std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages;
#endif
