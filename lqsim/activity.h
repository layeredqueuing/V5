/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May  1996.								*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $Id: activity.h 14381 2021-01-19 18:52:02Z greg $
 */

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <string>
#include <map>
#include <deque>
#include <lqio/dom_activity.h>
#include "actlist.h"
#include "histogram.h"
#include "target.h"
#include "result.h"

class Task;
class Entry;

typedef double (*distribution_func_ptr)( double, double );

class Activity {
    friend class Instance;
    

public:
    Activity( Task * cp=NULL, LQIO::DOM::Phase * dom=NULL );
    virtual ~Activity();

    const char * name() const { return _name.c_str(); }
    Task * task() const { return _task; }			/* pointer to task.	        */
    unsigned int index() const { return _index; }
    
    Activity& set_task( Task * cp ) { _task = cp; return *this; }
    Activity& set_phase( unsigned int p ) { _phase = p; return *this; }

    double cv_sqr() const { return (_dom && _dom->hasCoeffOfVariationSquared()) ? _dom->getCoeffOfVariationSquaredValue() : 1.0; }
    double service() const;
    double think_time() const { return _think_time; }		/* Need to cache _think_time!!! */
    LQIO::DOM::Phase::Type type() const { return _dom ? _dom->getPhaseTypeFlag() : LQIO::DOM::Phase::Type::STOCHASTIC; }

    bool is_specified() const { return _dom != 0; } 	/* True if some value set.	*/
    bool is_activity() const { return dynamic_cast<LQIO::DOM::Activity *>(_dom) != 0; }
    bool is_reachable() const { return _is_reachable; }			/* True if we can reach it	*/
    Activity& set_is_start_activity( bool val ) { _is_start_activity = val; return *this; }
    bool is_start_activity() const { return _is_start_activity; }
    bool is_phase() const { return dynamic_cast<LQIO::DOM::Activity *>(_dom) == 0; };
    bool has_service_time() const { return _scale > 0.; }
    bool has_think_time() const { return _think_time > 0.; }
    bool has_lost_messages() const;
    
    void set_arrival_rate( const double r ) { _arrival_rate = r; }
    double get_slice_time() { return (*distribution_func)( _scale, _shape ); }
    Activity& set_DOM( LQIO::DOM::Phase* phaseInfo );
    LQIO::DOM::Phase* get_DOM() const { return _dom; }
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

    const Activity& print_raw_stat( FILE * output ) const;
    void print_debug_info();
    double find_children( std::deque<Activity *>& activity_stack, std::deque<ActivityList *>& fork_stack, const Entry * ep );

    double compute_minimum_service_time() const;
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
    ActivityList * realloc_list ( const list_type type, const ActivityList * input_list,  LQIO::DOM::ActivityList * dom_activitylist );
    const Entry * find_reply( const Entry * ep ) const;

private:
    LQIO::DOM::Phase* _dom;
    
    /*
     * Likely candidates for DOM stuff -- note that anything which is
     * called frequently from execute.c should have a cached copy
     * here.
     */
    const string _name;			/* Name of activity.		*/
    double _arrival_rate;		/* service time			*/
    double _cv_sqr;			/* cv_sqr			*/
    double _think_time;			/* Cached think time.	        */
    
    Task * _task;			/* Pointer to task class	*/
    unsigned _phase;			/* Phase number (not needed)	*/
    double _scale;			/* "scale" for slice distrib.	*/
    double _shape;			/* "shape" for slice distrib.	*/
    distribution_func_ptr distribution_func;
    const unsigned int _index;		/* My index (for joins.)	*/
    double _prewaiting;			/* Used for calculating the task waiting time variance only. Tao*/ 
    std::vector<const Entry *> _reply;	/* reply list.			*/
    
    bool _is_reachable;			/* True if we can reach it	*/
    bool _is_start_activity;		/* True if I am a start activity*/

public:
    ActivityList *_input;		/* Node which calls me.		*/
    ActivityList *_output;		/* Node which I call.		*/
    Targets tinfo;			/* target info			*/
    unsigned _active;			/* Number of active instances.	*/
    unsigned _cpu_active;		/* Number of active instances.	*/
    Histogram * _hist_data;            	/* histogram data.		*/
    result_t r_util;			/* Phase utilization.	        */
    result_t r_cpu_util;		/* Execution time.		*/
    result_t r_service;			/* Service time.		*/
    result_t r_slices;			/* Number of slices. 		*/
    result_t r_sends;			/* Actual # of sends.		*/
    result_t r_proc_delay; 		/* Delay to getting processor	*/
    result_t r_proc_delay_sqr; 		/* Delay to getting processor	*/
    result_t r_cycle;			/* Entry cycle time.	        */
    result_t r_cycle_sqr;  		/* Entry cycle time.	        */
    result_t r_afterQuorumThreadWait;	/* start tomari quorum 		*/

    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> domToNative;
};

typedef double (*activity_func_ptr)( const Activity * ap );

extern unsigned join_count;		/* non-zero if any joins	*/
extern unsigned fork_count;		/* non-zero if any forks	*/

Activity * find_or_create_activity ( const void * task, const char * activity_name );
void print_activity_info( FILE * output, const Activity * ap, const bool parse_flag, activity_func_ptr mean, activity_func_ptr stddev, activity_func_ptr mean2 );
void act_print_raw_stat( FILE * output, Activity * ap );
int act_find_phase_2( const Entry * ep, Activity * ap, int my_phase);

/*
 * Compare a entry name to a string.  Used by the find_if (and other algorithm type things).
 */

struct eqActivityStr 
{
    eqActivityStr( const char * s ) : _s(s) {}
    bool operator()(const Activity * p1 ) const { return strcmp( p1->name(), _s ) == 0; }

private:
    const char * _s;
};


#endif

