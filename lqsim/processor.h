/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996.								*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqsim/processor.h $
 *
 * $Id: processor.h 10477 2011-09-12 23:32:33Z greg $
 */

#ifndef	PROCESSOR_H
#define PROCESSOR_H

#include <set>
#include <vector>
#include <string>
#include <ostream>
#include <lqio/dom_processor.h>
#include "result.h"

extern void add_processor( LQIO::DOM::Processor* processor );

class Group;
class Instance;
class Task;

using namespace std;

class Processor {
private:
    /*
     * Translate input.h types to para_proto.h types.
     */

    static int scheduling_types[N_SCHEDULING_TYPES];

protected:
    static Processor * processor_table[MAX_NODES+1];

public:
    static Processor * find( const char * processor_name );
    static Processor * find( const int i ) { return processor_table[i]; }
    static void reschedule( Instance * ip );

private:
    Processor( const Processor& );
    Processor& operator=( const Processor& );

public:
    Processor( LQIO::DOM::Processor * );
    virtual ~Processor() {}
    virtual bool create();

    LQIO::DOM::Processor * getDOMProcessor() const { return _domProcessor; }

    const char * name() const { return _domProcessor->getName().c_str(); }
    double cpu_rate() const { return _domProcessor->hasRate() ? _domProcessor->getRateValue() : 1.0; }	/* Processor rate.		*/
    double quantum() const { return _domProcessor->hasQuantum() ? _domProcessor->getQuantumValue() : 0.0; }	/* Time quantum.		*/
    int replicas() const { return _domProcessor->getReplicas(); } 
    scheduling_type discipline() const { return _domProcessor->getSchedulingType(); }
    unsigned multiplicity() const;					/* Special access!		*/

    long node_id() const { return _node_id; }

    bool is_infinite() const;
    bool is_multiserver() const { return multiplicity() > 1; }
    bool derive_utilization() const;

    void add_task( Task * );

    void insertDOMResults();

public:
    bool trace_flag;			/* For tracing.			*/
    result_t r_util;			/* Utilization.			*/
    Group * group;			/*				*/

protected:
    long _node_id;			/* Parasol node id	.	*/

private:
    LQIO::DOM::Processor * _domProcessor;
    vector<Task *> _tasks;
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

    static void cpu_scheduler_task ( void * );

public:
    Custom_Processor( LQIO::DOM::Processor * );
    virtual ~Custom_Processor();

    virtual bool create();
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

/* ------------------------------------------------------------------------ */
/*
 * Compare to processors by their name.  Used by the set class to insert items
 */

struct ltProcessor
{
    bool operator()(const Processor * p1, const Processor * p2) const { return strcmp( p1->name(), p2->name() ) < 0; }
};


/*
 * Compare a processor name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqProcStr 
{
    eqProcStr( const char * s ) : _s(s) {}
    bool operator()(const Processor * p1 ) const { return strcmp( p1->name(), _s ) == 0; }

private:
    const char * _s;
};

extern set <Processor *, ltProcessor> processor;	/* Processor table.	*/


#endif
