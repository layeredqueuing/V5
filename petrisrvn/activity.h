/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* January 2005.							*/
/************************************************************************/

#ifndef _ACTIVITY_H
#define _ACTIVITY_H

/*
 * $Id: petrisrvn.h 10943 2012-06-13 20:21:13Z greg $
 *
 * Solve LQN using petrinets.
 */

#include "phase.h"
#include <set>
#include "actlist.h"

class Task;
class Entry;
class ActivityList;

namespace LQIO {
    namespace DOM {
	class ActivityList;
    }
}

class Activity : public Phase {
    friend void ActivityList::insert_DOM_results();

public:    
    Activity( LQIO::DOM::Activity *, Task * );
    virtual ~Activity() {}

    bool is_specified() const { return _is_specified; }
    virtual bool is_activity() const { return true; }
    unsigned int n_replies() const { return _replies.size(); }

    virtual double check();

    bool find_children( std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * e );
    double count_replies( std::deque<Activity *>& activity_stack, const Entry * e, const double rate, const unsigned curr_phase, unsigned& next_phase  );
    bool replies_to( const Entry * e ) const;

    void follow_activity_for_tokens( const Entry * e, unsigned p, const unsigned m,
				     const bool sum_forks, const double scale,
				     util_fnptr util_func, double mean_tokens[] );

    ActivityList * input() const { return _input; }

    Activity& add_reply_list();
    Activity& add_activity_lists();

    double transmorgrify( const double x_pos, const double y_pos, const unsigned m,
			  const Entry * e, const double p_pos, const short enabling,
			  struct place_object * end_place, bool can_reply );

    virtual double residence_time() const;
    virtual void insert_DOM_results();

    void activity_cycle_error( int err, const char *, std::deque<Activity *>& activity_stack );
    static void complete_activity_connections();

private:
    ActivityList * act_join_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylistint );
    ActivityList * act_or_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_fork_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_loop_list( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * realloc_list ( const list_type type, const ActivityList * input_list, LQIO::DOM::ActivityList * dom_activity_list );
    bool link_activity( double x_pos, double y_pos, const Entry * e, const unsigned m,
			double &p_pos, const short enabling,
			struct place_object * end_place, const bool can_reply );
    static void act_connect( ActivityList * src, ActivityList * dst );

private:
    ActivityList *_input;			/* Node which calls me.		*/
    ActivityList *_output;			/* Node which I call.		*/
    std::set<Entry *> _replies;			/* I reply to...		*/
    bool _is_start_activity;			/* I am a start activity	*/
    bool _is_reachable;				/* True if we can reach it	*/
    bool _is_specified;				/* True if I have an arg.	*/
    double _throughput[MAX_MULT];		/* Results.			*/

public:
    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> domToNative;
};

struct eqActivityStr 
{
    eqActivityStr( const std::string& s ) : _s(s) {}
    bool operator()(const Activity * p1 ) const { return _s == p1->name(); }

private:
    const std::string& _s;
};

#endif
