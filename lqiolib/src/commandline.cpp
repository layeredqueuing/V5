/*  -*- c++ -*-
 * $Id: commandline.cpp 15220 2021-12-15 15:18:47Z greg $
 *
 * File name generation.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * 2011
 *
 * ------------------------------------------------------------------------
 */

#include "commandline.h"

LQIO::CommandLine& 
LQIO::CommandLine::operator=( const std::string& s )
{
    _s = s;
    return *this;
}

LQIO::CommandLine& 
LQIO::CommandLine::append( int c, const char * optarg )
{
#if HAVE_GETOPT_LONG
    const option * o = _longopts;
    while ( o->name && c != o->val ) ++o;

    if ( o->name ) {
	_s += " --";
	_s += o->name;
	if ( optarg ) {
	    _s += "=";
	}
    } else {
	_s += " -";
	_s += c;
    }
#else
    _s += " -";
    _s += c;
#endif

    if ( optarg ) {
	_s += optarg;
    }	

    return *this;
}
