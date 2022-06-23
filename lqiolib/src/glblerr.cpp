/* -*- c++ -*-
 * $Id: glblerr.cpp 15706 2022-06-23 17:02:35Z greg $
 *
 * Error messages common to solvers.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December, 1995
 *
 * ----------------------------------------------------------------------
 */

#include "glblerr.h"

namespace LQIO {

    struct error_message_type global_error_messages[LSTGBLERRMSG+1] = {
        { error_severity::ERROR,    "** NO ERROR **" },
        { error_severity::FATAL,    "Internal error." },                                                                                        /* FTL_INTERNAL_ERROR                   */
        { error_severity::FATAL,    "No more memory." },                                                                                        /* FTL_NO_MEMORY                        */
        { error_severity::FATAL,    "Model has no %s." },                                                                                       /* ERR_NO_OBJECT                        */
        { error_severity::ERROR,    "%s \"%s\" previously defined." },                                                                          /* ERR_DUPLICATE_SYMBOL                 */
        { error_severity::ERROR,    "An error occurred while compiling the LQX program found in file: %s." },                                   /* ERR_LQX_COMPILATION                  */
        { error_severity::ERROR,    "An error occurred executing the LQX program found in file: %s." },                                         /* ERR_LQX_EXECUTION                    */
        { error_severity::ERROR,    "Both LQX and SPEX found in file %s.  Use one or the other." },                                             /* ERR_LQX_SPEX                         */
        { error_severity::ERROR,    "Cannot open directory \"%s\" -- %s." },                                                                    /* ERR_CANT_OPEN_DIRECTORY,             */
        { error_severity::ERROR,    "Cannot open file \"%s\" -- %s." },                                                                         /* ERR_CANT_OPEN_FILE,                  */
        { error_severity::ERROR,    "Cycle in activity graph for task \"%s\", backtrace is \"%s\"." },                                          /* ERR_CYCLE_IN_ACTIVITY_GRAPH          */
        { error_severity::ERROR,    "Cycle in call graph,  backtrace is \"%s\"." },                                                             /* ERR_CYCLE_IN_CALL_GRAPH              */
        { error_severity::ERROR,    "Decision %s has a decision type %s, which is not supported in this version." },                            /* ERR_INVALID_DECISION_TYPE            */
        { error_severity::ERROR,    "Destination entry \"%s\" must be different from source entry \"%s\"." },                                   /* ERR_SRC_EQUALS_DST                   */
        { error_severity::ERROR,    "Element \"%s\", missing attribute: \"%s\"."},                                                              /* ERR_MISSING_ATTRIBUTE                */
        { error_severity::ERROR,    "Element \"%s\", unexpected attribute: \"%s\"." },                                                          /* ERR_UNEXPECTED_ATTRIBUTE             */
        { error_severity::ERROR,    "Element \"%s\", invalid argument to attribute: \"%s\"." },                                                 /* ERR_INVALID_ARGUMENT                 */
        { error_severity::ERROR,    "Entry \"%s\" accepts both rendezvous and send-no-reply messages." },                                       /* ERR_OPEN_AND_CLOSED_CLASSES          */
        { error_severity::ERROR,    "Entry \"%s\" generates 4.2f replies."  },                                                                  /* ERR_NON_UNITY_REPLIES                */
        { error_severity::ERROR,    "Entry \"%s\" has invalid forwarding probability of %g." },                                                 /* ERR_INVALID_FORWARDING_PROBABILITY   */
        { error_severity::ERROR,    "Entry \"%s\" is not part of task \"%s\"."},                                                                /* ERR_WRONG_TASK_FOR_ENTRY             */
        { error_severity::ERROR,    "Entry \"%s\" is not specified." },                                                                         /* ERR_ENTRY_NOT_SPECIFIED              */
        { error_severity::ERROR,    "Entry \"%s\" must reply; the reply is not specified in the activity graph." },                             /* ERR_REPLY_NOT_GENERATED              */
        { error_severity::ERROR,    "Entry \"%s\" specified as both a lock and a unlock." },                                                    /* ERR_MIXED_RWLOCK_ENTRY_TYPES         */
        { error_severity::ERROR,    "Entry \"%s\" specified as both a signal and a wait." },                                                    /* ERR_MIXED_SEMAPHORE_ENTRY_TYPES      */
        { error_severity::ERROR,    "Entry \"%s\" specified using both activity and phase methods." },                                          /* ERR_MIXED_ENTRY_TYPES                */
        { error_severity::ERROR,    "External variables are present in file \"%s\", but there is no LQX program to resolve them." },            /* ERR_LQX_VARIABLE_RESOLUTION          */
        { error_severity::ERROR,    "Group \"%s\" has invalid share of %g."},                                                                   /* ERR_INVALID_SHARE                    */
        { error_severity::ERROR,    "Invalid calls from %s \"%s\" %s \"%s\" to entry \"%s\": %s." },                                            /* ERR_INVALID_CALL_PARAMETER           */
        { error_severity::ERROR,    "Invalid forwarding from entry \"%s\" to entry \"%s\": %s." },                                              /* ERR_INVALID_FWDING_PARAMETER         */
        { error_severity::ERROR,    "Invalid lower range value for histogram of %g." },                                                         /* ERR_HISTOGRAM_INVALID_MIN            */
        { error_severity::ERROR,    "Invalid probability of %g." },                                                                             /* ERR_INVALID_PROBABILITY              */
        { error_severity::ERROR,    "Invalid upper range value for histogram of %g." },                                                         /* ERR_HISTOGRAM_INVALID_MAX            */
        { error_severity::ERROR,    "Invalid %s for %s \"%s\": %s." },                                                                          /* ERR_INVALID_PARAMETER                */
        { error_severity::ERROR,    "No group specified for task \"%s\" running on processor \"%s\" using fair share scheduling." },            /* ERR_NO_GROUP_SPECIFIED               */
        { error_severity::ERROR,    "No quantum is specified for processor \"%s\" with  \"%s\" scheduling."},                                   /* ERR_NO_QUANTUM_SCHEDULING            */
        { error_severity::ERROR,    "No reference tasks have been specified in this model." },                                                  /* ERR_NO_REFERENCE_TASKS               */
        { error_severity::ERROR,    "No signal or wait specified for semaphore task \"%s\"." },                                                 /* ERR_NO_SEMAPHORE                     */
        { error_severity::ERROR,    "No lock or unlock specified for rwlock task \"%s\"." },                                                    /* ERR_NO_RWLOCK                        */
        { error_severity::ERROR,    "Non-reference task \"%s\" cannot have think time." },                                                      /* ERR_NON_REF_THINK_TIME               */
        { error_severity::ERROR,    "Non-semaphore task \"%s\" cannot have a %s for entry \"%s\"." },                                           /* ERR_NOT_SEMAPHORE_TASK               */
        { error_severity::ERROR,    "Non-rwlock task \"%s\" cannot have a %s for entry \"%s\"." },                                              /* ERR_NOT_RWLOCK_TASK                  */
        { error_severity::ERROR,    "Number of %s is outside of program limits of (1,%d)." },                                                   /* ERR_TOO_MANY_X                       */
        { error_severity::ERROR,    "OR-Fork \"%s\" for task \"%s\" branch probabilities do not sum to 1.0; sum is %4.2f." },            	/* ERR_OR_BRANCH_PROBABILITIES          */
        { error_severity::ERROR,    "Precedence \"%s\" list previously defined." },                                                             /* ERR_DUPLICATE_X_LIST                 */
        { error_severity::ERROR,    "Reference task \"%s\", entry \"%s\" cannot forward requests." },                                           /* ERR_REF_TASK_FORWARDING              */
        { error_severity::ERROR,    "Reference task \"%s\", entry \"%s\" cannot have open arrival stream." },                                   /* ERR_REFERENCE_TASK_OPEN_ARRIVALS     */
        { error_severity::ERROR,    "Reference task \"%s\", entry \"%s\" receives requests." },                                                 /* ERR_REFERENCE_TASK_IS_RECEIVER       */
        { error_severity::ERROR,    "Reference task \"%s\", replies to entry \"%s\" from activity \"%s\"." },                                   /* ERR_REFERENCE_TASK_REPLIES           */
        { error_severity::ERROR,    "Reference task \"%s\" must have a finite number of copies." },                                             /* ERR_REFERENCE_TASK_IS_INFINITE       */
        { error_severity::ERROR,    "Semaphore \"wait\" entry \"%s\" cannot accept send-no-reply requests." },                                  /* ERR_ASYNC_REQUEST_TO_WAIT            */
        { error_severity::ERROR,    "Start activity for entry \"%s\" is already defined.  Activity \"%s\" is a duplicate." },                   /* ERR_DUPLICATE_START_ACTIVITY         */
        { error_severity::ERROR,    "Symbol \"%s\" not previously defined." },                                                                  /* ERR_NOT_DEFINED                      */
        { error_severity::ERROR,    "Task \"%s\" cannot be an infinite server." },                                                              /* ERR_INFINITE_TASK                    */
        { error_severity::ERROR,    "Task \"%s\" has %d entries defined, exactly %d are required." },                                           /* ERR_ENTRY_COUNT_FOR_TASK             */
        { error_severity::ERROR,    "Task \"%s\" has activities but none are reachable." },                                                     /* ERR_NO_START_ACTIVITIES              */
        { error_severity::ERROR,    "Task \"%s\" has no entries." },                                                                            /* ERR_NO_ENTRIES_DEFINED_FOR_TASK      */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" is a start activity." },                                                      /* ERR_IS_START_ACTIVITY                */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" is not specified."},                                                          /* ERR_ACTIVITY_NOT_SPECIFIED           */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" is not reachable."},                                                          /* ERR_ACTIVITY_NOT_REACHABLE           */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" makes a duplicate reply for entry \"%s\"." },                                 /* ERR_DUPLICATE_REPLY                  */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" makes invalid reply for entry \"%s\"." },                                     /* ERR_INVALID_REPLY                    */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" previously used in the fork at line %d." },                                   /* ERR_DUPLICATE_ACTIVITY_RVALUE        */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" previously used in the join at line %d." },                                   /* ERR_DUPLICATE_ACTIVITY_LVALUE        */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" replies to entry \"%s\" which does not accept rendezvous requests." },        /* ERR_REPLY_SPECIFIED_FOR_SNR_ENTRY    */
        { error_severity::ERROR,    "Task \"%s\", activity \"%s\" is not reachable, join is \"%s\"." },                                         /* ERR_JOIN_BAD_PATH                    */
        { error_severity::ERROR,    "Task \"%s\", join \"%s\" does not match fork \"%s\"." },                                                   /* ERR_JOIN_PATH_MISMATCH               */
        { error_severity::ERROR,    "Task for entry \"%s\" has not been defined." },                                                            /* ERR_NO_TASK_FOR_ENTRY                */
        { error_severity::ADVISORY, "Messages dropped at task \"%s\" for open-class queues." },                                                 /* ADV_MESSAGES_DROPPED                 */
        { error_severity::ADVISORY, "No solve() call found in the lqx program in file: %s.  solve() was invoked implicitly." },                 /* ADV_LQX_IMPLICIT_SOLVE               */
        { error_severity::ADVISORY, "SPEX result variable \"%s\" was not used as an array or in an observation." },                             /* ADV_SPEX_UNUSED_RESULT_VARIABLE      */
        { error_severity::ADVISORY, "Too many dependent variables to plot from \"%s\" onwards." },                                              /* ADV_TOO_MANY_GNUPLOT_VARIABLES       */
        { error_severity::WARNING,  "%s \"%s\" is an infinite server with a multiplicity of %d." },                                             /* WRN_INFINITE_MULTI_SERVER            */
        { error_severity::WARNING,  "%s \"%s\" is not used." },                                                                                 /* WRN_NOT_USED                         */
        { error_severity::WARNING,  "%s \"%s\", %s \"%s\" has %s time defined, but its value is zero." },                                       /* WRN_XXXX_TIME_DEFINED_BUT_ZERO       */
        { error_severity::WARNING,  "%s scheduling specified for %s \"%s\" is not supported." },                                                /* WRN_SCHEDULING_NOT_SUPPORTED         */
        { error_severity::WARNING,  "Entry \"%s\" attribute type=\"%s\" - entry type should be \"%s\"." },                                      /* WRN_ENTRY_TYPE_MISMATCH              */
        { error_severity::WARNING,  "Entry \"%s\" does not receive any requests." },                                                            /* WRN_NO_REQUESTS_TO_ENTRY             */
        { error_severity::WARNING,  "Entry \"%s\" has no service time specified for any phase." },                                              /* WRN_NO_SERVICE_TIME                  */
        { error_severity::WARNING,  "Group \"%s\" specified for processor \"%s\" which is not running fair share scheduling." },                /* WRN_NON_CFS_PROCESSOR                */
        { error_severity::WARNING,  "Infinite server \"%s\" accepts either asynchronous messages or open arrivals." },                          /* WRN_INFINITE_SERVER_OPEN_ARRIVALS    */
        { error_severity::WARNING,  "Invalid phase, %d, specified for SPEX result \"%%%s\" for entry \"%s\"." },                                /* WRN_INVALID_SPEX_RESULT_PHASE        */
        { error_severity::WARNING,  "No SPEX Observations were specified in the input model." },                                                /* WRN_NO_SPEX_OBSERVATIONS             */
        { error_severity::WARNING,  "Parameter is specified multiple times." },                                                                 /* WRN_MULTIPLE_SPECIFICATION           */
        { error_severity::WARNING,  "Pragma \"%s\" is not recognized." },                                                                       /* WRN_PRAGMA_UNKNOWN                   */
        { error_severity::WARNING,  "Pragma \"%s\": invalid argument \"%s\"." },                                                                /* WRN_PRAGMA_ARGUMENT_INVALID          */
        { error_severity::WARNING,  "Processor \"%s\" has no tasks." },                                                                         /* WRN_NO_TASKS_DEFINED_FOR_PROCESSOR   */
        { error_severity::WARNING,  "Processor \"%s\" using \"%s\" scheduling has a non-zero quantum specified." },                             /* WRN_QUANTUM_SCHEDULING               */
        { error_severity::WARNING,  "Queue length parameter not supported for task \"%s\"." },                                                  /* WRN_QUEUE_LENGTH                     */
        { error_severity::WARNING,  "Reference task \"%s\" does not send any messages." },                                                      /* WRN_NO_SENDS_FROM_REF_TASK           */
        { error_severity::WARNING,  "Reference task \"%s\" has more than one entry defined." },                                                 /* WRN_TOO_MANY_ENTRIES_FOR_REF_TASK    */
        { error_severity::WARNING,  "Task \"%s\" with priority is running on processor \"%s\" which does not have priority scheduling." },      /* WRN_PRIO_TASK_ON_FIFO_PROC           */
        { error_severity::ERROR,    "The %s feature is not supported in this version." },                                                       /* ERR_NOT_SUPPORTED                    */
    };                                                                                                                                                                     
}
