/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 2012								*/
/************************************************************************/

/*
 * $Id: petrisrvn.cc 10943 2012-06-13 20:21:13Z greg $
 *
 * Generate a Petri-net from an SRVN description.
 *
 */

#include "petrisrvn.h"
#include "place.h"
#include "errmsg.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <lqio/dom_entity.h>
#include <lqio/dom_extvar.h>

using namespace std;

const double Place::SERVER_Y_OFFSET = 3.0;
const double Place::CLIENT_Y_OFFSET = 0.5;
const double Place::PLACE_X_OFFSET  = -0.25;
const double Place::PLACE_Y_OFFSET  = 0.35;

const char * Place::name() const
{
    return _dom->getName().c_str();
}

scheduling_type Place::get_scheduling() const
{
    return _dom->getSchedulingType();
}

void Place::set_scheduling( scheduling_type scheduling )
{
    const_cast<LQIO::DOM::Entity*>(_dom)->setSchedulingType( scheduling );
}


/*
 * We need a way to fake out infinity... so if copies is infinite, then we change to an infinite server.
 */

unsigned int Place::multiplicity() const
{
    if ( get_scheduling() == SCHEDULE_DELAY ) return 1;	/* Ignore for infinite servers. */

    unsigned int value = 1;
    try {
	value = get_dom()->getCopiesValue();
    }
    catch ( const std::domain_error& e ) {
	if ( !is_infinite() || std::strcmp( e.what(), "infinity" ) != 0 || value != 1 ) {	/* Will throw iff value == infinity */
	    get_dom()->throw_invalid_parameter( "multiplicity", e.what() );
	}
    }
    return value;
}

bool Place::is_infinite() const
{
    return get_dom()->isInfinite();
}


bool Place::has_random_queueing() const
{
    return get_scheduling() == SCHEDULE_RAND;
}



void
Place::check()
{
    if ( !scheduling_is_ok() ) {
	get_dom()->runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label.at(get_scheduling()).str.c_str() );
	set_scheduling( SCHEDULE_FIFO );
    }
    if ( get_dom()->getReplicasValue() != 1 ) {
	get_dom()->runtime_error( LQIO::ERR_NOT_SUPPORTED, "replication" );
    }
}
