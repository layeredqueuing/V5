/* -*- c++ -*-
 * $Id: errmsg.h 13684 2020-07-13 15:41:25Z greg $
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
#include "lqn2ps.h"

class Entry;

extern "C" {
    extern void severity_action(unsigned severity);
    extern void init_errmsg(void);
}

enum {
    FTL_LAYERIZATION=LQIO::LSTGBLERRMSG+1,
    ERR_ACTIVITY_NOT_REACHABLE,
    ERR_REPLICATION_PROCESSOR,
    ERR_REPLICATION,
    ERR_NO_CALLS_TO_ENTRY,
    ERR_NO_OBJECTS,
    ERR_NOT_IMPLEMENTED,
    ERR_SHOULD_NOT_IMPLEMENT,
    WRN_COEFFICIENT_OF_VARIATION,
    WRN_MIXED_PHASE_TYPE,
    LSTLCLERRMSG=WRN_MIXED_PHASE_TYPE
};

#endif
