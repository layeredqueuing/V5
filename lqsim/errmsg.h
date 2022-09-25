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
 * $Id: errmsg.h 15896 2022-09-23 18:03:32Z greg $
 */

#include <lqio/glblerr.h>
#include <vector>

enum {
    FTL_ACTIVITY_STACK_FULL=LQIO::LSTGBLERRMSG+1,
    FTL_MSG_POOL_EMPTY,
    ERR_REPLY_NOT_FOUND,
    ERR_CANNOT_CREATE_X,
    ERR_DELAY_MULTIPLY_DEFINED,
    ERR_INIT_DELAY,
    ERR_NO_QUANTUM_FOR_PS,
    ERR_QUANTUM_SPECIFIED_FOR_FIFO,
    ERR_SIGNAL_NO_WAIT,
    ERR_INITIALIZATION_FAILED,
    ADV_DEADLOCK,
    ADV_PRECISION,
    WRN_NO_PHASE_FOR_HISTOGRAM,
    WRN_NO_QUANTUM_FOR_PS,
    WRN_INVALID_PRIORITY
};

extern std::vector< std::pair<unsigned, LQIO::error_message_type> > local_error_messages;
