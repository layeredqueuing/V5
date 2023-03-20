/*  -*- c++ -*-
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January 2003
 *
 * ------------------------------------------------------------------------
 * $Id: entry.cc 16557 2023-03-20 10:21:04Z greg $
 * ------------------------------------------------------------------------
 */

#include "lqn2ps.h"
#include <algorithm>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <lqio/error.h>
#include <lqio/dom_extvar.h>
#include <lqio/bcmp_to_lqn.h>
#include <lqio/dom_document.h>
#include "activity.h"
#include "arc.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "label.h"
#include "model.h"
#include "processor.h"
#include "task.h"

std::set<Entry *,LT<Entry> > Entry::__entries;
std::map<std::string,unsigned> Entry::__key_table;		/* For squishName 	*/
std::map<std::string,std::string> Entry::__symbol_table;	/* For rename		*/

unsigned Entry::max_phases		= 0;

const char * Entry::phaseTypeFlagStr [] = { "Stochastic", "Determin" };

/* ------------------------ Constructors etc. ------------------------- */

Entry::Entry( const LQIO::DOM::DocumentObject * dom )
    : Element( dom, __entries.size()+1 ),		// Give this entry a unique id
      drawLeft(true),
      drawRight(true),
      _phases(),
      _owner(0),
      _requestType(request_type::NOT_CALLED),
      _calls(),
      _callers(),
      _startActivity(nullptr),
      _activityCall(nullptr),
      _activityCallers()
{
    /* Allocate phases */

    const LQIO::DOM::Entry * entryDOM  = dynamic_cast<const LQIO::DOM::Entry *>(dom);
    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	if ( entryDOM->hasPhase(p) ) {
	    _phases[p].setDOM(const_cast<LQIO::DOM::Entry *>(entryDOM)->getPhase(p));
	    _phases[p].initialize( this, p );
	}
    }
    _node = Node::newNode( Flags::entry_width, Flags::entry_height );
    _label = Label::newLabel();
}


/*
 * Copies everything except phase information.  See Entry::clone()
 */

Entry::Entry( const Entry& src )
    : Element( dynamic_cast<const LQIO::DOM::Entry *>(src.getDOM())->clone(), __entries.size()+1 ),
      drawLeft(src.drawLeft),
      drawRight(src.drawRight),
      _phases(),
      _owner(0),
      _requestType(src._requestType),
      _calls(),
      _callers(),
      _startActivity(nullptr),
      _activityCall(nullptr),
      _activityCallers()
{
    _node = Node::newNode( Flags::entry_width, Flags::entry_height );
    _label = Label::newLabel();
}




/*
 * Compare entry names for equality.
 */

int
Entry::operator==( const Entry& anEntry ) const
{
    return name() == anEntry.name();
}


/*
 * Reset globals.
 */

void
Entry::reset()
{
    max_phases = 0;
}


const Phase&
Entry::getPhase( unsigned p ) const
{
    const std::map<unsigned,Phase>::const_iterator i = _phases.find(p);
    assert( i != _phases.end() );
    return i->second;
}

/*
 * Create a phase if necesary (i.e., called as NON-const!).
 * A little complicated as we have to initialze the links if the phase
 * is not found.
 */

Phase&
Entry::getPhase( unsigned p ) 
{
    const std::pair<std::map<unsigned,Phase>::iterator,bool> i = _phases.insert( std::pair<unsigned,Phase>(p,Phase()) );
    std::map<unsigned,Phase>::iterator j = i.first;
    Phase& phase = j->second;
    if ( i.second ) {
	phase.initialize( this, p );
    }
    return phase;
}


/*
 * Free storage allocated in wait.  Storage was allocated by layerize.c
 * by calling configure.
 */

Entry::~Entry()
{
    /* Release forward links */

    std::for_each( _calls.begin(), _calls.end(), Delete<Call *> );
    delete _node;
    delete _label;

    if ( _activityCall ) {
	delete _activityCall;
    }
}

Entry *
Entry::clone( unsigned int replica ) const 
{
    std::ostringstream aName;
    aName << name() << "_" << replica;
    std::set<Entry *>::const_iterator nextEntry = find_if( __entries.begin(), __entries.end(), EQStr<Entry>( aName.str() ) );
    if ( nextEntry != __entries.end() ) {
	const std::string msg = "Entry::clone(): cannot add symbol " + aName.str();
	throw std::runtime_error( msg );
    }

    Entry * new_entry = new Entry( *this );
    new_entry->setName( aName.str() );
    LQIO::DOM::Entry * new_entry_dom  = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(new_entry->getDOM()));
    for ( std::map<unsigned,Phase>::const_iterator i = _phases.begin(); i != _phases.end(); ++i ) {
	const unsigned int p = i->first;
	if ( !dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->hasPhase(p) ) continue;
	Phase& new_phase = new_entry->getPhase(p);
	LQIO::DOM::Phase * new_phase_dom = new LQIO::DOM::Phase( *i->second.getDOM() );
	new_phase.setDOM( new_phase_dom );	/* Deep copy */
	new_entry_dom->setPhase( p, new_phase_dom );
    }

    return new_entry;
}

/* ------------------------ Instance Methods -------------------------- */

bool
Entry::hasPriority() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    return dom && dom->getEntryPriority();
}



const LQIO::DOM::ExternalVariable&
Entry::priority() const
{
    return *dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getEntryPriority();
}



/*
 * Set the max service time value.
 */

Entry&
Entry::histogram( const unsigned p, const double min, const double max, const unsigned n_bins )
{
    _phases[p].histogram( min, max, n_bins );
    return *this;
}


Entry&
Entry::histogramBin(  const unsigned p, const double begin, const double end, const double prob, const double conf95, const double conf99 )
{
    _phases[p].histogramBin( begin, end, prob, conf95, conf99 );
    return *this;
}


const LQIO::DOM::ExternalVariable &
Entry::Cv_sqr( const unsigned p ) const
{
    return getPhase(p).Cv_sqr();
}



/*
 * Return a reference so the operators +,-,*,/,<< work.
 */

const LQIO::DOM::ExternalVariable&
Entry::openArrivalRate() const
{
    return *dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getOpenArrivalRate();
}



void
Entry::addCall( const unsigned int p, LQIO::DOM::Call* domCall )
{
    /* Begin by extracting the from/to DOM entries from the call and their names */

    assert( 0 < p && p <= MAX_PHASES );
    LQIO::DOM::Entry* toDOMEntry = const_cast<LQIO::DOM::Entry*>(domCall->getDestinationEntry());
    const char* to_entry_name = toDOMEntry->getName().c_str();

    /* Make sure this is one of the supported call types */
    if (domCall->getCallType() != LQIO::DOM::Call::Type::SEND_NO_REPLY &&
	domCall->getCallType() != LQIO::DOM::Call::Type::RENDEZVOUS &&
	domCall->getCallType() != LQIO::DOM::Call::Type::NULL_CALL) {
	abort();
    }

    /* Internal Entry references */
    Entry * toEntry = Entry::find( to_entry_name );
    if ( !toEntry ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, to_entry_name );
    } else if ( this == toEntry ) {
	LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, name().c_str(), to_entry_name );
    } else if ( domCall->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS) {
	rendezvous( toEntry, p, domCall );
    } else if ( domCall->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY ) {
	sendNoReply( toEntry, p, domCall );
    }
}



/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Entry&
Entry::rendezvous( const Entry * toEntry, unsigned int p, const LQIO::DOM::Call * value )
{
    if ( value && const_cast<Entry *>(toEntry)->isCalledBy( request_type::RENDEZVOUS ) ) {
 	Model::rendezvousCount[0] += 1;
	Model::rendezvousCount[p] += 1;
	getPhase( p );
	Call * aCall = findOrAddCall( toEntry, &GenericCall::hasRendezvousOrNone );
	aCall->rendezvous( p, value );
    }

    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

const LQIO::DOM::ExternalVariable &
Entry::rendezvous( const Entry * anEntry, const unsigned p ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasRendezvous );
    if ( aCall ) {
	return aCall->rendezvous(p);
    } else {
	abort();
    }
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

double
Entry::rendezvous( const Entry * anEntry ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasRendezvous  );
    if ( aCall ) {
	return Entity::to_double( aCall->sumOfRendezvous() );
    } else {
	return 0.0;
    }
}



/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Entry&
Entry::sendNoReply( const Entry * toEntry, unsigned int p, const LQIO::DOM::Call * value )
{
    if ( value  && const_cast<Entry *>(toEntry)->isCalledBy( request_type::SEND_NO_REPLY ) ) {
	Model::sendNoReplyCount[0] += 1;
	Model::sendNoReplyCount[p] += 1;
	getPhase( p );
	Call * aCall = findOrAddCall( toEntry, &GenericCall::hasSendNoReplyOrNone );
	aCall->sendNoReply( p, value );
    }
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

const LQIO::DOM::ExternalVariable &
Entry::sendNoReply( const Entry * anEntry, const unsigned p ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasSendNoReply );
    if ( aCall ) {
	return aCall->sendNoReply(p);
    } else {
	abort();
    }
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

double
Entry::sendNoReply( const Entry * anEntry ) const
{
    const Call * aCall = findCall( anEntry, &GenericCall::hasSendNoReply  );
    if ( aCall ) {
	return aCall->sumOfSendNoReply();
    } else {
	return 0.0;
    }
}



/*
 * Retrieve forwarding probability to entry.
 */

const LQIO::DOM::ExternalVariable &
Entry::forward( const Entry * toEntry ) const
{
    const Call * aCall = findCall( toEntry, &GenericCall::hasForwarding );

    if ( aCall ) {
	return aCall->forward();
    } else {
	abort();
    }
}




Entry&
Entry::forward( const Entry * toEntry, const LQIO::DOM::Call * value )
{
    if ( value && const_cast<Entry *>(toEntry)->isCalledBy( request_type::RENDEZVOUS ) ) {
	Model::forwardingCount += 1;
	getPhase( 1 );
	Call * aCall = findOrAddCall( toEntry, &GenericCall::hasForwardingOrNone );
	aCall->forward( value );
    }
    return *this;
}




/*
 * Return true if the entry forwards to aTask.
 */

bool
Entry::forwardsTo( const Task * toTask ) const
{
    const Call * aCall = findCall( toTask );

    if ( aCall ) {
	return aCall->hasForwardingLevel();
    } else {
	return false;
    }
}


/*
 * Return fan in.
 */

unsigned
Entry::fanIn( const Entry * toEntry ) const
{
    Call * aCall = findCall( toEntry );
    if ( aCall ) {
	return aCall->fanIn();
    } else {
	return 0;
    }
}



/*
 * Return fan out.
 */

unsigned
Entry::fanOut( const Entry * toEntry ) const
{
    Call * aCall = findCall( toEntry );
    if ( aCall ) {
	return aCall->fanOut();
    } else {
	return 0;
    }
}



/*
 * Set starting activity for this entry.
 */

Entry&
Entry::setStartActivity( Activity * anActivity )
{
    _startActivity = anActivity;
    if ( _activityCall ) delete _activityCall;
    _activityCall = Arc::newArc();
    anActivity->rootEntry( this, _activityCall );
    return *this;
}


Entry&
Entry::setPhaseDOM( unsigned p, const LQIO::DOM::Phase* phase )
{
    _phases[p].setDOM( phase );
    return *this;
}

const LQIO::DOM::Phase *
Entry::getPhaseDOM( unsigned p ) const
{
    const std::map<unsigned,Phase>::const_iterator i = _phases.find(p);
    return i != _phases.end() ? i->second.getDOM() : nullptr;
}


void
Entry::removeDstCall( GenericCall * aCall)
{
    std::vector<GenericCall *>::iterator pos = std::find( _callers.begin(), _callers.end(), aCall );
    if ( pos != _callers.end() ) {
	_callers.erase( pos );
    }
}


void
Entry::removeSrcCall( Call * aCall )
{
    std::vector<Call *>::iterator pos = std::find( _calls.begin(), _calls.end(), aCall ) ;
    if ( pos != _calls.end() ) {
	_calls.erase( pos );
    }
}

void
Entry::deleteActivityReplyArc( Reply * aReply )
{
    std::vector<Reply *>::iterator pos = find_if( _activityCallers.begin(), _activityCallers.end(), EQ<GenericCall>( aReply ) );
    if ( pos != _activityCallers.end() ) {
	_activityCallers.erase( pos );
    }
}


/* -------------------------- Result Queries -------------------------- */

double Entry::openWait() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    return dom->getResultWaitingTime();
}
/* --- */

double Entry::processorUtilization() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    return dom->getResultProcessorUtilization();
}

/* --- */

double
Entry::throughput() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    return dom->getResultThroughput();
}

/* --- */

double
Entry::throughputBound() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    return dom->getResultThroughputBound();
}


double
Entry::utilization( const unsigned p ) const
{
    return getPhase(p).utilization();
}


double
Entry::utilization() const
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    return dom->getResultUtilization();
}

/*
 * Set the value of calls to entry `toEntry', `phase'.
 */

Call *
Entry::forwardingRendezvous( Entry * toEntry, const unsigned p, const double value )
{
    if ( value > 0.0 && toEntry->isCalledBy( request_type::RENDEZVOUS ) ) {
	ProxyEntryCall * aCall = findOrAddFwdCall( toEntry );
	const LQIO::DOM::Call * dom = aCall->getDOM(p);
	if ( dom ) {
	    /* Reset the value in the old call */
	    LQIO::DOM::ExternalVariable * mean = const_cast<LQIO::DOM::ExternalVariable *>(dom->getCallMean());
	    mean->set(value);
	} else {
	    /* Make a new call */
	    dom = new LQIO::DOM::Call( getDOM()->getDocument(),
				       LQIO::DOM::Call::Type::RENDEZVOUS,
				       const_cast<LQIO::DOM::Phase *>(_phases[p].getDOM()),
				       const_cast<LQIO::DOM::Entry*>(dynamic_cast<const LQIO::DOM::Entry*>(toEntry->getDOM())),
				       new LQIO::DOM::ConstantExternalVariable(value) );
	    aCall->rendezvous(p,dom);
	}
	return aCall;
    } else {
	return nullptr;
    }
}

LQIO::DOM::Phase::Type
Entry::phaseTypeFlag( const unsigned p ) const
{
    const std::map<unsigned,Phase>::const_iterator i = _phases.find(p);
    return i != _phases.end() ? i->second.phaseTypeFlag() : LQIO::DOM::Phase::Type::STOCHASTIC;
}


bool
Entry::hasServiceTime( const unsigned int p ) const
{
    const std::map<unsigned,Phase>::const_iterator i = _phases.find(p);
    return i != _phases.end() && i->second.hasServiceTime();
}


const LQIO::DOM::ExternalVariable&
Entry::serviceTime( const unsigned p ) const
{
    return getPhase(p).serviceTime();
}

LQX::SyntaxTreeNode *
Entry::serviceTime() const
{
    return std::accumulate( _phases.begin(), _phases.end(), static_cast<LQX::SyntaxTreeNode *>(nullptr), &Phase::accumulate_service_time );
}


LQX::SyntaxTreeNode *
Entry::thinkTime() const
{
    return std::accumulate( _phases.begin(), _phases.end(), static_cast<LQX::SyntaxTreeNode *>(nullptr), &Phase::accumulate_think_time );
}


double
Entry::executionTime( const unsigned p ) const
{
    if ( isStandardEntry() ) {
	const std::map<unsigned,Phase>::const_iterator i = _phases.find(p);
	if ( i == _phases.end() ) return 0.;
	const Phase& phase = i->second;
	return phase.executionTime();
    } else if ( getDOM() ) {
	return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getResultPhasePServiceTime(p);
    } else {
	return 0.;
    }
}



double
Entry::executionTime() const
{
    return std::accumulate( _phases.begin(), _phases.end(), 0.0, sum( &Phase::executionTime ) );
}


double
Entry::variance( const unsigned p ) const
{
    return getPhase(p).variance();
}


double
Entry::variance() const
{
    return std::accumulate( _phases.begin(), _phases.end(), 0.0, sum( &Phase::variance ) );
}


double
Entry::serviceExceeded( const unsigned p ) const
{
    return getPhase(p).serviceExceeded();
}


double
Entry::serviceExceeded() const
{
    return std::accumulate( _phases.begin(), _phases.end(), 0.0, sum( &Phase::serviceExceeded ) );
}

/*
 * Set the entry type field.
 */

bool
Entry::isCalledBy( const request_type requestType )
{
    if ( _requestType != request_type::NOT_CALLED && _requestType != requestType ) {
	getDOM()->runtime_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES );
	return false;
    } else {
	getPhase( 1 );        /* mark an entry which is called as being present */
	_requestType = requestType;
	return true;
    }
}


/*
 * Return true if this entry belongs to a reference task.
 * This is an error!
 */

bool
Entry::isReferenceTaskEntry() const {
    if ( owner() && owner()->isReferenceTask() ) {
	owner()->getDOM()->runtime_error( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, name().c_str() );
	return true;
    } else {
	return false;
    }
}


/*
 * Return true is this entry is selected for printing.
 */

bool
Entry::isSelectedIndirectly() const
{
    if ( Flags::chain() ) {
	return hasPath( Flags::chain() );
    } else if ( owner()->isSelected() ) {
	return true;
    }

    return std::any_of( calls().begin(), calls().end(), std::mem_fn( &GenericCall::isSelected ) );
}



bool
Entry::isActivityEntry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getEntryType() == LQIO::DOM::Entry::Type::ACTIVITY;
}

bool
Entry::isStandardEntry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getEntryType() == LQIO::DOM::Entry::Type::STANDARD;
}

bool
Entry::isSignalEntry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getSemaphoreFlag() == LQIO::DOM::Entry::Semaphore::SIGNAL;
}

bool
Entry::isWaitEntry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getSemaphoreFlag() == LQIO::DOM::Entry::Semaphore::WAIT;
}

bool
Entry::is_r_lock_Entry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == LQIO::DOM::Entry::RWLock::READ_LOCK;
}

bool
Entry::is_r_unlock_Entry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == LQIO::DOM::Entry::RWLock::READ_UNLOCK;
}

bool
Entry::is_w_lock_Entry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == LQIO::DOM::Entry::RWLock::WRITE_LOCK;
}

bool
Entry::is_w_unlock_Entry() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->getRWLockFlag() == LQIO::DOM::Entry::RWLock::WRITE_UNLOCK;
}


/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entryTypeOk( const LQIO::DOM::Entry::Entry::Type aType )
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    const bool rc = const_cast<LQIO::DOM::Entry *>(dom)->entryTypeOk( aType );
    if ( !rc ) {
	dom->input_error( LQIO::ERR_MIXED_ENTRY_TYPES );
    }
    return rc;
}


/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entrySemaphoreTypeOk( const LQIO::DOM::Entry::Semaphore aType )
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    const bool rc = const_cast<LQIO::DOM::Entry *>(dom)->entrySemaphoreTypeOk( aType );
    if ( !rc ) {
	getDOM()->runtime_error( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES );
    }
    return rc;
}

bool
Entry::entryRWLockTypeOk( const LQIO::DOM::Entry::RWLock aType )
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    const bool rc = const_cast<LQIO::DOM::Entry *>(dom)->entryRWLockTypeOk( aType );
    if ( !rc ) {
	getDOM()->runtime_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES );
    }
    return rc;
}


/*
 * Return the number of execution slices.
 */

double
Entry::numberSlices( const unsigned p ) const
{
    if ( !hasServiceTime(p) ) return 0.0;
    return std::accumulate( calls().begin(), calls().end(), 1.0, sum_phase( &Call::rendezvous, p ) );
}



/*
 * Return the slice time of this phase
 */

double
Entry::sliceTime( const unsigned p ) const
{
    return hasServiceTime(p) ? LQIO::DOM::to_double(serviceTime(p)) / numberSlices(p) : 0.0;
}



/*
 * Compute and return CV square for this entry.
 */

double
Entry::Cv_sqr() const
{
    const double t = executionTime();

    if ( !std::isfinite( t ) ) {
	return t;
    } else if ( t > 0.0 ) {
	return variance() / square(t);
    } else {
	return 0.0;
    }
}



/*
 * Return true if any phase has max service time parameter.
 */

bool
Entry::hasHistogram() const
{
    return std::any_of( _phases.begin(), _phases.end(), Predicate<Phase>( &Phase::hasHistogram ) );
}



/*
 * Return true if any phase has a call with a rendezvous.
 */

bool
Entry::hasRendezvous() const
{
    return std::any_of( calls().begin(), calls().end(), std::mem_fn( &GenericCall::hasRendezvous ) );
}



/*
 * Return true if any phase has a call with send-no-reply.
 */

bool
Entry::hasSendNoReply() const
{
    return std::any_of( calls().begin(), calls().end(), std::mem_fn( &GenericCall::hasSendNoReply ) );
}



/*
 * Return true if any phase has a call with forwarding.
 */

bool
Entry::hasForwarding() const
{
    return std::any_of( calls().begin(), calls().end(), std::mem_fn( &GenericCall::hasForwarding ) );
}



/*
 * Return true if there is an open arrival rate for this entry.
 */

bool
Entry::hasOpenArrivalRate() const
{
    return dynamic_cast<const LQIO::DOM::Entry *>(getDOM())->hasOpenArrivalRate();
}



/*
 * Return true if any phase has a call with forwarding.
 */

bool
Entry::hasForwardingLevel() const
{
    return std::any_of( calls().begin(), calls().end(), std::mem_fn( &GenericCall::hasForwardingLevel ) );
}



bool
Entry::isForwardingTarget() const
{
    return std::any_of( callers().begin(), callers().end(), std::mem_fn( &GenericCall::hasForwardingLevel ) );
}



/*
 * Return true if any aFunc returns true for any call
 */

bool
Entry::hasCalls( const callPredicate predicate ) const
{
    return std::any_of( calls().begin(), calls().end(), std::mem_fn( predicate ) );
}



double
Entry::maxServiceTime( const unsigned p ) const
{
    return getPhase(p).maxServiceTime();
}


/*
 * Return true if any phase has max service time parameter.
 */

bool
Entry::hasMaxServiceTime() const
{
    return std::any_of( _phases.begin(), _phases.end(), Predicate<Phase>( &Phase::hasMaxServiceTime ) );
}



/*
 * Return 1 if any phase is deterministic.
 */

bool
Entry::hasDeterministicPhases() const
{
    return std::any_of( _phases.begin(), _phases.end(), Predicate<Phase>( &Phase::isDeterministic ) );
}



/*
 * Return 1 if any phase is not exponential $(C^2_v \not= 1)$.
 */

bool
Entry::hasNonExponentialPhases() const
{
    return std::any_of( _phases.begin(), _phases.end(), Predicate<Phase>( &Phase::isNonExponential ) );
}


/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasThinkTime() const
{
    return std::any_of( _phases.begin(), _phases.end(), Predicate<Phase>( &Phase::hasThinkTime ) );
}


bool
Entry::hasThinkTime( const unsigned int p ) const
{
    const std::map<unsigned,Phase>::const_iterator i = _phases.find(p);
    if ( i != _phases.end() ) {
	const Phase& phase = i->second;
	return phase.hasThinkTime();
    } else {
	return false;
    }
}


const LQIO::DOM::ExternalVariable&
Entry::thinkTime( const unsigned p ) const
{
    return getPhase(p).thinkTime();
}



double
Entry::queueingTime( const unsigned p ) const
{
    const std::map<unsigned,Phase>::const_iterator i = _phases.find(p);
    if ( i != _phases.end() ) {
	const Phase& phase = i->second;
	return phase.queueingTime();	// Time queued for processor.
    } else {
	return 0.;
    }
}


/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasQueueingTime() const
{
    return std::any_of( _phases.begin(), _phases.end(), Predicate<Phase>( &Phase::hasQueueingTime ) );
}



/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Entry::findCall( const Entry * anEntry, const callPredicate aFunc ) const
{
    const std::vector<Call *>::const_iterator call = find_if( calls().begin(), calls().end(), Call::PredicateAndEntry( anEntry, aFunc ) );
    return call != calls().end() ? *call : nullptr;
}



/*
 * Locate the destination anEntry in the list of destinations.
 */

Call *
Entry::findCall( const Task * aTask ) const
{
    const std::vector<Call *>::const_iterator call = find_if( calls().begin(), calls().end(), Call::PredicateAndTask( aTask, &Call::isPseudoCall ) );
    return call != calls().end() ? *call : nullptr;
}



/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

Call *
Entry::findOrAddCall( const Entry * anEntry, const callPredicate aFunc )
{
    Call * aCall = findCall( anEntry, aFunc );
    if ( !aCall ) {
	aCall = new EntryCall( this, anEntry );
	addSrcCall( aCall );
	const_cast<Entry *>(anEntry)->addDstCall( aCall );
    }

    return aCall;
}


/*
 * Return index of destination anEntry.  If it is not found in the list
 * add it.
 */

ProxyEntryCall *
Entry::findOrAddFwdCall( const Entry * anEntry )
{
    ProxyEntryCall * aCall = dynamic_cast<ProxyEntryCall *>(findCall( anEntry, &GenericCall::isPseudoCall ));

    if ( !aCall ) {
	aCall = new ProxyEntryCall( this, anEntry );
	addSrcCall( aCall );
	const_cast<Entry *>(anEntry)->addDstCall( aCall );
    }

    return aCall;
}


Call *
Entry::findOrAddPseudoCall( const Entry * anEntry )
{
    Call * aCall = findCall( anEntry );

    if ( !aCall ) {
	aCall = new PseudoEntryCall( this, anEntry );
	aCall->linestyle( Graphic::LineStyle::DASHED_DOTTED );
	addSrcCall( aCall );
	const_cast<Entry *>(anEntry)->addDstCall( aCall );
    }

    return aCall;
}


/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Entry::countArcs( const callPredicate predicate ) const
{
    return count_if( calls().begin(), calls().end(), GenericCall::PredicateAndSelected( predicate ) );
}


/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Entry::countCallers( const callPredicate predicate ) const
{
    return count_if( callers().begin(), callers().end(), GenericCall::PredicateAndSelected( predicate ) );
}

unsigned
Entry::count_callers::operator()( unsigned int augend, const Entry * entry ) const
{
    return augend + entry->countCallers( _predicate );
}

/* static */ std::set<const Task *>
Entry::collect_callers( const std::set<const Task *>& in, const Entry * entry )
{
    std::set<const Task *> out = in;
    for ( std::vector<GenericCall *>::const_iterator call = entry->callers().begin(); call != entry->callers().end(); ++call ) {
	out.insert( (*call)->srcTask() );
    }
    return out;
}



/*
 * Aggregate activity service time (and calls) to phase p.
 * Results don't make sense, so don't bother with them.  Entry
 * results should be available anyway.
 */

Entry&
Entry::aggregateService( const Activity * anActivity, const unsigned p, const double rate )
{
    /* Aggregate the service time made by the activity to the entry.  Go to the entry dom to get the phase dom so that one can be created. */
    
    const LQIO::DOM::Entry * entry = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    LQIO::DOM::Phase * phase = const_cast<LQIO::DOM::Phase *>(getPhase(p).getDOM());
    if ( phase == nullptr ) {
	phase = const_cast<LQIO::DOM::Entry *>(entry)->getPhase(p);	/* create a DOM object */
	setPhaseDOM( p, phase );
    } else {
	assert( phase == entry->getPhase(p) );		/* Check ... */
    }
    /* Copy entry results to phase */
    phase->setResultServiceTime( entry->getResultPhasePServiceTime(p) );
    phase->setResultServiceTimeVariance( entry->getResultPhasePServiceTimeVariance(p) );
    phase->setResultVarianceServiceTime( entry->getResultPhasePVarianceServiceTime(p) );
    phase->setResultVarianceServiceTimeVariance( entry->getResultPhasePVarianceServiceTimeVariance(p) );
    phase->setResultUtilization( entry->getResultPhasePUtilization(p) );
    phase->setResultUtilizationVariance( entry->getResultPhasePUtilizationVariance(p) );
    phase->setResultProcessorWaiting( entry->getResultPhasePProcessorWaiting(p) );
    phase->setResultProcessorWaitingVariance( entry->getResultPhasePProcessorWaitingVariance(p) );
    
    Phase::merge( *phase, *anActivity->getDOM(), rate );

    const std::vector<Call *>& calls = anActivity->calls();
    for ( std::vector<Call *>::const_iterator call = calls.begin(); call != calls.end(); ++call ) {
	Entry * dstEntry = const_cast<Entry *>((*call)->dstEntry());
	dstEntry->removeDstCall( *call );
	
	/* Aggregate the calls made by the activity to the entry */

	Call * dstCall;
	if ( (*call)->isPseudoCall() ) {
	    dstCall = findOrAddFwdCall( dstEntry );
	} else {
	    dstCall = findOrAddCall( dstEntry );
	}
	dstCall->merge( getPhase(p), p, **call, rate );
	delete *call;
    }

    const_cast<std::vector<Call *>&>(calls).clear();
    const std::vector<LQIO::DOM::Call*>& dom_calls = anActivity->getDOM()->getCalls();
    for ( std::vector<LQIO::DOM::Call *>::const_iterator call = dom_calls.begin(); call != dom_calls.end(); ++call ) {
	delete *call;
    }
    const_cast<std::vector<LQIO::DOM::Call *>&>(dom_calls).clear();
    return *this;
}



/*
 * Move all phases up to phase 1.  Activities must have been aggregated prior to invoking this.
 * (see Task::topologicalSort).
 */

Entry&
Entry::aggregatePhases()
{
    /* Create a phase 1 (using getPhase()) if it does not exist, otherwise, use the existing phase 1 */

    const LQIO::DOM::Entry * entry = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    LQIO::DOM::Phase * phase_1 = const_cast<LQIO::DOM::Phase *>(getPhase(1).getDOM());
    if ( phase_1 == nullptr ) {
	phase_1 = const_cast<LQIO::DOM::Entry *>(entry)->getPhase(1);	/* create a DOM object */
	setPhaseDOM( 1, phase_1 );
    } else {
	assert( phase_1 == entry->getPhase(1) );		/* Check ... */
    }

    /* Merge up the times. */
    
    phase_1->setServiceTime(LQIO::DOM::BCMP_to_LQN::getExternalVariable(serviceTime()));		/*Sums over all phases */
    phase_1->setResultServiceTime(std::accumulate( _phases.begin(), _phases.end(), 0., &Phase::accumulate_execution ));

    /* Merge all calls to phase 1 */
    
    std::for_each( calls().begin(), calls().end(), Exec1<Call,LQIO::DOM::Phase&>( &Call::aggregatePhases, *phase_1 ) );

    /* Delete old stuff */

    assert( _phases.size() >= 1 );
    for ( std::map<unsigned,Phase>::iterator p = std::next(_phases.begin()); p != _phases.end(); ++p ) {
	Phase& phase = p->second;
	if ( phase.getDOM() ) const_cast<LQIO::DOM::Entry *>(entry)->erasePhase(p->first);
	phase.setDOM( nullptr );
    }
    _phases.erase(std::next(_phases.begin()), _phases.end());

    return *this;
}


/* static */ BCMP::Model::Station::Class
Entry::accumulate_demand( const BCMP::Model::Station::Class& augend, const Entry * entry )
{
    return std::accumulate( entry->_phases.begin(), entry->_phases.end(), augend, &Phase::accumulate_demand );
}

/*
 * Chase calls looking for cycles and the depth in the call tree.
 * The return value reflects the deepest depth in the call tree.
 */

size_t
Entry::findChildren( CallStack& callStack, const unsigned directPath ) const
{
    std::pair<std::vector<Call *>::const_iterator,std::vector<Call *>::const_iterator> list(calls().begin(),calls().end());
    size_t max_depth = std::max( followCalls( list, callStack, directPath ), callStack.size() );

    Activity * anActivity = startActivity();
    if ( anActivity ) {
	std::deque<const Activity *> activityStack;		// For checking for cycles.
	try {
	    max_depth = std::max( anActivity->findChildren( callStack, directPath, activityStack ), max_depth );
	}
	catch ( const Activity::cycle_error& error ) {
	    owner()->getDOM()->runtime_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, error.what() );
	    max_depth = std::max( max_depth, error.depth() );
	}
    }
    return max_depth;
}



/*
 * Check entry data.
 */

bool
Entry::check() const
{
    bool rc = true;
    
    /* concordance between c, phase_flag */

    if ( startActivity() ) {

	std::deque<const Activity *> activityStack;
	unsigned next_p = 1;
	double replies = startActivity()->aggregate( const_cast<Entry *>(this), 1, next_p, 1.0, activityStack, &Activity::aggregateReplies );
	if ( requestType() == request_type::RENDEZVOUS ) {
	    if ( replies == 0.0 ) {
		getDOM()->runtime_error( LQIO::ERR_REPLY_NOT_GENERATED );
		rc = false;
	    } else if ( fabs( replies - 1.0 ) > EPSILON ) {
		getDOM()->runtime_error( LQIO::ERR_NON_UNITY_REPLIES, name().c_str(), replies );
		rc = false;
	    }
	    max_phases = std::max( maxPhase(), max_phases );		/* Set global value.	*/
	}

    } else if ( isStandardEntry() ) {
	bool hasServiceTime = false;
	for ( std::map<unsigned,Phase>::const_iterator p = _phases.begin(); p != _phases.end(); ++p ) {
	    const Phase& phase = p->second;
	    max_phases = std::max( maxPhase(), max_phases );		/* Set global value.	*/
	    rc = phase.check() && rc;
	    hasServiceTime = hasServiceTime || phase.hasServiceTime();
	}

	/* Service time for the entry? */

	if ( !hasServiceTime ) {
	    const_cast<Entry *>(this)->getPhase( 1 );	/* force phase presence. */
	    if ( !owner()->hasThinkTime() ) {
		getDOM()->runtime_error( LQIO::WRN_XXXX_TIME_DEFINED_BUT_ZERO, "service" );
	    }
	}

	/* Set some globals for output formatting */

	Model::thinkTimePresent     = Model::thinkTimePresent     || hasThinkTime();
	Model::boundsPresent        = Model::boundsPresent        || throughputBound() > 0.0;

    } else {
	getDOM()->runtime_error( LQIO::ERR_NOT_SPECIFIED );
    }

    if ( owner()->scheduling() != SCHEDULE_SEMAPHORE && ( (isSignalEntry() || isWaitEntry()) ) ) {
	owner()->getDOM()->runtime_error( LQIO::ERR_NOT_SEMAPHORE_TASK, owner()->name().c_str(), (isSignalEntry() ? "signal" : "wait"), name().c_str() );
	rc = false;
    } else if ( owner()->scheduling() != SCHEDULE_RWLOCK 
		&& ( (is_r_lock_Entry() || is_r_unlock_Entry() || is_w_unlock_Entry()|| is_w_lock_Entry() ) ) ) {
	if ( is_r_lock_Entry() || is_r_unlock_Entry() ) {
	    owner()->getDOM()->runtime_error( LQIO::ERR_NOT_RWLOCK_TASK, (is_r_lock_Entry() ? "r_lock" : "r_unlock"), name().c_str() );
	    rc = false;
	} else {
	    owner()->getDOM()->runtime_error( LQIO::ERR_NOT_RWLOCK_TASK, (is_w_lock_Entry() ? "w_lock" : "w_unlock"), name().c_str());
	    rc = false;
	}
    } else if ( !owner()->isReferenceTask() && !hasOpenArrivalRate() && !isCalled() ) {
	getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
    }

    
    /* Forwarding probabilities o.k.? */

    const double sum = std::accumulate( calls().begin(), calls().end(), 0.0, sum_extvar( &Call::forward ) );
    if ( sum < 0.0 || 1.0 < sum ) {
	getDOM()->runtime_error(LQIO::ERR_INVALID_FORWARDING_PROBABILITY, sum );
	rc = false;
    } else if ( sum != 0.0 && owner()->isReferenceTask() ) {
	getDOM()->runtime_error( LQIO::ERR_REFERENCE_TASK_FORWARDING, owner()->name().c_str() );
	rc = false;
    }
    return rc;
}



/*
 * Aggregate activities to this entry.
 */


Entry&
Entry::aggregate()
{
    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>(getDOM());
    if ( startActivity() ) {

	std::deque<const Activity *> activityStack;
	unsigned next_p = 1;

	switch ( Flags::aggregation() ) {
	case Aggregate::ACTIVITIES:
	case Aggregate::PHASES:
	case Aggregate::ENTRIES:
	    startActivity()->aggregate( this, 1, next_p, 1.0, activityStack, &Activity::aggregateService );
	    _startActivity = nullptr;
	    const_cast<LQIO::DOM::Entry *>(dom)->setStartActivity( nullptr );
	    const_cast<LQIO::DOM::Entry *>(dom)->setEntryType( LQIO::DOM::Entry::Type::STANDARD );
	    break;

	case Aggregate::SEQUENCES:
	case Aggregate::THREADS:
	    startActivity()->transmorgrify( activityStack, 1.0 );
	    break;

	case Aggregate::NONE:
	    break;
	}
    }

    /* Convert entry if necessary */

    if ( dom->getEntryType() == LQIO::DOM::Entry::Type::STANDARD ) {
	_startActivity = nullptr;
	if ( _activityCall ) {
	    delete _activityCall;
	    _activityCall = nullptr;
	}
	_activityCallers.clear();
    }

    switch ( Flags::aggregation() ) {
    case Aggregate::PHASES:
    case Aggregate::ENTRIES:
	aggregatePhases();
	break;
    }

    return *this;
}



/*
 * Set the chains used by this entry.  (for queueing networks).
 */

unsigned
Entry::setChain( unsigned curr_k, unsigned next_k, const Entity * aServer, callPredicate aFunc )
{
    if ( aFunc != &GenericCall::hasSendNoReply && (!aServer || (owner()->hasProcessor(dynamic_cast<const Processor *>(aServer)) ) ) ) {
	setServerChain( curr_k ).setClientClosedChain( curr_k );		/* Catch case where there are no calls. */
    }

    if ( startActivity() ) {
	std::deque<const Activity *> activityStack;
	return startActivity()->setChain( activityStack, curr_k, next_k, aServer, aFunc );
    } else {
	for ( std::map<unsigned,Phase>::iterator p = _phases.begin(); p != _phases.end(); ++p ) {
	    Phase& phase = p->second;
	    phase.setChain( curr_k, aServer, aFunc );
	}
    }
    return next_k;
}


/*
 * Set the client chain to k.
 */

Entry&
Entry::setClientClosedChain( unsigned k )
{
    Element::setClientClosedChain( k );
    const_cast<Task *>(owner())->setClientClosedChain( k );
    return *this;
}


/*
 * Set the client chain to k.
 */

Entry&
Entry::setClientOpenChain( unsigned k )
{
    Element::setClientOpenChain( k );
    const_cast<Task *>(owner())->setClientOpenChain( k );
    return *this;
}


/*
 * Set the server chain k.
 */

Entry&
Entry::setServerChain( unsigned k )
{
    const_cast<Task *>(owner())->setServerChain( k );
    Element::setServerChain( k );
    return *this;
}


/*
 * Return all clients to this entry.
 */

unsigned
Entry::referenceTasks( std::vector<Entity *> &clients, Element * dst ) const
{
    if ( owner()->isReferenceTask() ) {
//!!! Check for phase 2, except reference task.
	if ( std::none_of( clients.begin(), clients.end(), EQ<Element>(owner()) ) ) {
	    clients.push_back(const_cast<Task *>(owner()));
	}
//!!! Need to create the pseudo arc to the task.
	if ( dynamic_cast<Processor *>(dst) ) {
	    const_cast<Task *>(owner())->findOrAddPseudoCall( dynamic_cast<Processor *>(dst) );
	} else if ( Flags::aggregation() == Aggregate::ENTRIES ) {
	    const_cast<Task *>(owner())->findOrAddPseudoCall( const_cast<Task *>(dynamic_cast<Entry *>(dst)->owner()) );
	} else {
	    const_cast<Entry *>(this)->findOrAddPseudoCall( dynamic_cast<Entry *>(dst) );
	}
    } else {
	for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	    const Entity * aTask = (*call)->srcTask();
//	if ( (*call)->isSelectedIndirected() && aTask->pathTest() ) {
//	    if ( aTask->pathTest() ) {
	    aTask->referenceTasks( clients, dst );
//	    }
	}
    }
    return clients.size();
}



/*
 * return all clients to this entry.
 */

unsigned
Entry::clients( std::vector<Task *> &clients, const callPredicate aFunc ) const
{
    for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	const Task * task = (*call)->srcTask();
	if ( (*call)->isSelected() && (!aFunc || ((*call)->*aFunc)()) && task->pathTest() && std::none_of( clients.begin(), clients.end(), EQ<Element>(task) ) ) {
	    clients.push_back( const_cast<Task *>(task) );
	}
    }
    return clients.size();
}



/*
 * Return the index used for sorting.
 */

double
Entry::getIndex() const
{
    double anIndex = std::numeric_limits<double>::max();

    for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	if ( !(*call)->isPseudoCall() ) {
	    anIndex = std::min( anIndex, (*call)->srcIndex() );
	}
    }

    return anIndex;
}



/*
 * Move entries higher up more left.
 */

int
Entry::span() const
{
    unsigned int mySpan = 0;
    const int myLevel = owner()->level();
    for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	mySpan = std::max( mySpan, myLevel - (*call)->srcLevel() );
    }
    return mySpan;
}



Graphic::Colour
Entry::colour() const
{
    switch ( Flags::colouring() ) {
    case Colouring::RESULTS:
	if ( Flags::have_results && Flags::graphical_output_style == Output_Style::JLQNDEF && !owner()->isReferenceTask() ) {
	    return colourForUtilization( owner()->isInfinite() ? 0.0 : utilization() / owner()->copiesValue() );
	} else if ( serviceExceeded() > 0. ) {
	    return Graphic::Colour::RED;
	}
	break;

    case Colouring::DIFFERENCES:
	if ( Flags::have_results ) {
	    return colourForDifference( executionTime() );
	}
	break;
	
    case Colouring::CLIENTS:
	if ( myPaths.size() ) {
	    return (Graphic::Colour)(*myPaths.begin() % 11 + 5);		// first element is smallest
	} else {
	    return Graphic::Colour::DEFAULT;
	}

    }

    return owner()->colour();
}


/*
 * Move the entry to x, y.  Drag all the arcs along with it.
 * Reply arcs could be smarter.  We can't sort them right now.
 */

Entry&
Entry::moveTo( const double x, const double y )
{
    Element::moveTo( x, y );
    _label->moveTo( _node->center() );

    moveSrc();		/* Move Arcs	*/
    moveDst();

    if ( _activityCall ) {
	_activityCall->moveSrc( bottomCenter() );

	std::sort( _activityCallers.begin(), _activityCallers.end(), Call::compareDst );

	std::vector<Reply *> left_side;
	std::vector<Reply *> right_side;

	Point dstPoint = bottomLeft();

	/* Sort left and right */

	for ( std::vector<Reply *>::const_iterator reply = _activityCallers.begin(); reply != _activityCallers.end(); ++reply ) {
	    if ( (*reply)->srcActivity()->left() < left() ) {
		right_side.push_back(*reply);
	    } else {
		left_side.push_back(*reply);
	    }
	}

	/* move left_side */

	double delta = width() / (static_cast<double>(1+left_side.size()) * 2.0);
	for ( std::vector<Reply *>::const_iterator reply = left_side.begin(); reply != left_side.end(); ++reply ) {
	    dstPoint.moveBy( delta, 0 );
	    (*reply)->moveDst( dstPoint );
	}

	/* move right_side */

	delta = width() / (static_cast<double>(1+right_side.size()) * 2.0);
	dstPoint = bottomCenter();
	for ( std::vector<Reply *>::const_iterator reply = right_side.begin(); reply != right_side.end(); ++reply ) {
	    dstPoint.moveBy( delta, 0 );
	    (*reply)->moveDst( dstPoint );
	}

    }
    return *this;
}


/*
 * Move all arcs I source.
 */

Entry&
Entry::moveSrc()
{
    sort( _calls.begin(), _calls.end(), Call::compareSrc );

    const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
    Point aPoint = _node->bottomLeft();
    const double delta = width() / static_cast<double>(countArcs() + 1 - nFwd );

    for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( (*call)->isSelected() && !(*call)->hasForwardingLevel() ) {
	    aPoint.moveBy( delta, 0 );
	    (*call)->moveSrc( aPoint );
#if defined(DEBUG)
	    if ( (*call)->hasForwarding() ) {
		std::cerr << "moveSrc: fwding ";
		(*call)->dump( std::cerr ) << std::endl;
	    }
#endif
	}
    }

    return *this;
}



/*
 * Move all arcs I sink.
 */

Entry&
Entry::moveDst()
{
    std::sort( _callers.begin(), _callers.end(), Call::compareDst );
    Point aPoint = _node->topLeft();

    if ( Flags::print_forwarding_by_depth ) {
	const double delta = width() / static_cast<double>(countCallers() + 1 );

	for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	    if ( (*call)->isSelected() ) {
		aPoint.moveBy( delta, 0 );
		(*call)->moveDst( aPoint );
	    }
	}

    } else {

	/*
	 * We add the outgoing forwarding arcs to the incomming side of the entry,
	 * so adjust the counts as necessary.
	 */

	const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
	const double delta = width() / static_cast<double>(countCallers() + 1 + nFwd );
	const double fy1 = Flags::y_spacing() / 2.0 + top();
	const double fy2 = Flags::y_spacing() / 1.5 + top();

	/* Draw incomming forwarding arcs from the same level first. */

	Point leftPoint( left(), fy1 );
	Point rightPoint( right(), fy2 );
	for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	    if ( (*call)->isSelected() && (*call)->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		(*call)->moveDst( aPoint );
		if ( (*call)->srcIndex() < owner()->index() ) {
		    (*call)->movePenultimate( leftPoint );
		    leftPoint.moveBy( delta, 0 );
		} else {
		    (*call)->movePenultimate( rightPoint );
		    rightPoint.moveBy( -delta, 0 );
		}
	    }
	}

	/* Move destination for other incomming arcs (which includes forwarding from other levels). 	*/
	/* Note that these lines are simple two-point vectors, so movePenultimate screws up. 		*/

	for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	    if ( (*call)->isSelected() && !(*call)->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		if ( (*call)->hasForwarding() && (*call)->nPoints() == 4 ) {
		    (*call)->pointAt(1) = aPoint;
		    (*call)->pointAt(2) = aPoint;
		}
		(*call)->moveDst( aPoint );
#if defined(DEBUG)
		if ( (*call)->hasForwarding() ) {
		    std::cerr << "moveDst: fwding ";
		    (*call)->dump( std::cerr ) << std::endl;
		}
#endif
	    }
	}

	/* Move source for outgoing forwarding arcs at same level */

	rightPoint.moveTo( right(), fy1 );
	leftPoint.moveTo( left(), fy2 );
	for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	    if ( (*call)->isSelected() && (*call)->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		(*call)->moveSrc( aPoint );
		if ( (*call)->dstIndex() >= owner()->index() ) {
		    (*call)->moveSecond( rightPoint );
		    rightPoint.moveBy( delta, 0 );
		} else {
		    (*call)->moveSecond( leftPoint );
		    leftPoint.moveBy( -delta, 0 );
		}
	    }
	}
    }

    return *this;
}



Entry&
Entry::scaleBy( const double sx, const double sy )
{
    Element::scaleBy( sx, sy );
    std::for_each( calls().begin(), calls().end(), ExecXY<GenericCall>( &GenericCall::scaleBy, sx, sy ) );
    if ( _activityCall ) {
	_activityCall->scaleBy( sx, sy );
    }
    return *this;
}



Entry&
Entry::translateY( const double dy )
{
    Element::translateY( dy );
    std::for_each( calls().begin(), calls().end(), Exec1<GenericCall,double>( &GenericCall::translateY, dy ) );
    if ( _activityCall ) {
	_activityCall->translateY( dy );
    }
    return *this;
}



Entry&
Entry::depth( const unsigned depth  )
{
    Element::depth( depth-1 );
    std::for_each( calls().begin(), calls().end(), Exec1<GenericCall,unsigned int>( &GenericCall::depth, depth-2 ) );
    if ( _activityCall ) {
	_activityCall->depth( depth-2 );
    }
    return *this;
}



/*
 * Label the node.
 */

Entry&
Entry::label()
{
    *_label << name();
    if ( !startActivity() && Flags::print_input_parameters() ) {
	_label->newLine() << '[' << service_time_of(*this)  << ']';
	if ( hasThinkTime() ) {
	    *_label << ",Z=" << think_time_of(*this);
	}
    }
    if ( Flags::have_results ) {
	if ( Flags::print[SERVICE].opts.value.b ) {
	    _label->newLine() << begin_math() << execution_time_of(*this) << end_math();
	}
	if ( Flags::print[VARIANCE].opts.value.b && Model::variancePresent ) {
	    _label->newLine() << begin_math( &Label::sigma ) << "=" << variance_of(*this) << end_math();
	}
	if ( Flags::print[SERVICE_EXCEEDED].opts.value.b && serviceExceeded() > 0. ) {
	    _label->newLine() << "!=";
	}
	if ( Flags::print[THROUGHPUT_BOUNDS].opts.value.b && Model::boundsPresent ) {
	    _label->newLine() << begin_math() << "L=" << opt_pct(throughputBound()) << end_math();
	}
	if ( Flags::print[ENTRY_THROUGHPUT].opts.value.b ) {
	    _label->newLine() << begin_math( &Label::lambda ) << "=" << opt_pct(throughput()) << end_math();
	}
	if ( Flags::print[ENTRY_UTILIZATION].opts.value.b ) {
	    _label->newLine() << begin_math( &Label::rho ) << "=" << opt_pct(utilization()) << end_math();
	}
//	if ( Flags::print[PROCESSOR_UTILIZATION].opts.value.b ) {
//	    _label->newLine() << begin_math( &Label::rho ) << "=" << opt_pct(processorUtilization()) << end_math();
//	}
    }

    /* Now do calls. */

    std::for_each( calls().begin(), calls().end(), std::mem_fn( &GenericCall::label ) );
    return *this;
}



Entry&
Entry::labelQueueingNetworkVisits( Label& aLabel )
{
    for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	if ( !dynamic_cast<Call *>((*call)) ) continue;
	if ( (*call)->hasRendezvous() ) {
	    aLabel << (*call)->srcName() << ':' << (*call)->dstName() << " ("
		   << print_rendezvous( *dynamic_cast<Call *>((*call)) ) << ")";
	    aLabel.newLine();
	}
	if ( (*call)->hasSendNoReply() ) {
	    aLabel << (*call)->srcName() << ':' << (*call)->dstName() << " ("
		   << print_sendnoreply( *dynamic_cast<Call *>((*call)) ) << ")";
	    aLabel.newLine();
	}
    }

    return *this;
}



Entry&
Entry::labelQueueingNetworkWaiting( Label& aLabel )
{
    for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	const Call * theCall = dynamic_cast<Call *>((*call));
	if ( theCall ) {
	    aLabel << theCall->srcName() << ':' << theCall->dstName() << "="
		   << print_wait( *theCall );
	    aLabel.newLine();
	}
    }

    return *this;
}



Entry&
Entry::labelQueueingNetworkService( Label& aLabel ) 
{
    aLabel.newLine() << name() << "[" << service_time_of(*this) << "]";
    return *this;
}


/*
 * Compute the service time for this entry.
 */

double
Entry::serviceTimeForSRVNInput() const
{
    return std::accumulate( _phases.begin(), _phases.end(), 0.0, sum( &Phase::serviceTimeForSRVNInput ) );
}



double
Entry::serviceTimeForSRVNInput( const unsigned p ) const
{
    return getPhase(p).serviceTimeForSRVNInput();
}


/*
 * Rename processors
 */

Entry&
Entry::rename()
{
    std::ostringstream name;
    name << "e" << elementId();
    const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setName( name.str() );
    return *this;
}


#if BUG_270
/* 
 * Find all callers to this entry, then move the arc to the client.  Delete the client arc.  
 * We don't do activities (yet)
 * Async calls to inf servers?  Do supply demand on processor.
 */

Entry&
Entry::linkToClients( const std::vector<EntityCall *>& proc )
{
    if ( isActivityEntry() ) throw not_implemented( "Entry::linkToClients", __FILE__, __LINE__ );

#if BUG_270_DEBUG
    std::cerr << "Entry::linkToClients() for " << name() << std::endl;
#endif
    for ( std::vector<GenericCall *>::const_iterator x = callers().begin(); x != callers().end(); ++x ) {
	const EntryCall * client_call = dynamic_cast<const EntryCall *>(*x);
	if ( !client_call ) throw not_implemented( "Entry::linkToClients", __FILE__, __LINE__ );
	Entry * client_entry = const_cast<Entry *>(client_call->srcEntry());
	client_entry->removeSrcCall( const_cast<EntryCall *>(client_call) );		// unlink from parent entry.

	/* What about the rate from the client to the server??? */
	for ( std::vector<Call *>::const_iterator server_call = calls().begin(); server_call != calls().end(); ++server_call ) {
	    Call * clone = dynamic_cast<Call *>((*server_call)->clone());
	    if ( dynamic_cast<EntryCall *>(clone) != nullptr ) {
		dynamic_cast<EntryCall *>(clone)->setSrcEntry( client_entry );	// Will be a phase/activity
	    }
	    clone->updateRateFrom( *client_call, **server_call );

	    /* Replace the DOM call with a clone and change the rate.   See Call.cc::rendezvous(p). */
	    
	    client_entry->addSrcCall( clone );	// copy to parent entry.  Duplicates?
	    const Entry * entry = (*server_call)->dstEntry();
	    const_cast<Entry *>(entry)->addDstCall( clone );
#if BUG_270_DEBUG
	    std::cerr << "  Move " << (*server_call)->srcName() <<  "->" << (*server_call)->dstName()
		      << "    to " << clone->srcName() << "->" << clone->dstName();
	    if ( clone->sumOfRendezvous() != nullptr ) {
		std::cerr << ", visits=" << Entity::to_double(clone->sumOfRendezvous());
	    }
#endif
	}

	/* Move the Processor calls to calling task too (owner of client). */

	for ( std::vector<EntityCall *>::const_iterator p = proc.begin(); p != proc.end(); ++p ) {
	    EntityCall * clone = dynamic_cast<EntityCall *>((*p)->clone());
	    if ( dynamic_cast<ProcessorCall *>(clone) ) {
		dynamic_cast<ProcessorCall *>(clone)->setSrcEntry( this );
		clone->updateRateFrom( *client_call );
	    }
	    
	    Task * client_task = const_cast<Task *>(client_entry->owner());
	    clone->setSrcTask( client_task );
	    client_task->addSrcCall( clone );	// copy to parent task.  Duplicates?
	    const Processor * processor = dynamic_cast<const Processor *>((*p)->dstEntity());
	    client_task->addProcessor( processor );
	    const_cast<Processor *>(processor)->addTask( client_task );
	    const_cast<Processor *>(processor)->addDstCall( clone );
#if BUG_270_DEBUG
	    std::cerr << "  Move " << (*p)->srcName() <<  "->" << (*p)->dstName()
		      << "    to " << clone->srcName() << "->" << clone->dstName();
	    if ( clone->sumOfRendezvous() != nullptr ) {
		std::cerr << ", visits=" << Entity::to_double(clone->sumOfRendezvous());
	    }
#endif
	    if ( dynamic_cast<ProcessorCall *>(clone) != nullptr && dynamic_cast<ProcessorCall *>(clone)->service_time() != nullptr ) {
		const Entry * entry = dynamic_cast<ProcessorCall *>(clone)->srcEntry();
		std::cerr << ", service time=" << Entity::to_double(dynamic_cast<ProcessorCall *>(clone)->service_time()) << " from " << entry->name();
	    }
	    std::cerr << std::endl;
	}
    }

    return *this;
}



Entry&
Entry::unlinkFromServers()
{
    std::for_each( calls().begin(), calls().end(), &Entry::remove_from_dst );
    return *this;
}


void
Entry::remove_from_dst( Call * call )
{
    if ( call != nullptr ) const_cast<Entry *>(call->dstEntry())->removeDstCall( call );
}
#endif



#if defined(REP2FLAT)
Entry *
Entry::find_replica( const std::string& entry_name, const unsigned replica ) 
{
    std::ostringstream aName;
    aName << entry_name << "_" << replica;
    std::set<Entry *>::const_iterator nextEntry = find_if( __entries.begin(), __entries.end(), EQStr<Entry>( aName.str() ) );
    if ( nextEntry == __entries.end() ) {
	std::string msg = "Entry::find_replica: cannot find symbol ";
	msg += aName.str();
	throw std::runtime_error( msg );
    }
    return *nextEntry;
}



Entry&
Entry::expand() 
{
    const unsigned replicas = owner()->replicasValue();
    for ( unsigned int replica = 1; replica <= replicas; ++replica ) {
	Entry * new_entry = clone( replica );
	__entries.insert( new_entry );

	/* Patch up observations */

	if ( replica == 1 ) {
	    cloneObservations( getDOM(), new_entry->getDOM() );
	}
    }
    return *this;
}


Entry&
Entry::expandCalls()
{
    std::for_each( calls().begin(), calls().end(), Exec1<Call,const Entry&>( &Call::expand, *this ) );
    return *this;
}


Entry&
Entry::replicateEntry( LQIO::DOM::DocumentObject ** root ) 
{
    const static struct {
	set_function first;
	get_function second;
    } entry_mean[] = { 
	// static std::pair<set_function,get_function> entry_mean[] = {
	{ &LQIO::DOM::DocumentObject::setResultThroughput, &LQIO::DOM::DocumentObject::getResultThroughput },
	{ &LQIO::DOM::DocumentObject::setResultUtilization, &LQIO::DOM::DocumentObject::getResultUtilization },
	{ &LQIO::DOM::DocumentObject::setResultProcessorUtilization, &LQIO::DOM::DocumentObject::getResultProcessorUtilization },
	{ &LQIO::DOM::DocumentObject::setResultSquaredCoeffVariation, &LQIO::DOM::DocumentObject::getResultSquaredCoeffVariation },
	{ &LQIO::DOM::DocumentObject::setResultWaitingTime, &LQIO::DOM::DocumentObject::getResultWaitingTime },
	{ &LQIO::DOM::DocumentObject::setResultPhase1ProcessorWaiting, &LQIO::DOM::DocumentObject::getResultPhase1ProcessorWaiting },
	{ &LQIO::DOM::DocumentObject::setResultPhase1ServiceTime, &LQIO::DOM::DocumentObject::getResultPhase1ServiceTime },
	{ &LQIO::DOM::DocumentObject::setResultPhase1Utilization, &LQIO::DOM::DocumentObject::getResultPhase1Utilization },
	{ &LQIO::DOM::DocumentObject::setResultPhase1VarianceServiceTime, &LQIO::DOM::DocumentObject::getResultPhase1VarianceServiceTime },
	{ &LQIO::DOM::DocumentObject::setResultPhase2ProcessorWaiting, &LQIO::DOM::DocumentObject::getResultPhase2ProcessorWaiting },
	{ &LQIO::DOM::DocumentObject::setResultPhase2ServiceTime, &LQIO::DOM::DocumentObject::getResultPhase2ServiceTime },
	{ &LQIO::DOM::DocumentObject::setResultPhase2Utilization, &LQIO::DOM::DocumentObject::getResultPhase2Utilization },
	{ &LQIO::DOM::DocumentObject::setResultPhase2VarianceServiceTime, &LQIO::DOM::DocumentObject::getResultPhase2VarianceServiceTime },
	{ &LQIO::DOM::DocumentObject::setResultPhase3ProcessorWaiting, &LQIO::DOM::DocumentObject::getResultPhase3ProcessorWaiting },
	{ &LQIO::DOM::DocumentObject::setResultPhase3ServiceTime, &LQIO::DOM::DocumentObject::getResultPhase3ServiceTime },
	{ &LQIO::DOM::DocumentObject::setResultPhase3Utilization, &LQIO::DOM::DocumentObject::getResultPhase3Utilization },
	{ &LQIO::DOM::DocumentObject::setResultPhase3VarianceServiceTime, &LQIO::DOM::DocumentObject::getResultPhase3VarianceServiceTime },
	{ nullptr, nullptr }
    };

    const static struct {
	set_function first;
	get_function second;
    } entry_variance[] = { 
	//static std::pair<set_function,get_function> entry_variance[] = {
	{ &LQIO::DOM::DocumentObject::setResultThroughputVariance, &LQIO::DOM::DocumentObject::getResultThroughputVariance },
	{ &LQIO::DOM::DocumentObject::setResultUtilizationVariance, &LQIO::DOM::DocumentObject::getResultUtilizationVariance },
	{ &LQIO::DOM::DocumentObject::setResultProcessorUtilizationVariance, &LQIO::DOM::DocumentObject::getResultProcessorUtilizationVariance },
	{ &LQIO::DOM::DocumentObject::setResultSquaredCoeffVariationVariance, &LQIO::DOM::DocumentObject::getResultSquaredCoeffVariationVariance },
	{ &LQIO::DOM::DocumentObject::setResultWaitingTimeVariance, &LQIO::DOM::DocumentObject::getResultWaitingTimeVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase1ProcessorWaitingVariance, &LQIO::DOM::DocumentObject::getResultPhase1ProcessorWaitingVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase1ServiceTimeVariance, &LQIO::DOM::DocumentObject::getResultPhase1ServiceTimeVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase1UtilizationVariance, &LQIO::DOM::DocumentObject::getResultPhase1UtilizationVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase1VarianceServiceTimeVariance, &LQIO::DOM::DocumentObject::getResultPhase1VarianceServiceTimeVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase2ProcessorWaitingVariance, &LQIO::DOM::DocumentObject::getResultPhase2ProcessorWaitingVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase2ServiceTimeVariance, &LQIO::DOM::DocumentObject::getResultPhase2ServiceTimeVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase2UtilizationVariance, &LQIO::DOM::DocumentObject::getResultPhase2UtilizationVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase2VarianceServiceTimeVariance, &LQIO::DOM::DocumentObject::getResultPhase2VarianceServiceTimeVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase3ProcessorWaitingVariance, &LQIO::DOM::DocumentObject::getResultPhase3ProcessorWaitingVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase3ServiceTimeVariance, &LQIO::DOM::DocumentObject::getResultPhase3ServiceTimeVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase3UtilizationVariance, &LQIO::DOM::DocumentObject::getResultPhase3UtilizationVariance },
	{ &LQIO::DOM::DocumentObject::setResultPhase3VarianceServiceTimeVariance, &LQIO::DOM::DocumentObject::getResultPhase3VarianceServiceTimeVariance },
	{ nullptr, nullptr }
    };

    unsigned int replica = 0;
    std::string root_name = baseReplicaName( replica );
    LQIO::DOM::Entry * dom = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(getDOM()));
    if ( replica == 1 ) {
	*root = const_cast<LQIO::DOM::DocumentObject *>(getDOM());
	std::pair<std::set<Entry *>::iterator,bool> rc = __entries.insert( this );
	if ( !rc.second ) throw std::runtime_error( "Duplicate entry" );
	(*root)->setName( root_name );
	const_cast<LQIO::DOM::Document *>(dom->getDocument())->addEntry( dom );		    /* Reconnect all of the dom stuff. */
    } else if ( root_name == (*root)->getName() ) {
	for ( unsigned int i = 0; entry_mean[i].first != nullptr; ++i ) {
	    update_mean( *root, entry_mean[i].first, getDOM(), entry_mean[i].second, replica );
	    update_variance( *root, entry_variance[i].first, getDOM(), entry_variance[i].second );
	}
    }

    /* Do phases */

    for ( std::map<unsigned,Phase>::iterator p = _phases.begin(); p != _phases.end(); ++p ) {
	const LQIO::DOM::Phase * phase = const_cast<const LQIO::DOM::Entry *>(dom)->getPhase(p->first);
	p->second.replicatePhase( const_cast<LQIO::DOM::Phase *>(phase), replica );
    }

    return *this;
}

Entry&
Entry::replicateCall() 
{
    unsigned int replica = 0;
    std::string root_name = baseReplicaName( replica );
    if ( replica == 1 ) {
	std::vector<Call *> old_calls = _calls;
	_calls.clear();

	/* 
	 * Remove Calls from DOM.  Reinsert useful calls.  A bit of a
	 * pain since DOM calls are attached to DOM phases while
	 * drawing calls are associated with entries
	 */
	
	std::for_each( _phases.begin(), _phases.end(), Exec( &Phase::replicateCall ) );

	Call * root = nullptr;
	std::for_each( old_calls.begin(), old_calls.end(), Exec2<Call, std::vector<Call *>&, Call **>( &Call::replicateCall, _calls, &root ) );
    }
    return *this;
}
#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                  */
/* ------------------------------------------------------------------------ */

/*
 * Draw the entry.
 */

const Entry&
Entry::draw( std::ostream& output ) const
{
    std::ostringstream aComment;
    aComment << "Entry " << name();
    if ( _startActivity ) {
	aComment << " A " << _startActivity->name();
    } else {
	aComment << " s [" << service_time_of( *this ) << "]";
    };
#if defined(BUG_375)
    aComment << " span=" << span() << ", index=" << index();
#endif
    _node->comment( output, aComment.str() );

    const double dx = adjustForSlope( fabs(height()) );

    _node->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() ).fillColour( colour() );	/* fillColour ignored since its a line. */
    std::vector<Point> points;
    if ( Flags::graphical_output_style == Output_Style::JLQNDEF ) {
	points.resize(4);
	points[0] = _node->topRight();
	points[1] = _node->topLeft().moveBy( dx, 0 );
	points[2] = _node->bottomLeft();
	points[3] = _node->bottomRight().moveBy( -dx, 0 );
	_node->polygon( output, points );
    } else {
	if ( drawLeft && drawRight ) {
	    points.resize(4);
	    points[0] = _node->topLeft().moveBy( dx, 0 );
	    points[1] = _node->bottomLeft();
	    points[2] = _node->bottomRight().moveBy( -dx, 0 );
	    points[3] = _node->topRight();
	} else if ( drawLeft ) {
	    points.resize(3);
	    points[0] = _node->topLeft().moveBy( dx, 0 );
	    points[1] = _node->bottomLeft();
	    points[2] = _node->bottomRight().moveBy( -dx, 0 );
	} else if ( drawRight ) {
	    points.resize(3);
	    points[0] = _node->bottomLeft();
	    points[1] = _node->bottomRight().moveBy( -dx, 0 );
	    points[2] = _node->topRight();
	} else {
	    points.resize(2);
	    points[0] = _node->bottomLeft();
	    points[1] = _node->bottomRight().moveBy( -dx, 0 );
	}
	_node->polyline( output, points );
    }

    _label->backgroundColour( colour() ).comment( output, aComment.str() );
    output << *_label;

    Call * lastCall = nullptr;
    for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( (*call)->isSelected() ) {
	    if ( (*call)->hasForwarding() ) {

		/*
		 * Remove duplicate sections in arc.  Do here because
		 * all arcs have been moved by now.  This will likely
		 * have to get smarter over time.
		 */

		if ( lastCall ) {
		    if ( (*call)->pointAt(1) == lastCall->pointAt(1) && (*call)->pointAt(2) == lastCall->pointAt(2) ) {
			Call * call2 = const_cast<Call *>(*call);
			Point aPoint = lastCall->pointAt(3);
			const_cast<Point&>(call2->pointAt(1)) = const_cast<Point&>(call2->pointAt(2)) = aPoint;
		    }
		} else {
		    lastCall = *call;
		}
	    }
	    output << *(*call);
	}
    }

    if ( _activityCall ) {
	_activityCall->penColour( colour() );
	_activityCall->draw( output );
    }

    /* Draw reply arcs here for PostScript layering */

    std::for_each( _activityCallers.begin(), _activityCallers.end(), ConstExec1<GenericCall,std::ostream&>(&GenericCall::draw, output) );
    return *this;
}

/*
 * translate the from and to entry names to indecies.  Return 1 if all went well.
 */

bool
map_entry_names( const char * from_entry_name, Entry * & fromEntry,
		 const char * to_entry_name, Entry * & toEntry,
		 err_func_t err_func )
{
    bool rc    = true;
    fromEntry = Entry::find( from_entry_name );
    toEntry   = Entry::find( to_entry_name );

    if ( !fromEntry ) {
	(*err_func)( LQIO::ERR_NOT_DEFINED, from_entry_name );
	rc = false;
    }
    if ( !toEntry ) {
	(*err_func)( LQIO::ERR_NOT_DEFINED, to_entry_name );
	rc = false;
    }
    if ( fromEntry == toEntry && fromEntry ) {
	(*err_func)( LQIO::ERR_SRC_EQUALS_DST, to_entry_name, from_entry_name );
	rc = false;
    }
    return rc;
}



/*
 * Compare entries (for sorting).  Sort forwarded entries first.
 */

bool
Entry::compare( const Entry * e1, const Entry * e2 )
{
    if ( (e1->isForwardingTarget() && !e2->isForwardingTarget())
	 || ( !e1->hasForwardingLevel() && e2->hasForwardingLevel() ) ) {
	return true;
    } else if ( (!e1->isForwardingTarget() && e2->isForwardingTarget())
		|| ( e1->hasForwardingLevel() && !e2->hasForwardingLevel() ) ) {
	return false;
    } else {
	return Element::compare( e1, e2 );
    }
}

/*----------------------------------------------------------------------*/
/*		 	   Called from yyparse.  			*/
/*----------------------------------------------------------------------*/

Entry *
Entry::find( const std::string& name )
{
    std::set<Entry *>::const_iterator entry = find_if( __entries.begin(), __entries.end(), EQStr<Entry>( name ) );
    return entry != __entries.end() ? *entry : nullptr;
}


/*
 *  Add an entry.
 */

Entry *
Entry::create( LQIO::DOM::Entry* dom )
{
    const std::string& entry_name = dom->getName();
    if ( Entry::find( entry_name ) ) {
	dom->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return nullptr;
    } else {
	Entry * anEntry = new Entry( dom );
	assert( anEntry != nullptr );
	__entries.insert( anEntry );
	return anEntry;
    }
}

/*----------------------------------------------------------------------*/
/*		 	    Output formatting.   			*/
/*----------------------------------------------------------------------*/

static std::ostream&
format( std::ostream& output, int p ) {
    switch ( Flags::output_format() ) {
    case File_Format::SRVN:
	if ( p != 1 ) {
	    output << ' ';
	}
	break;

    case File_Format::OUTPUT:
    case File_Format::PARSEABLE:
    case File_Format::RTF:
	output << std::setw( maxDblLen );
	break;

    default:
	if ( p != 1 ) {
	    output << ',';
	}
	break;
    }
    return output;
}


Label&
Entry::print_execution_time( Label& label, const Entry& entry )
{
    for ( unsigned p = 1; p <= entry.maxPhase(); ++p ) {
	if ( p > 1 ) label << ",";
	label << opt_pct(entry.executionTime(p));
    }
    return label;
}


Label&
Entry::print_queueing_time( Label& label, const Entry& entry )
{
    for ( unsigned p = 1; p <= entry.maxPhase(); ++p ) {
	if ( p > 1 ) label << ",";
	label << opt_pct(entry.queueingTime(p));
    }
    return label;
}


Label&
Entry::print_variance( Label& label, const Entry& entry )
{
    for ( unsigned p = 1; p <= entry.maxPhase(); ++p ) {
	if ( p > 1 ) label << ",";
	label << opt_pct(entry.variance(p));
    }
    return label;
}

std::ostream&
Entry::print_service_time( std::ostream& output, const Entry& entry )
{
    for ( unsigned p = 1; p <= entry.maxPhase(); ++p ) {
	format( output, p );
	if ( entry.hasServiceTime(p) ) {
	    output << entry.serviceTime(p);
	} else {
	    output << 0.0;
	}
    }
    return output;
}


std::ostream&
Entry::print_think_time( std::ostream& output, const Entry& entry )
{
    for ( unsigned p = 1; p <= Entry::max_phases; ++p ) {
	format( output, p );
	if ( entry.hasThinkTime(p) ) {
	    output << entry.thinkTime(p);
	} else {
	    output << 0.0;
	}
    }
    return output;
}

SRVNEntryManip service_time_of( const Entry& entry ) { return SRVNEntryManip( &Entry::print_service_time, entry ); }
SRVNEntryManip think_time_of( const Entry& entry ) { return SRVNEntryManip( &Entry::print_think_time, entry ); }
