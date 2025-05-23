/* -*- c++ -*-
 * $Id: model.cc 17542 2025-04-15 18:27:31Z greg $
 *
 * Layer-ization of model.  The basic concept is from the reference
 * below.  However, model partioning is more complex than task vs device.
 *
 *     author =  "Rolia, Jerome Alexander",
 *     title =   "Predicting the Performance of Software Systems",
 *     school =  "Univerisity of Toronto",
 *     year =    1992,
 *     address = "Toronto, Ontario, Canada.  M5S 1A1",
 *     month =   jan,
 *     note =    "Also as: CSRI Technical Report CSRI-260",
 *
 * Users create and solve models in three steps:
 * 1) Call the appropriate constructor.
 * 2) Call initialize (after lqx starts executing)
 *    a) Call extend (add replicas, think server...)
 *    b) Call generate().  (adds entities to submodels.)
 *       i)  topologicalSort() to sort to visit all nodes and sort into layers.
 *           Activitly lists checked and are set up during the sorting process.
 *       ii) addToSubmodel() to add server stations to the basic model.
 *    c) Call configure (dimension arrays)
 *    d) Call initializeSubmodels (initialize service times, etc...)
 * 3) Call solve()
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * ------------------------------------------------------------------------
 */

#include "lqns.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <functional>
#include <iostream>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <lqio/dom_bindings.h>
#include <lqio/dom_document.h>
#include <lqio/error.h>
#include <lqio/filename.h>
#include <lqio/input.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
#include <mva/fpgoop.h>
#include <mva/mva.h>
#include <mva/open.h>
#include <mva/server.h>
#include "activity.h"
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "flags.h"
#include "generate.h"
#include "group.h"
#include "interlock.h"
#include "model.h"
#include "option.h"
#include "overtake.h"
#include "pragma.h"
#include "processor.h"
#include "report.h"
#include "runlqx.h"
#include "submodel.h"
#include "synmodel.h"
#include "task.h"
#include "variance.h"

unsigned int Model::__print_interval = 0;
Processor * Model::__think_server = nullptr;
unsigned Model::__sync_submodel = 0;

LQIO::DOM::Document::InputFormat Model::__input_format = LQIO::DOM::Document::InputFormat::AUTOMATIC;

std::set<Processor *,Model::lt_replica<Processor>> Model::__processor;
std::set<Group *,Model::lt_replica<Group>> Model::__group;
std::set<Task *,Model::lt_replica<Task>> Model::__task;
std::set<Entry *,Model::lt_replica<Entry>> Model::__entry;

/*----------------------------------------------------------------------*/
/*                           Factory Methods                            */
/*----------------------------------------------------------------------*/

/*
 * Open output files, solve, and print.
 */

int
Model::solve( solve_using solve_function, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat )
{
    /* Loading the model */
    LQIO::DOM::Document* document = Model::load(inputFileName,outputFileName);

    /* Make sure we got a document */

    if ( document == nullptr  ) {
	return INVALID_INPUT;
    } else if ( LQIO::io_vars.anError() ) {
	delete document;
	return INVALID_INPUT;
    }

    document->mergePragmas( pragmas.getList() );	/* Save pragmas -- prepare will process */
    if ( Model::prepare(document) == false ) {
	delete document;
	return INVALID_INPUT;
    }

    if ( LQIO::Spex::input_variables().empty() ) {
	if ( LQIO::Spex::__no_header ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --no-header is ignored for " << inputFileName << "." << std::endl;
	}
	if ( LQIO::Spex::__print_comment ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --print-comment is ignored for " << inputFileName << "." << std::endl;
	}
    }

    /* declare Model * at this scope but don't instantiate due to problems with LQX programs and registering external symbols*/
    Model * model = Model::create( document, inputFileName, outputFileName, outputFormat );
    int status = 0;

    /* We can simply run if there's no control program */
    LQX::Program * program = document->getLQXProgram();
    if ( program == nullptr ) {

	/* There is no control flow program, check for $-variables */
	if (document->getSymbolExternalVariableCount() != 0) {
	    LQIO::runtime_error( LQIO::ERR_LQX_VARIABLE_RESOLUTION, inputFileName.c_str() );
	    status = INVALID_INPUT;
	} else {
	    /* Make sure values are up to date */
	    model->recalculateDynamicValues();

	    /* Simply invoke the solver for the current DOM state */

	    try {
		if ( model->check() && model->initialize() ) {
		    if ( Pragma::spexComment() ) {	// Not spex/lqx, so output on stderr.
			std::cerr << inputFileName << ": " << document->getModelComment() << std::endl;
		    }
		    model->compute();
		} else {
		    status = INVALID_INPUT;
		}
	    }
	    catch ( const std::domain_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": domain error - " << e.what() << std::endl;
		status = INVALID_INPUT;
	    }
	    catch ( const std::range_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": range error - " << e.what() << std::endl;
		status = INVALID_OUTPUT;
	    }
	    catch ( const floating_point_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": floating point error - " << e.what() << std::endl;
		status = INVALID_OUTPUT;
	    }
	    catch ( const std::runtime_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": run time error - " << e.what() << std::endl;
		status = INVALID_INPUT;
	    }
	}

    } else {

	if ( Options::Trace::verbose() ) {
	    std::cerr << "Compile LQX..." << std::endl;
	}

	/* Attempt to run the program */
	document->registerExternalSymbolsWithProgram( program );

	if ( flags.print_lqx ) {
	    program->print( std::cerr );
	}

	LQX::Environment * environment = program->getEnvironment();
	environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, solve_function, model));
	LQIO::RegisterBindings(environment, document);

	FILE * output = nullptr;
	if ( !outputFileName.empty() && outputFileName != "-" ) {
	  output = fopen( outputFileName.string().c_str(), "w" );
	    if ( !output ) {
		LQIO::runtime_error( LQIO::ERR_CANT_OPEN_FILE, outputFileName.c_str(), strerror( errno ) );
		status = FILEIO_ERROR;
	    } else {
		environment->setDefaultOutput( output );      /* Default is stdout */
	    }
	}

	if ( status == 0 ) {
	    /* Invoke the LQX program itself */
	    if ( !program->invoke() ) {
		LQIO::runtime_error( LQIO::ERR_LQX_EXECUTION, inputFileName.c_str() );
		status = INVALID_INPUT;
	    } else if ( !SolverInterface::Solve::solveCallViaLQX ) {
		/* There was no call to solve the LQX */
		LQIO::runtime_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, inputFileName.c_str() );
		std::vector<LQX::SymbolAutoRef> args;
		environment->invokeGlobalMethod("solve", &args);
	    }
	}

	if ( output ) fclose( output );
	delete program;
    }

    /* Clean things up */
    if ( model ) delete model;
    delete document;

    return status;
}




/*
 * Step 1: load model.
 */

LQIO::DOM::Document*
Model::load( const std::filesystem::path& input_filename, const std::filesystem::path& output_filename )
{
    if ( Options::Trace::verbose() ) std::cerr << "Load: " << input_filename << "..." << std::endl;

    /*
     * Initialize everything that needs it before parsing
     */

    set_fp_ok( false );			// Reset floating point. -- stop on overflow?
    if ( matherr_disposition == fp_exception_reporting::IMMEDIATE_ABORT ) {
	set_fp_abort();
    }

    LQIO::io_vars.reset();
    Entry::reset();

    /*
     * Read input file and parse it.
     */

    unsigned errorCode = 0;

    /* Attempt to load in the document from the filename/ptr and configured io_vars */
    return LQIO::DOM::Document::load(input_filename, __input_format, errorCode, false);
}


/*
 * Step 2: convert DOM into lqn entities.
 */

bool
Model::prepare(const LQIO::DOM::Document* document)
{
    /* Tell the user that we are starting to load up */
    if ( Options::Trace::verbose() ) std::cerr << "Prepare: ..." << std::endl;

    /* Update the pragma list from the document (merge), then set globals here as this has to be done prior to runlqx() */
    Pragma::set( document->getPragmaList() );
    LQIO::io_vars.severity_level = Pragma::severityLevel();
    LQIO::Spex::__no_header = !Pragma::spexHeader();
    LQIO::Spex::__print_comment = Pragma::spexComment();
    MVA::MOL_multiserver_underrelaxation = Pragma::molUnderrelaxation();

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */

    const std::map<std::string,LQIO::DOM::Processor *>& processors = document->getProcessors();
    std::for_each( processors.begin(), processors.end(), Processor::create );

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1.5: Add Groups] */

    const std::map<std::string,LQIO::DOM::Group*>& groups = document->getGroups();
    std::for_each( groups.begin(), groups.end(), Group::create );

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */

    /* In the DOM, tasks have entries, but here entries need to go first */
    const std::map<std::string,LQIO::DOM::Task*>& tasks = document->getTasks();
    std::vector<Activity*> activityList;

    /* Add all of the processors we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator nextTask = tasks.begin(); nextTask != tasks.end(); ++nextTask ) {
	LQIO::DOM::Task* task = nextTask->second;

	/* Prepare to iterate over all of the entries */
	std::vector<Entry*> entries;
	std::vector<LQIO::DOM::Entry*> activityEntries;

	/* Add the entries so we can reverse them */
	for ( std::vector<LQIO::DOM::Entry*>::const_iterator nextEntry = task->getEntryList().begin(); nextEntry != task->getEntryList().end(); ++nextEntry ) {
	    entries.push_back( Entry::create( *nextEntry, entries.size() ) );
	    if ((*nextEntry)->getStartActivity() != nullptr) {
		activityEntries.push_back(*nextEntry);
	    }
	}

	/* Now we can go ahead and add the task */
	Task* newTask = Task::create(task, entries);

	/* Add activities for the task (all of them) */
	const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
	for (std::map<std::string,LQIO::DOM::Activity*>::const_iterator activity = activities.begin(); activity != activities.end(); ++activity) {
	    activityList.push_back(add_activity(newTask, const_cast<LQIO::DOM::Activity*>(activity->second)));
	}

	/* Set all the start activities */
	for ( std::vector<LQIO::DOM::Entry*>::const_iterator nextEntry = activityEntries.begin(); nextEntry != activityEntries.end(); ++nextEntry) {
	    set_start_activity(newTask, *nextEntry);
	}
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 3: Add Calls/Phase Parameters] */

    /* Add all of the calls for all phases to the system */
    /* We use this to add all calls */
    const std::map<std::string,LQIO::DOM::Entry*>& allEntries = document->getEntries();
    for ( std::map<std::string,LQIO::DOM::Entry*>::const_iterator nextEntry = allEntries.begin(); nextEntry != allEntries.end(); ++nextEntry ) {
	LQIO::DOM::Entry* entry = nextEntry->second;
	Entry* newEntry = Entry::find(entry->getName());
	if ( newEntry == nullptr ) continue;

	newEntry->setEntryInformation( entry );

	/* Go over all of the entry's phases and add the calls */
	for (unsigned p = 1; p <= entry->getMaximumPhase(); ++p) {
	    LQIO::DOM::Phase* phase = entry->getPhase(p);
	    const std::vector<LQIO::DOM::Call*>& originatingCalls = phase->getCalls();

	    /* Add all of the calls to the system */
	    std::for_each( originatingCalls.begin(), originatingCalls.end(), Call::Create( newEntry, p ) );

	    /* Set the phase information for the entry */
	    newEntry->setDOM(p, phase);
	}

	/* Add in all of the P(frwd) calls */
	const std::vector<LQIO::DOM::Call*>& forwarding = entry->getForwarding();
	for ( std::vector<LQIO::DOM::Call*>::const_iterator nextFwd = forwarding.begin(); nextFwd != forwarding.end(); ++nextFwd ) {
	    Entry* targetEntry = Entry::find((*nextFwd)->getDestinationEntry()->getName());
	    newEntry->setForwardingInformation(targetEntry, *nextFwd );
	}
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 4: Add Calls/Lists for Activities] */

    /* Go back and add all of the lists and calls now that activities all exist */

    for (std::vector<Activity*>::iterator activity = activityList.begin(); activity != activityList.end(); ++activity) {
	(*activity)->add_calls().add_reply_list().add_activity_lists();
    }

    /* Use the generated connections list to finish up */
    Activity::completeConnections();
    std::for_each( __task.begin(), __task.end(), std::mem_fn( &Task::linkForkToJoin ) );	/* Link forks to joins		*/

    /* Tell the user that we have finished */
    return true;
}



/*
 * Dynamic Updates / Late Finalization
 * In order to integrate LQX's support for model changes we need to
 * have a way of re-calculating what used to be static for all
 * dynamically editable values
 */

void
Model::recalculateDynamicValues()
{
    setModelParameters();
    std::for_each( __processor.begin(), __processor.end(), std::mem_fn( &Processor::recalculateDynamicValues ) );
    std::for_each( __group.begin(), __group.end(), std::mem_fn( &Group::recalculateDynamicValues ) );
    std::for_each( __task.begin(), __task.end(), std::mem_fn( &Task::recalculateDynamicValues ) );
}



/*
 * Step 3: Factory method for creating the layers and initializing the MVA submodels
 */

Model *
Model::create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat )
{
    static const std::map<const Pragma::Layering, create_func> create_funcs = {
	{ Pragma::Layering::BATCHED,  			    &Batch_Model::create },
	{ Pragma::Layering::BACKPROPOGATE_BATCHED,  	    &BackPropogate_Batch_Model::create },
	{ Pragma::Layering::METHOD_OF_LAYERS,  		    &MOL_Model::create },
	{ Pragma::Layering::BACKPROPOGATE_METHOD_OF_LAYERS, &BackPropogate_MOL_Model::create },
	{ Pragma::Layering::SRVN,  			    &SRVN_Model::create },
	{ Pragma::Layering::SQUASHED,  			    &Squashed_Model::create },
	{ Pragma::Layering::HWSW,  			    &HwSw_Model::create }
    };

    Activity::clearConnectionMaps();
    MVA::__bounds_limit = Pragma::tau();

    /*
     * Fold, Mutilate and Spindle before main loop processing in solve.c
     * disable model checking and expansion at this stage with LQX programs
     */

    if ( Options::Trace::verbose() ) std::cerr << "Create: " << Pragma::getLayeringStr() << " layers..." << std::endl;

    create_func f = create_funcs.at(Pragma::layering());
    Model * model = (*f)( document, inputFileName, outputFileName, outputFormat );

    if ( !model ) throw std::runtime_error( "could not create model" );

    return model;
}



/*
 * Called when the model is about to be solved to set model parameters.
 */

void
Model::setModelParameters()
{
    /* The print interval can be set by option.cc */
    if ( __print_interval == 0 ) {
	__print_interval = getDOM()->getModelPrintIntervalValue();
    }
    if ( __print_interval != getDOM()->getModelPrintIntervalValue() ) {
	const_cast<LQIO::DOM::Document *>(getDOM())->setModelPrintInterval( new LQIO::DOM::ConstantExternalVariable( __print_interval ) );
    }

    if ( _iteration_limit == 0 ) {
	_iteration_limit = Pragma::iterationLimit() > 0 ? Pragma::iterationLimit() : getDOM()->getModelIterationLimitValue();
    }
    if ( _iteration_limit < 1 ) {
	LQIO::input_error( ADV_ITERATION_LIMIT, _iteration_limit, 50 );
	_iteration_limit =  50;
    }
    if ( _iteration_limit != getDOM()->getModelIterationLimitValue() ) {
	const_cast<LQIO::DOM::Document *>(getDOM())->setModelIterationLimit( new LQIO::DOM::ConstantExternalVariable( _iteration_limit ) );
    }

    _convergence_value = Pragma::convergenceValue() > 0. ? Pragma::convergenceValue() : getDOM()->getModelConvergenceValue();
    if ( _convergence_value <= 0. ) {
	LQIO::input_error( ADV_CONVERGENCE_VALUE, _convergence_value, 0.00001 );
	_convergence_value = 0.00001;
    } else if ( _convergence_value > 0.01 ) {
	LQIO::input_error( ADV_LARGE_CONVERGENCE_VALUE, _convergence_value );
    }
    if ( _convergence_value != getDOM()->getModelConvergenceValue() ) {
	const_cast<LQIO::DOM::Document *>(getDOM())->setModelConvergence( new LQIO::DOM::ConstantExternalVariable( _convergence_value ) );
    }
    
    _underrelaxation = Pragma::underrelaxation() > 0. ? Pragma::underrelaxation() : getDOM()->getModelUnderrelaxationCoefficientValue();
    if ( _underrelaxation <= 0.0 || 2.0 < _underrelaxation ) {
	LQIO::input_error( ADV_UNDERRELAXATION, _underrelaxation );
	_underrelaxation = 0.9;
    }
    if ( _underrelaxation != getDOM()->getModelUnderrelaxationCoefficientValue() ) {
	const_cast<LQIO::DOM::Document *>(getDOM())->setModelUnderrelaxationCoefficient( new LQIO::DOM::ConstantExternalVariable( _underrelaxation ) );
    }

    if ( Pragma::spexConvergence() > 0 ) {
	const_cast<LQIO::DOM::Document *>(getDOM())->setSpexConvergence( new LQIO::DOM::ConstantExternalVariable( Pragma::spexConvergence() ) );
    }
    if ( Pragma::spexIterationLimit() > 0 ) {
	const_cast<LQIO::DOM::Document *>(getDOM())->setSpexIterationLimit( new LQIO::DOM::ConstantExternalVariable( Pragma::spexIterationLimit() ) );
    }
    if ( Pragma::spexUnderrelaxation() > 0 ) {
	const_cast<LQIO::DOM::Document *>(getDOM())->setSpexUnderrelaxation( new LQIO::DOM::ConstantExternalVariable( Pragma::spexUnderrelaxation() ) );
    }
}

/*----------------------------------------------------------------------*/
/*                         Abstract Superclass.                         */
/*----------------------------------------------------------------------*/

/*
 * Constructor.
 */

Model::Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat )
    : _submodels(), _converged(false), _iterations(0), _MVAStats(),
      _convergence_value(0), _iteration_limit(0), _underrelaxation(0),
      _step_count(0), _model_initialized(false), _document(document),
      _input_file_name(inputFileName), _output_file_name(outputFileName), _output_format(outputFormat)
{
    __sync_submodel = 0;
}

/*
 * Destructor.  Free client and server arrays.
 */

Model::~Model()
{
    std::for_each( _submodels.begin(), _submodels.end(), []( Submodel * submodel ){ delete submodel; } );

    std::for_each( __processor.begin(), __processor.end(), []( Processor * processor ){ delete processor; } );
    __processor.clear();		/* Global, so get rid of them */

    std::for_each( __group.begin(), __group.end(), []( Group * group ){ delete group; } );
    __group.clear();

    if ( __think_server ) {
	delete __think_server;
	__think_server = nullptr;
    }

    std::for_each( __task.begin(), __task.end(), []( Task * task ){ delete task; } );
    __task.clear();

    std::for_each( __entry.begin(), __entry.end(), []( Entry * entry ){ delete entry; } );
    __entry.clear();
}



/*
 * Step 4: Perform all actions normally done in createModel() that
 * need to be delayed until after LQX programs begin execution to
 * avoid problems with unset variables.
 */

bool
Model::initialize()
{
    if ( Options::Trace::verbose() ) std::cerr << "Initialize..." << std::endl;

    if ( !_model_initialized ) {

	/* Expand replicas and add think server. */
	extend();			/* Do this before Task::initializeProcessor() */

	std::for_each( __task.begin(), __task.end(), std::mem_fn( &Task::initializeProcessor ) );	/* Set Processor Service times.	*/

	if ( Options::Trace::verbose() ) std::cerr << "Generate... " << std::endl;
	if ( generate( assignSubmodel() ) ) {
	    _model_initialized = true;
	    partition();
	    configure();		/* Dimension arrays and threads	*/
	    initializeSubmodels();	/* Init MVA values (pop&waits). */		/* -- Step 2 -- */
	}

	reorderOutput();
	if ( Options::Debug::submodels() ) {	/* Print out layers... 		*/
	    for ( const auto submodel : _submodels ) submodel->print( std::cout );
	}

    } else {
	reinitializeSubmodels();
    }

    return !LQIO::io_vars.anError();
}


/*
 * Expand replicas, add think centers for think time.
 */

void
Model::extend()
{
    /* Replicas. Create extra objects as needed.  */

    if ( Pragma::replication() == Pragma::Replication::EXPAND || Pragma::replication() == Pragma::Replication::PRUNE ) {
	/* Copy over original sets because we are going to insert the new objects directly */
	const std::set<Processor *,lt_replica<Processor>> processors(Model::__processor);
//	const std::set<Group *,lt_replica<Group>> Model::__group;
	const std::set<Task *,lt_replica<Task>> tasks(Model::__task);
	const std::set<Entry *,lt_replica<Entry>> entries(Model::__entry);

	/* Create processors and entries first as tasks need them.  */
	std::for_each( processors.begin(), processors.end(), std::mem_fn( &Processor::expand ) );
	std::for_each( entries.begin(), entries.end(), std::mem_fn( &Entry::expand ) );
	std::for_each( tasks.begin(), tasks.end(), std::mem_fn( &Task::expand ) );

	/* Calls are done after all entries have been created and linked to tasks */
	std::for_each( entries.begin(), entries.end(), std::mem_fn( &Entry::expandCalls ) );
	std::for_each( tasks.begin(), tasks.end(), std::mem_fn( &Task::expandCalls ) );
    }

    /* Think times. */

    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {

	/* Add a delay server for think times. */

	if ( (*task)->hasThinkTime() ) {
	    if ( !__think_server ) {
		__think_server = new DelayServer();
		__think_server->setClosedModelServer( true );
	    }
	    __think_server->addTask( *task );	/* link the task in */
	}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
	if ( Pragma::getQuorumDelayedCalls() == Pragma::KEEP_ALL_QUORUM_DELAYED_CALLS &&
	     (*task)->hasForks() && !flags.disable_expanding_quorum_tree ) {
	    (*task)->expandQuorumGraph();
	}
#endif
    }
}



/*
 * Check input parameters.  Return true if all went well.  Return false
 * and set anError to true otherwise.
 */

bool
Model::check()
{
    if ( Options::Trace::verbose() ) std::cerr << "Check..." << std::endl;

    if ( LQIO::io_vars.anError() ) return false;	/* Don't bother */

    bool rc = std::all_of( __processor.begin(), __processor.end(), std::mem_fn( &Processor::check ) )
	&& std::all_of( __task.begin(), __task.end(), std::mem_fn( &Task::check ) );

    if ( std::none_of( __task.begin(), __task.end(), std::mem_fn( &Task::isReferenceTask ) )
	 && std::none_of( __task.begin(), __task.end(), std::mem_fn( &Task::hasOpenArrivals ) ) ) {
	rc = false;
	LQIO::runtime_error( LQIO::ERR_NO_REFERENCE_TASKS );
    }

    return rc && !LQIO::io_vars.anError();
}



/*
 * Generate layers.  Tag all top-level tasks (i.e., those without any
 * calls thereto) and then locate their children.  findChildren will()
 * set the depth value for each task and _max_depth.  There should be a
 * minimum of two layers.
 *
 * There are two arrays - one for clients and one for servers.  Clients
 * *always* call a server (otherwise the solver gets screwed up).
 * Servers NEED NOT be called, so all tasks are stuck in the server
 * array.  NOTE that the client and server arrays are indexed from 1 to n.
 * not 0 to n-1.
 */

bool
Model::generate( unsigned max_depth )
{
    _submodels.resize( max_depth );

    for ( unsigned int i = 1; i <= max_depth; ++i ) {
	_submodels[i] = new MVASubmodel(i);
    }

    /* Add submodel for join delay calculation */

    if ( std::any_of( __task.begin(), __task.end(), std::mem_fn( &Task::hasForks ) ) ) {
	__sync_submodel = nSubmodels() + 1;	/* Set global here, used in addToSubmodel */
	_submodels.push_back(new SynchSubmodel(__sync_submodel));
    }

    /* Build model. */

    addToSubmodel();			/* Add tasks to layers.		*/
    std::for_each( _submodels.begin(), _submodels.end(), std::mem_fn( &Submodel::addClients ) );

    /* split/prune submodels -- this will have to be done by subclass */

    return !LQIO::io_vars.anError();
}



/*
 * Initialize the fan-in/out and the waiting time arrays.  The latter need to
 * know the number of layers.  Called after the model has been GENERATED.  The
 * model MUST BE COMPLETE before it can be initialized.  Tasks must be done
 * before processors.
 */

void
Model::configure()
{
    _MVAStats.resize( nSubmodels() );	/* MVA statistics by level.	*/
    for ( auto task : __task ) task->configure( nSubmodels() );
    for ( auto processor : __processor ) processor->configure( nSubmodels() );
    if ( __think_server ) {
	__think_server->configure( nSubmodels() );
    }
}



/*
 * Initialize waiting times, interlocking etc.  Servers are done by subclasses
 * because of the structure of the model.  Clients are done here.
 */

void
Model::initializeSubmodels()
{
    if ( Pragma::interlock() ) {
	std::for_each( __task.begin(), __task.end(), std::mem_fn( &Task::createInterlock ) );
	if ( Options::Debug::interlock() ) {
	    Interlock::printPathTable( std::cout );
	}
    }

    /*
     * Initialize waiting times and populations at servers Done in reverse
     * order (bottom up) because waits propogate upwards.
     */

    std::for_each( _submodels.rbegin(), _submodels.rend(), std::mem_fn( &Submodel::initializeSubmodel ) );

    /* Initialize Interlocking -- must be done after all submodels initialized. */

    if ( Pragma::interlock() ) {
	std::for_each( _submodels.begin(), _submodels.end(), std::mem_fn( &Submodel::initializeInterlock ) );
    }
    
    /* build stations and customers as needed. */

    std::for_each( _submodels.begin(), _submodels.end(), std::mem_fn( &Submodel::build ) );      	    /* For use by MVA stats/generate*/
}



/*
 * Re-initialize waiting times, interlocking etc.  Servers are done by
 * subclasses because of the structure of the model.  Clients are done here.
 */

void
Model::reinitializeSubmodels()
{
    /*
     * Reset all counters before a run.  In particular, iterations needs to
     * reset to zero for the convergence test.
     */

    _iterations = 0;
    _step_count = 0;

    std::for_each( _MVAStats.begin(), _MVAStats.end(), std::mem_fn( &MVACount::initialize ) );
    if ( Pragma::interlock() ) {
	std::for_each( __entry.begin(), __entry.end(), std::mem_fn( &Entry::resetInterlock ) );
    }

    /*
     * Reinitialize the MVA stuff
     */

    std::for_each( __task.begin(), __task.end(), std::mem_fn( &Task::createInterlock ) );

    /*
     * Initialize waiting times and populations at servers Done in reverse
     * order (bottom up) because waits propogate upwards.
     */

    std::for_each( _submodels.rbegin(), _submodels.rend(), std::mem_fn( &Submodel::reinitializeSubmodel ) );

    /* Reinitialize Interlocking */

    if ( Pragma::interlock() ) {
	std::for_each( _submodels.begin(), _submodels.end(), std::mem_fn( &Submodel::initializeInterlock ) );
    }
    
    /* Rebuild stations and customers as needed. */

    std::for_each( _submodels.begin(), _submodels.end(), std::mem_fn( &Submodel::rebuild ) );
}



/*
 * Reorder the entities (for output) in the DOM to level (ref task = 0), then name.
 */

void
Model::reorderOutput() const
{
    /* For ordering the multiset. Never use || here! */
    struct compare {
	bool operator()( const Entity *l, const Entity *r ) const { return l->submodel() < r->submodel() && l->name() < r->name(); }
    };
    /* For only copying the originals back */
    const struct predicate {
	bool operator()( const Entity *e ) const { return !e->isReplica() && e->getDOM() != nullptr; }
    } predicate;

    /* go through all the tasks and processors and save based on their depth. Processor and tasks names can collide. */

    std::multiset<Entity *,compare> mapped;
    std::copy_if( __task.begin(), __task.end(), std::inserter( mapped, mapped.end() ), predicate );
    std::copy_if( __processor.begin(), __processor.end(), std::inserter( mapped, mapped.end() ), predicate );
	       
    /* Change the order in the DOM.  DO NOT clear entities because the vector has to be the right size */
    std::vector<LQIO::DOM::Entity *>& entities = const_cast<std::vector<LQIO::DOM::Entity *>&>(getDOM()->getEntities());
    assert( mapped.size() == entities.size() );
    std::transform( mapped.begin(), mapped.end(), entities.begin(), []( Entity * entity ){ return entity->getDOM(); } );
}



/*
 * Now that the model is constructed, solve it by calling Model::run().
 */

bool
Model::compute()
{
    SolverReport report( const_cast<LQIO::DOM::Document *>(getDOM()), _MVAStats );
    setModelParameters();

    if ( flags.no_execute || flags.bounds_only ) {
	return true;
    }

    if ( Options::Trace::verbose() ) std::cerr << "Solve..." << std::endl;

    report.start();

    _converged = false;
    const double delta = run();
    report.finish( _converged, delta, _iterations );
    sanityCheck();
    if ( !_converged ) {
	LQIO::runtime_error( ADV_SOLVER_ITERATION_LIMIT, _iterations, delta, convergenceValue() );
    }
    if ( report.faultCount() ) {
	LQIO::runtime_error( ADV_MVA_FAULTS, report.faultCount() );
    }
    if ( flags.ignore_overhanging_threads ) {
	LQIO::runtime_error( ADV_NO_OVERHANG );
    }

    /* OK.  It solved. Now save the output. */

    const_cast<LQIO::DOM::Document *>(getDOM())->setResultDescription();
    report.insertDOMResults();
    insertDOMResults();
    getDOM()->print( _output_file_name, getDOM()->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix : std::string(""), _output_format, flags.rtf_output );

    if ( flags.print_overtaking ) {
	printOvertaking( std::cout );
    }

    if ( Generate::__mode != Generate::Output::NONE ) {
	Generate::output( _submodels );
    }
    if ( Options::Trace::verbose() ) {
	report.print( std::cout );
    }

    return true;
}



/*
 * Output the results.
 */

bool
Model::reload()
{
    /* Default mapping */

    LQIO::Filename directory_name( hasOutputFileName() ? _output_file_name : _input_file_name, "d" );		/* Get the base file name */

    if ( access( directory_name.str().c_str(), R_OK|X_OK ) < 0 ) {
	LQIO::runtime_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name.str().c_str(), strerror( errno ) );
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    }

    unsigned int errorCode;
    if ( !const_cast<LQIO::DOM::Document *>(getDOM())->loadResults( directory_name(), _input_file_name,
								    SolverInterface::Solve::customSuffix, _output_format, errorCode ) ) {
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    } else {
	return getDOM()->getResultValid();
    }
}


bool
Model::restart()
{
    try {
	if ( reload() ) return true;
    }
    catch ( const LQX::RuntimeException& e ) {}
    return compute();
}



/*
 * Check results for sanity.
 */

void
Model::sanityCheck()
{
    std::for_each( __task.begin(), __task.end(), std::mem_fn( &Entity::sanityCheck ) );
    std::for_each( __processor.begin(), __processor.end(), std::mem_fn( &Entity::sanityCheck ) );
}



/*
 * Set relaxation
 */

double
Model::underrelaxation() const
{
    if ( _iterations <= 1 ) {
	return 1.0;
    } else {
	return _underrelaxation;
    }
}


void
Model::insertDOMResults() const
{
    std::for_each( __task.begin(), __task.end(), std::mem_fn( &Task::insertDOMResults ) );
    std::for_each( __processor.begin(), __processor.end(), std::mem_fn( &Processor::insertDOMResults ) );
    std::for_each( __group.begin(), __group.end(), std::mem_fn( &Group::insertDOMResults ) );
}



/*
 * Intermediate result printer.
 */

void
Model::printIntermediate( const double convergence ) const
{
    SolverReport report( const_cast<LQIO::DOM::Document *>(getDOM()), _MVAStats );
    report.insertDOMResults();
    report.finish( _converged, convergence, _iterations );	/* Save results */

    insertDOMResults();

    getDOM()->print( _output_file_name, SolverInterface::Solve::customSuffix, _output_format, flags.rtf_output, _iterations );
}


std::ostream&
Model::printSubmodelWait( std::ostream& output ) const
{
    int precision = output.precision(6);
    std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );
    output.setf( std::ios::left, std::ios::adjustfield );

    output << std::setw(8) <<  "Submodel    ";
    for ( unsigned i = 1; i <= nSubmodels(); ++i ) output << std::setw(8) << i;
    output << std::endl;
    for ( const auto task : __task ) task->printSubmodelWait( output );

    output.setf( flags );
    output.precision( precision );
    return output;
}



std::ostream&
Model::printOvertaking( std::ostream& output ) const
{
    std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
    output << std::endl << std::endl << "Inservice probabilities (p->'a'):" << std::endl << std::endl;
    for ( unsigned int i = 0; i < 4; ++i ) {
	output << "Entry " << "abcd"[i] << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-7) << " ";
    }
    output << "p_d ";
    const unsigned int n_phases = getDOM()->getMaximumPhase();
    for ( unsigned int p = 1; p <= n_phases; ++p ) {
	output << "Phase " << p << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-7) << " ";
    }
    output << std::endl;

    for ( std::set<Task *>::const_iterator nextServer = __task.begin(); nextServer != __task.end(); ++nextServer ) {
	const Task * aServer = *nextServer;
	if ( aServer->markovOvertaking() ) {
	    for ( std::set<Task *>::const_iterator nextClient = __task.begin(); nextClient != __task.end(); ++nextClient ) {
		const Task * aClient = *nextClient;
		Overtaking overtaking( aClient, aServer );
		output << overtaking;
	    }
	}
    }
    return output;
    output.flags(oldFlags);
}



/*
 * Sort tasks into layers.  Start from reference tasks and tasks
 * with open arrivals only.  If a task has open arrivals, start from level
 * 1 so that it is treated as a server.
 */

unsigned
Model::topologicalSort()
{
    unsigned max_depth = 0;
    static const NullCall null_call;				/* Place holder */

    /* Only do reference tasks or those with open arrivals */

    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	Call::stack callStack;
	switch ( (*task)->rootLevel() ) {
	case Task::root_level_t::IS_REFERENCE: (*task)->setSubmodel( 1 ); break;
	case Task::root_level_t::IS_NON_REFERENCE: continue;
	case Task::root_level_t::HAS_OPEN_ARRIVALS: callStack.push_back(&null_call); break;	/* Open arrivals start at 1 */
	}
	try {
	    max_depth = std::max( (*task)->findChildren( callStack, true ), max_depth );
	}
	catch( const Call::call_cycle& error ) {
	    std::string msg = std::accumulate( callStack.rbegin(), callStack.rend(), callStack.back()->dstName(), &Call::stack::fold );
	    (*task)->getDOM()->runtime_error( LQIO::ERR_CYCLE_IN_CALL_GRAPH, msg.c_str() );
	}
    }

    /* Stop the process here and now on any error. */

    if ( LQIO::io_vars.anError() ) {
	throw std::runtime_error( "Model::topologicalSort" );
    }
    return max_depth;
}

/*----------------------------------------------------------------------*/
/*                             Rolia Model.                             */
/*----------------------------------------------------------------------*/

/*
 * Sort the task list by layer.  The number of submodels corresponds
 * to the depth of the call graph.  However, if we need to solve for
 * synchronization, tack a spare submodel in between the software
 * submodels and the hardware submodel.
 */

unsigned
MOL_Model::assignSubmodel( )
{
    unsigned max_layers = topologicalSort();
    _HWSubmodel = max_layers;
    return max_layers;
}



/*
 * Stick the tasks into their appropriate layers.  Quite simple for this
 * class.
 */

void
MOL_Model::addToSubmodel()
{
    /* Tasks go in submodels 1 - (_max_depth-1). */

    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	if ( !(*task)->isReferenceTask() ) {
	    const unsigned int i = (*task)->submodel();
	    if ( i == 0 ) continue;
	    _submodels[i]->addServer( *task );
	}
	if ( (*task)->hasForks() ) {
	    _submodels[__sync_submodel]->addClient( *task );
	}
	if ( (*task)->hasSyncs() ) {
	    _submodels[__sync_submodel]->addServer( *task );
	}
    }

    /* Processors and other devices go in submodel n */

    for ( std::set<Processor *>::const_iterator processor = __processor.begin(); processor != __processor.end(); ++processor ) {
	if ( (*processor)->submodel() == 0 ) continue;
	(*processor)->setSubmodel( _HWSubmodel );		// Force all devices to this level
	_submodels[_HWSubmodel]->addServer( *processor );
    }

    if ( __think_server ) {
	__think_server->setSubmodel( _HWSubmodel );		// Force all devices to this level
	_submodels[_HWSubmodel]->addServer(__think_server);
    }
}



/*
 * Solve the model starting from the top and working down.  Return the
 * number of inner loop _iterations.
 */

double
MOL_Model::run()
{
    const bool verbose = flags.trace_convergence || Options::Trace::verbose();
    SolveSubmodel solveSubmodel( *this, verbose );		/* Helper class for iterator */

    double delta = 0.0;
    do {
	do {
	    _iterations += 1;
	    if ( verbose ) std::cerr << "Iteration: " << _iterations << " ";

	    std::for_each( _submodels.begin(), &_submodels[_HWSubmodel], solveSubmodel );

	    delta = sqrt( std::accumulate( __task.begin(), __task.end(), 0.0, []( double l, const Entity * r ){ return l + square( r->deltaUtilization() ); } ) / __task.size() );		/* RMS */

	    if ( delta > convergenceValue() ) {
		backPropogate();
	    }
	} while ( delta > convergenceValue() &&  _iterations < iterationLimit() );		/* -- Step 4 -- */

	/* Print intermediate results if necessary */

	if ( flags.trace_intermediate ) {
	    printIntermediate( delta );
	}

	if ( flags.trace_wait ) {
	    printSubmodelWait();
	}

	/* Solve hardware model. */

	solveSubmodel( _submodels[_HWSubmodel] );		/* -- Step 6 -- */

	if ( flags.trace_wait ) {
	    printSubmodelWait();
	}

	delta = sqrt( std::accumulate( __processor.begin(), __processor.end(), 0.0, []( double l, const Entity * r ){ return l + square( r->deltaUtilization() ); } ) / __processor.size() );		/* RMS */
	if ( verbose ) std::cerr << " [" << delta << "]" << std::endl;

    } while ( ( _iterations < flags.min_steps || delta > convergenceValue() ) && _iterations < iterationLimit() );

    _converged = (delta <= convergenceValue() || _iterations == 1);	/* The model will never be converged with one step, so ignore */
    return delta;
}

/*----------------------------------------------------------------------*/
/*                      Back Propogate HwSw Model                       */
/*----------------------------------------------------------------------*/

/*
 * Back propogate the contention.  Think times will not be computed
 * properly as they are computed during the forward pass.
 */

void
BackPropogate_MOL_Model::backPropogate()
{
    if ( nSubmodels() < 4 ) return;
    const bool verbose = flags.trace_convergence || Options::Trace::verbose();
    std::for_each( _submodels.rbegin() + 2, _submodels.rend() - 1, SolveSubmodel( *this, verbose ) );
}

/*----------------------------------------------------------------------*/
/*                       Batched Partition Model                        */
/*----------------------------------------------------------------------*/

/*
 * Sort the task list by layer.  The number of submodels corresponds
 * to the depth of the call graph.  However, if we need to solve for
 * synchronization, tack a spare submodel in at the end.
 */

unsigned
Batch_Model::assignSubmodel()
{
    return topologicalSort();
}



/*
 * Stick the tasks into their appropriate layers.  Also stick
 * processors in.  Tasks and processors all go into the server arrays.
 */

void
Batch_Model::addToSubmodel()
{
    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	if ( !(*task)->isReferenceTask() ) {
	    const unsigned int i = (*task)->submodel();
	    if ( i == 0 ) continue;
	    _submodels[i]->addServer( *task );
	}
	if ( (*task)->hasForks() ) {
	    _submodels[__sync_submodel]->addClient( *task );
	}
	if ( (*task)->hasSyncs() ) { // Answer is always NO for now.
	    _submodels[__sync_submodel]->addServer( *task );
	}
    }

    for ( std::set<Processor *>::const_iterator processor = __processor.begin(); processor != __processor.end(); ++processor ) {
	const unsigned int i = (*processor)->submodel();
	if ( i == 0 ) continue;		// Device not used.
	_submodels[i]->addServer( *processor );
    }

    if ( __think_server ) {
	const unsigned int i = __think_server->submodel();
	if ( i > 0 ) {
	    _submodels[i]->addServer( __think_server );
	}
    }
}



/*
 * For each submodel, look for disjoint chains.  If found delete. This
 * may be extended to the general case and may result in adding
 * submodels.
 */

void
Batch_Model::partition()
{
    std::for_each( _submodels.begin(), _submodels.end(), std::mem_fn( &Submodel::partition ) );
}



/*
 * Solve the model starting from the top and working down.  Return the
 * number of inner loop _iterations.
 */

double
Batch_Model::run()
{
    double delta = 0.0;
    const bool verbose = (flags.trace_convergence || Options::Trace::verbose()) && !(Options::Trace::mva() || flags.trace_wait);
    const double count = __task.size() + __processor.size();

    do {
	_iterations += 1;
	if ( verbose ) std::cerr << "Iteration: " << _iterations << " ";

	std::for_each( _submodels.begin(), _submodels.end(), SolveSubmodel( *this, verbose ) );

	/* compute convergence for next pass. */

	delta = sqrt( std::accumulate( __processor.begin(), __processor.end(),
				       std::accumulate( __task.begin(), __task.end(), 0.0,
							[]( double l, const Entity * r ){ return l + square( r->deltaUtilization() ); } ),
				       []( double l, const Entity * r ){ return l + square( r->deltaUtilization() ); } ) / count );		/* RMS */

	if ( delta > convergenceValue() ) {
	    backPropogate();
	}

	if ( flags.trace_intermediate && _iterations % printInterval() == 0 ) {
	    printIntermediate( delta );
	}

	if ( flags.trace_wait ) {
	    printSubmodelWait();
	}

	if ( Options::Trace::mva() || flags.trace_wait ) {
	    std::cout << std::endl << "-*- -- -*- " << _iterations << ":   " << delta << " -*- -- -*-" << std::endl << std::endl;
	} else if ( Options::Trace::verbose() || flags.trace_convergence ) {
	    std::cerr << " [" << delta << "]" << std::endl;
	}
    } while ( ( _iterations < flags.min_steps || delta > convergenceValue() ) && _iterations < iterationLimit() );
    _converged = (delta <= convergenceValue() || _iterations == 1);	/* The model will never be converged with one step, so ignore */
    return delta;
}

/*----------------------------------------------------------------------*/
/*                      Back Propogate Batch Model                      */
/*----------------------------------------------------------------------*/

/*
 * Back propogate the contention.  Think times will not be computed
 * properly as they are computed during the forward pass.
 */

void
BackPropogate_Batch_Model::backPropogate()
{
    if ( nSubmodels() < 3 ) return;
    const bool verbose = (flags.trace_convergence || Options::Trace::verbose()) && !(Options::Trace::mva() || flags.trace_wait);
    std::for_each( _submodels.rbegin() + 1, _submodels.rend() - 1, SolveSubmodel( *this, verbose ) );
}

/*----------------------------------------------------------------------*/
/*                          SRVN Layers Model                           */
/*----------------------------------------------------------------------*/

/*
 * The trick for srvn layers is to assign each server to its own
 * submodel.  The sorter here simple re-sorts the graph so that each
 * task and processor is assigned a unigue ``depth''.  Then, the batch
 * layerizer (which simply iterates among the submodels) takes over.
 */

unsigned
SRVN_Model::assignSubmodel()
{
    topologicalSort();

    /* Build the list of all servers for this model */

    std::set<Entity *> servers;
    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	if ( (*task)->isReferenceTask() || (*task)->submodel() == 0 ) continue;
	servers.insert( *task );
	(*task)->setSubmodel( servers.size() );
    }
    for ( std::set<Processor *>::const_iterator processor = __processor.begin(); processor != __processor.end(); ++processor ) {
	if ( (*processor)->submodel() == 0 ) continue;
	servers.insert( *processor );
	(*processor)->setSubmodel( servers.size() );
    }
    if ( __think_server && __think_server->submodel() != 0 ) {
        servers.insert( __think_server );
	__think_server->setSubmodel( servers.size() );
    }
    return servers.size();
}

/*----------------------------------------------------------------------*/
/*                        Squashed Layers Model                         */
/*----------------------------------------------------------------------*/

/*
 * The trick for squashed layers is to assign each server to the first
 * submodel.  Then, the batch layerizer (which simply iterates among
 * the submodels) takes over.
 */

unsigned
Squashed_Model::assignSubmodel()
{
    topologicalSort();

    /* Build the list of all servers for this model */

    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	if ( !(*task)->isReferenceTask() && (*task)->submodel() > 1 ) {
	    (*task)->setSubmodel( 1 );
	}
    }
    for ( std::set<Processor *>::const_iterator processor = __processor.begin(); processor != __processor.end(); ++processor ) {
	if ( (*processor)->submodel() > 1 ) {
	    (*processor)->setSubmodel( 1 );
	}
    }
    if ( __think_server && __think_server->submodel() != 0 ) {
        __think_server->setSubmodel( 1 );
    }

    return 1;
}

/*----------------------------------------------------------------------*/
/*                          HwSw Layers Model                           */
/*----------------------------------------------------------------------*/

/*
 * This one is similar to MOL except all the tasks are put in submodel
 * 1 and all the processors are put in submodel 2. The batch layerizer
 * (which simply iterates among the submodels) takes over.
 */

unsigned
HwSw_Model::assignSubmodel()
{
    topologicalSort();

    /* Build the list of all servers for this model */

    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	if ( !(*task)->isReferenceTask() && (*task)->submodel() > 1 ) {
	    (*task)->setSubmodel( 1 );
	}
    }
    for ( std::set<Processor *>::const_iterator processor = __processor.begin(); processor != __processor.end(); ++processor ) {
	if ( (*processor)->submodel() > 1 ) {
	    (*processor)->setSubmodel( 2 );
	}
    }
    if ( __think_server && __think_server->submodel() != 0 ) {
        __think_server->setSubmodel( 2 );
    }

    return 2;
}

/*
 * Solve a single sub-model.
 */

void
Model::SolveSubmodel::operator()( Submodel * submodel  )
{
    const unsigned depth = submodel->number();
    _model._step_count += 1;
    if ( _verbose ) std::cerr << ".";
    submodel->solve( _model._iterations, _model._MVAStats[depth], _model.underrelaxation() );  //REP N-R
}
