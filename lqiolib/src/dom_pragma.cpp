/*
 *  $Id: dom_pragma.cpp 14945 2021-08-18 20:50:57Z greg $
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

	Pragma::Pragma() : _loadedPragmas()
	{
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
	    std::for_each( list.begin(), list.end(), Merge(*this) );
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
	    const std::map<const std::string,const std::set<std::string>*>::const_iterator i = __pragmas.find(s);
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
	    const std::map<const std::string,const std::set<std::string>*>::const_iterator i = __pragmas.find( param );
	    if ( i == __pragmas.end() ) {
		LQIO::solution_error( LQIO::WRN_PRAGMA_UNKNOWN, param.c_str() );
		return false;
	    } else if ( i->second != nullptr ) {
		const std::set<std::string>& values = *i->second;
		if ( values.find( value ) == values.end() ) {
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


	/* 
	 * Return true or false depending on value.   Throw on anything not allowed.  
	 * __true_false_arg here is a map with a return value which should match the
	 * __true_false_args global set.
	 */
	
	bool Pragma::isTrue(const std::string& value)
	{
	    static const std::map<const std::string,bool> __true_false_arg = {
		{LQIO::DOM::Pragma::_true_,  true},
		{LQIO::DOM::Pragma::_yes_,   true},
		{LQIO::DOM::Pragma::_false_, false},
		{LQIO::DOM::Pragma::_no_,    false},
		{"t", true},
		{"y", true},
		{"f", false},
		{"n", false},
		{"",  true},		/* No argument */
	    };
	    const std::map<const std::string,bool>::const_iterator arg = __true_false_arg.find( value );
	    if ( arg == __true_false_arg.end() ) throw std::domain_error( value );
	    return arg->second;
	}

	/*
	 * Return the severity level 
	 */

	LQIO::severity_t Pragma::getSeverityLevel(const std::string& value )
	{
	    static const std::map<const std::string,const LQIO::severity_t> __serverity_level_arg = {
		{ "",					LQIO::NO_ERROR },	/* Report all	*/
		{ LQIO::DOM::Pragma::_all_,		LQIO::NO_ERROR },	
		{ LQIO::DOM::Pragma::_advisory_,	LQIO::ADVISORY_ONLY },	/* Report Runtime only */
		{ LQIO::DOM::Pragma::_run_time_,	LQIO::RUNTIME_ERROR },	/* Report fatal only (!) */
		{ LQIO::DOM::Pragma::_warning_,		LQIO::WARNING_ONLY }	/* Report Advisory & Runtime */
	    };

	    const std::map<const std::string,const LQIO::severity_t>::const_iterator arg = __serverity_level_arg.find( value );
	    if ( arg == __serverity_level_arg.end() ) throw std::domain_error( value );
	    return arg->second;
	}

	/* Labels */
	
	const char * Pragma::_abort_all_ = 			"abort-all";		// Quorum
	const char * Pragma::_abort_local_ = 			"abort-local";		// Quorum
	const char * Pragma::_abort_remote_ = 			"abort-remote";		// Quorum
	const char * Pragma::_advisory_ =			"advisory";
	const char * Pragma::_all_ =				"all";
	const char * Pragma::_batched_ =			"batched";
	const char * Pragma::_batched_back_ =			"batched-back";
	const char * Pragma::_bcmp_ =				"bcmp";			// BUG 270
	const char * Pragma::_block_period_ =			"block-period";
	const char * Pragma::_bruell_ =				"bruell";		// multiserver
	const char * Pragma::_conway_ =				"conway";		// multiserver
	const char * Pragma::_custom_ = 		    	"custom";		// multiserver
	const char * Pragma::_custom_natural_ =		    	"custom-natural";
	const char * Pragma::_cycles_ =				"cycles";
	const char * Pragma::_default_ =			"default";
	const char * Pragma::_default_natural_ = 		"default-natural";
	const char * Pragma::_deterministic_ = 			"deterministic";	// Quorum
	const char * Pragma::_exact_ =				"exact";
	const char * Pragma::_expand_ =				"expand";
	const char * Pragma::_exponential_ =			"exponential";
	const char * Pragma::_extended_ =			"extended";		// BUG 270
	const char * Pragma::_false_ =				"false";
	const char * Pragma::_fast_ =				"fast";
	const char * Pragma::_fixed_rate_ =			"fixed-rate";
	const char * Pragma::_force_infinite_ =			"force-infinite";
	const char * Pragma::_force_multiserver_ =		"force-multiserver";
	const char * Pragma::_force_random_queueing_ =		"force-random-queueing";
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
	const char * Pragma::_lqn_ =				"lqn";			// BUG 270
	const char * Pragma::_mak_ =				"mak";
	const char * Pragma::_markov_ =				"markov";
	const char * Pragma::_max_blocks_ =			"max-blocks";
	const char * Pragma::_mol_ =				"mol";
	const char * Pragma::_mol_back_ =			"mol-back";
	const char * Pragma::_multiserver_ = 			"multiserver";
	const char * Pragma::_multiservers_ = 			"multiservers";
	const char * Pragma::_mva_ = 				"mva";
	const char * Pragma::_nice_ =				"nice";
	const char * Pragma::_no_ =				"no";
	const char * Pragma::_no_entry_ =			"no-entry";
	const char * Pragma::_none_ =				"none";
	const char * Pragma::_one_step_ =			"one-step";
	const char * Pragma::_one_step_linearizer_ =		"one-step-linearizer";
	const char * Pragma::_overtaking_ =			"overtaking";
	const char * Pragma::_pan_ =				"pan";
	const char * Pragma::_precision_ =			"precision";
	const char * Pragma::_processor_scheduling_ =		"processor-scheduling";
	const char * Pragma::_processors_ =			"processors";
	const char * Pragma::_prune_ =				"prune";
	const char * Pragma::_queue_size_ =			"queue-size";		// Petrisrvn.
	const char * Pragma::_quorum_delayed_calls_ =		"quorum-delayed-calls";	// Quroum
	const char * Pragma::_quorum_distribution_ = 		"quorum-distribution";	// Quroum
	const char * Pragma::_quorum_idle_time_ = 		"quorum-idle-time";	// Quroum
	const char * Pragma::_quorum_reply_ =			"quorum-reply";		// Quorum
	const char * Pragma::_reiser_ =				"reiser";		// multiserver
	const char * Pragma::_reiser_ps_ =			"reiser-ps";		// multiserver
	const char * Pragma::_replication_ =			"replication";
	const char * Pragma::_reschedule_on_async_send_ =	"reschedule-on-async-send";
	const char * Pragma::_rolia_ =				"rolia";		// multiserver
	const char * Pragma::_rolia_ps_ =			"rolia-ps";		// multiserver
	const char * Pragma::_run_time_ =			"run-time";
	const char * Pragma::_scheduling_model_ = 		"scheduling-model";
	const char * Pragma::_schmidt_ =			"schmidt";		// multiserver
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
	const char * Pragma::_suri_ =				"suri";			// multiserver
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
	const char * Pragma::_zhou_ =				"zhou";			// multiserver 

	/* Args */
	
	const std::set<std::string> Pragma::__bcmp_args = { _lqn_, _extended_, _true_, _yes_, _false_, _no_, "t", "y", "f", "n", "" };
	const std::set<std::string> Pragma::__force_infinite_args = { _none_, _fixed_rate_, _multiservers_, _all_ };
	const std::set<std::string> Pragma::__force_multiserver_args = { _none_, _processors_, _tasks_, _all_ };
	const std::set<std::string> Pragma::__layering_args = { _batched_, _batched_back_, _mol_, _mol_back_, _squashed_, _srvn_, _hwsw_ };
	const std::set<std::string> Pragma::__multiserver_args = { _bruell_, _conway_, _default_, _reiser_, _reiser_ps_, _rolia_, _rolia_ps_, _schmidt_, _suri_, _zhou_ };
	const std::set<std::string> Pragma::__mva_args = { _linearizer_, _exact_, _schweitzer_, _fast_, _one_step_, _one_step_linearizer_ };
	const std::set<std::string> Pragma::__overtaking_args = { _markov_, _rolia_, _simple_, _special_, _none_ };
	const std::set<std::string> Pragma::__processor_args = { _default_, scheduling_label[SCHEDULE_DELAY].XML, scheduling_label[SCHEDULE_FIFO].XML, scheduling_label[SCHEDULE_HOL].XML, scheduling_label[SCHEDULE_PPR].XML, scheduling_label[SCHEDULE_PS].XML, scheduling_label[SCHEDULE_RAND].XML };
	const std::set<std::string> Pragma::__quorum_delayed_calls_args = { _keep_all_, _abort_all_, _abort_local_, _abort_remote_ };
	const std::set<std::string> Pragma::__quorum_distribution_args = { _threepoint_, _gamma_, _geometric_, _deterministic_ };
	const std::set<std::string> Pragma::__quorum_idle_time_args = { _default_, _join_delay_ };
	const std::set<std::string> Pragma::__replication_args = { _expand_, _prune_, _pan_ };
	const std::set<std::string> Pragma::__scheduling_model_args = { _default_, _default_natural_, _custom_, _custom_natural_ };
	const std::set<std::string> Pragma::__task_args = { _default_, scheduling_label[SCHEDULE_DELAY].XML, scheduling_label[SCHEDULE_FIFO].XML, scheduling_label[SCHEDULE_HOL].XML, scheduling_label[SCHEDULE_RAND].XML };
	const std::set<std::string> Pragma::__threads_args = { _hyper_, _mak_, _none_, _exponential_ };
	const std::set<std::string> Pragma::__true_false_arg = { _true_, _false_, _yes_, _no_, "t", "f", "y", "n", "" };
	const std::set<std::string> Pragma::__variance_args = { _default_, _none_, _stochastic_, _mol_, _no_entry_, _init_only_ };
	const std::set<std::string> Pragma::__warning_args = { _all_, _warning_, _advisory_, _run_time_, "" };

	/* Pragmas */
	
	const std::map<const std::string,const std::set<std::string>*> Pragma::__pragmas = {
	    { _bcmp_,			    &__true_false_arg },	    /* lqns */
	    { _block_period_,      	    nullptr },			    /* lqsim */
	    { _cycles_,  	    	    &__true_false_arg },	    /* lqns */
	    { _force_infinite_,		    &__force_infinite_args },	    /* */
	    { _force_multiserver_, 	    &__force_multiserver_args },    /* lqns */
	    { _force_random_queueing_,	    &__true_false_arg },	    /* petrisrvn */
	    { _initial_delay_,     	    nullptr },			    /* lqsim */
	    { _initial_loops_,     	    nullptr },			    /* lqsim */
	    { _interlocking_,      	    &__true_false_arg },	    /* lqns */
	    { _layering_,     	    	    &__layering_args },		    /* lqns */
	    { _max_blocks_,  	    	    nullptr },			    /* lqsim */
	    { _multiserver_,  	    	    &__multiserver_args },	    /* lqns */
	    { _mva_,  		   	    &__mva_args },		    /* lqns */
	    { _nice_,              	    nullptr },			    /* lqsim */
	    { _overtaking_,  	    	    &__overtaking_args },	    /* lqns */
	    { _precision_,  	    	    nullptr },			    /* lqsim */
	    { _processor_scheduling_,	    &__processor_args },
	    { _prune_,  		    &__true_false_arg },	    /* lqns */
	    { _queue_size_,		    nullptr },			    /* petrisrvn, lqsim? */
	    { _quorum_delayed_calls_,  	    &__quorum_delayed_calls_args }, /* lqns */
	    { _quorum_distribution_,  	    &__quorum_distribution_args },  /* lqns */
	    { _quorum_idle_time_,  	    &__quorum_idle_time_args },     /* lqns */
	    { _quorum_reply_, 		    &__true_false_arg },	    /* lqsim */
	    { _reschedule_on_async_send_,   &__true_false_arg },
	    { _replication_,		    &__replication_args },	    /* lqns */
	    { _run_time_,  	    	    nullptr },			    /* lqsim */
	    { _scheduling_model_,  	    &__scheduling_model_args },
	    { _seed_value_,  	    	    nullptr },			    /* lqsim */
	    { _severity_level_,  	    &__warning_args },
	    { _spex_header_,  		    &__true_false_arg },
	    { _stop_on_bogus_utilization_,  nullptr },			    /* lqns */
	    { _stop_on_message_loss_,  	    &__true_false_arg },
	    { _task_scheduling_,  	    &__task_args },
	    { _tau_,               	    nullptr },			    /* lqns */
	    { _threads_,  		    &__threads_args },		    /* lqns */
	    { _underrelaxation_,	    nullptr },			    /* lqns */
	    { _variance_,  		    &__variance_args }		    /* lqns */
	};
	
    }
}
