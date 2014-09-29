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
 * $Id$
 */

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <string>
#include <map>
#include <lqio/dom_activity.h>
#include "actlist.h"
#include "histogram.h"
#include "target.h"
#include "stack.h"
#include "result.h"

class Task;
class Entry;
namespace LQIO {
    namespace DOM {
	class ActivityList;
    };
};

typedef double (*distribution_func_ptr)( double, double );

extern int activity_count; // global variable

class Activity {
    friend class Instance;
    

public:
    Activity( Task * cp=0, LQIO::DOM::Phase * dom_phase=0 );
    virtual ~Activity();

    const char * name() const { return _name.c_str(); }
    Task * task() const { return _task; }			/* pointer to task.	        */

    double cv_sqr() const { return (_dom_phase && _dom_phase->hasCoeffOfVariationSquared()) ? _dom_phase->getCoeffOfVariationSquaredValue() : 1.0; }
    double service() const;
    double think_time() const { return _think_time; }		/* Need to cache _think_time!!! */
    phase_type type() const { return _dom_phase ? _dom_phase->getPhaseTypeFlag() : PHASE_STOCHASTIC; }

    bool is_specified() const { return _dom_phase != 0; } 	/* True if some value set.	*/
    bool is_activity() const { return dynamic_cast<LQIO::DOM::Activity *>(_dom_phase) != 0; }
    bool is_phase() const { return dynamic_cast<LQIO::DOM::Activity *>(_dom_phase) == 0; };
    bool has_service_time() const { return _scale > 0.; }
    bool has_lost_messages() const;
    
    void set_cv_sqr( const double c ) { abort(); }
    void set_phase_type( phase_type t ) { abort(); }		
    void set_service( const double s );
    void set_think_time( const double t ) { abort(); }
    double get_slice_time() { return (*distribution_func)( _scale, _shape ); }
    Activity& set_DOM( LQIO::DOM::Phase* phaseInfo );
    LQIO::DOM::Phase* get_DOM() const { return _dom_phase; }
    const std::vector<LQIO::DOM::Call*>& get_calls() const { return _dom_phase->getCalls(); }
    
    Activity& rename( const char * );
    double configure( Task * cp = 0 );

    double count_replies( para_stack_t * activity_stack, const Entry * ep, const double rate, const unsigned int curr_phase, unsigned int * next_phase );

    Activity& add_calls();
    Activity& add_reply_list();
    Activity& add_activity_lists();
    Activity& act_add_reply( Entry * );

    FILE * print_raw_stat( FILE * output ) const;
    void print_debug_info();
    double find_children( para_stack_t * activity_stack, para_stack_t * fork_stack, const Entry * ep );

    void reset_stats();
    void accumulate_data();

    void insertDOMResults();

private:
    ActivityList * act_join_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylistint );
    ActivityList * act_or_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_fork_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_loop_list( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * realloc_list ( const list_type type, const ActivityList * input_list,  LQIO::DOM::ActivityList * dom_activitylist );
    const Entry * find_reply( const Entry * ep );

private:
    LQIO::DOM::Phase* _dom_phase;
    
    /*
     * Likely candidates for DOM stuff -- note that anything which is
     * called frequently from execute.c should have a cached copy
     * here.
     */
    const string _name;			/* Name of activity.		*/
    double _service;			/* service time			*/
    double _cv_sqr;			/* cv_sqr			*/
    double _think_time;			/* Cached think time.	        */
    
    Task * _task;			/* Pointer to task class	*/
    double _scale;			/* "scale" for slice distrib.	*/
    double _shape;			/* "shape" for slice distrib.	*/
    distribution_func_ptr distribution_func;
    
public:
    int index;				/* My index (for joins.)	*/
    struct activity_list_t *_input;	/* Node which calls me.		*/
    struct activity_list_t *_output;	/* Node which I call.		*/
    Targets tinfo;			/* target info			*/
    unsigned _active;			/* Number of active instances.	*/
    unsigned _cpu_active;		/* Number of active instances.	*/
    unsigned my_phase;			/* True if in phase 2.		*/
    bool is_reachable;			/* True if we can reach it	*/
    bool is_start_activity;		/* True if I am a start activity*/
    double pt_prewaiting;		/* Used for calculating the task waiting time variance only. Tao*/ 
    Histogram * _hist_data;            	/* Structure which stores histogram data for this activity */
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

    int activity_id;

    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> domToNative;

private:
    vector<const Entry *> _reply;	/* reply list.			*/
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

