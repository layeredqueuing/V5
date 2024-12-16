/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 2009								*/
/************************************************************************/

/*
 * Input processing.
 *
 * $Id: model.cc 17514 2024-12-05 20:33:10Z greg $
 */

#include "lqsim.h"
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <numeric>
#include <random>
#include <sstream>
#include <errno.h>
#include <unistd.h>
#if HAVE_MCHECK_H
#include <mcheck.h>
#endif
#include <lqio/dom_bindings.h>
#include <lqio/error.h>
#include <lqio/filename.h>
#include <lqio/input.h>
#include <lqio/json_document.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
#include "lqsim.h"
#include "activity.h"
#include "entry.h"
#include "errmsg.h"
#include "group.h"
#include "instance.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "runlqx.h"		// Coupling here is ugly at the moment
#include "task.h"

#if HAVE_PARASOL
extern "C" {
    extern void test_all_stacks();
}

int Model::__genesis_task_id = 0;
#endif
Model * Model::__model = nullptr;
bool Model::__enable_print_interval = false;
unsigned int Model::__print_interval = 0;
double Model::max_service = 0.0;
const double Model::simulation_parameters::DEFAULT_TIME = 1e5;
bool deferred_exception = false;	/* domain error detected during run.. throw after parasol stops. */
bool print_lqx = false;

/*----------------------------------------------------------------------*/
/*   Input processing.  Read input, extend and validate.                */
/*----------------------------------------------------------------------*/

/*
 * Initialize input parser parameters.
 */

Model::Model( LQIO::DOM::Document* document, const std::filesystem::path& input_file_name, const std::filesystem::path& output_file_name, LQIO::DOM::Document::OutputFormat output_format )
    : _document(document), _input_file_name(input_file_name), _output_file_name(output_file_name), _output_format(output_format), _parameters(), _confidence(0.0)
{
    __model = this;

    /* Initialize globals */

    open_arrival_count		= 0;
    max_service    		= 0.0;
    total_tasks    		= 0;
    client_init_count 		= 0;	/* For auto blocking (-C)	*/
}


Model::~Model()
{
    std::for_each( Processor::__processors.begin(), Processor::__processors.end(), []( Processor * processor ){ delete processor; } );
    Processor::__processors.clear();

    std::for_each( Group::__groups.begin(), Group::__groups.end(), []( Group * group ){ delete group; } );
    Group::__groups.clear();

    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), []( Task * task ){ delete task; } );
    Task::__tasks.clear();

    std::for_each( Entry::__entries.begin(), Entry::__entries.end(), []( Entry * entry ){ delete entry; } );
    Entry::__entries.clear();

    Activity::actConnections.clear();
    Activity::domToNative.clear();

    __model = nullptr;
}


/*
 * PARASOL mainline - initializes the simulation, processes command line
 * arguments, kick starts genesis, & drives the simulation.
 * The simulation itself runs as a child to simplify exit processing by
 * the simulator, redirection of input and output, and a host of other
 * things.
 */

int
Model::solve( solve_using run_function, const std::filesystem::path& input_file_name, LQIO::DOM::Document::InputFormat input_format, const std::filesystem::path& output_file_name, LQIO::DOM::Document::OutputFormat output_format, const LQIO::DOM::Pragma& pragmas )
{
    LQIO::io_vars.reset();

    /* load the model into the DOM document.  */

    unsigned int status = 0;
    LQIO::DOM::Document* document = LQIO::DOM::Document::load( input_file_name, input_format, status, false );

    /* Make sure we got a document */

    if ( document == nullptr || LQIO::io_vars.anError() ) return INVALID_INPUT;

    if ( LQIO::Spex::input_variables().empty() ) {
	if ( LQIO::Spex::__no_header ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --no-header is ignored for " << input_file_name << "." << std::endl;
	}
	if ( LQIO::Spex::__print_comment ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --print-comment is ignored for " << input_file_name << "." << std::endl;
	}
    }

    Model model( document, input_file_name, output_file_name, output_format );
    LQX::Program * program = nullptr;
    FILE * output = nullptr;
    try { 
	document->mergePragmas( pragmas.getList() );       /* Save pragmas */
	if ( !model.prepare() ) throw std::runtime_error( "Model::prepare" );

#if BUG_313
	extend();			/* convert entry think times	*/
#endif

	/* We can simply run if there's no control program */

	program = document->getLQXProgram();
	if ( program ) {
	    /* Attempt to run the program */
	    document->registerExternalSymbolsWithProgram(program);

	    if ( print_lqx ) {
		program->print( std::cerr );
	    }
	    
	    LQX::Environment * environment = program->getEnvironment();
	    environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, run_function, &model));
	    LQIO::RegisterBindings(environment, document);
	
	    if ( !output_file_name.empty() && output_file_name != "-" ) {
	        output = fopen( output_file_name.string().c_str(), "w" );
		if ( !output ) {
		    runtime_error( LQIO::ERR_CANT_OPEN_FILE, output_file_name.c_str(), strerror( errno ) );
		    status = FILEIO_ERROR;
		} else {
		    environment->setDefaultOutput( output );	/* Default is stdout */
		}
	    }

	    if ( status == 0 ) {
		/* Invoke the LQX program itself */
		if ( !program->invoke() ) {		/* Run simulation	*/
		    LQIO::runtime_error( LQIO::ERR_LQX_EXECUTION, input_file_name.c_str() );
		    status = INVALID_INPUT;
		} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
		    /* There was no call to solve the LQX */
		    LQIO::runtime_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, input_file_name.c_str() );
		    std::vector<LQX::SymbolAutoRef> args;
		    SolverInterface::Solve::implicitSolve = true;
		    environment->invokeGlobalMethod("solve", &args);
		}
	    }
	
	} else {
	    /* There is no control flow program, check for $-variables */
	    if ( document->getSymbolExternalVariableCount() != 0 ) {
		LQIO::runtime_error( LQIO::ERR_LQX_VARIABLE_RESOLUTION, input_file_name.c_str() );
		status = INVALID_INPUT;
	    } else if ( !model.start() ) {		/* Run simulation	*/
		status = INVALID_OUTPUT;
	    }
	}
    }
    catch ( const std::domain_error& e ) {
	status = INVALID_INPUT;
    }
    catch ( const std::runtime_error& e ) {
	status = INVALID_INPUT;
    }

    /* Clean up */
    
    if ( output ) fclose( output );
    if ( program ) delete program;
    if ( document ) delete document;
    
    return status;
}


/*
 * Step 2: convert DOM into lqn entities.  Runs outside of simulation.
 */

bool
Model::prepare()
{
    if ( __print_interval == 0 ) {
	__print_interval = _document->getModelPrintIntervalValue();
    }
    Pragma::set( _document->getPragmaList() );
    LQIO::io_vars.severity_level = Pragma::__pragmas->severity_level();
    LQIO::Spex::__print_comment = !Pragma::__pragmas->spex_comment();
    LQIO::Spex::__no_header = !Pragma::__pragmas->spex_header();


    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */

    const std::map<std::string,LQIO::DOM::Processor*>& processorList = _document->getProcessors();
    std::for_each( processorList.begin(), processorList.end(), Processor::add );

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1.5: Add Groups] */

    const std::map<std::string,LQIO::DOM::Group*>& groups = _document->getGroups();
    std::for_each( groups.begin(), groups.end(), Group::add );

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */

    /* In the DOM, tasks have entries, but here entries need to go first */
    const std::map<std::string,LQIO::DOM::Task*>& tasks = _document->getTasks();

    /* Add all of the tasks we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator nextTask = tasks.begin(); nextTask != tasks.end(); ++nextTask ) {
	LQIO::DOM::Task* dom = nextTask->second;
	const std::vector<LQIO::DOM::Entry*>& entries = dom->getEntryList();
	const std::map<std::string,LQIO::DOM::Activity*>& activities = dom->getActivities();
	
	/* Now we can go ahead and add the task */
	Task* newTask = Task::add(dom);

	/* Add the entries so we can reverse them */
	std::vector<LQIO::DOM::Entry*> activityEntries;
	for ( std::vector<LQIO::DOM::Entry*>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    const_cast<std::vector<Entry *>&>(newTask->entries()).push_back( Entry::add( *entry, newTask ) );
	    if ((*entry)->getStartActivity() != nullptr) {
		activityEntries.push_back(*entry);
	    }
	}

	/* Add activities for the task (all of them) */
	std::for_each( activities.begin(), activities.end(), [=]( auto& activity ){ newTask->add_activity(const_cast<LQIO::DOM::Activity*>(activity.second)); } );

	/* Set all the start activities */
	std::for_each( activityEntries.begin(), activityEntries.end(), [=]( auto entry ){ newTask->set_start_activity(entry); } );
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 3: Add Calls/Phase Parameters] */

    /* Add all of the calls for all phases to the system */
    const std::map<std::string,LQIO::DOM::Entry*>& allEntries = _document->getEntries();
    for ( std::map<std::string,LQIO::DOM::Entry*>::const_iterator nextEntry = allEntries.begin(); nextEntry != allEntries.end(); ++nextEntry ) {
	LQIO::DOM::Entry* entry = nextEntry->second;
	Entry* newEntry = Entry::find(entry->getName());
	if ( newEntry == nullptr ) continue;

	/* Go over all of the entry's phases and add the calls */
	for (unsigned p = 1; p <= entry->getMaximumPhase(); ++p) {
	    LQIO::DOM::Phase* phase = entry->getPhase(p);
	    const std::vector<LQIO::DOM::Call*>& originatingCalls = phase->getCalls();

	    /* Add all of the calls to the system */
	    std::for_each( originatingCalls.begin(), originatingCalls.end(), [=]( LQIO::DOM::Call * call ){ newEntry->add_call( p, call ); } );			/* Add the call to the system */
	}

	/* Add in all of the P(frwd) calls */
	const std::vector<LQIO::DOM::Call*>& forwarding = entry->getForwarding();
	std::for_each( forwarding.begin(), forwarding.end(), [=]( LQIO::DOM::Call * call ){ Entry* targetEntry = Entry::find(call->getDestinationEntry()->getName()); newEntry->add_forwarding(targetEntry, call ); } );

	/* Add reply */

	if ( newEntry->is_regular() ) {
	    const Task * aTask = newEntry->task();
	    if ( aTask->type() != Task::Type::CLIENT && aTask->type() != Task::Type::OPEN_ARRIVAL_SOURCE ) {
		newEntry->_phase[0].act_add_reply( newEntry );
	    }
	}

    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 4: Add Calls/Lists for Activities] */

    /* Go back and add all of the lists and calls now that activities all exist */
    for ( std::set<Task *>::const_iterator task = Task::__tasks.begin(); task != Task::__tasks.end(); ++task ) {
	std::for_each( (*task)->activities().begin(), (*task)->activities().end(), []( Activity * activity ){ activity->add_calls().add_reply_list().add_activity_lists(); } );
    }

    /* Use the generated connections list to finish up */
    complete_activity_connections();

    /* Tell the user that we have finished */
    return !LQIO::io_vars.anError();
}



#if BUG_313
/*
 * Add a think server if there are any entries with think times.
 */

/* static */ void
Model::extend()
{
    for ( std::set<Task *>::const_iterator task = Task::__tasks.begin(); task != Task::__tasks.end(); ++task ) {
	if ( (*task)->has_think_time() ) {
	}
    }
}
#endif


/*
#if !HAVE_PARASOL
 * Called from Model::start to create the processor mutexs and task threads.
 * Everything should be running with this method returns.
#else
 * Called from ps_genesis() by ps_run_parasol().
 *
 * Create all of the parasol tasks instances after the simulation has
 * started.  Some construction is needed after the simulation has
 * initialized and started simply because we need run-time info.
 * Model::create() will call Task::initialize() which will call
 * XXX::initialize(). XXX:configure() is called from ::start() which
 * is called before ps_run_parasol().
#endif
 */

bool
Model::create()
{
#if HAVE_PARASOL
    std::for_each( Processor::__processors.begin(), Processor::__processors.end(), std::mem_fn( &Processor::create ) );
#endif
    std::for_each( Group::__groups.begin(), Group::__groups.end(), std::mem_fn( &Group::create ) );
    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), std::mem_fn( &Task::create ) );

    if ( std::none_of( Task::__tasks.begin(), Task::__tasks.end(), std::mem_fn( &Task::is_reference_task ) ) && open_arrival_count == 0 ) {
	LQIO::runtime_error( LQIO::ERR_NO_REFERENCE_TASKS );
    }

    if ( LQIO::io_vars.anError() ) return false;		/* Early termination */

    if ( _parameters._block_period < max_service * 100 && !no_execute_flag ) {
	(void) fprintf( stderr, "%s: ***ERROR*** Simulation duration is too small!\n\tThe largest service time period is %G\n\tIncrease the run time to %G\n",
			LQIO::io_vars.toolname(),
			max_service,
			max_service * 100 );
	if ( !debug_flag ) {
	    exit( INVALID_ARGUMENT );
	}
    }

    return true;
}

/* -------------------------------------------------------------------- */
/* Stuff called from runlqx.cc						*/
/* -------------------------------------------------------------------- */

/*
 * Solve the model.
 */

bool
Model::start()
{
    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), std::mem_fn( &Task::configure ) );

    /*
     * This will compute the minimum service time for each entry.  Note thate XXX::initialize() is called
     * through Model::run()
     */

    std::deque<Entry *> stack;
    const double client_cycle_time = std::accumulate( Entry::__entries.begin(), Entry::__entries.end(), 0.0, [=]( double l, Entry * r ) { return l + r->compute_minimum_service_time( const_cast<std::deque<Entry *>&>(stack) ); } );

    /* Which we can use here... */

    _parameters.set( _document->getPragmaList(), client_cycle_time );
    Random::seed( _parameters._seed );

    deferred_exception = false;

    _start_time.init();

#if HAVE_PARASOL
    int simulation_flags= 0x00;
    if (debug_interactive_stepping) {
	simulation_flags = RPF_STEP; 	/* tomari quorum */
    }
    if (trace_driver) {
	simulation_flags = simulation_flags | RPF_TRACE|RPF_WARNING;
    } else {
	simulation_flags = simulation_flags | RPF_WARNING;
    }

    ps_run_parasol( _parameters._run_time+1.0, _parameters._seed, simulation_flags );	/* Calls ps_genesis */
#else
    run();
#endif

    /*
     * Run completed.
     * Print final results
     */

    /* Parasol statistics if desired */

    if ( raw_stat_flag ) {
	print( std::cout );
//	print_raw_stats( stddbg );
    }

    if ( !deferred_exception && LQIO::io_vars.anError() == 0 ) {
	_document->setResultDescription();
	_document->print( _output_file_name, _document->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix : std::string(""), _output_format, rtf_flag );

	if ( _confidence > _parameters._precision && _parameters._precision > 0.0 ) {
	    LQIO::runtime_error( ADV_PRECISION, _parameters._precision, _parameters._block_period * number_blocks + _parameters._initial_delay, _confidence );
	}
	for ( std::set<Task *>::const_iterator task = Task::__tasks.begin(); task != Task::__tasks.end(); ++task ) {
	    const auto entries = (*task)->entries();
	    std::for_each( entries.begin(), entries.end(), []( const Entry * entry ){ if ( entry->has_lost_messages() ) { entry->getDOM()->runtime_error( LQIO::ADV_MESSAGES_DROPPED ); } } );
	}
    }

#if STACK_TESTING
    if ( check_stacks ) {
#if HAVE_MCHECK
	mcheck_check_all();
#endif
	fprintf( stderr, "%s: ", _input_file_name.string().c_str() );
#if HAVE_PARASOL
	test_all_stacks();
#endif
    }
#endif

#if HAVE_PARASOL
    for ( unsigned i = 1; i <= total_tasks; ++i ) {
	Instance::Instance * ip = object_tab.at(i);
	if ( !ip ) continue;
	delete ip;
	object_tab[i] = nullptr;
    }
#endif

    if ( deferred_exception ) {
	throw std::domain_error( "invalid parameter" );
    }
    return LQIO::io_vars.anError() == 0;
}



/*
 * Read result files only.  LQX print uses these results.
 */

bool
Model::reload()
{
    /* Default mapping */

    LQIO::Filename directory_name( getOutputFileName(), "d" );		/* Get the base file name */

    if ( access( directory_name.str().c_str(), R_OK|W_OK|X_OK ) < 0 ) {
	runtime_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name.str().c_str(), strerror( errno ) );
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    }

    unsigned int errorCode = 0;
    if ( !_document->loadResults( directory_name(), _input_file_name,
				  SolverInterface::Solve::customSuffix, _output_format, errorCode ) ) {
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    } else {
	return _document->getResultValid();
    }
}


/*
 * Run the simulation for invalid or missing results.  Otherwise, reload existing results.
 */

bool
Model::restart()
{
    try {
	if ( reload() ) {
	    return true;
	} else {
	    return start();
	}
    }
    catch ( const LQX::RuntimeException& e ) {
	return start();
    }
}

/* -------------------------------------------------------------------- */
/* Parasol interface							*/
/* -------------------------------------------------------------------- */
#if HAVE_PARASOL
/*
 * Progenator task.
 */

void
ps_genesis(void *)
{
    Model::__genesis_task_id = ps_myself;
    if (!Model::__model->run() ) {
	LQIO::runtime_error( ERR_INITIALIZATION_FAILED );
    }
    ps_suspend( ps_myself );
}
#endif

/*
 *  Run the simulation.  Called from ps_genesis.  Create all parasol
 *  processors and tasks (instances) through Model::create(), then
 *  start all the tasks.
 */

bool
Model::run()
{
    bool rc = false;
    if ( verbose_flag ) {
	(void) putc( 'C', stderr );		/* Constructing */
    }

#ifdef LQX_DEBUG
    printf( "In Model::run() ps_now: %g\n", Processor::now() );
    fflush( stdout );
#endif

    try {
	if ( !create() ) {
	    rc = false;
	} else if ( no_execute_flag ) {
	    rc = true;
	} else {
	    /*
	     * Start all of the tasks.
	     */

	    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), []( Task * task ){ task->run(); } );

	    if ( _parameters._initial_delay ) {
		if ( verbose_flag ) {
		    (void) putc( 'I', stderr );
		}
		sleep( _parameters._initial_delay );
		if ( deferred_exception ) throw std::runtime_error( "terminating" );
	    }

	    /*
	     * Reset all counters (stuff during "initial_delay" is ignored)
	     */

	    reset_stats();

	    /*
	     * Accumulate statistical data.
	     */

	    bool valid = false;
	    for ( number_blocks = 1; !valid && number_blocks <= _parameters._max_blocks; number_blocks += 1 ) {

		if ( verbose_flag ) {
		    (void) fprintf( stderr, " %c", "0123456789"[number_blocks%10] );
		}

		sleep( _parameters._block_period );
		accumulate_data();

		if ( number_blocks > 2 ) {
		    _confidence = rms_confidence();
		    if ( verbose_flag ) {
			(void) fprintf( stderr, "[%.2g]", _confidence );
			if ( number_blocks % 10 == 0 ) {
			    (void) putc( '\n', stderr );
			}
		    }
		    valid = _confidence <= _parameters._precision;
		}

		insertDOMResults();

		if ( __enable_print_interval && !valid && __print_interval > 0 && number_blocks % __print_interval == 0 ) {
		    print_intermediate();
		}

		if ( deferred_exception ) throw std::runtime_error( "terminating" );
	    }

	    if ( verbose_flag || trace_driver ) (void) putc( '\n', stderr );

	    rc = true;
	}
    }
    catch ( const std::domain_error& e ) {
	rc = false;
	deferred_exception = true;
    }
    catch ( const std::runtime_error& e ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": runtime error: " << e.what() << std::endl;
	rc = false;
	deferred_exception = true;
    }

#if HAVE_PARASOL
    /* Force simulation to terminate. */

    ps_run_time = -1.1;
    ps_sleep(1.0);

    /* Remove instances */

    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), std::mem_fn( &Task::stop ) );
#endif

    return rc;
}

/*----------------------------------------------------------------------*/
/*			  Statistics Functions.				*/
/*----------------------------------------------------------------------*/

/*
 * Accumulate data from this run.
 */

void
Model::accumulate_data()
{
    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), std::mem_fn( &Task::accumulate_data ) );
    std::for_each( Group::__groups.begin(), Group::__groups.end(), std::mem_fn( &Group::accumulate_data ) );
    std::for_each( Processor::__processors.begin(), Processor::__processors.end(), std::mem_fn( &Processor::accumulate_data ) );
}



/*
 * Reset data from this run.
 */

void
Model::reset_stats()
{
    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), std::mem_fn( &Task::reset_stats ) );
    std::for_each( Group::__groups.begin(), Group::__groups.end(), std::mem_fn( &Group::reset_stats ) );
    std::for_each( Processor::__processors.begin(), Processor::__processors.end(), std::mem_fn( &Processor::reset_stats ) );
}



/*
 * Check waiting time statistics to see if we meet termination
 * precision.  Normalize confidence and convert to percentage.
 */

double
Model::rms_confidence()
{
    double sum_sqr = 0.0;
    double n = 0;

    for ( std::set<Task *>::const_iterator task = Task::__tasks.begin(); task != Task::__tasks.end(); ++task ) {
	if ( (*task)->type() == Task::Type::OPEN_ARRIVAL_SOURCE || (*task)->is_async_inf_server() ) continue;		/* Skip. */

	for ( std::vector<Entry *>::const_iterator nextEntry = (*task)->entries().begin(); nextEntry != (*task)->entries().end(); ++nextEntry ) {
	    double temp = normalized_conf95( (*nextEntry)->r_cycle );
	    if ( temp > 0 ) {
		sum_sqr += ( temp * temp );
		n += 1;
	    }
	}
    }
    return n > 0 ? sqrt( sum_sqr / n ) : 0.0;
}

double
Model::normalized_conf95( const Result& stat )
{
    double temp = stat.mean();
    if ( temp ) {
	return sqrt(stat.variance()) * Result::conf95( number_blocks ) * 100.0 / temp;
    }
    return -1.0;
}

/*----------------------------------------------------------------------*/
/*			  Output Functions.				*/
/*----------------------------------------------------------------------*/

/*
 * Set up file descriptors and call proper output routine.
 */

void
Model::print_intermediate()
{
    _document->setResultConvergenceValue(_confidence)
	.setResultValid(_confidence <= _parameters._precision)
	.setResultIterations(number_blocks)
	.setResultDescription();

    _document->print( _output_file_name, SolverInterface::Solve::customSuffix, _output_format, rtf_flag, number_blocks );
}


/*
 * Human format statistics.
 */

std::ostream&
Model::print( std::ostream& output ) const
{
    static const unsigned int long_width = 99;
    static const unsigned int short_width = 69;

/*
  123456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789_
  ssssssssssssssssssssssssssssssssssssss tttttttt nnnnnnnnnnnn nnnnnnnnnnnn nnnnnnnnnnnn nnnnnnnn
  Name                                     Type       Mean        95% +/-      99% +/-   #Obs|Int
  ssssssssssssssssssssssssssssssssssssss tttttttt nnnnnnnnnnnn nnnnnnnn
  Name                                     Type       Mean     #Obs|Int
*/
    if ( number_blocks > 2 ) {
	output << "Blocked simulation statistics for " << _input_file_name << std::endl
	       << "\tTime = " << Processor::now() << ".  Period = " << _parameters._block_period << std::endl << std::endl
	       << "Name                                     Type       Mean        95%% +/-      99%% +/-   #Obs/Int" << std::endl
	       << std::setw( long_width ) << std::setfill( '-' ) << "-" << std::endl;
    } else {
	output << "Simulation statistics for " << _input_file_name << std::endl
	       << "\ttime = " << Processor::now() << std::endl << std::endl
	       << "Name                                     Type       Mean     #Obs/Int" << std::endl
	       << std::setw( short_width ) << std::setfill( '-' ) << "-" << std::endl;
    }

    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), [&]( const Task * task ){ task->print( output ); } );

    const int fill = (((number_blocks > 2) ? long_width : short_width) - 23) / 2;
    output << std::setw( fill ) << std::setfill( '-' ) << "-" << " Processor Information " << std::setw( fill ) << std::setfill( '-' ) << "-" << std::endl;

    std::for_each( Processor::__processors.begin(), Processor::__processors.end(), [&]( const Processor * processor ){ processor->print( output ); } );

    output << std::endl << std::endl;
    return output;
}


/*
 * Go through data structures and insert results into the DOM
 */

void
Model::insertDOMResults()
{
    /* Insert general information about simulation run */

    LQIO::DOM::CPUTime stop_time;
    stop_time.init();
    stop_time -= _start_time;
    stop_time.insertDOMResults( *_document );

    _document->setResultConvergenceValue(_confidence)
	.setResultValid( _confidence <= _parameters._precision || _parameters._precision == 0.0 )
	.setResultIterations(number_blocks)
	.setResultSolverInformation()
	.setResultPlatformInformation();

    std::for_each( Task::__tasks.begin(), Task::__tasks.end(), std::mem_fn( &Task::insertDOMResults ) );
    std::for_each( Group::__groups.begin(), Group::__groups.end(), std::mem_fn( &Group::insertDOMResults ) );
    std::for_each( Processor::__processors.begin(), Processor::__processors.end(), std::mem_fn( &Processor::insertDOMResults ) );
}

/* ------------------------------------------------------------------------ */
/*
 * set the simulation run time parameters.
 * Full auto will derive based on service times of tasks.
 */

void Model::simulation_parameters::set( const std::map<std::string,std::string>& pragmas, double minimum_cycle_time )
{
    if ( !set( _seed, pragmas, LQIO::DOM::Pragma::_seed_value_ ) ) {
	/* Not set... Randomize. */
	_seed = (long)time( (time_t *)0 );
	std::stringstream value;
	value << _seed;
	__model->_document->addPragma(LQIO::DOM::Pragma::_seed_value_,value.str());	/* set value in DOM */

    }
    unsigned long initial_loops = 0;
    if ( set( _precision, pragmas, LQIO::DOM::Pragma::_precision_ ) ) {
	/* if initial loops NOT set and run-time set then -A, otherwise -C */

	if ( !set( initial_loops, pragmas, LQIO::DOM::Pragma::_initial_loops_ ) && set( _run_time, pragmas, LQIO::DOM::Pragma::_run_time_ ) ) {
	    set( _initial_delay, pragmas, LQIO::DOM::Pragma::_initial_delay_ );
	    return;		 // run_time set;
	} else {
	    /* -C */
	    _max_blocks = MAX_BLOCKS;
	    _initial_delay = minimum_cycle_time * initial_loops * 2;
	    if ( !set( _run_time, pragmas, LQIO::DOM::Pragma::_run_time_ ) ) {
		_block_period = _initial_delay * 100;
	    } else {
		_block_period = (_run_time - _initial_delay) / _max_blocks;
		return;		 // run_time set;
	    }
	}

    } else if ( set( _max_blocks, pragmas, LQIO::DOM::Pragma::_max_blocks_ ) ) {
	/* -B */
	if ( !set( _block_period, pragmas, LQIO::DOM::Pragma::_block_period_ ) ) {
	    _block_period = DEFAULT_TIME;
	}
	set( _initial_delay, pragmas, LQIO::DOM::Pragma::_initial_delay_ );

    } else if ( set( _block_period, pragmas, LQIO::DOM::Pragma::_block_period_ ) ) {
	/* -A */
	if ( !set( _precision, pragmas, LQIO::DOM::Pragma::_precision_ ) ) {
	    _precision = 1.0;
	}
	set( _initial_delay, pragmas, LQIO::DOM::Pragma::_initial_delay_ );

    } else if ( set( _run_time, pragmas, LQIO::DOM::Pragma::_run_time_ ) ) {
	/* -T */
	_max_blocks = 1;
	return;			// run_time set.
    }

    /* Set run_time. */
    _run_time = _initial_delay + _max_blocks * _block_period;
}


bool Model::simulation_parameters::set( double& parameter, const std::map<std::string,std::string>& pragmas, const std::string& value )
{
    std::map<std::string,std::string>::const_iterator i = pragmas.find( value );
    if ( i != pragmas.end() ) {
	char * endptr = nullptr;
	parameter = std::strtod( i->second.c_str(), &endptr );
	if ( *endptr != '\0' ) {
	    throw std::invalid_argument( value + "=" + i->second );
	}
	return true;
    } else {
	return false;
    }
}


bool Model::simulation_parameters::set( unsigned long& parameter, const std::map<std::string,std::string>& pragmas, const std::string& value )
{
    std::map<std::string,std::string>::const_iterator i = pragmas.find( value );
    if ( i != pragmas.end() ) {
	char * endptr = nullptr;
	parameter = std::strtol( i->second.c_str(), &endptr, 10 );
	if ( *endptr != '\0' ) {
	    throw std::invalid_argument( value + "=" + i->second );
	}
	return true;
    } else {
	return false;
    }
}
