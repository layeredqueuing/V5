/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/group.cc $
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
 * $Id: group.cc 14689 2021-05-24 17:58:47Z greg $
 * ------------------------------------------------------------------------
 */

#include "dim.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <lqio/input.h>
#include <lqio/error.h>
#include <lqio/dom_group.h>
#include "errmsg.h"
#include "group.h"
#include "processor.h"
#include "model.h"

/* ------------------------ Constructors etc. ------------------------- */

Group::Group( LQIO::DOM::Group* dom, const Processor * processor )
    : _dom(dom),
      _taskList(),
      _processor(processor),
      _share(0.0),
      _cap(false),
      _replica_number(1)
{ 
    initialize();
}

Group::~Group()
{
}

/*
 * Check that the group share is within range.
 */

bool
Group::check() const
{
    return true;
}

void 
Group::initialize()
{
    /* Do not set share here.
     * in case lqx has yet to set a value to the group share.
     */

}


Group&
Group::recalculateDynamicValues()
{
    /* read gorup share from lqx;  */
    initialize();
    return *this;
}

const Group&
Group::insertDOMResults() const
{
//    _dom->setResultUtilization( getGroupUtil() );
    return *this;
}

/* ----------------------- External functions. ------------------------ */


Group *
Group::find( const std::string& group_name )
{
    std::set<Group *>::const_iterator nextGroup = find_if( Model::__group.begin(), Model::__group.end(), EQStr<Group>( group_name ) );
    if ( nextGroup == Model::__group.end() ) {
	return 0;
    } else {
	return *nextGroup;
    }
}

/*
 * Add a group to the model.   
 */

void 
Group::create( const std::pair<std::string,LQIO::DOM::Group*>& p )
{
    LQIO::DOM::Group* group_dom = p.second;
    const std::string& group_name = p.first;
    
    /* Extract variables from the DOM */
    Processor* processor = Processor::find(group_dom->getProcessor()->getName());
    if (processor == nullptr) { return; }
	
    /* Check that no group was added with the existing name */
    if ( find_if( Model::__group.begin(), Model::__group.end(), EQStr<Group>( group_name ) ) != Model::__group.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", group_name.c_str() );
	return;
    } 
       	
    /* Generate a new group with the parameters and add it to the list */
    // <<<<<<< .mine
    Group * group = new Group( group_dom, processor );

    processor->addGroup(  group );
    Model::__group.insert( group );

    /* Generate a new group with the parameters and add it to the list */
    Model::__group.insert( new Group( group_dom, processor ) );
}
