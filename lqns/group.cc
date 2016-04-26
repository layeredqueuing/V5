/* -*- c++ -*-
 * $HeadURL: svn://localhost/lqn/trunk/lqns/proc.cc $
 * 
 * Everything you wanted to know about a task, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 2008
 *
 * ------------------------------------------------------------------------
 * $Id: group.cc 11963 2014-04-10 14:36:42Z greg $
 * ------------------------------------------------------------------------
 */

#include "dim.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <lqio/input.h>
#include <lqio/error.h>
#include <lqio/dom_group.h>
#include "cltn.h"
#include "errmsg.h"
#include "group.h"
#include "processor.h"

class Group;

set<Group *,ltGroup> group;

/* ---------------------- Overloaded Operators ------------------------ */

ostream&
operator<<( ostream& output, const Group& self )
{
    self.print( output );
    return output;
}

/* ------------------------ Constructors etc. ------------------------- */

Group::Group( const char * aStr, const Processor * aProcessor, const double share, const bool cap )
    : myName(aStr), myProcessor(aProcessor), myShare(share), myCap(cap)
{ 
}

ostream& 
Group::print( ostream& output ) const
{
    return output;
}


/* ----------------------- External functions. ------------------------ */


Group *
Group::find( const char * group_name )
{
    set<Group *,ltGroup>::const_iterator nextGroup = find_if( group.begin(), group.end(), eqGroupStr( group_name ) );
    if ( nextGroup == group.end() ) {
	return 0;
    } else {
	return *nextGroup;
    }
}

void 
add_group(LQIO::DOM::Group* domGroup)
{
    /* Extract variables from the DOM */
    int cap = domGroup->getCap();
    double group_share = domGroup->getGroupShareValue();
    std::string group_name = domGroup->getName().c_str();
    const Processor* aProcessor = Processor::find(domGroup->getProcessor()->getName().c_str());
    if (aProcessor == NULL) { return; }
	
    /* Check that no group was added with the existing name */
    if ( find_if( group.begin(), group.end(), eqGroupStr( group_name.c_str() ) ) != group.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", group_name.c_str() );
	return;
    } 
	
    /* Check that the group share is within range */
    if ( group_share <= 0.0 || 1.0 < group_share ) {
	LQIO::input_error2( LQIO::ERR_INVALID_SHARE, group_name.c_str(), group_share );
    }
	
    /* Generate a new group with the parameters and add it to the list */
    Group * aGroup = new Group( group_name.c_str(), aProcessor, group_share, cap  );
    group.insert( aGroup );
}
