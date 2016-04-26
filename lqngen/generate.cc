/* lqngen.cc    -- Greg Franks Thu Jul 29 2004
 * Model file generator.
 *
 * $Id: generate.cc 12547 2016-04-05 18:32:45Z greg $
 */

#include "lqngen.h"
#include "generate.h"
#include <cstdlib>
#include <assert.h>
#include <cstring>
#include <algorithm>
#include <lqio/input.h>
#include <lqio/dom_task.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_extvar.h>
#include <lqio/dom_document.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
#include <lqx/SyntaxTree.h>
#include <../lqiolib/src/srvn_gram.h>		/* Derived file */
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include "randomvar.h"

unsigned Generate::__iteration_limit    = 50;
unsigned Generate::__print_interval     = 10;
double Generate::__convergence_value    = 0.00001;
double Generate::__underrelaxation      = 0.9;
std::string Generate::__comment 	= "";

static const unsigned int REF_LAYER = 0;
static const unsigned int SERVER_LAYER = 1;

unsigned int Generate::__number_of_clients	= 1;
unsigned int Generate::__number_of_processors	= 1;
unsigned int Generate::__number_of_tasks	= 1;
unsigned int Generate::__number_of_layers	= 1;
double Generate::__outgoing_requests		= 1;
Generate::layering_t Generate::__task_layering	    = Generate::RANDOM_LAYERING;
Generate::layering_t Generate::__processor_layering = Generate::RANDOM_LAYERING;

RV::RandomVariable * Generate::__service_time			= 0;
RV::RandomVariable * Generate::__think_time			= 0;
RV::RandomVariable * Generate::__forwarding_probability		= 0;
RV::RandomVariable * Generate::__rendezvous_rate		= 0;
RV::RandomVariable * Generate::__send_no_reply_rate		= 0;
RV::RandomVariable * Generate::__customers_per_client		= 0;
RV::RandomVariable * Generate::__task_multiplicity		= 0;
RV::RandomVariable * Generate::__processor_multiplicity		= 0;
RV::RandomVariable * Generate::__number_of_entries		= 0;
RV::Probability      Generate::__probability_second_phase	= 0.;
RV::Probability      Generate::__probability_infinite_server	= 0.;

std::vector<std::string> Generate::__random_variables;

static struct document_observation {
    int flag;
    int key;
    const char * lqx_heading;
    const char * lqx_attribute;
    const char * spex_variable;
} document_observation[] = {
    { Flags::ITERATIONS,   KEY_ITERATIONS,   "iters",   "iterations",      "$i" },
    { Flags::MVA_WAITS,    KEY_WAITING,      "waits",   "waits"            "$w" },
    { Flags::ELAPSED_TIME, KEY_ELAPSED_TIME, "time",    "elapsed_time",    "$time" },
    { Flags::USER_TIME,    KEY_USER_TIME,    "user",    "user_cpu_time",   "$usr" },
    { Flags::SYSTEM_TIME,  KEY_SYSTEM_TIME,  "system",  "system_cpu_time", "$sys" },
};

/*
 * Initialize probabilities.
 */

Generate::Generate( LQIO::DOM::Document * document, const unsigned runs ) 
    : _document(document), _runs(runs), _number_of_layers(0)
{
}


Generate::Generate( const unsigned layers, const unsigned runs ) 
    : _document(0), _runs(runs), _number_of_layers(layers)
{
    _document = new LQIO::DOM::Document( &io_vars, LQIO::DOM::Document::AUTOMATIC_INPUT );		// For XML output.
    _number_of_tasks.resize( _number_of_layers + 1 );
    _task.resize( _number_of_layers + 1 );
    if ( Flags::lqx_spex_output ) {
	LQIO::DOM::Spex::initialize_control_parameters();
    }
}


Generate::~Generate()
{
    delete _document;
    _document = 0;
}

Generate&
Generate::operator()()
{
    generate();
    reparameterize();
    return *this;
}

/*
 * Generate a model (DOM).  All parameters are initialized to 1.
 */

Generate&
Generate::generate()
{
    if ( _processor.size() > 0 ) return *this;

    populateLayers();

    std::vector<unsigned int> stride(  _number_of_layers + 1 );
    const unsigned int width = static_cast<unsigned int>( ceil( log10( __number_of_tasks ) ) );

    /* Create all clients, and give them their own processor */

    for ( unsigned int i = 0; i < __number_of_clients; ++i ) {
	std::ostringstream name;
	name << ( Flags::long_names ? "Client" : "c" ) << setw( width ) << setfill( '0' ) << i;
	LQIO::DOM::Processor * proc = addProcessor( name.str(), SCHEDULE_DELAY );		// Hide this one.
	vector<LQIO::DOM::Entry *> entries;
	LQIO::DOM::Entry * entry = addEntry( name.str(), RV::Probability(0.0) );
	_entry.push_back( entry );
	entries.push_back( entry );
	_task[REF_LAYER].push_back( addTask( name.str(), SCHEDULE_CUSTOMER, entries, proc ) );	// Get RV for number of customers.
    }
    stride[REF_LAYER] = _entry.size();

    /* Create all processors */

    for ( unsigned int i = 0; i < __number_of_processors; ++i ) {
	std::ostringstream name;
	name << ( Flags::long_names ? "Processor" : "p" ) << setw( width ) << setfill( '0' ) << i;
	LQIO::DOM::Processor * proc = addProcessor( name.str(), SCHEDULE_PS );
	_processor.push_back( proc );
    }

    /* Create and distribute all tasks,  connect to processors */

    const unsigned int n = _number_of_tasks.size();
    const double tasks_per_processor = static_cast<double>(n) / static_cast<double>(__number_of_processors);
    unsigned int k = 0;
    const bool extra_entries = !dynamic_cast<RV::Constant *>(__number_of_entries) || (*__number_of_entries)() > 1;
    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
	for ( unsigned int j = 0; j < _number_of_tasks[i]; ++j, ++k ) {
	    std::ostringstream entry_name;
	    entry_name << ( Flags::long_names ? "Entry" : "e" ) << setw( width ) << setfill( '0' ) << k;
	    if ( extra_entries ) {
		entry_name << "_0";
	    }

	    vector<LQIO::DOM::Entry *> entries;		/* List attached to task. */
	    LQIO::DOM::Entry * entry = addEntry( entry_name.str(), __probability_second_phase );
	    _entry.push_back( entry );
	    entries.push_back( entry );

	    /* Add extra entries */

	    if ( extra_entries ) {
		const unsigned n_entries = (*__number_of_entries)();
		for ( unsigned int e = 1; e < n_entries; ++e ) {
		    std::ostringstream extra_name;
		    extra_name << ( Flags::long_names ? "Entry" : "e" ) << setw( width ) << setfill( '0' ) << k
			       << "_" << e;
		    entry = addEntry( extra_name.str(), __probability_second_phase );
		    _entry.push_back( entry );
		    entries.push_back( entry );
		}
	    }
	    
	    /* Add the processor -- */
	    
	    unsigned int p;
	    if ( Generate::__processor_layering == BREADTH_FIRST_LAYERING ) {
		p = static_cast<int>(static_cast<double>(k) / tasks_per_processor);
	    } else if ( Generate::__processor_layering == DEPTH_FIRST_LAYERING ) {
		p = k % __number_of_processors;		/* top down.. */
	    } else if ( k <  __number_of_processors ) {
		p = k;				/* Assign first set of processors deterministically */
	    } else {
		p = static_cast<unsigned int>(floor( drand48() * __number_of_processors) );
	    }

	    std::ostringstream task_name;
	    task_name  << ( Flags::long_names ? "Task"  : "t" ) << setw( width ) << setfill( '0' ) << k;
	    _task[i].push_back( addTask( task_name.str(), SCHEDULE_FIFO, entries, _processor[p] ) );
	}
	stride[i] = _entry.size();
    }

    /* 
     * Connect all servers to parents.  The first entry of each server
     * is connected to a task in layer-1, all other entries can go to
     * any other entry above this layer.
     */

    for ( unsigned int j = SERVER_LAYER; j <= _number_of_layers; ++j ) {
	unsigned int i = 0;
	const unsigned int number_of_clients = _task[j-1].size();
	for ( std::vector<LQIO::DOM::Task *>::const_iterator tp = _task[j].begin(); tp != _task[j].end(); ++tp ) {
	    const vector<LQIO::DOM::Entry *>& entries = (*tp)->getEntryList();
	    for ( std::vector<LQIO::DOM::Entry *>::const_iterator ep = entries.begin(); ep != entries.end(); ++ep ) {
		LQIO::DOM::Entry * dst = *ep;
		if ( ep == entries.begin() ) {
		    const unsigned int k = __task_layering == RANDOM_LAYERING 
			? static_cast<unsigned int>(number_of_clients * drand48())	/* Picks any immediate client task.	*/
			: i;								/* Pick the immediate client. 		*/
		    LQIO::DOM::Entry * src = _task[j-1][k]->getEntryList().front();
		    LQIO::DOM::Call * call = addCall( src, dst, __probability_second_phase );
		    _call.push_back( call );
		    i = (i + 1) % number_of_clients;		// too few clients handled here.
		} else {
		    /* Choose randomly. */
		    const unsigned int e = static_cast<unsigned int>(stride[j-1] * drand48());	/* Picks a higher layer task. 	*/
		    LQIO::DOM::Entry * src = _entry.at(e);
		    for ( unsigned int retry = 0; retry < 3; ++retry ) {
			LQIO::DOM::Call * call = addCall( src, dst, __probability_second_phase );
			if ( call ) {
			    _call.push_back( call );
			    break;
			}
		    }
		}
	    }
	}
    }

    /* Add any extra calls here -  choose randomly, but don't add duplicate call */

    if ( Generate::__outgoing_requests > 1 ) {
	const unsigned int extra_calls = _call.size() * Generate::__outgoing_requests - _call.size();
	const unsigned n_entries = _entry.size();
	for ( unsigned int i = 0; i < extra_calls; ++i ) {
	    const unsigned int server_entry = static_cast<unsigned int>((n_entries - stride[REF_LAYER]) * drand48()) + stride[REF_LAYER];
	    /* Find layer with server entry */
	    unsigned int server_layer = SERVER_LAYER;
	    while ( server_layer < _number_of_layers && stride[server_layer+1] < server_entry ) {
		server_layer += 1;
	    }
	    const unsigned int client_entry = static_cast<unsigned int>(stride[server_layer-1] * drand48());
	    /* Find layer with client entry */
	    unsigned int client_layer = REF_LAYER;
	    while ( client_layer < _number_of_layers && stride[client_layer+1] < client_entry ) {
		client_layer += 1;
	    }
	    assert( client_entry < server_entry && client_layer < server_layer );
	    LQIO::DOM::Entry * src = _entry.at(client_entry);
	    LQIO::DOM::Entry * dst = _entry.at(server_entry);

	    /* Add the call */
	    
	    for ( unsigned int retry = 0; retry < 3; ++retry ) {
		LQIO::DOM::Call * call = addCall( src, dst, client_layer > REF_LAYER ? __probability_second_phase : 0 );
		if ( call ) {
		    _call.push_back( call );
		    break;
		}
	    }
	}
    }
    

    /* Connect any unconnected clients to any server. */

    const unsigned int n_clients = _task[REF_LAYER].size();
    for ( unsigned int i = 0; i < n_clients; ++i ) {
	LQIO::DOM::Entry * src = _task[REF_LAYER][i]->getEntryList().front();
	LQIO::DOM::Phase * phase = src->getPhase(1);
	if ( phase->getCalls().size() == 0 ) {
	    const unsigned int n_entries = _entry.size() - n_clients;
	    const unsigned int e = static_cast<unsigned int>(static_cast<double>(n_entries) * drand48() ) + n_clients;	/* Skip ref task */
	    LQIO::DOM::Call * call = addCall( src, _entry.at(e), RV::Probability(0.) );		/* Ref tasks don't have second phase */
	    _call.push_back( call );
	}
    }

    /* ---- */

    _document->setModelParameters( __comment.c_str(), new LQIO::DOM::ConstantExternalVariable( __convergence_value ), 
				   new LQIO::DOM::ConstantExternalVariable( __iteration_limit ), 
				   new LQIO::DOM::ConstantExternalVariable( __print_interval ), 
				   new LQIO::DOM::ConstantExternalVariable( __underrelaxation ), 0 );
    return *this;
}


/* 
 * Determine how many tasks should go into each layer.  Generally, to
 * ensure that all layers are filled, population goes row by row and
 * adds columns as needed.
 */

void
Generate::populateLayers() 
{
    const unsigned int n = _number_of_tasks.size();
    unsigned int count = 0;
    
    switch ( __task_layering ) {
    case RANDOM_LAYERING:
	for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
	    _number_of_tasks[i] += 1;		/* Guarantee one task per layer */
	    count += 1;
	}
	for ( ; count < __number_of_tasks; ++count ) {
	    unsigned int i = (drand48() * __number_of_layers) + 1;
	    _number_of_tasks[i] += 1;		/* Now stick the rest in anywhere */
	}
	break;
	
    case DETERMINISTIC_LAYERING:
    case UNIFORM_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		_number_of_tasks[i] += 1;
		count += 1;
		if ( count >= __number_of_tasks ) goto done;
	    }
	}
	break;

    case PYRAMID_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		for ( unsigned int j = i; j < n; ++j ) {
		    _number_of_tasks[j] += 1;
		    count += 1;
		    if ( count >= __number_of_tasks ) goto done;
		}
	    }
	}
	break;

    case FUNNEL_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		for ( unsigned int j = SERVER_LAYER; j <= n - i; ++j ) {
		    _number_of_tasks[j] += 1;
		    count += 1;
		    if ( count >= __number_of_tasks ) goto done;
		}
	    }
	}
	break;

    case HOUR_GLASS_LAYERING:
	for ( unsigned mid_point = (n+1)/2;; ) {
	    for ( unsigned int i = SERVER_LAYER; i <= mid_point; ++i ) {
		for ( unsigned int j = SERVER_LAYER; j <= mid_point - i; ++j ) {
		    _number_of_tasks[j] += 1;
		    count += 1;
		    if ( count >= __number_of_tasks ) goto done;
		    _number_of_tasks[n - j] += 1;
		    count += 1;
		    if ( count >= __number_of_tasks ) goto done;
		}
	    }
	    /* Odd number of layers. */
	    const unsigned int j = n/2+1;
	    if ( j != mid_point ) {
		_number_of_tasks[mid_point] += 1;
		count += 1;
		if ( count >= __number_of_tasks ) goto done;
	    }
	}
	break;

    case FAT_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		for ( unsigned int j = i; j <= n - i; ++j ) {
		    _number_of_tasks[j] += 1;
		    count += 1;
		    if ( count >= __number_of_tasks ) goto done;
		}
	    }
	}
	
    default:
	abort();
    }

done: ;
}


/*
 * Add a processor to layer.
 */

LQIO::DOM::Processor * 
Generate::addProcessor( const string& name, const scheduling_type sched_flag )
{
    LQIO::DOM::Processor * processor = new LQIO::DOM::Processor( _document, name.c_str(), sched_flag );
    processor->setRateValue( 1.0 );
    processor->setCopiesValue( 1 );
    if ( sched_flag == SCHEDULE_PS ) {
	processor->setQuantumValue( 0.1 );
    }
    _document->addProcessorEntity( processor );    /* Map into the document */

    return processor;
}


/*
 * Add a task to a layer.
 */

LQIO::DOM::Task *
Generate::addTask( const string& name, const scheduling_type sched_flag, const vector<LQIO::DOM::Entry *>& entries, LQIO::DOM::Processor * processor )
{
    LQIO::DOM::Task * task = new LQIO::DOM::Task( _document, name.c_str(), sched_flag, entries, processor );
    _document->addTaskEntity(task);
    processor->addTask(task);
    task->setCopiesValue( 1 );
    return task;
}


/*
 * Add an entry to a layer.
 */

LQIO::DOM::Entry *
Generate::addEntry( const string& name, const RV::Probability& pr_2nd_phase )
{
    LQIO::DOM::Entry* entry = new LQIO::DOM::Entry( _document, name.c_str() );
    _document->addEntry(entry);
    _document->db_check_set_entry( entry, name, LQIO::DOM::Entry::ENTRY_STANDARD );
    unsigned int n_phases = 1;
    if ( pr_2nd_phase() != 0.0 ) {
	n_phases = 2;
    }
    for ( unsigned p = 1; p <= n_phases; ++p ) {
	LQIO::DOM::Phase* phase = entry->getPhase(p);
	string phase_name=name;
	phase_name += '_';
	phase_name += "_123"[p];
	phase->setName( phase_name );
	phase->setServiceTimeValue( 1.0 );
    }

    return entry;
}


/*
 * Create a synch call from phase 1 of src to dst.
 */

LQIO::DOM::Call *
Generate::addCall( LQIO::DOM::Entry * src, LQIO::DOM::Entry * dst, const RV::Probability& pr_2nd_phase )
{
    const unsigned n_phases = src->getMaximumPhase();
    LQIO::DOM::Phase * phase = src->getPhase( ( n_phases > 1 && pr_2nd_phase() != 0.0 ) ? 2 : 1 );

    string name = phase->getName();
    name += '_';
    name += dst->getName();

    LQIO::DOM::Call * call = new LQIO::DOM::Call( _document, LQIO::DOM::Call::RENDEZVOUS, phase, dst );
    call->setCallMean( new LQIO::DOM::ConstantExternalVariable( 1.0 ) );
    call->setName( name );
    phase->addCall( call );

    return call;
}


/*
 * Translate all constants to variables, then output control program.  This works on the DOM.
 */

void
Generate::reparameterize()
{
    switch ( Flags::output_format ) {
    case FORMAT_SRVN:
	addSpex( &Generate::makeVariables, getNumberOfRuns() > 1 ? &ModelVariable::spex_random : &ModelVariable::spex_scalar );
	break;

    case FORMAT_XML:
	if ( Flags::sensitivity > 0.0 ) {
	    addSensitivityLQX( &Generate::makeVariables, &ModelVariable::lqx_sensitivity );
	} else {
	    addLQX( &Generate::makeVariables, getNumberOfRuns() > 1 ? &ModelVariable::lqx_random : &ModelVariable::lqx_scalar );
	}
	break;
    }

    if ( Flags::verbose ) {
	printStatistics( cerr );
    }
}


/* 
 * MakeXxxVariable is called to convert a constant into a variable in the DOM.  The mean
 * of the variable is set to the orginal value in the DOM.
 */

void
Generate::makeVariables( const ModelVariable::variableValueFunc f )
{
    if ( Flags::sensitivity == 0.0 ) {
	const std::map<std::string,LQIO::DOM::Processor*>& processors = _document->getProcessors();
	for_each( processors.begin(), processors.end(), ProcessorVariable( *this, f ) );

	const std::map<std::string,LQIO::DOM::Task*>& tasks = _document->getTasks();
	for_each( tasks.begin(), tasks.end(), TaskVariable( *this, f ) );
    }

    const std::map<std::string,LQIO::DOM::Entry*>& entries = _document->getEntries();
    for_each( entries.begin(), entries.end(), EntryVariable( *this, f ) );
}

/* ------------------- Logic for creating an LQX program ------------------ */

void
Generate::addLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    __random_variables.clear();				/* Result result variables */
    (this->*f)( g );					/* Make and initialize Variables */

    const unsigned n = getNumberOfRuns();
    if ( n > 1 ) {
	_program << print_header( getDOM(), 1, "i" ) << endl;
	_program << indent( 1 ) << "for ( i = 0; i < " << n << "; i = i + 1 ) {" << endl;
	(this->*f)( &ModelVariable::lqx_loop_body );	/* Get and set loop variables */
	_program << indent( 2 ) << "solve();" << endl;
	_program << print_results( getDOM(), 2, "i" ) << endl;
	_program << indent( 1 ) << "}" << endl;
    } else {
	_program << print_header( getDOM(), 1 ) << endl;
	_program << indent( 1 ) << "solve();" << endl;
	_program << print_results( getDOM(), 1 ) << endl;
    }

    _document->setLQXProgramText( _program.str() );
}


void
Generate::addSensitivityLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    (this->*f)( g );		/* Should be the vector generator */

    _program << print_header( getDOM(), 1 ) << endl;
    const std::map<std::string,LQIO::DOM::Entry*>& entries = _document->getEntries();	
    forEach( entries.begin(), entries.end(), 1 );
    _document->setLQXProgramText( _program.str() );
}


/* static */ ostream&
Generate::printHeader( ostream& output, const LQIO::DOM::Document& document, const int i, const string& prefix )
{
    if ( !Flags::has_any_observation() ) return output;
    
    output << indent(i) << "println_spaced( \", ";
    if ( prefix.size() ) {
	output << "\", \"" << prefix;		/* The iteration number */
    }
    if ( Flags::observe[Flags::PARAMETERS] ) {
	for_each( __random_variables.begin(), __random_variables.end(), ParameterHeading( output, i + 2 ) );
    }

    for_each( &document_observation[0], &document_observation[5], DocumentHeading( output, i + 2 ) );

    /* Make the result variables */
    const map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    for_each( entities.begin(), entities.end(), EntityHeading( output, i + 2 ) );

    /* Entry stats here */
    const std::map<std::string,LQIO::DOM::Entry*>& entries = document.getEntries();
    for_each( entries.begin(), entries.end(), EntryHeading( output, i + 2 ) );

    output << "\" );";
    return output;
}


/* static */ ostream&
Generate::printResults( ostream& output, const LQIO::DOM::Document& document, const int i, const string& prefix )
{
    if ( !Flags::has_any_observation() ) return output;
    
    output << indent(i) << "println_spaced( \", \"";
    if ( prefix.size() ) {
	output  << ", " << prefix;
    }

    if ( Flags::observe[Flags::PARAMETERS] ) {
        for_each( __random_variables.begin(), __random_variables.end(), ParameterResult( output, i + 2 ) );
    }

    for_each( &document_observation[0], &document_observation[5], DocumentResult( output, i + 2 ) );

    /* Make the result variables */
    const map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    for_each( entities.begin(), entities.end(), EntityResult( output, i + 2 ) );

    /* Entry stats here */
    const std::map<std::string,LQIO::DOM::Entry*>& entries = document.getEntries();
    for_each( entries.begin(), entries.end(), EntryResult( output, i + 2 ) );

    output << ");";
    return output;
}

/*
 * Recursive function to iterate over arrays (for sensitivity analysis over service times).
 */

void
Generate::forEach( std::map<std::string,LQIO::DOM::Entry*>::const_iterator e, const std::map<std::string,LQIO::DOM::Entry*>::const_iterator& end, const unsigned int i )
{
    if ( e == end ) {
	_program << indent( i ) << "solve();" << endl;
	_program << print_results( getDOM(), i ) << endl;
    } else {
	const LQIO::DOM::Entry * entry = e->second;
	if ( isReferenceTask( entry->getTask() ) ) {
	    forEach( ++e, end, i );			/* Ignore reference tasks */
	} else {
	    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
	    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
		LQIO::DOM::Phase* phase = p->second;
		const std::string name = std::string( "$s_" ) + phase->getName();
		_program << indent( i ) << "foreach( " << name << " in " << &(name.c_str())[1] << " ) {" << endl;
		forEach( ++e, end, i+1 );
		_program << indent( i ) << "}" << endl;
	    }
	}
    }
}

/* ------------------------------------------------------------------------ */

void
Generate::ModelVariable::lqx_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) return;		// Ignore.

    _model._program << indent( 1 ) << var.getName() << " = ";

    /* Look for 'var' in SPEX input variables.  If it's found, then output the expression, otherwise, compose a new one. */
    LQX::SyntaxTreeNode * node = LQIO::DOM::Spex::get_input_var_expr( var.getName() );
    if ( node ) {
	node->print( _model._program, 0 );
    } else {
	_model._program << (*value)();
    }
    _model._program << ";" << endl;
}


/*
 * Generate random values inside the `for' loop.
 */

void
Generate::ModelVariable::lqx_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( value->getType() == RV::RandomVariable::DETERMINISTIC ) {
	_model._program << indent( 1 ) << var.getName() << " = " << (*value)() << ";" << endl;
    } else if ( Flags::observe[Flags::PARAMETERS] ) {
	/* Only needed for outputting values in printlns for Flags::show_variables */
	__random_variables.push_back( var.getName() );
    }
}

void
Generate::ModelVariable::lqx_sensitivity( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    double val = 0;
    if ( var.wasSet() ) {
	var.getValue( val );
    } else {
	val = (*value)();
    }
    const string& buf = var.getName();
    double delta = (val * Flags::sensitivity) - val;
    _model._program << indent( 1 ) << &buf[1] << " = [ "
		    << val-delta << ", "
		    << val << ", "
		    << val+delta << " ];" << endl;

    if ( Flags::observe[Flags::PARAMETERS] ) {
	__random_variables.push_back( var.getName() );
    }
}

/*
 * This method is invoked inside of the LQX `for' loop for random models.
 */

void
Generate::ModelVariable::lqx_loop_body( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( !var.wasSet() && value->getType() != RV::RandomVariable::DETERMINISTIC ) {
	_model._program << indent( 2 ) << var.getName() << " = " << value->name();
	if ( value->getType() == RV::RandomVariable::DISCREET ) {
	    /* Set the floor of the RV to 1.0.  Adjust mean. */
	    RV::RandomVariable * clone = value->clone();
	    clone->setMean( clone->getMean() - 1.0 );
	    _model._program << "( ";
	    for ( unsigned int i = 1; i <= clone->nArgs(); ++i ) {
		if ( i > 1 ) _model._program << ", ";
		_model._program << clone->getArg(i);
	    }
	    _model._program << " ) + 1;" << endl;
	    delete clone;
	} else {
	    _model._program << "( ";
	    for ( unsigned int i = 1; i <= value->nArgs(); ++i ) {
		if ( i > 1 ) _model._program << ", ";
		_model._program << value->getArg(i);
	    }
	    _model._program << " );" << endl;	
	}
    }
}


/*
 * This code is used to output results.  For LQX, there's the heading line, and the actual results.
 * For SPEX, it's simply the code to insert the necessary observation variables.
 */

void
Generate::ParameterHeading::operator()( std::string& s ) const
{
    _output << "\"," << endl << indent( _i ) << "  " << "\"" << s;
}

void
Generate::DocumentHeading::operator()( struct document_observation& obs ) const
{
    if ( Flags::observe[obs.flag] ) {
	_output << "\"," << endl << indent( _i ) << "  " << "\"" << obs.lqx_heading;
    }
}

void 
Generate::EntityHeading::operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& e ) const
{
    const LQIO::DOM::Entity * entity = e.second;
    if ( isInterestingProcessor( entity ) && Flags::observe[Flags::UTILIZATION] ) {
        _output << "\", " << endl << indent( _i ) << "  "
		<< "\"p(" << entity->getName() << ").util";
    } else if ( isReferenceTask( entity ) && Flags::observe[Flags::THROUGHPUT] ) {
        _output << "\", " << endl << indent( _i ) << "  "
		<< "\"t(" << entity->getName() << ").tput";
    } else if ( isServerTask( entity ) && Flags::observe[Flags::UTILIZATION] ) {
        _output << "\", " << endl << indent( _i ) << "  "
		<< "\"t(" << entity->getName() << ").util";
    }
}

/* Entry stats here */
void
Generate::EntryHeading::operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second;
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    for_each( phases.begin(), phases.end(), PhaseHeading( _output, _i, entry ) );
}

void
Generate::PhaseHeading::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    const LQIO::DOM::Phase * phase = p.second;
    if ( Flags::observe[Flags::RESIDENCE_TIME] ) {
	_output << "\", " << endl << indent( _i ) << "  "
	        << "\"e(" << _entry->getName() << "," << p.first << ").serv";
    }
    if ( Flags::observe[Flags::QUEUEING_TIME] ) {
	const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
	for_each( calls.begin(), calls.end(), CallHeading( _output, _i, _entry, p.first ) );
    }
}

void
Generate::CallHeading::operator()( const LQIO::DOM::Call * call ) const
{
    const LQIO::DOM::Entry * dst = call->getDestinationEntry();
    _output << "\", " << endl << indent( _i ) << "  "
	    << "\"y(" << _entry->getName() << "," << _phase << "," << dst->getName() << ").wait";
}

void
Generate::ParameterResult::operator()( std::string& s ) const
{
    _output << "," << endl << indent( _i ) << "  " << s;
}

void
Generate::DocumentResult::operator()( struct document_observation& obs ) const
{
    if ( Flags::observe[obs.flag] ) {
	_output << "," << endl << indent( _i ) << "  " << "document()." << obs.lqx_attribute;
    }
}


void 
Generate::EntityResult::operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& e ) const
{
    const LQIO::DOM::Entity * entity = e.second;
    if ( isInterestingProcessor( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	_output << "," << endl << indent( _i ) << "  " 
		<< "processor(\"" << entity->getName() << "\").utilization";
    } else if ( isReferenceTask( entity ) && Flags::observe[Flags::THROUGHPUT] ) {		/* Reference task */
	_output << "," << endl << indent( _i ) << "  "
		<< "task(\"" << entity->getName() << "\").throughput";
    } else if ( isServerTask( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	_output << "," << endl << indent( _i ) << "  "
		<< "task(\"" << entity->getName() << "\").utilization";
    }
}

/* Entry stats here */
void
Generate::EntryResult::operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second;
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    for_each( phases.begin(), phases.end(), PhaseResult( _output, _i, entry ) );
}

void
Generate::PhaseResult::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    const LQIO::DOM::Phase * phase = p.second;
    if ( Flags::observe[Flags::RESIDENCE_TIME] ) {
	_output << "," << endl << indent( _i ) << "  "
	       << "phase(entry(\"" << _entry->getName() << "\")," << p.first << ").service_time";
    }
    if ( Flags::observe[Flags::QUEUEING_TIME] ) {
	const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
	for_each( calls.begin(), calls.end(), CallResult( _output, _i, _entry, p.first ) );
    }
}

void
Generate::CallResult::operator()( const LQIO::DOM::Call * call ) const
{
    const LQIO::DOM::Entry * dst = call->getDestinationEntry();
    _output << "," << endl << indent( _i ) << "  "
	    << "call(phase(entry(\"" << _entry->getName() << "\")," << _phase << "),\"" << dst->getName() << "\").waiting";
}

/* static */ ostream& 
Generate::printIndent( ostream& output, const int i ) 
{
    output << setw( i * 3 ) << " ";
    return output;
}

/* ------------------ Logic for creating an SPEX program ------------------ */

void
Generate::addSpex( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    /* Insert all of the SPEX control code. */
    const unsigned n = getNumberOfRuns();
    if ( n > 1 ) {
	spex_array_comprehension( "$experiments", 1, n, 1 );
	spex_result_assignment_statement( "$0", 0 );		/* print out index first. */
    }

    /* Make the variables in the model. */
    (this->*f)( g );

    if ( Flags::has_any_observation() ) {

	for_each( &document_observation[0], &document_observation[5], documentObservation );

	/* Make the result variables */
	const map<unsigned,LQIO::DOM::Entity *>& entities = _document->getEntities();
	for_each( entities.begin(), entities.end(), EntityObservation() );

	/* Entry stats here */
	const std::map<std::string,LQIO::DOM::Entry*>& entries = _document->getEntries();
	for_each( entries.begin(), entries.end(), EntryObservation() );
    }
}

/*
 * Generate a vector of values.  For spex, it's "$x, $y = expr."
 */

void
Generate::ModelVariable::spex_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) {
	return;		// Ignore.
    } else if ( dynamic_cast<const RV::Constant *>(value) ) {
	return spex_scalar( var, value );
    } else {
	const std::string& name = var.getName();

	if ( !LQIO::DOM::Spex::has_input_var( name ) ) {	/* If set, previously, don't set again */
	    LQX::SyntaxTreeNode * expr = 0;

	    /* Get args and form call to RV generator */

	    if ( value->getType() == RV::RandomVariable::DISCREET ) {
		std::vector<LQX::SyntaxTreeNode *> * args = new std::vector<LQX::SyntaxTreeNode*>();
		/* Set the floor of the RV to 1.0.  Adjust mean. */
		RV::RandomVariable * clone = value->clone();
		clone->setMean( clone->getMean() - 1.0 );
		for ( unsigned int i = 1; i <= clone->nArgs(); ++i ) {
		    args->push_back( new LQX::ConstantValueExpression( clone->getArg(i) ) );
		}
		/* Need to add 1.. */
		expr = new LQX::MathExpression(LQX::MathExpression::ADD, new LQX::MethodInvocationExpression( clone->name(), args ), new LQX::ConstantValueExpression(1.0) );
		delete clone;
	    } else {
		std::vector<LQX::SyntaxTreeNode *> * args = new std::vector<LQX::SyntaxTreeNode*>();
		for ( unsigned int i = 1; i <= value->nArgs(); ++i ) {
		    args->push_back( new LQX::ConstantValueExpression( value->getArg(i) ) );
		}
		expr = new LQX::MethodInvocationExpression( value->name(), args );
	    }

	    spex_forall( "$experiments", name.c_str(), expr );
	    if ( Flags::observe[Flags::PARAMETERS] ) {
		spex_result_assignment_statement( name.c_str(), 0 );
	    }
	}
    }
}

void
Generate::ModelVariable::spex_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) return;		// Ignore.
    // Need to pass in function to store in spex. -- spex_forall() or spex_assignment}
    if ( !LQIO::DOM::Spex::has_input_var( var.getName() ) ) {	/* If set, previously, don't set again */
	LQX::SyntaxTreeNode * expr = new LQX::ConstantValueExpression( (*value)() );
	spex_assignment_statement( var.getName().c_str(), expr, true );
    }
}


void
Generate::ModelVariable::spex_sensitivity( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    double val = 0;
    if ( var.wasSet() ) {
	var.getValue( val );
    } else {
	val = (*value)();
    }
    double delta = (val * Flags::sensitivity) - val;
    std::vector<LQX::SyntaxTreeNode *> * list = new std::vector<LQX::SyntaxTreeNode*>();
    list->push_back( new LQX::ConstantValueExpression( val - delta ) );
    list->push_back( new LQX::ConstantValueExpression( val ) );
    list->push_back( new LQX::ConstantValueExpression( val + delta ) );
    spex_array_assignment( var.getName().c_str(), list, true );
}

void
Generate::documentObservation( struct document_observation& obs )
{
    if ( Flags::observe[obs.flag] ) {
	spex_document_observation( obs.key, obs.spex_variable );
	spex_result_assignment_statement( obs.spex_variable, 0 );
    }
}

void 
Generate::EntityObservation::operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& e ) const
{
    const LQIO::DOM::Entity * entity = e.second;
    if ( isInterestingProcessor( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	std::string name = "$p_";
	name += entity->getName();
	spex_processor_observation( (void *)entity, KEY_UTILIZATION, 0, name.c_str(), 0 );
	spex_result_assignment_statement( name.c_str(), 0 );
    } else if ( isReferenceTask( entity ) && Flags::observe[Flags::THROUGHPUT] ) {		/* Reference task */
	std::string name = "$f_";
	name += entity->getName();
	spex_task_observation( (void *)entity, KEY_THROUGHPUT, 0, 0, name.c_str(), 0 );
	spex_result_assignment_statement( name.c_str(), 0 );
    } else if ( isServerTask( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	std::string name = "$u_";
	name += entity->getName();
	spex_task_observation( (void *)entity, KEY_UTILIZATION, 0, 0, name.c_str(), 0 );
	spex_result_assignment_statement( name.c_str(), 0 );
    }
}

void
Generate::EntryObservation::operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second;
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    for_each( phases.begin(), phases.end(), PhaseObservation( entry ) );
}

void
Generate::PhaseObservation::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    const LQIO::DOM::Phase * phase = p.second;
    if ( Flags::observe[Flags::RESIDENCE_TIME] ) {
	std::string name = "$r_";
	name += phase->getName();
	spex_entry_observation( (void *)_entry, KEY_SERVICE_TIME, p.first, 0, name.c_str(), 0 );
	spex_result_assignment_statement( name.c_str(), 0 );
    }

    if ( Flags::observe[Flags::QUEUEING_TIME] ) {
	const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
	for_each( calls.begin(), calls.end(), CallObservation( _entry, p.first ) );
    }
}

void
Generate::CallObservation::operator()( const LQIO::DOM::Call * call ) const
{
    std::string name = "$w_";
    name += call->getName();
    spex_call_observation( (void *)_entry, KEY_WAITING, _phase, (void *)call->getDestinationEntry(), 0, name.c_str(), 0 );
    spex_result_assignment_statement( name.c_str(), 0 );
}

/* ------------------------------------------------------------------------ */

/*
 * Code which converts constants in the DOM to variables (and then creates necessary
 * LQX/SPEX code via the magic _f function).
 */

void
Generate::ProcessorVariable::operator()( const std::pair<std::string,LQIO::DOM::Processor *>& p ) const
{
    LQIO::DOM::Processor * processor = p.second;
    if ( processor->isInfinite() ) return;		/* Skip */

    LQIO::DOM::ExternalVariable * copies = const_cast<LQIO::DOM::ExternalVariable *>(processor->getCopies());
    RV::RandomVariable * multiplicity = const_cast<RV::RandomVariable *>(__processor_multiplicity);
    if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) && Flags::convert[Flags::PROCESSOR_MULTIPLICITY] ) {
	/* Convert a constant into a variable */
	if ( !Flags::override[Flags::PROCESSOR_MULTIPLICITY] ) {
	    /*  initialize variable to the constant value found in the DOM (otherwise use the RV) */
	    multiplicity = multiplicity->clone();
	    multiplicity->setMean( to_double( *copies ) );
	}
	copies = get_rv( "$m_", processor->getName(), multiplicity );	/* Create a lqx/spex variable */
	processor->setCopies( copies );			/* Change the DOM object to the variable */
    }
    (this->*_f)( *copies, multiplicity ); 		/* Insert spex/lqx assignment code for variable */
}


void
Generate::TaskVariable::operator()( const std::pair<std::string,LQIO::DOM::Task *>& t ) const
{
    LQIO::DOM::Task * task = t.second;
    LQIO::DOM::ExternalVariable * copies = const_cast<LQIO::DOM::ExternalVariable *>(task->getCopies());
    if ( isReferenceTask( task ) ) {
	RV::RandomVariable * customers = const_cast<RV::RandomVariable *>(__customers_per_client);
	if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) && Flags::convert[Flags::CUSTOMERS] ) {
	    /* Convert a constant into a variable and initialize it to the constant value. */
	    if ( !Flags::override[Flags::CUSTOMERS] ) {
		customers = customers->clone();
		customers->setMean( to_double( *copies ) );
	    }
	    copies = get_rv( "$c_", task->getName(), customers );
	    task->setCopies( copies );
	}
	(this->*_f)( *copies, customers );  		/* Inserts spex/lqx assignment code for variable */
#ifdef THINK_TIME
	LQIO::DOM::ExternalVariable * time = dynamic_cast<LQIO::DOM::ExternalVariable * >( task->getThinkTime() );
	RV::RandomVariable * think_time = const_cast<RV::RandomVariable *>(__think_time);
	if ( (!time || dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( time )) && Flags::convert[Flags::THINK_TIME] ) {
	    if ( !Flags::override[Flags::THINK_TIME] ) {
		think_time = think_time->clone();
		think_time->setMean( time ? to_double( *time ) : 0.0 );
	    }
	    time = get_rv( "$Z_", task->getName(), think_time );
	    task->setThinkTime( time );
	}
	if ( time ) (this->*_f)( *time, think_time ); 	/* Inserts spex/lqx assignment code for variable */
#endif
    } else if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) &&  __probability_infinite_server() != 0 ) {
	/* Any task with a constant multiplicity is eligible for conversion into an infinite server */
	/* Tasks with multiplicities set using a variable cannot be changed since the variable */
	/* would end up being unused, causing an exception at run-time. */
	task->setCopiesValue( 1 );
	task->setSchedulingType( SCHEDULE_DELAY );
	return;
    } else {
	RV::RandomVariable * multiplicity = const_cast<RV::RandomVariable *>(__task_multiplicity);
	if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) && Flags::convert[Flags::TASK_MULTIPLICITY] ) {
	    task->setSchedulingType( SCHEDULE_FIFO );	/* We might be converting an infinte server into a multi-server */
	    /* Convert a constant into a variable and initialize it to the constant value. */
	    if ( !Flags::override[Flags::TASK_MULTIPLICITY] ) {
		multiplicity = multiplicity->clone();
		multiplicity->setMean( to_double( *copies ) );
	    }
	    copies = get_rv( "$m_", task->getName(), multiplicity );
	    task->setCopies( copies );
	}
	(this->*_f)( *copies, multiplicity ); 		/* Inserts spex/lqx assignment code for variable */
    }
}



void
Generate::EntryVariable::operator()( const std::pair<std::string,LQIO::DOM::Entry *>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second; 
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    for_each( phases.begin(), phases.end(), PhaseVariable( _model, _f ) );
}


void
Generate::PhaseVariable::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    LQIO::DOM::Phase * phase = p.second;
    LQIO::DOM::ExternalVariable * service = const_cast<LQIO::DOM::ExternalVariable *>( phase->getServiceTime() );
    const bool use_think_time = isReferenceTaskPhase( phase );
    RV::RandomVariable * service_time = const_cast<RV::RandomVariable *>( use_think_time ? __think_time : __service_time );
    if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( service ) && (use_think_time ? Flags::convert[Flags::THINK_TIME] : Flags::convert[Flags::SERVICE_TIME]) ) {
	if ( !(use_think_time ? Flags::override[Flags::THINK_TIME] : Flags::override[Flags::SERVICE_TIME]) ) {
	    service_time = service_time->clone();
	    service_time->setMean( to_double( *service ) );
	}
	service = get_rv( "$s_", phase->getName(), service_time );
	phase->setServiceTime( service );
    } 
    (this->*_f)( *service, service_time ); 		/* Inserts spex/lqx assignment code for variable */

    if ( Flags::sensitivity == 0.0 ) {
	const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
	for_each( calls.begin(), calls.end(), CallVariable( _model, _f ) );
    }
}



void
Generate::CallVariable::operator()( LQIO::DOM::Call * call ) const
{
    /* check for var... */
    RV::RandomVariable * rate;
    switch ( call->getCallType() ) {
    case LQIO::DOM::Call::RENDEZVOUS:	 rate = const_cast<RV::RandomVariable *>(__rendezvous_rate); break;
    case LQIO::DOM::Call::SEND_NO_REPLY: rate = const_cast<RV::RandomVariable *>(__send_no_reply_rate); break;
    case LQIO::DOM::Call::FORWARD:       rate = const_cast<RV::RandomVariable *>(__forwarding_probability); break;
    default: abort();
    }

    LQIO::DOM::ExternalVariable * calls = const_cast<LQIO::DOM::ExternalVariable *>( call->getCallMean() );
    if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>(calls) && Flags::convert[Flags::REQUEST_RATE] ) {
	if ( !Flags::override[Flags::REQUEST_RATE] ) {
	    rate = rate->clone();
	    rate->setMean( to_double( *calls ) );
	}
	switch ( call->getCallType() ) {
	case LQIO::DOM::Call::RENDEZVOUS:    calls = get_rv( "$y_", call->getName(), rate ); break;
	case LQIO::DOM::Call::SEND_NO_REPLY: calls = get_rv( "$z_", call->getName(), rate ); break;
	case LQIO::DOM::Call::FORWARD:       calls = get_rv( "$f_", call->getName(), rate ); break;
	}
	call->setCallMean( calls );
    }
    (this->*_f)( *calls, rate ); 			/* Inserts spex/lqx assignment code for variable */
}

/*
 * Get either a value or a variable name.  The value is based on the
 * random variate class passed in.  If a variable is being created,
 * then a subsequent pass which produces the "code" will assign
 * values.
 */

LQIO::DOM::ExternalVariable * 
Generate::ModelVariable::get_rv( const std::string& prefix, const std::string& name, const RV::RandomVariable * value ) const
{
    std::string var = prefix + name;
    if ( Flags::lqx_spex_output ) {
	return _model._document->getSymbolExternalVariable( var );
    } else {
	return new LQIO::DOM::ConstantExternalVariable( (*value)() );
    }
}

/* ------------------------------------------------------------------------ */

/* 
 * Output an input file.
 */

ostream& 
Generate::print( ostream& output ) const
{
    switch ( Flags::output_format ) {
    case FORMAT_XML:  _document->serializeDOM( output );  break;	/* Output LQX code if running. */
    case FORMAT_SRVN: {
	LQIO::SRVN::Input srvn( getDOM(), _document->getEntities(), Flags::annotate_input, false );
	srvn.print( output );
    }
	break;
    }

    return output;
}



ostream&
Generate::printStatistics( ostream& output ) const
{
    output << "File Name: " << LQIO::input_file_name << endl;

    output << "    Number of server layers: " << _task.size() - 1 << endl;

    const double n_clients = static_cast<double>(_task[REF_LAYER].size());
    output << "    Number of clients: " << n_clients << endl;
    output << "    Mean number of customers per client: " << for_each( _task[REF_LAYER].begin(), _task[REF_LAYER].end(), AccumulateCustomers()).mean() << endl;
    output << "    Mean client think time: " << for_each( _entry.begin(), _entry.end(), AccumulateThinkTime()).mean() << endl;

    output << "    Number of processors: " << _processor.size() << endl;
    output << "    Mean processor multiplicity: " << for_each( _processor.begin(), _processor.end(), AccumulateMultiplicity()).mean() << endl;

    AccumulateMultiplicity threads = for_each(_task.begin(), _task.end(), AccumulateMultiplicity() );		/* Arg is pass by value, so assignment is needed */
    const double n_servers = static_cast<double>(threads.count());
    output << "    Number of server tasks: " << threads.count() << endl;
    output << "    Mean server task multiplicity: " << threads.mean() << endl;

    const double n_entries = static_cast<double>(_entry.size()) - n_clients;		/* Server entries only */
    output << "    Number of server entries: " << n_entries << endl;
    output << "    Mean number of entries at server tasks: " << n_entries / n_servers << endl;

    AccumulateServiceTime service_time = for_each(_entry.begin(), _entry.end(), AccumulateServiceTime() );	/* Arg is pass by value, so assignment is needed */
    const double n_phases = static_cast<double>(service_time.count());	 		/* Server entries only */
    output << "    Mean number of phases per server entry: " << n_phases / n_entries << endl;
    output << "    Mean phase service time: " << service_time.mean() << endl;
    

    AccumulateRequests requests = for_each( _call.begin(), _call.end(), AccumulateRequests() );			/* Arg is pass by value, so assignment is needed */
    const double n_calls = static_cast<double>( requests.count() );
    output << "    Number of calls: " << n_calls << endl;
    output << "    Mean fan-in per entry: " << n_calls / n_entries << endl;
    output << "    Mean fan-out per phase: " << n_calls / (n_phases + n_clients) << endl;
    output << "    Mean request rate per call: " << requests.mean() << endl;
    return output;
}

/* ------------------------------------------------------------------------ */

bool 
Generate::isReferenceTask( const LQIO::DOM::Entity * task ) 
{
    if ( !dynamic_cast<const LQIO::DOM::Task *>(task) ) return false;
    switch ( task->getSchedulingType() ) {
    case SCHEDULE_CUSTOMER:
    case SCHEDULE_POLL:
    case SCHEDULE_BURST: 
	return true;
    default:
	return false;
    }
}

bool
Generate::isReferenceTaskPhase( const LQIO::DOM::Phase * phase )
{
    const LQIO::DOM::Entry* entry = phase->getSourceEntry();
    return isReferenceTask( entry->getTask() );
}


bool 
Generate::isServerTask( const LQIO::DOM::Entity * task ) 
{
    if ( !dynamic_cast<const LQIO::DOM::Task *>(task) ) return false;
    switch ( task->getSchedulingType() ) {
    case SCHEDULE_CUSTOMER:
    case SCHEDULE_POLL:
    case SCHEDULE_BURST: 
	return false;
    default:
	return true;
    }
}

bool 
Generate::isInterestingProcessor( const LQIO::DOM::Entity * entity ) 
{
    const LQIO::DOM::Processor * processor = dynamic_cast<const LQIO::DOM::Processor *>(entity);
    if ( !processor ) return false;
    const std::set<LQIO::DOM::Task*>& tasks = processor->getTaskList();

    if ( tasks.size() == 0 ) return false;
    if ( tasks.size() > 1 ) return true;
    return isServerTask( *tasks.begin() );
}

/* ------------------------------------------------------------------------ */

Generate::Accumulate&
Generate::Accumulate::operator+=( const LQIO::DOM::ExternalVariable * augend )
{
    if ( augend->wasSet() ) {
	const double x = to_double( *augend );
	_sum += x;
	_sum_squared += ( x * x );
	_n += 1; 
    }
    return *this;
}

Generate::Accumulate&
Generate::Accumulate::operator+=( const Generate::Accumulate& augend )
{
    _sum         += augend._sum;
    _sum_squared += augend._sum_squared;
    _n           += augend._n;
    return *this;
}

double
Generate::Accumulate::mean() const
{
    return (_n > 0) ? _sum / static_cast<double>(_n) : 0.;
}

void
Generate::AccumulateCustomers::operator()( const LQIO::DOM::Task * task )
{
    if ( isReferenceTask( task ) ) {
	(*this) += task->getCopies();
    }
}

void
Generate::AccumulateMultiplicity::operator()( const LQIO::DOM::Processor * processor )
{
    (*this) += processor->getCopies();
}


void
Generate::AccumulateMultiplicity::operator()( const vector<LQIO::DOM::Task *>& tasks )
{
    (*this) += for_each( tasks.begin(), tasks.end(), AccumulateMultiplicity() );
}

void
Generate::AccumulateMultiplicity::operator()( const LQIO::DOM::Task * task )
{
    if ( isServerTask( task ) ) {
	(*this) += task->getCopies();
    }
}

void
Generate::AccumulateServiceTime::operator()( const LQIO::DOM::Entry * entry )
{
    if ( isServerTask( entry->getTask() ) ) {
	(*this) += for_each( entry->getPhaseList().begin(), entry->getPhaseList().end(), Generate::AccumulateServiceTime() );
    }
}

void
Generate::AccumulateServiceTime::operator()( const std::pair<unsigned,LQIO::DOM::Phase *>& phase )
{
    (*this) += phase.second->getServiceTime();
}

void
Generate::AccumulateThinkTime::operator()( const LQIO::DOM::Entry * entry )
{
    if ( isReferenceTask( entry->getTask() ) ) {
	(*this) += for_each( entry->getPhaseList().begin(), entry->getPhaseList().end(), Generate::AccumulateThinkTime() );	/* Should only be one... */
    }
}

void
Generate::AccumulateThinkTime::operator()( const std::pair<unsigned,LQIO::DOM::Phase *>& phase )
{
    (*this) += phase.second->getServiceTime();			/* lqngen sets the service time of reference task entries as 'think time' */
}

void
Generate::AccumulateRequests::operator()( const LQIO::DOM::Call * call )
{
    (*this) += call->getCallMean();
}

