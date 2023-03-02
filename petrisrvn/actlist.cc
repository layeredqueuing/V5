/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/************************************************************************/

/*
 * $Id: petrisrvn.cc 10943 2012-06-13 20:21:13Z greg $
 *
 * Generate a Petri-net from an SRVN description.
 *
 */

#include <setjmp.h>
#include <cstring>
#include <algorithm>
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_extvar.h>
#include "actlist.h"
#include "entry.h"
#include "errmsg.h"
#include "results.h"
#include "task.h"

static jmp_buf loop_env;


ActivityList::ActivityList( Type type, LQIO::DOM::ActivityList * dom )
    : _type(type), _dom(dom), u(), _n_acts(0)
{
    for ( unsigned int b = 0; b < MAX_BRANCH; ++b ) {
	list[b] = 0;
	for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	    FjT[b][m] = 0;
	}
	entry[b] = 0;
    }
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	FjF[m] = 0;
	FjP[m] = 0;
    }

    switch ( type ) {
    case Type::FORK:
    case Type::OR_FORK:
    case Type::AND_FORK:
	u.fork.prev    = 0;
	u.fork.join    = 0;
	for ( unsigned int i = 0; i < MAX_BRANCH; ++i ) {
	    u.fork.prob[i] = nullptr;
	    u.fork.reachable[i] = false;
	}
	break;

    case Type::LOOP:
	u.loop.prev    = 0;
	for ( unsigned int i = 0; i < MAX_BRANCH; ++i ) {
	    u.loop.count[i] = nullptr;
	}
	u.loop.endlist = 0;
	for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	    u.loop.LoopP[m] = 0;
	    u.loop.LoopT[m] = 0;
	}
	break;

    case Type::AND_JOIN:
    case Type::JOIN:
    case Type::OR_JOIN:
	u.join.next    = 0;
	u.join.type    = JoinType::UNDEFINED;
#if BUG_263
	u.join.quorumCount = 0;
#endif
	for ( unsigned int i = 0; i < MAX_BRANCH; ++i ) {
	    u.join.fork[i] = 0;
	    u.join.source[i] = 0;
	}
	for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	    u.join.tokens[m] = 0.0;
	    u.join.FjM[m] = 0;
	}
	break;

    default: abort();
    }
}

void
ActivityList::remove_netobj()
{
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	switch ( type() ) {
	case Type::OR_JOIN:
	case Type::JOIN:
	case Type::AND_JOIN:
	    u.join.FjM[m] = 0;
	    break;
	default:
	    break;
	}
	for ( unsigned int i = 0; i < MAX_BRANCH; ++i ) {
	    FjT[i][m] = 0;
	}
	FjF[m] = 0;
	FjP[m] = 0;
    }
}

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

bool
ActivityList::join_find_children( std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * e )
{
    const Activity * curr_activity = activity_stack.back();
    unsigned int j;

    /* Find join. -- we need its tokens too! */

    for ( j = 0; j < n_acts() && list[j] != curr_activity; ++j );
    if ( j == n_acts() ) abort();

    switch ( type() ) {

    case Type::JOIN:
    case Type::AND_JOIN:
    case Type::OR_JOIN:
	find_fork_list( e->task(), activity_stack, activity_stack.size(), fork_stack );
	/* Fall through */
    case Type::LOOP:
	entry[j] = e;
	break;

    default:
	abort();
    }

    if ( u.join.next ) {
	return u.join.next->fork_find_children( activity_stack, fork_stack, e );
    } else {
	return false;
    }
}




char *
ActivityList::fork_join_name() const
{
    char * aBuf = static_cast<char *>(malloc( BUFSIZ ));
    char * p = aBuf;
    for ( unsigned int i = 0; i < n_acts(); ++i ) {
	const char * q;
	if ( (p - aBuf) + strlen( list[i]->name() ) + 3 >= BUFSIZ ) {
	    break;
	} else if ( i > 0 ) {
	    *p++ = ' ';
	    switch ( type() ) {
	    case Type::OR_FORK:
	    case Type::OR_JOIN:
		*p++ = '+';
		break;
	    case Type::AND_FORK:
	    case Type::AND_JOIN:
		*p++ = '&';
		break;
	    case Type::LOOP:
		*p++ = '*';
		break;
	    case Type::FORK:
	    case Type::JOIN:
		abort();
	    }
	    *p++ = ' ';
	}
	q = list[i]->name();
	while ( *p = *q++ ) p++;
    }
    return aBuf;
}


/*
 * Return number of external sources for this join.
 */

bool ActivityList::check_external_joins() const
{
    unsigned e1;
    unsigned e2;

    if ( type() != Type::AND_JOIN || u.join.type != JoinType::SYNCHRONIZATION ) return false;

    for ( e1 = 0; e1 + 1 < n_acts(); ++e1 ) {
	for ( e2 = e1 + 1; e2 < n_acts(); ++e2 ) {
	    if ( entry[e1] == entry[e2] ) {
		LQIO::runtime_error( ERR_COMMON_ENTRY_EXTERNAL_SYNC, entry[e1]->task()->name(), entry[e1]->name() );
		return false;
	    }
	}
    }
    return true;
}

bool ActivityList::check_fork_no_join() const
{
    if ( type() == Type::AND_FORK ) {
	for ( unsigned int i = 0; i < n_acts(); ++i ) {
	    if ( !u.fork.reachable[i] ) return true;
	}
    }

    return false;
}



bool ActivityList::check_quorum_join() const
{
#if BUG_263
    return type() == Type::AND_JOIN && u.join.quorumCount > 0;
#else
    return false;
#endif
}

/*
 * Check for forks without joins
 */

bool ActivityList::check_fork_has_join() const
{
    if ( type() == Type::AND_FORK ) {
	for ( unsigned i = 0; i < n_acts(); ++i ) {
	    if ( u.fork.reachable[i] ) return true;
	}
    }
    return false;
}


void ActivityList::find_fork_list( const Task * curr_task, std::deque<Activity *>& activity_stack, int depth, std::deque<ActivityList *>& fork_stack )
{
    Activity * curr_activity = activity_stack.back();

    for ( unsigned int i = 0; i < n_acts(); ++i ) {
	if ( list[i] != curr_activity ) {
	    if ( setjmp( loop_env ) != 0 ) {
		curr_activity->activity_cycle_error( activity_stack );
	    } else {
		int j = backtrack( list[i], fork_stack, list[i] );
		if ( j >= 0 ) {
		    if ( !set_join_type( JoinType::INTERNAL_FORK_JOIN ) ) {
			get_dom()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, list[i]->name() );
		    } else if ( !u.join.fork[i] || std::find( fork_stack.begin(), fork_stack.end(), u.join.fork[i] ) != fork_stack.end() ) {
			ActivityList * fork_list = fork_stack[j];
			if ( (   fork_list->type() == Type::AND_FORK && type() == Type::AND_JOIN)
			     || (fork_list->type() == Type::OR_FORK && type() == Type::OR_JOIN) ) {
			    /* Fork and join match. */
			    u.join.fork[i] = fork_list;
			    fork_list->u.fork.join = this;
			} else if ( fork_list->type() == Type::OR_FORK && type() == Type::AND_JOIN ) {
			    /* Or fork connected to AND join? */
			    const LQIO::DOM::ActivityList * dom = fork_list->get_dom();
			    get_dom()->runtime_error( LQIO::ERR_FORK_JOIN_MISMATCH, dom->getListTypeName().c_str(), dom->getListName().c_str(), dom->getLineNumber() );
			} else {
			    /* This one is o.k., but is causing grief.. */
			    get_dom()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, list[i]->name() );
			}
		    }

		    if ( debug_flag ) {
			int k;
			fprintf( stddbg, "%sJoin: %*s %s: %s ",
				 type() == Type::AND_JOIN ? "AND" : "OR",
				 depth, " ",
				 curr_activity->name(), list[i]->name() );
			for ( k = fork_stack.size()-1; k >= 0; --k ) {
			    ActivityList * lp = fork_stack[k];
			    if ( j == k ) {
				fprintf( stddbg, "[%s] ", lp->u.fork.prev->list[0]->name() );
			    } else {
				fprintf( stddbg, "%s ", lp->u.fork.prev->list[0]->name() );
			    }
			}
			fprintf( stddbg, "\n" );
		    }
		} else {
		    if ( !set_join_type( JoinType::SYNCHRONIZATION ) ) {
			get_dom()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, curr_activity->name() );
		    } else if ( !add_to_join_list( j, activity_stack[0] ) ) {
			get_dom()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, curr_activity->name() );
		    }
		}
	    }
	}
    }
}


/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

bool
ActivityList::fork_find_children( std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * e )
{
    bool has_service_time = false;
    unsigned k;
    switch ( type() ) {

    case Type::FORK:
	for ( k = 0; k < n_acts(); ++k ) {
	    if ( list[k]->find_children( activity_stack, fork_stack, e ) ) {
		has_service_time = true;
	    }
	}
	break;

    case Type::OR_FORK:
    case Type::AND_FORK:
	fork_stack.push_back( this );
	for ( k = 0; k < n_acts(); ++k ) {
	    if ( debug_flag ) {
		Activity * ap = activity_stack.back();
		fprintf( stddbg, "%sFork: %*s %s -> %s\n",
			 type() == Type::AND_FORK ? "AND" : "OR",
			 static_cast<int>(activity_stack.size()), " ",
			 ap->name(), list[k]->name() );
	    }
	    if ( list[k]->find_children( activity_stack, fork_stack, e ) ) {
		has_service_time = true;
	    }
	}
	fork_stack.pop_back();
	break;


    case Type::LOOP:
	if ( u.loop.endlist ) {
	    if ( u.loop.endlist->find_children( activity_stack, fork_stack, e ) ) {
		has_service_time = true;
	    }
	}
	for ( k = 0; k < n_acts(); ++k ) {
	    std::deque<ActivityList *> branch_fork_stack;
	    list[k]->find_children( activity_stack, branch_fork_stack, e );
	}
	break;

    default:
	abort();
    }
    return has_service_time;
}



int
ActivityList::backtrack( Activity * activity, std::deque<ActivityList *>& fork_stack, Activity * start_activity )
{
    int depth = -1;
    std::deque<ActivityList *>::const_iterator j;
    ActivityList * fork_list = activity->input();
    ActivityList * join_list = 0;

    if ( !fork_list ) {
	return -1;

    } else switch ( fork_list->type() ) {

    case Type::OR_FORK:
    case Type::AND_FORK:
	for ( unsigned int i = 0; i < fork_list->n_acts(); ++i ) {
	    if ( fork_list->list[i] == activity ) {
		fork_list->u.fork.reachable[i] = true;
	    }
	}
	j = std::find( fork_stack.begin(), fork_stack.end(), fork_list );
	if ( j != fork_stack.end() ) {
	    return j - fork_stack.begin();
	}
	/* Fall through */
    case Type::FORK:
	join_list = fork_list->u.fork.prev;
	break;

    case Type::LOOP:
	join_list = fork_list->u.loop.prev;
	break;

    default:
	abort();
    }

    if ( !join_list ) {
	return -1;

    } else switch ( join_list->type() ) {

    case Type::JOIN:
    case Type::OR_JOIN:
    case Type::AND_JOIN:
	for ( unsigned int i = 0; i < join_list->n_acts(); ++i ) {
	    if ( join_list->list[i] == start_activity ) {
		longjmp( loop_env, 1 );
	    } else {
		int j = backtrack( join_list->list[i], fork_stack, start_activity );
		if ( j > depth ) {
		    depth = j;
		}
	    }
	}
	break;

    default:
	abort();
    }
    return depth;
}


/*
 * Add anActivity to the activity list provided it isn't there already
 * and the slot that it is to go in isn't already occupied.
 */

bool
ActivityList::add_to_join_list( unsigned int i, Activity * an_activity )
{
    for ( unsigned int j = 0; j < MAX_BRANCH; ++j ) {
	if ( j != i && u.join.source[j] == an_activity ) {
	    return false;
	} else if ( j == i ) {
	    if ( u.join.source[j] != 0 && u.join.source[j] != an_activity ) {
		return false;
	    } else {
		u.join.source[i] = an_activity;
	    }
	}
    }
    return true;
}



/*
 * Recursively find all children and grand children from `this'.  We
 * are looking for replies.  And forks are handled a bit strangely so
 * that we count things up correctly.
 */

double
ActivityList::join_count_replies( std::deque<Activity *>& activity_stack, const Entry * e,
				  const double rate, const unsigned curr_phase, unsigned& next_phase )
{
#if BUG_423
    std::cerr << "  join_count_replies( " << activity_stack.size() << ", " << e->name() << ", " << rate << ", " << curr_phase << ", " << next_phase << ")" << std::endl;
#endif
    double sum = 0.0;

    switch ( type() ) {

    case Type::AND_JOIN:
	/* If it is a sync point... */
	if ( u.join.type == JoinType::SYNCHRONIZATION && u.join.next ) {
	    sum = u.join.next->fork_count_replies( activity_stack, e, rate, curr_phase, next_phase );
	} else if ( curr_phase > next_phase ) {		/* BUG 151 */
	    next_phase = curr_phase;
#if BUG_263
	} else if ( u.join.quorumCount > 0 && e->n_phases() < 2 && e->task()->is_server() ) {
	    const_cast<Entry *>(e)->set_n_phases(2);			/* Force two phases for this entry. */
#endif
	}
	break;

    case Type::JOIN:
    case Type::OR_JOIN:
	if ( u.join.next ) {
	    sum = u.join.next->fork_count_replies( activity_stack, e, rate, curr_phase, next_phase );
	}
	break;

    default:
	abort();
    }

    return sum;
}


/*
 * Recursively find all children and grand children from `this'.  We
 * are looking for replies.  And forks are handled a bit strangely so
 * that we count things up correctly.
 */

double
ActivityList::fork_count_replies( std::deque<Activity *>& activity_stack,  const Entry * e,
				  const double rate, unsigned curr_phase, unsigned& next_phase  )
{
#if BUG_423
    std::cerr << "  fork_count_replies( " << activity_stack.size() << ", " << e->name() << ", " << rate << ", " << curr_phase << ", " << next_phase << ")" << std::endl;
#endif
    double sum = 0.0;
    unsigned i;
    ActivityList * join_list;

    next_phase = curr_phase;

    switch ( type() ) {

    case Type::AND_FORK:
	join_list = u.fork.join;
	for ( i = 0; i < n_acts(); ++i ) {
	    unsigned branch_phase = curr_phase;
#if BUG_263
	    if ( join_list && join_list->u.join.quorumCount != 0 ) {
		/* Don't allow replies on quorum branch */
		sum += list[i]->count_replies( activity_stack, e, 0, curr_phase, branch_phase );
	    } else {
#endif
		sum += list[i]->count_replies( activity_stack, e, rate, curr_phase, branch_phase );
#if BUG_263
	    }
#endif
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}

	/* Now follow the activities after the join */
#if BUG_423
    std::cerr << "**fork_count_replies( " << activity_stack.size() << ", " << e->name() << ", " << rate << ", " << curr_phase << ", " << next_phase << ") post join." << std::endl;
#endif

	if ( join_list && join_list->u.join.next ) {
	    curr_phase = next_phase;
	    ActivityList * fork_list = join_list->u.join.next;
	    for ( i = 0; i < fork_list->n_acts(); ++i ) {
		unsigned branch_phase = curr_phase;
		sum += fork_list->list[i]->count_replies( activity_stack, e, rate, curr_phase, branch_phase );
		if ( branch_phase > next_phase  ) {
		    next_phase = branch_phase;
		}
	    }
#if BUG_263
	    /* If we are a quorum join, always force a second phase */
	    if ( join_list->u.join.quorumCount > 0 && next_phase == 1 ) {
		next_phase = 2;
	    }
#endif		
	} else {
	    /* Flushing -- add a phase */
	    const_cast<Entry *>(e)->set_n_phases( next_phase );
	} 
	break;

    case Type::FORK:
	for ( i = 0; i < n_acts(); ++i ) {
	    unsigned branch_phase = curr_phase;
	    sum += list[i]->count_replies( activity_stack, e, rate, curr_phase, branch_phase ); 
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}
	break;

    case Type::OR_FORK:
	for ( i = 0; i < n_acts(); ++i ) {
	    unsigned branch_phase = curr_phase;
	    const double prob = LQIO::DOM::to_double(*u.fork.prob[i]);
	    if ( prob < 0. || 1.0 < prob ) {
		get_dom()->runtime_error( LQIO::ERR_INVALID_OR_BRANCH_PROBABILITY, list[i]->get_dom()->getName().c_str(), prob );
		break;
	    } 
	    sum += list[i]->count_replies( activity_stack, e, rate * prob, curr_phase, branch_phase );
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}
	if ( sum < 1.0 - EPSILON || 1.0 + EPSILON < sum ) {
	    get_dom()->runtime_error( LQIO::ERR_OR_BRANCH_PROBABILITIES, sum );
	}
	break;

			
    case Type::LOOP:
	if ( u.loop.endlist ) {
	    unsigned branch_phase = curr_phase;
	    sum += u.loop.endlist->count_replies( activity_stack, e, rate, curr_phase, branch_phase );
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}
	for ( i = 0; i < n_acts(); ++i ) {
	    /* ignore phase change */
	    unsigned branch_phase = curr_phase;
	    list[i]->count_replies( activity_stack, e, 0.0, curr_phase, branch_phase );
	}
	break;

    default:
	abort();
    }

    return sum;
}




bool
ActivityList::set_join_type( JoinType a_type )
{
    if ( u.join.type == JoinType::UNDEFINED ) {
	u.join.type = a_type;
	return true;
    } else {
	return u.join.type == a_type;
    }
}




bool ActivityList::is_join_list() const
{
    return type() == Type::JOIN || type() == Type::AND_JOIN || type() == Type::OR_JOIN;
}


bool ActivityList::is_fork_list() const
{
    return type() == Type::FORK || type() == Type::AND_FORK || type() == Type::OR_FORK;
}

bool ActivityList::is_loop_list() const
{
    return type() == Type::LOOP;
}

void ActivityList::insert_DOM_results() const
{
    if ( type() == Type::AND_JOIN && list[0]->_throughput[0] ) {
	_dom->setResultJoinDelay( u.join.tokens[0] / list[0]->_throughput[0] );
    }
}


void
ActivityList::path_error( int err, const char * task_name, std::deque<Activity *>& activity_stack )
{
    static char buf[BUFSIZ];
    char * buf2 = fork_join_name();
    Activity * ap = activity_stack.back();
    size_t l = snprintf( buf, BUFSIZ, "%s", ap->name() );

    int i;
    for  ( i = activity_stack.size()-1; i > 0; --i ) {
	ap = activity_stack[i-1];
	l += snprintf( &buf[l], BUFSIZ-l, ", %s", ap->name() );
    }
    LQIO::runtime_error( err, buf2, task_name, buf );
    free( buf2 );
}

void
ActivityList::follow_join_for_tokens( const Entry * e, unsigned p, const unsigned m,
				      Activity * curr_activity, const bool sum_forks, double scale,
				      Phase::util_fnptr util_func, double mean_tokens[] )
{
    switch ( type() ) {
    case Type::AND_JOIN:

	if ( util_func == &Phase::get_utilization ) {
	    if ( list[0] == curr_activity ) {
		u.join.tokens[m] += scale * get_pmmean( "AJ%s%d", list[0]->name(), m );
	    }
	}

	if ( u.join.type != JoinType::SYNCHRONIZATION ) {
	    return;	/* Handled on fork side. */
	}
	break;

    case Type::OR_JOIN:
	/* This handles both internal and external or-joins */
	/* I should probably scale by the ratio of throughtputs to the entry if its external */
	scale /= static_cast<double>(n_acts());
	break;
	
    case Type::JOIN:
	break;
		
    default:
	abort();
    }
		
    ActivityList * fork_list = u.join.next;
    if ( fork_list ) {
	fork_list->follow_fork_for_tokens( e, p, m, sum_forks, scale, util_func, mean_tokens );
    }
}

void
ActivityList::follow_fork_for_tokens( const Entry * e, unsigned p, const unsigned m,
				      const bool sum_forks, const double scale,
				      Phase::util_fnptr util_func, double mean_tokens[] )
{
    ActivityList * join_list;
    unsigned j;

    switch ( type() ) {

    case Type::AND_FORK:
	if ( sum_forks ) {
	    for ( j = 0; j < n_acts(); ++j ) {
		list[j]->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, mean_tokens );
	    }
	} else {
	    unsigned q;
	    double tokens[DIMPH+1];
	    double max_tokens[DIMPH+1];
	    for ( q = 0; q <= DIMPH; ++q ) {
		tokens[q] = 0;
		max_tokens[q] = 0;
	    }
	    for ( j = 0; j < n_acts(); ++j ) {
		list[j]->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, tokens );
		for ( q = 1; q <= DIMPH; ++q ) {
		    if ( tokens[q] > max_tokens[q] ) {
			max_tokens[q] = tokens[q];
		    }
		    tokens[q] = 0;
		}
	    }
	    for ( q = 1; q <= DIMPH; ++q ) {
		mean_tokens[q] += max_tokens[q];
	    }
	}
	join_list = u.fork.join;
	if ( join_list ) {
	    if ( distinguish_join_customers || m == 0 ) {
		join_list->u.join.tokens[m] = get_pmmean( "JJ%s%d", u.fork.prev->list[0]->name(), m );
	    }
	    if ( join_list->u.join.next ) {
		join_list->u.join.next->follow_fork_for_tokens(e, p, m, sum_forks, scale, util_func, mean_tokens );
	    }
	}
	break;
			
    case Type::OR_FORK:
    case Type::FORK:
	for ( j = 0; j < n_acts(); ++j ) {
	    list[j]->follow_activity_for_tokens(e, p, m, sum_forks, scale, util_func, mean_tokens );
	}
	break;

    case Type::LOOP:
	if ( u.loop.endlist ) {
	    u.loop.endlist->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, mean_tokens );
	}
	for ( j = 0; j < n_acts(); ++j ) {
	    list[j]->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, mean_tokens );
	}
	break;

    default:
	abort();
    }
}

