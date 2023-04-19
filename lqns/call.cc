/*  -*- c++ -*-
 * $Id: call.cc 16676 2023-04-19 11:56:50Z greg $
 *
 * Everything you wanted to know about a call to an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#define BUG_433 1

#include "lqns.h"
#include <cmath>
#include <limits>
#include <functional>
#include <sstream>
#include <mva/server.h>
#include "activity.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "flags.h"
#include "option.h"
#include "pragma.h"
#include "submodel.h"
#include "task.h"

/*----------------------------------------------------------------------*/
/*      Input processing.  Called from model.cc::prepareModel()         */
/*----------------------------------------------------------------------*/

void
Call::Create::operator()( const LQIO::DOM::Call * call )
{
    _src->add_call( _p, call );
}


std::set<Task *>& Call::add_client( std::set<Task *>& clients, const Call * call )
{
    if ( !call->hasForwarding() && call->srcTask()->isUsed() ) {
	clients.insert(const_cast<Task *>(call->srcTask()));
    }
    return clients;
}

std::set<Entity *>& Call::add_server( std::set<Entity *>& servers, const Call * call )
{
    servers.insert(const_cast<Entity *>(call->dstTask()));
    return servers;
}

/*----------------------------------------------------------------------*/
/*                            Static methods                            */
/*----------------------------------------------------------------------*/

/*
 * Ignore replicas because it's handled by fanOut.
 */

/* static */ double
Call::add_rendezvous( double sum, const Call * call )
{
    if ( Pragma::pan_replication() ) {
	return sum + call->rendezvous() * call->fanOut();
    } else {
	return sum + call->rendezvous();
    }
}

/*
 * Ignore replicas because it's handled by fanOut.
 */

/* static */ double
Call::add_rendezvous_no_fwd( double sum, const Call * call )
{
    if ( call->isForwardedCall() ) {
	return sum;
    } else {
	return Call::add_rendezvous( sum, call );
    }
}

/*----------------------------------------------------------------------*/
/*                            Generic  Calls                            */
/*----------------------------------------------------------------------*/

/*
 * Initialize and zero fields.   Reverse links are set here.  Forward
 * links are done by subclass.  Processor calls are linked specially.
 */

Call::Call( const Phase * fromPhase, const Entry * toEntry )
    : _dom(nullptr),
      _source(fromPhase),
      _destination(toEntry), 
      _chainNumber(0),
      _wait(0.0)
{
    if ( toEntry != nullptr ) {
	const_cast<Entry *>(_destination)->addDstCall( this );	/* Set reverse link	*/
    }
}


Call::Call( const Call& src )
    : _dom(src._dom),
      _source(nullptr),
      _destination(nullptr),
      _chainNumber(src._chainNumber),
      _wait(src._wait)
{
}

	   
/*
 * Clean up the mess.
 */

Call::~Call()
{
}


int
Call::operator==( const Call& item ) const
{
    return (dstEntry() == item.dstEntry());
}


int
Call::operator!=( const Call& item ) const
{
    return (dstEntry() != item.dstEntry());
}


/*+ BUG_425 */
/*
 * Set the number of customers by chain (reference task/open arrival).
 * Note! DO NOT follow forwarding calls as the transformation process
 * maps these to regular rendezvous from the proper source task.
 * Forwarding is effectively an infinite server, so we need the
 * source.
 */

Call&
Call::initCustomers( std::deque<const Task *>& stack, unsigned int customers )
{
    Task * task = const_cast<Task *>(dynamic_cast<const Task *>(dstTask()));
#if BUG_425
    std::cerr << std::setw( stack.size() * 2 ) << " " << "    Call::initCustomers(" << stack.size() << "," << customers << ") -> ";
    if ( task != nullptr ) std::cerr << task->print_name();
    std::cerr << std::endl;
#endif
    if ( task == nullptr ) return *this;	/* Don't care about processors */
    if ( hasRendezvous() ) {
	task->initCustomers( stack, customers );
    } else if ( hasSendNoReply() ) {
	/* Asynchronous send, so this is effectively an open arrival */
	std::deque<const Task *> stack2;
	task->initCustomers( stack2, std::numeric_limits<unsigned int>::max() );
    } // Ignore hasForwarding().
    return *this;
}
/*- BUG_425 */



/*+ BUG_433
 * Expand replicas (Not PAN_REPLICATION) from Call(src,dst) to
 * Call(src(src_replica),dst(dst_replica)).  There is no need to copy
 * src_replica=1 to dst_replica=1.  The real guts are in the copy
 * constructors.
 */

Call&
Call::expand()
{
    unsigned int next_replica = 1;
    const unsigned int src_replicas = srcTask()->replicas();
    for ( unsigned int src_replica = 1; src_replica <= src_replicas; ++src_replica ) {

	const unsigned int dst_replicas = dstEntry()->owner()->replicas();
	const unsigned int fan_out = fanOut();
	for ( unsigned int k = 1; k <= fan_out; k++ ) {

	    /* divide the destination entries equally between calling entries. */
	    const unsigned int dst_replica = (next_replica++ - 1) % dst_replicas + 1;
	    if ( src_replica == 1 && dst_replica == 1 ) continue;	/* This exists already */

	    Call * call = clone( src_replica, dst_replica );		/* 2 goes to 2, etc */
	    const_cast<Entity *>(call->dstTask())->addTask( call->srcTask() );
	}
    }
    return *this;
}
/*- BUG_433 */


bool
Call::check() const
{
    const int srcReplicas = srcTask()->replicas();
    const int dstReplicas = dstTask()->replicas();
    if ( srcReplicas > 1 || dstReplicas > 1 ) {
	const int fanOut = srcTask()->fanOut( dstTask() );
	const int fanIn  = dstTask()->fanIn( srcTask() );
	if ( fanIn == 0 || fanOut == 0 || srcReplicas * fanOut != dstReplicas * fanIn ) {
	    const std::string& srcName = srcTask()->name();
	    const std::string& dstName = dstTask()->name();
	    LQIO::runtime_error( LQIO::ERR_REPLICATION, 
				 fanOut, srcName.c_str(), srcReplicas,
				 fanIn,  dstName.c_str(), dstReplicas );
	    return false;
	}
    }
    return true;
}


/*
 * Common code for rendezvous, send-no-reply and forwarding.  Get and
 * check the value returned.  Additional checks for valid forwarding
 * probabilities, and a deterministic number of calls.
 */

double
Call::getDOMValue() const
{
    try {
	const double value = getDOM()->getCallMeanValue();
	if ( getDOM()->getCallType() != LQIO::DOM::Call::Type::FORWARD && getSource()->phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC && value != std::rint( value ) ) {
	    throw std::domain_error( "invalid integer" );
	} else if ( getDOM()->getCallType() == LQIO::DOM::Call::Type::FORWARD && value > 1.0 ) {
	    throw std::domain_error( "invalid probability" );
	}
	return value;
    }
    catch ( const std::domain_error &e ) {
	getDOM()->throw_invalid_parameter( "mean value", e.what() );
    }
    return 0.;
}



unsigned
Call::fanIn() const
{
    return dstTask()->fanIn( srcTask() );
}


unsigned
Call::fanOut() const
{
    return srcTask()->fanOut( dstTask() );
}


/*
 * Return the name of the destination entry.
 */

const std::string&
Call::dstName() const
{
    return dstEntry()->name();
}



/*
 * Return the submodel number.
 */

unsigned
Call::submodel() const
{
    return dstTask()->submodel();
}


/*
 * Return if this call matches type (for Entry::CallInfo)
 */

bool
Call::hasTypeForCallInfo( LQIO::DOM::Call::Type type ) const
{
    switch ( type ) {
    case LQIO::DOM::Call::Type::SEND_NO_REPLY: return hasSendNoReply();
    case LQIO::DOM::Call::Type::FORWARD: return isForwardedCall();
    case LQIO::DOM::Call::Type::RENDEZVOUS: return hasRendezvous() && !isForwardedCall();
    default: abort();
    }
    return false;
}
		

bool
Call::hasOvertaking() const
{
    return hasRendezvous() && dstEntry()->maxPhase() > 1;
}



/*
 * Return the total wait along this arc.  Take into account
 * replication.  This applies for both Pan and Bug 299 replication.
 */

double
Call::rendezvousDelay() const
{
    if ( !hasRendezvous() ) {
	return 0.0;
    } else if ( Pragma::pan_replication() ) {
	return rendezvous() * wait() * fanOut();
    } else {
	return rendezvous() * wait();
    }
}


#if PAN_REPLICATION
/*
 * Compute and save old rendezvous delay.		// REPL
 */

double
Call::rendezvousDelay( const unsigned k )
{
    if ( dstTask()->hasServerChain(k) ) {
	return rendezvous() * wait() * (fanOut() - 1);
    } else {
	return Call::rendezvousDelay();	// rendezvousDelay is already multiplied by fanOut.
    }
}
#endif


/*
 * Return the name of the source of this call.  Sources are always tasks.
 */

const std::string&
Call::srcName() const
{
    return getSource()->name();
}


/*
 * Return the source task of this call.  Sources are always tasks.
 */

const Task *
Call::srcTask() const
{
    return dynamic_cast<const Task *>(getSource()->owner());
}



double
Call::elapsedTime() const
{
    if (flags.trace_quorum) {
	std::cout <<"\nCall::elapsedTime(): call " << this->srcName() << " to " << dstEntry()->name() << std::endl;
    }

    if ( hasRendezvous() ) {
	return dstEntry()->elapsedTimeForPhase(1);
    } else {
	return 0.0;
    }
}



/*
 * Return time spent in the queue for call to this entry.
 */

double
Call::queueingTime() const
{
    if ( hasRendezvous() ) {
	if ( std::isinf( wait() ) ) return wait();
	const double q = wait() - elapsedTime();
	if ( q <= 0.000001 ) {
	    return 0.0;
	} else if ( q * elapsedTime() > 0. && (q/elapsedTime()) <= 0.0001 ) {
	    return 0.0;
	} else {
	    return q;
	}
    } else if ( hasSendNoReply() ) {
	return wait();
    } else {
	return 0.0;
    }
}


/*
 * Return the probability of dropping messages.
 */

double
Call::dropProbability() const
{
    double src_tput = getSource()->throughput();
    if ( !hasSendNoReply() || !std::isfinite(src_tput) || src_tput == 0.0) return 0.0;

    src_tput *= sendNoReply();
    const double dst_tput = dstEntry()->throughput();
    if ( src_tput > dst_tput ) {
	return 1.0 - dst_tput / src_tput;
    } else {
	return 0.0;
    }
}


const Call&
Call::insertDOMResults() const
{
    if ( getSource()->getReplicaNumber() != 1 ) return *this;		/* NOP */

    const_cast<LQIO::DOM::Call *>(getDOM())->setResultWaitingTime(queueingTime());
    const double drop_probability = dropProbability();
    if ( drop_probability > 0. ) {
	const_cast<LQIO::DOM::Call *>(getDOM())->setResultDropProbability( drop_probability );
    }
    return *this;
}


#if PAN_REPLICATION
/*
 * Return the adjustment factor for this call.  //REPL
 */

double
Call::nrFactor( const Submodel& aSubmodel, const unsigned k ) const
{
    const Entity * dst_task = dstTask();
    return aSubmodel.nrFactor( dst_task->serverStation(), index(), k ) * fanOut() * rendezvous();	// 5.20
}
#endif



/*
 * Return variance of this arc.
 */

double
Call::variance() const
{
    if ( hasRendezvous() ) {
	return dstEntry()->varianceForPhase(1) + square(queueingTime());
    } else {
	return 0.0;
    }
}



/*
 * Return the coefficient of variation for this particular call.
 */

double
Call::CV_sqr() const
{
#ifdef NOTDEF
    return dstEntry()->variance(1) / square(elapsedTime());
#endif
    return variance() / square(wait());
}



/*
 * Follow the call to its destination.
 */

const Call&
Call::followInterlock( Interlock::CollectTable& path ) const
{
    /* Mark current */

    if ( rendezvous() > 0.0 && !path.prune() ) {
	Interlock::CollectTable branch( path, path.calls() * rendezvous() );
	const_cast<Entry *>(dstEntry())->initInterlock( branch );
    }
    return *this;
}

/* ----------------------------- Save Results ----------------------------- */

/*
 * Call::Perform is invoked from task by submodel and interfaces to the MVA
 * Solver.
 */

void Call::Perform::operator()( Call * call ) 
{
    if ( call->submodel() != submodel() ) return;
    (this->*f())( *call );
}


unsigned int 
Call::Perform::submodel() const
{
    return _submodel.number();
}



bool
Call::Perform::hasServer( const Entity * server ) const
{
    return _submodel.hasServer( server );
}



/*
 * Set the visit ratio at the destination's station.  Called from
 * Task::initClientStation only.  The calling task is mapped to the base
 * replica if necessary.
 */

void
Call::Perform::setVisits( Call& call )
{
    const Entity * server = call.dstTask();
    if ( server->hasServerChain( k() ) && call.hasRendezvous() && !call.srcTask()->hasInfinitePopulation() ) {
	Server * station = server->serverStation();
	const unsigned e = call.dstEntry()->index();
	station->addVisits( e, k(), p(), call.rendezvous() * rate() );
    }
}


/*
 * Set the open arrival rate to the destination's station.  Called from
 * Task::initClientStation only.  The calling task is mapped to the base
 * replica if necessary.
 */

void
Call::Perform::setLambda( Call& call )
{
    Server * station = call.dstTask()->serverStation();
    const unsigned e = call.dstEntry()->index();
    if ( call.hasSendNoReply() ) {
	station->addVisits( e, 0, p(), call.getSource()->throughput() * call.sendNoReply() );
    } else if ( call.hasRendezvous() && call.srcTask()->isOpenModelServer() && call.srcTask()->isInfinite() ) {
	station->addVisits( e, 0, p(), call.getSource()->throughput() * call.rendezvous() );
    }
}



//tomari: set the chain number associated with this call.
void
Call::Perform::setChain( Call& call )
{
    const Entity * server = call.dstTask();
    if ( server->hasServerChain( k() )  ){

	call._chainNumber = k();

	if ( flags.trace_replication ) {
	    std::cout <<"\nCall::setChain, k=" << k() << "  " ;
	    std::cout <<",call from "<< call.srcName() << " To " << call.dstName()<< std::endl;
	}
    }
}


/*
 * Initialize waiting time.
 */

void
Call::Perform::initWait( Call& call )
{
    double time = 0.0;
    if ( call.isProcessorCall() ) {
	time = call.dstEntry()->serviceTimeForPhase(1);
    } else {
	time = call.elapsedTime();
    }
    call.setWait( time );			/* Initialize arc wait. 	*/
}



/*
 * Get the waiting time for this call from the mva submodel.  A call can
 * potentially orginate from multiple chains, so add them all up.  (Call
 * clearWait first.)
 */

void
Call::Perform::saveOpen( Call& call )
{
    const Entity * server = call.dstTask();		// BUG_433 -- remap to base?
    const unsigned e = call.dstEntry()->index();

#define DEBUG 0
    /*+ BUG_433 */
    if ( !hasServer( server ) ) {		// Need to check for server in submodel. Need code in client? This might work as-is. */
#if DEBUG
	std::cerr << "Call::Perform::saveOpenWait(submodel=" << _submodel.number() << ") server=" << server->print_name() << " pruned." << std::endl;
#endif
	server = server->mapToReplica( 1 );	// Map to base replica.
    }
    /*- BUG_433 */

    const Server * station = server->serverStation();
    if ( station->V( e, 0, p() ) > 0.0 ) {
	call.setWait( station->W[e][0][p()] );
    }
}



/*
 * Get the waiting time for this call from the mva submodel.  A call can
 * potentially orginate from multiple chains, so add them all up.  (Call
 * clearWait first.)  This may have to be changed if the result varies by
 * chain.  Priorities perhaps?
 */

void
Call::Perform::saveWait( Call& call )
{
    const Entity * server = call.dstTask();		// BUG_433 -- remap to base?
    const unsigned e = call.dstEntry()->index();

    /*+ BUG_433 */
    if ( !hasServer( server ) ) {		// Need to check for server in submodel. Need code in client? This might work as-is. */
#if DEBUG
	std::cerr << "Call::Perform::saveWait(submodel=" << _submodel.number() << ") server=" << server->print_name() << " pruned." << std::endl;
#endif
	server = server->mapToReplica( 1 );	// Map to base replica.
    }
    /*- BUG_433 */

    const Server * station = server->serverStation();
    if ( station->V( e, k(), p() ) > 0.0 ) {
	call.setWait( station->W[e][k()][p()] );
    }
}

/*----------------------------------------------------------------------*/
/*                              Phase Calls                             */
/*----------------------------------------------------------------------*/

/*
 * Call from a phase to a task.
 */

PhaseCall::PhaseCall( const Phase * fromPhase, const Entry * toEntry )
    : Call( fromPhase, toEntry ),
      FromEntry( fromPhase->entry() )
{
    const_cast<Phase *>(getSource())->addSrcCall( this );
}


/*+ BUG_433
 * Deep copy.  Set FromEntry to the replica.
 */

PhaseCall::PhaseCall( const PhaseCall& src, unsigned int src_replica, unsigned int dst_replica )
    : Call( src ),		/* link to DOM.	*/
      FromEntry( Entry::find( src.srcEntry()->name(), src_replica ) )
{
    /* Link to source replica */
    const Phase& src_phase = srcEntry()->getPhase(src.getSource()->getPhaseNumber());
    setSource( &src_phase );    /* Swap _source to the replica !!! */
    const_cast<Phase&>(src_phase).addSrcCall( this );

    /* Link to destination replica */
    if ( src.dstEntry() != nullptr ) {
	Entry * dst = Entry::find( src.dstEntry()->name(), dst_replica );
	if ( dst == nullptr ) {
	    std::ostringstream err;
	    err << "PhaseCall::PhaseCall: Can't find entry " << src.dstEntry()->name() << "." << dst_replica;
	    throw std::runtime_error( err.str() );
	}
	setDestination( dst );
	dst->addDstCall( this );	/* Set reverse link */
    }
    if ( Options::Debug::replication() ) {
	std::cerr << "PhaseCall::PhaseCall(\"" << getSource()->name() << "." << srcEntry()->getReplicaNumber() << "\"," << src_replica << "," << dst_replica << ")"
		  << " link to entry: " << dstEntry()->name() << "." << dstEntry()->getReplicaNumber() << std::endl;
    }
}
/*- BUG_433 */

/*----------------------------------------------------------------------*/
/*                           Forwarded Calls                            */
/*----------------------------------------------------------------------*/

/*
 * call added to transform forwarding to standard model.
 */

ForwardedCall::ForwardedCall( const Phase * fromPhase, const Entry * toEntry, const Call * fwdCall )
    : Call( fromPhase, toEntry ), PhaseCall( fromPhase, toEntry ), _fwdCall( fwdCall )
{
}

bool
ForwardedCall::check() const
{
    const Task * srcTask = dynamic_cast<const Task *>(_fwdCall->getSource()->owner());
    const int srcReplicas = srcTask->replicas();
    const int dstReplicas = dstTask()->replicas();
    if ( srcReplicas > 1 || dstReplicas > 1 ) {
	const int fanOut = srcTask->fanOut( dstTask() );
	const int fanIn  = dstTask()->fanIn( srcTask );
	if ( fanIn == 0 || fanOut == 0 || srcReplicas * fanOut != dstReplicas * fanIn ) {
	    const std::string& srcName = srcTask->name();
	    const std::string& dstName = dstTask()->name();
	    LQIO::runtime_error( LQIO::ERR_REPLICATION, 
				 fanOut, srcName.c_str(), srcReplicas,
				 fanIn,  dstName.c_str(), dstReplicas );
	    return false;
	}
    }
    return true;
}

const std::string&
ForwardedCall::srcName() const
{
    return _fwdCall->srcName();
}


const ForwardedCall&
ForwardedCall::insertDOMResults() const
{
    if ( getSource()->getReplicaNumber() != 1 ) return *this;		/* NOP */

    LQIO::DOM::Call* fwdDOM = const_cast<LQIO::DOM::Call *>(_fwdCall->getDOM());		/* Proxy */
    fwdDOM->setResultWaitingTime(queueingTime());
    return *this;
}

/*----------------------------------------------------------------------*/
/*                           Processor Calls                            */
/*----------------------------------------------------------------------*/

/*
 * Call to processor entry.
 */

ProcessorCall::ProcessorCall( const Phase * fromPhase, const Entry * toEntry )
    : Call( fromPhase, toEntry )
{
}

/*----------------------------------------------------------------------*/
/*                            Activity Calls                            */
/*----------------------------------------------------------------------*/

/*
 * Call added for activities.
 */

ActivityCall::ActivityCall( const Activity * fromActivity, const Entry * toEntry )
    : Call( fromActivity, toEntry ), FromActivity()
{
    const_cast<Phase *>(getSource())->addSrcCall( this );
}


ActivityCall::ActivityCall( const ActivityCall& src, unsigned int src_replica, unsigned int dst_replica )
    : Call( src ), FromActivity()
{
    const Task * task = Task::find( src.srcTask()->name(), src_replica );
    const Activity * src_activity = task->findActivity( src.getSource()->name() );
    setSource( src_activity );
    const_cast<Activity*>(src_activity)->addSrcCall( this );
    
    /* Link to destination replica */
    if ( src.dstEntry() != nullptr ) {
	Entry * dst = Entry::find( src.dstEntry()->name(), dst_replica );	// BUG_433
	if ( dst == nullptr ) {
	    std::ostringstream err;
	    err << "ActivityCall::ActivityCall: Can't find entry " << src.dstEntry()->name() << "." << dst_replica;
	    throw std::runtime_error( err.str() );
	}
	setDestination( dst );
	dst->addDstCall( this );	/* Set reverse link */
    }
    if ( Options::Debug::replication() ) {
	std::cerr << "ActivityCall::ActivityCall() from: " << getSource()->name() << "(" << task->getReplicaNumber() << ")"
		  << " to: " << dstEntry()->name() << "(" << dstEntry()->getReplicaNumber() << ")" << std::endl;
    }
}

/*----------------------------------------------------------------------*/
/*                       Activity Forwarded Calls                       */
/*----------------------------------------------------------------------*/

/*
 * call added to transform forwarding to standard model.
 */

ActivityForwardedCall::ActivityForwardedCall( const Activity * fromActivity, const Entry * toEntry )
    : Call( fromActivity, toEntry ), ActivityCall( fromActivity, toEntry )
{
}

/*----------------------------------------------------------------------*/
/*                        Phase Processor Calls                         */
/*----------------------------------------------------------------------*/

PhaseProcessorCall::PhaseProcessorCall( const Phase * fromPhase, const Entry * toEntry ) 
    : Call( fromPhase, toEntry ), ProcessorCall( fromPhase, toEntry ), FromEntry( fromPhase->entry() )
{
}

/*----------------------------------------------------------------------*/
/*                      Activity Processor Calls                        */
/*----------------------------------------------------------------------*/

ActivityProcessorCall::ActivityProcessorCall( const Activity * fromActivity, const Entry * toEntry )
    : Call( fromActivity, toEntry ), ProcessorCall( fromActivity, toEntry ), FromActivity()
{
}

/*----------------------------------------------------------------------*/
/*                          CallStack Functions                         */
/*----------------------------------------------------------------------*/

/*
 * We are looking for matching tasks for calls.
 */

bool 
Call::Find::operator()( const Call * call ) const
{
    if ( call == nullptr || call->getDOM() == nullptr ) return false;

    if ( call->hasSendNoReply() ) _broken = true;		/* Cycle broken - async call */
    if ( call->dstTask() == _call->dstTask() ) {
	if ( call->hasRendezvous() && _call->hasRendezvous() && !_broken ) {
	    throw call_cycle();		/* Dead lock */
	} else if ( call->dstEntry() == _call->dstEntry() && _direct_path ) {
	    throw call_cycle();		/* Livelock */
	} else {
	    return true;
	}
    }
    return false;
}

/*
 * We may skip back over forwarded calls when computing the size.
 */

unsigned
Call::stack::depth() const	
{
    return std::count_if( begin(), end(), std::mem_fn( &Call::hasNoForwarding ) );
}
