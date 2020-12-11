/*
 *  $Id: dom_pragma.cpp 14201 2020-12-10 16:33:17Z greg $
 *
 *  Created by Martin Mroz on 16/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>
#include <cstdlib>
#include <algorithm>
#include "dom_pragma.h"
#include "labels.h"
#include "input.h"
#include "glblerr.h"

namespace LQIO {
    namespace DOM {
	std::map<std::string,std::set<std::string>*> Pragma::__pragmas;

	Pragma::Pragma() : _loadedPragmas()
	{
	    initialize();
	}

	void Pragma::initialize()
	{
	    if ( __pragmas.size() != 0 ) return;

	    /* Number */
	    
	    __pragmas[Pragma::_block_period_] =     nullptr;		/* lqsim */
	    __pragmas[Pragma::_initial_delay_] =    nullptr;		/* lqsim */
	    __pragmas[Pragma::_initial_loops_] =    nullptr;		/* lqsim */
	    __pragmas[Pragma::_max_blocks_] = 	    nullptr;		/* lqsim */
	    __pragmas[Pragma::_nice_] =             nullptr;		/* lqsim */
	    __pragmas[Pragma::_precision_] = 	    nullptr;		/* lqsim */
	    __pragmas[Pragma::_run_time_] = 	    nullptr;		/* lqsim */
	    __pragmas[Pragma::_seed_value_] = 	    nullptr;		/* lqsim */
	    __pragmas[Pragma::_stop_on_bogus_utilization_] = nullptr;	/* lqns */
	    __pragmas[Pragma::_tau_] =              nullptr;		/* lqns */
	    __pragmas[Pragma::_underrelaxation_] =  nullptr;		/* lqns */

	    /* Boolean */
	    
	    static std::set<std::string> true_false_arg;
	    true_false_arg.insert("true");
	    true_false_arg.insert("false");
	    true_false_arg.insert(Pragma::_yes_);
	    true_false_arg.insert(Pragma::_no_);
	    __pragmas[Pragma::_cycles_] = &true_false_arg;			/* lqns */
	    __pragmas[Pragma::_interlocking_] = &true_false_arg;		/* lqns */
	    __pragmas[Pragma::_prune_] = &true_false_arg;			// BUG_270
	    __pragmas[Pragma::_quorum_reply_] =	&true_false_arg;		/* lqsim */
	    __pragmas[Pragma::_reschedule_on_async_send_] = &true_false_arg;
	    __pragmas[Pragma::_spex_header_] = &true_false_arg;
	    __pragmas[Pragma::_stop_on_message_loss_] = &true_false_arg;
	    
	    /* string */
	    	
	    static std::set<std::string> force_multiserver_args;		/* lqns */
	    force_multiserver_args.insert(Pragma::_none_);
	    force_multiserver_args.insert(Pragma::_processors_);
	    force_multiserver_args.insert(Pragma::_tasks_);
	    force_multiserver_args.insert(Pragma::_all_);
	    __pragmas[Pragma::_force_multiserver_] = &force_multiserver_args;	
	    
	    static std::set<std::string> layering_args;		/* lqns */
	    layering_args.insert(Pragma::_batched_);
	    layering_args.insert(Pragma::_batched_back_);
	    layering_args.insert(Pragma::_hwsw_);
	    layering_args.insert(Pragma::_mol_);
	    layering_args.insert(Pragma::_mol_back_);
	    layering_args.insert(Pragma::_squashed_);
	    layering_args.insert(Pragma::_srvn_);
	    __pragmas[_layering_] = &layering_args;
	    
	    static std::set<std::string> multiserver_args;	/* lqns */
	    multiserver_args.insert(Pragma::_bruell_);
	    multiserver_args.insert(Pragma::_conway_);
	    multiserver_args.insert(Pragma::_default_);
	    multiserver_args.insert(Pragma::_reiser_);
	    multiserver_args.insert(Pragma::_reiser_ps_);
	    multiserver_args.insert(Pragma::_rolia_);
	    multiserver_args.insert(Pragma::_rolia_ps_);
	    multiserver_args.insert(Pragma::_schmidt_);
	    multiserver_args.insert(Pragma::_suri_);
	    __pragmas[_multiserver_] = &multiserver_args;

	    static std::set<std::string> mva_args;
	    mva_args.insert(Pragma::_linearizer_);
	    mva_args.insert(Pragma::_exact_);
	    mva_args.insert(Pragma::_schweitzer_);
	    mva_args.insert(Pragma::_fast_);
	    mva_args.insert(Pragma::_one_step_);
	    mva_args.insert(Pragma::_one_step_linearizer_);
	    __pragmas[_mva_] = &mva_args;

	    static std::set<std::string> overtaking_args;
	    overtaking_args.insert(Pragma::_markov_);
	    overtaking_args.insert(Pragma::_rolia_);
	    overtaking_args.insert(Pragma::_simple_);
	    overtaking_args.insert(Pragma::_special_);
	    overtaking_args.insert(Pragma::_none_);
	    __pragmas[Pragma::_overtaking_] = &overtaking_args;

	    static std::set<std::string> processor_args;
	    processor_args.insert(Pragma::_default_);
	    processor_args.insert(scheduling_label[SCHEDULE_DELAY].XML);
	    processor_args.insert(scheduling_label[SCHEDULE_FIFO].XML);
	    processor_args.insert(scheduling_label[SCHEDULE_HOL].XML);
	    processor_args.insert(scheduling_label[SCHEDULE_PPR].XML);
	    processor_args.insert(scheduling_label[SCHEDULE_PS].XML);
	    processor_args.insert(scheduling_label[SCHEDULE_RAND].XML);
	    __pragmas[Pragma::_processor_scheduling_] = &processor_args;
	    
	    static std::set<std::string> quorum_distribution_args;
	    quorum_distribution_args.insert(Pragma::_threepoint_);
	    quorum_distribution_args.insert(Pragma::_gamma_);
	    quorum_distribution_args.insert(Pragma::_geometric_);
	    quorum_distribution_args.insert(Pragma::_deterministic_);
	    __pragmas[Pragma::_quorum_distribution_] = &quorum_distribution_args;

	    static std::set<std::string> quorum_delayed_calls_args;
	    quorum_delayed_calls_args.insert(Pragma::_keep_all_);
	    quorum_delayed_calls_args.insert(Pragma::_abort_all_);
	    quorum_delayed_calls_args.insert(Pragma::_abort_local_);
	    quorum_delayed_calls_args.insert(Pragma::_abort_remote_);
	    __pragmas[Pragma::_quorum_delayed_calls_] = &quorum_delayed_calls_args;

	    static std::set<std::string> quorum_idle_time_args;
	    quorum_idle_time_args.insert(Pragma::_default_);
	    quorum_idle_time_args.insert(Pragma::_join_delay_);
	    __pragmas[Pragma::_quorum_idle_time_] = &quorum_delayed_calls_args;

	    static std::set<std::string> scheduling_model_args;
	    scheduling_model_args.insert(Pragma::_default_);
	    scheduling_model_args.insert(Pragma::_default_natural_);
	    scheduling_model_args.insert(Pragma::_custom_);
	    scheduling_model_args.insert(Pragma::_custom_natural_);
	    __pragmas[Pragma::_scheduling_model_] = &scheduling_model_args;

	    static std::set<std::string> task_args;
	    task_args.insert(Pragma::_default_);
	    task_args.insert(scheduling_label[SCHEDULE_DELAY].XML);
	    task_args.insert(scheduling_label[SCHEDULE_FIFO].XML);
	    task_args.insert(scheduling_label[SCHEDULE_HOL].XML);
	    task_args.insert(scheduling_label[SCHEDULE_RAND].XML);
	    __pragmas[Pragma::_task_scheduling_] = &task_args;
	    
	    static std::set<std::string> threads_args;
	    threads_args.insert(Pragma::_hyper_);
	    threads_args.insert(Pragma::_mak_);
	    threads_args.insert(Pragma::_none_);
	    threads_args.insert(Pragma::_exponential_);
	    __pragmas[Pragma::_threads_] = &threads_args;

	    static std::set<std::string> variance_args;
	    variance_args.insert(Pragma::_default_);
	    variance_args.insert(Pragma::_none_);
	    variance_args.insert(Pragma::_stochastic_);
	    variance_args.insert(Pragma::_mol_);
	    variance_args.insert(Pragma::_no_entry_);
	    variance_args.insert(Pragma::_init_only_);
	    __pragmas["variance"] = &variance_args;

	    static std::set<std::string> warning_args;
	    warning_args.insert(Pragma::_all_);
	    warning_args.insert(Pragma::_warning_);
	    warning_args.insert(Pragma::_advisory_);
	    warning_args.insert(Pragma::_run_time_);
	    __pragmas["severity-level"] = &warning_args;
	}

	bool Pragma::insert(const std::string& param,const std::string& value)
	{
	    if ( check( param, value ) ) {
		_loadedPragmas[param] = value;		/* Overwrite */
		return true;
	    } else {
		return false;
	    }
	}

	/*
	 * Insert a list of the form pragma = value, pramga = value...
	 */
	
	bool Pragma::insert( const char * p )
	{
	    if ( !p ) return false;

	    bool rc = true;
	    do { 
		while ( isspace( *p ) ) ++p;            /* Skip leading whitespace. */
		std::string param;
		std::string value;
		while ( *p && !isspace( *p ) && *p != '=' && *p != ',' ) {
		    param += *p++;                      /* get parameter */
		}
		while ( isspace( *p ) ) ++p;
		if ( *p == '=' ) {
		    ++p;
		    while ( isspace( *p ) ) ++p;
		    while ( *p && !isspace( *p ) && *p != ',' ) {
			value += *p++;
		    }
		}
		while ( isspace( *p ) ) ++p;
		if ( !insert( param, value ) ) {
		    rc = false;
		}
	    } while ( *p++ == ',' );
	    return rc;
	}


	void Pragma::merge( const std::map<std::string,std::string>& list )
	{
	    for_each( list.begin(), list.end(), Merge(*this) );
	}

	
	const std::map<std::string,std::string>& Pragma::getList() const
	{
	    return _loadedPragmas;
	}

	const std::string Pragma::get( const std::string& s ) const
	{
	    std::map<std::string,std::string>::const_iterator iter = _loadedPragmas.find( s );
	    if ( iter != _loadedPragmas.end() ) {
		return iter->second;
	    } else {
		return std::string("");
	    }
	}
	
	const std::set<std::string>* Pragma::getValues( const std::string& s )
	{
	    const std::map<std::string,std::set<std::string>*>::const_iterator i = __pragmas.find(s);
	    if ( i != __pragmas.end() ) {
		return i->second;
	    } else {
		return nullptr;
	    }
	}

	void Pragma::clear()
	{
	    _loadedPragmas.clear();
	}


	bool Pragma::check( const std::string& param, const std::string& value ) const
	{
	    std::map<std::string,std::set<std::string>*>::const_iterator i = __pragmas.find( param );
	    if ( i == __pragmas.end() ) {
		LQIO::solution_error( LQIO::WRN_PRAGMA_UNKNOWN, param.c_str() );
		return false;
	    } else if ( i->second != nullptr ) {
		const std::set<std::string>& values = *i->second;
		std::set<std::string>::const_iterator j = values.find( value );
		if ( j == values.end() ) {
		    LQIO::solution_error( LQIO::WRN_PRAGMA_ARGUMENT_INVALID, param.c_str(), value.c_str() );
		    return false;
		}
	    } else {
		char * endptr = nullptr;
		strtod( value.c_str(), &endptr );
		if ( *endptr != '\0' ) {
		    LQIO::solution_error( LQIO::WRN_PRAGMA_ARGUMENT_INVALID, param.c_str(), value.c_str() );
		    return false;
		}
	    }
	    return true;
	}

	/* Labels */
	
	const char * Pragma::_abort_all_ = 			"abort-all";		// Quorum
	const char * Pragma::_abort_local_ = 			"abort-local";		// Quorum
	const char * Pragma::_abort_remote_ = 			"abort-remote";		// Quorum
	const char * Pragma::_advisory_ =			"advisory";
	const char * Pragma::_all_ =				"all";
	const char * Pragma::_batched_ =			"batched";
	const char * Pragma::_batched_back_ =			"batched-back";
	const char * Pragma::_block_period_ =			"block-period";
	const char * Pragma::_bruell_ =				"bruell";
	const char * Pragma::_conway_ =				"conway";
	const char * Pragma::_custom_ = 		    	"custom";
	const char * Pragma::_custom_natural_ =		    	"custom-natural";
	const char * Pragma::_cycles_ =				"cycles";
	const char * Pragma::_default_ =			"default";
	const char * Pragma::_default_natural_ = 		"default-natural";
	const char * Pragma::_deterministic_ = 			"deterministic";	// Quorum
	const char * Pragma::_exact_ =				"exact";
	const char * Pragma::_exponential_ =			"exponential";
	const char * Pragma::_false_ =				"false";
	const char * Pragma::_fast_ =				"fast";
	const char * Pragma::_force_multiserver_ =		"force-multiserver";
	const char * Pragma::_gamma_ = 				"gamma";		// Quorum
	const char * Pragma::_geometric_ = 			"geometric";		// Quorum
	const char * Pragma::_hwsw_ =				"hwsw";
	const char * Pragma::_hyper_ =				"hyper";
	const char * Pragma::_init_only_ =			"init-only";
	const char * Pragma::_initial_delay_ =			"initial-delay";
	const char * Pragma::_initial_loops_ =			"initial-loops";
	const char * Pragma::_interlocking_ =			"interlocking";
	const char * Pragma::_join_delay_ =			"join-delay";		// Quorum
	const char * Pragma::_keep_all_ = 			"keep-all";		// Quorum
	const char * Pragma::_layering_ = 			"layering";
	const char * Pragma::_linearizer_ =			"linearizer";
	const char * Pragma::_mak_ =				"mak";
	const char * Pragma::_markov_ =				"markov";
	const char * Pragma::_max_blocks_ =			"max-blocks";
	const char * Pragma::_mol_ =				"mol";
	const char * Pragma::_mol_back_ =			"mol-back";
	const char * Pragma::_multiserver_ = 			"multiserver";
	const char * Pragma::_mva_ = 				"mva";
	const char * Pragma::_nice_ =				"nice";
	const char * Pragma::_no_ =				"no";
	const char * Pragma::_no_entry_ =			"no-entry";
	const char * Pragma::_none_ =				"none";
	const char * Pragma::_one_step_ =			"one-step";
	const char * Pragma::_one_step_linearizer_ =		"one-step-linearizer";
	const char * Pragma::_overtaking_ =			"overtaking";
	const char * Pragma::_precision_ =			"precision";
	const char * Pragma::_processor_scheduling_ =		"processor-scheduling";
	const char * Pragma::_processors_ =			"processors";
	const char * Pragma::_prune_ =				"prune";
	const char * Pragma::_quorum_delayed_calls_ =		"quorum-delayed-calls";	// Quroum
	const char * Pragma::_quorum_distribution_ = 		"quorum-distribution";	// Quroum
	const char * Pragma::_quorum_idle_time_ = 		"quorum-idle-time";	// Quroum
	const char * Pragma::_quorum_reply_ =			"quorum-reply";		// Quorum
	const char * Pragma::_reiser_ =				"reiser";
	const char * Pragma::_reiser_ps_ =			"reiser-ps";
	const char * Pragma::_reschedule_on_async_send_ =	"reschedule-on-async-send";
	const char * Pragma::_rolia_ =				"rolia";
	const char * Pragma::_rolia_ps_ =			"rolia-ps";
	const char * Pragma::_run_time_ =			"run-time";
	const char * Pragma::_scheduling_model_ = 		"scheduling-model";
	const char * Pragma::_schmidt_ =			"schmidt";
	const char * Pragma::_schweitzer_ =			"schweitzer";
	const char * Pragma::_seed_value_ =			"seed-value";
	const char * Pragma::_severity_level_ = 		"severity-level";
	const char * Pragma::_simple_ =				"simple";
	const char * Pragma::_special_ =			"special";
	const char * Pragma::_spex_header_ =			"spex-header";
	const char * Pragma::_squashed_ =			"squashed";
	const char * Pragma::_srvn_ =				"srvn";
	const char * Pragma::_stochastic_ =			"stochastic";
	const char * Pragma::_stop_on_bogus_utilization_ =	"stop-on-bogus-utilization";
	const char * Pragma::_stop_on_message_loss_ =		"stop-on-message-loss";
	const char * Pragma::_suri_ =				"suri";
	const char * Pragma::_task_scheduling_ =		"task-scheduling";
	const char * Pragma::_tasks_ =				"tasks";
	const char * Pragma::_tau_ =				"tau";
	const char * Pragma::_threads_ =			"threads";
	const char * Pragma::_threepoint_ = 			"three-point";		// Quorum
	const char * Pragma::_true_ =				"true";
	const char * Pragma::_underrelaxation_ =		"underrelaxation";
	const char * Pragma::_variance_ =			"variance";
	const char * Pragma::_warning_ =			"warning";
	const char * Pragma::_yes_ =				"yes";
    }
}

