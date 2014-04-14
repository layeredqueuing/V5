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
#include <cmath>
#include <stdexcept>
#include <lqio/dom_entity.h>
#include <lqio/dom_extvar.h>

using namespace std;

#if (__GNUC__ < 4 || (__GNUC__ == 4 &&__GNUC_MINOR__ == 0))
const double Place::SERVER_Y_OFFSET = 3.0;
const double Place::CLIENT_Y_OFFSET = 0.5;
const double Place::PLACE_X_OFFSET  = -0.25;
const double Place::PLACE_Y_OFFSET  = 0.35;
#endif

const char * Place::name() const
{
    return _dom->getName().c_str();
}

scheduling_type Place::scheduling() const 
{ 
    return _dom->getSchedulingType(); 
}

/*
 * Check the scheduling type.  Return the default type if the value
 * supplied is not kosher.  Overridden by subclasses if the scheduling
 * type can be something other than FIFO.
 */

bool
Place::scheduling_is_ok( const unsigned bits ) const
{
    return bit_test( static_cast<unsigned>(scheduling()), bits );
}



/*
 * We need a way to fake out infinity... so if copies is infinite, then we change to an infinite server.
 */

unsigned int Place::multiplicity() const
{
    const LQIO::DOM::ExternalVariable * copies = get_dom()->getCopies(); 
    double value;
    if ( !copies ) return 1;
    assert(copies->getValue(value) == true);
    if ( isinf( value ) ) return 1;
    if ( value - floor(value) != 0 ) {
	throw domain_error( "Entity::copies" );
    }
    return static_cast<unsigned int>(value);
}

bool Place::is_infinite() const
{
    const LQIO::DOM::ExternalVariable * copies = get_dom()->getCopies(); 
    double value;
    if ( copies && (copies->wasSet() && copies->getValue(value) == true && isinf( value )) ) {
	return true;
    } else { 
	return scheduling() == SCHEDULE_DELAY; 
    }
}


bool Place::has_random_queueing() const
{
    return scheduling() == SCHEDULE_RAND;
}
