/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/************************************************************************/

/*
 * $Id: call.cc 17261 2024-09-07 19:42:53Z greg $
 *
 * Generate a Petri-net from an SRVN description.
 *
 */

#include <cmath>
#include <sstream>
#include <lqio/dom_call.h>
#include "call.h"
#include "phase.h"

bool
Call::is_rendezvous() const
{
    return _dom->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS;
}


bool
Call::is_send_no_reply() const
{
    return _dom->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY;
}

double
Call::value( const Phase * src, double upper_limit ) const
{
    try {
	const double value = _dom->getCallMeanValue();
	if ( src->has_deterministic_calls() ) {
	    if ( value != trunc(value) ) throw std::domain_error( "invalid integer" );
	} else if ( 0 < upper_limit && upper_limit < value ) {
	    std::stringstream ss;
	    ss << value << " > " << upper_limit;
	    throw std::domain_error( ss.str() );
	}
	return value;
    }
    catch ( const std::domain_error &e ) {
	_dom->throw_invalid_parameter( "mean value", e.what() );
    }
    return 0.;
}
