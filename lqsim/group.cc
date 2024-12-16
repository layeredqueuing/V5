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
 * $Id: group.cc 17501 2024-11-27 21:32:50Z greg $
 */

#include "lqsim.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <assert.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include "entry.h"
#include "errmsg.h"
#include "processor.h"
#include "group.h"

std::set<Group *, Group::ltGroup> Group::__groups;


Group::Group( LQIO::DOM::Group * dom, const Processor& processor ) 
    : _dom(dom),
      _processor(processor), 
      _total_tasks(dom->getTaskList().size()),
      r_util("Utilization",dom),
      _tasks()
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
	share = getDOM()->getGroupShareValue();
    }
    catch ( const std::domain_error& e ) {
	getDOM()->throw_invalid_parameter( "share", e.what() );
    }

    if ( _tasks.empty() ) {
	const std::set<LQIO::DOM::Task *>& dom_list = getDOM()->getTaskList();
	for ( std::set<LQIO::DOM::Task *>::const_iterator t = dom_list.begin(); t != dom_list.end(); ++t ) {
	    Task * cp = Task::find( (*t)->getName() );
	    assert( cp );
	    _tasks.insert(cp);
	    cp->setGroup(this);
#if HAVE_PARASOL
	    cp->set_group_id(-1);
#endif
	}
    } else {
	std::for_each( tasks().begin(), tasks().end(), []( Task * task ){ task->setGroup(nullptr); } );
#if HAVE_PARASOL
	std::for_each( tasks().begin(), tasks().end(), []( Task * task ){ task->set_group_id(-1); } );
#endif
    }

    /* Assign all multi-server tasks their own group */

    if ( _total_tasks > 1 ) {
	const double new_share = share / _total_tasks;
	for ( std::set<Task *>::iterator t = tasks().begin(); t != tasks().end(); ++t ) {
	    Task * cp = *t;
	    if ( cp->multiplicity() > 1 ) {
#if HAVE_PARASOL
		int group_id = ps_build_group( name(), new_share, _processor.node_id(), cap() );
		if ( group_id < 0 ) {
		    LQIO::input_error( ERR_CANNOT_CREATE_X, "group", name() );
		    return *this;
		}
		cp->set_group_id( group_id );
#endif
		share_sum += new_share;
	    }
	}
    }
    
    /* Now do the remaining tasks - fixed rate, or single multi-processor */

#if HAVE_PARASOL
    int group_id = ps_build_group( name(), std::max( 0.0, share - share_sum ), _processor.node_id(), cap() );
    if ( group_id < 0 ) {
	LQIO::input_error( ERR_CANNOT_CREATE_X, "group", name() );
	return *this;
    }
    std::for_each( tasks().begin(), tasks().end(), [=]( Task * task ){ if ( task->group_id() == -1 ) { task->set_group_id( group_id ); } } );
#endif

    std::for_each( tasks().begin(), tasks().end(), [=]( Task * task ){ if ( task->group() == nullptr ) { task->setGroup( this ); } } );

    return *this;
}

/*
 * Find the group and return it.  
 */

Group *
Group::find( const std::string& name  )
{
    if ( name.empty() ) return nullptr;
    std::set<Group *>::const_iterator group = find_if( Group::__groups.begin(), Group::__groups.end(), [=]( const Group * group ){ return group->name() ==  name; } );
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
    LQIO::DOM::Group* dom = p.second;

    if ( Group::find( group_name ) ) {
	dom->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return;
    }

    /* Extract variables from the DOM */
    const char * processor_name = dom->getProcessor()->getName().c_str();

    const Processor* processor = Processor::find(processor_name);
    if ( !processor ) {
	LQIO::input_error( LQIO::ERR_NOT_DEFINED, processor_name );
	return;
    } else if ( processor->discipline() != SCHEDULE_CFS ) {
	dom->input_error( LQIO::WRN_NON_CFS_PROCESSOR, processor_name );
    }

    Group * group = new Group( dom, *processor );
//    group->set_share( dom->getGroupShareValue() );		// set local copy. May update with multiserver.
    __groups.insert( group );
}



/*----------------------------------------------------------------------*/
/*			  Output Functions.				*/
/*----------------------------------------------------------------------*/

Group&
Group::insertDOMResults()
{
    if ( !getDOM() ) return *this;

    double proc_util_mean = 0.0;
    double proc_util_var  = 0.0;

    for ( std::set<Task *>::const_iterator t = tasks().begin(); t != tasks().end(); ++t ) {
	Task * cp = *t;
	
	for ( std::vector<Entry *>::const_iterator entry = cp->entries().begin(); entry != cp->entries().end(); ++entry ) {
	    for ( std::vector<Activity>::iterator phase = (*entry)->_phase.begin(); phase != (*entry)->_phase.end(); ++phase ) {
		proc_util_mean += phase->r_cpu_util.mean();
		proc_util_var  += phase->r_cpu_util.variance();
	    }
	}
	/* Entry utilization includes activities */
    }

    getDOM()->setResultUtilization(proc_util_mean);
    if ( number_blocks > 1 ) {
	getDOM()->setResultUtilizationVariance(proc_util_var);
    }
    return *this;
}
