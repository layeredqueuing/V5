/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/processor.cc $
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
 * $Id: processor.cc 17460 2024-11-12 14:14:37Z greg $
 * ------------------------------------------------------------------------
 */

#include "lqns.h"
#include <cmath>
#include <functional>
#include <numeric>
#include <sstream>
#include <lqio/input.h>
#include <lqio/labels.h>
#include <lqio/error.h>
#include <mva/multserv.h>
#include <mva/mva.h>
#include <mva/open.h>
#include <mva/server.h>
#include "activity.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "flags.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "submodel.h"
#include "task.h"

/* ------------------------ Constructors etc. ------------------------- */

Processor::Processor( LQIO::DOM::Processor* dom )
    : Entity( dom, std::vector<Entry *>() ),
      _groups()
{
}


Processor::Processor( const Processor& processor, unsigned int replica )
    : Entity( processor, replica ),
      _groups()
{
}



/*
 * Destructor...
 */

Processor::~Processor()
{
}

/* ------------------------ Instance Methods -------------------------- */

void
Processor::initializeServer()
{
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	DeviceEntry* device = dynamic_cast<DeviceEntry*>(*entry);
	if ( device == nullptr ) continue;	// abort()?
	device->initWait();
    }
}



bool
Processor::check() const
{
    Entity::check();
    
    /* Check replication */

    const unsigned int proc_replicas = this->replicas();
    for ( std::set<const Task *>::const_iterator task = tasks().begin(); task != tasks().end(); ++task ) {
	double temp = static_cast<double>((*task)->replicas()) / static_cast<double>(proc_replicas);
	if ( trunc( temp ) != temp  ) {			/* Integer multiple */
	    LQIO::runtime_error( LQIO::ERR_REPLICATION_PROCESSOR, (*task)->replicas(), (*task)->name().c_str(), proc_replicas, name().c_str() );
	}
    }
    
    if ( copies() != 1 && scheduling() == SCHEDULE_DELAY ) {
	getDOM()->runtime_error( LQIO::WRN_INFINITE_MULTI_SERVER, copies() );
	getDOM()->setCopies(new LQIO::DOM::ConstantExternalVariable(1.0));
    }
    return true;
}



/*
 * Denote whether this station belongs to the open, closed, or mixed
 * models when performing the MVA solution.
 */

Processor&
Processor::configure( const unsigned nSubmodels )
{
    if ( nEntries() == 0 ) {
	if ( isInteresting() || nClients() == 0 ) {
	    getDOM()->runtime_error( LQIO::WRN_NOT_USED );
	}
	return *this;
    }

    if ( copies() == 1 && scheduling() != SCHEDULE_DELAY && !Pragma::defaultProcessorScheduling() ) {
	/* Change scheduling type for uni-processors (usually from FCFS to PS) */
	getDOM()->setSchedulingType(Pragma::processorScheduling());
    }

    std::vector<Entry *>::const_iterator entry = entries().begin();
    double minS = (*entry)->serviceTime();
    double maxS = (*entry)->serviceTime();
    for ( ++entry; entry != entries().end(); ++entry ) {
	minS = std::min( minS, (*entry)->serviceTime() );
	maxS = std::max( maxS, (*entry)->serviceTime() );
    }

    /* Warn on wide service time range for FIFO scheduling. */
    if ( maxS > 0. && minS / maxS < 0.1 && !isMultiServer() && scheduling() != SCHEDULE_DELAY && scheduling() != SCHEDULE_PS ) {
	LQIO::runtime_error( ADV_SERVICE_TIME_RANGE, getDOM()->getTypeName(), name().c_str(), minS, maxS );
    }
    Entity::configure( nSubmodels );
    return *this;
}



double
Processor::rate() const
{
    double value = 1.0;
    if ( getDOM()->hasRate() ) {
	try {
	    value = getDOM()->getRateValue();
	}
	catch ( const std::domain_error& e ) {
	    getDOM()->throw_invalid_parameter( "rate", e.what() );
	}
    } 
    return value;
}



/*
 * Return the fan-in to this server from...
 */

unsigned
Processor::fanIn( const Task * aClient ) const
{
    return aClient->replicas() / replicas();
}


/*
 * Processors don't have a fanout ever 
 */

unsigned
Processor::fanOut( const Entity * aServer ) const
{
    throw LQIO::should_not_implement( "Entity::fanOut" );
    return 1;
}


/*
 * Return true if we want to keep this processor.  Interesting processors
 * will likely have queues.
 */
 
bool
Processor::isInteresting() const
{
    if ( !Pragma::prune() ) {
	return true;
    } else if ( isInfinite() ) {
	return false;
    } else {
	unsigned int sum = 0;
	for ( std::set<const Task *>::const_iterator next = tasks().begin(); next != tasks().end(); ++next ) {
	    const Task *aTask = *next;
	    if ( aTask->isInfinite() ) return true;
	    sum += aTask->copies();
	}
	return sum > copies();
    }
}


/*
 * Indicate whether the variance calculation should take place.  NOTE
 * that processors should not have the variance calculation set true,
 * so hasVariance is set in class Task rather than here.
 */

bool
Processor::hasVariance() const
{
    if ( Pragma::variance(Pragma::Variance::NONE)
	 || !Pragma::defaultProcessorScheduling()
	 || Pragma::forceMultiserver( Pragma::ForceMultiserver::PROCESSORS )
	 || scheduling() == SCHEDULE_PS
	 || isMultiServer()
	 || isInfinite() ) {
	return false;
    } else {
	return std::any_of( entries().begin(), entries().end(), std::mem_fn( &Entry::hasVariance ) );
    }
}
    

/*
 * Return true if this processor can schedule tasks with priority.
 */

bool
Processor::hasPriorities() const
{
    return scheduling() == SCHEDULE_HOL
	|| scheduling() == SCHEDULE_PPR;
}


/*
 * Return the scheduling type allowed for this object.  Overridden by
 * subclasses if the scheduling type can be something other than FIFO.
 */

bool
Processor::schedulingIsOK() const
{
    return isInfinite() && scheduling() == SCHEDULE_DELAY
	|| !isMultiServer() && ( scheduling() == SCHEDULE_HOL
				 || scheduling() == SCHEDULE_PPR )
	|| scheduling() == SCHEDULE_FIFO
	|| scheduling() == SCHEDULE_LIFO
	|| scheduling() == SCHEDULE_PS
	|| scheduling() == SCHEDULE_RAND;
}



/*
 * Return the base replica.
 */

Entity*
Processor::mapToReplica( size_t i ) const
{
    return find( name(), i );
}


/*
 * Expand replicas (Not PAN_REPLICATION).
 * Clone the processor.  Use orignal dom.  Mark as replica (through the copy constructor)
 */

Processor&
Processor::expand()
{
    const unsigned int replicas = this->replicas();
    for ( unsigned int replica = 2; replica <= replicas; ++replica ) {
	Model::__processor.insert( clone( replica ) );
    }
    return *this;
}



/*
 * Create (or recreate) a server.  If we're called a a second+ time,
 * and the station type changes, then we change the underlying
 * station.  We only return a station when we create one.
 */

Server *
Processor::makeServer( const unsigned nChains )
{
    if ( isInfinite() ) {

	/* ---------------- Infinite Servers ---------------- */

	if ( dynamic_cast<Infinite_Server *>(_station) ) return nullptr;
	_station = new Infinite_Server( nEntries(), nChains, maxPhase() );

    } else if ( isMultiServer() || Pragma::forceMultiserver( Pragma::ForceMultiserver::PROCESSORS ) ) {

	/* ---------------- Multi Servers ---------------- */

	if ( scheduling() == SCHEDULE_PS ) {

	    switch ( Pragma::multiserver() ) {
	    default:
	    case Pragma::Multiserver::DEFAULT:
	    case Pragma::Multiserver::CONWAY:
	    case Pragma::Multiserver::REISER:
	    case Pragma::Multiserver::REISER_PS:
	    case Pragma::Multiserver::SCHMIDT:
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Reiser_PS_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::ROLIA_PS:
	    case Pragma::Multiserver::ROLIA:
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(_station) && _station->mu() == copies() ) return nullptr;
		_station = new Rolia_PS_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::ZHOU:
		if ( dynamic_cast<Zhou_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		_station = new Zhou_Multi_Server( copies(), nEntries(), nChains );
		break;
	    }

	} else {

	    switch ( Pragma::multiserver() ) {
	    default:
	    case Pragma::Multiserver::DEFAULT:
		if ( copies() < 20 && nChains <= 5 ) {
		    if ( dynamic_cast<Conway_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		    _station = new Conway_Multi_Server( copies(), nEntries(), nChains );
		} else {
		    if ( dynamic_cast<Rolia_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		    _station = new Rolia_Multi_Server(  copies(), nEntries(), nChains );
		}
		break;

	    case Pragma::Multiserver::CONWAY:	/* Marginal size == copies() */
		if ( dynamic_cast<Conway_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Conway_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::REISER:
		if ( dynamic_cast<Reiser_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Reiser_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::REISER_PS:
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Reiser_PS_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::ROLIA:	/* Marginal size == 0 */
		if ( dynamic_cast<Rolia_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		_station = new Rolia_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::ROLIA_PS:
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		_station = new Rolia_PS_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::BRUELL:	/* Marginal size != copies() */
		if ( dynamic_cast<Bruell_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Bruell_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::SCHMIDT:
		if ( dynamic_cast<Schmidt_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Schmidt_Multi_Server( copies(), nEntries(), nChains );
		break;

	    case Pragma::Multiserver::SURI:
		throw LQIO::not_implemented( "Task::makeServer" );

	    case Pragma::Multiserver::ZHOU:
		if ( dynamic_cast<Zhou_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		_station = new Zhou_Multi_Server( copies(), nEntries(), nChains );
		break;
		
	    }
	}
    } else {
	switch ( scheduling() ) {
	default:
	case SCHEDULE_FIFO:
	    if ( hasVariance() ) {
		if ( dynamic_cast<HVFCFS_Server *>(_station) ) return nullptr;
		_station = new HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<FCFS_Server *>(_station) ) return nullptr;
		_station = new FCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	    break;

	case SCHEDULE_PPR:
	    if ( hasVariance() ) {
		if ( dynamic_cast<PR_HVFCFS_Server *>(_station) ) return nullptr;
		_station = new PR_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<PR_FCFS_Server *>(_station) ) return nullptr;
		_station = new PR_FCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	    break;

	case SCHEDULE_HOL:
	    if ( hasVariance() ) {
		if ( dynamic_cast<HOL_HVFCFS_Server *>(_station) ) return nullptr;
		_station = new HOL_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<HOL_FCFS_Server *>(_station) ) return nullptr;
		_station = new HOL_FCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	    break;

	case SCHEDULE_PS:
	    if ( dynamic_cast<PS_Server *>(_station) ) return nullptr;
	    _station = new PS_Server( nEntries(), nChains, maxPhase() );
	    break;

	}
    }

    return _station;
}


/*
 * Check results for sanity.
 */

const Entity&
Processor::sanityCheck() const
{
    Entity::sanityCheck();
    return *this;
}


/*
 * The superclass derives the utilization.  For processors, it's easy to get directly.
 */

double
Processor::computeUtilization( const MVASubmodel& submodel, const Server& station )
{
    return submodel.closedModelUtilization( station ) + submodel.openModelUtilization( station );
}


const Processor&
Processor::insertDOMResults(void) const
{
    if ( getDOM()== nullptr || getReplicaNumber() != 1 ) return *this;		/* NOP */

    Entity::insertDOMResults();
    
    getDOM()->setResultUtilization( std::accumulate( tasks().begin(), tasks().end(), 0.0, []( double l, const Task * r ){ return l + r->processorUtilization(); } ) );
    return *this;
}

/* -------------------------- Static methods -------------------------- */

/*
 * Add a processor to the model.
 */

void Processor::create( const std::pair<std::string,LQIO::DOM::Processor*>& p )
{
    const std::string& name = p.first;
    LQIO::DOM::Processor * dom = p.second;
    assert( name.size() > 0 );

    if ( Processor::find( name ) ) {
	dom->runtime_error( LQIO::ERR_DUPLICATE_SYMBOL );
    } else {
	Model::__processor.insert( new Processor( dom ) );
    }
}



/*
 * Find the processor and return it.
 */

Processor *
Processor::find( const std::string& name, unsigned int replica )
{
    std::set<Processor *>::const_iterator processor = std::find_if( Model::__processor.begin(), Model::__processor.end(), EqualsReplica<Processor>( name, replica ) );
    return ( processor != Model::__processor.end() ) ? *processor : nullptr;
}

/*----------------------------------------------------------------------*/
/*                               Printing                               */
/*----------------------------------------------------------------------*/

/*
 * Print out info for this processor.
 */

std::ostream&
Processor::print( std::ostream& output ) const
{
    const std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
    std::ostringstream ss;
    ss << print_name();
    output << std::setw(10) << ss.str()
	   << " " << std::setw(15) << print_info()
	   << " " << std::setw(9)  << print_type(); 
    output << " " << std::setw(10) << "--"
	   << " " << std::setw(3)  << "--";
    output << " " << print_entries();
    output.flags(oldFlags);
    return output;
}



/*
 * Print out who calls this processor (by submodel if submodel != 0 )
 */

std::ostream&
Processor::printTasks( std::ostream& output, unsigned int submodel ) const
{
    if ( submodel != 0 && this->submodel() != submodel ) return output;
    for ( std::set<const Task *>::const_iterator task = _tasks.begin(); task != _tasks.end(); ++task ) {
	if ( (isReplica() && (*task)->isReplica()) ) continue;
	output << std::setw(2) << " " << (*task)->print_name() << " *> " << print_name() << std::endl;
    }
    return output;
}

