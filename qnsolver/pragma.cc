/*  -*- c++ -*-
 * $Id: pragma.cc 15126 2021-11-25 03:29:42Z greg $ *
 * Pragma processing and definitions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 2021
 * ------------------------------------------------------------------------
 */

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <lqio/glblerr.h>
#include "pragma.h"

Pragma * Pragma::__cache = nullptr;
const std::map<const std::string,const Pragma::fptr> Pragma::__set_pragma =
{
    { LQIO::DOM::Pragma::_multiserver_,			&Pragma::setMultiserver },
    { LQIO::DOM::Pragma::_mva_,				&Pragma::setSolver }
};

/*
 * Set default values in the constructor.  Defaults are used below.
 */

Pragma::Pragma() :
    _multiserver(Model::Multiserver::DEFAULT),
    _solver(Model::Solver::EXACT_MVA)
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


void Pragma::setMultiserver(const std::string& value)
{
    static const std::map<const std::string,const Model::Multiserver> __multiserver_pragma = {
	{ LQIO::DOM::Pragma::_conway_,		Model::Multiserver::CONWAY },
	{ LQIO::DOM::Pragma::_reiser_,		Model::Multiserver::REISER },
	{ LQIO::DOM::Pragma::_reiser_ps_,	Model::Multiserver::REISER_PS },
	{ LQIO::DOM::Pragma::_rolia_,		Model::Multiserver::ROLIA },
	{ LQIO::DOM::Pragma::_rolia_ps_,	Model::Multiserver::ROLIA_PS },
	{ LQIO::DOM::Pragma::_suri_,		Model::Multiserver::SURI },
	{ LQIO::DOM::Pragma::_zhou_,		Model::Multiserver::ZHOU }
    };
    const std::map<const std::string,const Model::Multiserver>::const_iterator pragma = __multiserver_pragma.find( value );
    if ( pragma != __multiserver_pragma.end() ) {
	_multiserver = pragma->second;
    } else if ( value == LQIO::DOM::Pragma::_default_ ) {
	_multiserver = Model::Multiserver::DEFAULT;
    } else {
	throw std::domain_error( value );
    }
}

void Pragma::setSolver(const std::string& value)
{
    static const std::map<const std::string,const Model::Solver> __mva_pragma = {
	{ LQIO::DOM::Pragma::_exact_,	     	Model::Solver::EXACT_MVA },
	{ LQIO::DOM::Pragma::_fast_,		Model::Solver::LINEARIZER2 },
	{ LQIO::DOM::Pragma::_linearizer_,	Model::Solver::LINEARIZER },
	{ LQIO::DOM::Pragma::_schweitzer_,	Model::Solver::BARD_SCHWEITZER }
    };

    const std::map<const std::string,const Model::Solver>::const_iterator pragma = __mva_pragma.find( value );
    if ( pragma != __mva_pragma.end() ) {
	_solver = pragma->second;
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
