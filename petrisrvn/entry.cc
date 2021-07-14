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

#include "petrisrvn.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <sstream>
#include <lqio/glblerr.h>
#include <lqio/dom_entry.h>
#include "model.h"
#include "makeobj.h"
#include "errmsg.h"
#include "entry.h"
#include "task.h"
#include "phase.h"
#include "results.h"

using namespace std;

std::vector<Entry *> entry;
unsigned int Entry::__next_entry_id = 1;

Entry::Entry( LQIO::DOM::Entry * dom, Task * task ) 
    : forwards(),
#if defined(BUFFER_BY_ENTRY)
      ZZ(0),
#endif
      _dom(dom),
      _task(task),
      _start_activity(0),
      _entry_id(__next_entry_id++),
      _requests(NOT_CALLED),
      _replies(false),
      _random_queueing(false),
      _rel_prob(0.),
      _n_phases(0),
      _fwd()
{
    unsigned n_phases = dom->getEntryType() == LQIO::DOM::Entry::Type::STANDARD ? dom->getMaximumPhase() : 2;
    for ( unsigned int p = 1; p <= n_phases; ++p ) {
	phase[p].set_dom( dom->getPhase(p), this );
    }
    initialize();
}


void 
Entry::initialize()
{
    for ( unsigned m = 0; m < MAX_MULT; ++m ) {
	DX[m] = 0;
	GdX[m] = 0;
	_throughput[m] = 0;
    }
}

Entry *
Entry::create( LQIO::DOM::Entry * dom, Task * task )
{
    std::vector<Entry *>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( dom->getName() ) );
    if ( nextEntry != entry.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", dom->getName().c_str() );
	return 0;
    } else {
	Entry * ep = new Entry( dom, task );
	::entry.push_back( ep );
	return ep;
    }
}


/*
 * Perform entry checks.
 */

const char * Entry::name() const 
{
    return get_dom()->getName().c_str();
}


Entry&  Entry::set_start_activity( Activity * activity )
{
    _start_activity = activity; 
    return *this; 
}

double Entry::prob_fwd( const Entry * entry ) const
{	
    std::map<const Entry *,Call>::const_iterator e = _fwd.find(entry);
    if ( e != _fwd.end() ) {
	const LQIO::DOM::Call * call = e->second._dom;
	try { 
	    const double value = call->getCallMeanValue();
	    if ( value > 1.0 ) {
		std::stringstream ss;
		ss << value << " > " << 1;
		throw std::domain_error( ss.str() );
	    }
	    return value;
	}
	catch ( const std::domain_error& e ) {
	    const LQIO::DOM::Entry * dst = call->getDestinationEntry();
	    LQIO::solution_error( LQIO::ERR_INVALID_FWDING_PARAMETER, name(), dst->getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }
    return 0.0;
}


double Entry::yy(const Entry* entry) const
{
    double ysum = 0;
    for ( unsigned int p = 1; p <= DIMPH; ++p ) {
	ysum += phase[p].y(entry);
    }  
    return ysum;
}

double Entry::zz(const Entry* entry) const
{
    double zsum = 0;
    for ( unsigned int p = 1; p <= DIMPH; ++p ) {
	zsum += phase[p].z(entry);
    }  
    return zsum;
}

bool
Entry::test_and_set( LQIO::DOM::Entry::Type type )
{
    const bool rc = get_dom()->entryTypeOk( type );
    if ( !rc ) {
	input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, name() );
    }
    return rc;
}

bool
Entry::test_and_set_recv( requesting_type recv ) 
{
    if ( _requests != NOT_CALLED && _requests != recv ) {
	input_error2( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, name() );
	return false;
    } else {
	_requests = recv;
	return true;
    }
}


void 
Entry::add_call( const unsigned int p, LQIO::DOM::Call * call )
{
    /* Make sure this is one of the supported call types */
    if (call->getCallType() != LQIO::DOM::Call::Type::SEND_NO_REPLY && 
	call->getCallType() != LQIO::DOM::Call::Type::RENDEZVOUS ) {
	abort();
    }
	
    if ( !test_and_set( LQIO::DOM::Entry::Type::STANDARD ) ) return;
    phase[p].add_call( call );
}


/* static */ void Entry::add_fwd_call( LQIO::DOM::Call * call ) 
{
    /* Make sure this is one of the supported call types */
    if (call->getCallType() != LQIO::DOM::Call::Type::FORWARD) {
	abort();
    }

    /* Begin by extracting the from/to DOM entries from the call and their names */
    const char* from_entry_name = call->getSourceObject()->getName().c_str();
    const char* to_entry_name = call->getDestinationEntry()->getName().c_str();

    /* Internal Entry references */
    Entry * from_entry;
    Entry * to_entry;
	
    if ( Entry::find( from_entry_name, from_entry, to_entry_name, to_entry ) && to_entry->test_and_set_recv( RENDEZVOUS_REQUEST ) ) {
	from_entry->_fwd[to_entry]._dom = call;		/* Save dom */
    }
}

bool
Entry::check (void)
{
    _n_phases = 1;
    
    /*
     * Determine the maximum number of phases over all entries,
     * and check that all phases have some service time and set
     * the release probability.  The latter is only important when
     * we are forwarding.
     */
	 
    bool has_service_time = false;
    bool has_deterministic_phases = false;
	
    _n_phases = 1;
    if ( is_regular_entry() ) {
	for ( unsigned int p = 1; p <= DIMPH; ++p ) {
	    Phase * curr_phase = &this->phase[p];
	    if ( !curr_phase->get_dom() ) continue;

	    double calls = curr_phase->check();

	    if ( curr_phase->s() > 0.0 || curr_phase->think_time() > 0.0 ) {
		has_service_time = true;
	    }
	    if ( !curr_phase->has_stochastic_calls() ) {
		has_deterministic_phases = true;
	    }
	    if ( calls > 0 && curr_phase->s() == 0.0 ) {
		solution_error( LQIO::WRN_NO_SERVICE_TIME_FOR, "Entry", name(), "Phase", curr_phase->name() );
	    }
	    if ( ( calls > 0 || curr_phase->s() > 0.0 ) && p > n_phases() ) {
		_n_phases = p;
	    }
	}
    } else if ( is_activity_entry() ) {
	std::deque<Activity *> activity_stack;
	std::deque<ActivityList *> fork_stack;
	unsigned max_phase = 1;
	double n_replies;
	    
	has_service_time = start_activity()->find_children( activity_stack, fork_stack, this );
	n_replies = start_activity()->count_replies( activity_stack, this, 1.0, 1, max_phase );
	    
	if ( requests() == RENDEZVOUS_REQUEST ) {
	    if ( n_replies == 0 ) {
		solution_error( LQIO::ERR_REPLY_NOT_GENERATED, name() );
	    } else if ( fabs( n_replies - 1.0 ) > EPSILON ) {
		solution_error( LQIO::ERR_NON_UNITY_REPLIES, n_replies, name() );
	    }
	}
    } else {
	solution_error( LQIO::ERR_ENTRY_NOT_SPECIFIED, name() );
	_n_phases = 1;
    }

    if ( !has_service_time ) {
	if ( task()->type() == Task::Type::REF_TASK && !has_deterministic_phases ) {
	    LQIO::solution_error( ERR_BOGUS_REFERENCE_TASK, name(), task()->name() );
	} else {
	    solution_error( LQIO::WRN_NO_SERVICE_TIME, name() );
	}
    }

    const_cast<Task *>(task())->set_n_phases( n_phases() );

    if ( semaphore_type() != LQIO::DOM::Entry::Semaphore::NONE && task()->type() != Task::Type::SEMAPHORE ) {
	solution_error( LQIO::ERR_NOT_SEMAPHORE_TASK, task()->name(),
			(semaphore_type() == LQIO::DOM::Entry::Semaphore::SIGNAL ? "signal" : "wait"),
			name() );
    }

    /* Deal with forwarding. */
	
    _rel_prob = 1.0;		/* Set entry "release" probability */

    for ( vector<Entry *>::const_iterator d = ::entry.begin(); d != ::entry.end(); ++d ) {
	_rel_prob -= prob_fwd(*d);
    }

    if ( _rel_prob < 0.0 ) {
	if ( _rel_prob > -EPSILON ) {
	    _rel_prob = 0.0;	/* Call it FP truncation.	*/
	} else {
	    solution_error( LQIO::ERR_INVALID_FORWARDING_PROBABILITY, name(), 1.0 - _rel_prob );
	}
    }

    return !LQIO::io_vars.anError();
}



/*
 * Places for task state. (Ph1, Ph2, Waiting).
 */

double
Entry::transmorgrify( double base_x_pos, double base_y_pos, unsigned ix_e, struct place_object * d_place, 
		      unsigned m, short enabling )
{
    unsigned ne = task()->n_entries();
    double x_pos = base_x_pos + ix_e * 0.5;
    double y_pos = base_y_pos;
    double task_y_offset = Y_OFFSET(1.0);
    struct place_object * start_place = 0;
    double next_pos;
    const LAYER layer_mask = ENTRY_LAYER(entry_id())|(m == 0 ? PRIMARY_LAYER : 0);

    if ( task()->inservice_flag() ) {
	task_y_offset += 1.0;
    }

    if ( is_regular_entry() ) {

	/* create phases */

	unsigned p;
	double p_pos = 0.0;
			
	for ( p = 1; p <= n_phases(); ++p ) {
	    p_pos = phase[p].transmorgrify( x_pos, task_y_offset, m, layer_mask, p_pos, enabling );
	}

	/* Connect phases together */
			
	if ( !task()->inservice_flag() || task()->is_client() ) {
	    for ( p = 1; p < n_phases(); ++p ) {
		create_arc( layer_mask, TO_PLACE, phase[p].doneX[m], phase[p+1].ZX[m] );
	    }
	    /*+ BUG_164 */
	    if ( d_place ) {
		if ( task()->type() == Task::Type::SEMAPHORE && semaphore_type() == LQIO::DOM::Entry::Semaphore::WAIT ) {
		    create_arc( layer_mask, TO_PLACE, phase[n_phases()].doneX[m], task()->LX[m] );
		} else {
		    create_arc( layer_mask, TO_PLACE, phase[n_phases()].doneX[m], d_place );
		}
	    }
	    /*- BUG_164 */
	}
	start_place = phase[1].ZX[m];
	next_pos = X_OFFSET(p_pos+1,0.0);

    } else {

	/* Create activities */
	double p_pos;

	p_pos = start_activity()->transmorgrify( x_pos, task_y_offset, m, this, 0, enabling, d_place, true ) + 0.5;
	next_pos = X_OFFSET(p_pos+1,0.0);
	start_place = start_activity()->ZX[m];

    }

    /* Connect the dots. */

    if ( task()->is_client() ) {
	struct trans_object * c_trans;
	if ( task()->think_time() ) {
	    c_trans = create_trans( X_OFFSET(0,0), task_y_offset-0.5, layer_mask,
				    1.0/task()->think_time(), INFINITE_SERVER, EXPONENTIAL, "i%s%d", name(), m );
	} else {
	    c_trans = create_trans( X_OFFSET(0,0), task_y_offset-0.5, layer_mask,
				    1.0, 1, IMMEDIATE, "i%s%d", name(), m );
	}
	create_arc( layer_mask, TO_TRANS, c_trans, task()->TX[m] );
	create_arc( layer_mask, TO_PLACE, c_trans, start_place );
	/* c_trans acquire processor */
#if defined(BUG_622)
	if ( !is_regular_entry() ) {
	    create_arc( MEASUREMENT_LAYER, TO_PLACE, c_trans, phase[1].XX[m] );	/* start phase 1 */
	}
#endif	
			
    } else if ( is_regular_entry() && !task()->inservice_flag()
		&& requests() == RENDEZVOUS_REQUEST ) {

	create_arc( layer_mask, TO_PLACE, phase[1].doneX[m], DX[m] );

    } 

#if defined(BUG_622)
    if ( !is_regular_entry() ) {
	if ( task()->gdX[m] ) {
	    /* activities need flush, so link to flush transition */
	    create_arc( MEASUREMENT_LAYER, TO_TRANS, task()->gdX[m], phase[n_phases()].XX[m] );	/* End phase n */
	}
    }
#endif
    return next_pos;
}



/*
 * Create the forwarding places and transitions for each entry.
 */

void
Entry::create_forwarding_gspn()
{
    double x_off = 0.;
    for ( vector<Forwarding *>::const_iterator f = forwards.begin(); f != forwards.end(); ++f, x_off += 0.5 ) {
	const Phase * root = (*f)->_root;
	double x_pos = task()->get_x_pos() - 0.5 + x_off;
	double y_pos = root->_slice[0].WX_ypos[(*f)->_m];
	unsigned ne  = task()->n_entries();
	const LAYER layer_mask = ENTRY_LAYER(entry_id())|((*f)->_m == 0 ? PRIMARY_LAYER : 0);
	
	(*f)->f_place = create_place( X_OFFSET(1,1.0), y_pos + 1.0, layer_mask, 0,
				      "FWD%s%d%s%d", root->name(), (*f)->_slice_no, phase[1].name(), (*f)->_m );
	if ( this->release_prob() > 0.0 ) {
	    (*f)->f_trans = create_trans( X_OFFSET(1,1.0), y_pos + 0.5, layer_mask,
					  this->release_prob(), 1, IMMEDIATE,
					  "fwd%s%d%s%d", root->name(), (*f)->_slice_no, phase[1].name(), (*f)->_m );
	    create_arc( layer_mask, TO_TRANS, (*f)->f_trans, (*f)->f_place );
	    create_arc( layer_mask, TO_PLACE, (*f)->f_trans, root->_slice[0].WX[(*f)->_m] );
	}
    }
}


void 
Entry::remove_netobj()
{
    for ( unsigned int p = 1; p <= n_phases(); ++p ) {
        phase[p].remove_netobj();
    }
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
        DX[m] = 0;		/* done				*/
	GdX[m] = 0;		/* Guard (for joins).		*/
    }
#if defined(BUFFER_BY_ENTRY)
    ZZ = 0;			/* For open requests.		*/
#endif
}

/*
 * Compute the task utilization
 */

double
Entry::task_utilization ( unsigned p )
{
    unsigned max_m = task()->n_customers();
    if ( p <= n_phases() ) {
	double util = 0;
	for ( unsigned m = 0; m < max_m; ++m ) {
	    if ( task()->task_tokens[m] ) {
		util += task()->utilization(m) * phase[p].task_tokens[m] / task()->task_tokens[m];
	    }
	}
	return util;
    } else {
	return 0.0;
    }
}




/*
 * Check results for open arrivals.  Throughput at entry should match
 * open arrival rate.
 */

bool
Entry::check_open_result()
{
    Model::__open_class_error = false;
#if defined(BUFFER_BY_ENTRY)
    if ( openArrivalRate() > 0.0 && fabs ( openArrivalRate() - throughput[1] ) / openArrivalRate() > 0.05 ) {
	LQIO::solution_error( ADV_OPEN_ARRIVALS_DONT_MATCH, throughput[1], openArrivalRate(), name() );
	Model::__open_class_error = true;
    } else if ( requests() == SEND_NO_REPLY_REQUEST && get_prob( 0, "ZZ%s", name() ) > EPSILON ) {
	LQIO::solution_error( ADV_MESSAGES_LOST, name, get_prob( 0, "ZZ%s", name() ) );
	Model::__open_class_error = true;
    }
#else
    if ( openArrivalRate() > 0.0 && fabs ( openArrivalRate() - task()->multiplicity() * _throughput[0] ) / openArrivalRate() > 0.02 ) {
	LQIO::solution_error( ADV_OPEN_ARRIVALS_DONT_MATCH, task()->multiplicity() * _throughput[0], openArrivalRate(), name() );
	Model::__open_class_error = true;
    } else if ( requests() == SEND_NO_REPLY_REQUEST && get_prob( 0, "ZZ%s", task()->name()) > EPSILON ) {
	LQIO::solution_error( ADV_MESSAGES_LOST, task()->name(), get_prob( 0, "ZZ%s", task()->name() ) );
	Model::__open_class_error = true;
    }

#endif
    return !Model::__open_class_error;
}

void 
Entry::insert_DOM_results()
{
    double totalPhaseUtil = 0.0;
    double proc_tokens    = 0.0;
    double tput           = 0.0;
    double phase_utils[DIMPH+1];
    const unsigned max_m = task()->n_customers();

    /* Write the results into the DOM */

    for ( unsigned m = 0; m < max_m; ++m ) {
	tput += _throughput[m];
    }
    _dom->setResultThroughput(tput);
	
    for ( unsigned p = 1; p <= n_phases(); ++p ) {
	const double util = task_utilization( p );
	totalPhaseUtil   += util;
	phase_utils[p-1] += util;
	for ( unsigned m = 0; m < max_m; ++m ) {
	    proc_tokens  += phase[p].proc_tokens[0];
	}
    }		
	
    /* Store the utilization and squared coeff of variation */
    _dom->setResultUtilization(totalPhaseUtil)
	.setResultProcessorUtilization(proc_tokens);

	
    /* Store activity phase data */
    for ( unsigned p = 1; p <= n_phases(); ++p ) {
	if (is_activity_entry()) {	
	    _dom->setResultPhasePServiceTime(p,phase[p].residence_time())
		.setResultPhasePUtilization(p,phase_utils[p-1]);
//		.setResultPhasePProcessorWaiting(p,queueingTime(p));
	} else {
	    phase[p].insert_DOM_results();
	}
    }

    /* Store forwarding data */
    for ( std::map<const Entry *,Call>::iterator f = _fwd.begin(); f != _fwd.end(); ++f ) {
	Call& call = f->second;
	const Entry * entry = f->first;
	call._dom->setResultWaitingTime( queueing_time( entry ) );
    }
}


double Entry::queueing_time( const Entry * entry ) const
{
    std::map<const Entry *,Call>::const_iterator e = _fwd.find(entry);
    return ( e != _fwd.end() ) ? e->second._w : 0.;
}

/*
 * Find the entry and return it.  
 */

/* static */ Entry * Entry::find( const std::string& name)
{
    std::vector<Entry *>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( name ) );
    if ( nextEntry == entry.end() ) {
	return 0;
    } else {
	return *nextEntry;
    }
}


/*
 * Locate both entries.  return false on error.
 */

bool
Entry::find( const std::string& from_entry_name, Entry * & from_entry, const std::string& to_entry_name, Entry * & to_entry )
{
    bool rc    = true;
    from_entry = find( from_entry_name );
    to_entry   = find( to_entry_name );

    if ( !to_entry ) {
	rc = false;
    }

    if ( !from_entry ) {
	rc = false;
    } else if ( from_entry == to_entry ) {
	input_error2( LQIO::ERR_SRC_EQUALS_DST, to_entry_name.c_str(), from_entry_name.c_str() );
	rc = false;
    }
    return rc;
}
