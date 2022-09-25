/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/*									*/
/* November 1995							*/
/************************************************************************/

/*
 * $Id: labels.cpp 15895 2022-09-23 17:21:55Z greg $
 */

#include "input.h"
#include "labels.h"

namespace LQIO {
    const char * cv_square_str			= "Squared coefficient of variation of execution segments:";
    const char * forwarding_probability_str	= "Forwarding probability from entry to entry:";
    const char * fwd_waiting_time_str		= "Mean delay for a forwarded rendezvous:";
    const char * fwd_waiting_time_variance_str	= "Variance of delay for a forwarded rendezvous:";
    const char * group_info_str			= "Group identifiers:";
    const char * histogram_str			= "Service time distributions for entries and activities:";
    const char * join_delay_str			= "Mean delay for a join:";
    const char * loss_probability_str		= "Arrival Loss Probabilities:";
    const char * max_service_time_str		= "Service time limit:";
    const char * mean_life_time_str		= "Mean life times";
    const char * open_arrival_rate_str		= "Open arrival rates per entry:";
    const char * open_wait_str			= "Waiting time at tasks with open arrivals:";
    const char * overtaking_prob_str		= "Overtaking Probabilities:";
    const char * phase_type_str			= "Phase type flags:";
    const char * processor_info_str		= "Processor identifiers and scheduling algorithms:";
    const char * rendezvous_rate_str		= "Mean number of rendezvous from entry to entry:";
    const char * rwlock_hold_time_str		= "RWLock holding times:";
    const char * rwlock_reader_hold_time_str	= "RWLock reader holding times:";
    const char * rwlock_reader_hold_variance_str= "RWLock reader holding time variance:";
    const char * rwlock_writer_hold_time_str	= "RWLock writer holding times:";
    const char * rwlock_writer_hold_variance_str= "RWLock writer holding time variance:";
    const char * semaphore_hold_time_str	= "Semaphore holding times:";
    const char * semaphore_hold_variance_str	= "Semaphore holding time variance:";
    const char * send_no_reply_rate_str		= "Mean number of non-blocking sends from entry to entry:";
    const char * service_demand_str		= "Entry execution demands:";
    const char * service_time_exceeded_str	= "Probability maximum service time exceeded:";		/*  */
    const char * service_time_str		= "Service times:";
    const char * snr_waiting_time_str		= "Mean delay for a send-no-reply request:";
    const char * snr_waiting_time_variance_str	= "Variance of delay for a send-no-reply request:";
    const char * task_info_str			= "Task information:";
    const char * think_time_str			= "Entry think times:";
    const char * throughput_bounds_str		= "Type 1 throughput bounds: ";
    const char * throughput_str			= "Throughputs and utilizations per phase:";
    const char * utilization_str		= "Utilization and waiting per phase for processor: ";
    const char * variance_str			= "Service time variance (per phase)\nand squared coefficient of variation (over all phases):";
    const char * waiting_time_str		= "Mean delay for a rendezvous:";
    const char * waiting_time_variance_str	= "Variance of delay for a rendezvous:";
}

/*
 * Scheduling types.
 */

const char * LQIO::SCHEDULE::CUSTOMER	= "ref";
const char * LQIO::SCHEDULE::DELAY	= "inf";
const char * LQIO::SCHEDULE::FIFO	= "fcfs";
const char * LQIO::SCHEDULE::LIFO	= "lcfs";
const char * LQIO::SCHEDULE::HOL	= "hol";
const char * LQIO::SCHEDULE::PPR	= "pri";
const char * LQIO::SCHEDULE::RAND	= "rand";
const char * LQIO::SCHEDULE::PS	    	= "ps";
const char * LQIO::SCHEDULE::POLL	= "poll";
const char * LQIO::SCHEDULE::BURST	= "burst";
const char * LQIO::SCHEDULE::UNIFORM	= "uniform";
const char * LQIO::SCHEDULE::SEMAPHORE  = "semaphore";
const char * LQIO::SCHEDULE::CFS	= "cfs";
const char * LQIO::SCHEDULE::RWLOCK	= "rwlock";

const std::map<const scheduling_type,const LQIO::SCHEDULE::label_t> scheduling_label = 
{
    { SCHEDULE_CUSTOMER,    { "CUST",	      LQIO::SCHEDULE::CUSTOMER,    'r' } },
    { SCHEDULE_DELAY,       { "DELAY",	      LQIO::SCHEDULE::DELAY,       'i' } },
    { SCHEDULE_FIFO,        { "FCFS",	      LQIO::SCHEDULE::FIFO,        'f' } },
    { SCHEDULE_LIFO,        { "LCFS",	      LQIO::SCHEDULE::LIFO,        'l' } },
    { SCHEDULE_HOL,         { "HOL",	      LQIO::SCHEDULE::HOL,         'h' } },
    { SCHEDULE_PPR,         { "PPR",	      LQIO::SCHEDULE::PPR,         'p' } },
    { SCHEDULE_RAND,        { "RAND",	      LQIO::SCHEDULE::RAND,        'r' } },
    { SCHEDULE_PS,          { "PS",	      LQIO::SCHEDULE::PS,          's' } },
    { SCHEDULE_POLL,        { "POLL",	      LQIO::SCHEDULE::POLL,        'P' } },	/* Task */
    { SCHEDULE_BURST,       { "BURST",	      LQIO::SCHEDULE::BURST,       'b' } },
    { SCHEDULE_UNIFORM,     { "UNIFORM",      LQIO::SCHEDULE::UNIFORM,     'u' } },
    { SCHEDULE_SEMAPHORE,   { "SEMAPHORE",    LQIO::SCHEDULE::SEMAPHORE,   'S' } },
    { SCHEDULE_CFS,         { "CFS",	      LQIO::SCHEDULE::CFS,         'c' } },
    { SCHEDULE_RWLOCK,      { "RWLOCK",	      LQIO::SCHEDULE::RWLOCK,      'w' } }
};
