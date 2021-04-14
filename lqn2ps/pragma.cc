/*  -*- c++ -*-
 * $Id: pragma.cc 14596 2021-04-14 15:17:08Z greg $ *
 * Pragma processing and definitions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 2020
 * ------------------------------------------------------------------------
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <lqio/srvn_spex.h>
#include <lqio/glblerr.h>
#include "pragma.h"

Pragma * Pragma::__cache = nullptr;
std::map<const std::string,const Pragma::fptr> Pragma::__set_pragma =
{
    { LQIO::DOM::Pragma::_bcmp_, &Pragma::setBCMP },
    { LQIO::DOM::Pragma::_force_infinite_, &Pragma::setForceInfinite },
    { LQIO::DOM::Pragma::_layering_, &Pragma::setLayering },
    { LQIO::DOM::Pragma::_processor_scheduling_, &Pragma::setProcessorScheduling },
    { LQIO::DOM::Pragma::_prune_, &Pragma::setPrune },
    { LQIO::DOM::Pragma::_severity_level_, &Pragma::setSeverityLevel },
    { LQIO::DOM::Pragma::_spex_header_, &Pragma::setSpexHeader },
    { LQIO::DOM::Pragma::_task_scheduling_, &Pragma::setTaskScheduling }
};

/*
 * Set default values in the constructor.  Defaults are used below.
 */

Pragma::Pragma() :
    _force_infinite(Force_Infinite::NONE),
    _processor_scheduling(SCHEDULE_PS),
    _task_scheduling(SCHEDULE_FIFO),
    _default_processor_scheduling(true),
    _default_task_scheduling(true)
{
    if ( __cache != nullptr ) delete __cache;
    __cache = this;
}


void
Pragma::set( const std::map<std::string,std::string>& list )
{
    if ( __cache != nullptr ) delete __cache;
    __cache = new Pragma();

    std::for_each( list.begin(), list.end(), set_pragma );
}


void Pragma::set_pragma( const std::pair<std::string,std::string>& p )
{
    const std::string& param = p.first;
    const std::map<const std::string,const fptr>::const_iterator j = __set_pragma.find(param);
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

Pragma::BCMP Pragma::getBCMP()
{
    if ( Flags::bcmp_model ) return BCMP::STANDARD;
    return BCMP::LQN;
}


void Pragma::setBCMP( const std::string& value )
{
    static const std::map<const std::string,const Pragma::BCMP> __bcmp_pragma = {
	{ LQIO::DOM::Pragma::_extended_,BCMP::EXTENDED },
	{ LQIO::DOM::Pragma::_lqn_,     BCMP::LQN },		/* default */
	{ LQIO::DOM::Pragma::_true_,    BCMP::STANDARD },
	{ LQIO::DOM::Pragma::_yes_,	BCMP::STANDARD },
	{ LQIO::DOM::Pragma::_false_,   BCMP::LQN },
	{ LQIO::DOM::Pragma::_no_,	BCMP::LQN },
	{ "t",	 		     	BCMP::STANDARD },
	{ "y",	 		     	BCMP::STANDARD },
	{ "f",			     	BCMP::LQN },
	{ "n",	 		     	BCMP::LQN },
	{ "",	 		     	BCMP::STANDARD }
    };

    const std::map<const std::string,const BCMP>::const_iterator pragma = __bcmp_pragma.find( value );
    if ( input_output() ) return;		// Ignore if generating input.
    if ( pragma != __bcmp_pragma.end() ) {
	switch ( pragma->second ) {
	case BCMP::STANDARD:
	    Flags::print[QUEUEING_MODEL].value.i = 1;
	    Flags::print[AGGREGATION].value.i = AGGREGATE_ENTRIES;
	    Flags::bcmp_model = true;
	    break;
	default:
	    Flags::bcmp_model = false;
	    break;
	}
    }
}


/* static */ bool Pragma::forceInfinite( Force_Infinite value )
{
    assert( __cache != nullptr );
    return value != Force_Infinite::NONE
	&& ( __cache->_force_infinite == Force_Infinite::ALL
	     || __cache->_force_infinite == value );
}



void Pragma::setForceInfinite( const std::string& value )
{
    static const std::map<const std::string,Force_Infinite> __force_infinite_pragma = {
	{ LQIO::DOM::Pragma::_none_, 	    Force_Infinite::NONE },
	{ LQIO::DOM::Pragma::_fixed_rate_,  Force_Infinite::FIXED_RATE },
	{ LQIO::DOM::Pragma::_multiservers_,Force_Infinite::MULTISERVERS },
	{ LQIO::DOM::Pragma::_all_, 	    Force_Infinite::ALL }
    };

    const std::map<const std::string,Force_Infinite>::const_iterator pragma = __force_infinite_pragma.find( value );
    if ( pragma != __force_infinite_pragma.end() ) {
	_force_infinite = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    }
}


layering_format Pragma::layering()
{
    switch ( Flags::print[LAYERING].value.i ) {
    case LAYERING_BATCH:
    case LAYERING_HWSW:
    case LAYERING_MOL:
    case LAYERING_SQUASHED:
    case LAYERING_SRVN:
	return static_cast<layering_format>(Flags::print[LAYERING].value.i);
	
    default:
	return LAYERING_BATCH;
    }
}

void Pragma::setLayering(const std::string& value)
{
    static const std::map<const std::string,const layering_format> __layering_pragma = {
	{ LQIO::DOM::Pragma::_batched_,	    LAYERING_BATCH },
	{ LQIO::DOM::Pragma::_batched_back_,LAYERING_BATCH },
	{ LQIO::DOM::Pragma::_hwsw_,	    LAYERING_HWSW },
	{ LQIO::DOM::Pragma::_mol_,	    LAYERING_MOL },
	{ LQIO::DOM::Pragma::_mol_back_,    LAYERING_MOL },
	{ LQIO::DOM::Pragma::_squashed_,    LAYERING_SQUASHED },
	{ LQIO::DOM::Pragma::_srvn_,	    LAYERING_SRVN }
    };

    const std::map<const std::string,const layering_format>::const_iterator pragma = __layering_pragma.find( value );
    if ( pragma != __layering_pragma.end() ) {
	Flags::print[LAYERING].value.i = pragma->second;
    } else {
	throw std::domain_error( value.c_str() );
    } 
}


void Pragma::setProcessorScheduling(const std::string& value)
{
    static const std::map<const std::string,const scheduling_type> __processor_scheduling_pragma = {
	{ scheduling_label[SCHEDULE_DELAY].XML, SCHEDULE_DELAY },
	{ scheduling_label[SCHEDULE_FIFO].XML,  SCHEDULE_FIFO },
	{ scheduling_label[SCHEDULE_HOL].XML,   SCHEDULE_HOL },
	{ scheduling_label[SCHEDULE_PPR].XML,   SCHEDULE_PPR },
	{ scheduling_label[SCHEDULE_PS].XML,    SCHEDULE_PS },
	{ scheduling_label[SCHEDULE_RAND].XML,  SCHEDULE_RAND }
    };

    const std::map<const std::string,const scheduling_type>::const_iterator pragma = __processor_scheduling_pragma.find( value );
    if ( pragma != __processor_scheduling_pragma.end() ) {
	_default_processor_scheduling = false;
	_processor_scheduling = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_default_processor_scheduling = true;
    } else {
	throw std::domain_error( value.c_str() );
    }
}


void Pragma::setPrune(const std::string& value)
{
    if ( input_output() ) return;		// Ignore if generating input.
    Flags::prune = LQIO::DOM::Pragma::isTrue(value);
}


LQIO::severity_t severityLevel()
{
    return LQIO::io_vars.severity_level;
}


void Pragma::setSeverityLevel(const std::string& value)
{
    static const std::map<const std::string,const LQIO::severity_t> __serverity_level_pragma = {
	{ LQIO::DOM::Pragma::_advisory_,LQIO::ADVISORY_ONLY },
	{ LQIO::DOM::Pragma::_run_time_,LQIO::RUNTIME_ERROR },
	{ LQIO::DOM::Pragma::_warning_,	LQIO::WARNING_ONLY }
    };

    const std::map<const std::string,const LQIO::severity_t>::const_iterator pragma = __serverity_level_pragma.find( value );
    if ( pragma != __serverity_level_pragma.end() ) {
	LQIO::io_vars.severity_level = pragma->second;
    } else {
	LQIO::io_vars.severity_level = LQIO::NO_ERROR;
    }
}

bool Pragma::spexHeader()
{
    return !LQIO::Spex::__no_header;
}


void Pragma::setSpexHeader(const std::string& value)
{
    LQIO::Spex::__no_header = !LQIO::DOM::Pragma::isTrue( value );
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
	throw std::domain_error( value.c_str() );
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

    for ( std::map<const std::string,const Pragma::fptr>::const_iterator i = __set_pragma.begin(); i != __set_pragma.end(); ++i ) {
	output << "\t" << std::setw(20) << i->first;
	if ( i->first == LQIO::DOM::Pragma::_tau_ ) {
	    output << " = <int>" << std::endl;
	} else {
	    const std::set<std::string>* args = LQIO::DOM::Pragma::getValues( i->first );
	    if ( args && args->size() > 1 ) {
		output << " = {";

		size_t count = 0;
		for ( std::set<std::string>::const_iterator q = args->begin(); q != args->end(); ++q ) {
		    if ( q->empty() ) continue;
		    if ( count > 0 ) output << ",";
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
