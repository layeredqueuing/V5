/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/* January 2005.							*/
/************************************************************************/

#ifndef _ACTLIST_H
#define _ACTLIST_H

/*
 * $Id: petrisrvn.h 10943 2012-06-13 20:21:13Z greg $
 *
 * Solve LQN using petrinets.
 */

#include <deque>
#include "petrisrvn.h"
#include "phase.h"

namespace LQIO {
    namespace DOM {
	class ActivityList;
	class ExternalVariable;
    }
}

class Activity;
class Entry;
class Task;

class ActivityList {
    friend class Activity;

public:
    enum class Type {
	FORK,
	OR_FORK,
	AND_FORK,
	LOOP,
	JOIN,
	AND_JOIN,
	OR_JOIN
    };

    enum class JoinType
    {
	UNDEFINED,
	INTERNAL_FORK_JOIN,
	SYNCHRONIZATION
    };
    
private:
    ActivityList& operator=( const ActivityList& );
    ActivityList( const ActivityList& );
    
public:
    ActivityList( Type, LQIO::DOM::ActivityList * );
    virtual ~ActivityList() {}

    LQIO::DOM::ActivityList * get_dom() const { return _dom; }
    Type type() const { return _type; }
    unsigned int n_acts() const { return _n_acts; }
    char * fork_join_name() const;
    
    bool is_join_list() const;
    bool is_fork_list() const;
    bool is_loop_list() const;

    bool check_external_joins() const;
    bool check_fork_no_join() const;
    bool check_quorum_join() const;
    bool check_fork_has_join() const;

    void insert_DOM_results() const;

    bool join_find_children( std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * e );
    bool fork_find_children( std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * e );
    double fork_count_replies( std::deque<Activity *>& activity_stack,  const Entry * e, const double rate, unsigned curr_phase, unsigned& next_phase  );
    double join_count_replies( std::deque<Activity *>& activity_stack,  const Entry * e, const double rate, const unsigned curr_phase, unsigned& next_phase );
    void follow_join_for_tokens( const Entry * e, unsigned p, const unsigned m, Activity * curr_activity,
				 const bool sum_forks, double scale, Phase::util_fnptr util_func, double mean_tokens[] );

    void remove_netobj();

private:
    void find_fork_list( const Task * curr_task, std::deque<Activity *>& activity_stack, int depth, std::deque<ActivityList *>& fork_stack );
    bool add_to_join_list( unsigned int i, Activity * an_activity );
    void path_error( int err, const char * task_name, std::deque<Activity *>& activity_stack );
    bool set_join_type( JoinType a_type );
    int backtrack( Activity * activity, std::deque<ActivityList *>& fork_stack, Activity * start_activity );

    void follow_fork_for_tokens( const Entry * e, unsigned p, const unsigned m, 
				 const bool sum_forks, const double scale, Phase::util_fnptr util_func, double mean_tokens[] );


private:
    const Type _type;
    LQIO::DOM::ActivityList * _dom;
    union {
	struct {
	    ActivityList * prev;		/* Link to join list.		*/
	    ActivityList * join;		/* My join if I am a fork.	*/
	    const LQIO::DOM::ExternalVariable * prob[MAX_BRANCH];		/* Array of probabilities.	*/
	    bool reachable[MAX_BRANCH];		/* true if backtrack finds me	*/
	} fork;
	struct {
	    ActivityList * next;		/* Link to fork list.		*/
	    ActivityList * fork[MAX_BRANCH];	/* My fork if I am a join.*/
	    Activity * source[MAX_BRANCH];   	/* My start activity 	*/
	    struct place_object * FjM[MAX_MULT];/* Measuring place.		*/
	    double tokens[MAX_MULT];		/* Join-delay Result.		*/
	    JoinType type;			/* join type.			*/
#if defined(BUG_263)
	    int quorumCount; 			/* BUG_263			*/
#endif
	} join;
	struct {
	    ActivityList * prev;		/* Link to join list.		*/
	    const LQIO::DOM::ExternalVariable * count[MAX_BRANCH];
	    Activity * endlist;			/* For repeat nodes. 		*/
	    struct place_object * LoopP[MAX_MULT];	/* Loop Place		*/
	    struct trans_object * LoopT[MAX_MULT];	/* Loop Trans.		*/
	} loop;
    } u;
    unsigned _n_acts;
    Activity * list[MAX_BRANCH];		/* Array of activities.		*/
    struct trans_object * FjT[MAX_BRANCH][MAX_MULT];	/* Join Transition.	*/
    struct trans_object * FjF[MAX_MULT];	/* And Fork transition.		*/
    struct place_object * FjP[MAX_MULT];	/* And Join Place		*/
    const Entry * entry[MAX_BRANCH];		/* Calling entry.		*/
};

#endif
