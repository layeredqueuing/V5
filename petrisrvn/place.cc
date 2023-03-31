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

    const LQIO::DOM::ExternalVariable * copies = get_dom()->getCopies();
    if ( copies == nullptr ) return 1;
    double value = 1.;
    assert(copies->getValue(value) == true);
    if ( isinf( value ) ) return 1;
    std::stringstream err;
    if ( value < 1 ) {
	err << value << " < " << 1;
    } else if ( value != floor(value)  ) {
	err << "invalid integer";
    }
    if ( err.str().size() > 0 ) {
	get_dom()->throw_invalid_parameter( "multiplicity", err.str() );
    }
    return static_cast<unsigned int>(value);
}

bool Place::is_infinite() const
{
    if ( get_scheduling() == SCHEDULE_DELAY ) {
	return true;
    } else {
	double value = 0.0;
	const LQIO::DOM::ExternalVariable * copies = get_dom()->getCopies();
	return  copies && (copies->wasSet() && copies->getValue(value) == true && isinf( value ));
    }
}


bool Place::has_random_queueing() const
{
    return get_scheduling() == SCHEDULE_RAND;
}



void
Place::check()
{
    if ( get_dom()->getReplicasValue() != 1 ) {
	get_dom()->runtime_error( LQIO::ERR_NOT_SUPPORTED, "replication" );
    }
}
