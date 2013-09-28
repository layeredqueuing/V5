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

#include <vector>
#include <cmath>
#include <cassert>
#include <lqio/dom_extvar.h>
#include <lqio/dom_phase.h>
#include <lqio/dom_histogram.h>
#include <lqio/glblerr.h>
#include <lqio/error.h>
#include "petrisrvn.h"
#include "processor.h"
#include "errmsg.h"
#include "task.h"
#include "entry.h"
#include "phase.h"
#include "makeobj.h"
#include "results.h"
#include "model.h"

using namespace std;

double Phase::__parameter_x;		/* Offset for parameters.		*/
double Phase::__parameter_y;

slice_info_t::slice_info_t()
{
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	WX_xpos[m] = 0;
	WX_ypos[m] = 0;
	WX[m] = 0;	/* Wait for proc		*/
	ChX[m] = 0;	/* Choose next action.		*/
	PrX[m] = 0;	/* Processor request.		*/
	PgX[m] = 0;	/* Processor grant.		*/
	for ( unsigned int s = 0; s <= MAX_STAGE; ++s ) {
	    SX[m][s] = 0;	/* Service		*/
	}
    }
}


/*
 * Clear phase info.
 */

Phase::Phase( LQIO::DOM::Phase * dom, Task * task )
    : _dom(dom),
      _entry(0),
      _task(task),
      _prob_a(1.0),
      _n_slices(0)
{
    _rpar_s[0]       = 0;
    _rpar_s[1]       = 0;
    for ( unsigned m = 0; m < MAX_MULT; ++m ) {
	XX[m] = 0;
	ZX[m] = 0;		/* Think Time place.		*/
	doneX[m] = 0;		/* Phase is done place.		*/
	task_tokens[m] = 0.0;
	proc_tokens[m] = 0.0;
    }
}


Phase&
Phase::set_dom( LQIO::DOM::Phase * dom, Entry * entry )
{
    _dom = dom;
    _entry = entry;
    _task = entry->task();
    return *this;
}

/* ------------------------------------------------------------------------ */
/* Accessors and Mutators						    */
/* ------------------------------------------------------------------------ */

const char * Phase::name() const
{
    return get_dom()->getName().c_str();
}

unsigned int Phase::entry_id() const
{
    return entry() ? entry()->entry_id() : 0;
}

double Phase::s() const
{
    const LQIO::DOM::ExternalVariable * var = get_dom()->getServiceTime();
    double value;
    if ( var && var->getValue(value) ) {
	return value;
    } else {
	return 0.0;
    }
}


double Phase::think_time() const
{
    const LQIO::DOM::ExternalVariable * var = get_dom()->getThinkTime();
    double value;
    if ( var && var->getValue(value) ) {
	return value;
    } else {
	return 0.0;
    }
}


double Phase::coeff_of_var() const
{
    const LQIO::DOM::ExternalVariable * var = get_dom()->getCoeffOfVariationSquared();
    double value;
    if ( var && var->getValue(value) ) {
	return value;
    } else {
	return 1.0;		/* Default is one (exponential) */
    }
}


const Processor * Phase::processor() const
{
    return task()->processor();
}


LQIO::DOM::Call * Phase::get_call(const Entry* entry) const
{
    std::map<const Entry *,Call>::const_iterator e = _call.find(entry);
    return ( e != _call.end() ) ? e->second._dom : NULL;
}


short Phase::rpar_y(const Entry* entry) const
{
    std::map<const Entry *,Call>::const_iterator e = _call.find(entry);
    return ( e != _call.end() ) ? e->second._rpar_y : 0;
}

double Phase::y(Entry const* entry) const
{
    std::map<const Entry *,Call>::const_iterator e = _call.find(entry);
    return e != _call.end() && e->second.is_rendezvous() ? e->second._dom->getCallMeanValue() : 0.0;
}

double Phase::z(Entry const* entry) const
{
    std::map<const Entry *,Call>::const_iterator e = _call.find(entry);
    return e != _call.end() && e->second.is_send_no_reply() ? e->second._dom->getCallMeanValue() : 0.0;
}

std::vector<double>* Phase::get_histogram( const Entry * entry ) const
{
    std::map<const Entry *,Call>::const_iterator e = _call.find(entry);
    return e != _call.end() ? const_cast<std::vector<double>*>(&(e->second._bin)) : 0;
}

/*
 * Return service rate after adjusting for processor rates.
 */

double
Phase::service_rate() const
{
    if ( s() == 0.0 ) {
	return 1.0;
    } else {
	double sum = 1.0;
	for ( std::map<const Entry *,Call>::const_iterator e = _call.begin(); e != _call.end(); ++e ) {
	    sum += e->second._dom->getCallMeanValue();
	}

	Processor * processor = task()->processor();
	double rate = processor ? processor->rate() : 1.0;
	return sum * rate / s();
    }
}


bool Phase::has_stochastic_calls() const
{
    return get_dom()->getPhaseTypeFlag() == PHASE_STOCHASTIC;
}


bool
Phase::has_deterministic_service() const
{
    return coeff_of_var() == 0.0
	&& s() > 0.0
	&& task()->type() != OPEN_SRC;
}


bool
Phase::is_hyperexponential() const
{
    return coeff_of_var() > 1.0
	&& s() > 0.0
	&& task()->type() != OPEN_SRC;
}


int
Phase::n_stages() const
{
    if ( coeff_of_var() > 1.0 ) return 2;		/* Hyperexponential has 2  */
    else if ( coeff_of_var() == 1.0 ) return 1;		/* Exponential */
    else if ( coeff_of_var() >= 1.0/2.0 ) return 2;	/* Erlang 2 */
    else if ( coeff_of_var() >= 1.0/3.0 ) return 3;	/* Erlang 3 */
    else if ( coeff_of_var() > 0.0 ) return 4;		/* Punt -- Erlang 4 */
    else return 1;					/* Deterministic*/
}

/*------------------------------------------------------------------------*/

Phase&
Phase::add_call( LQIO::DOM::Call * call )
{
    LQIO::DOM::Entry* toDOMEntry = const_cast<LQIO::DOM::Entry*>(call->getDestinationEntry());
    Entry* to_entry = Entry::find(toDOMEntry->getName());
    if (!to_entry) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, toDOMEntry->getName().c_str() );
    } else if ( call->getCallType() == LQIO::DOM::Call::RENDEZVOUS ) {
	if ( to_entry->test_and_set_recv( RENDEZVOUS_REQUEST ) ) {
	    _call[to_entry]._dom = call;		/* Save dom */
	}
    } else if (call->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY
	       || call->getCallType() == LQIO::DOM::Call::QUASI_SEND_NO_REPLY ) {
	if ( to_entry->test_and_set_recv( SEND_NO_REPLY_REQUEST ) ) {
	    _call[to_entry]._dom = call;		/* Save dom */
	}
    }
    return *this;
}


/*
 * Check phase (slice) and update counts.
 */

double
Phase::check()
{
    double ysum = 0;
    double zsum = 0;
    const Processor * curr_proc = task()->processor();
    if ( this->s() > 0.0 && this->coeff_of_var() != 1.0 && curr_proc->scheduling() == SCHEDULE_PPR ) {
	LQIO::solution_error( WRN_PREEMPTIVE_SCHEDULING, curr_proc->name(), this->name() );
//	curr_proc->scheduling = SCHEDULE_FIFO;
    }

    if ( this->has_stochastic_calls() ) {
	this->_n_slices = 1;
	for ( std::map<const Entry *,Call>::const_iterator c = _call.begin(); c != _call.end(); ++c ) {
	    const Call& call = c->second;
	    if ( call.is_rendezvous() ) {
		ysum += call._dom->getCallMeanValue();
	    } else if ( call.is_send_no_reply() ) {
		zsum += call._dom->getCallMeanValue();
	    }
	}
    } else {
	for ( std::map<const Entry *,Call>::const_iterator c = _call.begin(); c != _call.end(); ++c ) {
	    const Call& call = c->second;
	    double value = call._dom->getCallMeanValue();
	    if ( call.is_rendezvous() ) {
		ysum += value;
	    } else if ( call.is_send_no_reply() ) {
		zsum += value;
	    }
	    if ( fmod( value, 1.0 ) > 1e-6 ) {
		solution_error( LQIO::ERR_NON_INTEGRAL_CALLS_FOR, "Entry", entry()->name(), "Phase", this->name(), value, c->first->name() );
	    }
	}
	this->_n_slices = (unsigned)(ysum + zsum + 1.001);
	if ( n_slices() >= DIMSLICE ) {
	    input_error2( LQIO::ERR_TOO_MANY_X, "slices ", DIMSLICE );
	}
    }
    _mean_processor_calls = ysum + 1;
    return ysum + zsum;
}



unsigned int
Phase::compute_offset( const Entry * b ) const
{
    unsigned int off = 0;
    for ( std::map<const Entry *,Call>::const_iterator e = _call.begin(); e != _call.end() && e->first != b; ++e ) {
	off += static_cast<unsigned int>( e->second._dom->getCallMeanValue() );
    }
    return off;
}

/*
 * create a phase.
 */

double
Phase::transmorgrify( const double x_pos, const double y_pos, const unsigned m,
		      const LAYER layer_mask, const double p_pos, const short enabling )
{
    const unsigned ne    = task()->n_entries();
    struct trans_object * c_trans;
    assert( this->n_slices() >= 1 );

    for ( unsigned int s = 0; s < this->n_slices(); ++s ) {
	slice_info_t * curr_slice = &this->_slice[s];
	const double s_pos = p_pos+s*3;
	unsigned int n = this->n_stages();

	curr_slice->WX_xpos[m] = X_OFFSET(1+s,0.25);
	curr_slice->WX_ypos[m] = y_pos;
	curr_slice->WX[m] = move_place_tag( create_place( X_OFFSET(s_pos,  0.25), y_pos, layer_mask, 0, "W%s%d%d", this->name(), m, s ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	curr_slice->SX[m][1] = move_place_tag( create_place( X_OFFSET(s_pos+1,0.25), y_pos, layer_mask, 0, "S%s%d%d", this->name(), m, s ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );

	/* Make places for erland/hyperexponential distributions */

	for ( unsigned int i = 2; i <= n; ++i ) {
	    curr_slice->SX[m][i] = move_place_tag( create_place( X_OFFSET(s_pos+1,0.25), y_pos+(i-1), layer_mask, 0, "S%d%s%d%d", i, this->name(), m, s ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	}
	curr_slice->ChX[m] = move_place_tag( create_place( X_OFFSET(s_pos+2,0.25), y_pos, layer_mask, 0, "Ch%s%d%d", this->name(), m, s ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
    }

    if ( this->think_time() > 0.0 ) {
	this->ZX[m] = create_place( X_OFFSET(p_pos,0.25), y_pos+1.0, layer_mask, 0, "Z%s%d",  this->name(),m );
	c_trans = create_trans( X_OFFSET(p_pos,0.25), y_pos+0.5, layer_mask, 1.0/this->think_time(), INFINITE_SERVER, EXPONENTIAL, "z%s%d",  this->name(),m );
	create_arc( layer_mask, TO_TRANS, c_trans, this->ZX[m] );
	create_arc( layer_mask, TO_PLACE, c_trans, this->_slice[0].WX[m] );
    } else {
	this->ZX[m] = this->_slice[0].WX[m];
    }

    this->done_xpos[m] = X_OFFSET(p_pos+this->n_slices()*3,0);
    this->done_ypos[m] = y_pos-0.5;
    if ( !task()->inservice_flag() || task()->is_client() ) {

	c_trans = create_trans( this->done_xpos[m], this->done_ypos[m], layer_mask, 1.0, 1, IMMEDIATE, "done%s%d", this->name(), m );
	create_arc( layer_mask, TO_TRANS, c_trans, this->_slice[this->n_slices()-1].ChX[m] );
	this->doneX[m] = c_trans;
    }

    for ( unsigned int s = 0; s < this->n_slices(); ++s ) {
	slice_info_t * curr_slice = &this->_slice[s];
	const double s_pos = p_pos+s*3;

	c_trans = create_trans( X_OFFSET(s_pos+1,0), y_pos - 0.5, layer_mask,
				this->_prob_a, 1, IMMEDIATE, "w%s%d%d", this->name(), m, s );
	if ( this->s() > 0.0 ) {
	    if ( task()->type() == OPEN_SRC ) {
		create_arc( layer_mask, INHIBITOR, c_trans, curr_slice->SX[m][1] );
	    } else {
		request_processor( c_trans, m, s );
	    }
	}
	create_arc( layer_mask, TO_TRANS, c_trans, curr_slice->WX[m] );
	create_arc( layer_mask, TO_PLACE, c_trans, curr_slice->SX[m][1] );

	if ( this->is_hyperexponential() )  {
	    c_trans = create_trans( X_OFFSET(s_pos+1,0), y_pos + 0.5, layer_mask, 1.0 - this->_prob_a, 1, IMMEDIATE, "w2%s%d%d", this->name(), m, s );
	    request_processor( c_trans, m, s );
	    create_arc( layer_mask, TO_TRANS, c_trans, curr_slice->WX[m] );
	    create_arc( layer_mask, TO_PLACE, c_trans, curr_slice->SX[m][2] );
	}
	if ( this->s() == 0 ) {
	    c_trans = create_trans( X_OFFSET(s_pos+2,0), y_pos - 0.5, layer_mask,  1.0, 1, IMMEDIATE, "s%s%d%d", this->name(), m, s );
	} else if ( this->has_deterministic_service() ) {
	    c_trans = create_trans( X_OFFSET(s_pos+2,0), y_pos - 0.5, layer_mask, -this->_rpar_s[0], 1, DETERMINISTIC, "s%s%d%d", this->name(), m, s );
	} else if ( task()->type() != OPEN_SRC ) {	/* Infinite server! */
	    c_trans = create_trans( X_OFFSET(s_pos+2,0), y_pos - 0.5, layer_mask, -this->_rpar_s[0], enabling, EXPONENTIAL, "s%s%d%d", this->name(), m, s );
	} else {
	    c_trans = create_trans( X_OFFSET(s_pos+2,0), y_pos - 0.5, layer_mask, -this->_rpar_s[0], 1, EXPONENTIAL, "s%s%d%d", this->name(), m, s );
	}
	if ( this->s() > 0.0 && task()->type() != OPEN_SRC ) {
	    processor_acquired( c_trans, m, s );
	}

	/* Create transitions for Erlang/Exponential */

	create_arc( layer_mask, TO_TRANS, c_trans, curr_slice->SX[m][1] );
	if ( !this->is_hyperexponential() ) {
	    unsigned int n = this->n_stages();
	    for ( unsigned i = 2; i <= n; ++i ) {
		create_arc( layer_mask, TO_PLACE, c_trans, curr_slice->SX[m][i] );
		c_trans = create_trans( X_OFFSET(s_pos+2,0), y_pos + 0.5 + (i-2), layer_mask, -this->_rpar_s[0], enabling, EXPONENTIAL, "s%d%s%d%d", i, this->name(), m, s );
		create_arc( layer_mask, TO_TRANS, c_trans, curr_slice->SX[m][i] );
	    }
	}
	create_arc( layer_mask, TO_PLACE, c_trans, curr_slice->ChX[m] );
	if ( this->s() > 0.0 && task()->type() != OPEN_SRC ) {
	    release_processor( c_trans, m, s );
	}
	if ( this->is_hyperexponential() ) {
	    c_trans = create_trans( X_OFFSET(s_pos+2,0), y_pos + 0.5, layer_mask, -this->_rpar_s[1], enabling, EXPONENTIAL, "s2%s%d%d", this->name(), m, s );
	    create_arc( layer_mask, TO_TRANS, c_trans, curr_slice->SX[m][2] );
	    create_arc( layer_mask, TO_PLACE, c_trans, curr_slice->ChX[m] );
	    release_processor( c_trans, m, s );
	}
    }

    return p_pos + this->n_slices() * 3;
}







/*
 * Request service FROM the processor.
 */

void
Phase::request_processor( struct trans_object * c_trans, const unsigned m, const unsigned s ) const
{
    c_trans->layer |= PROC_LAYER;
    if ( processor()->is_single_place_processor() ) {
	create_arc( PROC_LAYER, TO_TRANS, c_trans, processor()->PX );
    } else {
	create_arc( PROC_LAYER, TO_PLACE, c_trans, no_place( "Preq%s%s%d%d", processor()->name(), this->name(), m, s ) );
    }
}

void
Phase::processor_acquired( struct trans_object * c_trans, const unsigned m, const unsigned s ) const
{
    if ( processor()->is_single_place_processor() ) return;	/* NOP */

    c_trans->layer |= PROC_LAYER;
    create_arc( PROC_LAYER, TO_TRANS, c_trans, this->_slice[s].PgX[m] );
}


/*
 * Release the processor.
 */

void
Phase::release_processor( struct trans_object * c_trans, const unsigned m, const unsigned s ) const
{
    c_trans->layer |= PROC_LAYER;
    create_arc( PROC_LAYER, TO_PLACE, c_trans, processor()->PX );
    if ( !processor()->is_single_place_processor() ) {
#if defined(BUG_111)
	if ( n_slices() == 1 ) {
	} else {
	    create_arc( PROC_LAYER, TO_PLACE, c_trans, this->slice[s].PgX[m] );
	    create_arc( PROC_LAYER, TO_TRANS, this->doneX[m], this->slice[s].PgX[m] );
	    create_arc( PROC_LAYER, TO_PLACE, this->doneX[m], processor()->PX );
	}
#endif
	if ( bit_test( processor()->scheduling(), SCHED_PPR_BIT|SCHED_HOL_BIT ) ) {
	    create_arc( PROC_LAYER, TO_PLACE, c_trans, no_place( "Prio%d%s", task()->priority(), processor()->name() ) );
	}
    }
}


void
Phase::build_forwarding_list()
{
    for ( vector<Entry *>::const_iterator d = ::entry.begin(); d != ::entry.end(); ++d ) {
	if ( y(*d) == 0.0 ) continue;
	if ( has_stochastic_calls() ) {
	    follow_forwarding_path( 0, *d, y(*d) );
	} else {
	    for ( unsigned int s = 0; s < y(*d); ++s ) {
		follow_forwarding_path( s+1, *d, y(*d) );
	    }
	}
    }
}



/*
 * Follow all forwarding paths.
 */

void
Phase::follow_forwarding_path( const unsigned slice_no, Entry * a, double rate )
{
    double sum = 0.0;

    for ( vector<Entry *>::const_iterator b = ::entry.begin(); b != ::entry.end(); ++b ) {
        const double pr_fwd = a->prob_fwd(*b);
	if ( pr_fwd > 0.0 ) {
	    sum += pr_fwd;
	    follow_forwarding_path( slice_no, *b, rate * pr_fwd );
	}
    }
    if ( sum == 0.0 ) return;

    const unsigned max_m = task()->n_customers();
    for ( unsigned int m = 0; m < max_m; ++m ) {
	a->forwards.push_back( new Forwarding( this, slice_no, m, rate ) );
    }
}


/*
 * Create necessary service time parameters.  They are expressed as rates.
 * For hyperexponential service times, we approximate with two branches.
 * See # http://www.cs.duke.edu/~fishhai/misc/queue.pdf
 */

void
Phase::create_spar()
{
    if ( s() == 0 ) return;	/* Ignore phases with zero service times. */

    const double mu = service_rate();
    if ( is_hyperexponential() ) {
	this->_prob_a = (1.0 + sqrt( (coeff_of_var() - 1.0) / (coeff_of_var() + 1.0) )) / 2.0;
	this->_rpar_s[0] = create_rpar( __parameter_x, __parameter_y, SERVICE_RATE_LAYER, mu * (2.0 * this->_prob_a), "mu1%s", name() );
	inc_par_offsets();
	this->_rpar_s[1] = create_rpar( __parameter_x, __parameter_y, SERVICE_RATE_LAYER, mu * (2.0 * (1.0 - this->_prob_a)), "mu2%s", name() );
    } else {
	this->_prob_a = 1.0;
	this->_rpar_s[0] = create_rpar( __parameter_x, __parameter_y, SERVICE_RATE_LAYER, mu * n_stages(), "mu%s", name() );
	this->_rpar_s[1] = 0;
    }
    inc_par_offsets();
}



/*
 * Create necessary rate parameters.
 */

void
Phase::create_ypar( Entry * entry )
{
    std::map<const Entry *,Call>::iterator e = _call.find(entry);
    if ( e != _call.end() ) {
	e->second._rpar_y = create_rpar( __parameter_x, __parameter_y, CALL_RATE_LAYER, e->second._dom->getCallMeanValue(), "r%s%s", name(), entry->name() );
	inc_par_offsets();
    }
}



/*
 * Increment parameter offsets.
 */

void
Phase::inc_par_offsets(void)
{
    __parameter_y += 0.25;
    if ( __parameter_y >= Place::SERVER_Y_OFFSET ) {
	__parameter_x += 3.0;
	__parameter_y = 0.5;
    }
}



void 
Phase::remove_netobj()
{
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
        XX[m] = 0;		/* Service Time result (BUG_622)*/
	ZX[m] = 0;		/* Think Time.			*/
	doneX[m] = 0;		/* Phase is done.		*/
    }
}

void
Phase::insert_DOM_results()
{
    _dom->setResultServiceTime( residence_time() )
	.setResultUtilization( utilization() );

    for ( std::map<const Entry *,Call>::const_iterator c = _call.begin(); c != _call.end(); ++c ) {
	const Call& call = c->second;
	const Entry * entry = c->first;
	if ( call.is_rendezvous() ) {
	    const_cast<Call&>(call)._dom->setResultWaitingTime( queueing_time( entry ) );
	} else if ( call.is_send_no_reply() > 0.0 ) {
	    const_cast<Call&>(call)._dom->setResultWaitingTime( response_time( entry ) );
	    const_cast<Call&>(call)._dom->setResultDropProbability( drop_probability( entry ) );
	}
	LQIO::DOM::Histogram * histogram = const_cast<LQIO::DOM::Histogram *>(call._dom->getHistogram());
	if ( histogram != NULL ) {
	    std::vector<double> * bins = get_histogram( entry );
	    histogram->capacity( bins->size(), 0, bins->size() );

	    unsigned int i = 1;
	    for ( std::vector<double>::const_iterator b = bins->begin(); b != bins->end(); ++b, ++i ) {
		histogram->setBinMeanVariance( i, *b, 0 );
	    }
	}
    }
}

/*
 * Get the mean number of tokens waiting at an entry.
 */

double
Phase::get_utilization( unsigned m  )
{
    double mean_tokens = 0.0;
    unsigned int s;
    const unsigned int n = this->n_stages( );

    assert( 0 < this->n_slices() && this->n_slices() < DIMSLICE );

    for ( s = 0; s < this->n_slices(); ++s ) {
	mean_tokens += get_pmmean( "W%s%d%d", this->name(), m, s );	/* Processor wait.	*/
	mean_tokens += get_pmmean( "S%s%d%d", this->name(), m, s );	/* Entry service.	*/
	for ( unsigned int i = 2; i <= n; ++i ) {
	    mean_tokens += get_pmmean( "S%d%s%d%d", i, this->name(), m, s );	/* Entry service.	*/
	}
    }

    if ( this->think_time() > 0.0 ) {
	mean_tokens += get_pmmean( "Z%s%d", this->name(), m );	/* Entry sleep.	*/
    }

    /* find avg # of tokens in "I%s%s%s" places. */
    /* From that, plus throughput, we can find wait */

    for ( std::map<const Entry *,Call>::const_iterator c = _call.begin(); c != _call.end(); ++c ) {
        const Call& call = c->second;
	if ( call._dom->getCallMeanValue() == 0. ) continue;

	const Entry* entry = c->first;
	if ( call.is_rendezvous() ) {
	    mean_tokens += compute_queueing_delay( const_cast<Call&>(call), m, entry, entry->task()->multiplicity(), this );
	} else if ( call.is_send_no_reply() ) {
	    compute_queueing_delay( const_cast<Call&>(call), m, entry, 1, this );
	    /* Don't save tokens, because we don't block.  Just compute delay */
	}
    }

    if ( debug_flag ) {
	(void) fprintf( stddbg, "toks[%s]=%9.6g\n", this->name(), mean_tokens );
    }

    this->task_tokens[m] = mean_tokens;
    return mean_tokens;
}


double
Phase::utilization() const
{
    double sum = 0.0;
    const unsigned int mult = task()->multiplicity();
    for ( unsigned int m = 0; m < mult; ++m ) {
	sum += task_tokens[m];
    }
    return sum;
}


/*
 * Get the mean number of tokens waiting at an entry.
 */

double
Phase::get_processor_utilization ( unsigned m )
{
    double mean_tokens = 0.0;
    const unsigned int n = n_stages();
    Processor * h = task()->processor();

    if ( !h ) return 0.0;			/* No processor */

    assert( 0 < this->n_slices() && this->n_slices() < DIMSLICE );

    for ( unsigned int s = 0; s < this->n_slices(); ++s ) {
	if ( this->s() == 0.0 ) continue;

	if ( h->is_single_place_processor() ) {
	    mean_tokens += get_pmmean( "S%s%d%d", this->name(), m, s );	/* Entry service.	*/
	    for ( unsigned int i = 2; i <= n; ++i ) {
		mean_tokens += get_pmmean( "S%d%s%d%d", i, this->name(), m, s );	/* Entry service.	*/
	    }
	} else {
	    mean_tokens += get_pmmean( "Pgrt%s%s%d%d", h->name(), this->name(), m, s );
	}
    }

    this->proc_tokens[m] = mean_tokens;
    return mean_tokens;
}



/*
 * Determine rendezvous delay from a to b.  Follow forwarding paths.
 * queueing component is separated from in-service component.
 */

double
Phase::compute_queueing_delay( Call& call, const unsigned m, const Entry * b, const unsigned b_n, Phase * src_phase ) const
{
    const Task * j = b->task();
    unsigned n_s;		/* Number of slices.		*/
    unsigned off = 0;		/* Slice offset 		*/
    const double tput   = lambda( m, b, src_phase );	/* Entry throughput.		*/
    double mean_tokens  = 0.0;
    double queue_tokens = 0.0;	/* Queue_Tokens.		*/

    /* Drop probabiltity */

    std::map<const Entry *,Call>::iterator src = src_phase->_call.find(b);
    if ( src != src_phase->_call.end() && tput > 0 && (call._dom->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY
						       || call._dom->getCallType() == LQIO::DOM::Call::QUASI_SEND_NO_REPLY ) ) {
	src->second._dp = drop_lambda( m, b, src_phase ) / tput;
    }

    if ( this->has_stochastic_calls() ) {	/*+ BUG 47 */
	n_s = 1;
    } else {
	n_s = static_cast<unsigned int>(this->y(b) + this->z(b));
	off = compute_offset( b );
    }						/*- BUG 47 */

    /*
     * Queueing delay component
     */

    call._bin.resize(j->max_queue_length()+1,0);
    for ( unsigned int s = 0; s < n_s; ++s ) {
	if ( b->random_queueing() ) {
	    const double tokens = get_pmmean( "I%s%d%s%s%d", this->name(), s+off, b->name(), src_phase->name(), m );
	    queue_tokens += tokens;
	    call._bin[1] += tokens;
	} else {
	    const unsigned max_k = j->max_queue_length();
	    for ( unsigned k = 1; k <= max_k; ++k ) {
		const double tokens = get_pmmean( "I%s%d%s%s%d%d", this->name(), s+off, b->name(), src_phase->name(), m, k );
		queue_tokens += tokens;
		call._bin[max_k-k+1] += tokens;
	    }
	}
	for ( unsigned n = 0; n < b_n; ++n ) {
	    const double tokens = get_pmmean( "R%s%d%s%d%s%d", this->name(), s+off, b->name(), n, src_phase->name(), m );
	    mean_tokens += tokens;
	    call._bin[0] += tokens;
	}
    }

#if 0
    /*
     * Communications delay component.
     */

    if ( comm_delay_flag && comm_delay[t_des[i].pid][j->pid] > 0.0 ) {
	queue_tokens += get_pmmean( "DLYB%s%s", this->name(), b->name() );
	queue_tokens += get_pmmean( "DLYE%s%s", this->name(), b->name() );
    }
#endif

    if ( src != src_phase->_call.end() ) {
        if ( tput == 0.0 ) {
	    src->second._w = 0.0;
	} else {
	    src->second._w = queue_tokens / tput;
	}
    }

    if ( debug_flag ) {
	(void) fprintf( stddbg, "Pr{I%s->%s*}=%g,\ttput{req%s->%s}=%g\n",
			this->name(), b->name(), queue_tokens,
			this->name(), b->name(), tput );
    }

    /*
     * If we have forwarding, add their components to the royal mess.
     */

    for ( std::map<const Entry *,Call>::const_iterator fwd = b->_fwd.begin(); fwd != b->_fwd.end(); ++fwd ) {
	const LQIO::DOM::Call * call = fwd->second._dom;
	double pr_fwd = call->getCallMeanValue();
	if ( pr_fwd > 0.0 ) {
	    const Entry * d = fwd->first;
	    mean_tokens += b->phase[1].compute_queueing_delay( const_cast<Call&>(fwd->second), 0, d, d->task()->multiplicity(), src_phase );
	}
    }

    return mean_tokens + queue_tokens;		/* tokens queued for entry 'e' */
}


double Phase::queueing_time( const Entry * entry ) const
{
    std::map<const Entry *,Call>::const_iterator e = _call.find(entry);
    return ( e != _call.end() ) ? e->second._w : 0.;
}


double Phase::response_time( const Entry * dst ) const
{
    return queueing_time( dst ) + dst->phase[1].residence_time();
}


double Phase::residence_time() const
{
    if ( entry()->_throughput[0] > 0. ) {
	return task_tokens[0] / entry()->_throughput[0];
    } else {
	return 0.0;
    }
}


double Phase::drop_probability( const Entry * entry ) const
{
    std::map<const Entry *,Call>::const_iterator e = _call.find(entry);
    return ( e != _call.end() ) ? e->second._dp : 0.;
}


/*
 * Return the throughput of entry a, phase p, calling entry b.
 */

double
Phase::lambda( unsigned m, const Entry * b, const Phase * src_phase ) const
{
    double sum = 0.0;
    double calls = this->y(b) + this->z(b);
    if ( calls > 0.0 ) {
	if ( this->has_stochastic_calls() ) {
	    sum = get_tput( IMMEDIATE, "req%s0%s%s%d", this->name(), b->name(), src_phase->name(), m );
	} else {
	    unsigned s;				/*+ BUG 47 */
	    unsigned off = compute_offset( b );
	    for ( s = 0; s < calls; ++s ) {
		sum += get_tput( IMMEDIATE, "req%s%d%s%s%d", this->name(), s+off, b->name(), src_phase->name(), m );
	    }					/*- BUG 47 */
	}
    } else if ( entry()->prob_fwd(b) > 0.0 ) {
	sum = get_tput( IMMEDIATE, "req%s0%s%s%d", this->name(), b->name(), src_phase->name(), m );	/* BUG 64 */
    }
    return sum;
}



double
Phase::drop_lambda( unsigned m, const Entry * b, const Phase * src_phase ) const
{
    if ( this->z(b) > 0.0 ) {
        return get_tput( IMMEDIATE, "drop%s0%s%s%d", this->name(), b->name(), src_phase->name(), m );
    } else {
	return 0.0;
    }
}

bool
Call::is_rendezvous() const
{
    return _dom->getCallType() == LQIO::DOM::Call::RENDEZVOUS || _dom->getCallType() == LQIO::DOM::Call::QUASI_RENDEZVOUS;
}



bool
Call::is_send_no_reply() const
{
    return _dom->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY || _dom->getCallType() == LQIO::DOM::Call::QUASI_SEND_NO_REPLY;
}
