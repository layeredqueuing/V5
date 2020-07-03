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
 * $Id: group.cc 13560 2020-05-26 22:04:49Z greg $
 */
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <parasol.h>
#include <assert.h>
#include "lqsim.h"
#include <lqio/input.h>
#include <lqio/error.h>
#include "errmsg.h"
#include "processor.h"
#include "group.h"

set<Group *, ltGroup> group;


Group::Group( LQIO::DOM::Group * group, const Processor& processor ) 
    : _tasks(),
      _domGroup( group ),
      _processor(processor), 
      _total_tasks(group->getTaskList().size())
{
}


/*
 * Called from Model::create to create ALL groups.  IFF a group has a multiserver, then we re-assign that task to a new sub-group
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
	const set<LQIO::DOM::Task *>& dom_list = _domGroup->getTaskList();
	for ( set<LQIO::DOM::Task *>::const_iterator t = dom_list.begin(); t != dom_list.end(); ++t ) {
	    Task * cp = Task::find( (*t)->getName().c_str() );
	    assert( cp );
	    _task_list.insert(cp);
	    cp->set_group_id(-1);
	}
    } else {
	for ( set<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	    Task * cp = *t;
	    cp->set_group_id(-1);			// Reset the parasol group id.
	}
    }

    /* Assign all multi-server tasks their own group */

    if ( _total_tasks > 1 ) {
	const double new_share = share / _total_tasks;
	for ( set<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
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

    int group_id = ps_build_group( name(), max( 0.0, share - share_sum ), _processor.node_id(), cap() );
    if ( group_id < 0 ) {
	LQIO::input_error2( ERR_CANNOT_CREATE_X, "group", name() );
	return *this;
    }

    for ( set<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
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
Group::find( const char * group_name  )
{
    if ( !group_name ) return 0;
    set<Group *,ltGroup>::const_iterator nextGroup = find_if( ::group.begin(), ::group.end(), eqGroupStr( group_name ) );
    if ( nextGroup == group.end() ) {
	return 0;
    } else {
	return *nextGroup;
    }
}

/*----------------------------------------------------------------------*/
/*	 Input processing.  Called from input.y (yyparse()).		*/
/*----------------------------------------------------------------------*/

/*
 * Add a group to the model.
 */

void 
Group::add( LQIO::DOM::Group* domGroup )
{
    /* Extract variables from the DOM */
    const char * processor_name = domGroup->getProcessor()->getName().c_str();

    const Processor* aProcessor = Processor::find(processor_name);
    if ( !aProcessor ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, processor_name );
	return;
    } else if ( aProcessor->discipline() != SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, domGroup->getName().c_str(), processor_name );
    }

    const char * group_name = domGroup->getName().c_str();
    if ( Group::find( group_name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", group_name );
	return;
    }

    Group * aGroup = new Group( domGroup, *aProcessor );
//    aGroup->set_share( domGroup->getGroupShareValue() );		// set local copy. May update with multiserver.
    group.insert( aGroup );
}



/*----------------------------------------------------------------------*/
/*			  Output Functions.				*/
/*----------------------------------------------------------------------*/

void
Group::insertDOMResults()
{
    if ( !getDOMGroup() ) return;

    double proc_util_mean = 0.0;
    double proc_util_var  = 0.0;

    for ( set<Task *>::const_iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	Task * cp = *t;
	
	for ( vector<Entry *>::const_iterator entry = cp->_entry.begin(); entry != cp->_entry.end(); ++entry ) {
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
}
