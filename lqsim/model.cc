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
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqsim/model.cc $
 *
 * $Id: model.cc 14794 2021-06-11 12:13:01Z greg $
 */

/* Debug Messages for Loading */
#if defined(DEBUG_MESSAGES)
#define DEBUG(x) cout << x
#else
#define DEBUG(X) 
#endif

#include "lqsim.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <parasol.h>
#include <para_internals.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif
#if HAVE_UNISTD_H
#include <sys/stat.h>
#include <unistd.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <lqio/input.h>
#include <lqio/error.h>
#include <lqio/filename.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
#include "runlqx.h"
#include "errmsg.h"
#include "model.h"
#include "activity.h"
#include "entry.h"
#include "task.h"
#include "instance.h"
#include "processor.h"
#include "group.h"
#include "pragma.h"

#if HAVE_SYS_TIMES_H
typedef struct tms tms_t;
#else
typedef clock_t tms_t;
#endif
#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

extern "C" {
    extern void test_all_stacks();
}

int Model::__genesis_task_id = 0;
Model * Model::__model = NULL;
double Model::max_service = 0.0;
const double Model::simulation_parameters::DEFAULT_TIME = 1e5;
LQIO::DOM::Document::InputFormat Model::input_format = LQIO::DOM::Document::InputFormat::AUTOMATIC;
bool deferred_exception = false;	/* domain error detected during run.. throw after parasol stops. */

/*----------------------------------------------------------------------*/
/*   Input processing.  Read input, extend and validate.                */
/*----------------------------------------------------------------------*/

/*
 * Initialize input parser parameters.
 */

Model::Model( LQIO::DOM::Document* document, const string& input_file_name, const string& output_file_name ) 
    : _document(document), _input_file_name(input_file_name), _output_file_name(output_file_name), _parameters(), _confidence(0.0)
{
    __model = this;

    /* Initialize globals */
    
    open_arrival_count		= 0;
    join_count			= 0;
    fork_count			= 0;
    max_service    		= 0.0;
    total_tasks    		= 0;
    client_init_count 		= 0;	/* For auto blocking (-C)	*/
}


Model::~Model()
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

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	delete aTask;
    }
    task.clear();
	
    for ( set<Entry *,ltEntry>::const_iterator nextEntry = entry.begin(); nextEntry != entry.end(); ++nextEntry ) {
	const Entry * anEntry = *nextEntry;
	delete anEntry;
    }
    entry.clear();

    Activity::actConnections.clear();
    Activity::domToNative.clear();

    if ( _document ) {
	delete _document;
    }
    __model = NULL;
}


LQIO::DOM::Document* 
Model::load( const string& input_filename, const string& output_filename )
{
    LQIO::io_vars.reset();

    /* This is a departure from before -- we begin by loading a model */

    unsigned errorCode = 0;
    return  LQIO::DOM::Document::load( input_filename, input_format, errorCode, false );
}


bool 
Model::construct()
{
    /* Tell the user that we are starting to load up */
    DEBUG(endl << "[0]: Beginning model load, setting parameters." << endl);
    if ( !override_print_int ) {
	print_interval = _document->getModelPrintIntervalValue();
    }
    Pragma::set( _document->getPragmaList() );
    LQIO::io_vars.severity_level = Pragma::__pragmas->severity_level();
    LQIO::Spex::__no_header = !Pragma::__pragmas->spex_header();
    
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */
	
    /* We need to add all of the processors */
    const std::map<std::string,LQIO::DOM::Processor*>& processorList = _document->getProcessors();

    /* Add all of the processors we will be needing */
    for ( std::map<std::string,LQIO::DOM::Processor*>::const_iterator nextProcessor = processorList.begin(); nextProcessor != processorList.end(); ++nextProcessor ) {
	LQIO::DOM::Processor* processor = nextProcessor->second;
	DEBUG("[1]: Adding processor (" << processor->getName() << ")" << endl);
	Processor::add(processor);
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1.5: Add Groups] */
	
    const std::map<std::string,LQIO::DOM::Group*>& groups = _document->getGroups();
    std::map<std::string,LQIO::DOM::Group*>::const_iterator groupIter;
    for (groupIter = groups.begin(); groupIter != groups.end(); ++groupIter) {
	LQIO::DOM::Group* domGroup = const_cast<LQIO::DOM::Group*>(groupIter->second);
	DEBUG("[G]: Adding Group (" << domGroup->getName() << ")" << endl);
	Group::add(domGroup);
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */
	
    /* In the DOM, tasks have entries, but here entries need to go first */
    const std::map<std::string,LQIO::DOM::Task*>& taskList = _document->getTasks();
	
    /* Add all of the tasks we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator nextTask = taskList.begin(); nextTask != taskList.end(); ++nextTask ) {
	LQIO::DOM::Task* task = nextTask->second;
	std::vector<LQIO::DOM::Entry*>::const_iterator nextEntry;
	std::vector<LQIO::DOM::Entry*> activityEntries;
		
	/* Before we can add a task we have to add all of its entries */
	DEBUG("[2]: Preparing to add entries for Task (" << task->getName() << ")" << endl);
		
	/* Now we can go ahead and add the task */
	DEBUG("[3]: Adding Task (" << name << ")" << endl);
	Task* newTask = Task::add(task);
		
	/* Add the entries so we can reverse them */
	for ( nextEntry = task->getEntryList().begin(); nextEntry != task->getEntryList().end(); ++nextEntry ) {
	    newTask->_entry.push_back( Entry::add( *nextEntry, newTask ) );
	    if ((*nextEntry)->getStartActivity() != NULL) {
		activityEntries.push_back(*nextEntry);
	    }
	}
		
	/* Add activities for the task (all of them) */
	const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
	std::map<std::string,LQIO::DOM::Activity*>::const_iterator iter;
	for (iter = activities.begin(); iter != activities.end(); ++iter) {
	    const LQIO::DOM::Activity* activity = iter->second;
	    DEBUG("[3][a]: Adding Activity (" << activity->getName() << ") to Task." << endl);
	    newTask->add_activity(const_cast<LQIO::DOM::Activity*>(activity));
	}
		
	/* Set all the start activities */
	std::vector<LQIO::DOM::Entry*>::iterator entryIter;
	for (entryIter = activityEntries.begin(); entryIter != activityEntries.end(); ++entryIter) {
	    LQIO::DOM::Entry* theDOMEntry = *entryIter;
	    DEBUG("[3][b]: Setting Start Activity (" << theDOMEntry->getStartActivity()->getName().c_str() 
		  << ") for Entry (" << theDOMEntry->getName().c_str() << ")" << endl);
	    newTask->set_start_activity(theDOMEntry);
	}
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 3: Add Calls/Phase Parameters] */
	
    /* Add all of the calls for all phases to the system */
    const std::map<std::string,LQIO::DOM::Entry*>& allEntries = _document->getEntries();
    for ( std::map<std::string,LQIO::DOM::Entry*>::const_iterator nextEntry = allEntries.begin(); nextEntry != allEntries.end(); ++nextEntry ) {
	LQIO::DOM::Entry* entry = nextEntry->second;
	Entry* newEntry = Entry::find(entry->getName().c_str());
	if ( newEntry == nullptr ) continue;

//	newEntry->setEntryInformation( entry );

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
		newEntry->add_call( p, call );			/* Add the call to the system */
	    }
			
	    newEntry->set_DOM(p, phase);    	/* Set the phase information for the entry */
	}
		
	/* Add in all of the P(frwd) calls */
	const std::vector<LQIO::DOM::Call*>& forwarding = entry->getForwarding();
	std::vector<LQIO::DOM::Call*>::const_iterator nextFwd;
	for ( nextFwd = forwarding.begin(); nextFwd != forwarding.end(); ++nextFwd ) {
	    Entry* targetEntry = Entry::find((*nextFwd)->getDestinationEntry()->getName().c_str());
	    newEntry->add_forwarding(targetEntry, *nextFwd );
	}

	/* Add reply */

	if ( newEntry->is_regular() ) {
	    const Task * aTask = newEntry->task();
	    if ( aTask->type() != Task::CLIENT && aTask->type() != Task::OPEN_ARRIVAL_SOURCE ) {
		newEntry->_phase[0].act_add_reply( newEntry );	
	    }
	}

    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 4: Add Calls/Lists for Activities] */
	
    /* Go back and add all of the lists and calls now that activities all exist */
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	for ( std::vector<Activity*>::const_iterator ap = aTask->_activity.begin(); ap != aTask->_activity.end(); ++ap) {
	    Activity* activity = *ap;
	    DEBUG("[4]: Adding Task (" << theTask->name() << ") Activity (" << activity->name() << ") Calls and Lists." << endl);
	    activity->add_calls()
		.add_reply_list()
		.add_activity_lists();
	}
    }
	
    /* Use the generated connections list to finish up */
    DEBUG("[5]: Adding connections." << endl);
    complete_activity_connections();
	
    /* Tell the user that we have finished */
    DEBUG("[0]: Finished loading the model" << endl << endl);

    return !LQIO::io_vars.anError();
}



/*
 * Construct all of the parasol tasks instances and then start them
 * up.  Some construction is needed after the simulation has
 * initialized and started simply because we need run-time
 * info. Called from ps_genesis() by ps_run_parasol().
 * Model::create() will call Task::initialize() which will call
 * XXX::initialize(). XXX:configure() is called from ::start() which
 * is called before ps_run_parasol().
 */

bool
Model::create()
{
    for ( unsigned j = 0; j < MAX_NODES; ++j ) {
	link_tab[j] = -1;		/* Reset link table.	*/
    }

    for_each( ::processor.begin(), ::processor.end(), Exec<Processor>( &Processor::create ) );
    for_each( ::group.begin(), ::group.end(), Exec<Group>( &Group::create ) );
    for_each( ::task.begin(), ::task.end(), Exec<Task>( &Task::create ) );

    if ( count_if( ::task.begin(), ::task.end(), Predicate<Task>( &Task::is_reference_task ) ) == 0 && open_arrival_count == 0 ) {
	LQIO::solution_error( LQIO::ERR_NO_REFERENCE_TASKS );
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

/*----------------------------------------------------------------------*/
/*			  Output Functions.				*/
/*----------------------------------------------------------------------*/

/*
 * Set up file descriptors and call proper output routine.
 */

void
Model::print()
{
    /* Parasol statistics if desired */

    if ( raw_stat_flag ) {
	print_raw_stats( stddbg );
    }

    const bool lqx_output = _document->getResultInvocationNumber() > 0;
    const string directory_name = createDirectory();
    const string suffix = lqx_output ? SolverInterface::Solve::customSuffix : "";

    /* override is true for '-p -o filename.out filename.in' == '-p filename.in' */
 
    bool override = false;
    if ( hasOutputFileName() && LQIO::Filename::isRegularFile( _output_file_name.c_str() ) != 0 ) {
	LQIO::Filename filename( _input_file_name.c_str(), global_rtf_flag ? "rtf" : "out" );
	override = filename() == _output_file_name;
    }

    /* SRVN type statistics */

    if ( override || ((!hasOutputFileName() || directory_name.size() > 0 ) && _input_file_name != "-" ) ) {

	if ( _document->getInputFormat() == LQIO::DOM::Document::InputFormat::XML || global_xml_flag ) {	/* No parseable/json output, so create XML */
	    ofstream output;
	    LQIO::Filename filename( _input_file_name, "lqxo", directory_name.c_str(), suffix.c_str() );
	    filename.backup();
	    output.open( filename(), ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::OutputFormat::XML );		// don't save LQX
		output.close();
	    }
	}
    
	/* Parseable output. */

	if ( ( _document->getInputFormat() == LQIO::DOM::Document::InputFormat::LQN && lqx_output ) || global_parse_flag ) {
	    ofstream output;
	    LQIO::Filename filename( _input_file_name, "p", directory_name.c_str(), suffix.c_str() );
	    output.open( filename(), ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	    } else {
		_document->print( output, LQIO::DOM::Document::OutputFormat::PARSEABLE );
		output.close();
	    }
	}

	/* Regular output */

	ofstream output;
	LQIO::Filename filename( _input_file_name, global_rtf_flag ? "rtf" : "out", directory_name.c_str(), suffix.c_str() );
	output.open( filename(), ios::out );
	if ( !output ) {
	    solution_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
	} else {
	    _document->print( output, global_rtf_flag ? LQIO::DOM::Document::OutputFormat::RTF : LQIO::DOM::Document::OutputFormat::LQN );
	    output.close();
	}

    } else if ( _output_file_name == "-" || _input_file_name == "-" ) {

	if ( global_parse_flag ) {
	    _document->print( cout, LQIO::DOM::Document::OutputFormat::PARSEABLE );
	} else {
	    _document->print( cout, global_rtf_flag ? LQIO::DOM::Document::OutputFormat::RTF : LQIO::DOM::Document::OutputFormat::LQN );
	}

    } else {

	/* Do not map filename. */

	LQIO::Filename::backup( _output_file_name.c_str() );

	ofstream output;
	output.open( _output_file_name.c_str(), ios::out );
	if ( !output ) {
	    solution_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
	} else if ( global_xml_flag ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::XML );
	} else if ( global_parse_flag ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::PARSEABLE );
	} else if ( global_rtf_flag ) {
	    _document->print( output, LQIO::DOM::Document::OutputFormat::RTF );
	} else {
	    _document->print( output );
	}
	output.close();
    }
}



/*
 * Set up file descriptors and call proper output routine.
 */

void
Model::print_intermediate()
{
    _document->setResultConvergenceValue(_confidence)
	.setResultValid(_confidence <= _parameters._precision)
	.setResultIterations(number_blocks);

    const string directoryName = createDirectory();
    const string suffix = _document->getResultInvocationNumber() > 0 ? SolverInterface::Solve::customSuffix : "";

    string extension;
    if ( global_parse_flag ) {
	extension = "p";
    } else if ( global_xml_flag ) {
	extension = "lqxo";
    } else if ( global_rtf_flag ) {
	extension = "rtf";
    } else {
	extension = "out";
    }

    LQIO::Filename filename( _input_file_name, extension.c_str(), directoryName.c_str(), suffix.c_str() );

    /* Make filename look like an emacs autosave file. */
    filename << "~" << number_blocks << "~";

    ofstream output;
    output.open( filename(), ios::out );

    if ( !output ) {
	return;			/* Ignore errors */
    } else if ( global_xml_flag ) {
	_document->print( output, LQIO::DOM::Document::OutputFormat::XML );
    } else if ( global_parse_flag ) {
	_document->print( output, LQIO::DOM::Document::OutputFormat::PARSEABLE );
    } else {
	_document->print( output );
    }
    output.close();
}


/*
 * Human format statistics.
 */

void
Model::print_raw_stats( FILE * output ) const
{
    const int long_width	= 99;
    const int short_width	= 69;

    static const char * dashes = "--------------------------------------------------------------------------------------------";

/*
  123456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789_
  ssssssssssssssssssssssssssssssssssssss tttttttt nnnnnnnnnnnn nnnnnnnnnnnn nnnnnnnnnnnn nnnnnnnn
  Name                                     Type       Mean        95% +/-      99% +/-   #Obs|Int
  ssssssssssssssssssssssssssssssssssssss tttttttt nnnnnnnnnnnn nnnnnnnn
  Name                                     Type       Mean     #Obs|Int
*/
    if ( number_blocks > 2 ) {
	(void) fprintf( output, "Blocked simulation statistics for %s\n\tTime = %G.  Period = %G\n\n", _input_file_name.c_str(), ps_now, _parameters._block_period );
	(void) fprintf( output, "Name                                     Type       Mean        95%% +/-      99%% +/-   #Obs/Int\n");
	(void) fprintf( output, "%.*s\n", long_width, dashes );
    } else {
	(void) fprintf( output, "Simulation statistics for %s\n\ttime = %G.\n\n", _input_file_name.c_str(), ps_now );
	(void) fprintf( output, "Name                                     Type       Mean     #Obs/Int\n");
	(void) fprintf( output, "%.*s\n", short_width, dashes );
    }

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
    	const Task * cp = *nextTask;
    	cp->print( output );
    }
//    for_each ( task.begin(), task.end(), ConstExec1<Task,FILE *>( &Task::print ) );

    (void) fprintf( output, "\n%.*s Processor Information %.*s\n",
		    (((number_blocks > 2) ? long_width : short_width) - 23) / 2, dashes,
		    (((number_blocks > 2) ? long_width : short_width) - 23) / 2, dashes );
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->r_util.print_raw( output, "Processor %-11.11s - Utilization", aProcessor->name() );
    }

#ifdef	NOTDEF
    if ( ps_used_links ) {
	(void) fprintf( output, "\n%.*s Link Information %*.s\n",
			(((number_blocks > 2) ? long_width : short_width) - 18) / 2, dashes,
			(((number_blocks > 2) ? long_width : short_width) - 18) / 2, dashes );
	for ( l = 0; l < ps_used_links; ++l ) {
	    link_utilization[l].print_raw_stat( output, "Link %-16.16s - Utilization", ps_link_tab[l].name );
	}
    }
#endif
    (void) fprintf( output, "\n\n");
}

/*
 * Go through data structures and insert results into the DOM
 */

void 
Model::insertDOMResults()
{
    /* Insert general information about simulation run */

    _document->setResultConvergenceValue(_confidence)
	.setResultValid( _confidence <= _parameters._precision || _parameters._precision == 0.0 )
	.setResultIterations(number_blocks);

#if HAVE_SYS_TIMES_H
    tms_t time_buf;

    clock_t stop_time = times( &time_buf );
    _document->setResultUserTime( time_buf.tms_utime - _start_times.tms_utime );
    _document->setResultSysTime( time_buf.tms_stime - _start_times.tms_stime  );
#else
    clock_t stop_time = time( NULL );
#endif
    _document->setResultElapsedTime(stop_time - _start_clock);

#if HAVE_SYS_RESOURCE_H && HAVE_GETRUSAGE
    struct rusage r_usage;
    if ( getrusage( RUSAGE_SELF, &r_usage ) == 0 && r_usage.ru_maxrss > 0 ) {
	_document->setResultMaxRSS( r_usage.ru_maxrss );
    }
#endif

    string buf;

#if defined(HAVE_UNAME)
    struct utsname uu;		/* Get system triva. */

    uname( &uu );
    buf  = uu.nodename;
    buf += " ";
    buf += uu.sysname;
    buf += " ";
    buf += uu.release;
    _document->setResultPlatformInformation(buf);
#endif
    buf = LQIO::io_vars.lq_toolname;
    buf += " ";
    buf += VERSION;
    _document->setResultSolverInformation(buf);


    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	(*nextTask)->insertDOMResults();
    }
    for ( set<Group *,ltGroup>::const_iterator nextGroup = group.begin(); nextGroup != group.end(); ++nextGroup ) {
	(*nextGroup)->insertDOMResults();
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	(*nextProcessor)->insertDOMResults();
    }
}



/*----------------------------------------------------------------------*/
/* 			  Utility routines.				*/
/*----------------------------------------------------------------------*/

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



bool
Model::hasVariables() const
{
    return _document->getSymbolExternalVariableCount() != 0;
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
    int simulation_flags= 0x00;
    
    /* This will compute the minimum service time for each entry.  Note thate XXX::initialize() is called
     * through Model::run() */
    
    for_each( ::task.begin(), ::task.end(), Exec<Task>( &Task::configure ) );
    const double client_cycle_time = for_each( ::entry.begin(), ::entry.end(), ExecSum<Entry,double>( &Entry::compute_minimum_service_time ) ).sum();
    
    /* Which we can use here... */
    
    _parameters.set( _document->getPragmaList(), client_cycle_time );

    if (debug_interactive_stepping) {
	simulation_flags = RPF_STEP; 	/* tomari quorum */
    }
    if (trace_driver) {
	simulation_flags = simulation_flags | RPF_TRACE|RPF_WARNING;
    } else {
	simulation_flags = simulation_flags | RPF_WARNING;
    }

#if defined(HAVE_SYS_TIMES_H)
    _start_clock = times( &_start_times );
#else
    _start_clock = time( 0 );
#endif

    deferred_exception = false;
    ps_run_parasol( _parameters._run_time+1.0, _parameters._seed, simulation_flags );	/* Calls ps_genesis */

    /*
     * Run completed.
     * Print final results
     */

    print();
    if ( _confidence > _parameters._precision && _parameters._precision > 0.0 ) {
	LQIO::solution_error( ADV_PRECISION, _parameters._precision, _parameters._block_period * number_blocks + _parameters._initial_delay, _confidence );
    }
    if ( messages_lost ) {
	for ( std::set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	    const Task * cp = *nextTask;
	    if ( cp->has_lost_messages() )  {
		LQIO::solution_error( LQIO::ADV_MESSAGES_DROPPED, cp->name() );
	    }
	}
    }
    
    if ( check_stacks ) {
#if HAVE_MCHECK
	mcheck_check_all();
#endif
	fprintf( stderr, "%s: ", _input_file_name.c_str() );
	test_all_stacks();
    }

    for ( unsigned i = 1; i <= total_tasks; ++i ) {
	Instance * ip = object_tab.at(i);
	if ( !ip ) continue;
	delete ip;
	object_tab[i] = 0;
    }

    if ( deferred_exception ) {
	throw_bad_parameter();
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

    LQIO::Filename directory_name( hasOutputFileName() ? _output_file_name.c_str() : _input_file_name.c_str(), "d" );		/* Get the base file name */

    if ( access( directory_name().c_str(), R_OK|W_OK|X_OK ) < 0 ) {
	solution_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name().c_str(), strerror( errno ) );
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    }

    unsigned int errorCode = 0;
    if ( !_document->loadResults( directory_name(), _input_file_name, 
				  SolverInterface::Solve::customSuffix, errorCode ) ) {
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

/* ------------------------------------------------------------------------ */

/*
 *  Run the simulation.  Called from ps_genesis.
 */

bool
Model::run( int task_id )
{
    bool rc = false;
    __genesis_task_id = task_id;

    if ( verbose_flag ) {
	(void) putc( 'C', stderr );		/* Constructing */
    }

#ifdef LQX_DEBUG
    printf( "In Model::run() ps_now: %g\n", ps_now );
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

	    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
		Task * aTask = *nextTask;
		if ( !aTask->start() ) {
		    abort();
		}
	    }

	    if ( _parameters._initial_delay ) {
		if ( verbose_flag ) {
		    (void) putc( 'I', stderr );
		}
		ps_sleep( _parameters._initial_delay );
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

		ps_sleep( _parameters._block_period );
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

		if ( !valid && print_interval > 0 && number_blocks % print_interval == 0 ) {
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
	rc = false;
	deferred_exception = true;
    }

    /* Force simulation to terminate. */

    ps_run_time = -1.1;
    ps_sleep(1.0);

    /* Remove instances */

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->kill();		/* Should move instance deletion here. */
    }

    return rc;
}

/*
 * Accumulate data from this run.
 */

void
Model::accumulate_data()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = ::task.begin(); nextTask != ::task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->accumulate_data();
    }
    for ( set<Group *,ltGroup>::const_iterator nextGroup = group.begin(); nextGroup != group.end(); ++nextGroup ) {
	Group * aGroup = *nextGroup;
	aGroup->r_util.accumulate();
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->r_util.accumulate();
    }

#ifdef	NOTDEF
    /* Link utilization. */

    for ( l = 0; l < ps_used_links; ++l ) {
	accumulate( ps_link_tab[l].sp, &link_utilization[l] );
    }
#endif
}



/*
 * Reset data from this run.
 */

void
Model::reset_stats()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = ::task.begin(); nextTask != ::task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->reset_stats();
    }
    for ( set<Group *,ltGroup>::const_iterator nextGroup = group.begin(); nextGroup != group.end(); ++nextGroup ) {
	Group * aGroup = *nextGroup;
	aGroup->r_util.reset();
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->r_util.reset();
    }

#ifdef	NOTDEF
    /* Link utilization. */

    for ( l = 0; l < ps_used_links; ++l ) {
	ps_link_tab[l].sp.reset();
    }
#endif
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
    
    for ( set<Task *,ltTask>::const_iterator nextTask = ::task.begin(); nextTask != ::task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	if ( aTask->type() == Task::OPEN_ARRIVAL_SOURCE ) continue;		/* Skip. */

	for ( vector<Entry *>::const_iterator nextEntry = aTask->_entry.begin(); nextEntry != aTask->_entry.end(); ++nextEntry ) {
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
Model::normalized_conf95( const result_t& stat )
{
    double temp = stat.mean();
    if ( temp ) {
	return sqrt(stat.variance()) * result_t::conf95( number_blocks ) * 100.0 / temp;
    }
    return -1.0;
}


/*
 * Progenator task.
 */

void
ps_genesis (void *)
{
    if (!Model::__model->run( ps_myself ) ) {
	LQIO::solution_error( ERR_INITIALIZATION_FAILED );
    }
    ps_suspend( ps_myself );
}

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
    if ( set( _run_time, pragmas, LQIO::DOM::Pragma::_run_time_ ) ) {
	_max_blocks = 1;

    } else {
	unsigned long initial_loops = 0;
	if ( set( _precision, pragmas, LQIO::DOM::Pragma::_precision_ ) ) {
	    /* -C */
	    _max_blocks = MAX_BLOCKS;
	    if ( !set( initial_loops, pragmas, LQIO::DOM::Pragma::_initial_loops_ ) ) {
		initial_loops = static_cast<unsigned long>(INITIAL_LOOPS / _precision);
	    }
	    _initial_delay = minimum_cycle_time * initial_loops * 2;
	    if ( !set( _run_time, pragmas, LQIO::DOM::Pragma::_run_time_ ) ) {
		_block_period = _initial_delay * 100;
	    } else {
		_block_period = (_run_time - _initial_delay) / _max_blocks;
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

	} else {
	    /* full auto */
	}

	_run_time = _initial_delay + _max_blocks * _block_period;
    }
}


bool Model::simulation_parameters::set( double& parameter, const std::map<std::string,std::string>& pragmas, const char * value )
{
    std::map<std::string,std::string>::const_iterator i = pragmas.find( value );
    if ( i != pragmas.end() ) {
	char * endptr = nullptr;
	parameter = std::strtod( i->second.c_str(), &endptr );
	if ( *endptr != '\0' ) {
	    std::stringstream err;
	    err << value << "=" << i->second;
	    throw std::invalid_argument(err.str());
	}
	return true;
    } else {
	return false;
    }
}


bool Model::simulation_parameters::set( unsigned long& parameter, const std::map<std::string,std::string>& pragmas, const char * value )
{
    std::map<std::string,std::string>::const_iterator i = pragmas.find( value );
    if ( i != pragmas.end() ) {
	char * endptr = nullptr;
	parameter = std::strtol( i->second.c_str(), &endptr, 10 );
	if ( *endptr != '\0' ) {
	    std::stringstream err;
	    err << value << "=" << i->second;
	    throw std::invalid_argument(err.str());
	}
	return true;
    } else {
	return false;
    }
}
