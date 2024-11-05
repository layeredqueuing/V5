/* generate.cc    -- Greg Franks Thu Jul 29 2004
 *
 * Logic for generating and transforming the model.
 * If generating, generate() constructs the DOM.  Otherwise the DOM is loaded.
 * parameterize() is then called to convert constant parameters to variables.
 * LQX is created directly, rather than by doing LQX::_program->print().  This is less verbose.
 * However, to eliminate code here, the spex construction functions will have to save the
 * LQX expressions and then construct the program.
 * ------------------------------------------------------------------------
 * $Id: generate.cc 17431 2024-11-05 10:08:21Z greg $
 */

#include "lqngen.h"
#include <algorithm>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <lqio/dom_activity.h>
#include <lqio/dom_document.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_extvar.h>
#include <lqio/dom_group.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_task.h>
#include <lqio/input.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
#include <lqx/SyntaxTree.h>
#include <../lqiolib/src/srvn_gram.h>		/* Derived file */
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include "generate.h"
#include "randomvar.h"

unsigned Generate::__iteration_limit    = 50;
unsigned Generate::__print_interval     = 10;
double Generate::__convergence_value    = 0.00001;
double Generate::__underrelaxation      = 0.9;
std::string Generate::__comment 	= "";
std::map<std::string,std::string> Generate::__pragma;

static const unsigned int REF_LAYER = 0;
static const unsigned int SERVER_LAYER = 1;

Generate::layering_t Generate::__task_layering	    = Generate::RANDOM_LAYERING;
Generate::layering_t Generate::__processor_layering = Generate::RANDOM_LAYERING;


RV::RandomVariable * Generate::__customers_per_client		= 0;
RV::RandomVariable * Generate::__forwarding_probability		= 0;
RV::RandomVariable * Generate::__number_of_entries		= 0;
RV::RandomVariable * Generate::__outgoing_requests		= 0;
RV::RandomVariable * Generate::__processor_multiplicity		= 0;
RV::RandomVariable * Generate::__rendezvous_rate		= 0;
RV::RandomVariable * Generate::__send_no_reply_rate		= 0;
RV::RandomVariable * Generate::__service_time			= 0;
RV::RandomVariable * Generate::__task_multiplicity		= 0;
RV::RandomVariable * Generate::__think_time			= 0;
RV::Probability	     Generate::__probability_cfs_processor	= 0.;
RV::Probability      Generate::__probability_delay_server	= 0.;
RV::Probability      Generate::__probability_infinite_server	= 0.;
RV::Probability      Generate::__probability_second_phase	= 0.;
RV::Beta	     Generate::__group_share			= 0.8;

std::vector<std::string> Generate::__random_variables;

static struct document_observation {
    int flag;
    int key;
    const char * lqx_heading;
    const char * lqx_attribute;
    const char * spex_variable;
} document_observation[] = {
    { Flags::ITERATIONS,   KEY_ITERATIONS,   "iters",   "iterations",      "$i" },
    { Flags::MVA_WAITS,    KEY_WAITING,      "waits",   "waits",           "$w" },		/* Overloaded */
    { Flags::MVA_STEPS,    KEY_SERVICE_TIME, "steps",   "steps",           "$s" },		/* Overloaded */
    { Flags::ELAPSED_TIME, KEY_ELAPSED_TIME, "time",    "elapsed_time",    "$time" },
    { Flags::USER_TIME,    KEY_USER_TIME,    "user",    "user_cpu_time",   "$usr" },
    { Flags::SYSTEM_TIME,  KEY_SYSTEM_TIME,  "system",  "system_cpu_time", "$sys" },
};

struct HasKey {
    HasKey( unsigned int key ) : _key(key) {}
    bool operator()( const std::pair<const LQIO::DOM::DocumentObject *,LQIO::Spex::ObservationInfo>& p ) { return p.second.getKey() == _key; }
private:
    const int _key;
};

static inline unsigned int max( unsigned int a, unsigned int b ) { return a >= b ? a : b; }
static inline unsigned int min( unsigned int a, unsigned int b ) { return a <= b ? a : b; }

/* ------------------------------------------------------------------------ */
/*
 * Constructor for lqngen
 */

Generate::Generate( const LQIO::DOM::Document::InputFormat output_format, const unsigned runs, const unsigned layers, const unsigned customers, const unsigned processors, const unsigned clients, const unsigned tasks )
    : _document(0), _output_format(output_format), _runs(runs), _number_of_customers(customers), _number_of_layers(layers), 
      _number_of_processors(min(processors,tasks)), _number_of_clients(clients), _number_of_tasks(max(layers,tasks))
{
    _document = new LQIO::DOM::Document( LQIO::DOM::Document::InputFormat::AUTOMATIC );		// For XML output.
    _number_of_tasks_for_layer.resize( _number_of_layers + 1 );
    _task.resize( _number_of_layers + 1 );
    if ( Flags::spex_output ) {
	LQIO::Spex::clear();
    }
}


Generate::Generate( LQIO::DOM::Document * document, const LQIO::DOM::Document::InputFormat output_format, const unsigned runs, const unsigned customers )
    : _document(document), _output_format(output_format), _runs(runs), _number_of_customers(customers), _number_of_layers(0), _number_of_processors(0), _number_of_clients(0), _number_of_tasks(0)
{
    _task.resize( 2 );			/* Used for clients only */
    const std::map<std::string,LQIO::DOM::Task*>& tasks = document->getTasks();
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator t = tasks.begin(); t != tasks.end(); ++t ) {
	LQIO::DOM::Task * task = t->second;
	if ( task->getSchedulingType() == SCHEDULE_CUSTOMER ) {
	    _task[REF_LAYER].push_back( task );
	} else {
	    _task[SERVER_LAYER].push_back( task );
	}
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
    groupize();
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

    _document->setResultDescription( "Layered Queueing Network Model." );
    populateLayers();

    std::vector<unsigned int> stride(  _number_of_layers + 1 );
    const unsigned int width = static_cast<unsigned int>( ceil( log10( _number_of_tasks ) ) );

    /* Create all clients, and give them their own processor */

    for ( unsigned int i = 0; i < _number_of_clients; ++i ) {
	std::ostringstream name;
	name << ( Flags::long_names ? "Client" : "c" ) << std::setw( width ) << std::setfill( '0' ) << i;
	LQIO::DOM::Processor * proc = addProcessor( name.str(), SCHEDULE_DELAY );		// Hide this one.
	std::vector<LQIO::DOM::Entry *> entries;
	LQIO::DOM::Entry * entry = addEntry( name.str(), RV::Probability(0.0) );
	_entry.push_back( entry );
	entries.push_back( entry );
	_task[REF_LAYER].push_back( addTask( name.str(), SCHEDULE_CUSTOMER, entries, proc ) );	// Get RV for number of customers.
    }
    stride[REF_LAYER] = _entry.size();

    /* Create all processors */

    for ( unsigned int i = 0; i < _number_of_processors; ++i ) {
	std::ostringstream name;
	name << ( Flags::long_names ? "Processor" : "p" ) << std::setw( width ) << std::setfill( '0' ) << i;
	LQIO::DOM::Processor * proc = addProcessor( name.str(), SCHEDULE_PS );
	_processor.push_back( proc );
    }

    /* Create and distribute all tasks,  connect to processors */

    const unsigned int n = _number_of_tasks_for_layer.size();
    const double tasks_per_processor = static_cast<double>(n) / static_cast<double>(_number_of_processors);
    unsigned int k = 0;
    const bool extra_entries = !dynamic_cast<RV::Constant *>(__number_of_entries) || (*__number_of_entries)() > 1;
    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
	for ( unsigned int j = 0; j < _number_of_tasks_for_layer[i]; ++j, ++k ) {
	    std::ostringstream entry_name;
	    entry_name << ( Flags::long_names ? "Entry" : "e" ) << std::setw( width ) << std::setfill( '0' ) << k;
	    if ( extra_entries ) {
		entry_name << "_0";
	    }

	    std::vector<LQIO::DOM::Entry *> entries;		/* List attached to task. */
	    LQIO::DOM::Entry * entry = addEntry( entry_name.str(), __probability_second_phase );
	    _entry.push_back( entry );
	    entries.push_back( entry );

	    /* Add extra entries */

	    if ( extra_entries ) {
		const unsigned n_entries = (*__number_of_entries)();
		for ( unsigned int e = 1; e < n_entries; ++e ) {
		    std::ostringstream extra_name;
		    extra_name << ( Flags::long_names ? "Entry" : "e" ) << std::setw( width ) << std::setfill( '0' ) << k
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
		p = k % _number_of_processors;		/* top down.. */
	    } else if ( k <  _number_of_processors ) {
		p = k;				/* Assign first set of processors deterministically */
	    } else {
		p = static_cast<unsigned int>(floor( RV::RandomVariable::number() * _number_of_processors) );
	    }

	    std::ostringstream task_name;
	    task_name  << ( Flags::long_names ? "Task"  : "t" ) << std::setw( width ) << std::setfill( '0' ) << k;
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
	    const std::vector<LQIO::DOM::Entry *>& entries = (*tp)->getEntryList();
	    for ( std::vector<LQIO::DOM::Entry *>::const_iterator ep = entries.begin(); ep != entries.end(); ++ep ) {
		LQIO::DOM::Entry * dst = *ep;
		if ( ep == entries.begin() ) {
		    const unsigned int k = __task_layering == RANDOM_LAYERING
			? static_cast<unsigned int>(number_of_clients * RV::RandomVariable::number())	/* Picks any immediate client task.	*/
			: i;								/* Pick the immediate client. 		*/
		    if ( _task[j-1].size() > 0 ) {
			LQIO::DOM::Entry * src = _task[j-1][k]->getEntryList().front();
			LQIO::DOM::Call * call = addCall( src, dst, __probability_second_phase );
			_call.push_back( call );
			i = (i + 1) % number_of_clients;		// too few clients handled here.
		    }
		} else {
		    /* Choose randomly. */
		    const unsigned int e = static_cast<unsigned int>(stride[j-1] * RV::RandomVariable::number());	/* Picks a higher layer task. 	*/
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

    const double multiplier = (*Generate::__outgoing_requests)();
    if ( multiplier > 1.0 ) {
	const unsigned int extra_calls = _call.size() *  (multiplier - 1.0);
	const unsigned n_entries = _entry.size();
	for ( unsigned int i = 0; i < extra_calls; ++i ) {
	    const unsigned int server_entry = static_cast<unsigned int>((n_entries - stride[REF_LAYER]) * RV::RandomVariable::number()) + stride[REF_LAYER];
	    /* Find layer with server entry */
	    unsigned int server_layer = SERVER_LAYER;
	    while ( server_layer < _number_of_layers && stride[server_layer+1] < server_entry ) {
		server_layer += 1;
	    }
	    const unsigned int client_entry = static_cast<unsigned int>(stride[server_layer-1] * RV::RandomVariable::number());
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
	if ( !phase->hasRendezvous() ) {
	    const unsigned int n_entries = _entry.size() - n_clients;
	    const unsigned int e = static_cast<unsigned int>(static_cast<double>(n_entries) * RV::RandomVariable::number() ) + n_clients;	/* Skip ref task */
	    LQIO::DOM::Call * call = addCall( src, _entry.at(e), RV::Probability(0.) );		/* Ref tasks don't have second phase */
	    _call.push_back( call );
	}
    }

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
    const unsigned int n = _number_of_tasks_for_layer.size();
    unsigned int count = 0;

    switch ( __task_layering ) {
    case RANDOM_LAYERING:
	for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
	    _number_of_tasks_for_layer[i] += 1;		/* Guarantee one task per layer */
	    count += 1;
	}
	for ( ; count < _number_of_tasks; ++count ) {
	    unsigned int i = (RV::RandomVariable::number() * _number_of_layers) + 1;
	    _number_of_tasks_for_layer[i] += 1;		/* Now stick the rest in anywhere */
	}
	break;
	
    case DETERMINISTIC_LAYERING:
    case UNIFORM_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		_number_of_tasks_for_layer[i] += 1;
		count += 1;
		if ( count >= _number_of_tasks ) goto done;
	    }
	}
	break;

    case PYRAMID_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		for ( unsigned int j = i; j < n; ++j ) {
		    _number_of_tasks_for_layer[j] += 1;
		    count += 1;
		    if ( count >= _number_of_tasks ) goto done;
		}
	    }
	}
	break;

    case FUNNEL_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		for ( unsigned int j = SERVER_LAYER; j <= n - i; ++j ) {
		    _number_of_tasks_for_layer[j] += 1;
		    count += 1;
		    if ( count >= _number_of_tasks ) goto done;
		}
	    }
	}
	break;

    case HOUR_GLASS_LAYERING:
	for ( unsigned mid_point = (n+1)/2;; ) {
	    for ( unsigned int i = SERVER_LAYER; i <= mid_point; ++i ) {
		for ( unsigned int j = SERVER_LAYER; j <= mid_point - i; ++j ) {
		    _number_of_tasks_for_layer[j] += 1;
		    count += 1;
		    if ( count >= _number_of_tasks ) goto done;
		    _number_of_tasks_for_layer[n - j] += 1;
		    count += 1;
		    if ( count >= _number_of_tasks ) goto done;
		}
	    }
	    /* Odd number of layers. */
	    const unsigned int j = n/2+1;
	    if ( j != mid_point ) {
		_number_of_tasks_for_layer[mid_point] += 1;
		count += 1;
		if ( count >= _number_of_tasks ) goto done;
	    }
	}
	break;

    case FAT_LAYERING:
	for ( ;; ) {
	    for ( unsigned int i = SERVER_LAYER; i < n; ++i ) {
		for ( unsigned int j = i; j <= n - i; ++j ) {
		    _number_of_tasks_for_layer[j] += 1;
		    count += 1;
		    if ( count >= _number_of_tasks ) goto done;
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
Generate::addProcessor( const std::string& name, const scheduling_type sched_flag )
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
 * Add a group to a processor
 */

LQIO::DOM::Group *
Generate::addGroup( LQIO::DOM::Processor * processor, double share )
{
    if ( share <= 0 || 1 <= share ) throw std::invalid_argument( "share" );
    std::ostringstream name;
    name << ( Flags::long_names ? "Group" : "g" ) << _document->getNumberOfGroups() + 1;
    LQIO::DOM::Group * group = new LQIO::DOM::Group( _document, name.str().c_str(), processor );		/* Guarantee, not cap */
    group->setGroupShareValue( share );
    processor->addGroup( group );
    _document->addGroup( group );
    return group;
}



/*
 * Add a task to a layer.
 */

LQIO::DOM::Task *
Generate::addTask( const std::string& name, const scheduling_type sched_flag, const std::vector<LQIO::DOM::Entry *>& entries, LQIO::DOM::Processor * processor )
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
Generate::addEntry( const std::string& name, const RV::Probability& pr_2nd_phase )
{
    LQIO::DOM::Entry* entry = new LQIO::DOM::Entry( _document, name.c_str() );
    _document->addEntry(entry);
    _document->db_check_set_entry( entry, LQIO::DOM::Entry::Type::STANDARD );
    unsigned int n_phases = 1;
    if ( pr_2nd_phase() != 0.0 ) {
	n_phases = 2;
    }
    for ( unsigned p = 1; p <= n_phases; ++p ) {
	LQIO::DOM::Phase* phase = entry->getPhase(p);
	std::string phase_name=name;
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

    std::string name = phase->getName();
    name += '_';
    name += dst->getName();

    LQIO::DOM::Call * call = new LQIO::DOM::Call( _document, LQIO::DOM::Call::Type::RENDEZVOUS, phase, dst );
    call->setCallMeanValue( 1.0 );
    call->setName( name );
    phase->addCall( call );

    return call;
}



/*
 * Add groups, to taste.
 * First, go through all processors, finding those that aren't infinite and have more than one task.
 * These processors are eligible.
 * Next, add tasks to groups and voila, we're done.
 */

Generate&
Generate::groupize()
{
    const std::map<std::string,LQIO::DOM::Processor*>& processors = _document->getProcessors();
    for_each( processors.begin(), processors.end(), Groupize( *this ) );
    return *this;
}


void
Generate::groupize( LQIO::DOM::Processor* processor )
{
    const std::set<LQIO::DOM::Task*>& tasks = processor->getTaskList();
    const unsigned n_tasks = tasks.size();
    if ( n_tasks <= 1 || processor->isInfinite() || !__probability_cfs_processor() ) return;

    processor->setSchedulingType( SCHEDULE_CFS );

    /* Create groups */

    double remainder = 1.0;
    std::vector<LQIO::DOM::Group *> groups;
    do {
	double share = remainder *  __group_share();
	remainder -= share;
	groups.push_back( addGroup( processor, share ) );
    } while ( remainder > 0.2 && groups.size() + 1 < n_tasks );
    groups.push_back( addGroup( processor, remainder ) );

    /* Assign tasks to groups */

    unsigned int i = 0;
    const unsigned int n_groups = groups.size();
    for ( std::set<LQIO::DOM::Task *>::const_iterator t = tasks.begin(); t != tasks.end(); ++t ) {
	LQIO::DOM::Task * task = *t;
	LQIO::DOM::Group * group = groups.at(i);
	group->addTask( task );
	task->setGroup( group );
	i = (i + 1) % n_groups;
    }
}

/*
 * Translate all constant parameters in the DOM to variables, then output control program.
 * This works on the DOM regardless of whether it was generated or loaded.
 * LQX code is generated directly as a "program", rather than printing out the LQX through SPEX.
 * It's less verbose this way, and SPEX generation doesn't construct the LQX program.
 */

Generate&
Generate::reparameterize()
{
    if ( Flags::reset_pragmas ) {
	_document->clearPragmaList();
    }
    for ( std::map<std::string,std::string>::const_iterator pragma = __pragma.begin(); pragma != __pragma.end(); ++pragma ) {
	_document->addPragma( pragma->first, pragma->second );
    }

    /*
     * Distribute customers among clients 
     */
    
    if ( _number_of_customers != 0 ) {
	populate();
    }

    /*
     * Heavy lifting - swap parameters.
     */
    
    switch ( _output_format ) {
    case LQIO::DOM::Document::InputFormat::AUTOMATIC:
    case LQIO::DOM::Document::InputFormat::LQN:
	addSpex( &Generate::makeVariables, getNumberOfRuns() > 1 ? &ModelVariable::spex_random : &ModelVariable::spex_scalar );
	break;

    case LQIO::DOM::Document::InputFormat::XML:
	if ( Flags::spex_output ) {
	    addSpex( &Generate::makeVariables, getNumberOfRuns() > 1 ? &ModelVariable::spex_random : &ModelVariable::spex_scalar );
	} else if ( Flags::sensitivity > 0.0 ) {
	    addSensitivityLQX( &Generate::makeVariables, &ModelVariable::lqx_sensitivity );
	} else {
	    addLQX( &Generate::makeVariables, getNumberOfRuns() > 1 ? &ModelVariable::lqx_random : &ModelVariable::lqx_scalar );
	}
	break;
    }

    /*
     * Final steps
     */
    
    if ( !Flags::spex_output ) {
        _document->registerExternalSymbolsWithProgram( 0 );	/* Suppresses LQX output */
    }
    /* ---- */

    std::ostringstream comment;
    if ( __comment.size() == 0 ) {
	comment << "Layers: " << _number_of_layers
		<< ", Customers: " << (_number_of_customers > 0 ? _number_of_customers : for_each( _task[REF_LAYER].begin(), _task[REF_LAYER].end(), AccumulateCustomers()).mean())
		<< ", Clients: " << _number_of_clients
		<< ", Tasks: " << _number_of_tasks
		<< ", (Delay: " << for_each( ++(_task.begin()), _task.end(), AccumulateDelayServer() ).mean() << ")"
		<< ", Processors: " << _number_of_processors;
    }

    _document->setModelParameters( __comment.size() ? __comment : comment.str(),
				   new LQIO::DOM::ConstantExternalVariable( __convergence_value ),
				   new LQIO::DOM::ConstantExternalVariable( __iteration_limit ),
				   new LQIO::DOM::ConstantExternalVariable( __print_interval ),
				   new LQIO::DOM::ConstantExternalVariable( __underrelaxation ), 0 );
    if ( Flags::verbose ) {
	printStatistics( std::cerr );
    }

    return *this;
}


/*
 * Set the populations of the reference tasks by distributing the total customers.
 */

void
Generate::populate()
{
    Flags::override[Flags::CUSTOMERS] = false;		/* Use DOM value as we set that */

    const std::vector<LQIO::DOM::Task *>& task = _task[REF_LAYER];
    const unsigned int n_clients = task.size();
    std::vector<unsigned int> customers( n_clients );
    unsigned int count = 0;
    for ( unsigned int i = 0; i < n_clients; ++i ) {
	customers[i] = 1;
	count += 1;
    }
    for ( ; count < _number_of_customers; ++count ) {
	customers[static_cast<unsigned int>(RV::RandomVariable::number() * n_clients)] += 1;
    }
    for ( unsigned int i = 0; i < n_clients; ++i ) {
	task[i]->setCopiesValue( customers[i] );
    }
}



/*
 * MakeVariables is called to convert a constant into a variable in the DOM.  The mean
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
    LQIO::Spex::clear_input_variables();		/* Force LQX output */
    __random_variables.clear();				/* Result result variables */
    (this->*f)( g );					/* Make and initialize Variables */

    const unsigned n = getNumberOfRuns();
    if ( n > 1 ) {
	_program << print_header( getDOM(), 1, "i" ) << std::endl;
	_program << indent( 1 ) << "for ( i = 0; i < " << n << "; i = i + 1 ) {" << std::endl;
	(this->*f)( &ModelVariable::lqx_loop_body );	/* Get and set loop variables */
	_program << indent( 2 ) << "solve();" << std::endl;
	_program << print_results( getDOM(), 2, "i" ) << std::endl;
	_program << indent( 1 ) << "}" << std::endl;
    } else {
	_program << print_header( getDOM(), 1 ) << std::endl;
	_program << indent( 1 ) << "solve();" << std::endl;
	_program << print_results( getDOM(), 1 ) << std::endl;
    }

    _document->setLQXProgramText( _program.str() );
}


void
Generate::addSensitivityLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    (this->*f)( g );		/* Should be the vector generator */

    _program << print_header( getDOM(), 1 ) << std::endl;
    const std::map<std::string,LQIO::DOM::Entry*>& entries = _document->getEntries();	
    forEach( entries.begin(), entries.end(), 1 );
    _document->setLQXProgramText( _program.str() );
}


/* static */ std::ostream&
Generate::printHeader( std::ostream& output, const LQIO::DOM::Document& document, const int i, const std::string& prefix )
{
    if ( !Flags::has_any_observation() ) return output;

    output << indent(i) << "println_spaced( \", ";
    if ( prefix.size() ) {
	output << "\", \"" << prefix;		/* The iteration number */
    }
    if ( Flags::observe[Flags::PARAMETERS] ) {
	for_each( __random_variables.begin(), __random_variables.end(), ParameterHeading( output, i + 2 ) );
    }

    std::for_each( &document_observation[0], &document_observation[5], DocumentHeading( output, i + 2 ) );

    /* Make the result variables */
    const std::map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    for_each( entities.begin(), entities.end(), EntityHeading( output, i + 2 ) );

    /* Entry stats here */
    const std::map<std::string,LQIO::DOM::Entry*>& entries = document.getEntries();
    for_each( entries.begin(), entries.end(), EntryHeading( output, i + 2 ) );

    output << "\" );";
    return output;
}


/* static */ std::ostream&
Generate::printResults( std::ostream& output, const LQIO::DOM::Document& document, const int i, const std::string& prefix )
{
    if ( !Flags::has_any_observation() ) return output;

    output << indent(i) << "println_spaced( \", \"";
    if ( prefix.size() ) {
	output  << ", " << prefix;
    }

    if ( Flags::observe[Flags::PARAMETERS] ) {
        std::for_each( __random_variables.begin(), __random_variables.end(), ParameterResult( output, i + 2 ) );
    }

    std::for_each( &document_observation[0], &document_observation[5], DocumentResult( output, i + 2 ) );

    /* Make the result variables */
    const std::map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    std::for_each( entities.begin(), entities.end(), EntityResult( output, i + 2 ) );

    /* Entry stats here */
    const std::map<std::string,LQIO::DOM::Entry*>& entries = document.getEntries();
    std::for_each( entries.begin(), entries.end(), EntryResult( output, i + 2 ) );

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
	_program << indent( i ) << "solve();" << std::endl;
	_program << print_results( getDOM(), i ) << std::endl;
    } else {
	const LQIO::DOM::Entry * entry = e->second;
	if ( isReferenceTask( entry->getTask() ) ) {
	    forEach( ++e, end, i );			/* Ignore reference tasks */
	} else {
	    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
	    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
		LQIO::DOM::Phase* phase = p->second;
		const std::string name = std::string( "$s_" ) + phase->getName();
		_program << indent( i ) << "foreach( " << name << " in " << &(name.c_str())[1] << " ) {" << std::endl;
		forEach( ++e, end, i+1 );
		_program << indent( i ) << "}" << std::endl;
	    }
	}
    }
}

void
Generate::ModelVariable::lqx_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) return;		// Ignore.

    _model._program << indent( 1 ) << var.getName() << " = ";

    /* Look for 'var' in SPEX input variables.  If it's found, then output the expression, otherwise, compose a new one. */
    LQX::SyntaxTreeNode * node = LQIO::Spex::get_input_var_expr( var.getName() );
    if ( node ) {
	node->print( _model._program, 0 );
    } else {
	_model._program << (*value)();
    }
    _model._program << ";" << std::endl;
}


/*
 * Generate random values inside the `for' loop.
 */

void
Generate::ModelVariable::lqx_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.getName().size() == 0 ) return;
    if ( value->getType() == RV::RandomVariable::CONSTANT ) {
	_model._program << indent( 1 ) << var.getName() << " = " << (*value)() << ";" << std::endl;
    } else if ( Flags::observe[Flags::PARAMETERS] ) {
	/* Only needed for outputting values in printlns for Flags::show_variables */
	__random_variables.push_back( var.getName() );
    }
}

void
Generate::ModelVariable::lqx_sensitivity( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.getName().size() == 0 ) return;
    double val = 0;
    if ( var.wasSet() ) {
	var.getValue( val );
    } else {
	val = (*value)();
    }
    const std::string& buf = var.getName();
    double delta = (val * Flags::sensitivity) - val;
    _model._program << indent( 1 ) << &buf[1] << " = [ "
		    << val-delta << ", "
		    << val << ", "
		    << val+delta << " ];" << std::endl;

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
    if ( !var.wasSet() && value->getType() != RV::RandomVariable::CONSTANT ) {
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
	    _model._program << " ) + 1;" << std::endl;
	    delete clone;
	} else {
	    _model._program << "( ";
	    for ( unsigned int i = 1; i <= value->nArgs(); ++i ) {
		if ( i > 1 ) _model._program << ", ";
		_model._program << value->getArg(i);
	    }
	    _model._program << " );" << std::endl;	
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
    _output << "\"," << std::endl << indent( _i ) << "  " << "\"" << s;
}

void
Generate::DocumentHeading::operator()( struct document_observation& obs ) const
{
    if ( Flags::observe[obs.flag] ) {
	_output << "\"," << std::endl << indent( _i ) << "  " << "\"" << obs.lqx_heading;
    }
}

void
Generate::EntityHeading::operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& e ) const
{
    const LQIO::DOM::Entity * entity = e.second;
    if ( isInterestingProcessor( entity ) && Flags::observe[Flags::UTILIZATION] ) {
        _output << "\", " << std::endl << indent( _i ) << "  "
		<< "\"p(" << entity->getName() << ").util";
    } else if ( isReferenceTask( entity ) && Flags::observe[Flags::THROUGHPUT] ) {
        _output << "\", " << std::endl << indent( _i ) << "  "
		<< "\"t(" << entity->getName() << ").tput";
    } else if ( isServerTask( entity ) && Flags::observe[Flags::UTILIZATION] ) {
        _output << "\", " << std::endl << indent( _i ) << "  "
		<< "\"t(" << entity->getName() << ").util";
    }
}

/* Entry stats here */
void
Generate::EntryHeading::operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second;
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    std::for_each( phases.begin(), phases.end(), PhaseHeading( _output, _i, entry ) );
}

void
Generate::PhaseHeading::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    const LQIO::DOM::Phase * phase = p.second;
    if ( Flags::observe[Flags::RESIDENCE_TIME] ) {
	_output << "\", " << std::endl << indent( _i ) << "  "
	        << "\"e(" << _entry->getName() << "," << p.first << ").serv";
    }
    if ( Flags::observe[Flags::QUEUEING_TIME] ) {
	const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
	std::for_each( calls.begin(), calls.end(), CallHeading( _output, _i, _entry, p.first ) );
    }
}

void
Generate::CallHeading::operator()( const LQIO::DOM::Call * call ) const
{
    const LQIO::DOM::Entry * dst = call->getDestinationEntry();
    _output << "\", " << std::endl << indent( _i ) << "  "
	    << "\"y(" << _entry->getName() << "," << _phase << "," << dst->getName() << ").wait";
}

void
Generate::ParameterResult::operator()( std::string& s ) const
{
    _output << "," << std::endl << indent( _i ) << "  " << s;
}

void
Generate::DocumentResult::operator()( struct document_observation& obs ) const
{
    if ( Flags::observe[obs.flag] ) {
	_output << "," << std::endl << indent( _i ) << "  " << "document()." << obs.lqx_attribute;
    }
}


void
Generate::EntityResult::operator()( const std::pair<unsigned,LQIO::DOM::Entity *>& e ) const
{
    const LQIO::DOM::Entity * entity = e.second;
    if ( isInterestingProcessor( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	_output << "," << std::endl << indent( _i ) << "  "
		<< "processor(\"" << entity->getName() << "\").utilization";
    } else if ( isReferenceTask( entity ) && Flags::observe[Flags::THROUGHPUT] ) {		/* Reference task */
	_output << "," << std::endl << indent( _i ) << "  "
		<< "task(\"" << entity->getName() << "\").throughput";
    } else if ( isServerTask( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	_output << "," << std::endl << indent( _i ) << "  "
		<< "task(\"" << entity->getName() << "\").utilization";
    }
}

/* Entry stats here */
void
Generate::EntryResult::operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second;
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    std::for_each( phases.begin(), phases.end(), PhaseResult( _output, _i, entry ) );
}

void
Generate::PhaseResult::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    const LQIO::DOM::Phase * phase = p.second;
    if ( Flags::observe[Flags::RESIDENCE_TIME] ) {
	_output << "," << std::endl << indent( _i ) << "  "
	       << "phase(entry(\"" << _entry->getName() << "\")," << p.first << ").service_time";
    }
    if ( Flags::observe[Flags::QUEUEING_TIME] ) {
	const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
	std::for_each( calls.begin(), calls.end(), CallResult( _output, _i, _entry, p.first ) );
    }
}

void
Generate::CallResult::operator()( const LQIO::DOM::Call * call ) const
{
    const LQIO::DOM::Entry * dst = call->getDestinationEntry();
    _output << "," << std::endl << indent( _i ) << "  "
	    << "call(phase(entry(\"" << _entry->getName() << "\")," << _phase << "),\"" << dst->getName() << "\").waiting";
}

/* static */ std::ostream&
Generate::printIndent( std::ostream& output, const int i )
{
    output << std::setw( i * 3 ) << " ";
    return output;
}

/* ------------------ Logic for creating an SPEX program ------------------ */

/*
 * Generate SPEX.  Note that the program isn't saved at this time.
 * The LQX for the parameters and observation variables will have to be saved
 * then a call to Spex::construct_program( expr_list * main_line, expr_list * result, expr_list * convergence )
 * will have to be made.
 */

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

	std::for_each( &document_observation[0], &document_observation[5], documentObservation );

	/* Make the result variables */
	const std::map<unsigned,LQIO::DOM::Entity *>& entities = _document->getEntities();
	std::for_each( entities.begin(), entities.end(), EntityObservation() );

	/* Entry stats here */
	const std::map<std::string,LQIO::DOM::Entry*>& entries = _document->getEntries();
	std::for_each( entries.begin(), entries.end(), EntryObservation() );
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

	if ( !LQIO::Spex::has_input_var( name ) ) {	/* If set, previously, don't set again */
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
		expr = new LQX::MathExpression(LQX::MathOperation::ADD, new LQX::MethodInvocationExpression( clone->name(), args ), new LQX::ConstantValueExpression(1.0) );
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
    if ( !LQIO::Spex::has_input_var( var.getName() ) ) {	/* If set, previously, don't set again */
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
    std::pair<LQIO::Spex::obs_var_tab_t::const_iterator, LQIO::Spex::obs_var_tab_t::const_iterator> range = LQIO::Spex::observations().equal_range( entity );
    if ( isInterestingProcessor( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	if ( count_if( range.first, range.second, HasKey( KEY_UTILIZATION ) ) == 0 ) {
	    std::string name = "$p_";
	    name += entity->getName();
	    spex_processor_observation( entity, KEY_UTILIZATION, 0, name.c_str(), 0 );
	    spex_result_assignment_statement( name.c_str(), 0 );
	}
    } else if ( isReferenceTask( entity ) && Flags::observe[Flags::THROUGHPUT] ) {		/* Reference task */
	if ( count_if( range.first, range.second, HasKey( KEY_THROUGHPUT ) ) == 0 ) {
	    std::string name = "$f_";
	    name += entity->getName();
	    spex_task_observation( entity, KEY_THROUGHPUT, 0, 0, name.c_str(), 0 );
	    spex_result_assignment_statement( name.c_str(), 0 );
	}
    } else if ( isServerTask( entity ) && Flags::observe[Flags::UTILIZATION] ) {
	if ( count_if( range.first, range.second, HasKey( KEY_UTILIZATION ) ) == 0 ) {
	    std::string name = "$u_";
	    name += entity->getName();
	    spex_task_observation( entity, KEY_UTILIZATION, 0, 0, name.c_str(), 0 );
	    spex_result_assignment_statement( name.c_str(), 0 );
	}
    }
}

void
Generate::EntryObservation::operator()( const std::pair<std::string,LQIO::DOM::Entry*>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second;
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    std::pair<LQIO::Spex::obs_var_tab_t::const_iterator, LQIO::Spex::obs_var_tab_t::const_iterator> range = LQIO::Spex::observations().equal_range( entry );
    if ( Flags::observe[Flags::RESIDENCE_TIME] && count_if( range.first, range.second, HasKey( KEY_SERVICE_TIME ) ) == 0 ) {
	std::for_each( phases.begin(), phases.end(), PhaseObservation( entry ) );
    }
    if ( Flags::observe[Flags::QUEUEING_TIME] && count_if( range.first, range.second, HasKey( KEY_WAITING ) ) == 0 ) {
	std::for_each( phases.begin(), phases.end(), PhaseCallObservation( entry ) );
    }
}

void
Generate::PhaseObservation::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    const LQIO::DOM::Phase * phase = p.second;
    std::string name = "$r_";
    name += phase->getName();
    spex_entry_observation( _entry, KEY_SERVICE_TIME, p.first, 0, name.c_str(), 0 );
    spex_result_assignment_statement( name.c_str(), 0 );

}

void
Generate::PhaseCallObservation::operator()( const std::pair<unsigned, LQIO::DOM::Phase*>& p ) const
{
    const std::vector<LQIO::DOM::Call*>& calls = p.second->getCalls();
    std::for_each( calls.begin(), calls.end(), CallObservation( _entry, p.first ) );
}

void
Generate::CallObservation::operator()( const LQIO::DOM::Call * call ) const
{
    std::string name = "$w_";
    name += call->getName();
    spex_call_observation( _entry, KEY_WAITING, _phase, call->getDestinationEntry(), 0, name.c_str(), 0 );
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
    if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) &&  __probability_delay_server() != 0 ) {
	/* Any processor with a constant multiplicity is eligible for conversion into an infinite server */
	/* Processors with multiplicities set using a variable cannot be changed since the variable */
	/* would end up being unused, causing an exception at run-time. */
	processor->setCopiesValue( 1 );
	processor->setSchedulingType( SCHEDULE_DELAY );
    } else {
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
	if ( copies ) (this->*_f)( *copies, multiplicity ); 	/* Insert spex/lqx assignment code for variable */
    }
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
	if ( copies ) (this->*_f)( *copies, customers );  /* Inserts spex/lqx assignment code for variable */
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
	if ( copies ) (this->*_f)( *copies, multiplicity ); 	/* Inserts spex/lqx assignment code for variable */
    }

    /* Activities */

    const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
    std::for_each( activities.begin(), activities.end(), ActivityVariable( _model, _f ) );
}



void
Generate::EntryVariable::operator()( const std::pair<std::string,LQIO::DOM::Entry *>& e ) const
{
    const LQIO::DOM::Entry * entry = e.second;
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    std::for_each( phases.begin(), phases.end(), PhaseVariable( _model, _f ) );
}


void
Generate::PhaseVariable::transform( LQIO::DOM::Phase * phase ) const
{
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
    if ( service ) (this->*_f)( *service, service_time ); 	/* Inserts spex/lqx assignment code for variable */

    if ( Flags::sensitivity == 0.0 ) {
	const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
	std::for_each( calls.begin(), calls.end(), CallVariable( _model, _f ) );
    }
}



void
Generate::CallVariable::operator()( LQIO::DOM::Call * call ) const
{
    /* check for var... */
    RV::RandomVariable * rate;
    switch ( call->getCallType() ) {
    case LQIO::DOM::Call::Type::RENDEZVOUS:    rate = const_cast<RV::RandomVariable *>(__rendezvous_rate); break;
    case LQIO::DOM::Call::Type::SEND_NO_REPLY: rate = const_cast<RV::RandomVariable *>(__send_no_reply_rate); break;
    case LQIO::DOM::Call::Type::FORWARD:       rate = const_cast<RV::RandomVariable *>(__forwarding_probability); break;
    default: abort();
    }

    LQIO::DOM::ExternalVariable * calls = const_cast<LQIO::DOM::ExternalVariable *>( call->getCallMean() );
    if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>(calls) && Flags::convert[Flags::REQUEST_RATE] ) {
	if ( !Flags::override[Flags::REQUEST_RATE] ) {
	    rate = rate->clone();
	    rate->setMean( to_double( *calls ) );
	}
	switch ( call->getCallType() ) {
	case LQIO::DOM::Call::Type::RENDEZVOUS:    calls = get_rv( "$y_", call->getName(), rate ); break;
	case LQIO::DOM::Call::Type::SEND_NO_REPLY: calls = get_rv( "$z_", call->getName(), rate ); break;
	case LQIO::DOM::Call::Type::FORWARD:       calls = get_rv( "$f_", call->getName(), rate ); break;
	}
	call->setCallMean( calls );
    }
    if ( calls ) (this->*_f)( *calls, rate ); 		/* Inserts spex/lqx assignment code for variable */
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
    if ( Flags::spex_output ) {
	return _model._document->getSymbolExternalVariable( var );
    } else {
	return new LQIO::DOM::ConstantExternalVariable( (*value)() );
    }
}

/* ------------------------------------------------------------------------ */

/*
 * Output an input file.
 */

std::ostream&
Generate::print( std::ostream& output ) const
{
    switch ( _output_format ) {
    case LQIO::DOM::Document::InputFormat::AUTOMATIC:
    case LQIO::DOM::Document::InputFormat::LQN: {
	LQIO::SRVN::Input srvn( getDOM(), _document->getEntities(), Flags::annotate_input );
	srvn.print( output );
	break;
    }
    case LQIO::DOM::Document::InputFormat::XML:
	_document->print( output, LQIO::DOM::Document::OutputFormat::XML );	/* Output LQX code if running. */
	break;
    case LQIO::DOM::Document::InputFormat::JSON:
	_document->print( output, LQIO::DOM::Document::OutputFormat::JSON );	/* Output LQX code if running. */
	break;
    }

    return output;
}



std::ostream&
Generate::printStatistics( std::ostream& output ) const
{
    output << "File Name: " << LQIO::DOM::Document::__input_file_name << std::endl;

    output << "    Number of server layers: " << _task.size() - 1 << std::endl;

    const double n_clients = static_cast<double>(_task[REF_LAYER].size());
    output << "    Number of clients: " << n_clients << std::endl;
    output << "    Mean number of customers per client: " << std::for_each( _task[REF_LAYER].begin(), _task[REF_LAYER].end(), AccumulateCustomers()).mean() << std::endl;
    output << "    Mean client think time: " << std::for_each( _entry.begin(), _entry.end(), AccumulateThinkTime()).mean() << std::endl;

    output << "    Number of processors: " << _processor.size() << std::endl;
    output << "    Mean processor multiplicity: " << std::for_each( _processor.begin(), _processor.end(), AccumulateMultiplicity()).mean() << std::endl;

    AccumulateMultiplicity threads = std::for_each(_task.begin(), _task.end(), AccumulateMultiplicity() );		/* Arg is pass by value, so assignment is needed */
    const double n_servers = static_cast<double>(threads.count());
    output << "    Number of server tasks: " << threads.count() << std::endl;
    output << "    Mean server task multiplicity: " << threads.mean() << std::endl;

    const double n_entries = static_cast<double>(_entry.size()) - n_clients;		/* Server entries only */
    output << "    Number of server entries: " << n_entries << std::endl;
    output << "    Mean number of entries at server tasks: " << n_entries / n_servers << std::endl;

    AccumulateServiceTime service_time = std::for_each(_entry.begin(), _entry.end(), AccumulateServiceTime() );	/* Arg is pass by value, so assignment is needed */
    const double n_phases = static_cast<double>(service_time.count());	 		/* Server entries only */
    output << "    Mean number of phases per server entry: " << n_phases / n_entries << std::endl;
    output << "    Mean phase service time: " << service_time.mean() << std::endl;


    AccumulateRequests requests = std::for_each( _call.begin(), _call.end(), AccumulateRequests() );			/* Arg is pass by value, so assignment is needed */
    const double n_calls = static_cast<double>( requests.count() );
    output << "    Number of calls: " << n_calls << std::endl;
    output << "    Mean fan-in per entry: " << n_calls / n_entries << std::endl;
    output << "    Mean fan-out per phase: " << n_calls / (n_phases + n_clients) << std::endl;
    output << "    Mean request rate per call: " << requests.mean() << std::endl;
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
Generate::Accumulate::operator+=( double augend )
{
    _sum         += augend;
    _sum_squared += augend * augend;
    _n           += 1;
    return *this;
}

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
	const LQIO::DOM::ExternalVariable * copies = task->getCopies();
	if ( copies != NULL ) {
	    (*this) += copies;
	} else {
	    (*this) += 1.0;
	}
    }
}

void
Generate::AccumulateMultiplicity::operator()( const LQIO::DOM::Processor * processor )
{
    const LQIO::DOM::ExternalVariable * copies = processor->getCopies();
    if ( copies != NULL ) {
	(*this) += processor->getCopies();
    } else {
	(*this) += 1;
    }
}


void
Generate::AccumulateMultiplicity::operator()( const std::vector<LQIO::DOM::Task *>& tasks )
{
    (*this) += std::for_each( tasks.begin(), tasks.end(), AccumulateMultiplicity() );
}

void
Generate::AccumulateMultiplicity::operator()( const LQIO::DOM::Task * task )
{
    if ( isServerTask( task ) ) {
	const LQIO::DOM::ExternalVariable * copies = task->getCopies();
	if ( copies != NULL ) {
	    (*this) += copies;
	} else {
	    (*this) += 1;
	}
    }
}


void
Generate::AccumulateDelayServer::operator()( const std::vector<LQIO::DOM::Task *>& tasks )
{
    (*this) += std::for_each( tasks.begin(), tasks.end(), AccumulateDelayServer() );
}


void
Generate::AccumulateDelayServer::operator()( const LQIO::DOM::Task * task )
{
    if ( task->isInfinite() ) {
	(*this) += 1;
    }
}


void
Generate::AccumulateServiceTime::operator()( const LQIO::DOM::Entry * entry )
{
    if ( isServerTask( entry->getTask() ) ) {
	(*this) += std::for_each( entry->getPhaseList().begin(), entry->getPhaseList().end(), Generate::AccumulateServiceTime() );
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
	(*this) += std::for_each( entry->getPhaseList().begin(), entry->getPhaseList().end(), Generate::AccumulateThinkTime() );	/* Should only be one... */
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

