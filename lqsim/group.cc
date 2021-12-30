/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Sept 2008								*/
/************************************************************************/

/*
 * Input output processing.
 *
 * $Id: group.cc 15297 2021-12-30 16:21:19Z greg $
 */
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <parasol.h>
#include <assert.h>
#include "lqsim.h"
#include <lqio/input.h>
#include <lqio/error.h>
#include "errmsg.h"
#include "processor.h"
#include "group.h"

std::set<Group *, Group::ltGroup> Group::__groups;


Group::Group( LQIO::DOM::Group * group, const Processor& processor ) 
    : _tasks(),
      _domGroup( group ),
      _processor(processor), 
      _total_tasks(group->getTaskList().size())
{
}


/*
 * Called from Model::create to create ALL groups.  IFF a group has a
 * multiserver, then we re-assign that task to a new sub-group
 */

Group&
Group::create() 
{	
    double share = 0.;
    double share_sum = 0.0;

    try {
	share = _domGroup->getGroupShareValue();
    }
    catch ( const std::domain_error& e ) {
	solution_error( LQIO::ERR_INVALID_PARAMETER, "share", "group", name(), e.what() );
	throw_bad_parameter();
    }

    if ( _task_list.size() == 0 ) {
	const std::set<LQIO::DOM::Task *>& dom_list = _domGroup->getTaskList();
	for ( std::set<LQIO::DOM::Task *>::const_iterator t = dom_list.begin(); t != dom_list.end(); ++t ) {
	    Task * cp = Task::find( (*t)->getName().c_str() );
	    assert( cp );
	    _task_list.insert(cp);
	    cp->set_group_id(-1);
	}
    } else {
	for ( std::set<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	    Task * cp = *t;
	    cp->set_group_id(-1);			// Reset the parasol group id.
	}
    }

    /* Assign all multi-server tasks their own group */

    if ( _total_tasks > 1 ) {
	const double new_share = share / _total_tasks;
	for ( std::set<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	    Task * cp = *t;
	    if ( cp->multiplicity() > 1 ) {
		int group_id = ps_build_group( name(), new_share, _processor.node_id(), cap() );
		if ( group_id < 0 ) {
		    LQIO::input_error2( ERR_CANNOT_CREATE_X, "group", name() );
		    return *this;
		}
		cp->set_group_id( group_id );
		share_sum += new_share;
	    }
	}
    }
    
    /* Now do the remaining tasks - fixed rate, or single multi-processor */

    int group_id = ps_build_group( name(), std::max( 0.0, share - share_sum ), _processor.node_id(), cap() );
    if ( group_id < 0 ) {
	LQIO::input_error2( ERR_CANNOT_CREATE_X, "group", name() );
	return *this;
    }

    for ( std::set<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	Task * cp = *t;
	if ( cp->group_id() != -1 ) continue;
	cp->set_group_id(group_id);
    }

    r_util.init( VARIABLE, "Group  %-11.11s - Utilization     ", name() );

    return *this;
}

/*
 * Find the group and return it.  
 */

Group *
Group::find( const std::string& group_name  )
{
    if ( group_name.empty() ) return nullptr;
    std::set<Group *>::const_iterator group = find_if( Group::__groups.begin(), Group::__groups.end(), eqGroupStr( group_name ) );
    if ( group == Group::__groups.end() ) {
	return nullptr;
    } else {
	return *group;
    }
}

/*----------------------------------------------------------------------*/
/*	 Input processing.  Called from input.y (yyparse()).		*/
/*----------------------------------------------------------------------*/

/*
 * Add a group to the model.
 */

void 
Group::add( const std::pair<std::string,LQIO::DOM::Group*>& p )
{
    const std::string& group_name = p.first;
    LQIO::DOM::Group* domGroup = p.second;

    /* Extract variables from the DOM */
    const char * processor_name = domGroup->getProcessor()->getName().c_str();

    const Processor* aProcessor = Processor::find(processor_name);
    if ( !aProcessor ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, processor_name );
	return;
    } else if ( aProcessor->discipline() != SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, domGroup->getName().c_str(), processor_name );
    }

    if ( Group::find( group_name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", group_name.c_str() );
	return;
    }

    Group * aGroup = new Group( domGroup, *aProcessor );
//    aGroup->set_share( domGroup->getGroupShareValue() );		// set local copy. May update with multiserver.
    __groups.insert( aGroup );
}



/*----------------------------------------------------------------------*/
/*			  Output Functions.				*/
/*----------------------------------------------------------------------*/

Group&
Group::insertDOMResults()
{
    if ( !getDOMGroup() ) return *this;

    double proc_util_mean = 0.0;
    double proc_util_var  = 0.0;

    for ( std::set<Task *>::const_iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	Task * cp = *t;
	
	for ( std::vector<Entry *>::const_iterator entry = cp->_entry.begin(); entry != cp->_entry.end(); ++entry ) {
	    for ( std::vector<Activity>::iterator phase = (*entry)->_phase.begin(); phase != (*entry)->_phase.end(); ++phase ) {
		proc_util_mean += phase->r_cpu_util.mean();
		proc_util_var  += phase->r_cpu_util.variance();
	    }
	}
	/* Entry utilization includes activities */
    }

    getDOMGroup()->setResultUtilization(proc_util_mean);
    if ( number_blocks > 1 ) {
	getDOMGroup()->setResultUtilizationVariance(proc_util_var);
    }
    return *this;
}
