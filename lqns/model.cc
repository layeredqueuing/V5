/* -*- c++ -*-
 * $Id: model.cc 13562 2020-05-27 02:01:45Z greg $
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
 * 2) Call generate().  Generate calls addToSubmodel() to add server stations
 *    to the basic model then calls initialize() to set up the station
 *    parameters.
 * 3) Call solve().
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


#include "dim.h"
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <lqio/error.h>
#include <lqio/filename.h>
#include <lqio/input.h>
#include "errmsg.h"
#include "cltn.h"
#include "fpgoop.h"
#include "stack.h"
#include "model.h"
#include "entity.h"
#include "task.h"
#include "group.h"
#include "processor.h"
#include "entry.h"
#include "call.h"
#include "server.h"
#include "mva.h"
#include "open.h"
#include "submodel.h"
#include "synmodel.h"
#include "lqns.h"
#include "variance.h"
#include "pragma.h"
#include "option.h"
#include "generate.h"
#include "phase.h"
#include "activity.h"
#include "interlock.h"
#include "actlist.h"
#include "report.h"
#include "runlqx.h"
#include <lqio/srvn_output.h>

double Model::convergence_value = 0.00001;
unsigned Model::iteration_limit = 50;;
double Model::underrelaxation = 1.0;
unsigned Model::print_interval = 1;
Processor * Model::thinkServer = 0;
unsigned Model::sync_submodel = 0;
LQIO::DOM::Document::input_format Model::input_format = LQIO::DOM::Document::AUTOMATIC_INPUT;

/*
 * Compare two entities by their submodel. 
 */

struct lt_submodel
{
    bool operator()(const Entity * e1, const Entity * e2) const
	{
	    return (e1->submodel() < e2->submodel()) 
		|| (e1->submodel() == e2->submodel() && (!e2->domEntity || (e1->domEntity && (e1->domEntity->getId() < e2->domEntity->getId()))));
	}
};

/*----------------------------------------------------------------------*/
/*                           Factory Methods                            */
/*----------------------------------------------------------------------*/

/*
 * Factory method for creating the layers and initializing the MVA submodels
 */

Model *
Model::createModel( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName, bool check_model )
{
    Model *aModel = 0;

    Activity::actConnections.clear();
    Activity::domToNative.clear();
    MVA::boundsLimit = pragma.getTau();

    /*
     * Fold, Mutilate and Spindle before main loop processing in solve.c
     * disable model checking and expansion at this stage with LQX programs
     */

    if ( io_vars.anError() == false && (check_model == false || checkModel() ) ) {

	extendModel();			/* Do this before initProcessors() */

	if( check_model )
	    initProcessors();		/* Set Processor Service times.	*/

	switch ( pragma.getLayering() ) {
	case BATCHED_LAYERS:
	    aModel = new Batch_Model( document, inputFileName, outputFileName );
	    break;

	case BACKPROPOGATE_LAYERS:
	    aModel = new BackPropogate_Batch_Model( document, inputFileName, outputFileName );
	    break;

	case METHOD_OF_LAYERS:
	    aModel = new MOL_Model( document, inputFileName, outputFileName );
	    break;

	case BACKPROPOGATE_METHOD_OF_LAYERS:
	    aModel = new BackPropogate_MOL_Model( document, inputFileName, outputFileName );
	    break;

	case SRVN_LAYERS:
	    aModel = new SRVN_Model( document, inputFileName, outputFileName );
	    break;

	case SQUASHED_LAYERS:
	    aModel = new Squashed_Model( document, inputFileName, outputFileName );
	    break;

	case HWSW_LAYERS:
	    aModel = new HwSw_Model( document, inputFileName, outputFileName );
	    break;
	}

	assert( aModel != 0 );

	try {
	    if ( check_model ) {
		set<Task *,ltTask>::const_iterator nextTask = task.begin();
		aModel->generate( nextTask );
		if ( !io_vars.anError() ) {
		    aModel->setInitialized();
		} else {
		    delete aModel;
		    aModel = 0;
		}
	    }
	}
	catch ( const exception_handled& e ) {
	    delete aModel;
	    aModel = 0;
	}
    }
    return aModel;
}

bool
Model::initializeModel()
{
    /* perform all actions normally done in createModel() that need to be delayed until after */
    /* LQX programs begin execution to avoid problems with unset variables */

    checkModel();

    if ( !_model_initialized ) {

	initProcessors();		/* Set Processor Service times.	*/

	set<Task *,ltTask>::const_iterator nextTask = task.begin();
	this->generate( nextTask );

	_model_initialized = true;
    }
    return !io_vars.anError();
}


LQIO::DOM::Document*
Model::load( const string& input_filename, const string& output_filename )
{
    if ( flags.verbose ) {
	cerr << "Load: " << input_filename << "..." << endl;
    }

    /*
     * Initialize everything that needs it before parsing
     */

    set_fp_ok( false );			// Reset floating point. -- stop on overflow?
    if ( matherr_disposition == FP_IMMEDIATE_ABORT ) {
	set_fp_abort();
    }

    io_vars.reset();
    Entry::reset();
    Task::reset();
    ActivityList::reset();

    /*
     * Read input file and parse it.
     */

    unsigned errorCode = 0;

    /* Attempt to load in the document from the filename/ptr and configured io_vars */
    return LQIO::DOM::Document::load(input_filename, input_format, &::io_vars, errorCode, false);
}


/* 
 * Factory.
 */

bool
Model::prepare(const LQIO::DOM::Document* document)
{
    /* Tell the user that we are starting to load up */
    DEBUG(endl << "[0]: Beginning model load, setting parameters." << endl);

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 0: Add Pragmas] */
    const map<string,string>& pragmaList = document->getPragmaList();
    for ( map<string,string>::const_iterator pragmaIter = pragmaList.begin(); pragmaIter != pragmaList.end(); ++pragmaIter) {
	pragma.set( pragmaIter->first, pragmaIter->second );
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */

    /* We need to add all of the processors */
    const std::map<std::string,LQIO::DOM::Processor *>& procList = document->getProcessors();

    /* Add all of the processors we will be needing */
    for ( std::map<std::string,LQIO::DOM::Processor *>::const_iterator nextProc = procList.begin(); nextProc != procList.end(); ++nextProc ) {
	LQIO::DOM::Processor* processor = nextProc->second;
	DEBUG("[1]: Adding processor (" << processor->name() << ")" << endl);
	Processor::create(processor);
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1.5: Add Groups] */

    const std::map<std::string,LQIO::DOM::Group*>& groups = const_cast<LQIO::DOM::Document*>(document)->getGroups();
    std::map<std::string,LQIO::DOM::Group*>::const_iterator groupIter;
    for (groupIter = groups.begin(); groupIter != groups.end(); ++groupIter) {
	LQIO::DOM::Group* domGroup = const_cast<LQIO::DOM::Group*>(groupIter->second);
	DEBUG("[G]: Adding Group (" << domGroup->getName() << ")" << endl);
	add_group(domGroup);
    }

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */

    /* In the DOM, tasks have entries, but here entries need to go first */
    const std::map<std::string,LQIO::DOM::Task*>& taskList = document->getTasks();
    std::vector<Activity*> activityList;

    /* Add all of the processors we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator nextTask = taskList.begin(); nextTask != taskList.end(); ++nextTask ) {
	LQIO::DOM::Task* task = nextTask->second;

	/* Before we can add a task we have to add all of its entries */
	DEBUG("[2]: Preparing to add entries for Task (" << task->name() << ")" << endl);

	/* Prepare to iterate over all of the entries */
	Cltn<Entry*> entryCollection;
	std::vector<LQIO::DOM::Entry*>::const_iterator nextEntry;
	std::vector<LQIO::DOM::Entry*> activityEntries;

	/* Add the entries so we can reverse them */
	for ( nextEntry = task->getEntryList().begin(); nextEntry != task->getEntryList().end(); ++nextEntry ) {
	    entryCollection << Entry::create( *nextEntry, entryCollection.size() );
	    if ((*nextEntry)->getStartActivity() != NULL) {
		activityEntries.push_back(*nextEntry);
	    }
	}

	/* Now we can go ahead and add the task */
	DEBUG("[3]: Adding Task (" << name << ")" << endl);

	Task* newTask = Task::create(task, &entryCollection);

	/* Add activities for the task (all of them) */
	const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
	std::map<std::string,LQIO::DOM::Activity*>::const_iterator iter;
	for (iter = activities.begin(); iter != activities.end(); ++iter) {
	    const LQIO::DOM::Activity* activity = iter->second;
	    DEBUG("[3][a]: Adding Activity (" << activity->getName() << ") to Task." << endl);
	    activityList.push_back(add_activity(newTask, const_cast<LQIO::DOM::Activity*>(activity)));
	}

	/* Set all the start activities */
	for (nextEntry = activityEntries.begin(); nextEntry != activityEntries.end(); ++nextEntry) {
	    LQIO::DOM::Entry* theDOMEntry = *nextEntry;
	    DEBUG("[3][b]: Setting Start Activity (" << theDOMEntry->getStartActivity()->getName().c_str()
		  << ") for Entry (" << theDOMEntry->getName().c_str() << ")" << endl);
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
	assert(newEntry != NULL);

	newEntry->setEntryInformation( entry );

	/* Go over all of the entry's phases and add the calls */
	for (unsigned p = 1; p <= entry->getMaximumPhase(); ++p) {
	    LQIO::DOM::Phase* phase = entry->getPhase(p);
	    const std::vector<LQIO::DOM::Call*>& originatingCalls = phase->getCalls();
	    std::vector<LQIO::DOM::Call*>::const_iterator iter;

	    /* Add all of the calls to the system */
	    for (iter = originatingCalls.begin(); iter != originatingCalls.end(); ++iter) {
		LQIO::DOM::Call* call = *iter;

#if defined(DEBUG_MESSAGES)
		LQIO::DOM::Entry* src = const_cast<LQIO::DOM::Entry*>(call->getSourceEntry());
		LQIO::DOM::Entry* dst = const_cast<LQIO::DOM::Entry*>(call->getDestinationEntry());
		DEBUG("[4]: Phase " << call->getPhase() << " Call (" << src->getName() << ") -> (" << dst->getName() << ")" << endl);
#endif

		/* Add the call to the system */
		newEntry->add_call( p, call );
	    }

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
	DEBUG("[4]: Adding Task (" << theTask->name() << ") Activity (" << activity->name() << ") Calls and Lists." << endl);
	activity->add_calls()
	    .add_reply_list()
	    .add_activity_lists();
    }

    /* Use the generated connections list to finish up */
    DEBUG("[5]: Adding connections." << endl);
    complete_activity_connections ();

    /* Tell the user that we have finished */
    DEBUG("[0]: Finished loading the model" << endl << endl);
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

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->recalculateDynamicValues();
    }
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->recalculateDynamicValues();
    }
}


/*
 * Check input parameters.  Return true if all went well.  Return false
 * and set anError to true otherwise.
 */

bool
Model::checkModel()
{
    /* non-ref entries should receive either RV requests or open arrivals*/

    unsigned ref_tasks = 0;
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	aTask->check();

	if ( aTask->isReferenceTask() ) {
	    ref_tasks += 1;
	}
    }

    if ( ref_tasks == 0 && Entry::totalOpenArrivals == 0 ) {
	LQIO::solution_error( LQIO::ERR_NO_REFERENCE_TASKS );
    }

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	const Processor * aProcessor = *nextProcessor;
	aProcessor->check();
    }

    return !io_vars.anError();
}



/*
 * Delete the model.
 */

void
Model::dispose()
{
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	delete aProcessor;
    }
    processor.clear();

    for ( set<Group *,ltGroup>::const_iterator nextGroup = group.begin(); nextGroup != group.end(); ++nextGroup ) {
	const Group * aGroup = *nextGroup;
	delete aGroup;
    }
    group.clear();

    if ( thinkServer ) {
	delete thinkServer;
	thinkServer = 0;
    }

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	delete aTask;
    }
    task.clear();

    entry.clear();
}

/*
 *	Called from the parser to set important modelling parameters.
 *	It can also check validity of same if so desired.
 */

void
Model::setModelParameters( const LQIO::DOM::Document* doc )
{

    if ( !flags.override_print_interval ) {
	print_interval = doc->getModelPrintIntervalValue();
    }
    if ( !flags.override_iterations ) {
	int it_limit = doc->getModelIterationLimitValue();
	if ( it_limit < 5 ) {
	    LQIO::input_error2( ADV_ITERATION_LIMIT, it_limit, iteration_limit );
	} else {
	    iteration_limit = it_limit;
	}
    }
    if ( !flags.override_convergence ) {
	double conv_val = doc->getModelConvergenceValue();
	if ( conv_val <= 0 ) {
	    LQIO::input_error2( ADV_CONVERGENCE_VALUE, conv_val, convergence_value );
	} else {
	    if ( conv_val > 0.01 ) {
		LQIO::input_error2( ADV_LARGE_CONVERGENCE_VALUE, conv_val );
	    }
	    convergence_value = conv_val;
	}
    }
    if ( !flags.override_underrelaxation ) {
	double under = doc->getModelUnderrelaxationCoefficientValue();
	if ( under <= 0.0 || 2.0 < under ) {
	    LQIO::input_error2( ADV_UNDERRELAXATION, under, underrelaxation );
	} else {
	    underrelaxation = under;
	}
    }
}

/*----------------------------------------------------------------------*/
/*                         Abstract Superclass.                         */
/*----------------------------------------------------------------------*/

/*
 * Constructor.
 */

Model::Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName )
    : _converged(false), _iterations(0), _max_depth(1), _step_count(0), _model_initialized(false), _document(document), _input_file_name(inputFileName), _output_file_name(outputFileName)
{
    sync_submodel = 0;
}

/*
 * Destructor.  Free client and server arrays.
 */

Model::~Model()
{
    _submodels.deleteContents();
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

unsigned
Model::generate( set<Task *,ltTask>::const_iterator& nextTask )
{
    _max_depth = assignSubmodel( nextTask );

    _submodels.grow(_max_depth);

    for ( unsigned i = 1; i <= _max_depth; ++i ) {
	_submodels[i] = new MVASubmodel(i,this);
    }

    /* Add submodel for join delay calculation */

    if ( ActivityList::n_forks || ActivityList::n_joins ) {
	_submodels.grow(1);
	_max_depth += 1;
	sync_submodel = _max_depth;
	_submodels[sync_submodel] = new SynchSubmodel(sync_submodel,this);
    }

    /* Build model. */

//    prune();				/* Delete unused stuff. 	*/
    addToSubmodel();			/* Add tasks to layers.		*/
    MVAStats.grow(_max_depth);		/* MVA statistics by level.	*/
    configure();			/* Dimension arrays and threads	*/
    initialize();			/* Init MVA values (pop&waits). */		/* -- Step 2 -- */

    if ( Options::Debug::layers() ) {
	printLayers( cout );		/* Print out layers... 		*/
    }

    return _max_depth;
}



unsigned
Model::pruneLayers()
{
    unsigned count = 0;

    for ( unsigned i = 1; i <= _max_depth ;++i ) {
	while ( i <= _max_depth && !(*_submodels[i]) ) {
	    count += 1;
	    delete _submodels[i];
	    for ( unsigned j = i; j < _max_depth; ++j ) {
		_submodels[j] = _submodels[j+1];
		_submodels[j]->number( j );
	    }
	    _max_depth -= 1;
	}
    }
    _submodels.chop( count );

    if ( sync_submodel ) {
	sync_submodel -= count;
    }
    return count;
}



/*
 * Add think centers for think time.
 */

void
Model::extendModel()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;

	/* Add a delay server for think times. */

	if ( aTask->hasThinkTime() ) {
	    if ( !thinkServer ) {
		thinkServer = new DelayServer();
	    }
	    thinkServer->addTask( aTask );	/* link the task in */
	}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
	if ( pragma.getQuorumDelayedCalls() == KEEP_ALL_QUORUM_DELAYED_CALLS &&
	     aTask->hasForks() && !flags.disable_expanding_quorum_tree ) {
	    aTask->expandQuorumGraph();
	}
#endif
    }
}




/*
 * Processor entries are created automagically once the model has been
 * completely loaded in.  Think entries are done here too.
 */

void
Model::initProcessors()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->initProcessor();
    }
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
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->configure( nSubmodels() );
    }

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->configure( nSubmodels() );
    }

    if ( thinkServer ) {
	thinkServer->configure( nSubmodels() );
    }
}



/*
 * Initialize waiting times, interlocking etc.  Servers are done by
 * subclasses because of the structure of the model.  Clients are done
 * here.
 */

void
Model::initialize()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->initInterlock();
    }

    Sequence<Submodel *> nextSubmodel( _submodels );
    BackwardsSequence<Submodel *> prevSubmodel( _submodels );
    Submodel * aSubmodel;

    /* Initialize waiting times and populations at servers
     * Done in reverse order (bottom up) because waits propogate
     * upwards. */

    while ( aSubmodel = prevSubmodel() ) {
	aSubmodel->initServers( *this );
    }

    /* Initialize waiting times and populations for the reference tasks. */

    initClients();

    if ( Options::Debug::interlock() ) {
	Interlock::printPathTable( cout );
    }

    /* Initialize Interlocking */

    while ( aSubmodel = nextSubmodel() ) {
	aSubmodel->initInterlock();
    }

    while ( aSubmodel = nextSubmodel() ) {
	if ( aSubmodel->number() != flags.skip_submodel ) {
	    aSubmodel->build();		/* For use by MVA stats/generate*/
	}
    }

}



/*
 * Re-initialize waiting times, interlocking etc.  Servers are done by
 * subclasses because of the structure of the model.  Clients are done
 * here.
 */

Model&
Model::reinitialize()
{
    /*
     * Reset all counters before a run.  In particular, iterations
     * needs to reset to zero for the convergence test.
     */


    _iterations = 0;
    _step_count = 0;
    for ( unsigned i = 1; i <= _max_depth; ++i ) {
	MVAStats[i].initialize();
    }

    for ( set<Entry *,ltEntry>::const_iterator nextEntry = entry.begin(); nextEntry != entry.end(); ++nextEntry ) {
	Entry * anEntry = *nextEntry;
	anEntry->resetInterlock();
    }

    /*
     * Reinitialize the MVA stuff
     */

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->initInterlock();
    }

    Sequence<Submodel *> nextSubmodel( _submodels );
    BackwardsSequence<Submodel *> prevSubmodel( _submodels );
    Submodel * aSubmodel;

    /* Initialize waiting times and populations at servers
     * Done in reverse order (bottom up) because waits propogate
     * upwards. */

    while ( aSubmodel = prevSubmodel() ) {
	aSubmodel->reinitServers( *this );
	aSubmodel->reinitClients();
    }

    /* Initialize waiting times and populations for the reference tasks. */

    reinitClients();

    /* Reinitialize Interlocking */

    while ( aSubmodel = nextSubmodel() ) {
	aSubmodel->reinitInterlock();
    }

    while ( aSubmodel = nextSubmodel() ) {
	if ( aSubmodel->number() != flags.skip_submodel ) {
	    aSubmodel->rebuild();		/* For use by MVA stats/generate*/
	}
    }

    return *this;
}



/*
 * Go through and initialize all the reference tasks as they are not
 * caught the first time round.
 */

void
Model::initClients()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aClient = *nextTask;
	if ( aClient->isReferenceTask() ) {
	    aClient->initWait();
	    updateWait( aClient );	/* Initialize over all submodels. */
	    aClient->computeVariance();
	    aClient->initThroughputBound();
	    aClient->initPopulation();
	    aClient->initThreads();
	}
    }
}



/*
 * Go through and initialize all the reference tasks as they are not
 * caught the first time round.
 */

void
Model::reinitClients()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aClient = *nextTask;
	if ( aClient->isReferenceTask() ) {
	    updateWait( aClient );	/* Initialize over all submodels. */
	    aClient->computeVariance();
	    aClient->initThroughputBound();
	    aClient->initPopulation();
//	    aClient->initThreads();
	}
    }
}



/*
 * Read model description.  Return true if all went well.
 */

bool
Model::solve()
{
    SolverReport report( const_cast<LQIO::DOM::Document *>(_document) );
    setModelParameters( _document );

    if ( flags.no_execute || flags.bounds_only ) {
	return true;
    }

    report.start();
    reinitialize();

    _converged = false;
    const double delta = run();
    report.finish( delta, *this );
    sanityCheck();
    if ( !_converged ) {
	LQIO::solution_error( ADV_SOLVER_ITERATION_LIMIT, _iterations, delta, convergence_value );
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
    const string directoryName = createDirectory();
    const string suffix = lqx_output ? SolverInterface::Solve::customSuffix : "";

    /* override is true for '-p -o filename.out filename.in' == '-p filename.in' */

    bool override = false;
    if ( hasOutputFileName() && LQIO::Filename::isRegularFile( _output_file_name.c_str() ) != 0 ) {
	LQIO::Filename filename( _input_file_name.c_str(), flags.rtf_output ? "rtf" : "out" );
	override = filename() == _output_file_name;
    }

    if ( override || ((!hasOutputFileName() || directoryName.size() > 0 ) && _input_file_name != "-" )) {
	if ( _document->getInputFormat() == LQIO::DOM::Document::XML_INPUT || flags.xml_output ) {	/* No parseable/json output, so create XML */
	    LQIO::Filename filename( _input_file_name.c_str(), "lqxo", directoryName.c_str(), suffix.c_str() );
	    ofstream output;
	    filename.backup();
	    output.open( filename(), ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::XML_OUTPUT );
		output.close();
	    }
	}

	/* Parseable output. */

	if ( ( _document->getInputFormat() == LQIO::DOM::Document::LQN_INPUT && lqx_output ) || flags.parseable_output ) {
	    LQIO::Filename filename( _input_file_name.c_str(), "p", directoryName.c_str(), suffix.c_str() );
	    ofstream output;
	    output.open( filename(), ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::PARSEABLE_OUTPUT );
		output.close();
	    }
	}

	/* Regular output */

	LQIO::Filename filename( _input_file_name.c_str(), flags.rtf_output ? "rtf" : "out", directoryName.c_str(), suffix.c_str() );

	ofstream output;
	output.open( filename(), ios::out );
	if ( !output ) {
	    solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	} else if ( flags.rtf_output ) {
	    _document->print( output, LQIO::DOM::Document::RTF_OUTPUT );
	} else {
	    _document->print( output );
	    if ( flags.print_overtaking ) {
		printOvertaking( output );
	    }
	}
	output.close();

    } else if ( _output_file_name == "-" || _input_file_name == "-" ) {

	if ( flags.parseable_output ) {
	    _document->print( cout, LQIO::DOM::Document::PARSEABLE_OUTPUT );
	} else if ( flags.rtf_output ) {
	    _document->print( cout, LQIO::DOM::Document::RTF_OUTPUT );
	} else {
	    _document->print( cout );
	    if ( flags.print_overtaking ) {
		printOvertaking( cout );
	    }
	}

    } else {

	/* Do not map filename. */

	LQIO::Filename::backup( _output_file_name.c_str() );

	ofstream output;
	output.open( _output_file_name.c_str(), ios::out );
	if ( !output ) {
	    solution_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
	} else if ( flags.xml_output ) {
	    _document->print( output, LQIO::DOM::Document::XML_OUTPUT );
	} else if ( flags.parseable_output ) {
	    _document->print( output, LQIO::DOM::Document::PARSEABLE_OUTPUT );
	} else if ( flags.rtf_output ) {
	    _document->print( output, LQIO::DOM::Document::RTF_OUTPUT );
	} else {
	    _document->print( output );
	    if ( flags.print_overtaking ) {
		printOvertaking( output );
	    }
	}
	output.close();
    }
    if ( flags.verbose ) {
	report.print( cout );
    }

    cout.flush();
    cerr.flush();
    return true;
}



/*
 * Output the results.
 */

bool
Model::reload()
{
    /* Default mapping */

    LQIO::Filename directory_name( hasOutputFileName() ? _output_file_name.c_str() : _input_file_name.c_str(), "d" );		/* Get the base file name */

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
 * Solve the submodel.  Return largest delta value encountered.
 */

Model&
Model::solve( Sequence<Submodel *>& nextSubmodel )
{
    Submodel * aSubmodel;

    while ( aSubmodel = nextSubmodel() ) {
	const unsigned depth = aSubmodel->number();
	if ( depth == flags.skip_submodel ) continue;

	_step_count += 1;

	aSubmodel->solve( _iterations, MVAStats[depth], relaxation() );

    }

    return *this;
}



/*
 * Solve a single sub-model.
 */

Model&
Model::solveSubmodel( Submodel * aSubmodel )
{
    _step_count += 1;
    aSubmodel->solve( _iterations, MVAStats[aSubmodel->number()], relaxation() );  //REP N-R
    return *this;
}



/*
 * Final waiting time initilization -- updateWait/joinDelay.
 */

void
Model::updateWait( Entity * aTask ) const
{
    Submodel * aSubmodel;
    Sequence<Submodel *> nextSubmodel( _submodels );

    while ( aSubmodel = nextSubmodel() ) {
	aTask->updateWait( *aSubmodel, 1.0 );
    }
}




/*
 * Check results for sanity.
 */

void
Model::sanityCheck()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->sanityCheck();
    }

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->sanityCheck();
    }
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
	return underrelaxation;
    }
}


void
Model::insertDOMResults() const
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->insertDOMResults();
    }

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	const Processor * aProcessor = *nextProcessor;
	aProcessor->insertDOMResults();
    }
}


/*
 * Print out layers.
 */

ostream&
Model::printLayers( ostream& output ) const
{
    Submodel * aSubmodel;
    Sequence<Submodel *> nextSubmodel( _submodels );

    while ( aSubmodel = nextSubmodel() ) {
	output << *aSubmodel;
    }
    return output;
}



/*
 * Intermediate result printer.
 */

void
Model::printIntermediate( const double convergence ) const
{
    SolverReport report( const_cast<LQIO::DOM::Document *>(_document) );

    const string directoryName = createDirectory();
    const string suffix = _document->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix : "";

    report.insertDOMResults();
    insertDOMResults();

    string extension;
    if ( flags.parseable_output ) {
	extension = "p";
    } else if ( flags.xml_output ) {
	extension = "lqxo";
    } else if ( flags.rtf_output ) {
	extension = "rtf";
    } else {
	extension = "out";
    }

    LQIO::Filename filename( _input_file_name, extension.c_str(), directoryName.c_str(), suffix.c_str() );

    /* Make filename look like an emacs autosave file. */
    filename << "~" << _iterations << "~";

    report.finish( convergence, *this );	/* Save results */

    ofstream output;
    output.open( filename(), ios::out );

    if ( !output ) return;			/* Ignore errors */

    if ( flags.xml_output ) {
	_document->print( output, LQIO::DOM::Document::XML_OUTPUT );
    } else if ( flags.parseable_output ) {
	_document->print( output, LQIO::DOM::Document::PARSEABLE_OUTPUT );
    } else {
	_document->print( output );
    }
    output.close();
}


ostream&
Model::printSubmodelWait( ostream& output ) const
{
    int precision = output.precision(6);
    ios_base::fmtflags flags = output.setf( ios::left, ios::adjustfield );
    output.setf( ios::left, ios::adjustfield );

    output << setw(8) <<  "Submodel    ";
    for ( unsigned i = 1; i <= nSubmodels(); ++i ) {
	output << setw(8) << i;
    }
    output << endl;

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	aTask->printSubmodelWait( output );
    }

    output.setf( flags );
    output.precision( precision );
    return output;
}



ostream& 
Model::printOvertaking( ostream& output ) const
{
    ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
    output << endl << endl << "Inservice probabilities (p->'a'):" << endl << endl;
    for ( unsigned int i = 0; i < 4; ++i ) {
	output << "Entry " << "abcd"[i] << setw(LQIO::SRVN::ObjectOutput::__maxStrLen-7) << " ";
    }
    output << "p_d ";
    const unsigned int n_phases = _document->getMaximumPhase();
    for ( unsigned int p = 1; p <= n_phases; ++p ) {
	output << "Phase " << p << setw(LQIO::SRVN::ObjectOutput::__maxDblLen-7) << " ";
    }
    output << endl;

    for ( set<Task *,ltTask>::const_iterator nextServer = task.begin(); nextServer != task.end(); ++nextServer ) {
	const Task * aServer = *nextServer;
	if ( aServer->markovOvertaking() ) {
	    aServer->printOvertaking( output );
	}
    }
    return output;
    output.flags(oldFlags);
}


/*
 * Create a directory (if needed)
 */

string
Model::createDirectory() const
{
    string directoryName;
    if ( hasOutputFileName() && LQIO::Filename::isDirectory( _output_file_name.c_str() ) > 0 ) {
	directoryName = _output_file_name;
    }

    if ( _document->getResultInvocationNumber() > 0 ) {
	if ( directoryName.size() == 0 ) {
	    /* We need to create a directory to store output. */
	    LQIO::Filename filename( hasOutputFileName() ? _output_file_name.c_str() : _input_file_name.c_str(), "d" );		/* Get the base file name */
	    directoryName = filename();
	}
    }

    if ( directoryName.size() > 0 ) {
	int rc = access( directoryName.c_str(), R_OK|W_OK|X_OK );
	if ( rc < 0 ) {
	    if ( errno == ENOENT ) {
#if defined(WINNT)
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
 * Sort tasks into layers.  Start from reference tasks only and tasks
 * with open arrivals.  If a task has open arrivals, start from level
 * 1 so that it is treated as a server.
 */

unsigned
Model::topologicalSort( set<Task *,ltTask>::const_iterator& nextTask )
{
    CallStack callStack( task.size() + 2 );
    unsigned max_depth = 0;

    for ( ; nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	const int initialLevel = aTask->rootLevel();
	if ( initialLevel >= 0 ) {
	    callStack.grow( initialLevel );
	    try {
		max_depth = max( aTask->findChildren( callStack, true ), max_depth );
		callStack.shrink( initialLevel );
	    }
	    catch( call_cycle& error ) {
		callStack.shrink( error.depth() );
		max_depth = max( error.depth(), max_depth );
		LQIO::solution_error( LQIO::ERR_CYCLE_IN_CALL_GRAPH, error.what() );
	    }
	    assert ( callStack.size() == 0 );
	}
    }

    if ( io_vars.anError() ) {
	throw exception_handled( "Model::topologicalSort" );	//
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
MOL_Model::assignSubmodel( set<Task *,ltTask>::const_iterator& nextTask )
{
    unsigned max_layers = topologicalSort( nextTask );
    HWSubmodel = max_layers;
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

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( !aTask->isReferenceTask() ) {
	    _submodels[aTask->submodel()]->addServer( aTask );
	}
	if ( aTask->hasForks() ) {
	    _submodels[sync_submodel]->addClient( aTask );
	}
	if ( aTask->hasJoins() ) {
	    _submodels[sync_submodel]->addServer( aTask );
	}
    }

    /* Processors and other devices go in submodel n */

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->setSubmodel( HWSubmodel );		// Force all devices to this level
	_submodels[HWSubmodel]->addServer( aProcessor );
    }

    if ( thinkServer ) {
	thinkServer->setSubmodel( HWSubmodel );		// Force all devices to this level
	_submodels[HWSubmodel]->addServer(thinkServer);
    }

    const unsigned count = pruneLayers();		/* Remove gaps 			*/
    HWSubmodel -= count;
}



/*
 * Solve the model starting from the top and working down.  Return the
 * number of inner loop _iterations.
 */

double
MOL_Model::run()
{
    double delta = 0.0;
    double count = 0.0;

    Sequence<Submodel *> nextSubmodel( _submodels, HWSubmodel-1 );

    do {
	do {
	    _iterations += 1;
	    if ( flags.trace_convergence || flags.verbose ) cerr << "Iteration: " << _iterations << " ";

	    solve( nextSubmodel );			/* -- Step 3 -- */

	    delta = 0.0;
	    count = 0;
	    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
		const Task * aTask = *nextTask;
		delta += square( aTask->deltaUtilization() );
		count += 1;
	    }
	    delta = sqrt( delta / count );		/* RMS */

	    if ( delta > convergence_value ) {
		backPropogate();
	    }

	} while ( delta > convergence_value &&  _iterations < iteration_limit );		/* -- Step 4 -- */

	/* Print intermediate results if necessary */

	if ( flags.trace_intermediate ) {
	    printIntermediate( delta );
	}

	if ( flags.trace_wait ) {
	    printSubmodelWait();
	}

	/* Solve hardware model. */

	solveSubmodel( _submodels[HWSubmodel] );		/* -- Step 6 -- */

	if ( flags.trace_wait ) {
	    printSubmodelWait();
	}

	delta = 0.0;
	for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	    Processor * aProcessor = *nextProcessor;
	    delta += square( aProcessor->deltaUtilization() );
	    count += 1;
	}
	delta = sqrt( delta / count );		/* RMS */
	if ( flags.trace_convergence || flags.verbose ) cerr << " [" << delta << "]" << endl;

    } while ( ( _iterations < flags.min_steps || delta > convergence_value ) && _iterations < iteration_limit );

    _converged = (delta <= convergence_value);
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
    if ( _max_depth < 4 ) return;
    BackwardsSequence<Submodel *>prevSubmodel( _submodels, 2, sync_submodel ? sync_submodel - 1 : _max_depth - 2 );

    solve( prevSubmodel );
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
Batch_Model::assignSubmodel( set<Task *,ltTask>::const_iterator& nextTask  )
{
    return topologicalSort( nextTask );
}



/*
 * Stick the tasks into their appropriate layers.  Also stick
 * processors in.  Tasks and processors all go into the server arrays.
 */

void
Batch_Model::addToSubmodel()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	const int i = aTask->submodel();
	if ( i <= 0 ) continue;		/* Ignore - not used */
	if ( !aTask->isReferenceTask() ) {
	    _submodels[i]->addServer( aTask );
	}
	if ( aTask->hasForks() ) {
	    _submodels[sync_submodel]->addClient( aTask );
	}
	if ( aTask->hasJoins() ) { // Answer is always NO for now.
	    _submodels[sync_submodel]->addServer( aTask );
	}
    }

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	const int i = aProcessor->submodel();
	if ( i <= 0 ) continue;		// Device not used.
	_submodels[i]->addServer( aProcessor );
    }

    if ( thinkServer ) {
	const int i = thinkServer->submodel();
	if ( i > 0 ) {
	    _submodels[i]->addServer( thinkServer );
	}
    }

    pruneLayers();		/* Remove gaps 			*/
}



/*
 * Solve the model starting from the top and working down.  Return the
 * number of inner loop _iterations.
 */

double
Batch_Model::run()
{
    double delta = 0.0;

    Sequence<Submodel *> nextSubmodel( _submodels );

    do {
	_iterations += 1;
	if ( flags.trace_convergence || flags.verbose ) cerr << "Iteration: " << _iterations << " ";

	solve( nextSubmodel );

	/* compute convergence for next pass. */

	delta = 0.0;
	double count = 0;
	for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	    const Task * aTask = *nextTask;
	    delta += square( aTask->deltaUtilization() );
	    count += 1;
	}
	for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	    Processor * aProcessor = *nextProcessor;
	    delta += square( aProcessor->deltaUtilization() );
	    count += 1;
	}
	delta = sqrt( delta / count );		/* RMS */

	if ( delta > convergence_value ) {
	    backPropogate();
	}

	if ( flags.trace_intermediate && _iterations % print_interval == 0 ) {
	    printIntermediate( delta );
	}

	if ( flags.trace_wait ) {
	    printSubmodelWait();
	}

	if ( flags.trace_mva || flags.trace_wait ) {
	    cout << endl << "-*- -- -*- " << _iterations << ":   " << delta << " -*- -- -*-" << endl << endl;
	} else if ( flags.verbose || flags.trace_convergence ) cerr << " [" << delta << "]" << endl;

    } while ( ( _iterations < flags.min_steps || delta > convergence_value ) && _iterations < iteration_limit );

    _converged = (delta <= convergence_value);
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
    if ( _max_depth < 3 ) return;
    BackwardsSequence<Submodel *>prevSubmodel( _submodels, 2, sync_submodel ? sync_submodel - 1 : _max_depth - 1 );

    solve( prevSubmodel );
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
SRVN_Model::assignSubmodel( set<Task *,ltTask>::const_iterator& nextTask  )
{
    topologicalSort( nextTask );

    /* Build the list of all servers for this model */

    multiset<Entity *,lt_submodel> servers;
    for ( nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( aTask->isReferenceTask() || aTask->submodel() <= 0 ) continue;
	servers.insert( aTask );
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	if ( aProcessor->submodel() <= 0 ) continue;
	servers.insert( aProcessor );
    }
    if ( thinkServer && thinkServer->submodel() > 0 ) {
        servers.insert( thinkServer );
    }

    unsigned int submodel = 1;
    for ( multiset<Entity *,lt_submodel>::const_iterator next_server = servers.begin(); next_server != servers.end(); ++next_server, ++submodel ) {
        Entity * aServer = *next_server;
	aServer->setSubmodel( submodel );
    }

    return submodel - 1;		/* Initial submodel is one, so fix this. */ 
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
Squashed_Model::assignSubmodel( set<Task *,ltTask>::const_iterator& nextTask  )
{
    topologicalSort( nextTask );

    /* Build the list of all servers for this model */

    for ( nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( !aTask->isReferenceTask() && aTask->submodel() > 1 ) {
	    aTask->setSubmodel( 1 );
	}
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	if ( aProcessor->submodel() > 1 ) {
	    aProcessor->setSubmodel( 1 );
	}
    }
    if ( thinkServer && thinkServer->submodel() > 0 ) {
        thinkServer->setSubmodel( 1 );
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
HwSw_Model::assignSubmodel( set<Task *,ltTask>::const_iterator& nextTask  )
{
    topologicalSort( nextTask );

    /* Build the list of all servers for this model */

    for ( nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( !aTask->isReferenceTask() && aTask->submodel() > 1 ) {
	    aTask->setSubmodel( 1 );
	}
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	if ( aProcessor->submodel() > 1 ) {
	    aProcessor->setSubmodel( 2 );
	}
    }
    if ( thinkServer && thinkServer->submodel() > 0 ) {
        thinkServer->setSubmodel( 2 );
    }

    return 2;
}
