/*  -*- c++ -*-
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * Novmeber 1990.
 * August 1991.
 */

/*
 * $Id$
 */

#include <lqio/glblerr.h>

extern "C" {
    extern void severity_action(unsigned severity);
}

enum {
    FTL_TAG_TABLE_FULL=LQIO::LSTGBLERRMSG+1,
    ERR_DELAY_MULTIPLY_DEFINED,
    ERR_REPLICATION,
    ERR_SEND_NO_REPLIES_PROHIBITED,
    ERR_BOGUS_REFERENCE_TASK,
    ERR_MULTI_SYNC_SERVER,
    WRN_CONVERGENCE,
    WRN_PREEMPTIVE_SCHEDULING,
    ADV_MESSAGES_LOST,
    ADV_OPEN_ARRIVALS_DONT_MATCH,
    ADV_ERLANG_N,
    LSTLCLERRMSG=ADV_ERLANG_N
};

void init_errmsg( void );
