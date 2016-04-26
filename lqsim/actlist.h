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
 * $Id: actlist.h 11133 2012-10-04 14:02:19Z greg $
 */

#ifndef ACTLIST_H
#define ACTLIST_H

#include "stack.h"
#include "result.h"

class Entry;
class Activity;
class Histogram;
class Task;
class Activity;
namespace LQIO {
    namespace DOM {
	class ActivityList;
    }
}

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

typedef struct activity_list_t {
    
    list_type type;
    union {
	struct {
	    struct activity_list_t * prev;	/* Link to join list.		*/
	    struct activity_list_t * join;	/* Link to fork from join.	*/
	    bool * visit;			/* true if I visit a join.	*/
	    double * prob;			/* Array of probabilities.	*/
	    unsigned visit_count;		/* */
	} fork;
	struct {
	    struct activity_list_t * next;	/* Link to fork list.		*/
	    struct activity_list_t ** fork;	/* Link to join from fork.	*/
	    Activity ** source;			/* Link to source activity 	*/
	    result_t r_join;			/* results for join delays	*/
	    result_t r_join_sqr;		/* results for delays.		*/
	    unsigned visits;
	    join_type_t join_type;
	    int quorumCount; 			/* tomari quorum		*/
	    Histogram * _hist_data;
	} join;
	struct {
	    struct activity_list_t * prev;	/* Link to join list.		*/
	    Activity * endlist;			/* For repeat nodes. 		*/
	    double * count;			/* array of iterations		*/
	    double total;			/* total iterations.		*/
	} loop;
    } u;
    unsigned na;
    unsigned maxa;
    Activity ** list;				/* Array of activities.		*/
    LQIO::DOM::ActivityList* _dom_actlist;
} ActivityList;

void print_activity_connectivity( FILE *, Activity * );
double fork_find_children( activity_list_t * input, para_stack_t * activity_stack, para_stack_t * fork_stack, const Entry * ep );
double join_find_children( activity_list_t * input, para_stack_t * activity_stack, para_stack_t * fork_stack, const Entry * ep );
double fork_count_replies( activity_list_t * input, para_stack_t * activity_stack, const Entry * ep, const double rate, const unsigned int my_phase, unsigned int * next_phase );
double join_count_replies( activity_list_t * input, para_stack_t * activity_stack, const Entry * ep, const double rate, const unsigned int my_phase, unsigned int * next_phase );
void join_check( activity_list_t * join_list );
bool is_join_list( const activity_list_t * list );
bool is_fork_list( const activity_list_t * list );
bool is_loop_list( const activity_list_t * list );
void free_list( activity_list_t ** list_p );
void configure_list( activity_list_t * list );

/* Used by load.cc */

void complete_activity_connections ();
#endif
