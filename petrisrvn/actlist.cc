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
#include "task.h"
#include "entry.h"
#include "results.h"

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
#if defined(BUG_263)
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
		
    for ( j = 0; j < this->n_acts() && this->list[j] != curr_activity; ++j );
    if ( j == this->n_acts() ) abort();

    switch ( this->type() ) {

    case Type::JOIN:
    case Type::AND_JOIN:
    case Type::OR_JOIN:
	find_fork_list( e->task(), activity_stack, activity_stack.size(), fork_stack );
	/* Fall through */
    case Type::LOOP:
	this->entry[j] = e;
	break;
		
    default:
	abort();
    }

    if ( this->u.join.next ) {
	return this->u.join.next->fork_find_children( activity_stack, fork_stack, e );
    } else {
	return false;
    }
}




char *
ActivityList::fork_join_name() const
{
    char * aBuf = static_cast<char *>(malloc( BUFSIZ ));
    char * p = aBuf;
    for ( unsigned int i = 0; i < this->n_acts(); ++i ) {
	const char * q;
	if ( (p - aBuf) + strlen( list[i]->name() ) + 3 >= BUFSIZ ) {
	    break;
	} else if ( i > 0 ) {
	    *p++ = ' ';
	    switch ( this->type() ) {
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
	
    if ( this->type() != Type::AND_JOIN || this->u.join.type != JoinType::SYNCHRONIZATION ) return false;
    
    for ( e1 = 0; e1 + 1 < this->n_acts(); ++e1 ) {
	for ( e2 = e1 + 1; e2 < this->n_acts(); ++e2 ) {
	    if ( this->entry[e1] == this->entry[e2] ) {
		solution_error( LQIO::ERR_JOIN_PATH_MISMATCH, this->list[e1]->name(), this->list[e2]->name() );
		return false;
	    }
	}
    }
    return true;
}

bool ActivityList::check_fork_no_join() const
{
    if ( this->type() == Type::AND_FORK ) {
	for ( unsigned int i = 0; i < this->n_acts(); ++i ) {
	    if ( !this->u.fork.reachable[i] ) return true;
	}
    }

    return false;
}



bool ActivityList::check_quorum_join() const 
{
#if defined(BUG_263)
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

    for ( unsigned int i = 0; i < this->n_acts(); ++i ) {
	if ( this->list[i] != curr_activity ) {
	    if ( setjmp( loop_env ) != 0 ) {
		curr_activity->activity_cycle_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, curr_task->name(), activity_stack );
	    } else {
		int j = backtrack( this->list[i], fork_stack, this->list[i] );
		if ( j >= 0 ) {
		    if ( !this->set_join_type( JoinType::INTERNAL_FORK_JOIN ) ) {
			path_error( LQIO::ERR_JOIN_BAD_PATH, curr_task->name(), activity_stack );
		    } else if ( !this->u.join.fork[i] || std::find( fork_stack.begin(), fork_stack.end(), this->u.join.fork[i] ) != fork_stack.end() ) {
			ActivityList * fork_list = fork_stack[j];
			if ( (   fork_list->type() == Type::AND_FORK && this->type() == Type::AND_JOIN)
			     || (fork_list->type() == Type::OR_FORK && this->type() == Type::OR_JOIN) ) {
			    /* Fork and join match. */
			    this->u.join.fork[i] = fork_list;
			    fork_list->u.fork.join = this;
			} else if ( fork_list->type() == Type::OR_FORK && this->type() == Type::AND_JOIN ) {
			    /* Or fork connected to AND join? */
			    path_error( LQIO::ERR_JOIN_BAD_PATH, curr_task->name(), activity_stack );
			} else {
			    /* This one is o.k., but is causing grief.. */
			    path_error( LQIO::ERR_JOIN_BAD_PATH, curr_task->name(), activity_stack );
			}
		    }

		    if ( debug_flag ) {
			int k;
			fprintf( stddbg, "%sJoin: %*s %s: %s ",
				 this->type() == Type::AND_JOIN ? "AND" : "OR",
				 depth, " ",
				 curr_activity->name(), this->list[i]->name() );
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
		    if ( !this->set_join_type( JoinType::SYNCHRONIZATION ) ) {
			path_error( LQIO::ERR_JOIN_BAD_PATH, curr_task->name(), activity_stack );
		    } else if ( !add_to_join_list( j, activity_stack[0] ) ) {
			path_error( LQIO::ERR_JOIN_BAD_PATH, curr_task->name(), activity_stack );
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
    switch ( this->type() ) {

    case Type::FORK:
	for ( k = 0; k < this->n_acts(); ++k ) {
	    if ( this->list[k]->find_children( activity_stack, fork_stack, e ) ) {
		has_service_time = true;
	    }
	}
	break;

    case Type::OR_FORK:
    case Type::AND_FORK:
	fork_stack.push_back( this );
	for ( k = 0; k < this->n_acts(); ++k ) {
	    if ( debug_flag ) {
		Activity * ap = activity_stack.back();
		fprintf( stddbg, "%sFork: %*s %s -> %s\n",
			 this->type() == Type::AND_FORK ? "AND" : "OR",
			 static_cast<int>(activity_stack.size()), " ",
			 ap->name(), this->list[k]->name() );
	    }
	    if ( this->list[k]->find_children( activity_stack, fork_stack, e ) ) {
		has_service_time = true;
	    }
	}
	fork_stack.pop_back();
	break;

			
    case Type::LOOP:
	if ( this->u.loop.endlist ) {
	    if ( this->u.loop.endlist->find_children( activity_stack, fork_stack, e ) ) {
		has_service_time = true;
	    }
	}
	for ( k = 0; k < this->n_acts(); ++k ) {
	    std::deque<ActivityList *> branch_fork_stack;
	    this->list[k]->find_children( activity_stack, branch_fork_stack, e );
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
	if ( j != i && this->u.join.source[j] == an_activity ) {
	    return false;
	} else if ( j == i ) {
	    if ( this->u.join.source[j] != 0 && this->u.join.source[j] != an_activity ) {
		return false;
	    } else {
		this->u.join.source[i] = an_activity;
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
    double sum = 0.0;
    
    switch ( this->type() ) {

    case Type::AND_JOIN:
	/* If it is a sync point... */
	if ( this->u.join.type == JoinType::SYNCHRONIZATION && this->u.join.next ) {
	    sum = this->u.join.next->fork_count_replies( activity_stack, e, rate, curr_phase, next_phase );
	} else if ( curr_phase > next_phase ) {		/* BUG 151 */
	    next_phase = curr_phase;
#if defined(BUG_263)
	} else if ( this->u.join.quorumCount > 0 && e->n_phases() < 2 && e->task()->is_server() ) {
	    const_cast<Entry *>(e)->set_n_phases(2);			/* Force two phases for this entry. */
#endif
	}
	break;

    case Type::JOIN:
    case Type::OR_JOIN:
	if ( this->u.join.next ) {
	    sum = this->u.join.next->fork_count_replies( activity_stack, e, rate, curr_phase, next_phase );
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
    double sum = 0.0;
    unsigned i;
    ActivityList * join_list;
    
    next_phase = curr_phase;

    switch ( this->type() ) {

    case Type::AND_FORK:
	join_list = this->u.fork.join;
	for ( i = 0; i < this->n_acts(); ++i ) {
	    unsigned branch_phase = curr_phase;
#if defined(BUG_263)
	    if ( join_list && join_list->u.join.quorumCount != 0 ) {
		/* Don't allow replies on quorum branch */
		sum += this->list[i]->count_replies( activity_stack, e, 0, curr_phase, branch_phase );
	    } else {
#endif
		sum += this->list[i]->count_replies( activity_stack, e, rate, curr_phase, branch_phase );
#if defined(BUG_263)
	    }
#endif
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}

	/* Now follow the activities after the join */

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
#if defined(BUG_263)
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
	for ( i = 0; i < this->n_acts(); ++i ) {
	    unsigned branch_phase = curr_phase;
	    sum += this->list[i]->count_replies( activity_stack, e, rate, curr_phase, branch_phase ); 
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}
	break;

    case Type::OR_FORK:
	for ( i = 0; i < this->n_acts(); ++i ) {
	    unsigned branch_phase = curr_phase;
	    const double prob = LQIO::DOM::to_double(*this->u.fork.prob[i]);
	    if ( prob < 0. || 1.0 < prob ) {
		LQIO::solution_error( LQIO::ERR_INVALID_PROBABILITY, prob );
		break;
	    } 
	    sum += this->list[i]->count_replies( activity_stack, e, rate * prob, curr_phase, branch_phase );
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}
	if ( sum < 1.0 - EPSILON || 1.0 + EPSILON < sum ) {
	    LQIO::solution_error( LQIO::ERR_OR_BRANCH_PROBABILITIES, fork_join_name(), e->task()->name(), sum );
	}
	break;

			
    case Type::LOOP:
	if ( this->u.loop.endlist ) {
	    unsigned branch_phase = curr_phase;
	    sum += this->u.loop.endlist->count_replies( activity_stack, e, rate, curr_phase, branch_phase );
	    if ( branch_phase > next_phase ) {
		next_phase = branch_phase;
	    }
	}
	for ( i = 0; i < this->n_acts(); ++i ) {
	    /* ignore phase change */
	    unsigned branch_phase = curr_phase;
	    this->list[i]->count_replies( activity_stack, e, 0.0, curr_phase, branch_phase );
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
    if ( this->u.join.type == JoinType::UNDEFINED ) {
	this->u.join.type = a_type;
	return true;
    } else {
	return this->u.join.type == a_type;
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
    LQIO::solution_error( err, buf2, task_name, buf );
    free( buf2 );
}

void
ActivityList::follow_join_for_tokens( const Entry * e, unsigned p, const unsigned m,
				      Activity * curr_activity, const bool sum_forks, double scale,
				      Phase::util_fnptr util_func, double mean_tokens[] )
{
    switch ( this->type() ) {
    case Type::AND_JOIN:

	if ( util_func == &Phase::get_utilization ) {
	    if ( this->list[0] == curr_activity ) {
		this->u.join.tokens[m] += scale * get_pmmean( "AJ%s%d", this->list[0]->name(), m );
	    }
	}

	if ( this->u.join.type != JoinType::SYNCHRONIZATION ) {
	    return;	/* Handled on fork side. */
	}
	break;

    case Type::OR_JOIN:
	/* This handles both internal and external or-joins */
	/* I should probably scale by the ratio of throughtputs to the entry if its external */
	scale /= static_cast<double>(this->n_acts());
	break;
	
    case Type::JOIN:
	break;
		
    default:
	abort();
    }
		
    ActivityList * fork_list = this->u.join.next;
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

    switch ( this->type() ) {

    case Type::AND_FORK:
	if ( sum_forks ) {
	    for ( j = 0; j < this->n_acts(); ++j ) {
		this->list[j]->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, mean_tokens );
	    }
	} else {
	    unsigned q;
	    double tokens[DIMPH+1];
	    double max_tokens[DIMPH+1];
	    for ( q = 0; q <= DIMPH; ++q ) {
		tokens[q] = 0;
		max_tokens[q] = 0;
	    }
	    for ( j = 0; j < this->n_acts(); ++j ) {
		this->list[j]->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, tokens );
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
	join_list = this->u.fork.join;
	if ( join_list ) {
	    if ( distinguish_join_customers || m == 0 ) {
		join_list->u.join.tokens[m] = get_pmmean( "JJ%s%d", this->u.fork.prev->list[0]->name(), m );
	    }
	    if ( join_list->u.join.next ) {
		join_list->u.join.next->follow_fork_for_tokens(e, p, m, sum_forks, scale, util_func, mean_tokens );
	    }
	}
	break;
			
    case Type::OR_FORK:
    case Type::FORK:
	for ( j = 0; j < this->n_acts(); ++j ) {
	    this->list[j]->follow_activity_for_tokens(e, p, m, sum_forks, scale, util_func, mean_tokens );
	}
	break;

    case Type::LOOP:
	if ( this->u.loop.endlist ) {
	    this->u.loop.endlist->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, mean_tokens );
	}
	for ( j = 0; j < this->n_acts(); ++j ) {
	    this->list[j]->follow_activity_for_tokens( e, p, m, sum_forks, scale, util_func, mean_tokens );
	}
	break;

    default:
	abort();
    }
}

