/*  -*- c++ -*-
 * $Id: pragma.cc 14292 2020-12-30 16:29:20Z greg $ *
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
std::map<std::string,Pragma::pragma_bcmp> Pragma::__bcmp_pragma;
std::map<std::string,Pragma::fptr> Pragma::__set_pragma;
std::map<std::string,layering_format> Pragma::__layering_pragma;

std::map<std::string,LQIO::severity_t> Pragma::__serverity_level_pragma;

/*
 * Set default values in the constructor.  Defaults are used below.
 */

Pragma::Pragma()
{
    if ( __cache != nullptr ) delete __cache;
    __cache = this;

    initialize();
}


void
Pragma::initialize()
{

    if ( !__set_pragma.empty() ) return;

    __set_pragma[LQIO::DOM::Pragma::_bcmp_] = &Pragma::setBCMP;
    __set_pragma[LQIO::DOM::Pragma::_layering_] = &Pragma::setLayering;
    __set_pragma[LQIO::DOM::Pragma::_prune_] = &Pragma::setPrune;
    __set_pragma[LQIO::DOM::Pragma::_severity_level_] = &Pragma::setSeverityLevel;
    __set_pragma[LQIO::DOM::Pragma::_spex_header_] = &Pragma::setSpexHeader;

    __bcmp_pragma[LQIO::DOM::Pragma::_extended_] = 		BCMP_EXTENDED;
    __bcmp_pragma[LQIO::DOM::Pragma::_lqn_] = 			BCMP_LQN;		/* default */
    __bcmp_pragma[LQIO::DOM::Pragma::_true_] =	 		BCMP_STANDARD;
    __bcmp_pragma[LQIO::DOM::Pragma::_yes_] =	 		BCMP_STANDARD;
    __bcmp_pragma[LQIO::DOM::Pragma::_false_] =	 		BCMP_LQN;
    __bcmp_pragma[LQIO::DOM::Pragma::_no_] =	 		BCMP_LQN;
    __bcmp_pragma["t"] =	 				BCMP_STANDARD;
    __bcmp_pragma["y"] =	 				BCMP_STANDARD;
    __bcmp_pragma["f"] =			 		BCMP_LQN;
    __bcmp_pragma["n"] =	 				BCMP_LQN;
    __bcmp_pragma[""] =	 					BCMP_STANDARD;
    
    __layering_pragma[LQIO::DOM::Pragma::_batched_] =		LAYERING_BATCH;
    __layering_pragma[LQIO::DOM::Pragma::_batched_back_] = 	LAYERING_BATCH;
    __layering_pragma[LQIO::DOM::Pragma::_hwsw_] =	    	LAYERING_HWSW;
    __layering_pragma[LQIO::DOM::Pragma::_mol_] =	    	LAYERING_MOL;
    __layering_pragma[LQIO::DOM::Pragma::_mol_back_] =		LAYERING_MOL;
    __layering_pragma[LQIO::DOM::Pragma::_squashed_] =		LAYERING_SQUASHED;
    __layering_pragma[LQIO::DOM::Pragma::_srvn_] =	    	LAYERING_SRVN;

    __serverity_level_pragma[LQIO::DOM::Pragma::_advisory_] =	LQIO::ADVISORY_ONLY;
    __serverity_level_pragma[LQIO::DOM::Pragma::_run_time_] =	LQIO::RUNTIME_ERROR;
    __serverity_level_pragma[LQIO::DOM::Pragma::_warning_] =	LQIO::WARNING_ONLY;
}


void
Pragma::set( const std::map<std::string,std::string>& list )
{
    initialize();
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

Pragma::pragma_bcmp Pragma::getBCMP()
{
    if ( Flags::bcmp_model ) return BCMP_STANDARD;
    return BCMP_LQN;
}


void Pragma::setBCMP( const std::string& value )
{
    const std::map<std::string,pragma_bcmp>::const_iterator pragma = __bcmp_pragma.find( value );
    if ( input_output() ) return;		// Ignore if generating input.
    if ( pragma != __bcmp_pragma.end() ) {
	switch ( pragma->second ) {
	case BCMP_STANDARD:
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
    const std::map<std::string,layering_format>::const_iterator pragma = __layering_pragma.find( value );
    if ( pragma != __layering_pragma.end() ) {
	Flags::print[LAYERING].value.i = pragma->second;
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
    const std::map<std::string,LQIO::severity_t>::const_iterator pragma = __serverity_level_pragma.find( value );
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

/*
 * Print out available pragmas.
 */

std::ostream&
Pragma::usage( std::ostream& output )
{
    initialize();
    
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
