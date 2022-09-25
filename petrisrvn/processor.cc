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

std::vector<Processor *> __processor;

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
    clear();
}


void Processor::clear() 
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


void Processor::create( const std::pair<std::string,LQIO::DOM::Processor*>& p )
{
    const string& processor_name = p.first;
    LQIO::DOM::Processor * dom = p.second;
    if ( processor_name.size() == 0 ) abort();

    if ( dom->getReplicasValue() != 1 ) {
	dom->runtime_error( LQIO::ERR_NOT_SUPPORTED, "replication" );
    }

    if ( dom->getSchedulingType() != SCHEDULE_DELAY && !Pragma::__pragmas->default_processor_scheduling() ) {
	dom->setSchedulingType( Pragma::__pragmas->processor_scheduling() );
    }
	
    Processor * processor = new Processor( dom );
    scheduling_type scheduling = dom->getSchedulingType();
    processor->set_scheduling( scheduling );
    if ( !processor->scheduling_is_ok() ) {
	dom->runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label.at(scheduling).str.c_str() );
	processor->set_scheduling( SCHEDULE_FIFO );
    }
    __processor.push_back( processor );
}


/*
 * Suppress warning for processor scheduling if there is only one
 * thread on this __processor.  Processor::create is executed before
 * Task::create and we need to know if the task has threads or copies.
 */

void
Processor::initialize() 
{
    if ( get_scheduling() == SCHEDULE_PS ) {
	if ( n_tasks() > 1 || _tasks[0]->multiplicity() > 1 || _tasks[0]->n_threads() > 1 ) {
	    get_dom()->runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label.at(get_scheduling()).str.c_str() );
	    const_cast<LQIO::DOM::Entity *>(get_dom())->setSchedulingType( SCHEDULE_FIFO );
	}
    }
}


/*
 * Find the processor and return it.  
 */

Processor *
Processor::find( const std::string& name  )
{
    if ( name.size() == 0 ) return 0;
    vector<Processor *>::const_iterator nextProcessor = find_if( ::__processor.begin(), ::__processor.end(), eqProcStr( name ) );
    if ( nextProcessor == __processor.end() ) {
	return 0;
    } else {
	return *nextProcessor;
    }
}



double Processor::rate() const 
{
    if ( dynamic_cast<const LQIO::DOM::Processor *>(get_dom())->hasRate() ) {
	try {
	    return dynamic_cast<const LQIO::DOM::Processor *>(get_dom())->getRateValue();
	}
	catch ( const std::domain_error& e ) {
	    get_dom()->throw_invalid_parameter( "rate", e.what() );
	}
    }
    return 1.0;
}


unsigned int Processor::ref_count() const
{
    unsigned int count = 0;
    for ( vector<Task *>::const_iterator t = _tasks.begin(); t != _tasks.end(); ++t ) {
	count += (*t)->ref_count() * (*t)->n_threads();
    }
    return count;
}

bool Processor::is_single_place_processor() const
{
    return get_scheduling() == SCHEDULE_RAND
	|| get_scheduling() == SCHEDULE_DELAY
	|| ref_count() == 1
	|| is_infinite();
}

bool Processor::scheduling_is_ok() const
{
    return is_infinite() && get_scheduling() == SCHEDULE_DELAY
	|| multiplicity() == 1 && ( get_scheduling() == SCHEDULE_HOL
				    || get_scheduling() == SCHEDULE_PPR )
	|| get_scheduling() == SCHEDULE_FIFO
	|| get_scheduling() == SCHEDULE_RAND;
}


/* static */ unsigned
Processor::set_queue_length( void )
{
    unsigned max_count = 0;
	
    for ( vector<Processor *>::const_iterator h = ::__processor.begin(); h != ::__processor.end(); ++h ) {
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

	if ( curr_proc->has_priority_scheduling() ) {
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
    const unsigned int copies = multiplicity();		/* Check for validity before is_single_place... */

    if ( is_single_place_processor() ) {
		
        if ( !simplify_network || is_infinite() ) {
	    if ( this->ref_count() ) {
		this->PX = create_place( x_pos, y_pos, PROC_LAYER, ref_count(), "P%s", name() );
	    } else {
		this->PX = create_place( x_pos, y_pos, PROC_LAYER, open_model_tokens, "P%s", name() );
	    }
	}

    } else {
	unsigned i;			/* Task index.		*/
	unsigned j;
	unsigned k;			/* An index.		*/
	unsigned prio_count = 0;	/* */

	/* Regular server. */
		
	if ( copies > MAX_MULT ) {
	    LQIO::input_error2( LQIO::ERR_TOO_MANY_X, "cpus ", MAX_MULT );
	}

	this->PX = create_place( x_pos, y_pos, PROC_LAYER, copies, "P%s", name() );

	/* Create Queue state places for each priority (allow for instances of tasks) */

	for ( i = 0; i < this->n_tasks(); i = j ) {
	    int start_prio = this->_history[i].task->priority();
	    unsigned count = 0;
	    struct place_object * prio_place = 0;
			
	    /* Create Priority places */

	    if ( has_priority_scheduling() ) {
		prio_place = create_place( x_pos, y_pos + max_count,
					   PROC_LAYER, 1,
					   "Prio%d%s", start_prio, name() );
	    }
						   
	    /* Now create the request and queue places. */

	    for ( j = i; j < n_tasks() && this->_history[j].task->priority() == start_prio; ++j ) {
		count += this->_history[j].task->multiplicity() * this->_history[j].task->n_threads();
	    }

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
 * Create the request and queue places for the __processor.
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

		
    cur_task->set_proc_queue_count( count );	/* for get pmmean */
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
 * Make a queue to request service from the __processor.
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

	if ( get_scheduling() == SCHEDULE_PPR ) {
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
Processor::insert_DOM_results() const
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

	const_cast<LQIO::DOM::Entity *>((*t)->get_dom())->setResultProcessorUtilization(task_util);
	proc_util += task_util;
    }
    const_cast<LQIO::DOM::Entity *>(get_dom())->setResultUtilization(proc_util);
#if defined(BUG_393)
    const unsigned int m = multiplicity();
    if ( m > 1 && Pragma::__pragmas->save_marginal_probabilities() ) {
	LQIO::DOM::Entity * dom = const_cast<LQIO::DOM::Entity *>(get_dom());
	dom->setResultMarginalQueueProbabilitiesSize( m + 1 );
	for ( unsigned int i = 0; i <= m; ++i ) {
	    dom->setResultMarginalQueueProbability( m - i, get_prob( i, "P%s", name() ) );	/* Token distribution is backwards */
	}
    }
#endif
}



/*
 * Compute the waiting time for the processor for task i, entry e, phase p.
 * Only look at instance zero.
 */

double
Processor::get_waiting( const Phase& phase ) const
{
    double tokens = 0.0;
    double tput	  = 0.0;

    if ( phase.s() == 0.0 ) {
	return 0.0;
    }

    if ( is_single_place_processor() ) {
        if ( !simplify_network ) { 
	    tokens = get_pmmean( "W%s00", phase.name() );
	    tput   = get_tput( IMMEDIATE, "w%s00", phase.name() );
	}

    } else if ( get_scheduling() != SCHEDULE_RAND ) {
	unsigned int count = phase.task()->get_proc_queue_count();
	tokens = get_pmmean( "Preq%s%s00", name(), phase.name() );
	for ( unsigned int j = 1; j < count; j++ ) {
	    tokens += get_pmmean( "PI%s%s00%d", name(), phase.name(), j );
	}
	if ( has_priority_scheduling() ) {
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
