/* -*- c++ -*-
 * $HeadURL$
 * 
 * Everything you wanted to know about a task, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#include "dim.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <lqio/input.h>
#include <lqio/labels.h>
#include <lqio/error.h>
#include "fpgoop.h"
#include "cltn.h"
#include "errmsg.h"
#include "entry.h"
#include "entity.h"
#include "processor.h"
#include "task.h"
#include "server.h"
#include "multserv.h"
#include "call.h"
#include "lqns.h"
#include "pragma.h"
#include "activity.h"

set<Processor *, ltProcessor> processor;


/* ------------------------ Constructors etc. ------------------------- */

Processor::Processor( LQIO::DOM::Processor* aDomProcessor )
    : Entity( (LQIO::DOM::Entity*)aDomProcessor ), 
      myDOMProcessor(aDomProcessor)
{
}
	
/*
 * Destructor...
 */

Processor::~Processor()
{
    taskList.clearContents();
}

/* ------------------------ Instance Methods -------------------------- */

void 
Processor::check() const
{
    if ( rate() < 0.0 ) {
	LQIO::solution_error( LQIO::ERR_INVALID_PROC_RATE, name(), rate() );
    }

    if ( !schedulingIsOk( validScheduling() ) ) {
	if ( scheduling() == SCHEDULE_CFS ) {
	    io_vars.error_messages[LQIO::WRN_SCHEDULING_NOT_SUPPORTED].severity = LQIO::RUNTIME_ERROR;
	}
	LQIO::solution_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED,
			      scheduling_type_str[(unsigned)domEntity->getSchedulingType()],
			      "processor",
			      name() );
	domEntity->setSchedulingType(defaultScheduling());
    }
	
    if ( scheduling() == SCHEDULE_DELAY ) {
	if ( copies() != 1 ) {
	    solution_error( LQIO::WRN_INFINITE_MULTI_SERVER, "Processor", name(), copies() );
	    myDOMProcessor->setCopies(new LQIO::DOM::ConstantExternalVariable(1.0));
	}
    } else if ( pragma.getProcessor() != DEFAULT_PROCESSOR && copies() == 1 ) {
	/* Change scheduling type for uni-processors (usually from FCFS to PS) */
	myDOMProcessor->setSchedulingType(pragma.getProcessorScheduling());
    }
}



/*
 * Denote whether this station belongs to the open, closed, or mixed
 * models when performing the MVA solution.
 */

void
Processor::configure( const unsigned nSubmodels )
{
    if ( nEntries() == 0 ) {
	LQIO::solution_error( LQIO::WRN_NO_TASKS_DEFINED_FOR_PROCESSOR, name() );
	return;
    } 
	
    Sequence<Entry *> nextEntry( entries() );
    const Entry * anEntry;

    anEntry = nextEntry();
    double minS = anEntry->serviceTime();
    double maxS = anEntry->serviceTime();
    while ( anEntry = nextEntry() ) {
	minS = fmin( minS, anEntry->serviceTime() );
	maxS = fmax( maxS, anEntry->serviceTime() );
    }
    if ( maxS > 0. && minS / maxS < 0.1 
	 && !schedulingIsOk( SCHED_PS_BIT|SCHED_PS_HOL_BIT|SCHED_PS_PPR_BIT|SCHED_DELAY_BIT ) ) {
		LQIO::solution_error( ADV_SERVICE_TIME_RANGE, "processor", name(), minS, maxS );
    }
    Entity::configure( nSubmodels );
}



/*
 * Initialize population levels.  Since processors are servers, it
 * doesn't really matter.  However, we do have to figure out whether
 * we're in an open or a closed model (or both).
 */

Processor&
Processor::initPopulation()
{
    myPopulation = static_cast<double>(copies());	/* Doesn't matter... */

    Sequence<Task *> nextTask(taskList);
    Task *aTask;
    while ( aTask = nextTask() ) {
	Cltn<const Task *> sources;		/* Cltn of tasks already visited. */
	if ( aTask->countCallers( sources ) > 0. ) {
	    isInClosedModel( true );
	}
	if ( aTask->isInfinite() && aTask->isInOpenModel() ) {
	    isInOpenModel( true );
	}
    }
    return *this;
}



/*
 * Indicate whether the variance calculation should take place.  NOTE
 * that processors should not have the variance calculation set true,
 * so hasVariance is set in class Task rather than here.
 */

bool
Processor::hasVariance() const
{
    if ( pragma.getVariance() == NO_VARIANCE 
	 || pragma.getProcessor() != DEFAULT_PROCESSOR 
	 || scheduling() == SCHEDULE_PS
	 || isMultiServer() 
	 || isInfinite() ) {
	return false;
    }
	
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( anEntry->hasVariance() ) {
	    return true;
	}
    }
	
    return false;
}



/*
 * Return true if this processor can schedule tasks with priority.
 */

bool
Processor::hasPriorities() const
{
    return scheduling() == SCHEDULE_HOL 	       
	|| scheduling() == SCHEDULE_PPR
	|| scheduling() == SCHEDULE_PS_HOL
	|| scheduling() == SCHEDULE_PS_PPR;
}



/*
 * Return the processor for this processor???
 */

const Processor *
Processor::processor() const
{
    throw should_not_implement( "Processor::processor()", __FILE__, __LINE__ );
    return 0;
}



/*
 * Set the processor for this processor???
 */

Entity&
Processor::processor( Processor * ) 
{
    throw should_not_implement( "Processor::processor(Processor *)", __FILE__, __LINE__ );
    return *this;
}


/*
 * Add a task to the list of tasks for this processor.
 */

Processor&
Processor::addTask( Task * aTask )
{
    taskList += aTask;
    return *this;
}



/*
 * Remove aTask from the list of tasks for this processor.
 */

Processor&
Processor::removeTask( Task * aTask )
{
    taskList -= aTask;
    return *this;
}



/*
 * Return the scheduling type allowed for this object.  Overridden by
 * subclasses if the scheduling type can be something other than FIFO.
 */

unsigned
Processor::validScheduling() const
{
    if ( isInfinite() ) {
	return (unsigned)-1;
    } else if ( isMultiServer() ) {
	return SCHED_PS_BIT|SCHED_FIFO_BIT;
    } else {
	return SCHED_FIFO_BIT|SCHED_PPR_BIT|SCHED_HOL_BIT|SCHED_PS_BIT|SCHED_PS_PPR_BIT|SCHED_PS_HOL_BIT;
    }
}


/*
 * Create (or recreate) a server.  If we're called a a second+ time,
 * and the station type changes, then we change the underlying
 * station.  We only return a station when we create one.
 */

Server *
Processor::makeServer( const unsigned nChains ) 
{
    Server * oldStation = myServerStation;

    if ( isInfinite() ) {

	/* ---------------- Infinite Servers ---------------- */

	if ( dynamic_cast<Infinite_Server *>(myServerStation) ) return 0;
	myServerStation = new Infinite_Server( nEntries(), nChains, maxPhase() );

    } else if ( isMultiServer() ) {

	/* ---------------- Multi Servers ---------------- */

	if ( scheduling() == SCHEDULE_PS ) {

	    switch ( pragma.getMultiserver() ) {
	    default:
	    case DEFAULT_MULTISERVER:
	    case CONWAY_MULTISERVER:
	    case REISER_MULTISERVER:
	    case REISER_PS_MULTISERVER:
	    case SCHMIDT_MULTISERVER:
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Reiser_PS_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case ROLIA_PS_MULTISERVER:
	    case ROLIA_MULTISERVER:
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Rolia_PS_Multi_Server( copies(), nEntries(), nChains );
		break;
	    }

	} else {

	    switch ( pragma.getMultiserver() ) {
	    default:
	    case DEFAULT_MULTISERVER:
		if ( copies() < 20 && nChains <= 5 ) {
		    if ( dynamic_cast<Conway_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		    myServerStation = new Conway_Multi_Server( copies(), nEntries(), nChains );
		} else {
		    if ( dynamic_cast<Rolia_Multi_Server *>(myServerStation) ) return 0;
		    myServerStation = new Rolia_Multi_Server(  copies(), nEntries(), nChains );
		}
		break;

	    case CONWAY_MULTISERVER:
		if ( dynamic_cast<Conway_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Conway_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case REISER_MULTISERVER:
		if ( dynamic_cast<Reiser_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Reiser_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case REISER_PS_MULTISERVER:
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Reiser_PS_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case ROLIA_MULTISERVER:
		if ( dynamic_cast<Rolia_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Rolia_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case ROLIA_PS_MULTISERVER:
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Rolia_PS_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case BRUELL_MULTISERVER:
		if ( dynamic_cast<Bruell_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Bruell_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case SCHMIDT_MULTISERVER:
		if ( dynamic_cast<Schmidt_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Schmidt_Multi_Server( copies(), nEntries(), nChains );
		break;
	    }
	}
    } else {
	switch ( scheduling() ) {
	default:
	case SCHEDULE_FIFO:
	    if ( hasVariance() ) {
		if ( dynamic_cast<HVFCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<FCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new FCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	    break;
		
	case SCHEDULE_PPR:
	    if ( hasVariance() ) {
		if ( dynamic_cast<PR_HVFCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new PR_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<PR_FCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new PR_FCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	    break;
		
	case SCHEDULE_HOL:
	    if ( hasVariance() ) {
		if ( dynamic_cast<HOL_HVFCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new HOL_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<HOL_FCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new HOL_FCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	    break;
		
	case SCHEDULE_PS:
	    if ( dynamic_cast<PS_Server *>(myServerStation) ) return 0;
	    myServerStation = new PS_Server( nEntries(), nChains, maxPhase() );
	    break;

	case SCHEDULE_PS_HOL:
	    if ( dynamic_cast<HOL_PS_Server *>(myServerStation) ) return 0;
	    myServerStation = new HOL_PS_Server( nEntries(), nChains, maxPhase() );
	    break;

	case SCHEDULE_PS_PPR:
	    if ( dynamic_cast<PR_PS_Server *>(myServerStation) ) return 0;
	    myServerStation = new PR_PS_Server( nEntries(), nChains, maxPhase() );
	    break;

	}
    }

    if ( oldStation ) delete oldStation;

    return myServerStation;
}


void 
Processor::insertDOMResults(void) const
{
    Sequence<Task *> nextTask(taskList);
    const Task *aTask;
    double sumOfProcUtil = 0.0;

    while (aTask = nextTask())
    {
	Sequence<Entry *> nextEntry( aTask->entries() );
	const Entry * anEntry;

	while (anEntry = nextEntry())
        {
	    double util = anEntry->processorUtilization();
	    if (anEntry->isStandardEntry())
            {
		sumOfProcUtil += util;
	    }
	}

	if (aTask->hasActivities())
        {
	    Sequence<Activity *> nextActivity(aTask->activities());
	    Activity * anActivity;

	    while (anActivity = nextActivity())
            {
		const double util = anActivity->processorUtilization();
		sumOfProcUtil += util;
	    }
	}
    }
	
    if ( myDOMProcessor ) {
	myDOMProcessor->setResultUtilization(sumOfProcUtil);
    }
}


/*
 * Print out info for this processor.
 */

ostream&
Processor::print( ostream& output ) const
{
    const ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
    output << setw(8) << name()
	   << " " << setw(9) << processor_type( *this )
	   << " " << setw(5) << replicas() 
	   << " " << setw(12) << scheduling_type_str[scheduling()]
	   << "  "
	   << print_info( *this );	    /* Bonus information about stations -- derived by solver */
    output.flags(oldFlags);
    return output;
}

/* ----------------------- External functions. ------------------------ */

/*
 * Add a processor to the model.   
 */

void Processor::create( LQIO::DOM::Processor* domProcessor )
{
    /* Unroll some of the encapsulated information */
    const char* processor_name = domProcessor->getName().c_str();
	
    if ( !processor_name || strlen( processor_name ) == 0 ) abort();

    if ( Processor::find( processor_name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Processor", processor_name );
	return;
    }

    Processor * aProcessor;

    aProcessor = new Processor( domProcessor );
    ::processor.insert( aProcessor );

    if ( flags.trace_processor && strcmp( flags.trace_processor, processor_name ) == 0 ) {
	aProcessor->trace( true );
    }
}



/*
 * Find the processor and return it.  
 */

Processor *
Processor::find( const char * processor_name )
{
    set<Processor *,ltProcessor>::const_iterator nextProcessor = find_if( ::processor.begin(), ::processor.end(), eqProcStr( processor_name ) );
    if ( nextProcessor == ::processor.end() ) {
	return 0;
    } else {
	return *nextProcessor;
    }
}

static ostream&
processor_type_of( ostream& output, const Processor& aProcessor )
{
    char buf[12];
    const unsigned n      = aProcessor.copies();

    if ( aProcessor.scheduling() == SCHEDULE_CUSTOMER ) {
	sprintf( buf, "ref(%d)", n );
    } else if ( aProcessor.isInfinite() ) {
	sprintf( buf, "inf" );
    } else if ( n > 1 ) {
	sprintf( buf, "mult(%d)", n );
    } else {
	sprintf( buf, "serv" );
    }
    output << buf;
    return output;
}


SRVNProcessorManip
processor_type( const Processor& aProcessor )
{
    return SRVNProcessorManip( processor_type_of, aProcessor );
}
