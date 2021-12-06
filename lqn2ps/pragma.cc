/*  -*- c++ -*-
 * $Id: pragma.cc 15155 2021-12-06 18:54:53Z greg $ *
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

LQIO::DOM::Pragma pragmas;
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
    _force_infinite(ForceInfinite::NONE),
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
	catch ( const std::domain_error& e ) {
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
	{ LQIO::DOM::Pragma::_lqn_,	BCMP::LQN },		/* default */
	{ LQIO::DOM::Pragma::_true_,	BCMP::STANDARD },
	{ LQIO::DOM::Pragma::_yes_,	BCMP::STANDARD },
	{ LQIO::DOM::Pragma::_false_,	BCMP::LQN },
	{ LQIO::DOM::Pragma::_no_,	BCMP::LQN },
	{ "t",				BCMP::STANDARD },
	{ "y",				BCMP::STANDARD },
	{ "f",				BCMP::LQN },
	{ "n",				BCMP::LQN },
	{ "",				BCMP::STANDARD }
    };

    const std::map<const std::string,const BCMP>::const_iterator pragma = __bcmp_pragma.find( value );
    if ( input_output() ) return;		// Ignore if generating input.
    if ( pragma != __bcmp_pragma.end() ) {
	switch ( pragma->second ) {
	case BCMP::STANDARD:
	    Flags::print[QUEUEING_MODEL].opts.value.i = 1;
	    Flags::print[AGGREGATION].opts.value.x = Aggregate::ENTRIES;
	    Flags::bcmp_model = true;
	    break;
	default:
	    Flags::bcmp_model = false;
	    break;
	}
    }
}


/* static */ bool Pragma::forceInfinite( ForceInfinite arg )
{
    assert( __cache != nullptr );
    return (__cache->_force_infinite != ForceInfinite::NONE && arg == __cache->_force_infinite)
	|| (__cache->_force_infinite == ForceInfinite::ALL  && arg != ForceInfinite::NONE );
}



void Pragma::setForceInfinite( const std::string& value )
{
    static const std::map<const std::string,const ForceInfinite> __force_infinite_pragma = {
	{ LQIO::DOM::Pragma::_none_,		ForceInfinite::NONE },
	{ LQIO::DOM::Pragma::_fixed_rate_,	ForceInfinite::FIXED_RATE },
	{ LQIO::DOM::Pragma::_multiservers_,	ForceInfinite::MULTISERVERS },
	{ LQIO::DOM::Pragma::_all_,		ForceInfinite::ALL }
    };

    const std::map<const std::string,const ForceInfinite>::const_iterator pragma = __force_infinite_pragma.find( value );
    if ( pragma != __force_infinite_pragma.end() ) {
	_force_infinite = pragma->second;
    } else {
	throw std::domain_error( value );
    }
}


Layering Pragma::layering()
{
    switch ( Flags::print[LAYERING].opts.value.l ) {
    case Layering::BATCH:
    case Layering::HWSW:
    case Layering::MOL:
    case Layering::SQUASHED:
    case Layering::SRVN:
	return Flags::print[LAYERING].opts.value.l;
	
    default:
	return Layering::BATCH;
    }
}

void Pragma::setLayering(const std::string& value)
{
    static const std::map<const std::string,const Layering> __layering_pragma = {
	{ LQIO::DOM::Pragma::_batched_,		Layering::BATCH },
	{ LQIO::DOM::Pragma::_batched_back_,	Layering::BATCH },
	{ LQIO::DOM::Pragma::_hwsw_,		Layering::HWSW },
	{ LQIO::DOM::Pragma::_mol_,		Layering::MOL },
	{ LQIO::DOM::Pragma::_mol_back_,	Layering::MOL },
	{ LQIO::DOM::Pragma::_squashed_,	Layering::SQUASHED },
	{ LQIO::DOM::Pragma::_srvn_,		Layering::SRVN }
    };

    const std::map<const std::string,const Layering>::const_iterator pragma = __layering_pragma.find( value );
    if ( pragma != __layering_pragma.end() ) {
	Flags::print[LAYERING].opts.value.l = pragma->second;
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
    LQIO::io_vars.severity_level = LQIO::DOM::Pragma::getSeverityLevel( value );
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
