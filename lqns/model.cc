/* -*- c++ -*-
 * $Id: model.cc 15278 2021-12-27 21:52:20Z greg $
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
 *    d) Call initStations (initialize service times, etc...)
 * 3) Call solve()
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * ------------------------------------------------------------------------
 */

/* Debug Messages for Loading */
#if defined(DEBUG_MESSAGES)
#define DEBUG(x) cout << x
#else
#define DEBUG(X)
#endif


#include "lqns.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <lqio/dom_bindings.h>
#include <lqio/dom_document.h>
#include <lqio/error.h>
#include <lqio/filename.h>
#include <lqio/input.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
#include <mva/fpgoop.h>
#include <mva/server.h>
#include <mva/mva.h>
#include <mva/open.h>
#include "activity.h"
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "generate.h"
#include "group.h"
#include "interlock.h"
#include "flags.h"
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

double Model::__convergence_value = 0.;
unsigned Model::__iteration_limit = 0;
double Model::__underrelaxation = 0;
unsigned Model::__print_interval = 0;
Processor * Model::__think_server = nullptr;
unsigned Model::__sync_submodel = 0;
LQIO::DOM::Document::InputFormat Model::input_format = LQIO::DOM::Document::InputFormat::AUTOMATIC;

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
Model::create( const std::string& inputFileName, const std::string& outputFileName )
{
    /* Open input file. */

    if ( !flags.no_execute && flags.generate && Generate::file_name.size() == 0 ) {
        Generate::file_name = LQIO::Filename( inputFileName )();
    }

    /* This is a departure from before -- we begin by loading a model */
    LQIO::DOM::Document* document = Model::load(inputFileName,outputFileName);

    /* Make sure we got a document */

    if ( document == nullptr || LQIO::io_vars.anError() ) return INVALID_INPUT;

    document->mergePragmas( pragmas.getList() );       /* Save pragmas -- prepare will process */
    if ( Model::prepare(document) == false ) return INVALID_INPUT;

    if ( document->getInputFormat() == LQIO::DOM::Document::InputFormat::XML || document->getInputFormat() == LQIO::DOM::Document::InputFormat::JSON ) {
	if ( LQIO::Spex::__no_header ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --no-header is ignored for " << inputFileName << "." << std::endl;
	}
	if ( LQIO::Spex::__print_comment ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": --print-comment is ignored for " << inputFileName << "." << std::endl;
	}
    }

    /* declare Model * at this scope but don't instantiate due to problems with LQX programs and registering external symbols*/
    Model * model = nullptr;
    int rc = 0;

    /* We can simply run if there's no control program */
    LQX::Program * program = document->getLQXProgram();
    FILE * output = nullptr;
    if ( !program ) {

	/* There is no control flow program, check for $-variables */
	if (document->getSymbolExternalVariableCount() != 0) {
	    LQIO::solution_error( LQIO::ERR_LQX_VARIABLE_RESOLUTION, inputFileName.c_str() );
	    rc = INVALID_INPUT;
	} else {
	    /* Make sure values are up to date */
	    Model::recalculateDynamicValues( document );

	    /* Simply invoke the solver for the current DOM state */

	    try {
		model = Model::create( document, inputFileName, outputFileName );

		if ( model->check() && model->initialize() ) {
		    if ( Pragma::spexComment() ) {	// Not spex/lqx, so output on stderr.
			std::cerr << inputFileName << ": " << document->getModelCommentString() << std::endl;
		    }
		    model->solve();
		} else {
		    rc = INVALID_INPUT;
		}
	    }
	    catch ( const std::domain_error& e ) {
		rc = INVALID_INPUT;
	    }
	    catch ( const std::range_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": range error - " << e.what() << std::endl;
		rc = INVALID_OUTPUT;
	    }
	    catch ( const floating_point_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": floating point error - " << e.what() << std::endl;
		rc = INVALID_OUTPUT;
	    }
	    catch ( const std::runtime_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": run time error - " << e.what() << std::endl;
		rc = INVALID_INPUT;
	    }
	}

    } else {

	if ( flags.verbose ) {
	    std::cerr << "Compile LQX..." << std::endl;
	}

	/* Attempt to run the program */
	document->registerExternalSymbolsWithProgram( program );

	if ( flags.print_lqx ) {
	    program->print( std::cout );
	}

	model = Model::create( document, inputFileName, outputFileName );

	LQX::Environment * environment = program->getEnvironment();
	if ( flags.restart ) {
	    environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::restart, model));
	} else if ( flags.reload_only ) {
	    environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::reload, model));
	} else {
	    environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::solve, model));
	}
	LQIO::RegisterBindings(environment, document);

	if ( outputFileName.size() > 0 && outputFileName != "-" && LQIO::Filename::isRegularFile(outputFileName.c_str()) ) {
	    output = fopen( outputFileName.c_str(), "w" );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, outputFileName.c_str(), strerror( errno ) );
		rc = FILEIO_ERROR;
	    } else {
		environment->setDefaultOutput( output );      /* Default is stdout */
	    }
	}

	if ( rc == 0 ) {
	    /* Invoke the LQX program itself */
	    if ( !program->invoke() ) {
		LQIO::solution_error( LQIO::ERR_LQX_EXECUTION, inputFileName.c_str() );
		rc = INVALID_INPUT;
	    } else if ( !SolverInterface::Solve::solveCallViaLQX ) {
		/* There was no call to solve the LQX */
		LQIO::solution_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, inputFileName.c_str() );
		std::vector<LQX::SymbolAutoRef> args;
		environment->invokeGlobalMethod("solve", &args);
	    }
	}
    }

    /* Clean things up */
    if ( model ) delete model;
    if ( output ) fclose( output );
    if ( program ) delete program;
    delete document;
    return rc;
}




/*
 * Step 1: load model.
 */

LQIO::DOM::Document*
Model::load( const std::string& input_filename, const std::string& output_filename )
{
    if ( flags.verbose ) std::cerr << "Load: " << input_filename << "..." << std::endl;

    /*
     * Initialize everything that needs it before parsing
     */

    set_fp_ok( false );			// Reset floating point. -- stop on overflow?
    if ( matherr_disposition == FP_IMMEDIATE_ABORT ) {
	set_fp_abort();
    }

    LQIO::io_vars.reset();
    Entry::reset();

    /*
     * Read input file and parse it.
     */

    unsigned errorCode = 0;

    /* Attempt to load in the document from the filename/ptr and configured io_vars */
    return LQIO::DOM::Document::load(input_filename, input_format, errorCode, false);
}


/*
 * Step 2: convert DOM into lqn entities.
 */

bool
Model::prepare(const LQIO::DOM::Document* document)
{
    /* Tell the user that we are starting to load up */
    if ( flags.verbose ) std::cerr << "Prepare: ..." << std::endl;
    DEBUG(std::endl << "[0]: Beginning model load, setting parameters." << std::endl);

    /* Update the pragma list from the document (merge), then set globals here as this has to be done prior to runlqx() */
    Pragma::set( document->getPragmaList() );
    LQIO::io_vars.severity_level = Pragma::severityLevel();
    LQIO::Spex::__no_header = !Pragma::spexHeader();
    LQIO::Spex::__print_comment = Pragma::spexComment();
    if ( Pragma::has( LQIO::DOM::Pragma::_mol_underrelaxation_ ) ) {
	MVA::MOL_multiserver_underrelaxation = Pragma::molUnderrelaxation();
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */

    const std::map<std::string,LQIO::DOM::Processor *>& procList = document->getProcessors();
    std::for_each( procList.begin(), procList.end(), Processor::create );

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1.5: Add Groups] */

    const std::map<std::string,LQIO::DOM::Group*>& groups = document->getGroups();
    std::for_each( groups.begin(), groups.end(), Group::create );

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */

    /* In the DOM, tasks have entries, but here entries need to go first */
    const std::map<std::string,LQIO::DOM::Task*>& taskList = document->getTasks();
    std::vector<Activity*> activityList;

    /* Add all of the processors we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator nextTask = taskList.begin(); nextTask != taskList.end(); ++nextTask ) {
	LQIO::DOM::Task* task = nextTask->second;

	/* Before we can add a task we have to add all of its entries */
	DEBUG("[2]: Preparing to add entries for Task (" << task->name() << ")" << std::endl);

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
	DEBUG("[3]: Adding Task (" << name << ")" << std::endl);

	Task* newTask = Task::create(task, entries);

	/* Add activities for the task (all of them) */
	const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
	std::map<std::string,LQIO::DOM::Activity*>::const_iterator iter;
	for (iter = activities.begin(); iter != activities.end(); ++iter) {
	    const LQIO::DOM::Activity* activity = iter->second;
	    DEBUG("[3][a]: Adding Activity (" << activity->getName() << ") to Task." << std::endl);
	    activityList.push_back(add_activity(newTask, const_cast<LQIO::DOM::Activity*>(activity)));
	}

	/* Set all the start activities */
	for ( std::vector<LQIO::DOM::Entry*>::const_iterator nextEntry = activityEntries.begin(); nextEntry != activityEntries.end(); ++nextEntry) {
	    LQIO::DOM::Entry* theDOMEntry = *nextEntry;
	    DEBUG("[3][b]: Setting Start Activity (" << theDOMEntry->getStartActivity()->getName().c_str()
		  << ") for Entry (" << theDOMEntry->getName().c_str() << ")" << std::endl);
	    set_start_activity(newTask, theDOMEntry);
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
	std::vector<LQIO::DOM::Call*>::const_iterator nextFwd;
	for ( nextFwd = forwarding.begin(); nextFwd != forwarding.end(); ++nextFwd ) {
	    Entry* targetEntry = Entry::find((*nextFwd)->getDestinationEntry()->getName());
	    newEntry->setForwardingInformation(targetEntry, *nextFwd );
	}
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 4: Add Calls/Lists for Activities] */

    /* Go back and add all of the lists and calls now that activities all exist */
    std::vector<Activity*>::iterator actIter;
    for (actIter = activityList.begin(); actIter != activityList.end(); ++actIter) {
	Activity* activity = *actIter;
	DEBUG("[4]: Adding Task (" << theTask->name() << ") Activity (" << activity->name() << ") Calls and Lists." << std::endl);
	activity->add_calls()
	    .add_reply_list()
	    .add_activity_lists();
    }

    /* Use the generated connections list to finish up */
    DEBUG("[5]: Adding connections." << std::endl);
    Activity::completeConnections();

    std::for_each( __task.begin(), __task.end(), Exec<Task>( &Task::linkForkToJoin ) );	/* Link forks to joins		*/

    /* Tell the user that we have finished */
    DEBUG("[0]: Finished loading the model" << std::endl << std::endl);

    return true;
}



/*
 * Dynamic Updates / Late Finalization
 * In order to integrate LQX's support for model changes we need to
 * have a way of re-calculating what used to be static for all
 * dynamically editable values
 */

void
Model::recalculateDynamicValues( const LQIO::DOM::Document* document )
{
    setModelParameters(document);
    std::for_each( __processor.begin(), __processor.end(), Exec<Processor>( &Processor::recalculateDynamicValues ) );
    std::for_each( __group.begin(), __group.end(), Exec<Group>( &Group::recalculateDynamicValues ) );
    std::for_each( __task.begin(), __task.end(), Exec<Task>( &Task::recalculateDynamicValues ) );
}



/*
 * Step 3: Factory method for creating the layers and initializing the MVA submodels
 */

Model *
Model::create( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName )
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

    if ( flags.verbose ) std::cerr << "Create: " << Pragma::getLayeringStr() << " layers..." << std::endl;

    create_func f = create_funcs.at(Pragma::layering());
    Model * model = (*f)( document, inputFileName, outputFileName );

    if ( !model ) throw std::runtime_error( "could not create model" );

    return model;
}



/*
 * Called from the parser to set important modelling parameters.  It
 * can also check validity of same if so desired.
 */

void
Model::setModelParameters( const LQIO::DOM::Document* doc )
{
    if ( __print_interval == 0 ) {
	__print_interval = doc->getModelPrintIntervalValue();
    }
    if ( Pragma::has( LQIO::DOM::Pragma::_iteration_limit_ ) ) {
	__iteration_limit = Pragma::iterationLimit();
    } else {
	__iteration_limit = doc->getModelIterationLimitValue();
    }
    if ( __iteration_limit < 1 ) {
	LQIO::input_error2( ADV_ITERATION_LIMIT, __iteration_limit, 50 );
	__iteration_limit =  50;
    }
    if ( Pragma::has( LQIO::DOM::Pragma::_convergence_value_ ) ) {
	__convergence_value = Pragma::convergenceValue();
    } else {
	__convergence_value = doc->getModelConvergenceValue();
    }
    if ( __convergence_value <= 0 ) {
	LQIO::input_error2( ADV_CONVERGENCE_VALUE, __convergence_value, 0.00001 );
	__convergence_value = 0.00001;
    } else if ( __convergence_value > 0.01 ) {
	LQIO::input_error2( ADV_LARGE_CONVERGENCE_VALUE, __convergence_value );
    }
    if ( Pragma::has( LQIO::DOM::Pragma::_underrelaxation_ ) ) {
	__underrelaxation = Pragma::underrelaxation();
    } else {
	__underrelaxation = doc->getModelUnderrelaxationCoefficientValue();
    }
    if ( __underrelaxation <= 0.0 || 2.0 < __underrelaxation ) {
	LQIO::input_error2( ADV_UNDERRELAXATION, __underrelaxation );
	__underrelaxation = 0.9;
    }
}

/*----------------------------------------------------------------------*/
/*                         Abstract Superclass.                         */
/*----------------------------------------------------------------------*/

/*
 * Constructor.
 */

Model::Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName )
    : _converged(false), _iterations(0), _step_count(0), _model_initialized(false), _document(document), _input_file_name(inputFileName), _output_file_name(outputFileName)
{
    __sync_submodel = 0;
}

/*
 * Destructor.  Free client and server arrays.
 */

Model::~Model()
{
    std::for_each( _submodels.begin(), _submodels.end(), Delete<Submodel *> );

    std::for_each( __processor.begin(), __processor.end(), Delete<Processor *> );
    __processor.clear();		/* Global, so get rid of them */

    std::for_each( __group.begin(), __group.end(), Delete<Group *> );
    __group.clear();

    if ( __think_server ) {
	delete __think_server;
	__think_server = nullptr;
    }

    std::for_each( __task.begin(), __task.end(), Delete<Task *> );
    __task.clear();

    std::for_each( __entry.begin(), __entry.end(), Delete<Entry *> );
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
    if ( flags.verbose ) std::cerr << "Initialize..." << std::endl;

    if ( !_model_initialized ) {

	/* Expand replicas and add think server. */
	extend();			/* Do this before Task::initProcessor() */

	std::for_each( __task.begin(), __task.end(), Exec<Task>( &Task::initProcessor ) );	/* Set Processor Service times.	*/

	if ( flags.verbose ) std::cerr << "Generate... " << std::endl;
	if ( generate( assignSubmodel() ) ) {
	    _model_initialized = true;
	    optimize();
	    configure();		/* Dimension arrays and threads	*/
	    initStations();		/* Init MVA values (pop&waits). */		/* -- Step 2 -- */
	}

	if ( Options::Debug::layers() ) {	/* Print out layers... 		*/
	    std::for_each( _submodels.begin(), _submodels.end(), ConstPrint<Submodel>( &Submodel::print, std::cout ) );
	}

    } else {
	reinitStations();
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
	std::for_each( processors.begin(), processors.end(), Exec<Processor>( &Processor::expand ) );
	std::for_each( entries.begin(), entries.end(), Exec<Entry>( &Entry::expand ) );
	std::for_each( tasks.begin(), tasks.end(), Exec<Task>( &Task::expand ) );

	/* Calls are done after all entries have been created and linked to tasks */
	std::for_each( entries.begin(), entries.end(), Exec<Entry>( &Entry::expandCalls ) );
	std::for_each( tasks.begin(), tasks.end(), Exec<Task>( &Task::expandCalls ) );
    }

    /* Think times. */

    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {

	/* Add a delay server for think times. */

	if ( (*task)->hasThinkTime() ) {
	    if ( !__think_server ) {
		__think_server = new DelayServer();
	    }
	    __think_server->addTask( (*task) );	/* link the task in */
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
    if ( flags.verbose ) std::cerr << "Check..." << std::endl;

    if ( LQIO::io_vars.anError() ) return false;	/* Don't bother */

    bool rc = true;
    rc = std::all_of( __processor.begin(), __processor.end(), Predicate<Processor>( &Processor::check ) ) && rc;
    rc = std::all_of( __task.begin(), __task.end(), Predicate<Task>( &Task::check ) ) && rc;

    if ( std::none_of( __task.begin(), __task.end(), Predicate<Task>( &Task::isReferenceTask ) ) && Entry::totalOpenArrivals == 0 ) {
	rc = false;
	LQIO::solution_error( LQIO::ERR_NO_REFERENCE_TASKS );
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

    if ( std::any_of( __task.begin(), __task.end(), Predicate<Task>( &Task::hasForks ) ) ) {
	__sync_submodel = nSubmodels() + 1;	/* Set global here, used in addToSubmodel */
	_submodels.push_back(new SynchSubmodel(__sync_submodel));
    }

    /* Build model. */

    addToSubmodel();			/* Add tasks to layers.		*/
    std::for_each( _submodels.begin(), _submodels.end(), Exec<Submodel>( &Submodel::addClients ) );

    /* split/prune submodels -- this will have to be done by subclass */

    return !LQIO::io_vars.anError();
}



/*
 * Initialize the fan-in/out and the waiting time arrays.  The latter
 * need to know the number of layers.  Called after the model has been
 * GENERATED.  The model MUST BE COMPLETE before it can be
 * initialized.  Tasks must be done before processors.
 */

void
Model::configure()
{
    _MVAStats.resize( nSubmodels() );	/* MVA statistics by level.	*/
    std::for_each( __task.begin(), __task.end(), Exec1<Entity,unsigned>( &Entity::configure, nSubmodels() ) );
    std::for_each( __processor.begin(), __processor.end(), Exec1<Entity,unsigned>( &Entity::configure, nSubmodels() ) );
    if ( __think_server ) {
	__think_server->configure( nSubmodels() );
    }
}



/*
 * Initialize waiting times, interlocking etc.  Servers are done by
 * subclasses because of the structure of the model.  Clients are done
 * here.
 */

void
Model::initStations()
{
    if ( Pragma::interlock() ) {
	std::for_each( __task.begin(), __task.end(), Exec<Task>( &Task::createInterlock ) );
	if ( Options::Debug::interlock() ) {
	    Interlock::printPathTable( std::cout );
	}
    }

    /*
     * Initialize waiting times and populations at servers Done in
     * reverse order (bottom up) because waits propogate upwards.
     */

    std::for_each( _submodels.rbegin(), _submodels.rend(), Exec1<Submodel,const Model&>( &Submodel::initServers, *this ) );

    /* Initialize waiting times and populations for the reference tasks. */

    std::for_each( __task.begin(), __task.end(), Exec1<Entity,const Vector<Submodel *>&>( &Entity::initClient, getSubmodels() ) );

    /* Initialize Interlocking */

    if ( Pragma::interlock() ) {
	std::for_each( _submodels.begin(), _submodels.end(), Exec<Submodel>( &Submodel::initInterlock ) );
    }

    /* build stations and customers as needed. */

    std::for_each( _submodels.begin(), _submodels.end(), Exec<Submodel>( &Submodel::build ) );      	    /* For use by MVA stats/generate*/
}



/*
 * Re-initialize waiting times, interlocking etc.  Servers are done by
 * subclasses because of the structure of the model.  Clients are done
 * here.
 */

void
Model::reinitStations()
{
    /*
     * Reset all counters before a run.  In particular, iterations
     * needs to reset to zero for the convergence test.
     */

    _iterations = 0;
    _step_count = 0;

    std::for_each( _MVAStats.begin(), _MVAStats.end(), Exec<MVACount>( &MVACount::initialize ) );
    if ( Pragma::interlock() ) {
	std::for_each( __entry.begin(), __entry.end(), Exec<Entry>( &Entry::resetInterlock ) );
    }

    /*
     * Reinitialize the MVA stuff
     */

    std::for_each( __task.begin(),  __task.end(), Exec<Task>( &Task::createInterlock ) );

    /*
     * Initialize waiting times and populations at servers Done in
     * reverse order (bottom up) because waits propogate upwards.
     */

    std::for_each( _submodels.rbegin(), _submodels.rend(), Exec1<Submodel,const Model&>( &Submodel::reinitServers, *this ) );

    /* Initialize waiting times and populations for the reference tasks. */

    std::for_each( __task.begin(), __task.end(), Exec1<Entity,const Vector<Submodel *>&>( &Entity::reinitClient, getSubmodels() ) );

    /* Reinitialize Interlocking */

    if ( Pragma::interlock() ) {
	std::for_each( _submodels.begin(), _submodels.end(), Exec<Submodel>( &Submodel::initInterlock ) );
    }

    /* Rebuild stations and customers as needed. */

    std::for_each( _submodels.begin(), _submodels.end(), Exec<Submodel>( &Submodel::rebuild ) );
}



/*
 * Read model description.  Return true if all went well.
 */

bool
Model::solve()
{
    SolverReport report( const_cast<LQIO::DOM::Document *>(_document), _MVAStats );
    setModelParameters( _document );

    if ( flags.no_execute || flags.bounds_only ) {
	return true;
    }

    if ( flags.verbose ) std::cerr << "Solve..." << std::endl;

    report.start();

    _converged = false;
    const double delta = run();
    report.finish( _converged, delta, _iterations );
    sanityCheck();
    if ( !_converged ) {
	LQIO::solution_error( ADV_SOLVER_ITERATION_LIMIT, _iterations, delta, __convergence_value );
    }
    if ( report.faultCount() ) {
	LQIO::solution_error( ADV_MVA_FAULTS, report.faultCount() );
    }
    if ( flags.ignore_overhanging_threads ) {
	LQIO::solution_error( ADV_NO_OVERHANG );
    }

    /* OK.  It solved. Now save the output. */

    report.insertDOMResults();
    insertDOMResults();

    if ( flags.generate ) {
	Generate::makefile( nSubmodels() );	/* We are dumping C source -- make a makefile. */
    }

    const bool lqx_output = _document->getResultInvocationNumber() > 0;
    const std::string directoryName = createDirectory();
    const std::string suffix = lqx_output ? SolverInterface::Solve::customSuffix : "";

    /* override is true for '-p -o filename.out filename.in' == '-p filename.in' */

    bool override = false;
    if ( hasOutputFileName() && LQIO::Filename::isRegularFile( _output_file_name ) != 0 ) {
	LQIO::Filename filename( _input_file_name, flags.rtf_output ? "rtf" : "out" );
	override = filename() == _output_file_name;
    }

    if ( override || ((!hasOutputFileName() || directoryName.size() > 0 ) && _input_file_name != "-" )) {
	if ( _document->getInputFormat() == LQIO::DOM::Document::InputFormat::XML || flags.xml_output ) {	/* No parseable/json output, so create XML */
	    LQIO::Filename filename( _input_file_name, "lqxo", directoryName, suffix );
	    std::ofstream output;
	    filename.backup();
	    output.open( filename().c_str(), std::ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::OutputFormat::XML );
		output.close();
	    }
	}

	if ( _document->getInputFormat() == LQIO::DOM::Document::InputFormat::JSON || flags.json_output ) {
	    LQIO::Filename filename( _input_file_name, "lqjo", directoryName, suffix );
	    std::ofstream output;
	    output.open( filename().c_str(), std::ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::OutputFormat::JSON );
		output.close();
	    }
	}

	/* Parseable output. */

	if ( ( _document->getInputFormat() == LQIO::DOM::Document::InputFormat::LQN && lqx_output && !flags.xml_output ) || flags.parseable_output ) {
	    LQIO::Filename filename( _input_file_name, "p", directoryName, suffix );
	    std::ofstream output;
	    output.open( filename().c_str(), std::ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::OutputFormat::PARSEABLE );
		output.close();
	    }
	}

	/* Regular output */

	LQIO::Filename filename( _input_file_name, flags.rtf_output ? "rtf" : "out", directoryName, suffix );

	std::ofstream output;
	output.open( filename().c_str(), std::ios::out );
	if ( !output ) {
	    solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	} else if ( flags.rtf_output ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::RTF );
	} else {
	    _document->print( output );
	    if ( flags.print_overtaking ) {
		printOvertaking( output );
	    }
	}
	output.close();

    } else if ( _output_file_name == "-" || _input_file_name == "-" ) {

	if ( flags.parseable_output ) {
	    _document->print( std::cout, LQIO::DOM::Document::OutputFormat::PARSEABLE );
	} else if ( flags.rtf_output ) {
	    _document->print( std::cout, LQIO::DOM::Document::OutputFormat::RTF );
	} else {
	    _document->print( std::cout );
	    if ( flags.print_overtaking ) {
		printOvertaking( std::cout );
	    }
	}

    } else {

	/* Do not map filename. */

	LQIO::Filename::backup( _output_file_name );

	std::ofstream output;
	output.open( _output_file_name.c_str(), std::ios::out );
	if ( !output ) {
	    solution_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
	} else if ( flags.xml_output ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::XML );
	} else if ( flags.json_output ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::JSON );
	} else if ( flags.parseable_output ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::PARSEABLE );
	} else if ( flags.rtf_output ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::RTF );
	} else {
	    _document->print( output );
	    if ( flags.print_overtaking ) {
		printOvertaking( output );
	    }
	}
	output.close();
    }
    if ( flags.verbose ) {
	report.print( std::cout );
    }

    std::cout.flush();
    std::cerr.flush();
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

    if ( access( directory_name().c_str(), R_OK|X_OK ) < 0 ) {
	solution_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name().c_str(), strerror( errno ) );
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    }

    unsigned int errorCode;
    if ( !const_cast<LQIO::DOM::Document *>(_document)->loadResults( directory_name(), _input_file_name,
								     SolverInterface::Solve::customSuffix, errorCode ) ) {
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    } else {
	return _document->getResultValid();
    }
}


bool
Model::restart()
{
    try {
	if ( !reload() ) {
	    return solve();
	}
	return false;
    }
    catch ( const LQX::RuntimeException& e ) {
	return solve();
    }
}



/*
 * Check results for sanity.
 */

void
Model::sanityCheck()
{
    std::for_each( __task.begin(), __task.end(), ConstExec<Entity>( &Entity::sanityCheck ) );
    std::for_each( __processor.begin(), __processor.end(), ConstExec<Entity>( &Entity::sanityCheck ) );
}



/*
 * Set relaxation
 */

double
Model::relaxation() const
{
    if ( _iterations <= 1 ) {
	return 1.0;
    } else {
	return __underrelaxation;
    }
}


void
Model::insertDOMResults() const
{
    std::for_each( __task.begin(), __task.end(), ConstExec<Task>( &Task::insertDOMResults ) );
    std::for_each( __processor.begin(), __processor.end(), ConstExec<Processor>( &Processor::insertDOMResults ) );
    std::for_each( __group.begin(), __group.end(), ConstExec<Group>( &Group::insertDOMResults ) );
}



/*
 * Intermediate result printer.
 */

void
Model::printIntermediate( const double convergence ) const
{
    SolverReport report( const_cast<LQIO::DOM::Document *>(_document), _MVAStats );

    const std::string directoryName = createDirectory();
    const std::string suffix = _document->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix : "";

    report.insertDOMResults();
    insertDOMResults();

    std::string extension;
    if ( flags.parseable_output ) {
    	extension = "p";
    } else if ( flags.xml_output ) {
    	extension = "lqxo";
    } else if ( flags.rtf_output ) {
    	extension = "rtf";
    } else {
    	extension = "out";
    }

    LQIO::Filename filename( _input_file_name, extension, directoryName, suffix );

    /* Make filename look like an emacs autosave file. */
    filename << "~" << _iterations << "~";

    report.finish( _converged, convergence, _iterations );	/* Save results */

    std::ofstream output;
    output.open( filename().c_str(), std::ios::out );

    if ( !output ) return;			/* Ignore errors */

    if ( flags.xml_output ) {
	_document->print( output, LQIO::DOM::Document::OutputFormat::XML );
    } else if ( flags.json_output ) {
	_document->print( output, LQIO::DOM::Document::OutputFormat::JSON );
    } else if ( flags.parseable_output ) {
	_document->print( output, LQIO::DOM::Document::OutputFormat::PARSEABLE );
    } else {
	_document->print( output );
    }
    output.close();
}


std::ostream&
Model::printSubmodelWait( std::ostream& output ) const
{
    int precision = output.precision(6);
    std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );
    output.setf( std::ios::left, std::ios::adjustfield );

    output << std::setw(8) <<  "Submodel    ";
    for ( unsigned i = 1; i <= nSubmodels(); ++i ) {
	output << std::setw(8) << i;
    }
    output << std::endl;

    std::for_each( __task.begin(), __task.end(), ConstPrint<Task>( &Task::printSubmodelWait, output ) );

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
    const unsigned int n_phases = _document->getMaximumPhase();
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
 * Create a directory (if needed)
 */

std::string
Model::createDirectory() const
{
    std::string directoryName;
    if ( hasOutputFileName() && LQIO::Filename::isDirectory( _output_file_name ) > 0 ) {
	directoryName = _output_file_name;
    }

    if ( _document->getResultInvocationNumber() > 0 ) {
	if ( directoryName.empty() ) {
	    /* We need to create a directory to store output. */
	    LQIO::Filename filename( hasOutputFileName() ? _output_file_name : _input_file_name, "d" );		/* Get the base file name */
	    directoryName = filename();
	}
    }

    if ( directoryName.size() > 0 ) {
	int rc = access( directoryName.c_str(), R_OK|W_OK|X_OK );
	if ( rc < 0 ) {
	    if ( errno == ENOENT ) {
#if defined(__WINNT__)
		rc = mkdir( directoryName.c_str() );
#else
		rc = mkdir( directoryName.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH );
#endif
	    }
	    if ( rc < 0 ) {
		solution_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directoryName.c_str(), strerror( errno ) );
	    }
	}
    }
    return directoryName;
}

/*
 * Sort tasks into layers.  Start from reference tasks and tasks
 * with open arrivals only.  If a task has open arrivals, start from level
 * 1 so that it is treated as a server.
 */

unsigned
Model::topologicalSort()
{
    Call::stack callStack;
    unsigned max_depth = 0;
    static const NullCall null_call;				/* Place holder */

    /* Only do reference tasks or those with open arrivals */

    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	switch ( (*task)->rootLevel() ) {
	case Task::root_level_t::IS_NON_REFERENCE: continue;
	case Task::root_level_t::HAS_OPEN_ARRIVALS: callStack.push_back(&null_call); break;	/* Open arrivals start at 1 */
	default: break;
	}
	try {
	    max_depth = std::max( (*task)->findChildren( callStack, true ), max_depth );
	}
	catch( const Call::call_cycle& error ) {
	    std::string msg = std::accumulate( callStack.rbegin(), callStack.rend(), callStack.back()->dstName(), &Call::stack::fold );
	    LQIO::solution_error( LQIO::ERR_CYCLE_IN_CALL_GRAPH, msg.c_str() );
	}
    }

    /* Stop the process here and now on any error. */

    if ( LQIO::io_vars.anError() ) {
	throw exception_handled( "Model::topologicalSort" );
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
	    _submodels[(*task)->submodel()]->addServer( *task );
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
    const bool verbose = flags.trace_convergence || flags.verbose;
    SolveSubmodel solveSubmodel( *this, verbose );		/* Helper class for iterator */

    double delta = 0.0;
    do {
	do {
	    _iterations += 1;
	    if ( verbose ) std::cerr << "Iteration: " << _iterations << " ";

	    std::for_each( _submodels.begin(), &_submodels[_HWSubmodel], solveSubmodel );

	    delta = std::for_each( __task.begin(), __task.end(), ExecSumSquare<Task,double>( &Task::deltaUtilization ) ).sum();
	    delta = sqrt( delta / __task.size() );		/* RMS */

	    if ( delta > __convergence_value ) {
		backPropogate();
	    }
	} while ( delta > __convergence_value &&  _iterations < __iteration_limit );		/* -- Step 4 -- */

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

	delta = std::for_each( __processor.begin(), __processor.end(), ExecSumSquare<Processor,double>( &Processor::deltaUtilization ) ).sum();
	delta = sqrt( delta / __processor.size() );		/* RMS */
	if ( verbose ) std::cerr << " [" << delta << "]" << std::endl;

    } while ( ( _iterations < flags.min_steps || delta > __convergence_value ) && _iterations < __iteration_limit );

    _converged = (delta <= __convergence_value || _iterations == 1);	/* The model will never be converged with one step, so ignore */
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
    const bool verbose = flags.trace_convergence || flags.verbose;
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
	const unsigned int i = (*task)->submodel();
	if ( !(*task)->isReferenceTask() ) {
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
	if ( i <= 0 ) continue;		// Device not used.
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
Batch_Model::optimize()
{
    std::for_each( _submodels.begin(), _submodels.end(), Exec<Submodel>( &Submodel::optimize ) );
}



/*
 * Solve the model starting from the top and working down.  Return the
 * number of inner loop _iterations.
 */

double
Batch_Model::run()
{
    double delta = 0.0;
    const bool verbose = (flags.trace_convergence || flags.verbose) && !(flags.trace_mva || flags.trace_wait);
    const double count = __task.size() + __processor.size();

    do {
	_iterations += 1;
	if ( verbose ) std::cerr << "Iteration: " << _iterations << " ";

	std::for_each( _submodels.begin(), _submodels.end(), SolveSubmodel( *this, verbose ) );

	/* compute convergence for next pass. */

	delta =  std::for_each( __task.begin(), __task.end(), ExecSumSquare<Task,double>( &Task::deltaUtilization ) ).sum();
	delta += std::for_each( __processor.begin(), __processor.end(), ExecSumSquare<Processor,double>( &Processor::deltaUtilization ) ).sum();
	delta =  sqrt( delta / count );		/* RMS */

	if ( delta > __convergence_value ) {
	    backPropogate();
	}

	if ( flags.trace_intermediate && _iterations % __print_interval == 0 ) {
	    printIntermediate( delta );
	}

	if ( flags.trace_wait ) {
	    printSubmodelWait();
	}

	if ( flags.trace_mva || flags.trace_wait ) {
	    std::cout << std::endl << "-*- -- -*- " << _iterations << ":   " << delta << " -*- -- -*-" << std::endl << std::endl;
	} else if ( flags.verbose || flags.trace_convergence ) {
	    std::cerr << " [" << delta << "]" << std::endl;
	}
    } while ( ( _iterations < flags.min_steps || delta > __convergence_value ) && _iterations < __iteration_limit );
    _converged = (delta <= __convergence_value || _iterations == 1);	/* The model will never be converged with one step, so ignore */
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
    const bool verbose = (flags.trace_convergence || flags.verbose) && !(flags.trace_mva || flags.trace_wait);
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

    std::multiset<Entity *> servers;
    for ( std::set<Task *>::const_iterator task = __task.begin(); task != __task.end(); ++task ) {
	if ( (*task)->isReferenceTask() || (*task)->submodel() <= 0 ) continue;
	servers.insert( *task );
    }
    for ( std::set<Processor *>::const_iterator processor = __processor.begin(); processor != __processor.end(); ++processor ) {
	if ( (*processor)->submodel() <= 0 ) continue;
	servers.insert( *processor );
    }
    if ( __think_server && __think_server->submodel() > 0 ) {
        servers.insert( __think_server );
    }

    std::for_each( servers.begin(), servers.end(), Exec1<Entity,unsigned int>( &Entity::setSubmodel, 1 ) );

    return 0;
}

/*----------------------------------------------------------------------*/
/*                        Squashed Layers Model                         */
/*----------------------------------------------------------------------*/

/*
 * The trick for srvn layers is to assign each server to its own
 * submodel.  The sorter here simple re-sorts the graph so that each
 * task and processor is assigned a unigue ``depth''.  Then, the batch
 * layerizer (which simply iterates among the submodels) takes over.
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
    if ( __think_server && __think_server->submodel() > 0 ) {
        __think_server->setSubmodel( 1 );
    }

    return 1;
}

/*----------------------------------------------------------------------*/
/*                          HwSw Layers Model                           */
/*----------------------------------------------------------------------*/

/*
 * The trick for srvn layers is to assign each server to its own
 * submodel.  The sorter here simple re-sorts the graph so that each
 * task and processor is assigned a unigue ``depth''.  Then, the batch
 * layerizer (which simply iterates among the submodels) takes over.
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
    if ( __think_server && __think_server->submodel() > 0 ) {
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
    submodel->solve( _model._iterations, _model._MVAStats[depth], _model.relaxation() );  //REP N-R
}
