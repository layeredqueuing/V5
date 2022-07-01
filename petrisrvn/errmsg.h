/*  -*- c++ -*-
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * Novmeber 1990.
 * August 1991.
 */

/*
 * $Id: errmsg.h 15738 2022-07-01 01:16:27Z greg $
 */

#include <lqio/glblerr.h>
#include <vector>

enum {
    FTL_TAG_TABLE_FULL=LQIO::LSTGBLERRMSG+1,
    ERR_DELAY_MULTIPLY_DEFINED,
    ERR_SEND_NO_REPLIES_PROHIBITED,
    ERR_BOGUS_REFERENCE_TASK,
    ERR_MULTI_SYNC_SERVER,
    ERR_COMMON_ENTRY_EXTERNAL_SYNC,
    WRN_CONVERGENCE,
    WRN_PREEMPTIVE_SCHEDULING,
    ADV_MESSAGES_LOST,
    ADV_OPEN_ARRIVALS_DONT_MATCH,
    ADV_ERLANG_N
};

extern std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages;
