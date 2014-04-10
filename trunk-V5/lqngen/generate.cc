/* lqngen.cc    -- Greg Franks Thu Jul 29 2004
 * Model file generator.
 *
 * $Id$
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
#include <lqio/expat_document.h>
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
const char * Generate::__comment 	= "";

static const unsigned int REF_LAYER = 0;
static const unsigned int SERVER_LAYER = 1;


LQIO::DOM::ConstantExternalVariable * Generate::ONE = 0;

unsigned int Generate::__number_of_clients	= 1;
unsigned int Generate::__number_of_processors	= 1;
unsigned int Generate::__number_of_tasks	= 1;
unsigned int Generate::__number_of_layers	= 1;
Generate::layering_t Generate::__layering_type	= Generate::DETERMINISTIC_LAYERING;

RV::RandomVariable * Generate::__service_time			= 0;
RV::RandomVariable * Generate::__think_time			= 0;
RV::RandomVariable * Generate::__forwarding_probability		= 0;
RV::RandomVariable * Generate::__rendezvous_rate		= 0;
RV::RandomVariable * Generate::__send_no_reply_rate		= 0;
RV::RandomVariable * Generate::__customers_per_client		= 0;
RV::RandomVariable * Generate::__task_multiplicity		= 0;
RV::RandomVariable * Generate::__processor_multiplicity		= 0;
RV::RandomVariable * Generate::__probability_second_phase	= 0;
RV::RandomVariable * Generate::__probability_infinite_server	= 0;
RV::RandomVariable * Generate::__number_of_entries		= 0;

/*
 * Initialize probabilities.
 */

Generate::Generate( LQIO::DOM::Document * document, const unsigned runs ) 
    : _document(document), _runs(runs), _number_of_layers(0)
{
    if ( !ONE )	ONE = new LQIO::DOM::ConstantExternalVariable( 1.0 );
}


Generate::Generate( const unsigned layers, const unsigned runs ) 
    : _document(0), _runs(runs), _number_of_layers(layers)
{
    if ( !ONE ) ONE = new LQIO::DOM::ConstantExternalVariable( 1.0 );
    _document = new LQIO::DOM::Document( &io_vars, LQIO::DOM::Document::AUTOMATIC_INPUT );		// For XML output.
    _layer_CDF.resize( _number_of_layers + 1 );
    _task.resize( _number_of_layers + 1 );
    _entry.resize( _number_of_layers + 1 );
    _layer_CDF[0] = 0;
}


Generate::~Generate()
{
    delete _document;
    _document = 0;
}

/*
 * Generate a model.
 */

Generate&
Generate::operator()()
{
    if ( _processor.size() > 0 ) return *this;

    makeLayerCDF();

    std::vector<unsigned int> stride(  _number_of_layers + 1 );
    const unsigned int width = static_cast<unsigned int>( ceil( log10( __number_of_tasks ) ) );

    /* Create all clients, and give them their own processor */
    for ( unsigned int i = 0; i < __number_of_clients; ++i ) {
	std::ostringstream name;
	name << "c" << setw( width ) << setfill( '0' ) << i;
	LQIO::DOM::Processor * proc = addProcessor( name.str(), SCHEDULE_DELAY );		// Hide this one.
	vector<LQIO::DOM::Entry *> entries;
	LQIO::DOM::Entry * entry = addEntry( name.str() );
	_entry[REF_LAYER].push_back( entry );
	entries.push_back( entry );
	_task[REF_LAYER].push_back( addTask( name.str(), SCHEDULE_CUSTOMER, entries, proc ) );	// Get RV for number of customers.
    }

    /* Create all processors */
    for ( unsigned int i = 0; i < __number_of_processors; ++i ) {
	std::ostringstream name;
	name << "p" << i;
	LQIO::DOM::Processor * proc = addProcessor( name.str(), SCHEDULE_PS );
	_processor.push_back( proc );
    }

    /* Create and distribute all tasks,  connect to processors */

    for ( unsigned int i = 0; i < __number_of_tasks; ++i ) {
	std::ostringstream entry_name;
	std::ostringstream task_name;
	entry_name << "e" << setw( width ) << setfill( '0' ) << i;
	task_name  << "t" << setw( width ) << setfill( '0' ) << i;

	const unsigned int layer = ( i < _number_of_layers ? (i + 1) : getLayer( i ) );		/* Always put one task in every layer. */
	vector<LQIO::DOM::Entry *> entries;
	LQIO::DOM::Entry * entry = addEntry( entry_name.str(), __probability_second_phase );
	_entry[layer].push_back( entry );
	entries.push_back( entry );
	unsigned int p = ( i <  __number_of_processors ? i : static_cast<unsigned int>(floor( drand48() * __number_of_processors ) ) );
	const scheduling_type sched = ( __probability_infinite_server && (*__probability_infinite_server)() != 0.0 ) ? SCHEDULE_DELAY : SCHEDULE_FIFO;
	_task[layer].push_back( addTask( task_name.str(), sched, entries, _processor[p] ) );
    }

    /* Set stride so that we can route calls to other layers */

    unsigned int n_tasks = 0;
    for ( unsigned int i = 0; i <= _number_of_layers; ++i ) {
	n_tasks += _task[i].size();
	stride[i] = n_tasks;
    }

    /* Connect all servers to parents */

    assert( _entry[REF_LAYER].size() == __number_of_clients );

    for ( unsigned int j = SERVER_LAYER; j <= _number_of_layers; ++j ) {
	unsigned int i = 0;
	const unsigned int number_of_clients = _entry[j-1].size();
	for ( std::vector<LQIO::DOM::Entry *>::const_iterator dst = _entry[j].begin(); dst != _entry[j].end(); ++dst ) {
	    LQIO::DOM::Entry * src = _entry[j-1][i];
	    _call.push_back( addCall( src, *dst, __probability_second_phase ) );
	    i = (i + 1) % number_of_clients;		// too few clients handled here.
	}
    }

    /* Create extra entries on all tasks and move calls */

    if ( __number_of_entries ) {
	for ( unsigned int j = SERVER_LAYER; j <= _number_of_layers; ++j ) {
	    for ( std::vector<LQIO::DOM::Task *>::const_iterator task = _task[j].begin(); task != _task[j].end(); ++task ) {
		const unsigned n_entries = (*__number_of_entries)();
		vector<LQIO::DOM::Entry *>& entries = const_cast<vector<LQIO::DOM::Entry *>&>((*task)->getEntryList());
		const LQIO::DOM::Entry * entry = entries.front();
		for ( unsigned int i = 1; i < n_entries; ++i ) {
		    std::ostringstream name;
		    
		    name << entry->getName() << static_cast<char>((i-1)+'a');
		    LQIO::DOM::Entry * entry = addEntry( name.str(), __probability_second_phase );
		    entries.push_back(entry);

		    const unsigned int e = static_cast<unsigned int>(stride[j-1] * drand48());
		    unsigned int prev_stride = 0;
		    for ( unsigned int l = 0; l <= _number_of_layers; ++l ) {
			if ( e >= stride[l] ) {
			    prev_stride = stride[l];
			} else {
			    const unsigned int ee = e-prev_stride;
			    assert( ee < _entry[l].size() );
			    _call.push_back( addCall( _entry[l][ee], entry, __probability_second_phase ) );
			    break;
			}
		    }
		}
	    }
	}
    }

    /* If I have too many clients for the first server layer, randomly connect extra clients to ANY server */

    const unsigned int n_clients = _task[REF_LAYER].size();
    const unsigned int n_servers = _task[SERVER_LAYER].size();
    if ( n_clients > n_servers ) {
	for ( unsigned int i = n_servers; i < n_clients; ++i ) {
	    const std::vector<LQIO::DOM::Entry*>& list = _task[REF_LAYER][i]->getEntryList();
	    const unsigned int e = static_cast<unsigned int>(floor( static_cast<double>(n_tasks-stride[0]) * drand48() ) ) + stride[0];	/* Skip ref task */
	    unsigned int prev_stride = 0;
	    for ( unsigned int l = 0; l <= _number_of_layers; ++l ) {
		if ( e >= stride[l] ) {
		    prev_stride = stride[l];
		} else {
		    unsigned int ee = e-prev_stride;
		    assert( ee < _entry[l].size() );
		    _call.push_back( addCall( list[0], _entry[l][ee] ) );
		    break;
		}
	    }
	}
    }

    /* ---- */

    if ( Flags::spex_output ) {
	addSpex( &Generate::set_variables, getNumberOfRuns() > 1 ? &ModelVariable::spex_random : &ModelVariable::spex_scalar );
    } else if ( Flags::xml_output && Flags::lqx_output ) {
	addLQX( &Generate::set_variables, getNumberOfRuns() > 1 ? &ModelVariable::lqx_random : &ModelVariable::lqx_scalar );
    }
    _document->setModelParameters( __comment, new LQIO::DOM::ConstantExternalVariable( __convergence_value ), 
				   new LQIO::DOM::ConstantExternalVariable( __iteration_limit ), 
				   new LQIO::DOM::ConstantExternalVariable( __print_interval ), 
				   new LQIO::DOM::ConstantExternalVariable( __underrelaxation ), 0 );
    
    return *this;
}


/*
 * Translate all constants to variables, then output control program.
 */

void
Generate::reparameterize()
{
    if ( Flags::sensitivity > 0.0 ) {
	if ( Flags::spex_output ) {
	    addSensitivitySPEX( &Generate::sensitivity_variables, &ModelVariable::spex_sensitivity );
	} else {
	    addSensitivityLQX( &Generate::sensitivity_variables, &ModelVariable::lqx_sensitivity );
	}
    } else {
	if ( Flags::spex_output ) {
	    addSpex( &Generate::make_variables, getNumberOfRuns() > 1 ? &ModelVariable::spex_random : &ModelVariable::spex_scalar );
	} else {
	    addLQX( &Generate::make_variables, getNumberOfRuns() > 1 ? &ModelVariable::lqx_random : &ModelVariable::lqx_scalar );
	}
    }
}

/* ------------------------------------------------------------------------ */

void
Generate::makeLayerCDF() 
{
    _layer_CDF[0] = 0;		/* Don't use the ref task layer */

    const layering_t layering_type = (__layering_type == RANDOM_LAYERING ? static_cast<layering_t>(ceil( drand48() * static_cast<double>(FAT_LAYERING-DETERMINISTIC_LAYERING) ) + DETERMINISTIC_LAYERING) 
				      : __layering_type);
    double sum = 0;

    switch ( layering_type ) {
    case DETERMINISTIC_LAYERING:
    case UNIFORM_LAYERING:
	for ( unsigned int i = SERVER_LAYER; i <= _number_of_layers; ++i ) {
	    _layer_CDF[i] = static_cast<double>(i);
	}
	sum = _layer_CDF[_number_of_layers];
	break;

    case PYRAMID_LAYERING:
	for ( unsigned int i = SERVER_LAYER; i <= _number_of_layers; ++i ) {
	    _layer_CDF[i] = _layer_CDF[i-1] + static_cast<double>(i);
	}
	sum = _layer_CDF[_number_of_layers];
	break;

    case FUNNEL_LAYERING:
	for ( unsigned int i = SERVER_LAYER; i <= _number_of_layers; ++i ) {
	    sum += sum + static_cast<double>(i);
	}
	_layer_CDF[_number_of_layers] = sum;
	for ( unsigned int i = _number_of_layers-1; i >= SERVER_LAYER ; --i ) {
	    _layer_CDF[i] = _layer_CDF[i+1] - (_number_of_layers-i);
	}
	break;

    case FAT_LAYERING: {
	const unsigned int median = static_cast<unsigned int>(_number_of_layers / 2.0 + 0.5);
	for ( unsigned int i = SERVER_LAYER; i <= _number_of_layers; ++i ) {
	    _layer_CDF[i] = _layer_CDF[i-1] + (i <= median ? static_cast<double>(i) : static_cast<double>(_number_of_layers - i + 1));
	}
	sum = _layer_CDF[_number_of_layers];
    } break;
	
    default:
	abort();
    }

    /* Normalize */

    for ( unsigned int i = SERVER_LAYER; i <= _number_of_layers; ++i ) {
	_layer_CDF[i] /= sum;
    }

    /* debug */
    // cerr << "Sum = " << sum << std::endl;
    // double prev = 0;
    // for ( unsigned int i = 0; i <= _number_of_layers; ++i ) {
    // 	cerr << i << ": " << _layer_CDF[i] << ", delta = " << (_layer_CDF[i] - prev) << std::endl;
    // 	prev = _layer_CDF[i];
    // }
}

unsigned int
Generate::getLayer( const unsigned int i ) const
{
    /* Get the default layer for the task (starting from 1 working down) */

    unsigned int layer = 0;
    if ( __layering_type == DETERMINISTIC_LAYERING ) {
	layer = (i % _number_of_layers) + 1;
    } else {
	layer = SERVER_LAYER;
	const double rv = drand48();	
	while ( _layer_CDF[layer] < rv && layer < _number_of_layers ) {
	    ++layer;
	}
//	cerr << "RV is: " << rv << ", Layer is: " << layer << std::endl;
    }
    return layer;
}


/*
 * Add a processor to layer.
 */

LQIO::DOM::Processor * 
Generate::addProcessor( const string& name, const scheduling_type sched_flag )
{
    LQIO::DOM::ExternalVariable * n_copies = 0;
    if ( sched_flag == SCHEDULE_DELAY ) {
	n_copies = ONE;
    } else {
	n_copies = get_rv( "$p_", name, __processor_multiplicity );
    }
    LQIO::DOM::Processor * processor = new LQIO::DOM::Processor( _document, name.c_str(), sched_flag, n_copies );
    processor->setRateValue( 1.0 );
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

    if ( sched_flag == SCHEDULE_CUSTOMER ) {
	task->setCopies( get_rv( "$c_", name, __customers_per_client ) );
	task->setThinkTime( get_rv( "$Z_", name, __think_time ) );
    } else if ( sched_flag != SCHEDULE_DELAY ) {
	task->setCopies( get_rv( "$m_", name, __task_multiplicity ) );
    }

    return task;
}


/*
 * Add an entry to a layer.
 */

LQIO::DOM::Entry *
Generate::addEntry( const string& name, const RV::RandomVariable * pr_2nd_phase )
{
    LQIO::DOM::Entry* entry = new LQIO::DOM::Entry( _document, name.c_str(), (void *)0 );
    _document->addEntry(entry);
    _document->db_check_set_entry( entry, name, LQIO::DOM::Entry::ENTRY_STANDARD );
    unsigned int n_phases = 1;
    if ( pr_2nd_phase && (*pr_2nd_phase)() != 0.0 ) {
	n_phases = 2;
    }
    for ( unsigned p = 1; p <= n_phases; ++p ) {
	LQIO::DOM::Phase* phase = entry->getPhase(p);
	string phase_name=name;
	phase_name += '_';
	phase_name += "_123"[p];
	phase->setName( phase_name );
	phase->setServiceTime( get_rv( "$s_", phase_name, __service_time ) );
    }

    return entry;
}


/*
 * Create a synch call from phase 1 of src to dst.
 */

LQIO::DOM::Call *
Generate::addCall( LQIO::DOM::Entry * src, LQIO::DOM::Entry * dst, const RV::RandomVariable * pr_2nd_phase )
{
    const unsigned n_phases = src->getMaximumPhase();
    LQIO::DOM::Phase * phase = 0;
    if ( n_phases > 1 && pr_2nd_phase && (*pr_2nd_phase)() != 0.0 ) {
	phase = src->getPhase( 2 ); 
    } else {
	phase = src->getPhase( 1 ); 	
    }

    string name = phase->getName();
    name += '_';
    name += dst->getName();

    LQIO::DOM::Call * call;
    call = new LQIO::DOM::Call( _document, LQIO::DOM::Call::RENDEZVOUS, phase, dst );
    call->setCallMean( get_rv( "$y_", name, __rendezvous_rate ) );
    phase->addCall( call );
    call->setName( name );

    return call;
}



void
Generate::addLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    (this->*f)( g );					/* Make Variables */

    const unsigned n = getNumberOfRuns();
    if ( n > 1 ) {
	_program << print_header( *_document, 1, "i" );
	_program << indent( 1 ) << "for ( i = 0; i < " << n << "; i = i + 1 ) {" << endl;
	(this->*f)( &ModelVariable::lqx_function );	/* Get Variables */
	_program << indent( 2 ) << "solve();" << endl;
	_program << print_results( *_document, 2, "i" );
	_program << indent( 1 ) << "}" << endl;
    } else {
	_program << print_header( *_document, 1 );
	_program << indent( 1 ) << "solve();" << endl;
	_program << print_results( *_document, 1 );
    }
    _document->setLQXProgramText( _program.str() );
}


void
Generate::addSensitivityLQX( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    (this->*f)( g );		/* Should be the vector generator */

    _program << print_header( *_document, 1 );
    const std::map<std::string,LQIO::DOM::Entry*>& entries = _document->getEntries();	
    forEach( entries.begin(), entries.end(), 1 );
    _document->setLQXProgramText( _program.str() );
}

void
Generate::forEach( std::map<std::string,LQIO::DOM::Entry*>::const_iterator e, const std::map<std::string,LQIO::DOM::Entry*>::const_iterator& end, const unsigned int i )
{
    if ( e == end ) {
	_program << indent( i ) << "solve();" << endl;
	_program << print_results( *_document, i );
    } else {
	const LQIO::DOM::Entry * entry = e->second;
	if ( isReferenceTask( entry->getTask() ) ) {
	    forEach( ++e, end, i );
	} else {
	    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
	    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
		LQIO::DOM::Phase* phase = p->second;
		std::string name = "$s_";
		name += phase->getName();
		const char * buf = name.c_str();
		_program << indent( i ) << "foreach( " << name << " in " << &buf[1] << " ) {" << endl;
		forEach( ++e, end, i+1 );
		_program << indent( i ) << "}" << endl;
	    }
	}
    }
}



void
Generate::addSpex( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    /* Insert all of the SPEX control code. */
    const unsigned n = getNumberOfRuns();
    if ( n > 1 ) {
	spex_array_comprehension( "$experiments", 1, n, 1 );
    }

    /* Make the variables in the model. */
    (this->*f)( g );

    spex_result_assignment_statement( "$0", 0 );		/* print out index first. */

    insertSPEXObservations();
}


void
Generate::addSensitivitySPEX( get_set_var_fptr f, const ModelVariable::variableValueFunc g )
{
    (this->*f)( g );		/* Should be the vector generator */

    insertSPEXObservations();
}

/*
 * Get either a value or a variable name.  The value is based on the
 * random variate class passed in.  If a variable is being created,
 * then a subsequent pass which produces the "code" will assign
 * values.
 */

LQIO::DOM::ExternalVariable * 
Generate::get_rv( const std::string& prefix, const std::string& name, const RV::RandomVariable * value ) const
{
    std::string var = prefix + name;
    if ( Flags::lqx_output || Flags::spex_output ) {
	return _document->getSymbolExternalVariable( var );
    } else {
	return new LQIO::DOM::ConstantExternalVariable( (*value)() );
    }
}


bool 
Generate::isReferenceTask( const LQIO::DOM::Task * task ) 
{
    if ( !task ) return false;
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
Generate::isInterestingProcessor( const LQIO::DOM::Processor * processor ) 
{
    if ( !processor ) return false;
    const std::set<LQIO::DOM::Task*>& tasks = processor->getTaskList();

    if ( tasks.size() == 0 ) return false;
    if ( tasks.size() > 1 ) return true;
    return !isReferenceTask( *tasks.begin() );
}



/* ------------------------------------------------------------------------ */

/*
 * This function will SET any variables if they are NOT already defined (as constants)
 */

void
Generate::set_variables( const ModelVariable::variableValueFunc f )
{
    SetProcessorVariable setProcessorVariable( *this, f, __processor_multiplicity );
    SetTaskVariable setTaskVariable( *this, f, __customers_per_client, __think_time, __task_multiplicity );
    SetEntryVariable setEntryVariable( *this, f, __service_time );

    const std::map<std::string,LQIO::DOM::Task*>& tasks = _document->getTasks();

    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator t = tasks.begin(); t != tasks.end(); ++t ) {
	LQIO::DOM::Task * task = t->second;
	setTaskVariable( task );
	if ( !isReferenceTask( task ) ) {
	    setProcessorVariable( const_cast<LQIO::DOM::Processor *>(task->getProcessor()) );
	}
	const std::vector<LQIO::DOM::Entry*>& entries = task->getEntryList();
	for ( std::vector<LQIO::DOM::Entry*>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	    LQIO::DOM::Entry* entry = *e;
	    setEntryVariable( entry );
	    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
	    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
		const LQIO::DOM::Phase* phase = p->second;
		const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
		for_each( calls.begin(), calls.end(), SetCallVariable( *this, f, __rendezvous_rate, __send_no_reply_rate, __forwarding_probability ) );
	    }
	}
    }
}


/* 
 * Need to "make variables" by clobbering existing values 
 */

void
Generate::make_variables( const ModelVariable::variableValueFunc f )
{
    MakeProcessorVariable makeProcessorVariable( *this, f, __processor_multiplicity );
    MakeTaskVariable makeTaskVariable( *this, f, __customers_per_client, __think_time, __task_multiplicity );
    MakeEntryVariable makeEntryVariable( *this, f, __service_time );

    const std::map<std::string,LQIO::DOM::Task*>& tasks = _document->getTasks();

    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator t = tasks.begin(); t != tasks.end(); ++t ) {
	LQIO::DOM::Task * task = t->second;
	makeTaskVariable( task );
	if ( !isReferenceTask( task ) ) {
	    makeProcessorVariable( const_cast<LQIO::DOM::Processor *>(task->getProcessor()) );
	}
	const std::vector<LQIO::DOM::Entry*>& entries = task->getEntryList();
	for ( std::vector<LQIO::DOM::Entry*>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	    LQIO::DOM::Entry* entry = *e;
	    makeEntryVariable( entry );
	    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
	    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
		const LQIO::DOM::Phase* phase = p->second;
		const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
		for_each( calls.begin(), calls.end(), MakeCallVariable( *this, f, __rendezvous_rate, __send_no_reply_rate, __forwarding_probability ) );
	    }
	}
    }
}


/*
 * Make service time variables 
 */

void
Generate::sensitivity_variables( const ModelVariable::variableValueFunc f )
{
   MakeEntryVariable makeEntryVariable( *this, f, __service_time );

   const std::map<std::string,LQIO::DOM::Entry*>& entries = _document->getEntries();
   for ( std::map<std::string,LQIO::DOM::Entry*>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
       LQIO::DOM::Entry * entry = e->second;
       if ( !isReferenceTask( entry->getTask() ) ) continue;
       makeEntryVariable( entry );
   }
}

/* 
 * Output an input file.
 */

ostream& 
Generate::print( ostream& output ) const
{
    if ( Flags::xml_output ) {
#if defined(XML_OUTPUT)
	_document->serializeDOM( output, false );	/* Output LQX code if running. */
#endif
    } else {
	LQIO::SRVN::Input srvn( *_document, _document->getEntities(), Flags::annotate_input, false );
	srvn.print( output );
    }

    return output;
}



/* static */ ostream&
Generate::printHeader( ostream& output, const LQIO::DOM::Document& document, const int i, const string& prefix )
{
    output << indent(i) << "println_spaced( \", ";
    if ( prefix.size() ) {
	output << "\", \"" << prefix;		/* The iteration number */
    }
    if ( Flags::observe.iterations ) {
	output << "\"," << endl << indent( i + 2 ) << "  "
	       << "\"iterations";
    }
    if ( Flags::observe.mva_waits ) {
	output << "\", " << endl << indent( i + 2 ) << "  "
	       << "\"waits";
    }
    if ( Flags::observe.user_time ) {
	output << "\", " << endl << indent( i + 2 ) << "  "
	       << "\"usr cpu";
    }
    if ( Flags::observe.system_time ) {
	output << "\", " << endl << indent( i + 2 ) << "  "
	       << "\"sys cpu";
    }
    const map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    for ( map<unsigned,LQIO::DOM::Entity *>::const_iterator ep = entities.begin(); ep != entities.end(); ++ep ) {
	LQIO::DOM::Entity * entity = ep->second;
	if ( isInterestingProcessor( dynamic_cast<LQIO::DOM::Processor *>(entity) ) && Flags::observe.utilization ) {
	    output << "\", " << endl << indent( i + 2 ) << "  "
		   << "\"p(" << entity->getName() << ").util";
	} else if ( isReferenceTask( dynamic_cast<LQIO::DOM::Task *>(entity) ) && Flags::observe.throughput ) {
	    output << "\", " << endl << indent( i + 2 ) << "  "
		   << "\"t(" << entity->getName() << ").tput";
	} else if ( dynamic_cast<LQIO::DOM::Task *>(entity) && !isReferenceTask( dynamic_cast<LQIO::DOM::Task *>(entity) ) && Flags::observe.utilization ) {
	    output << "\", " << endl << indent( i + 2 ) << "  "
		   << "\"t(" << entity->getName() << ").util";
	}
    }
    output << "\" );" << endl;
    return output;
}


/* static */ ostream&
Generate::printResults( ostream& output, const LQIO::DOM::Document& document, const int i, const string& prefix )
{
    output << indent(i) << "println_spaced( \", \"";
    if ( prefix.size() ) {
	output  << ", " << prefix;
    }
    if ( Flags::observe.iterations ) {
	output << "," << endl << indent( i + 2 ) << "  "
	       << "document().iterations";
    }
    if ( Flags::observe.mva_waits ) {
	output << "," << endl << indent( i + 2 ) << "  "
	       << "document().waits";
    }
    if ( Flags::observe.mva_waits ) {
	output << "," << endl << indent( i + 2 ) << "  "
	       << "document().user_cpu_time";
    }
    if ( Flags::observe.mva_waits ) {
	output << "," << endl << indent( i + 2 ) << "  "
	       << "document().system_cpu_time";
    }
    const map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    for ( map<unsigned,LQIO::DOM::Entity *>::const_iterator ep = entities.begin(); ep != entities.end(); ++ep ) {
	LQIO::DOM::Entity * entity = ep->second;
	if ( isInterestingProcessor( dynamic_cast<LQIO::DOM::Processor *>(entity) ) && Flags::observe.utilization ) {
	    output << "," << endl << indent( i + 2 ) << "  " 
		   << "processor(\"" << entity->getName() << "\").utilization";
	} else if ( isReferenceTask( dynamic_cast<LQIO::DOM::Task *>(entity) ) && Flags::observe.throughput ) {		/* Reference task */
	    output << "," << endl << indent( i + 2 ) << "  "
		   << "task(\"" << entity->getName() << "\").throughput";
	} else if ( dynamic_cast<LQIO::DOM::Task *>(entity) && !isReferenceTask( dynamic_cast<LQIO::DOM::Task *>(entity) ) && Flags::observe.utilization ) {
	    output << "," << endl << indent( i + 2 ) << "  "
		   << "task(\"" << entity->getName() << "\").utilization";
	}
    }
    output << ");" << endl;
    return output;
}

void
Generate::insertSPEXObservations()
{
    if ( Flags::observe.iterations ) {
	std::string name = "$i";
	spex_document_observation( KEY_ITERATIONS, name.c_str() );
	spex_result_assignment_statement( name.c_str(), 0 );
    }
    if ( Flags::observe.mva_waits ) {
	std::string name = "$w";
	spex_document_observation( KEY_WAITING, name.c_str() );
	spex_result_assignment_statement( name.c_str(), 0 );
    }
    if ( Flags::observe.user_time ) {
	std::string name = "$usr";
	spex_document_observation( KEY_USER_TIME, name.c_str() );
	spex_result_assignment_statement( name.c_str(), 0 );
    }
    if ( Flags::observe.system_time ) {
	std::string name = "$sys";
	spex_document_observation( KEY_USER_TIME, name.c_str() );
	spex_result_assignment_statement( name.c_str(), 0 );
    }
    /* Make the result variables */
    const map<unsigned,LQIO::DOM::Entity *>& entities = _document->getEntities();
    for ( map<unsigned,LQIO::DOM::Entity *>::const_iterator ep = entities.begin(); ep != entities.end(); ++ep ) {
	LQIO::DOM::Entity * entity = ep->second;
	if ( isInterestingProcessor( dynamic_cast<LQIO::DOM::Processor *>(entity) ) && Flags::observe.utilization ) {
	    std::string name = "$p_";
	    name += entity->getName();
	    spex_processor_observation( entity, KEY_UTILIZATION, 0, name.c_str(), 0 );
	    spex_result_assignment_statement( name.c_str(), 0 );
	} else if ( isReferenceTask( dynamic_cast<LQIO::DOM::Task *>(entity) ) && Flags::observe.throughput ) {		/* Reference task */
	    std::string name = "$f_";
	    name += entity->getName();
	    spex_task_observation( entity, KEY_THROUGHPUT, 0, 0, name.c_str(), 0 );
	    spex_result_assignment_statement( name.c_str(), 0 );
	} else if ( dynamic_cast<LQIO::DOM::Task *>(entity) && !isReferenceTask( dynamic_cast<LQIO::DOM::Task *>(entity) ) && Flags::observe.utilization ) {
	    std::string name = "$u_";
	    name += entity->getName();
	    spex_task_observation( entity, KEY_UTILIZATION, 0, 0, name.c_str(), 0 );
	    spex_result_assignment_statement( name.c_str(), 0 );
	}
    }
}


/* static */ ostream& 
Generate::printIndent( ostream& output, const int i ) 
{
    output << setw( i * 3 ) << " ";
    return output;
}

ostream&
Generate::verbose( ostream& output ) const
{
    output << "Layer, tasks, multi, entries, phase 2" << std::endl;
    for ( unsigned int l = 0; l <= _number_of_layers; ++l ) {
	unsigned int is_multi = 0;
	try {
	    for ( std::vector<LQIO::DOM::Task *>::const_iterator task = _task[l].begin(); task != _task[l].end(); ++task ) {
		double copies = LQIO::DOM::to_double( *(*task)->getCopies() );
		if (  copies > 1. ) {
		    is_multi += 1;
		}
	    }
	}
	catch ( std::domain_error &e ) {		/* If we are using variables, can't tell, so ignore. */
	}
	unsigned int phase_2 = 0;
	for ( std::vector<LQIO::DOM::Entry *>::const_iterator entry = _entry[l].begin(); entry != _entry[l].end(); ++entry ) {
	    if ( (*entry)->hasPhase(2) ) {
		phase_2 += 1;
	    }
	}
	output << l << ", " << _task[l].size() << ", " << is_multi << ", " << _entry[l].size() << ", " << phase_2 << std::endl;
    }
    return output;
}

/* ------------------------------------------------------------------------ */

void
Generate::ModelVariable::lqx_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) return;		// Ignore.
    _model._program << indent( 1 ) << var << " = " << (*value)() << ";" << endl;
}


/*
 * Generate a vector of values (outside `for' loop).
 */

void
Generate::ModelVariable::lqx_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( value->getType() == RV::RandomVariable::DETERMINISTIC ) {
	_model._program << indent( 1 ) << var << " = " << (*value)() << ";" << endl;
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
    ostringstream name;
    name << var;
    const string& buf = name.str();
    double delta = val * Flags::sensitivity;
    _model._program << indent( 1 ) << &buf[1] << " = [ "
		    << val-delta << ", "
		    << val << ", "
		    << val+delta << " ];" << endl;
}

/*
 * This method is invoked inside of the LQX `for' loop for random models.
 */

void
Generate::ModelVariable::lqx_function( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( !var.wasSet() && value->getType() != RV::RandomVariable::DETERMINISTIC ) {
	_model._program << indent( 2 ) << var << " = " << value->name();
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
 * This method is invoked inside of the LQX `for' loop for random models.
 */

void
Generate::ModelVariable::lqx_index( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) return; // Ignore.
    ostringstream name;
    name << var;
    const string& buf = name.str();
    _model._program << indent( 2 ) << var << " = " << &buf[1] << "[i];" << endl; 
}

/*
 * Generate a vector of values.  For spex, it's "$x, $y = expr."
 */

void
Generate::ModelVariable::spex_random( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) {
	return;		// Ignore.
    } else if ( dynamic_cast<const RV::Deterministic *>(value) ) {
	return spex_scalar( var, value );
    } else {
	std::ostringstream name;
	name << var;

	/* Get args and form call to RV generator */

	LQX::SyntaxTreeNode * expr = 0;
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

	spex_forall( "$experiments", name.str().c_str(), expr );
    }
}

void
Generate::ModelVariable::spex_scalar( const LQIO::DOM::ExternalVariable& var, const RV::RandomVariable * value ) const
{
    if ( var.wasSet() ) return;		// Ignore.
    // Need to pass in function to store in spex. -- spex_forall() or spex_assignment}
    std::ostringstream name;
    name << var;
    LQX::SyntaxTreeNode * expr = new LQX::ConstantValueExpression( (*value)() );
    spex_assignment_statement( name.str().c_str(), expr, true );
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
    double delta = val * Flags::sensitivity;
    std::vector<LQX::SyntaxTreeNode *> * list = new std::vector<LQX::SyntaxTreeNode*>();
    list->push_back( new LQX::ConstantValueExpression( val - delta ) );
    list->push_back( new LQX::ConstantValueExpression( val ) );
    list->push_back( new LQX::ConstantValueExpression( val + delta ) );
    ostringstream name;
    name << var;
    spex_array_assignment( name.str().c_str(), list, true );
}

/* ------------------------------------------------------------------------ */

void
Generate::MakeProcessorVariable::operator()( LQIO::DOM::Processor * processor ) const
{
    LQIO::DOM::ExternalVariable * copies = const_cast<LQIO::DOM::ExternalVariable *>(processor->getCopies());
     if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) ) {
	copies = _model.get_rv( "$p_", processor->getName(), _multiplicity );
	processor->setCopies( copies );
    }
    (this->*_f)( *copies, _multiplicity );
}


void
Generate::SetProcessorVariable::operator()( const LQIO::DOM::Processor * processor ) const
{
    const LQIO::DOM::ExternalVariable * copies = processor->getCopies();
    if ( !copies->wasSet() ) {
	(this->*_f)( *copies, _multiplicity );
    }
}


void
Generate::MakeTaskVariable::operator()( LQIO::DOM::Task * task ) const
{
    LQIO::DOM::ExternalVariable * copies = const_cast<LQIO::DOM::ExternalVariable *>(task->getCopies());
    if ( isReferenceTask( task ) ) {
	if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) ) {
	    copies = _model.get_rv( "$c_", task->getName(), _customers );
	    task->setCopies( copies );
	}
	(this->*_f)( *copies, _customers ); 
	LQIO::DOM::ExternalVariable * think_time = dynamic_cast<LQIO::DOM::ExternalVariable * >( task->getThinkTime() );
	if ( !think_time || dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( think_time ) ) {
	    think_time = _model.get_rv( "$Z_", task->getName(), _think_time );
	    task->setThinkTime( think_time );
	}
	(this->*_f)( *think_time, _think_time );
    } else {
	if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( copies ) ) {
	    copies = _model.get_rv( "$c_", task->getName(), _multiplicity );
	    task->setCopies( copies );
	}
	(this->*_f)( *copies, _multiplicity ); 
    }
}



void
Generate::SetTaskVariable::operator()( const LQIO::DOM::Task * task ) const
{
    const RV::RandomVariable * var;
    if ( isReferenceTask( task ) ) {
	var = _customers; 
	LQIO::DOM::ExternalVariable * think_time = task->getThinkTime();
	if ( think_time && !think_time->wasSet() ) {
	    (this->*_f)( *think_time, _think_time );
	}
    } else {
	var = _multiplicity; 
    }

    const LQIO::DOM::ExternalVariable * copies = task->getCopies();
    if ( !copies->wasSet() ) {
	(this->*_f)( *copies, var ); 
    }
}



void
Generate::MakeEntryVariable::operator()( LQIO::DOM::Entry * entry ) const
{
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
	LQIO::DOM::Phase* phase = p->second;
	LQIO::DOM::ExternalVariable * service = const_cast<LQIO::DOM::ExternalVariable *>( phase->getServiceTime() );
	if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>( service ) ) {
	    service = _model.get_rv( "$s_", phase->getName(), _service_time );
	    phase->setServiceTime( service );
	}
	(this->*_f)( *service, _service_time );
    }
}



void
Generate::SetEntryVariable::operator()( const LQIO::DOM::Entry * entry ) const
{
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = entry->getPhaseList();
    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
	LQIO::DOM::Phase* phase = p->second;
	const LQIO::DOM::ExternalVariable * service = phase->getServiceTime();
	if ( !service->wasSet() ) {
	    (this->*_f)( *service, _service_time );
	}
    }
}



void
Generate::MakeCallVariable::operator()( LQIO::DOM::Call * call ) const
{
    /* check for var... */
    LQIO::DOM::ExternalVariable * calls = const_cast<LQIO::DOM::ExternalVariable *>( call->getCallMean() );
    if ( dynamic_cast<LQIO::DOM::ConstantExternalVariable *>(calls) ) {
	switch ( call->getCallType() ) {
	case LQIO::DOM::Call::RENDEZVOUS:    calls = _model.get_rv( "$y_", call->getName(), _rnv_rate ); break;
	case LQIO::DOM::Call::SEND_NO_REPLY: calls = _model.get_rv( "$z_", call->getName(), _snr_rate ); break;
	case LQIO::DOM::Call::FORWARD:       calls = _model.get_rv( "$f_", call->getName(), _forwarding_rate ); break;
	default: abort();
	}
	call->setCallMean( calls );
    }
    switch ( call->getCallType() ) {
    case LQIO::DOM::Call::RENDEZVOUS:    (this->*_f)( *calls, _rnv_rate ); break;
    case LQIO::DOM::Call::SEND_NO_REPLY: (this->*_f)( *calls, _snr_rate ); break;
    case LQIO::DOM::Call::FORWARD:       (this->*_f)( *calls, _forwarding_rate ); break;
    default: abort();
    }
}



void
Generate::SetCallVariable::operator()( const LQIO::DOM::Call * call ) const
{
    /* check for var... */
    const RV::RandomVariable * var;
    switch ( call->getCallType() ) {
    case LQIO::DOM::Call::RENDEZVOUS:    var = _rnv_rate; break;
    case LQIO::DOM::Call::SEND_NO_REPLY: var = _snr_rate; break;
    case LQIO::DOM::Call::FORWARD:       var = _forwarding_rate; break;
    default: abort();
    }
    const LQIO::DOM::ExternalVariable * calls = call->getCallMean();
    if ( !calls->wasSet() ) {
	(this->*_f)( *calls, var ); 
    }
}
