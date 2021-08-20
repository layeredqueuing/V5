/*  -*- c++ -*-
 * $Id: pragma.cc 14945 2021-08-18 20:50:57Z greg $ *
 * Pragma processing and definitions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * April 2011
 * ------------------------------------------------------------------------
 */

#include "lqns.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <lqio/glblerr.h>
#include "pragma.h"

Pragma * Pragma::__cache = nullptr;
const std::map<const std::string,const Pragma::fptr> Pragma::__set_pragma =
{
    { LQIO::DOM::Pragma::_cycles_,			&Pragma::setAllowCycles },
    { LQIO::DOM::Pragma::_force_infinite_,		&Pragma::setForceInfinite },
    { LQIO::DOM::Pragma::_force_multiserver_,		&Pragma::setForceMultiserver },
    { LQIO::DOM::Pragma::_interlocking_,		&Pragma::setInterlock },
    { LQIO::DOM::Pragma::_layering_,			&Pragma::setLayering },
    { LQIO::DOM::Pragma::_multiserver_,			&Pragma::setMultiserver },
    { LQIO::DOM::Pragma::_mva_,				&Pragma::setMva },
    { LQIO::DOM::Pragma::_overtaking_,			&Pragma::setOvertaking },
    { LQIO::DOM::Pragma::_processor_scheduling_,	&Pragma::setProcessorScheduling },
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    { LQIO::DOM::Pragma::_quorum_distribution_,		&Pragma::setQuorumDistribution },
    { LQIO::DOM::Pragma::_quorum_delayed_calls_,	&Pragma::setQuorumDelayedCalls },
    { LQIO::DOM::Pragma::_quorum_idle_time_,		&Pragma::setQuorumIdleTime },
#endif
    { LQIO::DOM::Pragma::_replication_,			&Pragma::setReplication },
#if RESCHEDULE
    { LQIO::DOM::Pragma::_reschedule_on_async_send_,	&Pragma::setRescheduleOnAsyncSend },
#endif
    { LQIO::DOM::Pragma::_severity_level_,		&Pragma::setSeverityLevel },
    { LQIO::DOM::Pragma::_spex_header_,			&Pragma::setSpexHeader },
    { LQIO::DOM::Pragma::_stop_on_bogus_utilization_,	&Pragma::setStopOnBogusUtilization },
    { LQIO::DOM::Pragma::_stop_on_message_loss_,	&Pragma::setStopOnMessageLoss },
    { LQIO::DOM::Pragma::_task_scheduling_,		&Pragma::setTaskScheduling },
    { LQIO::DOM::Pragma::_tau_,				&Pragma::setTau },
    { LQIO::DOM::Pragma::_threads_,			&Pragma::setThreads },
    { LQIO::DOM::Pragma::_variance_,			&Pragma::setVariance }
};

/*
 * Set default values in the constructor.  Defaults are used below.
 */

Pragma::Pragma() :
    _allow_cycles(false),
    _exponential_paths(false),
    _force_infinite(ForceInfinite::NONE),
    _force_multiserver(ForceMultiserver::NONE),
    _interlock(true),
    _layering(Layering::BATCHED),
    _multiserver(Multiserver::DEFAULT),
    _mva(MVA::LINEARIZER),
    _overtaking(Overtaking::MARKOV),
    _processor_scheduling(SCHEDULE_PS),
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    _quorum_distribution(QuorumDistribution::DEFAULT),
    _quorum_delayed_calls(QuorumDelayedCalls::DEFAULT),
    _quorum_idle_time(QuorumIdleTime::DEFAULT),
#endif
    _replication(Replication::EXPAND),
#if RESCHEDULE
    _reschedule_on_async_send(false),
#endif
    _severity_level(LQIO::NO_ERROR),
    _spex_header(true),
    _stop_on_bogus_utilization(0.),		/* Not a bool.	U > nn */
    _stop_on_message_loss(true),
    _task_scheduling(SCHEDULE_FIFO),
    _tau(8),
    _threads(Threads::HYPER),
    _variance(Variance::DEFAULT),
    /* Bonus */
    _default_processor_scheduling(true),
    _default_task_scheduling(true),
    _entry_variance(true),
    _init_variance_only(false)
{
}


void
Pragma::set( const std::map<std::string,std::string>& list )
{
    if ( __cache != nullptr ) delete __cache;
    __cache = new Pragma();

    for ( std::map<std::string,std::string>::const_iterator i = list.begin(); i != list.end(); ++i ) {
	const std::string& param = i->first;
	const std::map<const std::string,const fptr>::const_iterator j = __set_pragma.find(param);
	if ( j != __set_pragma.end() ) {
	    try {
		fptr f = j->second;
		(__cache->*f)(i->second);
	    }
	    catch ( const std::domain_error& e ) {
		LQIO::solution_error( LQIO::WRN_PRAGMA_ARGUMENT_INVALID, param.c_str(), e.what() );
	    }
	}
    }
}


void Pragma::setAllowCycles(const std::string& value)
{
    _allow_cycles = LQIO::DOM::Pragma::isTrue(value);
}


void Pragma::setForceInfinite(const std::string& value)
{
    static const std::map<const std::string,const ForceInfinite> __force_infinite_pragma = {
	{ LQIO::DOM::Pragma::_all_,		ForceInfinite::ALL },
	{ LQIO::DOM::Pragma::_fixed_rate_,	ForceInfinite::FIXED_RATE },
	{ LQIO::DOM::Pragma::_multiservers_,	ForceInfinite::MULTISERVERS },
	{ LQIO::DOM::Pragma::_none_,		ForceInfinite::NONE }
    };

    const std::map<const std::string,const ForceInfinite>::const_iterator pragma = __force_infinite_pragma.find( value );
    if ( pragma != __force_infinite_pragma.end() ) {
	_force_infinite = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}

void Pragma::setForceMultiserver(const std::string& value)
{
    static const std::map<const std::string,const Pragma::ForceMultiserver> __force_multiserver = {
	{ LQIO::DOM::Pragma::_all_,		ForceMultiserver::ALL },
	{ LQIO::DOM::Pragma::_none_,		ForceMultiserver::NONE },
	{ LQIO::DOM::Pragma::_tasks_,		ForceMultiserver::TASKS },
	{ LQIO::DOM::Pragma::_processors_,	ForceMultiserver::PROCESSORS }
    };

    const std::map<const std::string,const Pragma::ForceMultiserver>::const_iterator pragma = __force_multiserver.find( value );
    if ( pragma != __force_multiserver.end() ) {
	_force_multiserver = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}


void Pragma::setInterlock(const std::string& value)
{
    _interlock = LQIO::DOM::Pragma::isTrue(value);
}


/* static */

const std::map<const std::string,const Pragma::Layering> Pragma::__layering_pragma = {
    { LQIO::DOM::Pragma::_batched_,		Pragma::Layering::BATCHED },
    { LQIO::DOM::Pragma::_batched_back_,	Pragma::Layering::BACKPROPOGATE_BATCHED },
    { LQIO::DOM::Pragma::_hwsw_,		Pragma::Layering::HWSW },
    { LQIO::DOM::Pragma::_mol_,			Pragma::Layering::METHOD_OF_LAYERS },
    { LQIO::DOM::Pragma::_mol_back_,		Pragma::Layering::BACKPROPOGATE_METHOD_OF_LAYERS },
    { LQIO::DOM::Pragma::_squashed_,		Pragma::Layering::SQUASHED },
    { LQIO::DOM::Pragma::_srvn_,		Pragma::Layering::SRVN }
};

const std::string& Pragma::getLayeringStr()
{
    for ( std::map<const std::string,const Pragma::Layering>::const_iterator pragma = __layering_pragma.begin(); pragma != __layering_pragma.end(); ++pragma ) {
	if ( pragma->second == __cache->_layering ) return pragma->first;
    }
    abort();
    return __layering_pragma.begin()->first;	/* Not reached */
}

void Pragma::setLayering(const std::string& value)
{
    const std::map<const std::string,const Pragma::Layering>::const_iterator pragma = __layering_pragma.find( value );
    if ( pragma != __layering_pragma.end() ) {
	_layering = pragma->second;
#if 0
    } else if ( value == LQIO::DOM::Pragma::_prune_infinite_servers_ ) {
	/* NOP */
#endif
    } else {
	throw std::domain_error( value );
    }
}


void Pragma::setMultiserver(const std::string& value)
{
    static const std::map<const std::string,const Pragma::Multiserver> __multiserver_pragma = {
	{ LQIO::DOM::Pragma::_bruell_,		Pragma::Multiserver::BRUELL },
	{ LQIO::DOM::Pragma::_conway_,		Pragma::Multiserver::CONWAY },
	{ LQIO::DOM::Pragma::_reiser_,		Pragma::Multiserver::REISER },
	{ LQIO::DOM::Pragma::_reiser_ps_,	Pragma::Multiserver::REISER_PS },
	{ LQIO::DOM::Pragma::_rolia_,		Pragma::Multiserver::ROLIA },
	{ LQIO::DOM::Pragma::_rolia_ps_,	Pragma::Multiserver::ROLIA_PS },
	{ LQIO::DOM::Pragma::_schmidt_,		Pragma::Multiserver::SCHMIDT },
	{ LQIO::DOM::Pragma::_suri_,		Pragma::Multiserver::SURI },
	{ LQIO::DOM::Pragma::_zhou_,		Pragma::Multiserver::ZHOU }
    };
    const std::map<const std::string,const Pragma::Multiserver>::const_iterator pragma = __multiserver_pragma.find( value );
    if ( pragma != __multiserver_pragma.end() ) {
	_multiserver = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_multiserver = Multiserver::DEFAULT;
    } else {
	throw std::domain_error( value );
    }
}

void Pragma::setMva(const std::string& value)
{
    static const std::map<const std::string,const Pragma::MVA> __mva_pragma = {
	{ LQIO::DOM::Pragma::_exact_,		MVA::EXACT },
	{ LQIO::DOM::Pragma::_schweitzer_,	MVA::SCHWEITZER },
	{ LQIO::DOM::Pragma::_fast_,		MVA::FAST },
	{ LQIO::DOM::Pragma::_one_step_,	MVA::ONESTEP },
	{ LQIO::DOM::Pragma::_one_step_linearizer_, MVA::ONESTEP_LINEARIZER }
    };

    const std::map<const std::string,const Pragma::MVA>::const_iterator pragma = __mva_pragma.find( value );
    if ( pragma != __mva_pragma.end() ) {
	_mva = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}


void Pragma::setOvertaking(const std::string& value)
{
    static const std::map<const std::string,const Pragma::Overtaking> __overtaking_pragma = {
	{ LQIO::DOM::Pragma::_markov_,	Overtaking::MARKOV },
	{ LQIO::DOM::Pragma::_none_,	Overtaking::NONE },
	{ LQIO::DOM::Pragma::_rolia_,	Overtaking::ROLIA },
	{ LQIO::DOM::Pragma::_simple_,	Overtaking::SIMPLE },
	{ LQIO::DOM::Pragma::_special_,	Overtaking::SPECIAL }
    };

    const std::map<const std::string,const Pragma::Overtaking>::const_iterator pragma = __overtaking_pragma.find( value );
    if ( pragma != __overtaking_pragma.end() ) {
	_overtaking = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}


void Pragma::setProcessorScheduling(const std::string& value)
{
    static const std::map<const std::string,const scheduling_type> __processor_scheduling_pragma = {
	{ scheduling_label[SCHEDULE_DELAY].XML,	SCHEDULE_DELAY },
	{ scheduling_label[SCHEDULE_FIFO].XML,	SCHEDULE_FIFO },
	{ scheduling_label[SCHEDULE_HOL].XML,	SCHEDULE_HOL },
	{ scheduling_label[SCHEDULE_PPR].XML,	SCHEDULE_PPR },
	{ scheduling_label[SCHEDULE_PS].XML,	SCHEDULE_PS },
	{ scheduling_label[SCHEDULE_RAND].XML,	SCHEDULE_RAND }
    };

    const std::map<const std::string,const scheduling_type>::const_iterator pragma = __processor_scheduling_pragma.find( value );
    if ( pragma != __processor_scheduling_pragma.end() ) {
	_default_processor_scheduling = false;
	_processor_scheduling = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_default_processor_scheduling = true;
    } else {
	throw std::domain_error( value );
    }
}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
void Pragma::setQuorumDistribution(const std::string& value)
{
    static const std::map<const std::string,const Pragma::QuorumDistribution> __quorum_distribution_pragma = {
	{ LQIO::DOM::Pragma::_threepoint_,	Pragma::QuorumDistribution::THREEPOINT },
	{ LQIO::DOM::Pragma::_gamma_,		Pragma::QuorumDistribution::GAMMA },
	{ LQIO::DOM::Pragma::_geometric_,	Pragma::QuorumDistribution::CLOSEDFORM_GEOMETRIC },
	{ LQIO::DOM::Pragma::_deterministic_,	Pragma::QuorumDistribution::CLOSEDFORM_DETRMINISTIC },
	{ LQIO::DOM::Pragma::_default_,		Pragma::QuorumDistribution::DEFAULT }
    };

    const std::map<const std::string,const Pragma::QuorumDistribution>::const_iterator pragma = __quorum_distribution_pragma.find( value );
    if ( pragma != __quorum_distribution_pragma.end() ) {
	_quorum_distribution = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}


void Pragma::setQuorumDelayedCalls(const std::string& value)
{
    static const std::map<const std::string,const Pragma::QuorumDelayedCalls> __quorum_delayed_calls_pragma = {
	{ LQIO::DOM::Pragma::_keep_all_,	Pragma::QuorumDelayedCalls::KEEP_ALL },
	{ LQIO::DOM::Pragma::_abort_all_,	Pragma::QuorumDelayedCalls::ABORT_ALL },
	{ LQIO::DOM::Pragma::_abort_local_,	Pragma::QuorumDelayedCalls::ABORT_LOCAL_ONLY },
	{ LQIO::DOM::Pragma::_abort_remote_,	Pragma::QuorumDelayedCalls::ABORT_REMOTE_ONLY },
	{ LQIO::DOM::Pragma::_default_,		Pragma::QuorumDelayedCalls::DEFAULT }
    };

    const std::map<const std::string,const Pragma::QuorumDelayedCalls>::const_iterator pragma = __quorum_delayed_calls_pragma.find( value );
    if ( pragma != __quorum_delayed_calls_pragma.end() ) {
	_quorum_delayed_calls = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}


void Pragma::setQuorumIdleTime(const std::string& value)
{
    static const std::map<const std::string,const Pragma::QuorumIdleTime> __quorum_idle_time_pragma = {
	{ LQIO::DOM::Pragma::_default_,		Pragma::QuorumIdleTime::DEFAULT },
	{ LQIO::DOM::Pragma::_join_delay_,	Pragma::QuorumIdleTime::JOINDELAY }
    };

    const std::map<const std::string,const Pragma::QuorumIdleTime>::const_iterator pragma = __quorum_idle_time_pragma.find( value );
    if ( pragma != __quorum_idle_time_pragma.end() ) {
	_quorum_idle_time = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}
#endif


void Pragma::setReplication(const std::string& value)
{
    static const std::map<const std::string,const Pragma::Replication> __replication_pragma = {
	{ LQIO::DOM::Pragma::_expand_,		Pragma::Replication::EXPAND },
#if BUG_299_PRUNE
	{ LQIO::DOM::Pragma::_prune_,		Pragma::Replication::PRUNE },
#endif
	{ LQIO::DOM::Pragma::_pan_,		Pragma::Replication::PAN }
    };

    const std::map<const std::string,const Pragma::Replication>::const_iterator pragma = __replication_pragma.find( value );
    if ( pragma != __replication_pragma.end() ) {
	_replication = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}


#if RESCHEDULE
void Pragma::setRescheduleOnAsyncSend(const std::string& value)
{
    _reschedule_on_async_send = LQIO::DOM::Pragma::isTrue(value);
}
#endif


void Pragma::setSpexHeader(const std::string& value)
{
    _spex_header = LQIO::DOM::Pragma::isTrue( value );
}



void Pragma::setSeverityLevel(const std::string& value)
{
    _severity_level = LQIO::DOM::Pragma::getSeverityLevel( value );
}


void Pragma::setStopOnBogusUtilization(const std::string& value)
{
    char * endptr = nullptr;
    const double temp = std::strtod( value.c_str(), &endptr );
    if ( (temp < 1 && temp != 0) || *endptr != '\0' ) throw std::domain_error( value );
    _stop_on_bogus_utilization = temp;
}


void Pragma::setStopOnMessageLoss(const std::string& value)
{
    _stop_on_message_loss = LQIO::DOM::Pragma::isTrue(value);
}


void Pragma::setTaskScheduling(const std::string& value)
{
    static const std::map<const std::string,const scheduling_type> __task_scheduling_pragma = {
	{ scheduling_label[SCHEDULE_DELAY].XML,	SCHEDULE_DELAY },
	{ scheduling_label[SCHEDULE_FIFO].XML,	SCHEDULE_FIFO }
    };

    const std::map<const std::string,const scheduling_type>::const_iterator pragma = __task_scheduling_pragma.find( value );
    if ( pragma != __task_scheduling_pragma.end() ) {
	_default_task_scheduling = false;
	_task_scheduling = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_default_task_scheduling = true;
    } else {
	throw std::domain_error( value );
    }
}

void Pragma::setTau(const std::string& value)
{
    char * endptr = nullptr;
    const unsigned int temp = std::strtol( value.c_str(), &endptr, 10 );
    if ( temp > 20 || *endptr != '\0' ) throw std::domain_error( value );
    _tau = temp;
}


void Pragma::setThreads(const std::string& value)
{
    static const std::map<const std::string,const Pragma::Threads> __threads_pragma = {
	{ LQIO::DOM::Pragma::_hyper_,	Threads::HYPER },
	{ LQIO::DOM::Pragma::_mak_,	Threads::MAK_LUNDSTROM },
	{ LQIO::DOM::Pragma::_none_,	Threads::NONE }
    };

    const std::map<const std::string,const Pragma::Threads>::const_iterator pragma = __threads_pragma.find( value );
    if ( pragma != __threads_pragma.end() ) {
	_threads = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_exponential_ ) {
	_exponential_paths = true;
    } else {
	throw std::domain_error( value );
    }
}


void Pragma::setVariance(const std::string& value)
{
    static const std::map<const std::string,const Pragma::Variance> __variance_pragma = {
	{ LQIO::DOM::Pragma::_default_,		Variance::DEFAULT },
	{ LQIO::DOM::Pragma::_mol_,		Variance::MOL },
	{ LQIO::DOM::Pragma::_none_,		Variance::NONE },
	{ LQIO::DOM::Pragma::_stochastic_,	Variance::STOCHASTIC }
    };
	
    const std::map<const std::string,const Pragma::Variance>::const_iterator pragma = __variance_pragma.find( value );
    if ( pragma != __variance_pragma.end() ) {
	_variance = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_no_entry_ ) {
	_entry_variance = false;
    } else if ( value == LQIO::DOM::Pragma::_init_only_ ) {
	_init_variance_only = true;
    } else {
	throw std::domain_error( value );
    }
}

/*
 * Print out available pragmas.
 */

std::ostream&
Pragma::usage( std::ostream& output )
{
    output << "Valid pragmas: " << std::endl;
    std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );

    for ( std::map<const std::string,const fptr>::const_iterator i = __set_pragma.begin(); i != __set_pragma.end(); ++i ) {
	output << "\t" << std::setw(20) << i->first;
	if ( i->first == LQIO::DOM::Pragma::_tau_ ) {
	    output << " = <int>" << std::endl;
	} else {
	    const std::set<std::string>* args = LQIO::DOM::Pragma::getValues( i->first );
	    if ( args != nullptr && args->size() > 1 ) {
		output << " = {";

		size_t count = 0;
		for ( std::set<std::string>::const_iterator q = args->begin(); q != args->end(); ++q ) {
		    if ( q->empty() ) continue;
		    if ( count > 0 ) output << ",";
		    output << *q;
		    count += 1;
		}
		output << "}" << std::endl;
	    } else {
		output << " = <arg>" << std::endl;
	    }
	}
    }
    output.setf( flags );
    return output;
}
