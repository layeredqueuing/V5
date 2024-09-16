/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/* August 1991.								*/
/************************************************************************/

/*
 * $Id: errmsg.h 17290 2024-09-16 16:10:54Z greg $
 */

#include <lqio/glblerr.h>
#include <vector>

enum {
    FTL_ACTIVITY_STACK_FULL=LQIO::LSTGBLERRMSG+1,
    ERR_CANNOT_CREATE_X,
    ERR_DELAY_MULTIPLY_DEFINED,
    ERR_INITIALIZATION_FAILED,
    ERR_INIT_DELAY,
    ERR_MSG_POOL_EMPTY,
    ERR_NO_QUANTUM_FOR_PS,
    ERR_REPLY_NOT_FOUND,
    ERR_SIGNAL_NO_WAIT,
    ADV_DEADLOCK,
    ADV_PRECISION,
    WRN_NO_PHASE_FOR_HISTOGRAM,
    WRN_INVALID_PRIORITY
};

extern std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages;
