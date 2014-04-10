/* model.cc	-- Greg Franks Mon Feb  3 2003
 *
 * $Id$
 *
 * Load, slice, and dice the lqn model.
 */

#include "lqn2ps.h"
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <cmath>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_FLOAT_H
#include <float.h>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif
#if defined(X11_OUTPUT)
#include <sys/stat.h>
#endif
#include <lqio/dom_document.h>
#include <lqio/expat_document.h>
#include <lqio/srvn_output.h>
#include "stack.h"
#include "model.h"
#include "entry.h"
#include "activity.h"
#include "actlist.h"
#include "task.h"
#include "call.h"
#include "share.h"
#include "open.h"
#include "group.h"
#include "processor.h"
#include "layer.h"
#include "key.h"
#include "errmsg.h"
#include "graphic.h"
#include "label.h"
#include "runlqx.h"

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

Model * Model::__model = 0;

unsigned Model::iteration_limit   = 50;
unsigned Model::print_interval    = 10;
double Model::convergence_value   = 0.00001;
double Model::underrelaxation     = 0.9;

unsigned Model::openArrivalCount  = 0;
unsigned Model::forwardingCount	  = 0;
unsigned Model::rendezvousCount[MAX_PHASES+1];
unsigned Model::sendNoReplyCount[MAX_PHASES+1];
unsigned Model::phaseCount[MAX_PHASES+1];
int Model::maxModelNumber = 1;

/* input */
bool Model::deterministicPhasesPresent	= false;
bool Model::maxServiceTimePresent	= false;
bool Model::nonExponentialPhasesPresent	= false;
bool Model::thinkTimePresent		= false;
/* output */
bool Model::boundsPresent		= false;
bool Model::serviceExceededPresent	= false;
bool Model::variancePresent		= false;
bool Model::histogramPresent		= false;

Model::Stats Model::stats[Model::N_STATS];

static CommentManip print_comment( const char * aPrefix, const string& aStr );
static DoubleManip to_inches( const double );

Cltn<Group *> Model::group;

const char * Model::XMLSchema_instance = "http://www.w3.org/2001/XMLSchema-instance";

/*
 * Compare two entities by their submodel. 
 */

struct lt_submodel
{
    bool operator()(const Entity * e1, const Entity * e2) const
	{
	    return (e1->level() < e2->level()) 
		|| (e1->level() == e2->level() && (!dynamic_cast<const LQIO::DOM::Entity *>(e2->getDOM()) || (dynamic_cast<const LQIO::DOM::Entity *>(e1->getDOM()) && (dynamic_cast<const LQIO::DOM::Entity *>(e1->getDOM())->getId() < dynamic_cast<const LQIO::DOM::Entity *>(e2->getDOM())->getId()))));
	}
};

/* ------------------------ Constructors etc. ------------------------- */

Model::Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name )
    : myKey(0),
      _taskCount(0), 
      _processorCount(0),
      _entryCount(0),
      _document(document),
      _inputFileName( input_file_name ),
      _outputFileName( output_file_name ),
      _modelNumber(1),
      _scaling(1.0)
{
    __model = this;
    /* Check for more than one instance */

    if ( graphical_output() && Flags::print[KEY].value.i != 0 ) {
	myKey = new Key;
    }

    /* Reset counters */

    openArrivalCount		= 0;
    forwardingCount		= 0;
    for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	rendezvousCount[p]	= 0;
	sendNoReplyCount[p] 	= 0;
	phaseCount[p]		= 0;
    }

    stats[TOTAL_ENTRIES].name( "Entries" ).accumulate( &Model::nEntries );
    stats[TOTAL_LAYERS].name( "Layers" ).accumulate( &Model::nLayers );
    stats[TOTAL_PROCESSORS].name( "Processors" ).accumulate( &Model::nProcessors );
    stats[TOTAL_TASKS].name( "Tasks" ).accumulate( &Model::nTasks );
    stats[TOTAL_INFINITE_SERVERS].name( "Infinite Servers" ).accumulate( &Model::nInfiniteServers );
    stats[TOTAL_MULTISERVERS].name( "MultiServers" ).accumulate( &Model::nMultiServers );
    stats[ENTRIES_PER_TASK].name( "Entries per Task" );
    stats[OPEN_ARRIVALS_PER_ENTRY].name( "Open Arrivals per Entry" );
    stats[OPEN_ARRIVAL_RATE_PER_ENTRY].name( "Total Open Arrival Rate" );
    stats[PHASES_PER_ENTRY].name( "Number of Phases per Entry" );
    stats[RNVS_PER_ENTRY].name( "Rendezvous per Entry" );
    stats[RNV_RATE_PER_CALL].name( "RNV Rate per Call" );
    stats[SERVICE_TIME_PER_PHASE].name( "Service Time per Phase" );
    stats[SNRS_PER_ENTRY].name( "Send-no-replies per Entry" ); 
    stats[SNR_RATE_PER_CALL].name( "SNR Rate per Call" );
    stats[FORWARDING_PER_ENTRY].name( "Forwarding per Entry" );
    stats[FORWARDING_PROBABILITY_PER_CALL].name( "Forwarding Probabilty per Call" );
    stats[TASKS_PER_LAYER].name( "Tasks per Layer" );

    /* input */
    deterministicPhasesPresent	= false;
    maxServiceTimePresent	= false;
    nonExponentialPhasesPresent	= false;
    thinkTimePresent		= false;
    /* output */
    boundsPresent		= false;
    histogramPresent		= false;
    serviceExceededPresent	= false;
    variancePresent		= false;
}


/*
 * Delete the model.
 */

Model::~Model()
{
    opensource.deleteContents().chop( opensource.size() );
    group.deleteContents().chop( group.size() );

    layers.clear();
    __model = 0;
    if ( myKey ) {
	delete myKey;
    }
}

/*
 * Scale "model" by s.
 */

const Model&
Model::operator*=( const double s ) const
{ 
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].scaleBy( s, s );
    }
    if ( myKey ) {
	myKey->scaleBy( s, s );
    }
    Sequence<Group *> nextGroup( group );
    Group * aGroup;
    while( aGroup = nextGroup() )  {
	aGroup->scaleBy( s, s );
    }
    myOrigin  *= s;
    myExtent  *= s;
    const_cast<Model *>(this)->_scaling *= s;

    return *this; 
}


const Model&
Model::translateScale( const double s ) const
{
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].translateY( top() );
    }
    if ( myKey ) {
	myKey->translateY( top() );
    }
    Sequence<Group *> nextGroup( group );
    Group * aGroup;
    while( aGroup = nextGroup() )  {
	aGroup->translateY( top() );
    }
    *this *= s;

    return *this;
}


const Model&
Model::moveBy( const double dx, const double dy ) const
{
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].moveBy( dx, dy );
    }
    if ( myKey ) {
	myKey->moveBy( dx, dy );
    }
    Sequence<Group *> nextGroup( group );
    Group * aGroup;
    while( aGroup = nextGroup() )  {
	aGroup->moveBy( dx, dy );
    }
    myOrigin.moveBy( dx, dy );

    return *this;
}



/*
 * Delete stuff allocated externally.
 */
void
Model::free()
{
    for ( set<Share *,ltShare>::const_iterator nextShare = share.begin(); nextShare != share.end(); ++nextShare ) {
	Share * aShare = *nextShare;
	delete aShare;
    }
    share.clear();

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	delete aTask;
    }
    task.clear();

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	delete aProcessor;
    }
    processor.clear();

    for ( set<Entry *,ltEntry>::const_iterator nextEntry = entry.begin(); nextEntry != entry.end(); ++nextEntry ) {
	Entry * aEntry = *nextEntry;
	delete aEntry;
    }
    entry.clear();
}


#if HAVE_REGEX_T
/*
 * Add a group to the group list.
 */

void
Model::add_group( const string& s )
{
    group << new GroupByRegex( s );
}
#endif


/*
 * Create a group for each and every processor in the model so that
 * that GroupModel::justify() will collect everything up properly.
 */

void
Model::group_by_processor()
{
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	const Processor * aProcessor = *nextProcessor;
	group << new GroupByProcessor( aProcessor );
    }
}


/*
 * Create a group for each and every processor in the model so that
 * that GroupModel::justify() will collect everything up properly.
 */

void
Model::group_by_share()
{
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	if ( aProcessor->nShares() == 0 ) {
	    group << new GroupByProcessor( aProcessor );
	} else {
	    for ( set<Share *,ltShare>::const_iterator nextShare = aProcessor->shares().begin(); nextShare != aProcessor->shares().end(); ++nextShare ) {
		Share * aShare = *nextShare;
		group << new GroupByShareGroup( aProcessor, aShare );
	    }
	    group << new GroupByShareDefault( aProcessor );
	}
    }
}


/* 
 * Put boxes around submodels.
 */

void
Model::group_by_submodel()
{
    for ( unsigned i = 1; i < layers.size() ;++i ) {
	ostringstream s;
	s << "Submodel " << i;
	Group * aGroup = new GroupSquashed( s.str(), layers[i], layers[i+1] );
	aGroup->format( layers.size() ).label().resizeBox().positionLabel();
	group << aGroup;
    }
}

bool 
Model::prepare()
{
    /* We use this to add all calls */
    std::vector<LQIO::DOM::Entry*> allEntries;
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 0: Add Pragmas] */
    const map<string,string>& pragmaList = _document->getPragmaList();
    map<string,string>::const_iterator pragmaIter;
    for (pragmaIter = pragmaList.begin(); pragmaIter != pragmaList.end(); ++pragmaIter) {
	pragma( pragmaIter->first, pragmaIter->second );
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1: Add Processors] */
	
    /* We need to add all of the processors */
    const std::map<std::string,LQIO::DOM::Processor*>& processorList = _document->getProcessors();

    /* Add all of the processors we will be needing */
    for ( std::map<std::string,LQIO::DOM::Processor*>::const_iterator nextProcessor = processorList.begin(); nextProcessor != processorList.end(); ++nextProcessor ) {
	Processor::create(nextProcessor->second);
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 1.5: Add Groups] */
	
    const std::map<std::string,LQIO::DOM::Group*>& groups = _document->getGroups();
    std::map<std::string,LQIO::DOM::Group*>::const_iterator groupIter;
    for (groupIter = groups.begin(); groupIter != groups.end(); ++groupIter) {
	LQIO::DOM::Group* domGroup = const_cast<LQIO::DOM::Group*>(groupIter->second);
	Share::create(domGroup);
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 2: Add Tasks/Entries] */
	
    /* In the DOM, tasks have entries, but here entries need to go first */
    std::vector<Activity*> activityList;
    const std::map<std::string,LQIO::DOM::Task*>& taskList = _document->getTasks();
	
    /* Add all of the tasks we will be needing */
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator nextTask = taskList.begin(); nextTask != taskList.end(); ++nextTask ) {
	LQIO::DOM::Task* task = nextTask->second;

	/* Before we can add a task we have to add all of its entries */
		
	/* Prepare to iterate over all of the entries */
	Cltn<Entry*> entryCollection;
	std::vector<LQIO::DOM::Entry*>::const_iterator nextEntry;
	std::vector<LQIO::DOM::Entry*> activityEntries;
		
	/* Add the entries so we can reverse them */
	for ( nextEntry = task->getEntryList().begin(); nextEntry != task->getEntryList().end(); ++nextEntry ) {
	    allEntries.push_back( *nextEntry );		// Save the DOM entry.
	    entryCollection << Entry::create( *nextEntry );
	    if ((*nextEntry)->getStartActivity() != NULL) {
		activityEntries.push_back(*nextEntry);
	    }
	}

	/* Now we can go ahead and add the task */
	Task* aTask = Task::create(task, entryCollection);

	/* Add activities for the task (all of them) */
	const std::map<std::string,LQIO::DOM::Activity*>& activities = task->getActivities();
	std::map<std::string,LQIO::DOM::Activity*>::const_iterator iter;
	for (iter = activities.begin(); iter != activities.end(); ++iter) {
	    const LQIO::DOM::Activity* activity = iter->second;
	    activityList.push_back(Activity::create(aTask, const_cast<LQIO::DOM::Activity*>(activity)));
	}
		
	/* Set all the start activities */
	for (nextEntry = activityEntries.begin(); nextEntry != activityEntries.end(); ++nextEntry) {
	    LQIO::DOM::Entry* theDOMEntry = *nextEntry;
	    Entry * anEntry = Entry::find( theDOMEntry->getName() );
	    Activity * anActivity = aTask->findActivity(theDOMEntry->getStartActivity()->getName().c_str());
	    anEntry->setStartActivity( anActivity );
	}
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 3: Add Calls/Phase Parameters] */
	
    /* Add all of the calls for all phases to the system */
    const unsigned entryCount = allEntries.size();
    for (unsigned i = 0; i < entryCount; ++i) {
	LQIO::DOM::Entry* entry = allEntries[i];
	Entry* newEntry = Entry::find(entry->getName());
	assert(newEntry != NULL);

	/* Handle open arrivals */
	
	if ( entry->hasOpenArrivalRate() ) {
	    newEntry->isCalled( OPEN_ARRIVAL_REQUEST );
	}

	/* Go over all of the entry's phases and add the calls */
	for (unsigned p = 1; p <= entry->getMaximumPhase(); ++p) {
	    LQIO::DOM::Phase* phase = entry->getPhase(p);
	    const std::vector<LQIO::DOM::Call*>& originatingCalls = phase->getCalls();
	    std::vector<LQIO::DOM::Call*>::const_iterator iter;
			
	    /* Add all of the calls to the system */
	    for (iter = originatingCalls.begin(); iter != originatingCalls.end(); ++iter) {
		LQIO::DOM::Call* call = *iter;
				
		/* Add the call to the system */
		newEntry->addCall( p, call );
	    }
			
	    /* Set the phase information for the entry */
	    string phase_name = entry->getName();
	    phase_name += "_";
	    phase_name += p + '0';
	    phase->setName( phase_name );
	}
		
	/* Add in all of the P(frwd) calls */
	const std::vector<LQIO::DOM::Call*>& forwarding = entry->getForwarding();
	std::vector<LQIO::DOM::Call*>::const_iterator nextFwd;
	for ( nextFwd = forwarding.begin(); nextFwd != forwarding.end(); ++nextFwd ) {
	    Entry* targetEntry = Entry::find((*nextFwd)->getDestinationEntry()->getName());
	    newEntry->forward(targetEntry, *nextFwd );
	}
    }
	
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Step 4: Add Calls/Lists for Activities] */
	
    /* Go back and add all of the lists and calls now that activities all exist */
    std::vector<Activity*>::iterator actIter;
    for (actIter = activityList.begin(); actIter != activityList.end(); ++actIter) {
	Activity* activity = *actIter;
	activity->add_calls()
	    .add_reply_list()
	    .add_activity_lists();
    }
	
    /* Use the generated connections list to finish up */
    Activity::complete_activity_connections();
	
    /* Tell the user that we have finished */
    return true;
}



/* 
 * layerize and format the input file.
 */

bool
Model::process()
{
#if defined(REP2FLAT)
    switch ( Flags::print[REPLICATION].value.i ) {
    case REPLICATION_EXPAND: expandModel(); break;
    case REPLICATION_REMOVE: removeReplication(); break;
    }
#endif

    if ( !generate() ) return false;
    layerize();

    /* Assign numbers to layers. */

    unsigned j = 0;
    for ( unsigned i = 1; i <= layers.size() ;++i ) {
	if ( !layers[i] ) continue;
	j += 1;
	layers[i].number(j);
    }

    if ( Flags::debug_submodels ) {
	for ( unsigned i = 2; i <= layers.size(); ++i ) {
	    if ( !layers[i] ) continue;
	    layers[i].generateSubmodel().printSubmodel( cerr );		/* We normally don't need to generate a submodel, so do it here */
	}
    }

    if ( !check() ) return false;

    if ( !Flags::have_results && Flags::print[COLOUR].value.i == COLOUR_RESULTS ) {
	Flags::print[COLOUR].value.i = COLOUR_OFF;
    }
    
    if ( share_output() ) {
	group_by_share();
    } else if ( processor_output() ) {
	group_by_processor();
#if HAVE_REGEX_T
    } else if ( group.size() ) {
	Model::add_group( ".*" );		    /* Tack on default rule */
#endif
    }

    sort( Entity::compare );
    if ( Flags::rename_model ) {
	rename();
    } else if ( Flags::squish_names ) {
	squishNames();
    }

    Processor * surrogate_processor = 0;
    Task * surrogate_task = 0;

    unsigned int layer_number = 0;
    unsigned submodel  = Flags::print[QUEUEING_MODEL].value.i | Flags::print[SUBMODEL].value.i;
    if ( submodel > 0 ) {
	layer_number = submodel + 1;

 	if ( !selectSubmodel( submodel ) ) {
	    cerr << io_vars.lq_toolname << ": Submodel " << submodel << " is too big." << endl;
	    return false;
	} else if ( Flags::print[LAYERING].value.i == LAYERING_CLIENT ) {
 	    layers[layer_number].generateClientSubmodel();
	} else if ( Flags::print[LAYERING].value.i != LAYERING_SRVN ) {
 	    layers[layer_number].generateSubmodel();
	    if ( Flags::surrogates ) {
		layers[layer_number].transmorgrify( _document, surrogate_processor, surrogate_task );
		relayerize( layer_number );
		layers[layer_number+1].sort( Entity::compare ).format( 0 ).justify( io_vars.n_entries * Flags::entry_width );
	    }
	}
#if HAVE_REGEX_T
    } else if ( Flags::print[INCLUDE_ONLY].value.r && Flags::surrogates ) {

	/* Call transmorgrify on all layers */

	for ( unsigned i = layers.size() - 1; i >= 2; --i ) {	
	    layers[i].generateSubmodel();
	    layers[i].transmorgrify( _document, surrogate_processor, surrogate_task );
	    relayerize( i );
	    layers[i+1].sort( Entity::compare ).format( 0 ).justify( io_vars.n_entries * Flags::entry_width );
	}
#endif
    }

    if ( !totalize() ) {
	LQIO::solution_error( ERR_NO_OBJECTS );
	return false;
    }

    if ( graphical_output() ) {
	format();

	if ( group.size() == 0 && Flags::print_submodels ) {
	    group_by_submodel();
	}

	/* Compensate for the arcs on the right */

	if ( (queueing_output() || submodel_output()) && Flags::flatten_submodel  ) {
	    format( layers[layer_number] );
	}

	if ( queueing_output() ) {
	
	    /* Compensate for chains */

	    const double delta = Flags::icon_height / 5.0 * layers[layer_number+1].nChains();
	    myExtent.moveBy( delta, delta );
	    myOrigin.moveBy( 0, -delta / 2.0 );
	}

	/* Compensate for labels */

	if ( Flags::print_layer_number ) {
	    myExtent.moveBy( Flags::print[X_SPACING].value.f + layers[layers.size()].labelWidth() + 5, 0 );
	} 

	/* Compensate for processor/group boxes. */

	Sequence<Group *> nextGroup( group );
	Group * aGroup;

	while ( aGroup = nextGroup() ) {
	    double h = aGroup->y() - myOrigin.y();
	    if ( h < 0 ) {
		myOrigin.moveBy( 0, h );
		myExtent.moveBy( 0, -h );
	    }
	}

	/* Move the key iff necessary */

    	if ( myKey ) {
	    myKey->label().moveTo( myOrigin.x(), myOrigin.y() ); 

	    switch ( Flags::print[KEY].value.i ) {
	    case KEY_TOP_LEFT:	    myKey->moveBy( 0, myExtent.y() - myKey->height() ); break;
	    case KEY_TOP_RIGHT:	    myKey->moveBy( myExtent.x() - myKey->width(), myExtent.y() - myKey->height() ); break;
	    case KEY_BOTTOM_LEFT:   break;
	    case KEY_BOTTOM_RIGHT:  myKey->moveBy( myExtent.x() - myKey->width(), 0 ); break;
	    case KEY_BELOW_LEFT:
		moveBy( 0, myKey->height() );
		myOrigin.moveBy( 0, -myKey->height() );
		myKey->moveBy( 0, -myKey->height() );
		myExtent.moveBy( 0, myKey->height() );
		break;
	    case KEY_ABOVE_LEFT:    
		myKey->moveBy( 0, myExtent.y() ); 
		myExtent.moveBy( 0, myKey->height() );
		break;
	    }
	}

	finalScaleTranslate();
    }

    return true;
}


/*
 * Output the results.
 */

bool
Model::store()
{
#if defined(X11_OUTPUT)
    if ( Flags::print[OUTPUT_FORMAT].value.i == FORMAT_X11 ) {
	/* run the event loop */
	return true;
    } 
#endif
    if ( Flags::print[OUTPUT_FORMAT].value.i == FORMAT_NULL ) {

//	accumulateStatistics( _inputFileName );

    } else if ( output_output() && !Flags::have_results ) {

	cerr << io_vars.lq_toolname << ": There are no results to output for " << _inputFileName << endl;
	return false;

    } else if ( _outputFileName == "-" ) {

	cout.exceptions( ios::failbit | ios::badbit );
	cout << *this;

    } else {

	/* Default mapping */

	string directory_name;
	const char * suffix = "";

	if ( hasOutputFileName() && LQIO::Filename::isDirectory( _outputFileName.c_str() ) > 0 ) {
	    directory_name = _outputFileName;
	}

	if ( _document->getResultInvocationNumber() > 0 ) {
	    suffix = SolverInterface::Solve::customSuffix.c_str();
	    if ( directory_name.size() == 0 ) {
		/* We need to create a directory to store output. */
		LQIO::Filename filename( hasOutputFileName() ? _outputFileName.c_str() : _inputFileName.c_str(), "d" );		/* Get the base file name */
		directory_name = filename();
		int rc = access( directory_name.c_str(), R_OK|W_OK|X_OK );
		if ( rc < 0 ) {
#if defined(WINNT)
		    rc = mkdir( directory_name.c_str() );
#else
		    rc = mkdir( directory_name.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH );
#endif
		    if ( rc < 0 ) {
			solution_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name.c_str(), strerror( errno ) );
		    }
		}
	    }
	} else {
	    suffix = "";
	}

	LQIO::Filename filename;
	string extension = getExtension();
	if ( !hasOutputFileName() || directory_name.size() > 0 ) {
	    filename.generate( _inputFileName.c_str(), extension.c_str(), directory_name.c_str(), suffix );
	} else {
	    filename = _outputFileName.c_str();
	}
		
	if ( _inputFileName == filename() && input_output() ) {
#if defined(REP2FLAT)
	    if ( Flags::print[REPLICATION].value.i == REPLICATION_EXPAND ) {
		string ext = ".";		// look for .ext
		ext += extension;
		const size_t pos = filename.rfind( ext );
		if ( pos != string::npos ) {
		    filename.insert( pos, "-flat" );	// change filename.ext to filename-flat.ext
		} else {
		    ostringstream msg; 
		    msg << "Cannot overwrite input file " << filename() << " with a subset of original model.";
		    throw runtime_error( msg.str() );
		}
	    } else if ( partial_output() 
			|| Flags::print[AGGREGATION].value.i != AGGREGATE_NONE
			|| Flags::print[REPLICATION].value.i != REPLICATION_NOP ) {
		ostringstream msg; 
		msg << "Cannot overwrite input file " << filename() << " with a subset of original model.";
		throw runtime_error( msg.str() );
	    }
#else
	    if ( partial_output() 
		 || Flags::print[AGGREGATION].value.i != AGGREGATE_NONE ) {
		ostringstream msg; 
		msg << "Cannot overwrite input file " << filename() << " with a subset of original model.";
		throw runtime_error( msg.str() );
	    }
#endif
	}

	filename.backup();

	ofstream output;
	switch( Flags::print[OUTPUT_FORMAT].value.i ) {
#if defined(EMF_OUTPUT)
	case FORMAT_EMF:
	    if ( LQIO::Filename::isRegularFile( filename() ) == 0 ) {
		ostringstream msg; 
		msg << "Cannot open output file " << filename() << " - not a regular file.";
		throw runtime_error( msg.str() );
	    } else {
		output.open( filename(), ios::out|ios::binary );	/* NO \r's in output for windoze */
	    }
	    break;
#endif
#if HAVE_GD_H && HAVE_LIBGD
#if HAVE_GDIMAGEGIFPTR
	case FORMAT_GIF:
#endif
#if HAVE_LIBJPEG
	case FORMAT_JPEG:
#endif
#if HAVE_LIBPNG
	case FORMAT_PNG:
#endif
	    output.open( filename(), ios::out|ios::binary );	/* NO \r's in output for windoze */
	    break;
#endif /* HAVE_LIBGD */

	default:
	    output.open( filename(), ios::out );
	    break;
	}

	if ( !output ) {
	    ostringstream msg; 
	    msg << "Cannot open output file " << filename() << " - " << strerror( errno );
	    throw runtime_error( msg.str() );
	} else {
	    output.exceptions ( ios::failbit | ios::badbit );
	    output << *this;
	}
	output.close();
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

    LQIO::Filename directory_name( hasOutputFileName() ? _outputFileName.c_str() : _inputFileName.c_str(), "d" );		/* Get the base file name */

    if ( access( directory_name(), R_OK|X_OK ) < 0 ) {
	solution_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name(), strerror( errno ) );
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    }

    unsigned int errorCode;
    if ( !_document->loadResults( directory_name(), _inputFileName, 
				  SolverInterface::Solve::customSuffix, errorCode ) ) {
	throw LQX::RuntimeException( "--reload-lqx can't load results." );
    } else {
	label();
	store();
	return true;
    }
}



string
Model::getExtension()
{
    string extension;
    char * p;

    switch ( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
	extension = "tex";
	break;
#if defined(EMF_OUTPUT)
    case FORMAT_EMF:
	extension = "emf";
	break;
#endif
    case FORMAT_PSTEX:
    case FORMAT_FIG:
	extension = "fig";
	break;
#if HAVE_GD_H && HAVE_LIBGD
#if HAVE_GDIMAGEGIFPTR
    case FORMAT_GIF:
	extension = "gif";
	break;
#endif
#if HAVE_LIBJPEG
    case FORMAT_JPEG:
	extension = "jpg";
	break;
#endif
#if HAVE_LIBPNG
    case FORMAT_PNG:
	extension = "png";
	break;
#endif
#endif	/* HAVE_LIBGD */
    case FORMAT_OUTPUT:
	extension = "out";
	break;
    case FORMAT_PARSEABLE:
	extension = "p";
	break;
    case FORMAT_POSTSCRIPT:
	extension = "ps";
	break;
    case FORMAT_RTF:
	extension = "rtf";
	break;
    case FORMAT_SRVN:
#ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
	p = strrchr( const_cast<char *>(_inputFileName.c_str()), '.' );
#else
	p = strrchr( _inputFileName.c_str(), '.' );
#endif
	if ( p && *(p+1) != '\0' && strcmp( p+1, "lqnx") != 0 && strcmp( p+1, "lqxo" ) != 0 && strcmp( p+1, "xml" ) != 0 ) {
	    extension = p+1;
	} else {
	    extension = "lqn";
	}
	break;
#if defined(SVG_OUTPUT)
    case FORMAT_SVG:
	extension = "svg";
	break;
#endif
#if defined(SXD_OUTPUT)
    case FORMAT_SXD:
	abort();
	break;
#endif
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
	extension = "qnap";
	break;
#endif
#if defined(TXT_OUTPUT) 
    case FORMAT_TXT:
	extension = "txt";
	break;
#endif
    case FORMAT_XML:
	if ( queueing_output() ) {
	    extension = "pmif";
	} else {
	    extension = "lqnx";
	}
	break;

    default: 
	abort();
	break;
    }
    return extension;
}


/*
 * Sort tasks into layers.
 */

bool
Model::generate()
{
    _numberOfLayers = topologicalSort();

    /* At this point we need to do some intelligent thing about sticking tasks/processors into submodels. */

    layers.grow( _numberOfLayers + 1 );		// need an extra layer if submodel output.

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( aTask->level() == 0 || !aTask->isReachable() ) {
	    LQIO::solution_error( LQIO::WRN_NOT_USED, "Task", aTask->name().c_str() );
	} else if ( aTask->hasActivities() ) { 
	    aTask->generate();
	}	
    }

    return !io_vars.anError;
}



/*
 * Sort tasks into layers.  Start from reference tasks only and tasks
 * with open arrivals.  If a task has open arrivals, start from level
 * 1 so that it is treated as a server.
 */

unsigned
Model::topologicalSort()
{
    CallStack callStack( io_vars.n_tasks + 2 );
    unsigned max_depth = 0;

    set<Task *,ltTask>::const_iterator nextTask = task.begin();
    for ( unsigned i = 1; nextTask != task.end(); ++nextTask, ++i ) {
	Task * aTask = *nextTask;
	const int initialLevel = aTask->rootLevel();
	if ( initialLevel > 0 
#if HAVE_REGEX_T
	     && (Flags::client_tasks == 0 || regexec( Flags::client_tasks, const_cast<char *>(aTask->name().c_str()), 0, 0, 0 ) != REG_NOMATCH ) 
#endif
	     ) {

	    callStack.grow( initialLevel );
	    try { 
		max_depth = max( aTask->findChildren( callStack, i ), max_depth );
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

    return max_depth;
}



/*
 * Select a submodel.
 */

bool
Model::selectSubmodel( const unsigned submodel ) 
{
    if ( submodel == 0 || submodel >= _numberOfLayers) {
	return false;
    } else {
	layers[submodel+1].selectSubmodel();
	return true;
    }
}


Model&
Model::relayerize( const unsigned level )
{
    Cltn<Entity *>& entities = const_cast<Cltn<Entity *>& >(layers[level].entities());
    Entity * anEntity; Sequence<Entity *> nextEntity( entities );
    while ( anEntity = nextEntity() ) {
	if ( anEntity->level() > level ) {
	    layers[level] -= anEntity;
	    layers[level+1] += anEntity;
	}
    }
    return *this;
}

/* 
 *
 */

bool
Model::check()
{
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	if ( !layers[i].size() ) continue;

	layers[i].check();
    }
    return !io_vars.anError;
}



/* 
 * Now count them up. 
 */

unsigned
Model::totalize()
{
    /* Assign numbers to layers. */

    unsigned j = 0;
    for ( unsigned i = 1; i <= layers.size() ;++i ) {
	if ( !layers[i] ) continue;
	j += 1;
	layers[i].number(j);
    }

    const LayerSequence nextEntity( layers );
    const Entity * anEntity;
    _taskCount      = 0;
    _processorCount = 0;
    _entryCount     = 0;

    while ( anEntity = nextEntity() ) {
        const Task * aTask = dynamic_cast<const Task *>(anEntity);
        if ( aTask ) {
            _taskCount  += 1;
            _entryCount += aTask->nEntries();
        } else if ( dynamic_cast<const Processor *>(anEntity) ) {
            _processorCount += 1;
        }
    }
    return _taskCount + _processorCount;
}


/* 
 * Order tasks intelligently (!) in each layer. Start at the top. 
 */

Model const&
Model::sort( compare_func_ptr compare ) const
{
    const double width = io_vars.n_entries * Flags::entry_width;		/* A guess... */
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].sort( compare ).format( 0 ).justify( width );
    }	
    return *this;
}



/*
 * Rename all objects.  Objects have been sorted, so everything will look real perty.
 */

Model&
Model::rename()
{
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].rename();
    }

    return *this;
}



/*
 * Rename all objects.  Objects have been sorted, so everthing will look real perty.
 */

Model&
Model::squishNames()
{
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->squishName();
    }
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->squishName();
    }
    return *this;
}



/*
 * Format for printing.
 */

Model const&
Model::format() const
{
    myOrigin.moveTo( MAXDOUBLE, MAXDOUBLE );
    myExtent.moveTo( 0, 0 );

    double start_y = 0.0;

    for ( unsigned i = layers.size(); i > 0; --i ) {
	if ( layers[i].size() == 0 ) continue;
	layers[i].format( start_y ).label().depth( (layers.size() - (i-1)) * 10 ).sort( Entity::compareCoord );
	myOrigin.min( layers[i].x(), layers[i].y() );
	myExtent.max( layers[i].x() + layers[i].width(), layers[i].y() + layers[i].height() );

	start_y += (layers[i].height() + Flags::print[Y_SPACING].value.f);
    }

    justify();
    align();

    switch ( Flags::node_justification ) {
    case DEFAULT_JUSTIFY:
    case ALIGN_JUSTIFY:
	if ( Flags::print[LAYERING].value.i == LAYERING_BATCH 
	     || Flags::print[LAYERING].value.i == LAYERING_HWSW 
	     || Flags::print[LAYERING].value.i == LAYERING_MOL ) {
	    alignEntities();
	}
	break;
    }

    for ( unsigned i = layers.size(); i > 0; --i ) {
	if ( layers[i].size() == 0 ) continue;
	layers[i].moveLabelTo( right() + Flags::print[X_SPACING].value.f, layers[i].height() / 2.0 );

    }

    return *this;
}


const Model&
Model::format( Layer& serverLayer ) const
{
    Sequence<const Entity *> nextClient(serverLayer.clients());
    const Entity * aClient;

    Layer clientLayer;
    while ( aClient = nextClient() ) {
	clientLayer << const_cast<Entity *>(aClient);
    }

    double start_y = serverLayer.y();
    myOrigin.moveTo( MAXDOUBLE, MAXDOUBLE );
    myExtent.moveTo( 0, 0 );

    serverLayer.format( start_y ).sort( Entity::compareCoord );
    myOrigin.min( serverLayer.x(), serverLayer.y() );
    myExtent.max( serverLayer.x() + serverLayer.width(), serverLayer.y() + serverLayer.height() );
    start_y += ( serverLayer.x() + serverLayer.height() + Flags::print[Y_SPACING].value.f);

    clientLayer.format( start_y ).sort( Entity::compareCoord );
    myOrigin.min( clientLayer.x(), clientLayer.y() );
    myExtent.max( clientLayer.x() + clientLayer.width(), clientLayer.y() + clientLayer.height() );

    clientLayer.justify( right() );
    serverLayer.justify( right() );

    return *this;
}


/*
 * Justify the layers.
 */

Model const&
Model::justify() const
{
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].justify( right() );
    }	
    return *this;
}



/*
 * Align the layers horizontally.
 */

const Model&
Model::align() const
{
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].align( layers[i].height() );
    }	
    return *this;
}



/*
 * Align tasks between layers.
 */

const Model&
Model::alignEntities() const
{
    if ( layers.size() < 2 ) return *this;	/* No point */
    if ( layers.size() && Flags::print[LAYERING].value.i == LAYERING_BATCH ) {
	layers[1].fill( right() );
    }

    double maxRight = layers[1].x() + layers[1].width();
    for ( unsigned i = 2; i <= layers.size(); ++i ) {
	if ( !layers[i].size() ) continue;
	layers[i].alignEntities();
	maxRight = max( maxRight, layers[i].x() + layers[i].width() );
    }
    
    myOrigin.x( 0 );
    myExtent.x( maxRight );
    return *this;
}



/*
 * Our origin is lower left corner (Like TeX).  Most everything else
 * is upper right.  Scale to final image size so that the output is
 * roughly the same, regarless of format.
 */

Model const&
Model::finalScaleTranslate() const
{
    /*
     * Shift everything to origin.  Makes life easier for EMF and
     * other formats with clipping rectangles. Add a border.  
     */

    const double offset = Flags::print[BORDER].value.f;
    const double x_offset = offset - left();
    const double y_offset = offset - bottom();		/* Shift to origin */
    myOrigin.moveTo( 0, 0 );
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].moveBy( x_offset, y_offset );
    }
    if ( myKey ) {
	myKey->moveBy( x_offset, y_offset );
    }
    Sequence<Group *> nextGroup( group );
    Group * aGroup;
    while( aGroup = nextGroup() )  {
	aGroup->moveBy( x_offset, y_offset );
    }
    myExtent.moveBy( 2.0 * offset, 2.0 * offset );

    /* Rescale for output format. */

    switch ( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
	*this *= EEPIC_SCALING;
	break;

#if defined(EMF_OUTPUT)
    case FORMAT_EMF:
	translateScale( EMF_SCALING );
	break;
#endif

    case FORMAT_FIG:
    case FORMAT_PSTEX:
	translateScale( FIG_SCALING );
	break;

#if HAVE_GD_H && HAVE_LIBGD
#if HAVE_GDIMAGEGIFPTR
    case FORMAT_GIF:
#endif
#if HAVE_LIBJPEG
    case FORMAT_JPEG:
#endif
#if HAVE_LIBPNG
    case FORMAT_PNG:
#endif
	translateScale( GD_SCALING );
	break;
#endif	/* HAVE_LIBGD */

#if defined(SVG_OUTPUT)
    case FORMAT_SVG:
	/* TeX's origin is lower left corner.  SVG's is upper right.  Fix and scale */
	translateScale( SVG_SCALING );
	break;
#endif

#if defined(SXD_OUTPUT)
    case FORMAT_SXD:
	/* TeX's origin is lower left corner.  SXD's is upper right.  Fix and scale */
	translateScale( SXD_SCALING );
	break;
#endif

    }

    /* Final scaling */

    if ( Flags::print[MAGNIFICATION].value.f != 1.0 ) {
	*this *= Flags::print[MAGNIFICATION].value.f;
    }

    return *this;
}



Model const& 
Model::label() const
{
    Flags::have_results = Flags::print[RESULTS].value.b && ( _document->getResultIterations() > 0 || _document->getResultConvergenceValue() > 0 );
     for ( unsigned i = layers.size(); i > 0; --i ) {
 	if ( layers[i].size() == 0 ) continue;
	layers[i].label();
     }
    return *this;
}


unsigned
Model::count( const boolTaskFunc aFunc ) const
{
    const LayerSequence nextEntity( layers );
    const Entity * anEntity;
    unsigned count = 0;
    while ( anEntity = nextEntity() ) {
        const Task * aTask = dynamic_cast<const Task *>(anEntity);
        if ( aTask && aTask->isServerTask() && (aTask->*aFunc)() ) {
	    count += 1;
	}
    }
    return count;
}



unsigned
Model::count( const callFunc aFunc ) const
{
    const LayerSequence nextEntity( layers );
    const Entity * anEntity;
    unsigned count = 0;
    while ( anEntity = nextEntity() ) {
        const Task * aTask = dynamic_cast<const Task *>(anEntity);
        if ( aTask && aTask->isServerTask() ) {
	    Sequence<Entry *> nextEntry( aTask->entries() );
	    const Entry * anEntry;
	    while ( anEntry = nextEntry() ) {
		count += anEntry->countCallers( aFunc );
	    }
	}
    }
    return count;
}



unsigned 
Model::nMultiServers() const
{
    return count( &Task::isMultiServer );
}


unsigned 
Model::nInfiniteServers() const
{
    return count( &Task::isInfinite );
}


Model& 
Model::accumulateStatistics( const string& filename ) 
{
    stats[TOTAL_LAYERS].accumulate( this, filename );
    stats[TOTAL_TASKS].accumulate( this, filename );
    stats[TOTAL_PROCESSORS].accumulate( this, filename );
    stats[TOTAL_ENTRIES].accumulate( this, filename );
    stats[TOTAL_INFINITE_SERVERS].accumulate( this, filename );
    stats[TOTAL_MULTISERVERS].accumulate( this, filename );

    accumulateEntryStats( filename );
    accumulateTaskStats( filename );
    return *this;
}



const Model&
Model::accumulateTaskStats( const string& filename ) const
{
    /* Does not count ref. tasks. */

    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	unsigned nTasks = 0;
	Sequence<Entity *> nextEntity( layers[i].entities() );
	const Entity * anEntity;

	while ( anEntity = nextEntity() ) {

	    /* Does not count ref. tasks. */

	    if ( anEntity->isServerTask() ) {
		nTasks += 1;
	    }
	}
	if ( nTasks ) {	
	    stats[TASKS_PER_LAYER].accumulate( nTasks, filename );
	}
    }
    return *this;
}




const Model&
Model::accumulateEntryStats( const string& filename ) const
{
    const LayerSequence nextEntity( layers );
    const Entity * anEntity;

    while ( anEntity = nextEntity() ) {
        const Task * aTask = dynamic_cast<const Task *>(anEntity);

	/* Does not count ref. tasks. */

        if ( aTask && aTask->isServerTask() ) {
	    stats[ENTRIES_PER_TASK].accumulate( aTask->nEntries(), filename );
	    Sequence<Entry *> nextEntry( aTask->entries() );
	    const Entry * anEntry;
	    while ( anEntry = nextEntry() ) {
		if ( anEntry->hasOpenArrivalRate() ) {
		    stats[OPEN_ARRIVALS_PER_ENTRY].accumulate( 1.0, filename );
		    stats[OPEN_ARRIVAL_RATE_PER_ENTRY].accumulate( LQIO::DOM::to_double(anEntry->openArrivalRate()), filename );
		} else {
		    stats[OPEN_ARRIVALS_PER_ENTRY].accumulate( 0.0, filename );
		}
		if ( anEntry->hasForwarding() ) {
		    stats[FORWARDING_PER_ENTRY].accumulate( 1.0, filename );
		} else {
		    stats[FORWARDING_PER_ENTRY].accumulate( 0.0, filename );
		}
		stats[RNVS_PER_ENTRY].accumulate( anEntry->countCallers( &GenericCall::hasRendezvous ), filename );
		stats[SNRS_PER_ENTRY].accumulate( anEntry->countCallers( &GenericCall::hasSendNoReply ), filename );
		stats[PHASES_PER_ENTRY].accumulate( anEntry->maxPhase(), filename );
		for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
		    if ( anEntry->hasServiceTime(p) ) {
			stats[SERVICE_TIME_PER_PHASE].accumulate( LQIO::DOM::to_double(anEntry->serviceTime( p )), filename );
		    }
		}
		 
		/* get rendezvous rates and what have you */

		Sequence<Call *> nextCall( anEntry->callList() );
		const Call * aCall;
		while ( aCall = nextCall() ) {
		    for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
			if ( aCall->hasRendezvousForPhase(p) ) {
			    stats[RNV_RATE_PER_CALL].accumulate( LQIO::DOM::to_double(aCall->rendezvous(p)), filename );
			} 
			if ( aCall->hasSendNoReplyForPhase(p) ) {
			    stats[RNV_RATE_PER_CALL].accumulate( LQIO::DOM::to_double(aCall->sendNoReply(p)), filename );
			}
		    }
		    if ( aCall->hasForwarding() ) {
			stats[FORWARDING_PROBABILITY_PER_CALL].accumulate( LQIO::DOM::to_double(aCall->forward()), filename );
		    }
		}
	    }

	    Sequence<Activity *> nextActivity( aTask->activities() );
	    const Activity * anActivity;

	    while ( anActivity = nextActivity() ) {
		if ( anActivity->hasServiceTime() ) {
		    stats[SERVICE_TIME_PER_PHASE].accumulate( LQIO::DOM::to_double(anActivity->serviceTime()), filename );
		}
	    }
	}
    }
    return *this;
}



/*
 * Print out the model.
 */

ostream& 
Model::print( ostream& output ) const
{
    if ( Flags::print[PRECISION].value.i >= 0 ) {
	output.precision(Flags::print[PRECISION].value.i);
    }

    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
	printEEPIC( output );
	break;
#if defined(EMF_OUTPUT)
    case FORMAT_EMF:
	printEMF( output );
	break;
#endif
    case FORMAT_FIG:
    case FORMAT_PSTEX:
	printFIG( output );
	break;
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    case FORMAT_GIF:
	printGD( output, &GD::outputGIF );
	break;
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
    case FORMAT_JPEG:
	printGD( output, &GD::outputJPG );
	break;
#endif
    case FORMAT_NULL:
	break;
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    case FORMAT_PNG:
	printGD( output, &GD::outputPNG );
	break;
#endif
    case FORMAT_OUTPUT:
	printOutput( output );
	break;
    case FORMAT_PARSEABLE:
	printParseable( output );
	break;
    case FORMAT_POSTSCRIPT:
	printPostScript( output );
	break;
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
	printLayers( output );
	break;
#endif
    case FORMAT_RTF:
	printRTF( output );
	break;
#if defined(SVG_OUTPUT)
    case FORMAT_SVG:
	printSVG( output );
	break;
#endif
#if defined(SXD_OUTPUT)
    case FORMAT_SXD:
	printSXD( output );
	break;
#endif
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
	printTXT( output );
	break;
#endif
    case FORMAT_SRVN:
	printInput( output  );
	break;
#if defined(PMIF_OUTPUT)
    case FORMAT_XML:
	if ( queueing_output() ) {
	    printPMIF( output );
	} else {
	    printXML( output );
	}
	break;
#endif
#if defined(X11_OUTPUT)
    case FORMAT_X11:
	printX11( output );
	break;
#endif
    }

    return output;
}

/* ------------------------------------------------------------------------ */
/*                      Major model transmorgrification                     */
/* ------------------------------------------------------------------------ */
/* static */ void
Model::aggregate( LQIO::DOM::Document& document ) 
{
    const std::map<std::string,LQIO::DOM::Task *>& tasks = document.getTasks();
    for_each( tasks.begin(), tasks.end(), Model::Aggregate() );
}

#if 0
/*
 * Aggregate activities to activities and/or activities to phases.  If
 * activities are left after aggregation, we will have to recompute
 * their level because there likely is a lot less of them to draw.
 */

Task&
Task::aggregate()
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->aggregate();
    }

    switch ( Flags::print[AGGREGATION].value.i ) {
    case AGGREGATE_ENTRIES:
	activityList.deleteContents().chop(activityList.size());
	aggregateEntries();
	break;

    case AGGREGATE_ACTIVITIES:
    case AGGREGATE_PHASES:
	activityList.deleteContents().chop(activityList.size());
	break;

    default:
	/* Recompute levels. */
	Sequence<Activity *> nextActivity( activityList );
	Activity * anActivity;
	while ( anActivity = nextActivity() ) {
	    anActivity->resetLevel();
	}
	generate();
	break;
    }

    return *this;
}


/*
 * Aggregate all entries to this task.
 */

Task&
Task::aggregateEntries()
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    /* Aggregate calls to task */

    for ( unsigned i = 1; i <= paths().size(); ++i ) {
	while ( anEntry = nextEntry() ) {
	    anEntry->aggregateEntries( myPaths[i] );	/* Aggregate based on ref-task chain. */
	}
    }

    return *this;
}

/*
 * Aggregate activities to this entry.
 */


Entry&
Entry::aggregate()
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    if ( startActivity() ) {

	Stack<const Activity *> activityStack( owner()->activities().size() ); 
	unsigned next_p = 1;
	startActivity()->aggregate( this, 1, next_p, 1.0, activityStack, &Activity::aggregateService );

	switch ( Flags::print[AGGREGATION].value.i ) {
	case AGGREGATE_ACTIVITIES:
	case AGGREGATE_PHASES:
	case AGGREGATE_ENTRIES:
	    const_cast<LQIO::DOM::Entry *>(dom)->setEntryType( LQIO::DOM::Entry::ENTRY_STANDARD );
	    break;

	case AGGREGATE_SEQUENCES:
	case AGGREGATE_THREADS:
	    if ( startActivity()->transmorgrify() ) {
		const_cast<LQIO::DOM::Entry *>(dom)->setEntryType( LQIO::DOM::Entry::ENTRY_STANDARD );
	    }
	    break;

	default:
	    abort();
	}
    }

    /* Convert entry if necessary */

    if ( dom->getEntryType() == LQIO::DOM::Entry::ENTRY_STANDARD ) {
	myActivity = 0;
	if ( myActivityCall ) {
	    delete myActivityCall;
	    myActivityCall = 0;
	}
	myActivityCallers.clearContents();
    }

    switch ( Flags::print[AGGREGATION].value.i ) {
    case AGGREGATE_PHASES:
    case AGGREGATE_ENTRIES:
	aggregatePhases();
	break;
    }

    return *this;
}


/*
 * Aggregate all entries to the task level
 */

const Entry&
Entry::aggregateEntries( const unsigned k ) const
{
    if ( !hasPath( k ) ) return *this;		/* Not for this chain! */

    Task * srcTask = const_cast<Task *>(owner());

    const double scaling = srcTask->throughput() ? (throughput() / srcTask->throughput()) : (1.0 / srcTask->nEntries());
    const double s = srcTask->serviceTime( k ) + scaling * serviceTimeForSRVNInput();
    srcTask->serviceTime( k, s );
    const_cast<Processor *>(owner()->processor())->serviceTime( k, s );

    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	TaskCall * aTaskCall;
	if ( aCall->isPseudoCall() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddFwdCall( aCall->dstTask() ));
	} else if ( aCall->hasRendezvous() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddCall( aCall->dstTask(), &GenericCall::hasRendezvous ));
	    aTaskCall->rendezvous( aTaskCall->rendezvous() + scaling * aCall->sumOfRendezvous() );
	} else if ( aCall->hasSendNoReply() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddCall( aCall->dstTask(), &GenericCall::hasSendNoReply ));
	    aTaskCall->sendNoReply( aTaskCall->sendNoReply() + scaling * aCall->sumOfSendNoReply() );
	} else if ( aCall->hasForwarding() ) {
	    aTaskCall = dynamic_cast<TaskCall *>(srcTask->findOrAddCall( aCall->dstTask(), &GenericCall::hasForwarding ));
	    aTaskCall->taskForward( aTaskCall->forward() + scaling * aCall->forward() );
	} else {
	    abort();
	}
    }

    return *this;
}
#endif

#if defined(REP2FLAT)
Model&
Model::expandModel()
{
    /* Copy arrays */

    set<Processor *,ltProcessor> old_processor( processor );
    set<Task *,ltTask> old_task( task );
    set<Entry *,ltEntry> old_entry( entry );

    /* Reset old arrays for new entries. */

    entry.clear();
    task.clear();
    processor.clear();
    _document->clearAllMaps();

    /* Expand Processors */

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = old_processor.begin(); nextProcessor != old_processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;

	int numProcReplicas = aProcessor->replicas();
	for (int k = 1; k <= numProcReplicas; k++) {
	    aProcessor->expandProcessor(k);
	}
    }

    /*  Expand entries */

    for ( set<Entry *,ltEntry>::const_iterator nextEntry = old_entry.begin(); nextEntry != old_entry.end(); ++nextEntry ) {
	Entry * anEntry = *nextEntry;
	int numEntryReplicas = anEntry->owner()->replicas();

	for (int k = 1; k <= numEntryReplicas; k++) {
	    anEntry->expandEntry(k);
	}
    }

    /* Expand Tasks */

    for ( set<Task *,ltTask>::const_iterator nextTask = old_task.begin(); nextTask != old_task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	int numTaskReplicas = aTask->replicas();
	if (aTask->processor()->replicas() > aTask->replicas()) {
	    cout << "\nWARNING:expandTask(): number of processor '" <<
		aTask->processor()->name();
	    cout << "'s replicas (" << aTask->processor()->replicas() << ")" << endl;
	    cout << "is greater than the number of task '" << aTask->name() << "'s replicas (" 
		 << aTask->replicas() << ")." << endl;
	    cout << "So some processor replicas will not be used." << endl;
	}
	for (int k = 1; k <= numTaskReplicas; k++) {
	    aTask->expandTask(k);
	}
    }

    /* Expand calls */

    for ( set<Entry *,ltEntry>::const_iterator nextEntry = old_entry.begin(); nextEntry != old_entry.end(); ++nextEntry ) {
	Entry * anEntry = *nextEntry;
	anEntry->expandCalls();
    }

    /*  Delete all original Entities from the symbol table and collections */

    for ( set<Entry *,ltEntry>::const_iterator nextEntry = old_entry.begin(); nextEntry != old_entry.end(); ++nextEntry ) {
	Entry * anEntry = *nextEntry;
	delete anEntry;
    }

    for ( set<Task *,ltTask>::const_iterator nextTask = old_task.begin(); nextTask != old_task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	delete aTask;
    }

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = old_processor.begin(); nextProcessor != old_processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	delete aProcessor;
    }

    return *this;
}



Model& 
Model::removeReplication() 
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	aTask->removeReplication();
    }

    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	aProcessor->removeReplication();
    }

    return *this;
}
#endif

/* ------------------------ Graphical output. ------------------------- */

/*
 * Print all the layers.
 */

ostream&
Model::printEEPIC( ostream & output ) const
{
    output << "% Created By: " << io_vars.lq_toolname << " Version " << VERSION << endl
	   << "% Invoked as: " << command_line << ' ' << _inputFileName << endl
	   << "\\setlength{\\unitlength}{" << 1.0/EEPIC_SCALING << "pt}" << endl
	   << "\\begin{picture}("
	   << static_cast<int>(right()+0.5) << "," << static_cast<int>(top() + 0.5) 
	   << ")(" << static_cast<int>(bottom()+0.5)
	   << "," << -static_cast<int>(bottom()+0.5) << ")" << endl;

    printLayers( output );

    output << "\\end{picture}" << endl;
    return output;
}


#if defined(EMF_OUTPUT)
ostream& 
Model::printEMF( ostream& output ) const
{
    /* header start */
    EMF::init( output, right(), top(), command_line );
    printLayers( output );
    EMF::terminate( output );
    return output;
}
#endif


/* 
 * See http://www.xfig.org/userman/fig-format.html
 */

ostream&
Model::printFIG( ostream& output ) const
{
    output << "#FIG 3.2" << endl
	   << "Portrait" << endl
	   << "Center" << endl
	   << "Inches" << endl
	   << "Letter" << endl
	   << "75.00" << endl
	   << "Single" << endl
	   << "-2" << endl;
    output << "# Created By: " << io_vars.lq_toolname << " Version " << VERSION << endl
	   << "# Invoked as: " << command_line << ' ' << _inputFileName << endl
	   << print_comment( "# ", getDOM()->getModelComment() ) << endl;
    output << "1200 2" << endl;
    Fig::initColours( output );

    /* alignment markers */

    if ( (submodel_output()
	 || queueing_output()
	 || Flags::print[CHAIN].value.i)
	&& Flags::print_alignment_box ) {
	Fig alignment;
	Point point[4];
	point[0].moveTo( left(), bottom() );
	point[1].moveTo( left(), top() );
	point[2].moveTo( right(), top() );
	point[3].moveTo( right(), bottom() );
	alignment.polyline( output, 4, point, Fig::POLYGON, Graphic::WHITE, Graphic::TRANSPARENT, (layers.size()+1)*10 );
    }

    printLayers( output );

    return output;
}


#if HAVE_GD_H && HAVE_LIBGD
ostream& 
Model::printGD( ostream& output, outputFuncPtr func ) const
{
    GD::create( static_cast<int>(right()+0.5), static_cast<int>(top()+0.5) );
    printLayers( output );
    (* func)( output );
    GD::destroy();
    return output;
}
#endif


ostream& 
Model::printPostScript( ostream& output ) const
{
    printPostScriptPrologue( output, _inputFileName, 
			     static_cast<int>(left()+0.5),
			     static_cast<int>(bottom()+0.5),
			     static_cast<int>(right()+0.5),
			     static_cast<int>(top()+0.5) );
    
    output << "save" << endl;

    PostScript::init( output );

    printLayers( output );

    output << "showpage" << endl;
    output << "restore" << endl;
    return output;
}



#if defined(SVG_OUTPUT)
ostream& 
Model::printSVG( ostream& output ) const
{
    output << "<?xml version=\"1.0\" standalone=\"no\"?>" << endl
	   << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20000303 Stylable//EN\"" << endl
	   << "\"http://www.w3.org/TR/2000/03/WD-SVG-20000303/DTD/svg-20000303-stylable.dtd\">" << endl;
    output << "<!-- Title: " << _inputFileName << " -->" << endl;
    output << "<!-- Creator: " << io_vars.lq_toolname << " Version " << VERSION << " -->" << endl;
#if defined(HAVE_CTIME)
    output << "<!-- ";
    time_t clock = time( (time_t *)0 );
    for ( char * s = ctime( &clock ); *s && *s != '\n'; ++s ) {
	output << *s;
    }
    output << " -->" << endl;
#endif
    output << "<!-- For: " << get_userid() << " -->" << endl;
    output << "<!-- Invoked as: " << command_line << ' ' << _inputFileName << " -->" << endl;
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""
	   << right() / (SVG_SCALING * 72.0) << "in\" height=\""
	   << top() / (SVG_SCALING * 72.0) << "in\" viewBox=\""
	   << "0 0 " 
	   << static_cast<int>(right() + 0.5)<< " " 
	   << static_cast<int>(top() + 0.5) << "\">" << endl;
    output << "<desc>" << getDOM()->getModelComment() << "</desc>" << endl;

    printLayers( output );
    output << "</svg>" << endl;
    return output;
}
#endif

#if defined(SXD_OUTPUT)
/*
 * This guy is complicated...
 * We dump to contents.xml, then zip a bunch of crap together.
 * Yow!
 */

ostream& 
Model::printSXD( ostream& output ) const
{
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
	   << "<!DOCTYPE office:document-content PUBLIC \"-//OpenOffice.org//DTD OfficeDocument 1.0//EN\" \"office.dtd\">" << endl;

    set_indent(0);
    output << indent( +1 ) << "<office:document-content xmlns:office=\"http://openoffice.org/2000/office\" xmlns:style=\"http://openoffice.org/2000/style\" xmlns:text=\"http://openoffice.org/2000/text\" xmlns:table=\"http://openoffice.org/2000/table\" xmlns:draw=\"http://openoffice.org/2000/drawing\" xmlns:fo=\"http://www.w3.org/1999/XSL/Format\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:number=\"http://openoffice.org/2000/datastyle\" xmlns:presentation=\"http://openoffice.org/2000/presentation\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:chart=\"http://openoffice.org/2000/chart\" xmlns:dr3d=\"http://openoffice.org/2000/dr3d\" xmlns:math=\"http://www.w3.org/1998/Math/MathML\" xmlns:form=\"http://openoffice.org/2000/form\" xmlns:script=\"http://openoffice.org/2000/script\" office:class=\"drawing\" office:version=\"1.0\">" << endl;
    output << indent( 0 )  << "<office:script/>" << endl;

    SXD::init( output );

    output << indent( +1 ) << "<office:body>" << endl;
    output << indent( +1 ) << "<draw:page draw:name=\""
	   << _inputFileName
	   << "\" draw:style-name=\"dp1\" draw:master-page-name=\"Default\">" << endl;

    printLayers( output );

    output << indent( -1 ) << "</draw:page>" << endl;
    output << indent( -1 ) << "</office:body>" << endl;
    output << indent( -1 ) << "</office:document-content>" << endl;

    return output;
}
#endif
#if defined(X11_OUTPUT)
ostream& 
Model::printX11( ostream& output ) const
{
    return output;
}
#endif

/* ---------------------- Non-graphical output. ----------------------- */


/*
 * Change the order of the entitiy list from the order or input to the order we have assigned.
 */

map<unsigned, LQIO::DOM::Entity *>& 
Model::remapEntities() const
{
    map<unsigned, LQIO::DOM::Entity *>& entities = const_cast<map<unsigned, LQIO::DOM::Entity *>&>(getDOM()->getEntities());
    const LayerSequence nextEntity( layers );
    const Entity * anEntity;
    
    entities.clear();
    for ( unsigned i = 1; anEntity = nextEntity(); ++i ) {
	entities[i] = const_cast<LQIO::DOM::Entity *>(dynamic_cast<const LQIO::DOM::Entity *>(anEntity->getDOM()));	/* Our order, not the dom's */
    }
    return entities;
}


/* 
 * Output an input file.
 */

ostream& 
Model::printInput( ostream& output ) const
{
    LQIO::SRVN::Input srvn( *getDOM(), remapEntities(), Flags::annotate_input, Flags::print[RUN_LQX].value.b );
    srvn.print( output );
    return output;
}


/*
 * Output an output file.
 */

ostream& 
Model::printOutput( ostream& output ) const
{
    LQIO::SRVN::Output srvn( *getDOM(), remapEntities(), Flags::print[CONFIDENCE_INTERVALS].value.b, Flags::print[VARIANCE].value.b, Flags::print[HISTOGRAMS].value.b );
    srvn.print( output );
    return output;
}



/*
 * Output an output file.
 */

ostream& 
Model::printParseable( ostream& output ) const
{
    LQIO::SRVN::Parseable srvn( *getDOM(), remapEntities(), Flags::print[CONFIDENCE_INTERVALS].value.b );
    srvn.print( output );
    return output;
}



/*
 * Output an output file.
 */

ostream& 
Model::printRTF( ostream& output ) const
{
    LQIO::SRVN::RTF srvn( *getDOM(), remapEntities(), Flags::print[CONFIDENCE_INTERVALS].value.b );
    srvn.print( output );
    return output;
}



#if defined(TXT_OUTPUT)
/*
 * Dump the layers.
 */

ostream& 
Model::printTXT( ostream& output ) const
{
    printLayers( output );
    return output;
}
#endif
/*
 * Convert to XML output.
 */

void
Model::printXML( ostream& output ) const
{
    remapEntities();		/* Reorder to our order */
    _document->serializeDOM( output, Flags::print[RUN_LQX].value.b );	/* Don't output LQX code if running. */
}


#if defined(PMIF_OUTPUT)
/*
 * Convert to XML output.
 */

ostream&
Model::printPMIF( ostream& output ) const
{
    output << "<?xml version=\"1.0\"?>" << endl;
    output << "<!-- Generated by: " << io_vars.lq_toolname << ", version " << VERSION << " -->" << endl;
    set_indent(0);
    output << indent(+1) 
	   << "<QueueingNetworkModel "
	   << "xmlns:xsi=\"" << XMLSchema_instance << "\" xsi:noNamespaceSchemaLocation=\""
	   << "www.perfeng.com/pmif/pmifschema.xsd" << "\">" << endl;

    printLayers( output );

    output << indent(-1)
	   << "</QueueingNetworkModel>" << endl;
    return output;
}
#endif

/*
 * Print out one layer at at time.  Used by most graphical output routines.
 */

ostream&
Model::printLayers( ostream& output ) const
{
    if ( queueing_output() ) {
	const int submodel = Flags::print[QUEUEING_MODEL].value.i + 1;
	switch( Flags::print[OUTPUT_FORMAT].value.i ) {
#if defined(QNAP_OUTPUT)
	case FORMAT_QNAP: layers[submodel].printQNAP( output ); break;
#endif
#if defined(PMIF_OUTPUT)
	case FORMAT_XML: layers[submodel].printPMIF( output ); break;
#endif
	default: layers[submodel].drawQueueingNetwork( output ); break;
	}
    } else {
	Sequence<Group *> nextGroup( group );
	Group * aGroup;
	while ( aGroup = nextGroup() )  {
	    if ( aGroup->isPseudoGroup() ) {
		output << *aGroup;		/* Draw first */
	    }
	}
	while ( aGroup = nextGroup() )  {
	    if ( !aGroup->isPseudoGroup() ) {
		output << *aGroup;
	    }
	}

	for ( unsigned int i = 1; i <= layers.size(); ++i ) {
	    if ( !layers[i] ) continue;
#if defined(TXT_OUTPUT)
	    if ( Flags::print[OUTPUT_FORMAT].value.i == FORMAT_TXT ) {
		output << "---------- Layer " << i << " ----------" << endl;
	    }
#endif
	    output << layers[i];
	}
	if ( myKey ) {
	    output << *myKey;
	}
    }
    return output;
}



ostream&
Model::printEEPICprologue( ostream& output )
{
    output << "\\documentclass{article}" << endl;
    output << "\\usepackage{epic,eepic}" << endl;
    output << "\\usepackage[usenames]{color}" << endl;
    output << "\\begin{document}" << endl;
    return output;
}

ostream&
Model::printEEPICepilogue( ostream& output )
{
    output << "\\end{document}" << endl;
    return output;
}



/*
 * For 4-up printing run...
 * pstops '4:0@.5(0in,5.5in)+1@.5(4.5in,5.5in)+2@.5(0in,0in)+3@.5(4.5in,0in)' multi.ps > new.ps
 */

ostream&
Model::printPostScriptPrologue( ostream& output, const string& title, 
				unsigned left, unsigned top, unsigned right, unsigned bottom )
{
    output << "%!PS-Adobe-2.0" << endl;
    output << "%%Title: " << title << endl;
    output << "%%Creator: " << io_vars.lq_toolname << " Version " << VERSION << endl;
#if defined(HAVE_CTIME)
    time_t tloc;
    time( &tloc );
    output << "%%CreationDate: " << ctime( &tloc );
#endif
    output << "%%For: " << get_userid() << endl;
    output << "%%Invoked as: " << command_line << ' ' << title << endl;
    output << "%%Orientation: Portrait" << endl;
    output << "%%Pages: " << maxModelNumber << endl;
    output << "%%BoundingBox: " << left << ' ' << top << ' ' << right << ' ' << bottom << endl;
    output << "%%BeginSetup" << endl;
    output << "%%IncludeFeature: *PageSize Letter" << endl;
    output << "%%EndSetup" << endl;
    output << "%%Magnification: 0.7500" << endl;
    output << "%%EndComments" << endl;
    return output;
}

#if defined(SXD_OUTPUT)
/* Open Office output...
 * The output is somewhat more complicated because we have to write out a pile of crap.
 */
#if defined(WINNT)
#define MKDIR(a1,a2) mkdir( a1 )
#else
#define MKDIR(a1,a2) mkdir( a1, a2 )
#endif

const Model& 
Model::printSXD( const char * file_name ) const
{
    /* Use basename of input file name */
    LQIO::Filename dir_name( file_name, "" );

    if ( MKDIR( dir_name(), S_IRWXU ) < 0 ) {
	ostringstream msg;	
	msg << "Cannot create directory \"" << dir_name() << "\" - " << strerror( errno );
	throw runtime_error( msg.str() );
    } else {
	string meta_name = dir_name();
	meta_name += "/META-INF";
	if ( MKDIR( meta_name.c_str(), S_IRWXU ) < 0 ) {
	    ostringstream msg;	
	    msg << "Cannot create directory \"" << meta_name << "\" - " << strerror( errno );
	    rmdir( dir_name() );
	    throw runtime_error( msg.str() );
	} else try {
	    printSXD( file_name, dir_name(), "META-INF/manifest.xml", &Model::printSXDManifest );
	    printSXD( file_name, dir_name(), "content.xml", &Model::printSXD );
	    printSXD( file_name, dir_name(), "meta.xml", &Model::printSXDMeta );
	    printSXD( file_name, dir_name(), "styles.xml", &Model::printSXDStyles );
	    printSXD( file_name, dir_name(), "settings.xml", &Model::printSXDSettings );
	    printSXD( file_name, dir_name(), "mimetype", &Model::printSXDMimeType );
	    rmdir( meta_name.c_str() );
	} 
	catch ( runtime_error &error ) {
	    rmdir( meta_name.c_str() );
	    rmdir( dir_name() );
	    throw;
	}

	rmdir( dir_name() );
    }
    return *this;
}

const Model&
Model::printSXD( const char * dst_name, const char * dir_name, const char * file_name, const printSXDFunc aFunc ) const
{
    string pathname = dir_name;
    pathname += "/";
    pathname += file_name;

    ofstream output;
    output.open( pathname.c_str(), ios::out );
    if ( !output ) {
	ostringstream msg;
	msg << "Cannot open output file \"" << pathname << "\" - " << strerror( errno );
	throw runtime_error( msg.str() );
    } else {
	/* Write out all other XML goop needed */
	(this->*aFunc)( output );
	output.close();

#if !defined(WINNT)
	ostringstream command;
	command << "cd " << dir_name << "; zip -r ../" << dst_name << " " << file_name;
	int rc = system( command.str().c_str() );
	unlink( pathname.c_str() );	/* Delete now. */
	if ( rc != 0 ) {
	    ostringstream msg;
	    msg << "Cannot execute \"" << command.str() << "\" - ";
	    if ( rc < 0 ) {
		msg << strerror( errno );
	    } else {
		msg << "status=0x" << hex << rc;
	    }
	    throw runtime_error( msg.str() );
	}
#endif
    }
    return *this;
}


ostream&
Model::printSXDMeta( ostream& output ) const
{
    char buf[32];

    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    output << "<!DOCTYPE office:document-meta PUBLIC \"-//OpenOffice.org//DTD OfficeDocument 1.0//EN\" \"office.dtd\">" << endl;
    output << "<office:document-meta xmlns:office=\"http://openoffice.org/2000/office\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:meta=\"http://openoffice.org/2000/meta\" xmlns:presentation=\"http://openoffice.org/2000/presentation\" xmlns:fo=\"http://www.w3.org/1999/XSL/Format\" office:version=\"1.0\">" << endl;
    output << "<office:meta>" << endl;
#if defined(HAVE_CTIME)
    time_t tloc;
    time( &tloc );
    strftime( buf, 32, "%Y-%m-%dT%T", localtime( &tloc ) );
#endif
    output << "<dc:title>" << _inputFileName << "</dc:title>" << endl;
    output << "<dc:comment>" << getDOM()->getModelComment() << "</dc:comment>" << endl;
    output << "<dc:creator>" << get_userid() << "</dc:creator>" << endl;
    output << "<dc:date>" << buf << "</dc:date>" << endl;
    output << "<dc:language>en-US</dc:language>" << endl;

    LayerSequence nextEntity( layers );

    output << "<meta:generator>" << io_vars.lq_toolname << " Version " << VERSION << "</meta:generator>" << endl;
    output << "<meta:creation-date>" << buf << "</meta:creation-date>" << endl;
    output << "<meta:editing-cycles>1</meta:editing-cycles>" << endl;
#if defined(HAVE_SYS_TIMES_H)
    struct tms run_time;
    clock_t stop_clock = times( &run_time );
    strftime( buf, 32, "PT%MM%SS", localtime( &tloc ) );
    output << "<meta:editing-duration>" << buf << "</meta:editing-duration>" << endl;
#endif
    output << "<meta:user-defined meta:name=\"Info 1\">" << command_line << "</meta:user-defined>" << endl;
    output << "<meta:user-defined meta:name=\"Info 2\"/>" << endl;
    output << "<meta:user-defined meta:name=\"Info 3\"/>" << endl;
    output << "<meta:user-defined meta:name=\"Info 4\"/>" << endl;
    output << "<meta:document-statistic meta:object-count=\"" << nextEntity.size() << "\"/>" << endl;
    output << "</office:meta>" << endl;
    output << "</office:document-meta>" << endl;

    return output;
}

ostream&
Model::printSXDMimeType( ostream& output ) const
{
    output << "application/vnd.sun.xml.draw" << endl;
    return output;
}

ostream&
Model::printSXDSettings( ostream& output ) const
{
    set_indent(0);
    output << indent( 0 ) << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    output << indent( 0 ) << "<!DOCTYPE office:document-settings PUBLIC \"-//OpenOffice.org//DTD OfficeDocument 1.0//EN\" \"office.dtd\">" << endl;
    output << indent( 1 ) << "<office:document-settings xmlns:office=\"http://openoffice.org/2000/office\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:presentation=\"http://openoffice.org/2000/presentation\" xmlns:config=\"http://openoffice.org/2001/config\" xmlns:fo=\"http://www.w3.org/1999/XSL/Format\" office:version=\"1.0\">" << endl;
    output << indent( 1 ) << "<office:settings>" << endl;
    output << indent( -1 ) << "</office:settings>" << endl;
    output << indent( -1 ) << "</office:document-settings>" << endl;
    return output;
}

ostream&
Model::printSXDStyles( ostream& output ) const
{
    set_indent(0);
    return SXD::printStyles( output );
}

ostream&
Model::printSXDManifest( ostream& output ) const
{
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    output << "<!DOCTYPE manifest:manifest PUBLIC \"-//OpenOffice.org//DTD Manifest 1.0//EN\" \"Manifest.dtd\">" << endl;
    output << "<manifest:manifest xmlns:manifest=\"http://openoffice.org/2001/manifest\">" << endl;
    output << "<manifest:file-entry manifest:media-type=\"application/vnd.sun.xml.draw\" manifest:full-path=\"/\"/>" << endl;
    output << "<manifest:file-entry manifest:media-type=\"\" manifest:full-path=\"Pictures/\"/>" << endl;
    output << "<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>" << endl;
    output << "<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>" << endl;
    output << "<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"meta.xml\"/>" << endl;
    output << "<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"settings.xml\"/>" << endl;
    output << "</manifest:manifest>" << endl;
    return output;
}
#endif

/*
 *
 */

ostream&
Model::printStatistics( ostream& output, const char * filename ) const
{
    if ( filename ) {
	output << filename << ":" << endl;
    }

    unsigned n_activities = 0;
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	n_activities += aTask->nActivities();
    }
    
    output << "  Layers: " << layers.size() << endl
	   << "  Tasks: " << nTasks() << endl
	   << "  Processors: " << nProcessors() << endl
	   << "  Entries: " << nEntries() << endl
	   << "  Phases:" << phaseCount[1] << "," << phaseCount[2] << "," << phaseCount[3] << endl;
    if ( n_activities > 0 ) {
	output << "  Activites: " << n_activities << endl;
    }
    output << "  Customers: ";

    unsigned i = 0;
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	const Task * aTask = *nextTask;
	if ( aTask->isReferenceTask() ) {
	    i += 1;
	    if ( i > 1 ) {
		output << ",";
	    }
	    output << aTask->copies();
	}
    }
    output << endl;
    if ( openArrivalCount > 0 ) {
	output << "  Open Arrivals: " << openArrivalCount << endl;
    }
    if ( rendezvousCount[0] ) {
	output << "  Rendezvous: " << rendezvousCount[1] << "," << rendezvousCount[2] << "," << rendezvousCount[3] << endl;
    }
    if ( sendNoReplyCount[0] ) {
	output << "  Send-no-reply: " << sendNoReplyCount[1] << "," << sendNoReplyCount[2] << "," << sendNoReplyCount[3] << endl;
    }
    if ( forwardingCount ) {
	output << "  Forwarding: " << forwardingCount << endl;
    }
    return output;
}


ostream&
Model::printOverallStatistics( ostream& output ) 
{
    for ( int i = 0; i < N_STATS; ++i ) {
	if ( stats[i].sum() > 0 ) {
	    output << stats[i] << endl;
	}
    }
    return output;
}


ostream&
Model::printSummary( ostream& output ) const
{
    if ( _inputFileName.size() ) {
	output << _inputFileName << ":";
    }
    if ( graphical_output() ) {
	output << "  width=" << to_inches( right() ) << "\", height=" << to_inches( top() ) << "\"";
    }
    output << endl;

    if ( group.size() == 0 && Flags::print_submodels ) {
	for ( unsigned int i = 1; i < layers.size(); ++i ) {
	    cerr << "    " << setw( 2 ) << i << ": ";
	    unsigned int n_clients = layers[i].size();
	    unsigned int n_servers = layers[i+1].size();
	    if ( n_clients ) output << n_clients << " " << plural( "Client", n_clients ) << (n_servers ? ", " : "");
	    if ( n_servers ) output << n_servers << " " << plural( "Server", n_servers );
	    output << "." << endl;
	}
    } else {
	for ( unsigned int i = 1; i <= layers.size(); ++i ) {
	    if ( !layers[i] ) continue;
	    cerr << "    " << setw( 2 ) << i << ": ";
	    layers[i].printSummary( cerr ) << endl;
	}
    }

    return output;
}



const char *
Model::get_userid()
{
#if defined(HAVE_GETUID)
    struct passwd * passwd = getpwuid(getuid());
    return passwd->pw_name;
#elif defined(WINNT)
    return getenv("USERNAME");
#else
    return "";
#endif
}

/*----------------------------------------------------------------------*/
/*                             Batch Model.                             */
/*----------------------------------------------------------------------*/


/* 
 * Stick tasks and  processors into layers based on their call depth.
 * This corresponds to the batched model used in LQNS.
 */

Model&
Batch_Model::layerize()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;

	if ( aTask->level() == 0  || !aTask->pathTest() ) continue;

	layers[aTask->level()] << aTask;

	/* find who calls me and stick them in too */
	Sequence<Entry *> nextEntry(aTask->entries());
	Entry *anEntry;
	while ( anEntry = nextEntry() ) {
	    if ( (graphical_output() || queueing_output()) && anEntry->hasOpenArrivalRate() ) {
		layers[aTask->level()-1] << new OpenArrivalSource(anEntry);
	    }
	}

	Processor * aProcessor = const_cast<Processor *>(aTask->processor());
	if ( aProcessor && aProcessor->isInteresting() ) {
	    layers[aProcessor->level()] += static_cast<Entity *>(aProcessor);
	}
    }

    return *this;
}

/*----------------------------------------------------------------------*/
/*                              HWSW Model.                             */
/*----------------------------------------------------------------------*/


/* 
 * Stick tasks into the top two layers (the software model);
 * Stick all processors into the bottom layer (the hardware model).
 */

Model&
HWSW_Model::layerize()
{
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;

	if ( aTask->level() == 0 || !aTask->pathTest() ) {
	    continue;
	} else if ( aTask->level() > SERVER_LEVEL ) {
	    aTask->setLevel( SERVER_LEVEL );
	}
	    
	layers[aTask->level()] << aTask;

	/* find who calls me and stick them in too */
	Sequence<Entry *> nextEntry(aTask->entries());
	Entry *anEntry;
	while ( anEntry = nextEntry() ) {
	    if ( (graphical_output() || queueing_output()) && anEntry->hasOpenArrivalRate() ) {
		layers[CLIENT_LEVEL] << new OpenArrivalSource(anEntry);
	    }
	}

	Processor * aProcessor = const_cast<Processor *>(aTask->processor());
	if ( aProcessor && aProcessor->isInteresting() ) {
	    aProcessor->setLevel( PROCESSOR_LEVEL );
	    layers[PROCESSOR_LEVEL] += static_cast<Entity *>(aProcessor);
	}
    }

    return *this;
}

/*----------------------------------------------------------------------*/
/*                          Strict (MOL) Model.                         */
/*----------------------------------------------------------------------*/


/* 
 * Stick tasks into all but the bottom layer (the software model);
 * stick all processors into the bottom layer (the hardware model).
 * This technique corresonds to Rolia's.
 */

Model&
MOL_Model::layerize()
{
    const unsigned PROC_LEVEL = nLayers();

    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;

	if ( aTask->level() == 0 || !aTask->pathTest() ) continue;

	layers[aTask->level()] << static_cast<Entity *>(aTask);

	/* find who calls me and stick them in too */
	Sequence<Entry *> nextEntry(aTask->entries());
	Entry *anEntry;
	while ( anEntry = nextEntry() ) {
	    if ( (graphical_output() || queueing_output()) && anEntry->hasOpenArrivalRate() ) {
		layers[aTask->level()-1] << new OpenArrivalSource(anEntry);
	    }
	}

	Processor * aProcessor = const_cast<Processor *>(aTask->processor());
	if ( aProcessor && aProcessor->isInteresting() ) {
	    aProcessor->setLevel( PROC_LEVEL );
	    layers[PROC_LEVEL] += static_cast<Entity *>(aProcessor);
	}
    }

    return *this;
}

/*
 * Each group consists of a set of processors and their tasks.  These
 * are all formatted then justified as needed.
 */

Model const&
Group_Model::justify() const
{
    const unsigned MAX_LEVEL = layers.size();
    
    myOrigin.x( 0 );

    /* Now sort by groups */

    Sequence<Group *> nextGroup( group );
    Group * aGroup;

    for ( double x = 0.0; aGroup = nextGroup(); x += Flags::print[X_SPACING].value.f )  {

	aGroup->format( MAX_LEVEL ).label().resizeBox().positionLabel();

	/* The next column starts here.  PseudoGroups (for group
	 * scheduling) are ignored if they have no default tasks */

	if ( !aGroup->isPseudoGroup() ) {
	    aGroup->moveGroupBy( x, 0.0 );
	    aGroup->depth( (MAX_LEVEL+1) * 10 );
	    x += aGroup->width();
	} else {
	    x = aGroup->x() + aGroup->width();
	    aGroup->depth( (MAX_LEVEL+2) * 10 );
	}
	myExtent.x( x );
    }

    for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
	layers[i].moveLabelTo( right(), layers[i].height() / 2.0 ).sort( Entity::compareCoord );
    }

    return *this;
}

/*
 * This version of justify moves all of the processors and all of the tasks together.
 */

Model const&
ProcessorTask_Model::justify() const
{
    const unsigned MAX_LEVEL = layers.size();
    double procWidthPts = 0.0;
    double taskWidthPts = 0.0;

    Vector2<Layer> procLayer;
    Vector2<Layer> taskLayer;
    procLayer.grow(MAX_LEVEL);
    taskLayer.grow(MAX_LEVEL);

    for ( unsigned i = MAX_LEVEL; i > 0; --i ) {
	if ( layers.size() == 0 ) continue;

	Sequence<Entity *> nextEntity( layers[i].entities() );
	Entity * anEntity;

	while ( anEntity = nextEntity() ) {
	    if ( dynamic_cast<Processor *>(anEntity) ) {
		procLayer[i] << anEntity;
	    } else {
		taskLayer[i] << anEntity;
	    }
	}

	/* Move all processors in a given layer together */
	if ( procLayer[i].size() ) {
	    procLayer[i].reformat();
	    procWidthPts = max( procWidthPts, procLayer[i].x() + procLayer[i].width() );
	}
	/* Move all tasks in a given layer together */
	if ( taskLayer[i].size() ) {
	    taskLayer[i].reformat();
	    taskWidthPts = max( taskWidthPts, taskLayer[i].x() + taskLayer[i].width() );
	}
    }

    /* Set max width */

    myExtent.x( procWidthPts + Flags::print[X_SPACING].value.f + taskWidthPts );

    /* Now, move all tasks */

    for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
	if ( layers[i].size() == 0 ) continue;

	switch ( Flags::node_justification ) {
	case ALIGN_JUSTIFY:
	case CENTER_JUSTIFY:
	    justify2( procLayer[i], taskLayer[i], (right() - (taskLayer[i].width() + procLayer[i].width() + Flags::print[X_SPACING].value.f)) / 2.0 );
	    break;
	case RIGHT_JUSTIFY:
	    justify2( procLayer[i], taskLayer[i],  right() - (taskLayer[i].width() + procLayer[i].width() + Flags::print[X_SPACING].value.f) );
	    break;
	case LEFT_JUSTIFY:
	    justify2( procLayer[i], taskLayer[i],  0.0 );
	    break;
	case DEFAULT_JUSTIFY:
	    if ( Flags::print[LAYERING].value.i == LAYERING_PROCESSOR_TASK ) {
		procLayer[i].justify( procWidthPts, RIGHT_JUSTIFY );
		taskLayer[i].justify( taskWidthPts, LEFT_JUSTIFY ).moveBy( procWidthPts + Flags::print[X_SPACING].value.f, 0.0 );
	    } else {
		taskLayer[i].justify( taskWidthPts, RIGHT_JUSTIFY );
		procLayer[i].justify( procWidthPts, LEFT_JUSTIFY ).moveBy( taskWidthPts + Flags::print[X_SPACING].value.f, 0.0 );
	    }
	    break;
	}
    }

    return *this;
}


const Model&
ProcessorTask_Model::justify2( Layer &procLayer, Layer &taskLayer, const double offset ) const
{
    if ( Flags::print[LAYERING].value.i == LAYERING_PROCESSOR_TASK ) {
	procLayer.moveBy( offset, 0.0 );
	taskLayer.moveBy( offset + procLayer.width() + Flags::print[X_SPACING].value.f, 0.0 );
    } else {
	taskLayer.moveBy( offset, 0.0 );
	procLayer.moveBy( offset + taskLayer.width() + Flags::print[X_SPACING].value.f, 0.0 );
    }
    return *this;
}

/* 
 * The trick for the SRVN Model is to retain the layering from the
 * topological sorter, but to treat each server as its own submodel.
 * All of the servers are ordered based on their level then entityID.
 * We only select the server who matches the submodel.
 */

bool
SRVN_Model::selectSubmodel( const unsigned submodel ) 
{
    /* Build the list of all servers for this model */

    multiset<Entity *,lt_submodel> servers;
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( aTask->isReferenceTask() || aTask->level() <= 0 ) continue;
	servers.insert( aTask );
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	if ( aProcessor->level() <= 0 ) continue;
	servers.insert( aProcessor );
    }

    unsigned int s = 1;
    for ( multiset<Entity *,lt_submodel>::const_iterator next_server = servers.begin(); next_server != servers.end(); ++next_server, ++s ) {
        Entity * aServer = *next_server;
	if ( s == submodel ) {
	    aServer->isSelected( true );
	    return true;
	}
    }

    return false;
}

/* 
 * The trick for squashed layering is to simply jamb all tasks in
 * layer 1 and all processors in layer 2.
 */

bool
Squashed_Model::generate()
{
    topologicalSort();

    /*
     * Now go through and reset the level field on all the objects.
     */
	   
    for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	Task * aTask = *nextTask;

	if ( aTask->hasActivities() ) { 
	    aTask->generate();
	}
	if ( aTask->level() == 0 ) {
	    LQIO::solution_error( LQIO::WRN_NOT_USED, "Task", aTask->name().c_str() );
	} else if ( aTask->level() > SERVER_LEVEL ) {
	    aTask->setLevel( SERVER_LEVEL );
	}
    }
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = processor.begin(); nextProcessor != processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	if ( aProcessor->level() == 0 ) {
	    LQIO::solution_error( LQIO::WRN_NOT_USED, "Processor", aProcessor->name().c_str() );
	} else {
	    aProcessor->setLevel( PROCESSOR_LEVEL );
	}
    }

    layers.grow( PROCESSOR_LEVEL );

    return true;
}

Model const&
Squashed_Model::justify() const
{
    Model::justify();

    /* Now sort by groups */

    Sequence<Group *> nextGroup( group );
    Group * aGroup;

    while ( aGroup = nextGroup() )  {
	aGroup->format( 0 ).label().resizeBox().positionLabel();
    }

    return *this;
}

/*----------------------------------------------------------------------*/
/*                          Strict Client Model.                        */
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/* Sequences of objects over all the layers.				*/
/*----------------------------------------------------------------------*/

Model::LayerSequence::LayerSequence( const Vector2<Layer> & layers ) 
    : myLayers( layers ), layerIndex(1), entityIndex(1)
{
}



/*
 * Return the next entity in the list.
 */

Entity *
Model::LayerSequence::operator()() const
{
    while ( layerIndex <= myLayers.size() ) {
	const Cltn<Entity *>& entities = myLayers[layerIndex].entities();
	if ( entityIndex <= entities.size() ) {
	    Entity * anEntity = entities[entityIndex];
	    entityIndex += 1;
	    if ( anEntity->isSelectedIndirectly() ) {
		return anEntity;
	    }
	} else {
	    entityIndex = 1;
	    layerIndex += 1;
	}
    } 

    layerIndex = 1;
    return 0;
}


/*
 * We sequence over ourself to get size.
 */

unsigned
Model::LayerSequence::size() const
{
    unsigned count = 0;
    while ( (*this)() ) {
	count += 1;
    }
    return count;
}


const Model::LayerSequence&
Model::LayerSequence::rewind() const
{
    layerIndex = 1;
    entityIndex = 1;
    return *this;
}

Model::Stats::Stats() 
    : n(0), x(0), x_sqr(0), log_x(0), one_x(0), min(MAXDOUBLE), max(-MAXDOUBLE), myFunc(0)
{
    min_filename = "";
    max_filename = "";
}



Model::Stats&
Model::Stats::accumulate( double value, const string& filename )
{
    n += 1;
    x += value;
    x_sqr += value * value;
    log_x += log( value );
    one_x += 1.0 / value;
    if ( value < min ) {
	min = value;
	min_filename = filename;
    } else if ( value == min && min_filename.find( filename ) == string::npos ) {
	min_filename += ", ";
	min_filename += filename;
    }
    if ( value > max ) {
	max = value;
	max_filename = filename;
    } else if ( value == max && max_filename.find( filename ) == string::npos ) {
	max_filename += ", ";
	max_filename += filename;
    }
    return *this;
}


Model::Stats&
Model::Stats::accumulate( const Model * aModel, const string& filename )
{
    assert( aModel && myFunc );
    return accumulate( static_cast<double>((aModel->*myFunc)()), filename );
}


/*
 * Compute population standard deviation.
 */

ostream&
Model::Stats::print( ostream& output) const
{
    output << myName << ":" << endl;
    double stddev = 0.0;
    if ( n > 1 ) {
	const double numerator = x_sqr - ( x * x ) / static_cast<double>(n);
	if ( numerator > 0.0 ) {
	    stddev = sqrt( numerator / static_cast<double>( n ) );
	} 
    }
    output << "  mean   = " << x / static_cast<double>(n) << endl;
    output << "  geom   = " << exp( log_x / static_cast<double>(n) ) << endl;	/* Geometric mean */
    output << "  stddev = " << stddev << endl;
    output << "  max    = " << max << " (" << max_filename << ")" << endl;
    output << "  min    = " << min << " (" << min_filename << ")" << endl;
    return output;
}

Model::Aggregate::Aggregate() 
{
}

void 
Model::Aggregate::operator()( const std::pair<std::string,LQIO::DOM::Task *>& tp )
{
    LQIO::DOM::Task * task = tp.second;
    if ( task->getActivities().size() != 0 ) {
	const std::vector<LQIO::DOM::Entry *>& entries = task->getEntryList();
	for_each( entries.begin(), entries.end(), Model::Aggregate() );
    }
    /* For each entry that has activities do... */
    /* Merge sequences to single (new?) activity */
    /* OK - is an entry defined by activities, so lets rip em out. */
}

void 
Model::Aggregate::operator()( const LQIO::DOM::Entry * entry )
{
    /* aggregate this activity to phase */
    aggregate( const_cast<LQIO::DOM::Entry *>(entry), 1, 1.0, entry->getStartActivity() );
}


unsigned int
Model::Aggregate::aggregate( LQIO::DOM::Entry * entry, unsigned int p, double rate, const LQIO::DOM::Activity * activity ) 
{
    if ( !activity ) return p;

    /* aggregate activity service time to phase */

    LQIO::DOM::Phase * phase = entry->getPhase(p);		// Will create a phase if necessary.
    LQIO::DOM::ExternalVariable * dst_service_time = const_cast<LQIO::DOM::ExternalVariable *>(phase->getServiceTime());
    LQIO::DOM::ExternalVariable * src_service_time = const_cast<LQIO::DOM::ExternalVariable *>(activity->getServiceTime());
    if ( !dst_service_time ) {
	phase->setServiceTime( src_service_time );
    } else if ( phase->hasServiceTime() && activity->hasServiceTime() ) {
	*dst_service_time *= rate;
	*src_service_time += *dst_service_time;
    }
	
    /* Move calls from activity to phase */

    std::vector<LQIO::DOM::Call *>& dst_calls = const_cast<std::vector<LQIO::DOM::Call *>&>(phase->getCalls());
    const std::vector<LQIO::DOM::Call *>& src_calls = activity->getCalls();
    for ( std::vector<LQIO::DOM::Call *>::const_iterator src_call = src_calls.begin(); src_call != src_calls.end(); ++src_call ) {
	LQIO::DOM::Call * call = *src_call;
	LQIO::DOM::ExternalVariable * dst_call_mean = const_cast<LQIO::DOM::ExternalVariable *>(call->getCallMean());
	if ( dst_call_mean && dst_call_mean->wasSet() ) {
	    *dst_call_mean *= rate;    // Adjust rate... 
	}
	dst_calls.push_back( call );
    }
    
    // if activity replies to entry, p = 2 

    /* Go down the activity list, follow all paths of forks, stop at joins (except if only one activity). */

    LQIO::DOM::ActivityList * join = activity->getOutputToList();
    if ( join ) {

	std::vector<const LQIO::DOM::Activity*>& join_list = const_cast<std::vector<const LQIO::DOM::Activity*>&>(join->getList());
	if ( join_list.size() == 1 ) {
	    LQIO::DOM::ActivityList * fork = join->getNext();
	    const std::vector<const LQIO::DOM::Activity*>& fork_list = fork->getList();
	    for ( std::vector<const LQIO::DOM::Activity*>::const_iterator ap = fork_list.begin(); ap != fork_list.end(); ++ap ) {
		p = aggregate( entry, p, rate, (*ap ) );	// set rate if or-fork or loop 
	    }
	}
    
	/* remove activity */

	for ( std::vector<const LQIO::DOM::Activity*>::iterator item = join_list.begin(); item != join_list.end(); ++item ) {
	    if ( activity == *item ) {
		join_list.erase( item );
		break;
	    }
	}
    }
    // delete activity
    
    return p;
}

static ostream&
print_comment_str( ostream& output, const char * aPrefix, const string & aComment )
{
    output << aPrefix;
    for ( unsigned int i = 0; i < aComment.length(); ++i) {
	output << aComment[i];
	if ( aComment[i] == '\n' ) {
	    output << aPrefix;
	}
    }
    return output;
}

static ostream&
to_inches_str( ostream& output, const double value )
{
    switch ( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_FIG:
    case FORMAT_PSTEX:
	output << value / (FIG_SCALING * PTS_PER_INCH);
	break;

    case FORMAT_EEPIC:
	output << value / (EEPIC_SCALING * PTS_PER_INCH);
	break;
#if defined(EMF_OUTPUT)
    case FORMAT_EMF:
	output << value / (EMF_SCALING * PTS_PER_INCH);
	break;
#endif
#if defined(SVG_OUTPUT)
    case FORMAT_SVG:
	output << value / (SVG_SCALING * PTS_PER_INCH);
	break;
#endif
#if defined(SXD_OUTPUT)
    case FORMAT_SXD:
	output << value / (SXD_SCALING * PTS_PER_INCH);
	break;
#endif
    default:
	output << value / PTS_PER_INCH;
	break;
    }
    return output;
}



static CommentManip print_comment( const char * aPrefix, const string& aStr )
{
    return CommentManip( &print_comment_str, aPrefix, aStr );
}


static DoubleManip 
to_inches( const double value )
{
    return DoubleManip( &to_inches_str, value );
}
