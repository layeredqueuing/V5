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
 * $Id$
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

bool
Group::create() 
{	
    double share = _domGroup->getGroupShareValue();
    double share_sum = 0.0;

    if ( _task_list.size() == 0 ) {
	const vector<LQIO::DOM::Task *>& dom_list = _domGroup->getTaskList();
	for ( vector<LQIO::DOM::Task *>::const_iterator t = dom_list.begin(); t != dom_list.end(); ++t ) {
	    Task * cp = Task::find( (*t)->getName().c_str() );
	    assert( cp );
	    _task_list.push_back(cp);
	    cp->set_group_id(-1);
	}
    } else {
	for ( vector<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	    Task * cp = *t;
	    cp->set_group_id(-1);			// Reset the parasol group id.
	}
    }

    /* Assign all multi-server tasks their own group */

    if ( _total_tasks > 1 ) {
	const double new_share = share / _total_tasks;
	for ( vector<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	    Task * cp = *t;
	    if ( cp->multiplicity() > 1 ) {
		int group_id = ps_build_group( name(), new_share, _processor.node_id(), cap() );
		if ( group_id < 0 ) {
		    LQIO::input_error2( ERR_CANNOT_CREATE_X, "group", name() );
		    return false;
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
	return false;
    }

    for ( vector<Task *>::iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	Task * cp = *t;
	if ( cp->group_id() != -1 ) continue;
	cp->set_group_id(group_id);
    }

    r_util.init( VARIABLE, "Group  %-11.11s - Utilization     ", name() );

    return true;
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

    for ( vector<Task *>::const_iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	Task * cp = *t;
	
	for ( vector<Entry *>::const_iterator next_entry = cp->_entry.begin(); next_entry != cp->_entry.end(); ++next_entry ) {
	    Entry * ep = *next_entry;
	    for ( unsigned p = 1; p <= cp->max_phases; ++p ) {
		proc_util_mean += ep->phase[p].r_cpu_util.mean();
		proc_util_var  += ep->phase[p].r_cpu_util.variance();
	    }
	}
	/* Entry utilization includes activities */
    }

    getDOMGroup()->setResultUtilization(proc_util_mean);
    if ( number_blocks > 1 ) {
	getDOMGroup()->setResultUtilizationVariance(proc_util_var);
    }
}
