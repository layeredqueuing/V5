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
 * $Id$
 */

#include <sstream>
#include <parasol.h>
#include "lqsim.h"
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include "model.h"
#include "activity.h"
#include "task.h"
#include "instance.h"
#include "errmsg.h"
#include "stack.h"
#include "processor.h"

using namespace std;
  
static bool set_join_type( ActivityList * join_list, join_type_t a_type );
static bool add_to_join_list( ActivityList * join_list, unsigned i, Activity * an_activity );
static inline int i_max( int a, int b ) { return a > b ? a : b; }
static void activity_path_error( int, const ActivityList *, para_stack_t * activity_stack );
static void activity_cycle_error( int err, const char * task_name, para_stack_t * activity_stack );
static char * fork_join_name( const ActivityList * );
static int fork_backtrack( Activity *, para_stack_t *, Activity * );
static int join_backtrack( ActivityList *, para_stack_t *, Activity * );


bool
is_join_list( const ActivityList * list )
{
    return list->type == ACT_JOIN_LIST || list->type == ACT_AND_JOIN_LIST || list->type == ACT_OR_JOIN_LIST;
}


bool
is_fork_list( const ActivityList * list )
{
    return list->type == ACT_FORK_LIST || list->type == ACT_AND_FORK_LIST || list->type == ACT_OR_FORK_LIST;
}

bool
is_loop_list( const ActivityList * list )
{
    return list->type == ACT_LOOP_LIST;
}

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

double
join_find_children( ActivityList * join_list, para_stack_t * activity_stack, para_stack_t * fork_stack, const Entry * ep )
{
    double sum = 0.0;
    Activity * my_activity = static_cast<Activity *>(top( activity_stack ));

    switch ( join_list->type ) {
    case ACT_AND_JOIN_LIST:

	/* Look for the fork on the fork stack */
	for ( unsigned i = 0; i < join_list->na; ++i ) {
	    if ( join_list->list[i] != my_activity ) {
		int j = fork_backtrack( join_list->list[i], fork_stack, join_list->list[i] );
		Server_Task * cp = dynamic_cast<Server_Task *>(ep->task());
		if ( j >= 0 ) {
		    if ( !set_join_type( join_list, JOIN_INTERNAL_FORK_JOIN ) ) {
			activity_path_error( LQIO::ERR_JOIN_BAD_PATH, join_list, activity_stack );
		    } else if ( !join_list->u.join.fork[i] || stack_find( fork_stack, join_list->u.join.fork[i] ) >= 0 ) {
			join_list->u.join.fork[i] = static_cast<ActivityList *>(fork_stack->stack[j]);
			join_list->u.join.fork[i]->u.fork.join = join_list;
		    }
		    if ( debug_flag ) {
			int k;
			Activity * ap = (Activity *)top( activity_stack );
			fprintf( stddbg, "AndJoin: %*s %s: %s ", activity_stack->depth, " ",
				 ap->name(), join_list->list[i]->name() );
			for ( k = fork_stack->depth-1; k >= 0; --k ) {
			    ActivityList * lp = static_cast<ActivityList *>(fork_stack->stack[k]);
			    if ( j == k ) {
				fprintf( stddbg, "[%s] ", lp->u.fork.prev->list[0]->name() );
			    } else {
				fprintf( stddbg, "%s ", lp->u.fork.prev->list[0]->name() );
			    }
			}
			fprintf( stddbg, "\n" );
		    }
		} else if ( j == -1 ) {
		    if ( !set_join_type( join_list, JOIN_SYNCHRONIZATION ) ) {
			activity_path_error( LQIO::ERR_JOIN_BAD_PATH, join_list, activity_stack );
		    } else if ( !add_to_join_list( join_list, i, static_cast<Activity *>(activity_stack->stack[0]) ) ) {
			activity_path_error( LQIO::ERR_JOIN_BAD_PATH, join_list, activity_stack );
		    } else {
			cp->set_synchronization_server();
		    }
		} else {
		    activity_cycle_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, cp->name(), activity_stack );
		}
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
fork_find_children( ActivityList * fork_list, para_stack_t * activity_stack, para_stack_t * fork_stack, const Entry * ep )
{
    double sum = 0.0;
    double prob = 0.0;
    unsigned i;

    switch ( fork_list->type ) {
    case ACT_AND_FORK_LIST:
	push( fork_stack, fork_list );
	for ( i = 0; i < fork_list->na; ++i ) {
	    if ( debug_flag ) {
		Activity * ap = (Activity *)top( activity_stack );
		fprintf( stddbg, "AndFork: %*s %s -> %s\n", activity_stack->depth, " ",
			 ap->name(), fork_list->list[i]->name() );
	    }
	    sum += fork_list->list[i]->find_children( activity_stack, fork_stack, ep );
	}
	pop( fork_stack );
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
	    Activity * ap = (Activity *)top( activity_stack );
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
	    para_stack_t branch_fork_stack;
	    stack_init( &branch_fork_stack );
	    fork_list->list[i]->find_children( activity_stack, &branch_fork_stack, ep );
	    stack_delete( &branch_fork_stack );    
	}
	break;

    default:
	abort();
    }

    return sum;
}


int
fork_backtrack( Activity * activity, para_stack_t * fork_stack, Activity * start_activity )
{
    ActivityList * fork_list = activity->_input;

    if ( fork_list ) {
	int pos = -1;

	switch ( fork_list->type ) {

	case ACT_AND_FORK_LIST:
	    /* Tag branch as coming from a join */
	    for ( unsigned i = 0; i < fork_list->na; ++i ) {
		if ( fork_list->list[i] == activity ) {
		    fork_list->u.fork.visit[i] = true;
		}
	    }
	    pos = stack_find( fork_stack, fork_list );
	    if ( pos < 0 ) {
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
    }

    return -1;
}


/*
 * Search backwards up activity list looking for a match on forkStack
 */

int
join_backtrack( ActivityList * join_list, para_stack_t * fork_stack, Activity * start_activity )
{
    int depth = -1;
    if ( join_list ) {
	unsigned i;
	assert( is_join_list( join_list ) );
	for ( i = 0; i < join_list->na; ++i ) {
	    if ( join_list->list[i] == start_activity ) {
		return -2;
	    } else {
		int j = fork_backtrack( join_list->list[i], fork_stack, start_activity );
		if ( j > depth ) {
		    depth = j;
		}
	    }
	}
    }
    return depth;
}


/*
 * Recursively find all children and grand children from `this'.  We
 * are looking for replies.  And forks are handled a bit strangely so
 * that we count things up correctly.
 */

double
join_count_replies( ActivityList * join_list, para_stack_t * activity_stack, const Entry * ep,
		    const double rate, const unsigned int curr_phase, unsigned int *next_phase  )
{
    double sum = 0.0;

    switch ( join_list->type ) {
    case ACT_AND_JOIN_LIST:
	/* If it is a sync point... */
	if ( join_list->u.join.join_type == JOIN_SYNCHRONIZATION && join_list->u.join.next ) {
	    sum = fork_count_replies( join_list->u.join.next, activity_stack, ep, rate, curr_phase, next_phase );
	}
	break;

    case ACT_JOIN_LIST:
    case ACT_OR_JOIN_LIST:
	if ( join_list->u.join.next ) {
	    sum = fork_count_replies( join_list->u.join.next, activity_stack, ep, rate, curr_phase, next_phase );
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
fork_count_replies( ActivityList * fork_list, para_stack_t * activity_stack, const Entry * ep,
		    const double rate, const unsigned int curr_phase, unsigned int * next_phase  )
{
    double sum = 0.0;
    unsigned i;
    unsigned int branch_phase;
    ActivityList * join_list;
    double branch_rate = rate;

    *next_phase = curr_phase;

    switch ( fork_list->type ) {
    case ACT_AND_FORK_LIST:
	join_list = fork_list->u.fork.join;
	if ( join_list && join_list->u.join.quorumCount != 0 ) {
	    branch_rate = 0.0;	/* Don't allow reply here */
	}
	for ( i = 0; i < fork_list->na; ++i ) {
	    branch_phase = curr_phase;
	    sum += fork_list->list[i]->count_replies( activity_stack, ep, branch_rate, curr_phase, &branch_phase );
	    *next_phase = i_max( *next_phase, branch_phase );
	}

	/* Now follow the activities after the join */

	if ( join_list && join_list->u.join.next ) {
	    sum += fork_count_replies( join_list->u.join.next, activity_stack, ep, rate, *next_phase, next_phase );
	} else {
	    /* Flushing */
	    Task * cp = ep->task();
	    if ( cp->max_phases < *next_phase ) {
		cp->max_phases = *next_phase;
	    }
	}
	break;

    case ACT_FORK_LIST:
	for ( i = 0; i < fork_list->na; ++i ) {
	    branch_phase = curr_phase;
	    sum += fork_list->list[i]->count_replies( activity_stack, ep,
				  rate, curr_phase, &branch_phase );
	    *next_phase = i_max( *next_phase, branch_phase );
	}
	break;

    case ACT_OR_FORK_LIST:
	for ( i = 0; i < fork_list->na; ++i ) {
	    branch_phase = curr_phase;
	    sum += fork_list->list[i]->count_replies( activity_stack, ep,
				  rate * fork_list->u.fork.prob[i], curr_phase, &branch_phase );
	    *next_phase = i_max( *next_phase, branch_phase );
	}
	break;

    case ACT_LOOP_LIST:
	if ( fork_list->u.loop.endlist ) {
	    branch_phase = curr_phase;
	    sum += fork_list->u.loop.endlist->count_replies( activity_stack, ep, rate, curr_phase, &branch_phase );
	    *next_phase = i_max( *next_phase, branch_phase );
	}

	/*
	 * For the branches, set rate = 0, because we want to force an error.
	 * Ignore phase information because it isn't valid
	 */

	for ( i = 0; i < fork_list->na; ++i ) {
	    branch_phase = curr_phase;
	    fork_list->list[i]->count_replies( activity_stack, ep, 0.0, curr_phase, &branch_phase );
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
	    LQIO::solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, src, dst );
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
    ostringstream buf;
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
activity_cycle_error( int err, const char * task_name, para_stack_t * activity_stack )
{
    ostringstream buf;
    Activity * ap = static_cast<Activity *>(top( activity_stack ) );

    buf << ap->name();

    for  ( unsigned i = activity_stack->depth-1; i > 0; --i ) {
	ap = static_cast<Activity *>(activity_stack->stack[i-1]);
	buf << ", " << ap->name();
    }
    LQIO::solution_error( err, task_name, buf.str().c_str() );
}


static void
activity_path_error( int err, const ActivityList * list, para_stack_t * activity_stack )
{
    ostringstream buf;
    char * buf2 = fork_join_name( list );
    Activity * ap = static_cast<Activity *>(top( activity_stack ));

    buf << ap->name();

    for  ( unsigned i = activity_stack->depth-1; i > 0; --i ) {
	ap = static_cast<Activity *>(activity_stack->stack[i-1]);
	buf << ", " <<  ap->name();
    }
    LQIO::solution_error( err, buf2, ap->task()->name(), buf.str().c_str() );
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
	assert( is_join_list( list ) );
	list->u.join.next = dst;
    }
    if ( dst ) {
	ActivityList * list = dst;
	if ( is_loop_list( list ) ) {
	    list->u.loop.prev = src;
	} else if ( is_fork_list( list ) ) {
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

