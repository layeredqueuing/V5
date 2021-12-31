/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* July 2003								*/
/************************************************************************/

/*
 * Activities are arcs in the graph that do work.
 * Nodes are points in the graph where splits and joins take place.
 *
 * $Id: actlist.cc 15304 2021-12-31 15:51:38Z greg $
 */

#include "lqsim.h"
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <deque>
#include <sstream>
#include <lqio/dom_actlist.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include <parasol.h>
#include "model.h"
#include "activity.h"
#include "task.h"
#include "instance.h"
#include "errmsg.h"
#include "processor.h"

static bool set_join_type( ActivityList * join_list, join_type_t a_type );
static bool add_to_join_list( ActivityList * join_list, unsigned i, Activity * an_activity );
static inline int i_max( int a, int b ) { return a > b ? a : b; }
static void activity_path_error( int, const ActivityList *, std::deque<Activity *>& activity_stack );
static void activity_cycle_error( int err, const char * task_name, std::deque<Activity *>& activity_stack );
static char * fork_join_name( const ActivityList * );
static std::deque<ActivityList *>::iterator fork_backtrack( Activity *, std::deque<ActivityList *>&, Activity * );
static std::deque<ActivityList *>::iterator join_backtrack( ActivityList *, std::deque<ActivityList *>&, Activity * );

void
ActivityList::free()
{
    ::free( list );
    switch ( type ) {
    case ACT_OR_FORK_LIST:
	::free( u.fork.prob );
	break;
    case ACT_AND_FORK_LIST:
	::free( u.fork.visit );
	break;
    case ACT_LOOP_LIST:
	::free( u.loop.count );
	break;
    case ACT_AND_JOIN_LIST:
	::free( u.join.fork );
	if ( u.join._hist_data ) {
	    delete u.join._hist_data;
	}
	break;
    }
    ::free( this );
}



ActivityList&
ActivityList::configure()
{
    double sum = 0.0;
    switch ( type ) {
    case ACT_AND_JOIN_LIST:
	u.join.quorumCount = dynamic_cast<LQIO::DOM::AndJoinActivityList*>(_dom_actlist)->getQuorumCountValue();
/* Histogram to go here */
	if ( _dom_actlist ) {
	    if ( !u.join._hist_data && _dom_actlist->hasHistogram() ) {
		u.join._hist_data = new Histogram( _dom_actlist->getHistogram() );
	    }
	}
	break;

    case ACT_OR_FORK_LIST:
	for ( unsigned i = 0; i < na; ++i ) {
	    const double prob = _dom_actlist->getParameterValue(dynamic_cast<LQIO::DOM::Activity *>(list[i]->get_DOM()));
	    u.fork.prob[i] = prob;
	    sum += prob;
	}
	if ( sum > 1.00001 ) {
	}
	break;

    case ACT_LOOP_LIST:
	u.loop.total = 0;
	for ( unsigned i = 0; i < na; ++i ) {
	    const double value = _dom_actlist->getParameterValue(dynamic_cast<LQIO::DOM::Activity *>(list[i]->get_DOM()));
	    u.loop.count[i] = value;
	    u.loop.total += value;
	}
	break;
    }
    return *this;
}



ActivityList&
ActivityList::initialize()
{
    return *this;
}


bool
ActivityList::is_join_list() const
{
    return type == ACT_JOIN_LIST || type == ACT_AND_JOIN_LIST || type == ACT_OR_JOIN_LIST;
}


bool
ActivityList::is_fork_list() const
{
    return type == ACT_FORK_LIST || type == ACT_AND_FORK_LIST || type == ACT_OR_FORK_LIST;
}

bool
ActivityList::is_loop_list() const 
{
    return type == ACT_LOOP_LIST;
}

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

double
join_find_children( ActivityList * join_list, std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0.0;
    Activity * my_activity = activity_stack.back();

    switch ( join_list->type ) {
    case ACT_AND_JOIN_LIST:

	/* Look for the fork on the fork stack */
	for ( unsigned i = 0; i < join_list->na; ++i ) {
	    if ( join_list->list[i] == my_activity ) continue;
	    try {
		std::deque<ActivityList *>::iterator j = fork_backtrack( join_list->list[i], fork_stack, join_list->list[i] );
		if ( j != fork_stack.end() ) {
		    if ( !set_join_type( join_list, JOIN_INTERNAL_FORK_JOIN ) ) {
			activity_path_error( LQIO::ERR_JOIN_BAD_PATH, join_list, activity_stack );
		    } else if ( !join_list->u.join.fork[i] || std::find( fork_stack.begin(), fork_stack.end(), join_list->u.join.fork[i] ) != fork_stack.end() ) {
			join_list->u.join.fork[i] = *j;
			join_list->u.join.fork[i]->u.fork.join = join_list;
		    }
		    if ( debug_flag ) {
			Activity * ap = activity_stack.back();
			std::string buf = "And Join: ";
			for ( size_t k = 0; k < activity_stack.size(); ++k ) buf += ' ';
			buf += ap->name();
			buf += ' ';
			buf += join_list->list[i]->name();
			for ( std::deque<ActivityList *>::const_reverse_iterator lp = fork_stack.rbegin(); lp != fork_stack.rend(); ++lp ) {
			    buf += ' ';
			    buf += (*lp)->u.fork.prev->list[0]->name();
			}
			fprintf( stddbg, "%s\n", buf.c_str() );
		    }
		} else {
		    if ( !set_join_type( join_list, JOIN_SYNCHRONIZATION ) ) {
			activity_path_error( LQIO::ERR_JOIN_BAD_PATH, join_list, activity_stack );
		    } else if ( !add_to_join_list( join_list, i, activity_stack.back() ) ) {
			activity_path_error( LQIO::ERR_JOIN_BAD_PATH, join_list, activity_stack );
		    } else {
			Server_Task * cp = dynamic_cast<Server_Task *>(ep->task());
			cp->set_synchronization_server();
		    }
		}
	    }
	    catch ( Activity * ) {
		Server_Task * cp = dynamic_cast<Server_Task *>(ep->task());
		activity_cycle_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, cp->name(), activity_stack );
	    }
	}
	break;

    case ACT_JOIN_LIST:
    case ACT_OR_JOIN_LIST:
	break;

    default:
	abort();
    }
    if ( join_list->u.join.next ) {
	sum = fork_find_children( join_list->u.join.next, activity_stack, fork_stack, ep );
    }
    return sum;
}


/*
 * Add anActivity to the activity list provided it isn't there already
 * and the slot that it is to go in isn't already occupied.
 */

static bool
add_to_join_list( ActivityList * join_list, unsigned i, Activity * an_activity )
{
    unsigned j;

    if ( join_list->u.join.source[i] == 0 ) { 
	join_list->u.join.source[i] = an_activity;
    } else if ( join_list->u.join.source[i] != an_activity ) {
	return false;
    }

    for ( j = 0; j < join_list->maxa; ++j ) {
	if ( j != i && join_list->u.join.source[j] == an_activity ) return false;
    }
    return true;
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

double
fork_find_children( ActivityList * fork_list, std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0.0;
    double prob = 0.0;
    unsigned i;

    switch ( fork_list->type ) {
    case ACT_AND_FORK_LIST:
	fork_stack.push_back( fork_list );
	for ( i = 0; i < fork_list->na; ++i ) {
	    if ( debug_flag ) {
		Activity * ap = activity_stack.back();
		std::string buf = "AndFork: ";
		for ( size_t k = 0; k < activity_stack.size(); ++k ) buf += ' ';
		buf += ap->name();
		buf += ' ';
		buf += fork_list->list[i]->name();
		fprintf( stddbg, "%s\n", buf.c_str() );
	    }
	    sum += fork_list->list[i]->find_children( activity_stack, fork_stack, ep );
	}
	fork_stack.pop_back();
	break;

    case ACT_FORK_LIST:
	for ( i = 0; i < fork_list->na; ++i ) {
	    sum += fork_list->list[i]->find_children( activity_stack, fork_stack, ep );
	}
	break;

    case ACT_OR_FORK_LIST:
	for ( i = 0; i < fork_list->na; ++i ) {
	    sum += fork_list->list[i]->find_children( activity_stack, fork_stack, ep );
	    prob += fork_list->u.fork.prob[i];
	}
	if ( fabs( 1.0 - prob ) > EPSILON ) {
	    Activity * ap = activity_stack.back();
	    char * aBuf = fork_join_name( fork_list );
	    LQIO::solution_error( LQIO::ERR_MISSING_OR_BRANCH, aBuf, ap->task()->name(), prob );
	    free( aBuf );
	}
	break;

    case ACT_LOOP_LIST:
	if ( fork_list->u.loop.endlist ) {
	    sum += fork_list->u.loop.endlist->find_children( activity_stack, fork_stack, ep );
	}

	for ( i = 0; i < fork_list->na; ++i ) {
	    std::deque<ActivityList *> branch_fork_stack;
	    fork_list->list[i]->find_children( activity_stack, branch_fork_stack, ep );
	}
	break;

    default:
	abort();
    }

    return sum;
}


std::deque<ActivityList *>::iterator 
fork_backtrack( Activity * activity, std::deque<ActivityList *>& fork_stack, Activity * start_activity )
{
    std::deque<ActivityList *>::iterator pos = fork_stack.end();
    ActivityList * fork_list = activity->_input;
    if ( fork_list == NULL ) return pos;
	
    switch ( fork_list->type ) {

    case ACT_AND_FORK_LIST:
	/* Tag branch as coming from a join */
	for ( unsigned i = 0; i < fork_list->na; ++i ) {
	    if ( fork_list->list[i] == activity ) {
		fork_list->u.fork.visit[i] = true;
	    }
	}
	pos = std::find( fork_stack.begin(), fork_stack.end(), fork_list );
	if ( pos == fork_stack.end() ) {
	    return join_backtrack( fork_list->u.fork.prev, fork_stack, start_activity );
	} else {
	    return pos;
	}

    case ACT_OR_FORK_LIST:
    case ACT_FORK_LIST:
	return join_backtrack( fork_list->u.fork.prev, fork_stack, start_activity );

    case ACT_LOOP_LIST:
	return join_backtrack( fork_list->u.loop.prev, fork_stack, start_activity );

    default:
	abort();
    }

    return pos;
}


/*
 * Search backwards up activity list looking for a match on forkStack
 */

std::deque<ActivityList *>::iterator
join_backtrack( ActivityList * join_list, std::deque<ActivityList *>& fork_stack, Activity * start_activity )
{
    std::deque<ActivityList *>::iterator pos = fork_stack.end();
    if ( join_list == NULL ) return pos;
    assert( join_list->is_join_list() );

    size_t depth = 0;
    for ( unsigned int i = 0; i < join_list->na; ++i ) {
	if ( join_list->list[i] == start_activity ) throw start_activity;
	std::deque<ActivityList *>::iterator temp = fork_backtrack( join_list->list[i], fork_stack, start_activity );
	if ( temp != fork_stack.end() && temp - fork_stack.end() > depth ) {
	    pos = temp;
	    depth = temp - fork_stack.end();
	}
    }
    return pos;
}


/*
 * Recursively find all children and grand children from `this'.  We
 * are looking for replies.  And forks are handled a bit strangely so
 * that we count things up correctly.
 */

double
join_collect( ActivityList * join_list, std::deque<Activity *>& activity_stack, ActivityList::Collect& data )
{
    double sum = 0.0;

    switch ( join_list->type ) {
    case ACT_AND_JOIN_LIST:
	/* If it is a sync point... */
	if ( join_list->u.join.join_type == JOIN_SYNCHRONIZATION && join_list->u.join.next ) {
	    sum = fork_collect( join_list->u.join.next, activity_stack, data );
	}
	break;

    case ACT_JOIN_LIST:
    case ACT_OR_JOIN_LIST:
	if ( join_list->u.join.next ) {
	    sum = fork_collect( join_list->u.join.next, activity_stack, data );
	}
	break;

    default:
	abort();
    }
    return sum;
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

double
fork_collect( ActivityList * fork_list, std::deque<Activity *>& activity_stack, ActivityList::Collect& data )
{
    double sum = 0.0;
    unsigned i;
    unsigned int next_phase = data.phase;
    ActivityList * join_list;

    switch ( fork_list->type ) {
    case ACT_AND_FORK_LIST:
	join_list = fork_list->u.fork.join;
	for ( i = 0; i < fork_list->na; ++i ) {
	    ActivityList::Collect branch(data);
	    branch.can_reply =  !join_list || join_list->u.join.quorumCount == 0;
	    sum += fork_list->list[i]->collect( activity_stack, data );
	    next_phase = i_max( next_phase, branch.phase );
	}
	data.phase = next_phase;

	/* Now follow the activities after the join */

	if ( join_list && join_list->u.join.next ) {
	    sum += fork_collect( join_list->u.join.next, activity_stack, data );
	} else {
	    /* Flushing */
	    Task * cp = data._e->task();
	    if ( cp->max_phases() < next_phase ) {
		cp->max_phases( next_phase );
	    }
	}
	break;

    case ACT_FORK_LIST:
	for ( i = 0; i < fork_list->na; ++i ) {
	    ActivityList::Collect branch(data);
	    sum += fork_list->list[i]->collect( activity_stack, branch );
	    next_phase = i_max( next_phase, branch.phase );
	}
	data.phase = next_phase;
	break;

    case ACT_OR_FORK_LIST:
	for ( i = 0; i < fork_list->na; ++i ) {
	    ActivityList::Collect branch(data);
	    branch.rate = data.rate * fork_list->u.fork.prob[i];
	    sum += fork_list->list[i]->collect( activity_stack, branch );
	    next_phase = i_max( next_phase, branch.phase );
	}
	data.phase = next_phase;
	break;

    case ACT_LOOP_LIST:
	/*
	 * For the branches, set rate = 0, because we want to force an error.
	 * Ignore phase information because it isn't valid
	 */

	for ( i = 0; i < fork_list->na; ++i ) {
	    ActivityList::Collect branch(data);
	    branch.can_reply = false;
	    sum += fork_list->list[i]->collect( activity_stack, data );
	}

	if ( fork_list->u.loop.endlist ) {
	    sum += fork_list->u.loop.endlist->collect( activity_stack, data );
	}
	break;

    default:
	abort();
    }

    return sum;
}



/*
 * Check for match
 */

void
join_check( ActivityList * join_list )
{
    unsigned i;

    for ( i = 1; i < join_list->na; ++i ) {
	if ( join_list->u.join.fork[i] != join_list->u.join.fork[0] ) {
	    const char * src = "???";
	    const char * dst = "???";
	    if ( join_list->u.join.fork[i] && join_list->u.join.fork[i]->u.fork.prev->list[0] ) {
		src = join_list->u.join.fork[i]->u.fork.prev->list[0]->name();
	    }
	    if ( join_list->u.join.next->list[0] ) {
		dst = join_list->u.join.next->list[0]->name();
	    }
	    LQIO::solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, "task", dst, src );
	}
    }
}


static bool
set_join_type( ActivityList * join_list, join_type_t a_type )
{
    if ( join_list->u.join.join_type == JOIN_UNDEFINED ) {
	join_list->u.join.join_type = a_type;
	return true;
    } else {
	return join_list->u.join.join_type == a_type;
    }
}

void
print_activity_connectivity( FILE * output, Activity * ap )
{
    unsigned i;
    ActivityList * op;

    if ( ap->_input ) {
	if ( ap->_input->u.fork.prev ) {
	    op = ap->_input->u.fork.prev;
	    fprintf( output, "\toutput from:" );
	    for ( i = 0; i < op->na; ++i ) {
		fprintf( output, " %s", op->list[i]->name() );
	    }
	    fprintf( output, "\n" );
	}
    }

    if ( ap->_output ) {
	op = ap->_output;
	if ( op->u.join.next ) {
	    op = ap->_output->u.join.next;
	    fprintf( output, "\tinput to:   " );
	    for ( i = 0; i < op->na; ++i ) {
		fprintf( output, " %s", op->list[i]->name() );
	    }
	    fprintf( output, "\n" );
	    switch ( op->type ) {
	    case ACT_LOOP_LIST:
		if ( op->u.loop.endlist ) {
		    fprintf( output, "\tcalls      : %s\n",
			     op->u.loop.endlist->name() );
		}
	    }
	}
    }
}



static char *
fork_join_name( const ActivityList * list )
{
    std::ostringstream buf;
    unsigned i;
    for ( i = 0; i < list->na ; ++i ) {
	if ( i > 0 ) {
	    switch ( list->type ) {
	    case ACT_OR_FORK_LIST:
	    case ACT_OR_JOIN_LIST:
		buf << " + ";
		break;
	    case ACT_AND_FORK_LIST:
	    case ACT_AND_JOIN_LIST:
		buf << " & ";
		break;
	    case ACT_LOOP_LIST:
		buf << " * ";
		break;
	    }
	}
	buf << list->list[i]->name();
    }
    return strdup( buf.str().c_str() );
}




static void
activity_cycle_error( int err, const char * task_name, std::deque<Activity *>& activity_stack )
{
    std::string buf;

    for ( std::deque<Activity *>::const_reverse_iterator i = activity_stack.rbegin(); i != activity_stack.rend(); ++i ) {
	if ( i != activity_stack.rbegin() ) {
	    buf += ", ";
	}
	buf += (*i)->name();
    }
    LQIO::solution_error( err, task_name, buf.c_str() );
}


static void
activity_path_error( int err, const ActivityList * list, std::deque<Activity *>& activity_stack )
{
    char * buf2 = fork_join_name( list );
    Activity * ap = activity_stack.back();
    std::string buf;

    for ( std::deque<Activity *>::const_reverse_iterator i = activity_stack.rbegin(); i != activity_stack.rend(); ++i ) {
	if ( i != activity_stack.rbegin() ) {
	    buf += ", ";
	}
	buf += (*i)->name();
    }
    LQIO::solution_error( err, buf2, ap->task()->name(), buf.c_str() );
    free( buf2 );
}

/* -------------------- Functions called by parser. --------------------- */

/*
 * Connect the src and dst lists together.
 */

static void
act_connect ( ActivityList * src, ActivityList * dst )
{
    if ( src ) {
	ActivityList * list = src;
	assert( list->is_join_list() );
	list->u.join.next = dst;
    }
    if ( dst ) {
	ActivityList * list = dst;
	if ( list->is_loop_list() ) {
	    list->u.loop.prev = src;
	} else if ( list->is_fork_list() ) {
	    list->u.fork.prev = src;
	} else {
	    abort();
	}
    }
}

void complete_activity_connections ()
{
    /* We stored all the necessary connections and resolved the list identifiers so finalize */
    std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*>::iterator iter;
    for (iter = Activity::actConnections.begin(); iter != Activity::actConnections.end(); ++iter) {
	ActivityList* src = Activity::domToNative[iter->first];
	ActivityList* dst = Activity::domToNative[iter->second];
	assert(src != NULL && dst != NULL);
	act_connect(src, dst);
    }
}

