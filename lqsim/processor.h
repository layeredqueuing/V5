/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996.								*/
/************************************************************************/

/*
 * Lqsim-parasol Processor interface.
 *
 * $Id: processor.h 17600 2025-11-25 20:26:11Z greg $
 */

#ifndef	PROCESSOR_H
#define PROCESSOR_H

#include <vector>
#include <map>
#include <string>
#if HAVE_PARASOL
#include <parasol/para_internals.h>
#else
#include <mutex>
#endif
#include <lqio/dom_processor.h>
#include "lqsim.h"
#include "result.h"

class Group;
class Task;
namespace Instance {
    class Instance;
}

class Processor {
public:
    typedef void (Processor::*compute_fptr)( double );

#if HAVE_PARASOL
    static inline double now() { return ps_now; }
    inline void sleep( double time ) { ps_sleep( time ); }
#else
    static double now();
    void sleep( double time );
#endif

    /*
     * Compare to processors by their name.  Used by the set class to
     * insert items
     */

    struct ltProcessor
    {
	bool operator()(const Processor * p1, const Processor * p2) const { return p1->name() < p2->name(); }
    };

    static std::set<Processor *, ltProcessor> __processors;	/* Processor table.	*/

private:
    /*
     * Translate input.h types to para_proto.h types.
     */

#if HAVE_PARASOL
    static const std::map<const scheduling_type,const int> scheduling_types;
#endif
   
    
protected:
#if HAVE_PARASOL
    static Processor * processor_table[MAX_NODES+1];
#endif

public:
    static Processor * find( const std::string& );
#if HAVE_PARASOL
    static Processor * find( const int i ) { return processor_table[i]; }
#endif
    static void reschedule( Instance::Instance * ip );
    static void add( const std::pair<std::string,LQIO::DOM::Processor*>& );

private:
    Processor( const Processor& ) = delete;
    Processor& operator=( const Processor& ) = delete;

public:
    Processor( LQIO::DOM::Processor * );
    virtual ~Processor() {}

    LQIO::DOM::Processor * getDOM() const { return _dom; }

    const std::string& name() const { return _dom->getName(); }
    double cpu_rate() const { return _dom->hasRate() ? _dom->getRateValue() : 1.0; }		/* Processor rate.		*/
    double quantum() const { return _dom->hasQuantum() ? _dom->getQuantumValue() : 0.0; }	/* Time quantum.		*/
    int replicas() const { return _dom->hasReplicas() ? _dom->getReplicasValue() : 0.0; } 
    scheduling_type discipline() const { return _dom->getSchedulingType(); }
    unsigned multiplicity() const;								/* Special access!		*/
    void add_task( Task * );

#if HAVE_PARASOL
    Processor& create();
    long node_id() const { return _node_id; }
#else
//   compute will add event (END) with time + delta
//    void acquire();	// get processor lock, insert onto event list for start  increment global bust count.
//    void release();	// release processor lock, decrement global busy count.
#endif

    bool is_infinite() const;
    bool is_multiserver() const { return multiplicity() > 1; }
    bool derive_utilization() const;

    compute_fptr get_compute_func() const;

    virtual Processor& reset_stats() { r_util.reset(); return *this; }
    virtual Processor& accumulate_data() { r_util.accumulate(); return *this; }
    virtual Processor& insertDOMResults();

    std::ostream& print( std::ostream& ) const;

private:
    void compute( double time );

public:
    bool trace_flag;			/* For tracing.			*/
#if HAVE_PARASOL
    ParasolResult r_util;		/* Utilization.			*/
#else
    VariableResult r_util;		/* Utilization.			*/
#endif
    Group * _group;			/*				*/

protected:
#if HAVE_PARASOL
    long _node_id;			/* Parasol node id	.	*/
#endif
    int _active;			/* Number of processors running.*/

private:
    LQIO::DOM::Processor * _dom;
    std::vector<Task *> _tasks;
#if !HAVE_PARASOL
    std::mutex _mutex;
#endif
};

#if HAVE_PARASOL

class Custom_Processor : public Processor
{
    typedef enum
    {
	PROC_RUNNING_TASK,
	PROC_PRIO_PREEMPTING_TASK,
	PROC_PREEMPTING_TASK,
	PROC_IDLE,
	PROC_GENERAL
    } processor_events;

    static void cpu_scheduler_task( void * );

public:
    Custom_Processor( LQIO::DOM::Processor * );
    virtual ~Custom_Processor();

    virtual Custom_Processor& create();
    long scheduler() const { return _scheduler; }

private:
    void main();
    double run_task( long );
    void trace( const processor_events event, ... );
    
private:
    Instance::Instance ** _active_task;	/* Active tasks.		*/
    long _scheduler;			/* Port of the scheduler.	*/
};
#endif
#endif
