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
#include <lqio/srvn_output.h>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif

const char * Generate::model_opts[] = {
    "iteration-limit",
    "print-interval",
    "overtaking",
    "convergence",
    "underrelaxation",
    "comment",
    0
};

unsigned Generate::iteration_limit   = 50;
unsigned Generate::print_interval    = 10;
double Generate::convergence_value   = 0.00001;
double Generate::underrelaxation     = 0.9;


const char * Generate::comment = "";

LQIO::DOM::ConstantExternalVariable * Generate::ONE = 0;
LQIO::DOM::ConstantExternalVariable * Generate::ZERO = 0;

/*
 * Initialize probabilities.
 */

Generate::Generate( const unsigned runs ) 
    : _runs(runs)
{
    _document = LQIO::DOM::Document::create( &io_vars );		// For XML output.

    if ( !comment || strlen(comment) == 0 ) {
	comment = command_line.c_str();
    }
    if ( !ONE )   ONE = new LQIO::DOM::ConstantExternalVariable( 1.0 );
    if ( !ZERO ) ZERO = new LQIO::DOM::ConstantExternalVariable( 0.0 );
}


Generate::Generate( const unsigned runs, LQIO::DOM::Document * document ) 
    : _document(document ), _runs(runs)
{
    if ( !ONE )   ONE = new LQIO::DOM::ConstantExternalVariable( 1.0 );
    if ( !ZERO ) ZERO = new LQIO::DOM::ConstantExternalVariable( 0.0 );
}


Generate::~Generate()
{
    delete _document;
    _document = 0;
    comment = "";
}

/* 
 * Generate a default model by invoking functions directy.
 */

void
Generate::fixed( variateFunc get_parameter )
{
    _task.resize( 2 );
    _entry.resize( 2 );
    const string client_name = "client";
    const string server_name = "server"; 

    opt[SERVICE_TIME].opts.func = &Generate::deterministic;
    opt[SERVICE_TIME].mean = 1;
    opt[PHASE2_PROBABILITY].opts.func = &Generate::deterministic;
    opt[PHASE2_PROBABILITY].mean = 0;
    opt[RNV_PROBABILITY].opts.func = &Generate::deterministic;
    opt[RNV_PROBABILITY].mean = 1;
    opt[RNV_REQUESTS].opts.func = &Generate::deterministic;
    opt[RNV_REQUESTS].mean = 1;
    opt[THINK_TIME].opts.func = &Generate::deterministic;
    opt[THINK_TIME].mean = 0;

    LQIO::DOM::Processor * clientProcessor = addProcessor( client_name, SCHEDULE_DELAY, ONE );
    LQIO::DOM::Processor * serverProcessor = addProcessor( server_name, SCHEDULE_PS, ONE );
    _processor.push_back( serverProcessor );
 
    LQIO::DOM::Entry * clientEntry = addEntry( client_name, 1, get_parameter );
    vector<LQIO::DOM::Entry *> clientEntries;
    _entry[0].push_back( clientEntry );
    clientEntries.push_back( clientEntry );
    _task[0].push_back( addTask( client_name, SCHEDULE_CUSTOMER, ONE, clientEntries, clientProcessor ) );

    LQIO::DOM::Entry * serverEntry = addEntry( server_name, 1, get_parameter );
    vector<LQIO::DOM::Entry *> serverEntries;
    _entry[1].push_back( serverEntry );
    serverEntries.push_back( serverEntry) ;
    _task[1].push_back( addTask( server_name, SCHEDULE_FIFO, ONE, serverEntries, serverProcessor ) );

    _call.push_back( addCall( clientEntry, serverEntry, get_parameter ) );

    if ( Flags::lqx_output ) {
	addLQX( &Generate::serialize, getNumberOfRuns() > 1 ? &ModelVariable::vector : &ModelVariable::scalar );
    }
    _document->setModelParameters( comment, new LQIO::DOM::ConstantExternalVariable( convergence_value ), 
				   new LQIO::DOM::ConstantExternalVariable( iteration_limit ), 
				   new LQIO::DOM::ConstantExternalVariable( print_interval ), 
				   new LQIO::DOM::ConstantExternalVariable( underrelaxation ), 0 );
}



/*
 * Generate a random model.
 */

void
Generate::random( variateFunc get_parameter )
{
    const unsigned n_layers = unsigned_rv( NUMBER_OF_LAYERS );
    assert( n_layers >= 2 );
    _task.resize( n_layers );
    _entry.resize( n_layers );

    /* Create Reference tasks */

    const unsigned n_ref_tasks = unsigned_rv( NUMBER_OF_CUSTOMERS );
    assert( n_ref_tasks > 0 );
    for ( unsigned r = 0; r < n_ref_tasks; ++r ) {
 	ostringstream proc_name;
	proc_name << "p" << _document->getNumberOfProcessors() + 1;
	LQIO::DOM::Processor * aProcessor = addProcessor( proc_name.str(), SCHEDULE_DELAY, ONE );

 	ostringstream entry_name;
	entry_name << "e" << _document->getNumberOfEntries() + 1;
	vector<LQIO::DOM::Entry *> entries;
	LQIO::DOM::Entry * anEntry = addEntry( entry_name.str(), 1, get_parameter );
	entries.push_back( anEntry );
	_entry[0].push_back( anEntry );

 	ostringstream task_name;
	task_name << "t" << _document->getNumberOfTasks() + 1;
	_task[0].push_back( addTask( task_name.str(), SCHEDULE_CUSTOMER, 
				     (this->*get_parameter)( CUSTOMERS, task_name.str() ), 		/* Copies */
				     entries,
				     aProcessor,
				     (this->*get_parameter)( THINK_TIME, task_name.str() ) ) );
    }

    /* Create processors */

    const unsigned n_tasks = n_layers == 2 ? 0 : max( unsigned_rv( NUMBER_OF_TASKS ), n_layers-2 );		/* Need one task per layer, not counting reference tasks. */
    const unsigned n_procs = min( unsigned_rv( NUMBER_OF_PROCESSORS ), n_tasks );
    assert( n_procs >= 0 );
    for ( unsigned i = 1; i <= n_procs; ++i ) {
	LQIO::DOM::Processor * aProcessor;
	ostringstream proc_name;
	proc_name << "p" << _document->getNumberOfProcessors() + 1;
        const double x = drand48();
        if ( x < opt[INFINITE_SERVER].mean ) {
            aProcessor = addProcessor( proc_name.str(), SCHEDULE_DELAY, ONE );
        } else if ( x < opt[INFINITE_SERVER].mean + opt[MULTI_SERVER].mean ) {
            aProcessor = addProcessor( proc_name.str(), SCHEDULE_FIFO, (this->*get_parameter)( PROCESSOR_MULTIPLICITY, proc_name.str() ) );
        } else {
            aProcessor = addProcessor( proc_name.str(), SCHEDULE_PS, ONE );
        }
	_processor.push_back( aProcessor );
    }
    shuffle( _processor );

    /* Create other tasks */

    for ( unsigned j = 0; j < n_tasks; ++j ) {
	const unsigned int h = j < n_procs ? j : uniform(0,n_procs);		/* Pick randomly after all assigned */
	assert( h < n_procs );
	LQIO::DOM::Processor * aProcessor = _processor[h];

	const unsigned int l = ((j+1) < n_layers) ? j : uniform( 1, n_layers-1 );	/* Pick randomly after all assigned */

	vector<LQIO::DOM::Entry *> entries;
	const unsigned nEntries = unsigned_rv( NUMBER_OF_ENTRIES );
	assert( nEntries > 0 );
	for ( unsigned e = 1; e <= nEntries; ++e ) {
	    ostringstream entry_name;
	    entry_name << "e" << _document->getNumberOfEntries() + 1;
	    LQIO::DOM::Entry * anEntry = addEntry( entry_name.str(), choose( PHASE2_PROBABILITY ) ? 2 : 1, get_parameter );
	    entries.push_back( anEntry );
	    _entry[l].push_back( anEntry );
	}

	ostringstream task_name;
	task_name << "t" << _document->getNumberOfTasks() + 1;

	LQIO::DOM::Task * aTask;
	const double x = drand48();
	if ( x < opt[INFINITE_SERVER].mean ) {
	    aTask = addTask( task_name.str(), SCHEDULE_DELAY, ONE, entries, aProcessor );
	} else if ( x < opt[INFINITE_SERVER].mean + opt[MULTI_SERVER].mean ) {
	    aTask = addTask( task_name.str(), SCHEDULE_FIFO, (this->*get_parameter)( TASK_MULTIPLICITY, task_name.str() ), entries, aProcessor );
	} else {
	    aTask = addTask( task_name.str(), SCHEDULE_FIFO, ONE, entries, aProcessor );
	}

	_task[l].push_back( aTask );
    }


    /* 
     * Connect reference tasks to top layer.  All layer 2 servers need
     * to connect to a client task, and all client tasks must call a
     * server task.  If more clients, than servers, choose server entry
     * randomly, and vice-versa 
     */

    const std::vector<LQIO::DOM::Entry*>& src_entries = _entry[0];
    const unsigned int n_src_entries = src_entries.size();
    const std::vector<LQIO::DOM::Entry*>& dst_entries = _entry[1];
    const unsigned int n_dst_entries = dst_entries.size();

    const unsigned int n_clients = min( n_src_entries, n_dst_entries );
    for ( unsigned int i = 0; i < n_clients; ++i ) {
	_call.push_back( addCall( src_entries[i], dst_entries[i], get_parameter ) );
    }

    if ( n_clients < _entry[0].size() ) {

	/* Too few servers - make more connections */

	for ( unsigned int i = n_clients; i < n_src_entries; ++i ) {
	    const unsigned e = n_dst_entries * drand48();
	    _call.push_back( addCall( src_entries[i], dst_entries[e], get_parameter ) );
	}

    } else if (  n_clients < _entry[1].size() ) {
		
	/* Too many server entries... */

	for ( unsigned int i = n_clients; i < n_dst_entries; ++i ) {
	    const unsigned int e = n_src_entries * drand48();
	    _call.push_back(  addCall( src_entries[e], dst_entries[i], get_parameter ) );
	}
    }

    /*
     * Now connect all entries for l>2 layers to upper level layers.
     * First entry on every task must go to an entry in l-1 to ensure
     * that this task goes in this layer.  Other entries can go to
     * anywhere.
     */

    for ( unsigned int l = 2; l < n_layers; ++l ) {
	for ( unsigned int i = 0; i < _task[l].size(); ++i ) {
	    const std::vector<LQIO::DOM::Entry*>& dst_entries = _task[l][i]->getEntryList();

	    for ( unsigned int j = 0; j < dst_entries.size(); ++j ) {
		const unsigned int k = ( j == 0 ) ? l - 1 : l * drand48();
		const unsigned int e = _entry[k].size() * drand48();		/* Pick a random enty */
		_call.push_back( addCall( _entry[k][e], dst_entries[j], get_parameter ) );
	    }
	}
    }

    if ( Flags::lqx_output ) {
	addLQX( &Generate::serialize2, getNumberOfRuns() > 1 ? &ModelVariable::vector : &ModelVariable::scalar );
    }
    _document->setModelParameters( comment, new LQIO::DOM::ConstantExternalVariable( convergence_value ), 
				   new LQIO::DOM::ConstantExternalVariable( iteration_limit ), 
				   new LQIO::DOM::ConstantExternalVariable( print_interval ), 
				   new LQIO::DOM::ConstantExternalVariable( underrelaxation ), 0 );
}


void
Generate::reparameterize( )
{
    addLQX( &Generate::serialize2, getNumberOfRuns() > 1 ? &ModelVariable::vector : &ModelVariable::scalar );
}

/*
 * Set the general arguments, G "comment" ...
 */

bool
Generate::getGeneralArgs( char * options )
{
    char * value;

    while ( *options ) {
	switch( getsubopt( &options, const_cast<char * const *>(Generate::model_opts), &value ) ) {
	case MODEL_COMMENT:
	    Generate::comment = value;
	    break;

	case ITERATION_LIMIT:
	    if ( !value || (Generate::iteration_limit = (unsigned)strtol( value, 0, 10 )) == 0 ) {
		cerr << io_vars.lq_toolname << "iteration-limit=" << value << " is invalid, choose non-negative integer." << endl;
		(void) exit( 3 );
	    }
	    break;

	case PRINT_INTERVAL:
	    if ( !value || (Generate::print_interval = (unsigned)strtol( value, 0, 10 )) == 0 ) {
		cerr << io_vars.lq_toolname << "print-interval=" << value << " is invalid, choose non-negative integer." << endl;
		(void) exit( 3 );
	    }
	    break;

	case CONVERGENCE_VALUE:
	    if ( !value || (Generate::convergence_value = strtod( value, 0 )) == 0 ) {
		cerr << io_vars.lq_toolname << "convergence=" << value << " is invalid, choose non-negative real." << endl;
		(void) exit( 3 );
	    }
	    break;

	case UNDERRELAXATION:
	    if ( !value || (Generate::underrelaxation = strtod( value, 0 )) < -1.0 || 1.0 < Generate::underrelaxation ) {
		cerr << io_vars.lq_toolname << "underrelaxation=" << value << " is invalid, choose real between 0.0 and 1.0." << endl;
		(void) exit( 3 );
	    }
	    break;

	default:
	    return false;
	}
    }
    return true;
}

/*
 * Add a processor to layer.
 */

LQIO::DOM::Processor * 
Generate::addProcessor( const string& name, const scheduling_type sched_flag, LQIO::DOM::ExternalVariable * n_copies )
{
    LQIO::DOM::Processor * dom_processor;

    dom_processor = new LQIO::DOM::Processor( _document, name.c_str(), sched_flag, n_copies, 1, (void *)0 );
    dom_processor->setRateValue( 1.0 );
    if ( sched_flag == SCHEDULE_PS ) {
	dom_processor->setQuantumValue( 0.1 );
    }
    _document->addProcessorEntity( dom_processor );    /* Map into the document */

    return dom_processor;
}


/*
 * Add a task to a layer.
 */

LQIO::DOM::Task *
Generate::addTask( const string& name, const scheduling_type sched_flag, LQIO::DOM::ExternalVariable * n_copies, 
		   const vector<LQIO::DOM::Entry *>& dom_entries, LQIO::DOM::Processor * dom_processor, LQIO::DOM::ExternalVariable * think_time )
{
    LQIO::DOM::Task * dom_task;
    dom_task = new LQIO::DOM::Task( _document, name.c_str(), sched_flag, dom_entries, 0, 
				    dom_processor,
				    /* priority */ 0, 		
				    n_copies, /* replicas */ 1, /* Group */ 0, (void *)0 );


    if ( think_time ) {
	dom_task->setThinkTime( think_time );
    }

    _document->addTaskEntity(dom_task);
    dom_processor->addTask(dom_task);
    return dom_task;
}


/*
 * Add an entry to a layer.
 */

LQIO::DOM::Entry *
Generate::addEntry( const string& name, unsigned n_phases, variateFunc get_parameter )
{
    LQIO::DOM::Entry* dom_entry = new LQIO::DOM::Entry( _document, name.c_str(), (void *)0 );
    _document->addEntry(dom_entry);
    for ( unsigned p = 1; p <= n_phases; ++p ) {
	LQIO::DOM::Phase* dom_phase = dom_entry->getPhase(p);
	string phase_name=name;
	phase_name += '_';
	phase_name += "_123"[p];
	dom_phase->setName( phase_name );
        dom_phase->setServiceTime( (this->*get_parameter)( SERVICE_TIME, phase_name ) );
    }

    return dom_entry;
}


LQIO::DOM::Call *
Generate::addCall( LQIO::DOM::Entry * dom_src, LQIO::DOM::Entry * dom_dst, variateFunc get_parameter )
{
    const unsigned p = choose( PHASE2_PROBABILITY ) ? 2 : 1;
    LQIO::DOM::Phase * phase = dom_src->getPhase( p ); 
    LQIO::DOM::Call * dom_call;
    string call_name = phase->getName();
    call_name += '_';
    call_name += dom_dst->getName();
    if ( choose( RNV_PROBABILITY ) ) {
	dom_call = new LQIO::DOM::Call( _document,
					LQIO::DOM::Call::RENDEZVOUS, 
					phase,
					dom_dst, p,
					(this->*get_parameter)( RNV_REQUESTS, call_name ) );
    } else {
	dom_call = new LQIO::DOM::Call( _document,
					LQIO::DOM::Call::SEND_NO_REPLY, 
					phase,
					dom_dst, p,
					(this->*get_parameter)( SNR_REQUESTS, call_name ) );
    }
    dom_src->appendOriginatingCall(dom_call);
    dom_call->setName( call_name );

    return dom_call;
}



void
Generate::addLQX( serializeFunc f, const ModelVariable::variableValueFunc g )
{
    const unsigned n = getNumberOfRuns();
    (this->*f)( g );					/* Make Variables */

    if ( n > 1 ) {
	_program << print_header( *_document, 1, "\"i, \"" );
	_program << indent( 1 ) << "for ( i = 0; i < " << n << "; i = i + 1 ) {" << endl;
	(this->*f)( &ModelVariable::index );		/* Get Variables */
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
Generate::serialize( const ModelVariable::variableValueFunc f )  
{
    for_each( _processor.begin(), _processor.end(), GetProcessorVariable( *this, f ) );
    for ( vector<vector<LQIO::DOM::Task *> >::const_iterator layer = _task.begin(); layer != _task.end(); ++layer ) {
	for_each( (*layer).begin(), (*layer).end(), GetTaskVariable( *this, f ) );		//vector of vector
    }
    for ( vector<vector<LQIO::DOM::Entry *> >::const_iterator layer = _entry.begin(); layer != _entry.end(); ++layer ) {
	for_each( (*layer).begin(), (*layer).end(), GetEntryVariable( *this, f ) );	//vector of vector
    }
    for_each( _call.begin(), _call.end(), GetCallVariable( *this, f ) );	
}



void
Generate::serialize2( const ModelVariable::variableValueFunc f )
{
    SetProcessorVariable setProcessorVariable( *this, f );
    SetTaskVariable setTaskVariable( *this, f );
    SetEntryVariable setEntryVariable( *this, f );
    SetCallVariable setCallVariable( *this, f );

    const map<unsigned,LQIO::DOM::Entity *>& entities = _document->getEntities();			// For remapping by lqn2ps.
    for ( map<unsigned,LQIO::DOM::Entity *>::const_iterator ep = entities.begin(); ep != entities.end(); ++ep ) {
	LQIO::DOM::Entity * entity = ep->second;
	LQIO::DOM::Processor * processor = dynamic_cast<LQIO::DOM::Processor *>(entity);
	if ( !processor ) continue;
	setProcessorVariable( processor );
	const std::vector<LQIO::DOM::Task*>& tasks = processor->getTaskList();
	for ( std::vector<LQIO::DOM::Task*>::const_iterator tp = tasks.begin(); tp != tasks.end(); ++tp ) {
	    LQIO::DOM::Task * task = *tp;
	    setTaskVariable( task );
	    const vector<LQIO::DOM::Entry*>& entries = task->getEntryList();
	    for ( vector<LQIO::DOM::Entry*>::const_iterator entp = entries.begin(); entp != entries.end(); ++entp ) {
		LQIO::DOM::Entry * entry = *entp;
		setEntryVariable( entry );
		const unsigned n_phases = entry->getMaximumPhase();
		for ( unsigned p = 1; p <= n_phases; ++p ) {
		    if ( !entry->hasPhase(p) ) continue;
		    const LQIO::DOM::Phase* phase = entry->getPhase(p);
		    const std::vector<LQIO::DOM::Call*>& calls = phase->getCalls();
		    for_each( calls.begin(), calls.end(), setCallVariable );
		}
	    }
	}
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
	LQIO::SRVN::Input srvn( *_document, _document->getEntities(), Flags::annotate_input );
	srvn.print( output );
    }

    return output;
}



ostream&
Generate::printHeader( ostream& output, const LQIO::DOM::Document& document, const int i, const string& prefix )
{
    output << indent(i) << "println(";
    if ( prefix.size() ) {
	output << prefix << ", ";
    }
    const map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    for ( map<unsigned,LQIO::DOM::Entity *>::const_iterator ep = entities.begin(); ep != entities.end(); ++ep ) {
	LQIO::DOM::Entity * entity = ep->second;
	if ( ep != entities.begin() ) {
	    output << ", \"," << endl << indent( i + 2 ) << "  ";
	}
	if ( dynamic_cast<LQIO::DOM::Processor *>(entity) ) {
	    output << "\"p(" << entity->getName() << ").util";
	} else if ( entity->getSchedulingType() == SCHEDULE_CUSTOMER ) {
	    output << "\"t(" << entity->getName() << ").tput";
	} else {
	    output << "\"t(" << entity->getName() << ").util";
	}
    }
    output << "\");" << endl;	
    return output;
}


ostream&
Generate::printResults( ostream& output, const LQIO::DOM::Document& document, const int i, const string& prefix )
{
    output << indent(i) << "println(";
    if ( prefix.size() ) {
	output << prefix << ", \", \", ";
    }
    const map<unsigned,LQIO::DOM::Entity *>& entities = document.getEntities();
    for ( map<unsigned,LQIO::DOM::Entity *>::const_iterator ep = entities.begin(); ep != entities.end(); ++ep ) {
	LQIO::DOM::Entity * entity = ep->second;
	if ( ep != entities.begin() ) {
	    output << ", \", \"," << endl << indent( i + 2 ) << "  ";
	}
	if ( dynamic_cast<LQIO::DOM::Processor *>(entity) ) {
	    output << "processor(\"" << entity->getName() << "\").utilization";
	} else if ( entity->getSchedulingType() == SCHEDULE_CUSTOMER ) {		/* Reference task */
	    output << "task(\"" << entity->getName() << "\").throughput";
	} else {
	    output << "task(\"" << entity->getName() << "\").utilization";
	}
    }
    output << ");" << endl;
    return output;
}

ostream& 
Generate::printIndent( ostream& output, const int i ) 
{
    output << setw( i * 3 ) << " ";
    return output;
}
 
template <class Type> void
Generate::shuffle( vector<Type>& array ) 
{
    const unsigned n = array.size();
    for ( unsigned i = n; i >= 1; --i ) {
	const unsigned k = static_cast<unsigned>(drand48() * i);
	if ( i-1 != k ) {
	    Type temp = array[k];
	    array[k] = array[i-1];
	    array[i-1] = temp;
	}
    }
}

double
Generate::double_rv( opt_values i )
{
    return (*opt[i].opts.func)( opt[i].floor, opt[i].mean );
}

unsigned
Generate::unsigned_rv( unsigned i )
{
    if ( opt[i].opts.func == &Generate::uniform ) {
	return static_cast<unsigned>( (*opt[i].opts.func)( opt[i].floor, opt[i].mean + 1.0 ) );	/* Range */
    } else {
	return static_cast<unsigned>( (*opt[i].opts.func)( opt[i].floor, opt[i].mean ) + 0.5 );	/* Round off */
    }
}


LQIO::DOM::ExternalVariable * 
Generate::get_constant( const opt_values arg, const string& ) const
{
    return new LQIO::DOM::ConstantExternalVariable( get_value( arg ) );
}


LQIO::DOM::ExternalVariable * 
Generate::get_variable( const opt_values arg, const string& name ) const
{
    assert( name.size() > 0 );
    string var = "$";
    switch ( arg ) {
    case CUSTOMERS:		/* Fall through */
    case TASK_MULTIPLICITY:	var += "n_";	break;
    case PROCESSOR_MULTIPLICITY:var += "m_";	break;
    case RNV_REQUESTS:		var += "y_";	break;
    case SERVICE_TIME:		var += "s_";	break;
    case SNR_REQUESTS:		var += "z_";	break;
    case THINK_TIME:		var += "z_";	break;
    default:
	abort();
    }
    var += name;

    return _document->getSymbolExternalVariable( var );
}


double
Generate::get_value( const opt_values arg ) 
{
    switch ( arg ) {
    case CUSTOMERS:
    case NUMBER_OF_ENTRIES:
    case NUMBER_OF_PROCESSORS:
    case NUMBER_OF_TASKS:
    case PROCESSOR_MULTIPLICITY:
    case TASK_MULTIPLICITY:
	return unsigned_rv( arg );
    case RNV_REQUESTS:
    case SERVICE_TIME:
    case SNR_REQUESTS:
    case THINK_TIME:
	return double_rv( arg );
    default:
	abort();
    }
    return 0.;
}



bool
Generate::choose( unsigned i )
{
    return drand48() < opt[i].mean;
}
 
/* ------------------------------------------------------------------------ */

void
Generate::ModelVariable::scalar( const LQIO::DOM::ExternalVariable& var, const opt_values index ) const
{
    if ( var.wasSet() ) return;		// Ignore.
    _model._program << indent( 1 ) << var << " = " << get_value( index ) << ";" << endl;
}


/*
 * Generate a vector of values.
 */

void
Generate::ModelVariable::vector( const LQIO::DOM::ExternalVariable& var, const opt_values index ) const
{
    if ( var.wasSet() ) return;		// Ignore.
    ostringstream name;
    name << var;
    const string& buf = name.str();
    _model._program << indent( 1 ) << &buf[1] << " = [";
    for ( unsigned int i = 0; i < _model.getNumberOfRuns(); ++i ) {
	if ( i > 0 ) _model._program << ", ";
	_model._program << get_value( index );
    }
    _model._program << "];" << endl;	// = get... 
}

void
Generate::ModelVariable::index( const LQIO::DOM::ExternalVariable& var, const opt_values index ) const
{
    if ( var.wasSet() ) return;		// Ignore.
    ostringstream name;
    name << var;
    const string& buf = name.str();
    _model._program << indent( 2 ) << var << " = " << &buf[1] << "[i];" << endl;	// = get... 
}

void
Generate::GetProcessorVariable::operator()( const LQIO::DOM::Processor * processor ) 
{
    const LQIO::DOM::ExternalVariable& copies = *processor->getCopies();
    (this->*_f)( copies, PROCESSOR_MULTIPLICITY );
}


void
Generate::SetProcessorVariable::operator()( const LQIO::DOM::Processor * processor ) 
{
    const LQIO::DOM::ExternalVariable * copies = processor->getCopies();
    if ( !copies->wasSet() ) {
	(this->*_f)( *copies, PROCESSOR_MULTIPLICITY );
    }
}


void
Generate::GetTaskVariable::operator()( const LQIO::DOM::Task * task ) 
{
    const LQIO::DOM::ExternalVariable& copies = *task->getCopies();
    switch ( task->getSchedulingType() ) {
    case SCHEDULE_CUSTOMER:
    case SCHEDULE_POLL:
    case SCHEDULE_BURST: (this->*_f)( copies, CUSTOMERS ); break;
    default:		 (this->*_f)( copies, TASK_MULTIPLICITY ); break;
    }
    const LQIO::DOM::ExternalVariable * think_time = task->getThinkTime();
    if ( think_time ) {
	(this->*_f)( *think_time, THINK_TIME );
    }
}



void
Generate::SetTaskVariable::operator()( const LQIO::DOM::Task * task ) 
{
    opt_values i;
    switch ( task->getSchedulingType() ) {
    case SCHEDULE_CUSTOMER: /* Fall through */
    case SCHEDULE_POLL:     /* Fall through */
    case SCHEDULE_BURST:    i = CUSTOMERS; break;
    default:		    i = TASK_MULTIPLICITY; break;
    }

    const LQIO::DOM::ExternalVariable * copies = task->getCopies();
    if ( !copies->wasSet() ) {
	(this->*_f)( *copies, i ); 
    }

    LQIO::DOM::ExternalVariable * think_time = task->getThinkTime();
    if ( think_time && !think_time->wasSet() ) {
	(this->*_f)( *think_time, THINK_TIME );
    }
}



void
Generate::GetEntryVariable::operator()( const LQIO::DOM::Entry * entry ) 
{
    const unsigned n_phases = entry->getMaximumPhase();
    for ( unsigned p = 1; p <= n_phases; ++p ) {
	LQIO::DOM::Phase* phase = entry->getPhase(p);
	const LQIO::DOM::ExternalVariable& service = *phase->getServiceTime();
	(this->*_f)( service, SERVICE_TIME );
    }
}



void
Generate::SetEntryVariable::operator()( const LQIO::DOM::Entry * entry ) 
{
    const unsigned n_phases = entry->getMaximumPhase();
    for ( unsigned p = 1; p <= n_phases; ++p ) {
	if ( !entry->hasPhase(p) ) continue;
	const LQIO::DOM::Phase* phase = entry->getPhase(p);
	const LQIO::DOM::ExternalVariable * service = phase->getServiceTime();
	if ( !service->wasSet() ) {
	    (this->*_f)( *service, SERVICE_TIME );
	}
    }
}



void
Generate::GetCallVariable::operator()( const LQIO::DOM::Call * call ) 
{
    /* check for var... */
    const LQIO::DOM::ExternalVariable& calls = *call->getCallMean();
    switch ( call->getCallType() ) {
    case LQIO::DOM::Call::RENDEZVOUS:    (this->*_f)( calls, RNV_REQUESTS ); break;
    case LQIO::DOM::Call::SEND_NO_REPLY: (this->*_f)( calls, SNR_REQUESTS ); break;
    case LQIO::DOM::Call::FORWARD:       (this->*_f)( calls, FORWARDING_REQUESTS ); break;
    default: abort();
    }
}



void
Generate::SetCallVariable::operator()( const LQIO::DOM::Call * call ) 
{
    /* check for var... */
    opt_values i;
    switch ( call->getCallType() ) {
    case LQIO::DOM::Call::RENDEZVOUS:    i = RNV_REQUESTS; break;
    case LQIO::DOM::Call::SEND_NO_REPLY: i = SNR_REQUESTS; break;
    case LQIO::DOM::Call::FORWARD:       i = FORWARDING_REQUESTS; break;
    default: abort();
    }
    const LQIO::DOM::ExternalVariable * calls = call->getCallMean();
    if ( !calls->wasSet() ) {
	(this->*_f)( *calls, i ); 
    }
}
