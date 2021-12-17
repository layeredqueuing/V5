/*
 *  $Id: srvn_input.h 15220 2021-12-15 15:18:47Z greg $
 *  libsrvnio2
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_SRVN_INPUT_H__
#define __LQIO_SRVN_INPUT_H__

#include "input.h"

#if defined(__cplusplus)
extern "C" {
#endif
    extern unsigned srvnlinenumber;		/* See srvn_scan.l, srvn_gram.y */
    extern int srvn_start_token;		/* */
	
    void * srvn_add_processor(const char *processor_name, scheduling_type scheduling_flag, void * cpu_quantum );
    void * srvn_add_group(const char *, void *, const char *, int cap );
    void * srvn_add_task(const char * task_name, const scheduling_type scheduling, const void * entries, const char * processor_name );
    void * srvn_add_entry(const char *, const void *);
    void * srvn_find_task( const char * );

    /* inout.c */

    void srvn_act_connect( const void *, void * src, void * dst );
    void srvn_add_activity_call( const char * src_task, const char * src_activity, const char * dst_entry, void * rate, const short );

    void * srvn_act_add_reply( const void *, const void *, void * );
    void srvn_act_add_reply_list( const void *, void *, void * );
    void * srvn_act_and_fork_list( const void *, void *, void * );
    void * srvn_act_and_join_list( const void *, void *, void *, void * );
    void * srvn_act_fork_item( const void *, void * );
    void * srvn_act_join_item( const void *, void * );
    void * srvn_act_loop_list( const void *, void *, void *, void * );
    void * srvn_act_or_fork_list( const void *, void *, void *, void * );
    void * srvn_act_or_join_list( const void *, void *, void * );

    void srvn_add_communication_delay(const char * from_proc, const char * to_proc, void * delay);
    void srvn_add_phase_call( const char * src_entry, int phase, const char * dst_entry, const void * rate, const short );

    void * srvn_get_task( const char * );
    void * srvn_get_entry( const char * );

    void srvn_set_histogram(void * entry_v, const unsigned phase, const double min, const double max, const int n_bins );
    void srvn_set_model_parameters(const char *model_comment, void * conv_val, void * it_limit, void * print_int, void * underrelax_coeff);
    void srvn_set_phase_type_flag(void * entry_v, unsigned n_phases, ...);
    void srvn_set_rwlock_flag( void * entry_v, int );
    void srvn_set_semaphore_flag( void * entry_v, int );
    void srvn_set_start_activity( void * entry_v, const char *);
    void srvn_set_proc_multiplicity( void * proc_v, void * );
    void srvn_set_proc_rate( void * proc_v, void * );
    void srvn_set_proc_replicas( void * proc_v, void * );
    void srvn_set_task_group( void * task_v, const char * );
    void srvn_set_task_multiplicity( void * task_v, void * );
    void srvn_set_task_priority( void * task_v, void * );
    void srvn_set_task_queue_length( void * task_v, void * );
    void srvn_set_task_replicas( void * task_v, void * );
    void srvn_set_task_think_time( void * task_v, void * );
    void srvn_set_task_tokens( void * task_v, int );
    void srvn_store_coeff_of_variation(void * entry_v, unsigned n_phases, ...);
    void srvn_store_entry_priority(void * entry_v, const int arg);
    void srvn_store_fanin( const char * src_name, const char * dst_name, void * value );
    void srvn_store_fanout( const char * src_name, const char * dst_name, void * value );
    void srvn_store_lqx_program_text(const char * program_text);
    void srvn_store_open_arrival_rate(void * entry_v, void * arg);
    void srvn_store_phase_service_time(void * entry_v, unsigned n_phases, ... );
    void srvn_store_phase_think_time(void *entry_v, unsigned n_phases, ... );
    void srvn_store_phase_xml_dom( const char * entry_name, int p ); 		/* BUG_469 */
    void srvn_store_prob_forward_data( void * from_entry, void * to_entry, void * prob );
    void srvn_store_queue_insertion(void * entry_v, const char *queue_name, double rate);
    void srvn_store_rnv_data(void * from_entry, void * to_entry, unsigned n_phases, ...);
    void srvn_store_snr_data(void * from_entry, void * to_entry, unsigned n_phases, ...);

    void * srvn_get_activity( void *, const char * );
    void * srvn_find_activity( void *, const char * );
    
    void srvn_set_activity_histogram( void *, const double min, const double max, const int n_bins );
    void srvn_set_activity_phase_type_flag( void *, const int );
    void srvn_set_activity_call_name( const void *, const void *, const void *, void * );
    void srvn_store_activity_coeff_of_variation( void *, void * ); 
    void * srvn_store_activity_rnv_data( void *, void *, void * ); 
    void srvn_store_activity_service_time( void *, void * ); 
    void * srvn_store_activity_snr_data( void *, void *, void * ); 
    void srvn_store_activity_think_time( void *, void * );


    void srvn_pragma(const char* pragmaText);
    void LQIO_error( const char * fmt, ... );
    void srvnwarning( const char * fmt, ... );


    void * srvn_int_constant( const int );
    void * srvn_real_constant( const double );
    void * srvn_variable( const char * );
    double srvn_get_infinity();
#if defined(__cplusplus)
}


namespace LQIO {
    namespace DOM {
	class Document;
    }
    namespace SRVN {
	bool load(DOM::Document&, const std::string&, bool);
    }
}


#endif
  
#endif /* __LQIO_SRVN_INPUT_H__ */
