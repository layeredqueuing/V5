/* -*- c++ -*-
 * $Id: glblerr.h 15609 2022-05-30 17:19:07Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 1995
 *
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 *----------------------------------------------------------------------
 */

#if     !defined(LQIO_GLBLERR_H)
#define LQIO_GLBLERR_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "error.h"

namespace LQIO {

#if defined(MSDOS) && defined(NO_ERROR)
#undef NO_ERROR
#endif

    extern struct error_message_type global_error_messages[];

    enum {
        FTL_INTERNAL_ERROR=1,
        FTL_NO_MEMORY,
        ERR_NO_OBJECT,
        ERR_NON_UNITY_REPLIES,
        ERR_DUPLICATE_SYMBOL,
        ERR_LQX_COMPILATION,
        ERR_LQX_EXECUTION,
	ERR_LQX_SPEX,
        ERR_CANT_OPEN_DIRECTORY,
        ERR_CANT_OPEN_FILE,
        ERR_CYCLE_IN_ACTIVITY_GRAPH,
        ERR_CYCLE_IN_CALL_GRAPH,
        ERR_TOO_MANY_PHASES,
	ERR_INVALID_DECISION_TYPE,
	ERR_INVALID_DECISION_PATH_TYPE,
        ERR_SRC_EQUALS_DST,
        ERR_MISSING_ATTRIBUTE,
	ERR_UNEXPECTED_ATTRIBUTE,
	ERR_INVALID_ARGUMENT,
        ERR_OPEN_AND_CLOSED_CLASSES,
        ERR_INVALID_FORWARDING_PROBABILITY,
        ERR_WRONG_TASK_FOR_ENTRY,
        ERR_ENTRY_NOT_SPECIFIED,
        ERR_REPLY_NOT_GENERATED,
        ERR_MIXED_SEMAPHORE_ENTRY_TYPES,
        ERR_MIXED_RWLOCK_ENTRY_TYPES,
        ERR_MIXED_ENTRY_TYPES,
        ERR_LQX_VARIABLE_RESOLUTION,
        ERR_LESS_ENTRIES_THAN_TASKS,
        ERR_NO_TASK_DEFINED_FOR_GROUP,
        ERR_INVALID_SHARE,
        ERR_NON_INTEGER,
        ERR_NON_INTEGER_EXPRESSION,
        ERR_INVALID_CALL_PARAMETER,
        ERR_INVALID_FWDING_PARAMETER,
        ERR_HISTOGRAM_INVALID_MIN,
        ERR_JOIN_BAD_PATH,
        ERR_INVALID_PROBABILITY,
        ERR_HISTOGRAM_INVALID_MAX,
        ERR_INVALID_PARAMETER,
        ERR_MULTIPLY_DEFINED,
        ERR_NO_GROUP_SPECIFIED,
        ERR_NO_QUANTUM_SCHEDULING,
        ERR_NO_REFERENCE_TASKS,
        ERR_NO_SEMAPHORE,
        ERR_NO_RWLOCK,
        ERR_NON_REF_THINK_TIME,
        ERR_NOT_SEMAPHORE_TASK,
        ERR_NOT_RWLOCK_TASK,
        ERR_TOO_MANY_X,
        ERR_MISSING_OR_BRANCH,
        ERR_PARSE_ERROR,
	ERR_DUPLICATE_X_LIST,
        ERR_INVALID_PROC_RATE,
        ERR_NO_GROUP_DEFINED_FOR_PROCESSOR,
        ERR_RANGE_ERROR,
        ERR_REF_TASK_FORWARDING,
        ERR_REFERENCE_TASK_OPEN_ARRIVALS,
        ERR_REFERENCE_TASK_IS_RECEIVER,
        ERR_REFERENCE_TASK_REPLIES,
        ERR_REFERENCE_TASK_IS_INFINITE,
        ERR_RWLOCK_CONCURRENT_READERS,
        ERR_ASYNC_REQUEST_TO_WAIT,
        ERR_ASYNC_REQUEST_TO_LOCKENTRIES,
        ERR_DUPLICATE_START_ACTIVITY,
        ERR_NOT_DEFINED,
        ERR_INFINITE_TASK,
        ERR_ENTRY_COUNT_FOR_TASK,
	ERR_NO_START_ACTIVITIES,
	ERR_NO_ENTRIES_DEFINED_FOR_TASK,
	ERR_IS_START_ACTIVITY,
        ERR_ACTIVITY_NOT_SPECIFIED,
	ERR_ACTIVITY_NOT_REACHABLE,
        ERR_DUPLICATE_REPLY,
        ERR_INVALID_REPLY,
        ERR_DUPLICATE_ACTIVITY_RVALUE,
        ERR_DUPLICATE_ACTIVITY_LVALUE,
        ERR_REPLY_SPECIFIED_FOR_SNR_ENTRY,
        ERR_JOIN_PATH_MISMATCH,
	ERR_NO_TASK_FOR_ENTRY,
        ADV_MESSAGES_DROPPED,
        ADV_LQX_IMPLICIT_SOLVE,
	ADV_SPEX_UNDEFINED_RESULT_VARIABLE,
	ADV_SPEX_UNUSED_RESULT_VARIABLE,
	ADV_TOO_MANY_GNUPLOT_VARIABLES,
        WRN_XXXX_TIME_DEFINED_BUT_ZERO,
        WRN_NO_CALLS_FOR,
        WRN_INFINITE_MULTI_SERVER,
        WRN_NOT_USED,
        WRN_SCHEDULING_NOT_SUPPORTED,
        WRN_NO_REQUESTS_TO_ENTRY,
        WRN_NO_SERVICE_TIME,
        WRN_ENTRY_TYPE_MISMATCH,
        WRN_NON_CFS_PROCESSOR,
        WRN_INVALID_SPEX_RESULT_PHASE,
	WRN_NO_SPEX_OBSERVATIONS,
        WRN_QUANTUM_SCHEDULING,
        WRN_DEFINED_NE_SPECIFIED_X,
	WRN_INFINITE_SERVER_OPEN_ARRIVALS,
	WRN_PRAGMA_UNKNOWN,
	WRN_PRAGMA_ARGUMENT_INVALID,
        WRN_MULTIPLE_SPECIFICATION,
        WRN_NO_TASKS_DEFINED_FOR_PROCESSOR,
        WRN_QUEUE_LENGTH,
        WRN_NO_SENDS_FROM_REF_TASK,
        WRN_TOO_MANY_ENTRIES_FOR_REF_TASK,
        WRN_PRIO_TASK_ON_FIFO_PROC,
	ERR_NOT_SUPPORTED,
        LSTGBLERRMSG=ERR_NOT_SUPPORTED                     /* Define LAST! */
    };
}
#endif
