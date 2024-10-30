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
 * $Id: actlist.cc 17404 2024-10-30 01:38:06Z greg $
 */

#include "lqsim.h"
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <deque>
#include <numeric>
#include <sstream>
#include <lqio/dom_actlist.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include "activity.h"
#include "actlist.h"
#include "errmsg.h"
#include "instance.h"
#include "model.h"
#include "task.h"

static inline int i_max( int a, int b ) { return a > b ? a : b; }
static void activity_cycle_error( std::deque<Activity *>& activity_stack );

const std::string
ActivityList::get_name() const
{
    std::string sep;

    switch ( get_type() ) {
    case Type::OR_FORK_LIST:
    case Type::OR_JOIN_LIST:
	sep = "+";
	break;
    case Type::AND_FORK_LIST:
    case Type::AND_JOIN_LIST:
	sep = "&";
	break;
    case Type::LOOP_LIST:
	sep = "*";
	break;
    }
    
    return std::accumulate( std::next( _list.begin() ), _list.end(), std::string(_list.front()->name()), fold( sep ) );
}


std::string
ActivityList::fold::operator()( const std::string& s1, const Activity * a2 ) const
{
    return s1 + " " + _op + " " + a2->name();
}

AndJoinActivityList::AndJoinActivityList( ActivityList::Type type, LQIO::DOM::ActivityList * dom )
    : OutputActivityList(type,dom),
      _fork(),
      _source(),
      _join_type(AndJoinActivityList::Join::UNDEFINED),
      _quorum_count(0),
      r_join("Join delay",dom),
      r_join_sqr("Join delay squared",dom),
      _hist_data(nullptr)
{
    if ( getDOM()->hasHistogram() ) {
	// _hist_data = new Histogram();
    }
}


AndJoinActivityList::~AndJoinActivityList()
{
    if ( _hist_data != nullptr ) {
	delete _hist_data;
    }
}

/* ------------------------------------------------------------------------ */

/*
 * Allocate space for an activity.
 */

AndJoinActivityList&
AndJoinActivityList::push_back( Activity * activity )
{
    ActivityList::push_back( activity );
    _source.push_back( nullptr );
    return *this;
}

OrForkActivityList&
OrForkActivityList::push_back( Activity * activity )
{
    ActivityList::push_back( activity );
    _prob.push_back( 0.0 );
    return *this;
}

AndForkActivityList&
AndForkActivityList::push_back( Activity * activity )
{
    ActivityList::push_back( activity );
    return *this;
}

LoopActivityList&
LoopActivityList::push_back( Activity * activity )
{
    ActivityList::push_back( activity );
    _count.push_back( 0.0 );
    return *this;
}

/* ------------------------------------------------------------------------ */

AndForkActivityList&
AndForkActivityList::initialize()
{
    _visits = 0;
    for ( unsigned j = 0; j < size(); ++j ) {
	if ( at(j) ) {
	    _visits += 1;
	}
    }
    return *this;
}


AndJoinActivityList&
AndJoinActivityList::initialize()
{
    r_join.init();
    r_join_sqr.init();
    return *this;
}

/* ------------------------------------------------------------------------ */


/*
 * Set parameters from the dom.  Called when instances are being created.
 */

AndJoinActivityList&
AndJoinActivityList::configure()
{
    _quorum_count = dynamic_cast<const LQIO::DOM::AndJoinActivityList*>(getDOM())->getQuorumCountValue();
    if ( !_hist_data && getDOM()->hasHistogram() ) {
	_hist_data = new Histogram( getDOM()->getHistogram() );
    }
    return *this;
}

AndForkActivityList&
AndForkActivityList::configure()
{
    return *this;
}

OrForkActivityList&
OrForkActivityList::configure()
{
    for ( std::vector<Activity *>::const_iterator i = _list.begin(); i != _list.end(); ++i ) {
	_prob.at(i - _list.begin()) = getDOM()->getParameterValue(dynamic_cast<LQIO::DOM::Activity *>((*i)->getDOM()));
    }
    return *this;
}

LoopActivityList&
LoopActivityList::configure()
{
    _total = 0.0;
    for ( std::vector<Activity *>::const_iterator i = _list.begin(); i != _list.end(); ++i ) {
	const double value = getDOM()->getParameterValue(dynamic_cast<LQIO::DOM::Activity *>((*i)->getDOM()) );
	_count.at( i - _list.begin() ) = value;
	_total += value;
    }
    return *this;
}

/* ------------------------------------------------------------------------ */

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

double
OutputActivityList::find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep )
{
    if ( _next != nullptr ) {
	return _next->find_children( activity_stack, fork_stack, ep );
    } else {
	return 0.0;
    }
}


double
AndJoinActivityList::find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep )
{
    Activity * my_activity = activity_stack.back();

    if ( _fork == nullptr ) {
	/* Look for the fork on the fork stack */
	std::set<AndForkActivityList *> result_set(fork_stack.begin(),fork_stack.end());
	
	try {
	    for ( std::vector<Activity *>::iterator i = _list.begin(); i != _list.end(); ++i ) {
		if ( (*i) == my_activity ) continue;		/* This is result set */
		if ( (*i)->_input == nullptr ) {
		    result_set.clear();				/* Intersection with null */
		    break;
		}

		std::set<AndForkActivityList *> branch_set;
		std::deque<AndJoinActivityList *> join_stack;
		join_stack.push_back( this );
		(*i)->_input->fork_backtrack( fork_stack, join_stack, branch_set );

		/* Find intersection of branches */
	
		std::set<AndForkActivityList *> intersection;
		std::set_intersection( branch_set.begin(), branch_set.end(),
				       result_set.begin(), result_set.end(),
				       std::inserter( intersection, intersection.end() ) );
		result_set = intersection;
	    }
	    
	    /* Result should be all forks that match on all branches.  Take the one closest to top-of-stack */

	    if ( !result_set.empty() ) {

		for ( std::deque<AndForkActivityList *>::const_reverse_iterator fork_list = fork_stack.rbegin(); fork_list != fork_stack.rend() && _fork == nullptr; ++fork_list ) {
		    if ( result_set.find( *fork_list ) == result_set.end() ) continue;

		    /* Set type for join */
	    
		    if ( !set_join_type( Join::INTERNAL_FORK_JOIN ) ) {
			getDOM()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, activity_stack.back()->name().c_str() );
		    }

		    if ( debug_flag ) {
#if 0
			Activity * ap = activity_stack.back();
			std::string buf = "And Join: ";
			for ( size_t k = 0; k < activity_stack.size(); ++k ) buf += ' ';
			buf += ap->name();
			buf += ' ';
			buf += (*i)->name();
			for ( std::deque<AndForkActivityList *>::const_reverse_iterator lp = fork_stack.rbegin(); lp != fork_stack.rend(); ++lp ) {
			    buf += '\n';
			    buf += (*lp)->get_name();
			}
			fprintf( stddbg, "%s\n", buf.c_str() );
#endif
		    }

		    /* Set the links */

		    _fork = *fork_list;
		    const_cast<AndForkActivityList *>(_fork)->set_join( this );
		}
		
	    } else {
		if ( !set_join_type( Join::SYNCHRONIZATION ) ) {
		    getDOM()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, activity_stack.back()->name().c_str() );
		} else {
		    Server_Task * cp = dynamic_cast<Server_Task *>(ep->task());
		    cp->set_synchronization_server();
		}
	    }
	}
	catch ( const cycle_error& e ) {
//	    Server_Task * cp = dynamic_cast<Server_Task *>(ep->task());
	    activity_cycle_error( activity_stack );
	}
    }
    return OutputActivityList::find_children( activity_stack, fork_stack, ep );
}

/* ------------------------------------------------------------------------ */

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

double
AndForkActivityList::find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0.0;

    fork_stack.push_back( this );
    for ( std::vector<Activity *>::iterator i = _list.begin(); i != _list.end(); ++i ) {
	if ( debug_flag ) {
	    Activity * ap = activity_stack.back();
	    std::string buf = "AndFork: ";
	    for ( size_t k = 0; k < activity_stack.size(); ++k ) buf += ' ';
	    buf += ap->name();
	    buf += ' ';
	    buf += (*i)->name();
	    fprintf( stddbg, "%s\n", buf.c_str() );
	}
	sum += (*i)->find_children( activity_stack, fork_stack, ep );
    }
    fork_stack.pop_back();
    return sum;
}

double
ForkActivityList::find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0.0;

    for ( std::vector<Activity *>::iterator i = _list.begin(); i != _list.end(); ++i ) {
	sum += (*i)->find_children( activity_stack, fork_stack, ep );
    }
    return sum;
}


double
OrForkActivityList::find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0.0;
    double prob = 0.0;

    for ( std::vector<Activity *>::iterator i = _list.begin(); i != _list.end(); ++i ) {
	sum += (*i)->find_children( activity_stack, fork_stack, ep );
	prob += _prob.at(i-_list.begin());
    }
    if ( fabs( 1.0 - prob ) > EPSILON ) {
	getDOM()->runtime_error( LQIO::ERR_OR_BRANCH_PROBABILITIES, prob );
    }
    return sum;
}

double
LoopActivityList::find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep )
{
    double sum = 0.0;;
    if ( _exit != nullptr ) {
	sum += _exit->find_children( activity_stack, fork_stack, ep );
    }

    for ( std::vector<Activity *>::iterator i = _list.begin(); i != _list.end(); ++i ) {
	std::deque<AndForkActivityList *> branch_stack;
	sum += (*i)->find_children( activity_stack, branch_stack, ep );
    }
    return sum;
}


/*
 * Add anActivity to the activity list provided it isn't there already
 * and the slot that it is to go in isn't already occupied.
 */

bool
AndJoinActivityList::add_to_join_list( unsigned i, Activity * activity )
{
    if ( _source[i] == nullptr ) { 
	_source[i] = activity;
    } else if ( _source[i] != activity ) {
	return false;
    }

    for ( std::vector<Activity *>::const_iterator j = _source.begin(); j != _source.end(); ++j ) {
	if ( j - _source.begin() != i && *j == activity ) return false;
    }
    return true;
}

/* ------------------------------------------------------------------------ */

/*
 * Search backwards up activity list looking for a match on forkStack
 */

void
OutputActivityList::join_backtrack( std::deque<AndForkActivityList *>& fork_stack, std::deque<AndJoinActivityList *>& join_stack, std::set<AndForkActivityList *>& result_set ) 
{
    for ( std::vector<Activity *>::iterator i = _list.begin(); i != _list.end(); ++i ) {
	if ( (*i)->_input == nullptr ) continue;
	(*i)->_input->fork_backtrack( fork_stack, join_stack, result_set );
    }
}

void
InputActivityList::fork_backtrack( std::deque<AndForkActivityList *>& fork_stack, std::deque<AndJoinActivityList *>& join_stack, std::set<AndForkActivityList *>& result_set )
{
    if ( _prev ) {
	return _prev->join_backtrack( fork_stack, join_stack, result_set );
    }
}

void
AndForkActivityList::fork_backtrack( std::deque<AndForkActivityList *>& fork_stack, std::deque<AndJoinActivityList *>& join_stack, std::set<AndForkActivityList *>& result_set )
{
    result_set.insert( this );
    InputActivityList::fork_backtrack( fork_stack, join_stack, result_set );
}


void
AndJoinActivityList::join_backtrack( std::deque<AndForkActivityList *>& fork_stack, std::deque<AndJoinActivityList *>& join_stack, std::set<AndForkActivityList *>& result_set ) 
{
    if ( std::find( join_stack.begin(), join_stack.end(), this ) != join_stack.end() ) {
	throw AndJoinActivityList::cycle_error( *this );
    }
    join_stack.push_back( dynamic_cast<AndJoinActivityList *>(this) );
    OutputActivityList::join_backtrack( fork_stack, join_stack, result_set );
}


AndJoinActivityList::cycle_error::cycle_error( AndJoinActivityList& list )
    : std::runtime_error( std::accumulate( std::next( list.begin() ), list.end(), std::string(list.front()->name()), fold ) )
{
}


std::string
AndJoinActivityList::cycle_error::fold( const std::string& s1, const Activity * a2 )
{
    return s1 + "&" + a2->name();
}

/* ------------------------------------------------------------------------ */

/*
 * Recursively find all children and grand children from `this'.  We
 * are looking for replies.  And forks are handled a bit strangely so
 * that we count things up correctly.  If our path's cross, we have a
 * loop and abort.
 */

double
OutputActivityList::collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const
{
    if ( _next != nullptr ) {
	return _next->collect( activity_stack, data );
    } else {
	return 0.0;
    }
}

double
AndJoinActivityList::collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const
{
    /* If it is a sync point... */
    if ( _join_type == Join::SYNCHRONIZATION ) {
	return OutputActivityList::collect( activity_stack, data );
    } else {
	return 0.0;
    }
}

double
ForkActivityList::collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const
{
    double sum = 0.0;
    unsigned int next_phase = data.phase;
    
    for ( std::vector<Activity *>::const_iterator i = _list.begin(); i != _list.end(); ++i ) {
	ActivityList::Collect branch(data);
	sum += (*i)->collect( activity_stack, branch );
	next_phase = i_max( next_phase, branch.phase );
    }

    data.phase = next_phase;
    return sum;
}

double
AndForkActivityList::collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const
{
    double sum = 0.0;
    unsigned next_phase = data.phase;

    for ( std::vector<Activity *>::const_iterator i = _list.begin(); i != _list.end(); ++i ) {
	ActivityList::Collect branch(data);
	branch.can_reply =  !dynamic_cast<AndJoinActivityList*>(get_join()) || dynamic_cast<AndJoinActivityList*>(get_join())->get_quorum_count() == 0;
	sum += (*i)->collect( activity_stack, branch );
	next_phase = i_max( next_phase, branch.phase );
    }
    data.phase = next_phase;

    /* Now follow the activities after the join */

    if ( get_join() && get_join()->get_next() ) {
	sum += get_join()->get_next()->collect( activity_stack, data );
    } else {
	/* Flushing */
	Task * cp = data._e->task();
	cp->max_phases( next_phase );
    }
    return sum;
}

double
OrForkActivityList::collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const
{
    double sum = 0.0;
    unsigned int next_phase = data.phase;
    for ( std::vector<Activity *>::const_iterator i = _list.begin(); i != _list.end(); ++i ) {
	ActivityList::Collect branch(data);
	branch.rate = data.rate * _prob.at(i-_list.begin());
	sum += (*i)->collect( activity_stack, branch );
	next_phase = i_max( next_phase, branch.phase );
    }
    data.phase = next_phase;
    return sum;
}

double
LoopActivityList::collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const
{
    double sum = 0.0;

    /*
     * For the branches, set rate = 0, because we want to force an error.
     * Ignore phase information because it isn't valid
     */

    for ( std::vector<Activity *>::const_iterator i = _list.begin(); i != _list.end(); ++i ) {
	ActivityList::Collect branch(data);
	branch.can_reply = false;
	sum += (*i)->collect( activity_stack, branch );
    }

    if ( get_exit() ) {
	sum += get_exit()->collect( activity_stack, data );
    }

    return sum;
}

/* ------------------------------------------------------------------------ */

/*
 * Randomly shuffle items.
 */

void
ActivityList::shuffle()
{
    const size_t n = _list.size();
    for ( size_t i = n; i >= 1; --i ) {
	size_t k = static_cast<size_t>(drand48() * i);
	if ( i-1 != k ) {
	    std::swap( _list[k], _list[i-1] );
	}
    }
}

/*
 * Check for match
 */

bool
AndJoinActivityList::set_join_type( Join type )
{
    if ( _join_type == AndJoinActivityList::Join::UNDEFINED ) {
	_join_type = type;
	return true;
    } else {
	return _join_type == type;
    }
}


AndJoinActivityList&
AndJoinActivityList::reset_stats()
{
    r_join.reset();
    r_join_sqr.reset();
    if ( _hist_data ) {
	_hist_data->reset();
    }
    return *this;
}


AndJoinActivityList&
AndJoinActivityList::accumulate_data()
{
    r_join_sqr.accumulate_variance( r_join.accumulate() );
    if ( _hist_data ) {
	_hist_data->accumulate_data();
    }
    return *this;
}



std::ostream&
AndJoinActivityList::print( std::ostream& output ) const
{
    output << r_join
	   << r_join_sqr;
    return output;
}

AndJoinActivityList&
AndJoinActivityList::insertDOMResults()
{
    LQIO::DOM::AndJoinActivityList * dom = dynamic_cast<LQIO::DOM::AndJoinActivityList *>(getDOM());
    
    dom->setResultJoinDelay(r_join.mean())
	.setResultVarianceJoinDelay(r_join_sqr.mean());
		
    if ( number_blocks > 1 ) {
	dom->setResultJoinDelayVariance( r_join.variance())
	    .setResultVarianceJoinDelayVariance(r_join_sqr.variance());
    }

    /* Do the fork/join results here */
    if ( _hist_data ) {
	_hist_data->insertDOMResults();
    }
    return *this;
}


void
print_activity_connectivity( FILE * output, Activity * ap )
{
#if 0
    unsigned i;
    ActivityList * op;

    if ( ap->_input ) {
	if ( ap->_input->prev ) {
	    op = ap->_input->prev;
	    fprintf( output, "\toutput from:" );
	    for ( i = 0; i < op->na; ++i ) {
		fprintf( output, " %s", op->list[i]->name() );
	    }
	    fprintf( output, "\n" );
	}
    }

    if ( ap->_output ) {
	op = ap->_output;
	if ( op->next ) {
	    op = ap->_output->next;
	    fprintf( output, "\tinput to:   " );
	    for ( i = 0; i < op->na; ++i ) {
		fprintf( output, " %s", op->list[i]->name() );
	    }
	    fprintf( output, "\n" );
	    switch ( op->type ) {
	    case Type::LOOP_LIST:
		if ( op->u.loop.endlist ) {
		    fprintf( output, "\tcalls      : %s\n",
			     op->u.loop.endlist->name() );
		}
	    }
	}
    }
#endif
}



static void
activity_cycle_error( std::deque<Activity *>& activity_stack )
{
    std::string buf;
    Activity * ap = activity_stack.back();

    for ( std::deque<Activity *>::const_reverse_iterator i = activity_stack.rbegin(); i != activity_stack.rend(); ++i ) {
	if ( i != activity_stack.rbegin() ) {
	    buf += ", ";
	}
	buf += (*i)->name();
    }
    ap->task()->getDOM()->runtime_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, buf.c_str() );
}

/* -------------------- Functions called by parser. --------------------- */

/*
 * Connect the src and dst lists together.
 */

static void
act_connect ( ActivityList * src, ActivityList * dst )
{
    if ( dynamic_cast<OutputActivityList *>(src) ) {
	dynamic_cast<OutputActivityList *>(src)->set_next( dynamic_cast<InputActivityList *>(dst) );
    }
    if ( dynamic_cast<InputActivityList *>(dst) ) {
	dynamic_cast<InputActivityList *>(dst)->set_prev( dynamic_cast<OutputActivityList *>(src) );
    }
}

void complete_activity_connections ()
{
    /* We stored all the necessary connections and resolved the list identifiers so finalize */
    std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*>::iterator iter;
    for (iter = Activity::actConnections.begin(); iter != Activity::actConnections.end(); ++iter) {
	ActivityList* src = Activity::domToNative[iter->first];
	ActivityList* dst = Activity::domToNative[iter->second];
	assert(src != nullptr && dst != nullptr);
	act_connect(src, dst);
    }
}

