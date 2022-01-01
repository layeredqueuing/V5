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
 * $Id: processor.h 15317 2022-01-01 16:44:56Z greg $
 */

#ifndef	PROCESSOR_H
#define PROCESSOR_H

#include <vector>
#include <map>
#include <string>
#include <lqio/dom_processor.h>
#include "result.h"

class Group;
class Instance;
class Task;

class Processor {
public:

    /*
     * Compare to processors by their name.  Used by the set class to
     * insert items
     */

    struct ltProcessor
    {
	bool operator()(const Processor * p1, const Processor * p2) const { return strcmp( p1->name(), p2->name() ) < 0; }
    };


    /*
     * Compare a processor name to a string.  Used by the find_if (and
     * other algorithm type things.
     */

    struct eqProcStr 
    {
    eqProcStr( const std::string& s ) : _s(s) {}
	bool operator()(const Processor * p1 ) const { return _s == p1->name(); }

    private:
	const std::string _s;
    };

    static std::set<Processor *, ltProcessor> __processors;	/* Processor table.	*/

private:
    /*
     * Translate input.h types to para_proto.h types.
     */

    static const std::map<const scheduling_type,const int> scheduling_types;

protected:
    static Processor * processor_table[MAX_NODES+1];

public:
    static Processor * find( const std::string& );
    static Processor * find( const int i ) { return processor_table[i]; }
    static void reschedule( Instance * ip );
    static void add( const std::pair<std::string,LQIO::DOM::Processor*>& );

private:
    Processor( const Processor& ) = delete;
    Processor& operator=( const Processor& ) = delete;

public:
    Processor( LQIO::DOM::Processor * );
    virtual ~Processor() {}
    Processor& create();

    LQIO::DOM::Processor * getDOM() const { return _dom; }

    const char * name() const { return _dom->getName().c_str(); }
    double cpu_rate() const { return _dom->hasRate() ? _dom->getRateValue() : 1.0; }	/* Processor rate.		*/
    double quantum() const { return _dom->hasQuantum() ? _dom->getQuantumValue() : 0.0; }	/* Time quantum.		*/
    int replicas() const { return _dom->hasReplicas() ? _dom->getReplicasValue() : 0.0; } 
    scheduling_type discipline() const { return _dom->getSchedulingType(); }
    unsigned multiplicity() const;					/* Special access!		*/

    long node_id() const { return _node_id; }

    bool is_infinite() const;
    bool is_multiserver() const { return multiplicity() > 1; }
    bool derive_utilization() const;

    void add_task( Task * );

    virtual Processor& reset_stats() { r_util.reset(); return *this; }
    virtual Processor& accumulate_data() { r_util.accumulate(); return *this; }
    virtual Processor& insertDOMResults();

public:
    bool trace_flag;			/* For tracing.			*/
    result_t r_util;			/* Utilization.			*/
    Group * _group;			/*				*/

protected:
    long _node_id;			/* Parasol node id	.	*/

private:
    LQIO::DOM::Processor * _dom;
    std::vector<Task *> _tasks;
};



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
    int _active;			/* Number of processors running.*/
    Instance ** _active_task;		/* Active tasks.		*/
    long _scheduler;			/* Port of the scheduler.	*/
};
#endif
