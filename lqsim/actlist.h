/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May  1996.								*/
/* August 2009								*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $Id: actlist.h 13751 2020-08-10 02:27:53Z greg $
 */

#ifndef ACTLIST_H
#define ACTLIST_H

#include "result.h"

class Entry;
class Activity;
class Histogram;
class Task;
class Activity;

typedef enum list_type
{
    ACT_FORK_LIST,
    ACT_OR_FORK_LIST,
    ACT_AND_FORK_LIST,
    ACT_LOOP_LIST,
    ACT_JOIN_LIST,
    ACT_AND_JOIN_LIST,
    ACT_OR_JOIN_LIST
} list_type;

typedef enum join_type
{
    JOIN_UNDEFINED,
    JOIN_INTERNAL_FORK_JOIN,
    JOIN_SYNCHRONIZATION
} join_type_t;

struct ActivityList {
    
    struct Collect
    {
	typedef double (Activity::*fptr)( ActivityList::Collect& ) const;

	Collect( const Entry * e, fptr f ) : _e(e), rate(1.), phase(1), can_reply(true), _f(f) {};
	const Entry * _e;
	double rate;
	unsigned int phase;
	bool can_reply;
	fptr _f;
    };
	
    list_type type;
    union {
	struct {
	    struct ActivityList * prev;		/* Link to join list.		*/
	    struct ActivityList * join;		/* Link to fork from join.	*/
	    bool * visit;			/* true if I visit a join.	*/
	    double * prob;			/* Array of probabilities.	*/
	    unsigned visit_count;		/* */
	} fork;
	struct {
	    struct ActivityList * next;		/* Link to fork list.		*/
	    struct ActivityList ** fork;	/* Link to join from fork.	*/
	    Activity ** source;			/* Link to source activity 	*/
	    result_t r_join;			/* results for join delays	*/
	    result_t r_join_sqr;		/* results for delays.		*/
	    join_type_t join_type;
	    int quorumCount; 			/* tomari quorum		*/
	    Histogram * _hist_data;
	} join;
	struct {
	    struct ActivityList * prev;		/* Link to join list.		*/
	    Activity * endlist;			/* For repeat nodes. 		*/
	    double * count;			/* array of iterations		*/
	    double total;			/* total iterations.		*/
	} loop;
    } u;
    unsigned na;
    unsigned maxa;
    Activity ** list;				/* Array of activities.		*/
    LQIO::DOM::ActivityList* _dom_actlist;

    void free();
    struct ActivityList& configure();

    bool is_join_list() const;
    bool is_fork_list() const;
    bool is_loop_list() const;
};

void print_activity_connectivity( FILE *, Activity * );
double fork_find_children( ActivityList * input, std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * ep );
double join_find_children( ActivityList * input, std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * ep );
double fork_collect( ActivityList * input, std::deque<Activity *>& activity_stack, ActivityList::Collect& data );
double join_collect( ActivityList * input, std::deque<Activity *>& activity_stack, ActivityList::Collect& data );
void join_check( ActivityList * join_list );

/* Used by load.cc */

void complete_activity_connections ();
#endif
