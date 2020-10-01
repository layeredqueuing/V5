/*  -*- c++ -*-
 * $Id: pragma.cc 13905 2020-10-01 11:32:09Z greg $ *
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

Pragma * Pragma::__cache = nullptr;
std::map<std::string,Pragma::fptr> Pragma::__set_pragma;

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
    __set_pragma[LQIO::DOM::Pragma::_severity_level_] = &Pragma::setSeverityLevel;
    __set_pragma[LQIO::DOM::Pragma::_spex_header_] = &Pragma::setSpexHeader;
    __set_pragma[LQIO::DOM::Pragma::_stop_on_bogus_utilization_] = &Pragma::setStopOnBogusUtilization;
    __set_pragma[LQIO::DOM::Pragma::_stop_on_message_loss_] = &Pragma::setStopOnMessageLoss;
    __set_pragma[LQIO::DOM::Pragma::_tau_] = &Pragma::setTau;
    __set_pragma[LQIO::DOM::Pragma::_threads_] = &Pragma::setThreads;
    __set_pragma[LQIO::DOM::Pragma::_variance_] = &Pragma::setVariance;
}
    
void
Pragma::set( const std::map<std::string,std::string>& list )
{
    initialize();
    
    if ( __cache != nullptr ) delete __cache;
    __cache = new Pragma();

    for ( std::map<std::string,std::string>::const_iterator i = list.begin(); i != list.end(); ++i ) {
	const std::string& param = i->first;
	const std::map<std::string,fptr>::const_iterator j = __set_pragma.find(param);
	if ( j != __set_pragma.end() ) {
	    fptr f = j->second;
	    (__cache->*f)(i->second);
	}
    }
}


void Pragma::setAllowCycles(const std::string& value)
{
    _allow_cycles = isTrue(value);
}

void Pragma::setForceMultiserver(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_all_ ) {
	_force_multiserver = FORCE_ALL;
    } else if ( value == LQIO::DOM::Pragma::_tasks_ ) {
	_force_multiserver = FORCE_TASKS;
    } else if ( value == LQIO::DOM::Pragma::_processors_ ) {
	_force_multiserver = FORCE_PROCESSORS;
    } else {
	_force_multiserver = FORCE_NONE;
    }
}

void Pragma::setInterlock(const std::string& value)
{
    _interlock = isTrue(value);
}

void Pragma::setLayering(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_batched_back_ ) {
	_layering = BACKPROPOGATE_LAYERS;
    } else if ( value == LQIO::DOM::Pragma::_mol_ ) {
	_layering = METHOD_OF_LAYERS;
    } else if ( value == LQIO::DOM::Pragma::_mol_back_ ) {
	_layering = BACKPROPOGATE_METHOD_OF_LAYERS;
    } else if ( value == LQIO::DOM::Pragma::_srvn_ ) {
	_layering = SRVN_LAYERS;
    } else if ( value == LQIO::DOM::Pragma::_squashed_ ) {
	_layering = SQUASHED_LAYERS;
    } else if ( value == LQIO::DOM::Pragma::_hwsw_ ) {
	_layering = HWSW_LAYERS;
    } else {
	_layering = BATCHED_LAYERS;
    }
}

void Pragma::setMultiserver(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_conway_ ) {
	_multiserver = CONWAY_MULTISERVER;
    } else if ( value == LQIO::DOM::Pragma::_reiser_ ) {
	_multiserver = REISER_MULTISERVER;
    } else if ( value == LQIO::DOM::Pragma::_reiser_ps_ ) {
	_multiserver = REISER_PS_MULTISERVER;
    } else if ( value == LQIO::DOM::Pragma::_rolia_ ) {
	_multiserver = ROLIA_MULTISERVER;
    } else if ( value == LQIO::DOM::Pragma::_rolia_ps_ ) {
	_multiserver = ROLIA_PS_MULTISERVER;
    } else if ( value == LQIO::DOM::Pragma::_bruell_ ) {
	_multiserver = BRUELL_MULTISERVER;
    } else if ( value == LQIO::DOM::Pragma::_schmidt_ ) {
	_multiserver = SCHMIDT_MULTISERVER;
    } else if ( value == LQIO::DOM::Pragma::_suri_ ) {
	_multiserver = SURI_MULTISERVER;
    } else {
	_multiserver = DEFAULT_MULTISERVER;
    }
}

void Pragma::setMva(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_exact_ ) {
	_mva = EXACT_MVA;
    } else if ( value == LQIO::DOM::Pragma::_schweitzer_ ) {
	_mva = SCHWEITZER_MVA;
    } else if ( value == LQIO::DOM::Pragma::_fast_ ) {
	_mva = FAST_MVA;
    } else if ( value == LQIO::DOM::Pragma::_one_step_ ) {
	_mva = ONESTEP_MVA;
    } else if ( value == LQIO::DOM::Pragma::_one_step_linearizer_ ) {
	_mva = ONESTEP_LINEARIZER;
    } else {
	_mva = LINEARIZER_MVA;
    }
}


void Pragma::setOvertaking(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_markov_ ) {
	_overtaking = MARKOV_OVERTAKING;
    } else if ( value == LQIO::DOM::Pragma::_rolia_ ) {
	_overtaking = ROLIA_OVERTAKING;
    } else if ( value == LQIO::DOM::Pragma::_rolia_ ) {
	_overtaking = SIMPLE_OVERTAKING;
    } else if ( value == LQIO::DOM::Pragma::_rolia_ ) {
	_overtaking = SPECIAL_OVERTAKING;
    } else {
	_overtaking = NO_OVERTAKING;
    }
}

void Pragma::setProcessorScheduling(const std::string& value)
{
    _default_processor_scheduling = false;
    if ( value == scheduling_label[SCHEDULE_DELAY].XML ) {
	_processor_scheduling = SCHEDULE_DELAY;
    } else if ( value == scheduling_label[SCHEDULE_FIFO].XML ) {
	_processor_scheduling = SCHEDULE_FIFO;
    } else if ( value == scheduling_label[SCHEDULE_HOL].XML ) {
	_processor_scheduling = SCHEDULE_HOL;
    } else if ( value == scheduling_label[SCHEDULE_PPR].XML ) {
	_processor_scheduling = SCHEDULE_PPR;
    } else if ( value == scheduling_label[SCHEDULE_PS].XML ) {
	_processor_scheduling = SCHEDULE_PS;
    } else if ( value == scheduling_label[SCHEDULE_RAND].XML ) {
	_processor_scheduling = SCHEDULE_RAND;
    } else {
	_default_processor_scheduling = true;
    }
}

void Pragma::setSpexHeader(const std::string& value)
{
    _spex_header = isTrue( value );
}

void Pragma::setSeverityLevel(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_warning_ ) {
	_severity_level = LQIO::WARNING_ONLY;
    } else if ( value == LQIO::DOM::Pragma::_advisory_) {
	_severity_level = LQIO::ADVISORY_ONLY;
    } else if ( value == LQIO::DOM::Pragma::_run_time_) {
	_severity_level = LQIO::RUNTIME_ERROR;
    } else {
	_severity_level = LQIO::NO_ERROR;
    }
}

void Pragma::setStopOnBogusUtilization(const std::string& value)
{
    char * endptr = nullptr;
    _stop_on_bogus_utilization = std::strtod( value.c_str(), &endptr );
    if ( (_stop_on_bogus_utilization < 1 && _stop_on_bogus_utilization != 0) || *endptr != '\0' ) throw std::domain_error( "Invalid stop_on_bogus_utilization" );
}

void Pragma::setStopOnMessageLoss(const std::string& value)
{
    _stop_on_message_loss = isTrue(value);
}

void Pragma::setTau(const std::string& value)
{
    char * endptr = nullptr;
    _tau = std::strtol( value.c_str(), &endptr, 10 );
    if ( _tau > 20 || *endptr != '\0' ) throw std::domain_error( "Invalid tau" );
}

void Pragma::setThreads(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_exponential_ ) {
	_exponential_paths = true;
    } else if ( value == LQIO::DOM::Pragma::_hyper_ ) {
	_threads = HYPER_THREADS; 
    } else if ( value == LQIO::DOM::Pragma::_mak_ ) {
	_threads = MAK_LUNDSTROM_THREADS;
    } else {
	_threads = NO_THREADS;
    }
}
	
void Pragma::setVariance(const std::string& value)
{
    if ( value == LQIO::DOM::Pragma::_no_entry_ ) {
	_entry_variance = false;
    } else if ( value == LQIO::DOM::Pragma::_init_only_ ) {
	_init_variance_only = true;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_variance = DEFAULT_VARIANCE;
    } else if ( value == LQIO::DOM::Pragma::_stochastic_ ) {
	_variance = STOCHASTIC_VARIANCE;
    } else if ( value == LQIO::DOM::Pragma::_mol_ ) {
	_variance = MOL_VARIANCE;
    } else {
	_variance = NO_VARIANCE;
    }
}


bool Pragma::isTrue(const std::string& value )
{
    return value == "true" || value == LQIO::DOM::Pragma::_yes_;
}

/*
 * Print out available pragmas.
 */

ostream&
Pragma::usage( ostream& output )
{
    Pragma::initialize();
    
    output << "Valid pragmas: " << endl;
    ios_base::fmtflags flags = output.setf( ios::left, ios::adjustfield );

    for ( std::map<std::string,Pragma::fptr>::const_iterator i = __set_pragma.begin(); i != __set_pragma.end(); ++i ) {
	output << "\t" << std::setw(20) << i->first;
	if ( i->first == LQIO::DOM::Pragma::_tau_ ) {
	    output << " = <int>" << endl;
	} else {
	    const std::set<std::string>* args = LQIO::DOM::Pragma::getValues( i->first );
	    if ( args && args->size() > 1 ) {
		output << " = {";

		for ( std::set<std::string>::const_iterator q = args->begin(); q != args->end(); ++q ) {
		    if ( q != args->begin() ) output << ",";
		    output << *q;
		}
		output << "}" << endl;
	    } else {
		output << " = <arg>" << endl;
	    }
	}
    }
    output.setf( flags );
    return output;
}
