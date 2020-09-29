/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 1991.                                                         */
/* March 2010.								*/
/************************************************************************/

/*
 * $Id: labels.h 13887 2020-09-28 22:04:18Z greg $
 */

#if	!defined(LQIO_LABELS_H)
#define	LQIO_LABELS_H

namespace LQIO {

    /* Labels for input records */

    extern const char * cv_square_str;
    extern const char * forwarding_probability_str;
    extern const char * max_service_time_str;
    extern const char * open_arrival_rate_str;
    extern const char * phase_type_str;
    extern const char * processor_info_str;
    extern const char * group_info_str;
    extern const char * rendezvous_rate_str;
    extern const char * send_no_reply_rate_str;
    extern const char * service_demand_str;
    extern const char * task_info_str;
    extern const char * think_time_str;
    extern const char * decision_info_str;
    extern const char * decision_path_info_str;

    /* Labels for output records */

    extern const char * abort_prob_str;
    extern const char * abort_prob_variance_str;
    extern const char * fwd_waiting_time_str;
    extern const char * fwd_waiting_time_variance_str;
    extern const char * histogram_str;
    extern const char * join_delay_str;
    extern const char * loss_probability_str;
    extern const char * mean_life_time_str;
    extern const char * number_of_retries_str;
    extern const char * number_of_retries_variance_str;
    extern const char * open_wait_str;
    extern const char * overtaking_prob_str;
    extern const char * rwlock_hold_time_str;
    extern const char * rwlock_reader_hold_time_str;
    extern const char * rwlock_reader_hold_variance_str;
    extern const char * rwlock_writer_hold_time_str;
    extern const char * rwlock_writer_hold_variance_str;
    extern const char * semaphore_hold_time_str;
    extern const char * semaphore_hold_variance_str;
    extern const char * service_time_exceeded_str;
    extern const char * service_time_str;
    extern const char * snr_waiting_time_str;
    extern const char * snr_waiting_time_variance_str;
    extern const char * success_prob_str;
    extern const char * success_prob_variance_str;
    extern const char * throughput_bounds_str;
    extern const char * throughput_str;
    extern const char * timeout_prob_str;
    extern const char * timeout_prob_variance_str;
    extern const char * utilization_str;
    extern const char * variance_str;
    extern const char * waiting_time_str;
    extern const char * waiting_time_variance_str;
}
    
#endif
