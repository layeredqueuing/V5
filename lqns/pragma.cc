/*  -*- c++ -*-
 * $Id: pragma.cc 14319 2021-01-02 04:11:00Z greg $ *
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include "pragma.h"
#include "lqio/glblerr.h"

Pragma * Pragma::__cache = nullptr;
std::map<std::string,Pragma::fptr> Pragma::__set_pragma;

std::map<std::string,Pragma::pragma_force_multiserver> Pragma::__force_multiserver;
std::map<std::string,Pragma::pragma_layering> Pragma::__layering_pragma;
std::map<std::string,Pragma::pragma_multiserver> Pragma::__multiserver_pragma;
std::map<std::string,Pragma::pragma_mva> Pragma::__mva_pragma;
std::map<std::string,Pragma::pragma_overtaking> Pragma::__overtaking_pragma;
std::map<std::string,scheduling_type> Pragma::__processor_scheduling_pragma;
std::map<std::string,LQIO::severity_t> Pragma::__serverity_level_pragma;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
std::map<std::string,Pragma::pragma_quorum_distribution> Pragma::__quorum_distribution_pragma;
std::map<std::string,Pragma::pragma_quorum_delayed_calls> Pragma::__quorum_delayed_calls_pragma;
std::map<std::string,Pragma::pragma_quorum_idle_time> Pragma::__quorum_idle_time_pragma;
#endif
std::map<std::string,Pragma::pragma_threads> Pragma::__threads_pragma;
std::map<std::string,Pragma::pragma_variance> Pragma::__variance_pragma;


/*
 * Set default values in the constructor.  Defaults are used below.
 */

Pragma::Pragma() :
    _allow_cycles(false),
    _exponential_paths(false),
    _force_multiserver(FORCE_NONE),
    _interlock(true),
    _layering(BATCHED_LAYERS),
    _multiserver(DEFAULT_MULTISERVER),
    _mva(LINEARIZER_MVA),
    _overtaking(MARKOV_OVERTAKING),
    _processor_scheduling(SCHEDULE_PS),
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    _quorum_distribution(DEFAULT_QUORUM_DISTRIBUTION),
    _quorum_delayed_calls(DEFAULT_QUORUM_DELAYED_CALLS),
    _quorum_idle_time(DEFAULT_IDLETIME),
#endif
#if RESCHEDULE
    _reschedule_on_async_send(false),
#endif
    _severity_level(LQIO::NO_ERROR),
    _spex_header(true),
    _stop_on_bogus_utilization(0),
    _stop_on_message_loss(true),
    _tau(8),
    _threads(HYPER_THREADS),
    _variance(DEFAULT_VARIANCE),
    _default_processor_scheduling(true),
    _init_variance_only(false),
    _entry_variance(true)
{
}


/*
 * we use maps.  They have to be initialized dynamically.  This is a singleton.
 */

void
Pragma::initialize()
{
    if ( __set_pragma.size() != 0 ) return;

    __set_pragma[LQIO::DOM::Pragma::_cycles_] = &Pragma::setAllowCycles;
    __set_pragma[LQIO::DOM::Pragma::_force_multiserver_] = &Pragma::setForceMultiserver;
    __set_pragma[LQIO::DOM::Pragma::_interlocking_] = &Pragma::setInterlock;
    __set_pragma[LQIO::DOM::Pragma::_layering_] = &Pragma::setLayering;
    __set_pragma[LQIO::DOM::Pragma::_multiserver_] = &Pragma::setMultiserver;
    __set_pragma[LQIO::DOM::Pragma::_mva_] = &Pragma::setMva;
    __set_pragma[LQIO::DOM::Pragma::_overtaking_] = &Pragma::setOvertaking;
    __set_pragma[LQIO::DOM::Pragma::_processor_scheduling_] = &Pragma::setProcessorScheduling;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    __set_pragma[LQIO::DOM::Pragma::_quorum_distribution_] = &Pragma::setQuorumDistribution;
    __set_pragma[LQIO::DOM::Pragma::_quorum_delayed_calls_] = &Pragma::setQuorumDelayedCalls;
    __set_pragma[LQIO::DOM::Pragma::_quorum_idle_time_] = &Pragma::setQuorumIdleTime;
#endif
#if RESCHEDULE
    __set_pragma[LQIO::DOM::Pragma::_reschedule_on_async_send_] = &Pragma::setRescheduleOnAsyncSend;
#endif
    __set_pragma[LQIO::DOM::Pragma::_severity_level_] = &Pragma::setSeverityLevel;
    __set_pragma[LQIO::DOM::Pragma::_spex_header_] = &Pragma::setSpexHeader;
    __set_pragma[LQIO::DOM::Pragma::_stop_on_bogus_utilization_] = &Pragma::setStopOnBogusUtilization;
    __set_pragma[LQIO::DOM::Pragma::_stop_on_message_loss_] = &Pragma::setStopOnMessageLoss;
    __set_pragma[LQIO::DOM::Pragma::_tau_] = &Pragma::setTau;
    __set_pragma[LQIO::DOM::Pragma::_threads_] = &Pragma::setThreads;
    __set_pragma[LQIO::DOM::Pragma::_variance_] = &Pragma::setVariance;

    __force_multiserver[LQIO::DOM::Pragma::_all_] =		FORCE_ALL;
    __force_multiserver[LQIO::DOM::Pragma::_none_] =		FORCE_NONE;
    __force_multiserver[LQIO::DOM::Pragma::_tasks_] =		FORCE_TASKS;
    __force_multiserver[LQIO::DOM::Pragma::_processors_] =	FORCE_PROCESSORS;

    __layering_pragma[ LQIO::DOM::Pragma::_batched_] =	    BATCHED_LAYERS;
    __layering_pragma[ LQIO::DOM::Pragma::_batched_back_] = BACKPROPOGATE_LAYERS;
    __layering_pragma[ LQIO::DOM::Pragma::_hwsw_] =	    HWSW_LAYERS;
    __layering_pragma[ LQIO::DOM::Pragma::_mol_] =	    METHOD_OF_LAYERS;
    __layering_pragma[ LQIO::DOM::Pragma::_mol_back_] =	    BACKPROPOGATE_METHOD_OF_LAYERS;
    __layering_pragma[ LQIO::DOM::Pragma::_squashed_] =	    SQUASHED_LAYERS;
    __layering_pragma[ LQIO::DOM::Pragma::_srvn_] =	    SRVN_LAYERS;

    __multiserver_pragma[LQIO::DOM::Pragma::_bruell_] =		BRUELL_MULTISERVER;
    __multiserver_pragma[LQIO::DOM::Pragma::_conway_] =		CONWAY_MULTISERVER;
    __multiserver_pragma[LQIO::DOM::Pragma::_reiser_] =		REISER_MULTISERVER;
    __multiserver_pragma[LQIO::DOM::Pragma::_reiser_ps_] =	REISER_PS_MULTISERVER;
    __multiserver_pragma[LQIO::DOM::Pragma::_rolia_] =		ROLIA_MULTISERVER;
    __multiserver_pragma[LQIO::DOM::Pragma::_rolia_ps_] =	ROLIA_PS_MULTISERVER;
    __multiserver_pragma[LQIO::DOM::Pragma::_schmidt_] =	SCHMIDT_MULTISERVER;
    __multiserver_pragma[LQIO::DOM::Pragma::_suri_] =		SURI_MULTISERVER;

    __mva_pragma[LQIO::DOM::Pragma::_exact_] = 			EXACT_MVA;
    __mva_pragma[LQIO::DOM::Pragma::_schweitzer_] = 		SCHWEITZER_MVA;
    __mva_pragma[LQIO::DOM::Pragma::_fast_] = 			FAST_MVA;
    __mva_pragma[LQIO::DOM::Pragma::_one_step_] = 		ONESTEP_MVA;
    __mva_pragma[LQIO::DOM::Pragma::_one_step_linearizer_] = 	ONESTEP_LINEARIZER;
    
    __overtaking_pragma[LQIO::DOM::Pragma::_markov_] =	MARKOV_OVERTAKING;
    __overtaking_pragma[LQIO::DOM::Pragma::_none_] =	NO_OVERTAKING;
    __overtaking_pragma[LQIO::DOM::Pragma::_rolia_] =	ROLIA_OVERTAKING;
    __overtaking_pragma[LQIO::DOM::Pragma::_simple_] =	SIMPLE_OVERTAKING;
    __overtaking_pragma[LQIO::DOM::Pragma::_special_] =	SPECIAL_OVERTAKING;

    __processor_scheduling_pragma[scheduling_label[SCHEDULE_DELAY].XML] =	SCHEDULE_DELAY;
    __processor_scheduling_pragma[scheduling_label[SCHEDULE_FIFO].XML] =	SCHEDULE_FIFO;
    __processor_scheduling_pragma[scheduling_label[SCHEDULE_HOL].XML] =		SCHEDULE_HOL;
    __processor_scheduling_pragma[scheduling_label[SCHEDULE_PPR].XML] =		SCHEDULE_PPR;
    __processor_scheduling_pragma[scheduling_label[SCHEDULE_PS].XML] =		SCHEDULE_PS;
    __processor_scheduling_pragma[scheduling_label[SCHEDULE_RAND].XML] =	SCHEDULE_RAND;

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    __quorum_distribution_pragma[LQIO::DOM::Pragma::_threepoint_] = 	THREEPOINT_QUORUM_DISTRIBUTION;
    __quorum_distribution_pragma[LQIO::DOM::Pragma::_gamma_] = 		GAMMA_QUORUM_DISTRIBUTION;
    __quorum_distribution_pragma[LQIO::DOM::Pragma::_geometric_] = 	CLOSEDFORM_GEOMETRIC_QUORUM_DISTRIBUTION;
    __quorum_distribution_pragma[LQIO::DOM::Pragma::_deterministic_] = 	CLOSEDFORM_DETRMINISTIC_QUORUM_DISTRIBUTION;
    __quorum_distribution_pragma[LQIO::DOM::Pragma::_default_] = 	DEFAULT_QUORUM_DISTRIBUTION;

    __quorum_delayed_calls_pragma[LQIO::DOM::Pragma::_keep_all_] =	KEEP_ALL_QUORUM_DELAYED_CALLS;
    __quorum_delayed_calls_pragma[LQIO::DOM::Pragma::_abort_all_] =	ABORT_ALL_QUORUM_DELAYED_CALLS;
    __quorum_delayed_calls_pragma[LQIO::DOM::Pragma::_abort_local_] =	ABORT_LOCAL_ONLY_QUORUM_DELAYED_CALLS;
    __quorum_delayed_calls_pragma[LQIO::DOM::Pragma::_abort_remote_] =	ABORT_REMOTE_ONLY_QUORUM_DELAYED_CALLS;
    __quorum_delayed_calls_pragma[LQIO::DOM::Pragma::_default_] =	DEFAULT_QUORUM_DELAYED_CALLS;

    __quorum_idle_time_pragma[LQIO::DOM::Pragma::_default_] = 		DEFAULT_IDLETIME;
    __quorum_idle_time_pragma[LQIO::DOM::Pragma::_join_delay_] =	JOINDELAY_IDLETIME;
#endif
    
    __serverity_level_pragma[LQIO::DOM::Pragma::_advisory_] =	LQIO::ADVISORY_ONLY;
    __serverity_level_pragma[LQIO::DOM::Pragma::_run_time_] =	LQIO::RUNTIME_ERROR;
    __serverity_level_pragma[LQIO::DOM::Pragma::_warning_] =	LQIO::WARNING_ONLY;

    __threads_pragma[LQIO::DOM::Pragma::_hyper_] =	HYPER_THREADS;
    __threads_pragma[LQIO::DOM::Pragma::_mak_] =	MAK_LUNDSTROM_THREADS;
    __threads_pragma[LQIO::DOM::Pragma::_none_] =	NO_THREADS;

    __variance_pragma[LQIO::DOM::Pragma::_default_] =		DEFAULT_VARIANCE;
    __variance_pragma[LQIO::DOM::Pragma::_mol_] =		MOL_VARIANCE;
    __variance_pragma[LQIO::DOM::Pragma::_none_] =		NO_VARIANCE;
    __variance_pragma[LQIO::DOM::Pragma::_stochastic_] =	STOCHASTIC_VARIANCE;
}
    

void
Pragma::set( const std::map<std::string,std::string>& list )
{
    initialize();

    if ( __cache != nullptr ) delete __cache;
    __cache = new Pragma();

    std::for_each( list.begin(), list.end(), set_pragma );
}


void Pragma::set_pragma( const std::pair<std::string,std::string>& p )
{
    const std::string& param = p.first;
    const std::map<std::string,fptr>::const_iterator j = __set_pragma.find(param);
    if ( j != __set_pragma.end() ) {
	try {
	    fptr f = j->second;
	    (__cache->*f)(p.second);
	}
	catch ( std::domain_error& e ) {
	    LQIO::solution_error( LQIO::WRN_PRAGMA_ARGUMENT_INVALID, param.c_str(), e.what() );
	}
    }
}


void Pragma::setAllowCycles(const std::string& value)
{
    _allow_cycles = LQIO::DOM::Pragma::isTrue(value);
}

void Pragma::setForceMultiserver(const std::string& value)
{
    const std::map<std::string,pragma_force_multiserver>::const_iterator pragma = __force_multiserver.find( value );
    if ( pragma != __force_multiserver.end() ) {
	_force_multiserver = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    }
}

void Pragma::setInterlock(const std::string& value)
{
    _interlock = LQIO::DOM::Pragma::isTrue(value);
}

void Pragma::setLayering(const std::string& value)
{
    const std::map<std::string,pragma_layering>::const_iterator pragma = __layering_pragma.find( value );
    if ( pragma != __layering_pragma.end() ) {
	_layering = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    } 
}

void Pragma::setMultiserver(const std::string& value)
{
    const std::map<std::string,pragma_multiserver>::const_iterator pragma = __multiserver_pragma.find( value );
    if ( pragma != __multiserver_pragma.end() ) {
	_multiserver = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_multiserver = DEFAULT_MULTISERVER;
    } else {
	throw std::domain_error( value.c_str() );
    }
}

void Pragma::setMva(const std::string& value)
{
    const std::map<std::string,pragma_mva>::const_iterator pragma = __mva_pragma.find( value );
    if ( pragma != __mva_pragma.end() ) {
	_mva = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    }
}


void Pragma::setOvertaking(const std::string& value)
{
    const std::map<std::string,pragma_overtaking>::const_iterator pragma = __overtaking_pragma.find( value );
    if ( pragma != __overtaking_pragma.end() ) {
	_overtaking = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    }
}

void Pragma::setProcessorScheduling(const std::string& value)
{
    const std::map<std::string,scheduling_type>::const_iterator pragma = __processor_scheduling_pragma.find( value );
    if ( pragma != __processor_scheduling_pragma.end() ) {
	_default_processor_scheduling = false;
	_processor_scheduling = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_default_processor_scheduling = true;
    } else {
	throw std::domain_error( value.c_str() );
    }
}


#if BUG_270
void Pragma::setPrune(const std::string& value)
{
    _prune = LQIO::DOM::Pragma::isTrue(value);
}
#endif


#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
void Pragma::setQuorumDistribution(const std::string& value)
{
    const std::map<std::string,pragma_quorum_distribution>::const_iterator pragma = __quorum_distribution_pragma.find( value );
    if ( pragma != __quorum_distribution_pragma.end() ) {
	_quorum_distribution = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    }
}
    
void Pragma::setQuorumDelayedCalls(const std::string& value)
{
    const std::map<std::string,pragma_quorum_delayed_calls>::const_iterator pragma = __quorum_delayed_calls_pragma.find( value );
    if ( pragma != __quorum_delayed_calls_pragma.end() ) {
	_quorum_delayed_calls = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    }
}

void Pragma::setQuorumIdleTime(const std::string& value)
{
    const std::map<std::string,pragma_quorum_idle_time>::const_iterator pragma = __quorum_idle_time_pragma.find( value );
    if ( pragma != __quorum_idle_time_pragma.end() ) {
	_quorum_idle_time = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    }
}
#endif


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
    const std::map<std::string,LQIO::severity_t>::const_iterator pragma = __serverity_level_pragma.find( value );
    if ( pragma != __serverity_level_pragma.end() ) {
	_severity_level = pragma->second;
    } else {
	_severity_level = LQIO::NO_ERROR;
    }
}

void Pragma::setStopOnBogusUtilization(const std::string& value)
{
    char * endptr = nullptr;
    const double temp = std::strtod( value.c_str(), &endptr );
    if ( (temp < 1 && temp != 0) || *endptr != '\0' ) throw std::domain_error( value.c_str() );
    _stop_on_bogus_utilization = temp;
}

void Pragma::setStopOnMessageLoss(const std::string& value)
{
    _stop_on_message_loss = LQIO::DOM::Pragma::isTrue(value);
}

void Pragma::setTau(const std::string& value)
{
    char * endptr = nullptr;
    const unsigned int temp = std::strtol( value.c_str(), &endptr, 10 );
    if ( temp > 20 || *endptr != '\0' ) throw std::domain_error( value.c_str() );
    _tau = temp;
}

void Pragma::setThreads(const std::string& value)
{
    const std::map<std::string,pragma_threads>::const_iterator pragma = __threads_pragma.find( value );
    if ( pragma != __threads_pragma.end() ) {
	_threads = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_exponential_ ) {
	_exponential_paths = true;
    } else {
	throw std::domain_error( value.c_str() );
    }
}
	
void Pragma::setVariance(const std::string& value)
{
    const std::map<std::string,pragma_variance>::const_iterator pragma = __variance_pragma.find( value );
    if ( pragma != __variance_pragma.end() ) {
	_variance = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_no_entry_ ) {
	_entry_variance = false;
    } else if ( value == LQIO::DOM::Pragma::_init_only_ ) {
	_init_variance_only = true;
    } else {
	throw std::domain_error( value.c_str() );
    }
}

/*
 * Print out available pragmas.
 */

std::ostream&
Pragma::usage( std::ostream& output )
{
    Pragma::initialize();

    output << "Valid pragmas: " << std::endl;
    std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );

    for ( std::map<std::string,Pragma::fptr>::const_iterator i = __set_pragma.begin(); i != __set_pragma.end(); ++i ) {
	output << "\t" << std::setw(20) << i->first;
	if ( i->first == LQIO::DOM::Pragma::_tau_ ) {
	    output << " = <int>" << std::endl;
	} else {
	    const std::set<std::string>* args = LQIO::DOM::Pragma::getValues( i->first );
	    if ( args && args->size() > 1 ) {
		output << " = {";

		for ( std::set<std::string>::const_iterator q = args->begin(); q != args->end(); ++q ) {
		    if ( q != args->begin() ) output << ",";
		    output << *q;
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
