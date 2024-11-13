/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May  1996.								*/
/************************************************************************/

/*
 * Activities (and phases).
 *
 * $Id: activity.h 17457 2024-11-12 11:19:54Z greg $
 */

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <string>
#include <map>
#include <deque>
#include <lqio/dom_activity.h>
#include "actlist.h"
#include "random.h"
#include "result.h"
#include "target.h"

class Task;
class Entry;
class Histogram;

class Activity {
    friend class Instance;		// for _calls;
    friend class Entry;
    friend class Pseudo_Entry;
    
public:
    Activity( Task * cp, LQIO::DOM::Phase * dom );
    virtual ~Activity();

    const std::string& name() const { return _name; }
    Task * task() const { return _task; }			/* pointer to task.	        */
    unsigned int index() const { return _index; }
    
    Activity& set_task( Task * cp ) { _task = cp; return *this; }
    Activity& set_phase( unsigned int p ) { _phase = p; return *this; }
    Activity& set_service_time( double s ) { _service_time = s; return *this; }	/* for open arrival sources */
    double get_service_time() const;

    double cv_sqr() const { return (_dom && _dom->hasCoeffOfVariationSquared()) ? _dom->getCoeffOfVariationSquaredValue() : 1.0; }
    LQIO::DOM::Phase::Type type() const { return _dom ? _dom->getPhaseTypeFlag() : LQIO::DOM::Phase::Type::STOCHASTIC; }

    bool is_specified() const { return has_service_time() || has_think_time() || has_calls(); } 	/* True if some value set.	*/
    bool is_activity() const { return dynamic_cast<LQIO::DOM::Activity *>(_dom) != nullptr; }
    bool is_reachable() const { return _is_reachable; }			/* True if we can reach it	*/
    Activity& set_is_start_activity( bool val ) { _is_start_activity = val; return *this; }
    bool is_start_activity() const { return _is_start_activity; }
    bool is_phase() const { return dynamic_cast<LQIO::DOM::Activity *>(_dom) == nullptr; };
    bool has_service_time() const { return _dom != nullptr && _dom->hasServiceTime(); }
    bool has_think_time() const { return _dom != nullptr && _dom->hasThinkTime(); }
    bool has_calls() const { return _calls.size() > 0; }
    bool has_lost_messages() const;
    
    double get_slice_time() { return (*_slice_time)(); }
    double get_think_time() { return (*_think_time)(); }
    LQIO::DOM::Phase* getDOM() const { return _dom; }
    const std::vector<LQIO::DOM::Call*>& get_calls() const { return _dom->getCalls(); }
    
    Activity& rename( const std::string& );
    double configure();
    Activity& initialize();

    double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& );
    double count_replies( ActivityList::Collect& data ) const;

    Activity& add_calls();
    Activity& add_reply_list();
    Activity& add_activity_lists();
    Activity& act_add_reply( Entry * );

    std::ostream& print( std::ostream& output ) const;
    void print_debug_info();
    double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep );

    double compute_minimum_service_time( std::deque<Entry *>& ) const;
    double compute_minimum_service_time( ActivityList::Collect& data ) const;

    Activity& reset_stats();
    Activity& accumulate_data();
    Activity& insertDOMResults();

private:
    ActivityList * act_join_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylistint );
    ActivityList * act_or_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_fork_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_loop_list( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * realloc_list ( const ActivityList::Type type, const ActivityList * input_list,  LQIO::DOM::ActivityList * dom_activitylist );
    bool replies_to( const Entry * ep ) const;

private:
    LQIO::DOM::Phase* _dom;
    
    const std::string _name;		/* Name of activity.		*/
    double _service_time;		/* service time			*/
    double _cv_sqr;			/* cv_sqr			*/
    Random * _slice_time;		/* Distribution generator	*/
    Random * _think_time;		/* Distribution generator	*/
    Targets _calls;			/* target info			*/
    
    Task * _task;			/* Pointer to task class	*/
    unsigned _phase;			/* Phase number (not needed)	*/
    
    const unsigned int _index;		/* My index (for joins.)	*/
    double _prewaiting;			/* Used for calculating the task waiting time variance only. Tao*/ 
    std::vector<const Entry *> _reply;	/* reply list.			*/
    
    bool _is_reachable;			/* True if we can reach it	*/
    bool _is_start_activity;		/* True if I am a start activity*/

public:
    InputActivityList *_input;		/* Node which calls me.		*/
    OutputActivityList *_output;	/* Node which I call.		*/
    unsigned _active;			/* Number of active instances.	*/
    unsigned _cpu_active;		/* Number of active instances.	*/
    Histogram * _hist_data;            	/* histogram data.		*/
    VariableResult r_util;		/* Phase utilization.	        */
    VariableResult r_cpu_util;		/* Execution time.		*/
    SampleResult r_service;		/* Service time.		*/
    SampleResult r_slices;		/* Number of slices. 		*/
    SampleResult r_sends;		/* Actual # of sends.		*/
    SampleResult r_proc_delay; 		/* Delay to getting processor	*/
    SampleResult r_proc_delay_sqr;	/* Delay to getting processor	*/
    SampleResult r_cycle;		/* Entry cycle time.	        */
    SampleResult r_cycle_sqr;  		/* Entry cycle time.	        */
    SampleResult r_afterQuorumThreadWait;	/* start tomari quorum 		*/

    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> domToNative;
};
#endif
