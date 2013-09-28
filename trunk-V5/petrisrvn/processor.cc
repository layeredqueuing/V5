/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/************************************************************************/

/*
 * $Id: petrisrvn.cc 10943 2012-06-13 20:21:13Z greg $
 *
 * Generate a Petri-net from an SRVN description.
 *
 */

#include <algorithm>
#include <lqio/glblerr.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_processor.h>
#include "petrisrvn.h"
#include "processor.h"
#include "task.h"
#include "entry.h"
#include "activity.h"
#include "results.h"
#include "makeobj.h"
#include "errmsg.h"
#include "pragma.h"

using namespace std;

double Processor::__x_offset;

std::vector<Processor *> processor;

/*----------------------------------------------------------------------*/
/* Processors.								*/
/*----------------------------------------------------------------------*/

/*
 * Add a processor to the model.   For multi-servers, we just clone
 * the processor.  The number_of_cpus[proc_id] is set so that any clone
 * can determine this piece of info.
 */

/*ARGSUSED*/

Processor::Processor( LQIO::DOM::Entity * dom )
    : Place( dom ),
      PX(0)
{
    initialize();
}


void Processor::initialize() 
{
    PX = 0;
    for ( unsigned e = 0; e <= DIME; ++e ) {
	_history[e].task = 0;
	_history[e].request_place = 0;
	_history[e].grant_trans   = 0;
	_history[e].grant_place   = 0;
    }
    for ( unsigned m = 0; m < MAX_MULT; ++m ) {
	proc_tokens[m] = 0.0;
    }
}


void Processor::create( LQIO::DOM::Processor * dom )
{
    const string& processor_name = dom->getName();
    if ( processor_name.size() == 0 ) abort();

    if ( dom->getRateValue() <= 0.0 ) {
	LQIO::input_error2( LQIO::ERR_INVALID_PROC_RATE, processor_name.c_str(), dom->getRate() );
    }
    if ( dom->getReplicas() != 1 ) {
	LQIO::input_error2( ERR_REPLICATION, "processor", processor_name.c_str() );
    }
    const unsigned int m = dom->getCopiesValue();
    if ( m == 0 ) {
	dom->setSchedulingType(SCHEDULE_DELAY);	/* Force processor to infinite server. */
    } else if ( m > MAX_MULT ) {
	LQIO::input_error2( LQIO::ERR_TOO_MANY_X, "cpus ", MAX_MULT );
    }

    if ( pragma.processor_scheduling() != SCHEDULE_FIFO && dom->getSchedulingType() != SCHEDULE_DELAY ) {
	dom->setSchedulingType( pragma.processor_scheduling() );
    }
	
    const scheduling_type scheduling_flag = dom->getSchedulingType();
    if ( !bit_test( scheduling_flag, SCHED_PPR_BIT|SCHED_HOL_BIT|SCHED_FIFO_BIT|SCHED_DELAY_BIT|SCHED_RAND_BIT ) ) {
	input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_type_str[scheduling_flag], "processor", processor_name.c_str() );
	dom->setSchedulingType( SCHEDULE_FIFO );
    }
    
    processor.push_back( new Processor( dom ) );
}

/*
 * Find the processor and return it.  
 */

Processor *
Processor::find( const std::string& name  )
{
    if ( name.size() == 0 ) return 0;
    vector<Processor *>::const_iterator nextProcessor = find_if( ::processor.begin(), ::processor.end(), eqProcStr( name ) );
    if ( nextProcessor == processor.end() ) {
	return 0;
    } else {
	return *nextProcessor;
    }
}



double Processor::rate() const 
{
    return dynamic_cast<LQIO::DOM::Processor *>(get_dom())->getRateValue();
}


unsigned int Processor::ref_count() const
{
    unsigned int count = 0;
    for ( vector<Task *>::const_iterator t = _tasks.begin(); t != _tasks.end(); ++t ) {
	count += (*t)->multiplicity() * (*t)->n_threads();
    }
    return count;
}


bool Processor::is_single_place_processor() const
{
    return scheduling() == SCHEDULE_RAND
	|| scheduling() == SCHEDULE_DELAY
	|| ref_count() == 1
	|| is_infinite();
}


/* static */ unsigned
Processor::set_queue_length( void )
{
    unsigned max_count = 0;
	
    for ( vector<Processor *>::const_iterator h = ::processor.begin(); h != ::processor.end(); ++h ) {
	Processor * curr_proc = *h;
	unsigned j;
	unsigned n_tasks = 0;

	if ( curr_proc->is_single_place_processor() ) continue;

	/* Make fifo queues to NON infinite processors with multiple callers. */
	
	for ( vector<Task *>::const_iterator t = curr_proc->_tasks.begin(); t != curr_proc->_tasks.end(); ++t ) {
	    Task * curr_task = *t;
	    
	    for ( j = 0; j < n_tasks; ++j ) {
		if ( curr_proc->_history[j].task->priority() > curr_task->priority() ) break;
	    }

	    if ( j < n_tasks ) {
		unsigned k;
		for ( k = n_tasks; k > j; --k ) {
		    curr_proc->_history[k].task = curr_proc->_history[k-1].task;
		}
	    }
	    curr_proc->_history[j].task = curr_task;
	    n_tasks += 1;
	}

	/* Find largest count (cosmetic really) */
				
	for ( unsigned int i = 0; i < n_tasks; i = j ) {
	    int start_prio = curr_proc->_history[i].task->priority();
	    unsigned count = 0;
			
	    for ( j = i; j < n_tasks && curr_proc->_history[j].task->priority() == start_prio; ++j ) {
		count += curr_proc->_history[j].task->multiplicity();
	    }

	    if ( count > max_count ) {
		max_count = count;
	    }

	}

	if ( curr_proc->scheduling_is_ok( SCHED_PPR_BIT|SCHED_HOL_BIT ) ) {
	    max_count += 2;
	}

    }
    return max_count;
}


/*
 * Templates for creating processors.  Each entry and phase is assigned
 * a unique stream so that we can find processor waiting times.
 */

void
Processor::transmorgrify( unsigned max_count )
{
    set_origin( __x_offset, Task::__server_y_offset + 3.0 );
    double x_pos = get_x_pos();
    double y_pos = get_y_pos();

    if ( is_infinite() ) {

	/* Infinite server -- just create as many processors as tasks. */
	if ( this->ref_count() ) {
	    this->PX = create_place( x_pos, y_pos, PROC_LAYER,
				     this->ref_count(), "P%s", name() );
	} else {
	    this->PX = create_place( x_pos, y_pos, PROC_LAYER,
				     open_model_tokens, "P%s", name() );
	}

    } else if ( is_single_place_processor() ) {
		
	this->PX = create_place( x_pos, y_pos, PROC_LAYER,
				 this->multiplicity(), "P%s", name() );

    } else {
	unsigned i;			/* Task index.		*/
	unsigned j;
	unsigned k;			/* An index.		*/
	unsigned prio_count = 0;	/* */

	/* Regular server. */
		
	this->PX = create_place( x_pos, y_pos, PROC_LAYER,
				 this->multiplicity(), "P%s", name() );

	/* Create Queue state places for each priority (allow for instances of tasks) */

	for ( i = 0; i < this->n_tasks(); i = j ) {
	    int start_prio = this->_history[i].task->priority();
	    unsigned count = 0;
	    struct place_object * prio_place = 0;
			
	    /* Create Priority places */

	    if ( scheduling_is_ok( SCHED_PPR_BIT|SCHED_HOL_BIT ) ) {
		prio_place = create_place( x_pos, y_pos + max_count,
					   PROC_LAYER, 1,
					   "Prio%d%s", start_prio, name() );
	    }
						   
	    /* Now create the request and queue places. */

	    for ( j = i; j < n_tasks() && this->_history[j].task->priority() == start_prio; ++j ) {
		count += this->_history[j].task->multiplicity() * this->_history[j].task->n_threads();
	    }

	    this->_proc_queue_count = count;
	    for ( k = 1; k < count; ++k ) {
		(void) create_place( x_pos, y_pos + k, FIFO_LAYER, 1,
				     "P%dSh%s%d", start_prio, name(), k );
	    }

	    x_pos += 0.5;
			
	    for ( k = i; k < j; ++k ) {
		unsigned m;	/* multiplicity index.	*/
		unsigned max_m = this->_history[k].task->n_customers();

		for ( m = 0; m < max_m; ++m ) {
		    x_pos = make_queue( x_pos, y_pos, start_prio,
					prio_place, IMMEDIATE + prio_count,
					this->_history, count, max_count,
					i, k, m );
		}
	    }

	    prio_count += 1;
	}

	this->PX->center.y = IN_TO_PIX( y_pos + max_count );
    }

    __x_offset = x_pos + 1.0;
}


/*
 * Create the request and queue places for the processor.
 */

double
Processor::make_queue( double x_pos, double y_pos, const int priority,
		       struct place_object * prio_place,
		       const short trans_prio, history_t history[], 
		       const unsigned count, unsigned depth,
		       const unsigned low, const unsigned curr, const unsigned m )
{
    Task * cur_task	= history[curr].task;
    unsigned p;

		
    for ( vector<Entry *>::const_iterator e = cur_task->entries.begin(); e != cur_task->entries.end(); ++e ) {
	Entry * cur_entry = *e;
	for ( p = 1; p <= cur_entry->n_phases(); ++p ) {
	    x_pos = make_fifo_queue( x_pos, y_pos, priority, prio_place,
				     trans_prio, history, 
				     count, depth, low, curr, m,
				     &cur_entry->phase[p] );
	}
    }


    for ( vector<Activity *>::const_iterator a = cur_task->activities.begin(); a != cur_task->activities.end(); ++a ) {
	x_pos = make_fifo_queue( x_pos, y_pos, priority, prio_place,
				 trans_prio, history, 
				 count, depth, low, curr, m,
				 (*a) );
    }
	
    return x_pos;
}



/*
 * Make a queue to request service from the processor.
 */

double
Processor::make_fifo_queue( double x_pos, double y_pos, const int priority,
			    struct place_object * prio_place,
			    const short trans_prio, history_t history[], 
			    const unsigned count, unsigned depth,
			    const unsigned low, const unsigned curr, const unsigned m,
			    Phase * curr_phase )
{
    struct trans_object * c_trans;
    struct place_object * c_place;

    unsigned k; 	/* Queue Counter 	*/
    struct place_object * p_place = 0;
		
    if ( curr_phase->s() == 0.0 ) return x_pos;

    for ( unsigned int s = 0; s < curr_phase->n_slices(); ++s ) {
	c_place = create_place( x_pos, y_pos, PROC_LAYER, 0, "Preq%s%s%d%d", name(), curr_phase->name(), m, s );
	c_trans = create_trans( x_pos, y_pos + 0.5, PROC_LAYER, 1.0, 1, trans_prio, "preq%s%s%d%d", name(), curr_phase->name(), m, s );
	create_arc( PROC_LAYER, TO_TRANS, c_trans, c_place );
			
	for ( k = 1; k < count; k++ ) {
	    create_arc( FIFO_LAYER, TO_TRANS, c_trans, no_place( "P%dSh%s%d", priority, name(), k ) );
	    c_place = create_place( x_pos, y_pos + k, PROC_LAYER, 0, "PI%s%s%d%d%d", name(), curr_phase->name(), m, s, k );
				
	    create_arc( PROC_LAYER, TO_PLACE, c_trans, c_place );
				
	    c_trans = create_trans( x_pos, y_pos + (double)k + 0.5, PROC_LAYER, 1.0, 1, trans_prio, "pi%s%s%d%d%d", name(), curr_phase->name(), m, s, k );
	    create_arc( FIFO_LAYER, TO_PLACE, c_trans, no_place( "P%dSh%s%d", priority, name(), k ) );
	    create_arc( PROC_LAYER, TO_TRANS, c_trans, c_place );
	}

	/* If priority scheduling then add stuff here. */

	if ( prio_place ) {
	    create_arc( PROC_LAYER, TO_TRANS, c_trans, prio_place );

	    p_place = create_place( x_pos, y_pos + (double)depth, PROC_LAYER, 0, "PR%s%s%d%d", name(), curr_phase->name(), m, s );
	    create_arc( PROC_LAYER, TO_PLACE, c_trans, p_place );
	    c_trans = create_trans( x_pos, y_pos + (double)depth + 0.5, PROC_LAYER, 1.0, 1, trans_prio, "pr%s%s%d%d", name(), curr_phase->name(), m, s );
	    create_arc( PROC_LAYER, TO_TRANS, c_trans, p_place );
	    history[curr].request_place = p_place;

	    /* HOL queueing */
				
	    for ( unsigned int i = 0; i < low; ++i ) {
		create_arc( PROC_LAYER, TO_PLACE, history[i].grant_trans, prio_place );
		create_arc( PROC_LAYER, TO_TRANS, history[i].grant_trans, prio_place );
	    }

	    depth += 2;
	}
			
	create_arc( PROC_LAYER, TO_TRANS, c_trans, this->PX );
			
	/* This place is connected to the sX transition */
			
	c_place = create_place( x_pos, y_pos + (double)depth, PROC_LAYER, 0, "Pgrt%s%s%d%d", name(), curr_phase->name(), m, s );
	create_arc( PROC_LAYER, TO_PLACE, c_trans, c_place );
	curr_phase->_slice[s].PgX[m] = c_place;
			
	history[curr].grant_trans = c_trans;
	history[curr].grant_place = c_place;

	if ( this->scheduling() == SCHEDULE_PPR ) {
	    unsigned i;

	    /* Steal processor from other PR places here */
				
	    for ( i = 0; i < low; ++i ) {
		c_trans = create_trans( x_pos-0.25*(i+1), y_pos+(double)depth-1.5+0.25*(i+1), PROC_LAYER, 1.0, 1, trans_prio, "prmpt%s%s%d%d%d", name(), curr_phase->name(), m, s, i );
		create_arc( PROC_LAYER, TO_TRANS, c_trans, p_place );
		create_arc( PROC_LAYER, TO_PLACE, c_trans, c_place);
		create_arc( PROC_LAYER, TO_TRANS, c_trans, history[i].grant_place );
		create_arc( PROC_LAYER, TO_PLACE, c_trans, history[i].request_place );
	    }
	}

	x_pos += 0.5;
    } /* s */

    return x_pos;
}

/* -------------------------------------------------------------------- */
/* Ouptut								*/
/* -------------------------------------------------------------------- */

/*
 * Print results related to each processor.
 */

void
Processor::insert_DOM_results ()
{
    /* For all tasks, For all entries, for all phases ... */
    double proc_util = 0;
    for ( vector<Task *>::const_iterator t = _tasks.begin(); t != _tasks.end(); ++t ) {
	double task_util = 0.0;
	const unsigned int mult = (*t)->multiplicity();
	for ( vector<Entry *>::const_iterator e = (*t)->entries.begin(); e != (*t)->entries.end(); ++e ) {
	    if ( !(*e)->is_regular_entry() ) continue;
	    for ( unsigned int p = 1; p <= (*e)->n_phases(); p++ ) {
		for ( unsigned int m = 0; m < mult; ++m ) {
		    task_util += (*e)->phase[p].proc_tokens[m];
		}
		(*e)->phase[p].get_dom()->setResultProcessorWaiting( get_waiting( (*e)->phase[p] ) );
	    }
	}
	for ( vector<Activity *>::const_iterator a = (*t)->activities.begin(); a != (*t)->activities.end(); ++a ) {
	    double util = 0.0;
	    for ( unsigned int m = 0; m < mult; ++m ) {
		util += (*a)->proc_tokens[m];
	    }
	    dynamic_cast<LQIO::DOM::Activity *>((*a)->get_dom())->setResultProcessorWaiting( get_waiting( **a ) )
		.setResultProcessorUtilization( util );
	    task_util += util;
	}

	(*t)->get_dom()->setResultProcessorUtilization(task_util);
	proc_util += task_util;
    }
    get_dom()->setResultUtilization(proc_util);
}



/*
 * Compute the waiting time for the processor for task i, entry e, phase p.
 * Only look at instance zero.
 */

double
Processor::get_waiting( const Phase& phase )
{
    double tokens = 0.0;
    double tput	  = 0.0;

    if ( phase.s() == 0.0 ) {
	return 0.0;
    }

    if ( is_single_place_processor() ) {
	tokens = get_pmmean( "W%s00", phase.name() );
	tput   = get_tput( IMMEDIATE, "w%s00", phase.name() );

    } else if ( scheduling() != SCHEDULE_RAND ) {
	tokens = get_pmmean( "Preq%s%s00", name(), phase.name() );
	for ( unsigned int j = 1; j < _proc_queue_count; j++ ) {
	    tokens += get_pmmean( "PI%s%s00%d", name(), phase.name(), j );
	}
	if ( this->scheduling() != SCHEDULE_FIFO ) {
	    tokens += get_pmmean( "PR%s%s00", name(), phase.name() );
	}
	tput = get_tput( IMMEDIATE, "preq%s%s00", name(), phase.name() );
    }

    if ( debug_flag && tput > 0 ) {
	(void) fprintf( stddbg, "Proc %s entry %s: tokens=%g, tput=%g\n",
			name(), phase.name(), tokens, tput );
    }

    return tput ? (tokens * (phase.mean_processor_calls()) / tput) : 0.0;
}
