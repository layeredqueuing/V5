/*  -*- c++ -*-
 * $Id: call.cc 15141 2021-12-02 15:31:46Z greg $
 *
 * Everything you wanted to know about a call to an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 */

#include "lqn2ps.h"
#include <algorithm>
#include <numeric>
#include <cassert>
#include <cstdlib>
#include <cmath>
#if HAVE_IEEEFP_H && !defined(MSDOS)
#include <ieeefp.h>
#endif
#include <lqio/input.h>
#include <lqio/dom_call.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_task.h>
#include <lqio/srvn_spex.h>
#include "call.h"
#include "arc.h"
#include "label.h"
#include "entry.h"
#include "entity.h"
#include "task.h"
#include "open.h"
#include "processor.h"
#include "phase.h"
#include "activity.h"
#include "errmsg.h"

static SRVNCallManip print_calls( const Call& aCall );
static LabelCallManip print_drop_probability( const Call& aCall );

bool Call::__hasVariance = false;

/*----------------------------------------------------------------------*/
/*                            Generic  Calls                            */
/*----------------------------------------------------------------------*/

GenericCall::GenericCall()
    : _label(nullptr), _arc(nullptr)
{
    _arc = Arc::newArc();
    _label = Label::newLabel();
    _label->justification( Flags::label_justification );
}


/*
 * Deep copy.
 */

GenericCall::GenericCall( const GenericCall& src )
{
    _arc = Arc::newArc();
    _label = src._label->clone();
}


GenericCall::~GenericCall()
{
    delete _arc;
    delete _label;
}



double
GenericCall::srcIndex() const
{
    return srcTask()->index();
}


unsigned
GenericCall::srcLevel() const
{
    return srcTask()->level();
}


/*
 * Return true IFF this call forwards and the src and dst are on the
 * same layer.
 */

bool
GenericCall::hasForwardingLevel() const
{
    return !Flags::print_forwarding_by_depth
	&& hasForwarding()
	&& dstLevel() == srcLevel()
	&& !isLoopBack();
}

Graphic::colour_type
GenericCall::colour() const
{
    return Graphic::DEFAULT_COLOUR;
}

GenericCall&
GenericCall::moveSrc( const Point& aPoint )
{
    _arc->moveSrc( aPoint );
    return *this;
}



GenericCall&
GenericCall::moveSrcBy( const double dx, const double dy )
{
    _arc->moveSrcBy( dx, dy );
    return *this;
}



GenericCall&
GenericCall::moveDst( const Point& aPoint )
{
    _arc->moveDst( aPoint );
    return *this;
}



GenericCall&
GenericCall::moveSecond( const Point& aPoint )
{
    _arc->moveSecond( aPoint );
    return *this;
}



GenericCall&
GenericCall::movePenultimate( const Point& aPoint )
{
    _arc->movePenultimate( aPoint );
    return *this;
}



GenericCall&
GenericCall::scaleBy( const double sx, const double sy )
{
    _arc->scaleBy( sx, sy );
    _label->scaleBy( sx, sy );
    return *this;
}



GenericCall&
GenericCall::translateY( const double dy )
{
    _arc->translateY( dy );
    _label->translateY( dy );
    return *this;
}



GenericCall&
GenericCall::depth( const unsigned curDepth )
{
    _arc->depth( curDepth );
    _label->depth( curDepth-1 );
    return *this;
}



const GenericCall&
GenericCall::draw( std::ostream& output ) const
{
    if ( !isSelected() ) return *this;
    
    std::ostringstream aComment;
    aComment << "Call "
	     << srcName() << " "
	     << dstName();
    if ( dynamic_cast<const Call *>(this) ) {
	if ( hasRendezvous() ) {
	    aComment << " y (" << print_rendezvous( *dynamic_cast<const Call *>(this) ) << ")";
	}
	if ( hasSendNoReply() ) {
	    aComment << " z (" << print_sendnoreply( *dynamic_cast<const Call *>(this) ) << ")";
	}
	if ( hasForwarding() ) {
	    aComment << " F (" << print_forwarding( *dynamic_cast<const Call *>(this) ) << ")";
	}
    }
    _arc->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() );
    _arc->comment( output, aComment.str() );
    output << *_arc;

    _label->backgroundColour( Graphic::DEFAULT_COLOUR ).comment( output, aComment.str() );
    output << *_label;
    return *this;
}


std::ostream& GenericCall::dump( std::ostream& output ) const
{
    const unsigned int n = nPoints();
    for ( unsigned int i = 0; i < n; ++i ) {
	if ( i > 0 ) std::cerr << ", ";
	std::cerr << pointAt(i);
    }
    return output;
}


/*
 * Compare destinations of the arcs leaving the source entry (for sorting).
 */

bool
GenericCall::compareSrc( const GenericCall * call1, const GenericCall * call2 )
{
    if ( dynamic_cast<const Call *>(call1) && dynamic_cast<const Call *>(call2) && dynamic_cast<const Call *>(call1)->dstEntry() == dynamic_cast<const Call *>(call2)->dstEntry() ) {
	if ( call1->hasForwarding() && !call2->hasForwarding() ) {
	    return false;
	} else if ( !call1->hasForwarding() && call2->hasForwarding() ) {
	    return true;
	}
    }

    const double diff = call1->dstIndex() - call2->dstIndex();
    if ( fabs( diff ) < 1.0 ) {
	if ( (call1->dstLevel() <= call2->dstLevel() && call1->srcIndex() <= call1->dstIndex())
	    || (call1->dstLevel() >= call2->dstLevel() && call1->srcIndex() >= call1->dstIndex())) {
	    return false;
	} else {
	    return true;
	}
    }
    return diff < 0;
}


/*
 * Compare sources of the calls entering entry (for sorting).
 */

bool
GenericCall::compareDst( const GenericCall * call1, const GenericCall * call2 )
{
    if ( dynamic_cast<const Call *>(call1) && dynamic_cast<const Call *>(call2) && dynamic_cast<const Call *>(call1)->dstEntry() == dynamic_cast<const Call *>(call2)->dstEntry() ) {
	if ( call1->hasForwarding() && !call2->hasForwarding() ) {
	    return false;
	} else if ( !call1->hasForwarding() && call2->hasForwarding() ) {
	    return true;
	}
    }

    const double diff = call1->srcIndex() - call2->srcIndex();
    if ( fabs( diff ) < 1.0 ) {
	if ( (call1->srcLevel() < call2->srcLevel() && call1->srcIndex() >= call1->dstIndex())
	    || (call1->srcLevel() > call2->srcLevel() && call1->srcIndex() <= call1->dstIndex()) ) {
	    return true;
	} else {
	    return false;
	}
    }
    return diff < 0;
}


void
GenericCall::dump() const
{
    std::cout << "(" << srcName() << "->" << dstName() << ") ";
    switch ( callType() ) {
    case LQIO::DOM::Call::Type::NULL_CALL: std::cout << "NULL"; break;
    case LQIO::DOM::Call::Type::RENDEZVOUS: std::cout << "RNV"; break;
    case LQIO::DOM::Call::Type::SEND_NO_REPLY: std::cout << "SNR"; break;
    default: std::cout << "???"; break;
    }
}

/*----------------------------------------------------------------------*/
/*                          Calls between Entries                       */
/*----------------------------------------------------------------------*/

Call::Call()
    : GenericCall(),
      _destination(nullptr),
      _callType(LQIO::DOM::Call::Type::NULL_CALL),
      _calls(),
      _forwarding(nullptr)
{
}

/*
 * Initialize and zero fields.   Reverse links are set here.  Forward
 * links are done by subclass.  Processor calls are linked specially.
 */

Call::Call( const Entry * toEntry, const unsigned nPhases )
    : GenericCall(),
      _destination(toEntry),
      _callType(LQIO::DOM::Call::Type::NULL_CALL),
      _calls(nPhases,nullptr),
      _forwarding(nullptr)
{
}


/*
 * Deep copy.
 */

Call::Call( const Call& src )
    : GenericCall( src ),
      _destination(src._destination),
      _callType(src._callType),
      _calls(src._calls),
      _forwarding(src._forwarding)

{
    for ( std::vector<const LQIO::DOM::Call *>::iterator call = _calls.begin(); call != _calls.end(); ++call ) {
	if ( *call != nullptr ) *call = (*call)->clone();
    }
    if ( _forwarding != nullptr ) _forwarding = _forwarding->clone();
}

/*
 * Clean up the mess.
 */

Call::~Call()
{
    _destination = nullptr;			/* to whom I am referring to	*/
}


int
Call::operator==( const Call& item ) const
{
    return (dstEntry() == item.dstEntry());
}


bool
Call::checkReplication() const
{
    bool ok = true;
    double srcReplicasValue = 1;
    double dstReplicasValue = 1;
    if ( srcTask()->isReplicated() ) {
	const LQIO::DOM::ExternalVariable& srcReplicas = srcTask()->replicas();
	if ( srcReplicas.wasSet() ) ok = srcReplicas.getValue( srcReplicasValue );
    }
    if ( ok && dstTask()->isReplicated() ) {
	const LQIO::DOM::ExternalVariable& dstReplicas = dstTask()->replicas();
	if ( dstReplicas.wasSet() ) ok = dstReplicas.getValue( dstReplicasValue );
    }
    if ( ok && (srcReplicasValue > 1 || dstReplicasValue > 1 ) ) {
	const std::string& srcName = srcTask()->name();
	const std::string& dstName = dstTask()->name();
	double fanOutValue = 1;
	double fanInValue = 1;
	const LQIO::DOM::ExternalVariable * fan_out = dynamic_cast<const LQIO::DOM::Task *>(srcTask()->getDOM())->getFanOut( dstName );
	if ( fan_out != nullptr && fan_out->wasSet() ) fan_out->getValue( fanOutValue );
	const LQIO::DOM::ExternalVariable * fan_in = dynamic_cast<const LQIO::DOM::Task *>(dstTask()->getDOM())->getFanIn( srcName );
	if ( fan_in != nullptr && fan_in->wasSet() ) fan_in->getValue( fanInValue );
	if ( srcReplicasValue * fanOutValue != dstReplicasValue * fanInValue ) {
	    LQIO::solution_error( ERR_REPLICATION,
				  static_cast<int>(fanOutValue), srcName.c_str(), static_cast<int>(srcReplicasValue),
				  static_cast<int>(fanInValue),  dstName.c_str(), static_cast<int>(dstReplicasValue) );
	    return false;
	}
    }
    return true;
}


unsigned
Call::fanIn() const
{
    return dstTask()->fanInValue( srcTask() );
}

unsigned
Call::fanOut() const
{
    return srcTask()->fanOutValue( dstTask() );
}


/*
 * Add all phases of src to the receiver.  multiply by rate.
 */

Call&
Call::merge( Phase & phase, const Call& src, const double rate )
{
    const size_t n = numberOfPhases();
    for ( size_t p = 1; p <= n; ++p ) {
	merge( phase, p, src, rate );
    }
    return *this;
}



/*
 * Merge calls from the activity (which is phase 1) to phase p of the entry.
 */

Call&
Call::merge( Phase& phase, const unsigned int p, const Call& src, const double rate )
{
    assert( ( (isPseudoCall() && src.isPseudoCall() ) || (!isPseudoCall() && !src.isPseudoCall() ) ) && ( 1 <= p && p <= numberOfPhases() ) );

    if ( src._calls[0] == nullptr ) {
	return *this;
    } else if ( !equalType( src ) ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    } else {
	_callType = src._callType;
	if ( p > _calls.size() ) {
	    _calls.resize(p);				/* Make it big enough if neccessary */
	}
	const LQIO::DOM::Call * call = src._calls[0];	/* Phases go from 1-3, vector goes from 0-2. */
	if ( _calls[p-1] == nullptr ) {
	    LQIO::DOM::Call * newCall = call->clone();		/* Copy call. */
	    _calls[p-1] = newCall;
	    newCall->setSourceObject(const_cast<LQIO::DOM::Phase *>(phase.getDOM()));
	    newCall->setDestinationEntry(const_cast<LQIO::DOM::Entry *>(call->getDestinationEntry()));
	    const_cast<LQIO::DOM::Phase *>(phase.getDOM())->addCall( newCall );
	} else {
	    const_cast<LQIO::DOM::Call *>(_calls[p-1])->setCallMeanValue( _calls[p-1]->getCallMeanValue() + call->getCallMeanValue() * rate );
	}
    }

    /* No Forwarding since we are merging from activities */

    if ( _arc ) {
	if ( hasSendNoReply() ) {
	    _arc->arrowhead(Graphic::OPEN_ARROW);
	} else {
	    _arc->arrowhead(Graphic::CLOSED_ARROW);
	    if ( hasForwarding() ) {
		_arc->linestyle(Graphic::DASHED);
	    }
	}
    }

    setArcType();
    return *this;
}


/*
 * Move all phases to phase 1.
 */

Call&
Call::aggregatePhases( LQIO::DOM::Phase& src )
{
    for ( unsigned p = 1; p < maxPhase(); ++p ) {		// Remember! p[0] is phase 1
	if ( _calls.at(p) ) {
	    const LQIO::DOM::Call * call = _calls[p];
	    if ( _calls[0] == nullptr ) {
		LQIO::DOM::Call * clone = call->clone();		/* copy call to phase 1 */
		_calls[0] = clone;
		clone->setSourceObject( const_cast<LQIO::DOM::DocumentObject *>(call->getSourceObject()) );
		clone->setDestinationEntry( const_cast<LQIO::DOM::Entry *>(call->getDestinationEntry()) );
		src.addCall( clone );
	    } else {
		const double rnv_src = to_double(*call->getCallMean());
		const_cast<LQIO::DOM::Call *>(_calls[0])->setCallMeanValue(to_double(*_calls[0]->getCallMean()) + rnv_src);
	    }
	}
    }

    setArcType();
    return *this;
}


#if defined(BUG_270)
/*
 * multiply rate from client call (by phase) by server (by phase) and overwrite clone.  The 
 * server should only have one phase.
 */

Call&
Call::updateRateFrom( const Call& client, const Call& server )
{
    if ( !client.equalType( server ) ) return *this;
    
    const size_t client_size = client._calls.size();
    const size_t server_size = server.maxPhase();
    assert( server_size == 1 );	/* Otherwise, server has more phases... */
    _calls.resize( client_size );
    try {
	/* The value in either the source or destination may not be a constant */
	for ( size_t p = 0; p < client_size; ++p ) {
	    const double client_calls = client._calls[p] != nullptr ? client._calls[p]->getCallMeanValue() : 0.;
	    if ( client_calls == 0 ) {
		if ( _calls[p] != nullptr ) {
		    delete _calls[p];
		    _calls[p] = nullptr;
		}
	    } else {
		if ( _calls[p] == nullptr ) {
		    _calls[p] = server._calls[0]->clone();
		}
		const_cast<LQIO::DOM::Call *>(_calls[p])->setCallMeanValue( server._calls[0]->getCallMeanValue() * client_calls );
	    }
	}
    }
    catch ( const std::domain_error& e ) {
	/* so if it isn't, just ignore the rate.  Signal problem? */
    }
    return *this;
}
#endif


/*
 * Set the arc type (ie., arrow and line style)
 */

Call&
Call::setArcType()
{
    if ( _arc ) {
	if ( hasSendNoReply() ) {
	    _arc->arrowhead(Graphic::OPEN_ARROW);
	} else {
	    _arc->arrowhead(Graphic::CLOSED_ARROW);
	    if ( hasForwarding() ) {
		_arc->linestyle(Graphic::DASHED);
	    }
	}
    }

    if ( (hasRendezvous() || hasForwarding()) && hasSendNoReply() ) {
 	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    }

    return *this;
}


void
Call::reset()
{
    __hasVariance = false;
}



const LQIO::DOM::ExternalVariable *
Call::sumOfRendezvous() const
{
    if ( !hasRendezvous() ) return nullptr;
    return std::accumulate( _calls.begin(), _calls.end(), static_cast<const LQIO::DOM::ExternalVariable *>(nullptr), &Call::sum_of_calls );
}


const LQIO::DOM::ExternalVariable &
Call::rendezvous( const unsigned p ) const
{
    if ( 0 < p && p <= _calls.size() && _calls[p-1] != nullptr && hasRendezvous() ) return *_calls[p-1]->getCallMean();
    else return Element::ZERO;
}


Call&
Call::rendezvous( const unsigned p, const LQIO::DOM::Call * value )
{
    if ( hasSendNoReply() ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    } else if ( !_calls.at(p-1) ) {
	_callType = LQIO::DOM::Call::Type::RENDEZVOUS;
	_calls[p-1] = value;
	if ( _arc ) {
	    _arc->arrowhead(Graphic::CLOSED_ARROW);
	}
    }
    return *this;
}

Call&
Call::rendezvous( const unsigned p, const double value )
{
    if ( hasSendNoReply() ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    } else if ( !_calls.at(p-1) ) {
	_callType = LQIO::DOM::Call::Type::RENDEZVOUS;
	const_cast<LQIO::DOM::Call *>(_calls[p-1])->setCallMeanValue(value);
	if ( _arc ) {
	    _arc->arrowhead(Graphic::CLOSED_ARROW);
	}
    }
    return *this;
}


double
Call::sumOfSendNoReply() const
{
    const LQIO::DOM::ExternalVariable * sum = std::accumulate( _calls.begin(), _calls.end(), static_cast<const LQIO::DOM::ExternalVariable *>(nullptr), &Call::sum_of_calls );
    return sum != nullptr ? to_double( *sum ) : 0.0;
}



const LQIO::DOM::ExternalVariable &
Call::sendNoReply( const unsigned p ) const
{
    if ( 0 < p && p <= _calls.size() && _calls[p-1] && hasSendNoReply() ) return *_calls[p-1]->getCallMean();
    else return Element::ZERO;
}


Call&
Call::sendNoReply( const unsigned p, const LQIO::DOM::Call * value )
{
    if ( hasRendezvous() || hasForwarding() ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    } else if ( !_calls.at(p-1) ) {
	_callType = LQIO::DOM::Call::Type::SEND_NO_REPLY;
	_calls[p-1] = value;
	if ( _arc ) {
	    _arc->arrowhead(Graphic::OPEN_ARROW);
	}
    }
    return *this;
}

Call&
Call::sendNoReply( const unsigned p, const double value )
{
    if ( hasRendezvous() || hasForwarding() ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    } else if ( !_calls.at(p-1) ) {
	_callType = LQIO::DOM::Call::Type::SEND_NO_REPLY;
	const_cast<LQIO::DOM::Call *>(_calls[p-1])->setCallMeanValue(value);
	if ( _arc ) {
	    _arc->arrowhead(Graphic::OPEN_ARROW);
	}
    }
    return *this;
}

const LQIO::DOM::ExternalVariable &
Call::forward() const
{
    if ( _forwarding ) return *_forwarding->getCallMean();
    else return Element::ZERO;
}


Call&
Call::forward( const LQIO::DOM::Call * value )
{
    if ( !hasSendNoReply() ) {
	_forwarding = value;
	if ( _arc ) {
	    _arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	    if ( !Flags::print_forwarding_by_depth && _arc->nPoints() == 2 ) {
		_arc->resize( 4 );
	    }
	}
    } else {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    }
    return *this;
}


const LQIO::DOM::ExternalVariable *
Call::sum_of_calls( const LQIO::DOM::ExternalVariable * augend, const LQIO::DOM::Call * addend ) 
{
    if ( addend == nullptr ) return augend;
    else return Entity::addExternalVariables( augend, addend->getCallMean() );
}

/* --- */

const LQIO::DOM::Call *
Call::getDOM( const unsigned int p ) const
{
    if ( 0 < p && p <= _calls.size() && _calls.at(p-1) ) {
	return _calls[p-1];
    } else {
	return _forwarding;
    }
}


/*
 * Return true if the source and destination calls are compatible.
 */

bool
Call::equalType( const Call& dst ) const
{
    return _callType == LQIO::DOM::Call::Type::NULL_CALL
	|| ((hasRendezvous() || hasForwarding()) && (dst.hasRendezvous() || dst.hasForwarding()))
	|| (hasSendNoReply() && dst.hasSendNoReply());
}


bool
Call::hasWaiting() const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	LQIO::DOM::Call * dom = const_cast<LQIO::DOM::Call *>(getDOM(p));
	if ( dom && dom->getCallMean() != nullptr ) return true;
    }

    return false;
}


Call&
Call::waiting( const unsigned p, const double value )
{
    LQIO::DOM::Call * dom = const_cast<LQIO::DOM::Call *>(getDOM(p));
    if ( dom && value > 0.0 ) {
	dom->setResultWaitingTime( value );
    }
    return *this;
}

double
Call::waiting(unsigned int p) const
{
    const LQIO::DOM::Call * dom = getDOM(p);
    return dom ? dom->getResultWaitingTime() : 0.0;
}



double
Call::waitVariance(unsigned int p) const
{
    const LQIO::DOM::Call * dom = getDOM(p);
    return dom->getResultWaitingTimeVariance();
}


Call&
Call::waitVariance( const unsigned p, const double value )
{
    LQIO::DOM::Call * dom = const_cast<LQIO::DOM::Call *>(getDOM(p));
    if ( dom && value > 0.0 ) {
	dom->setResultWaitingTimeVariance( value );
	__hasVariance = true;
    }
    return *this;
}

bool
Call::hasDropProbability( const unsigned p ) const
{
    const LQIO::DOM::Call * dom = getDOM(p);
    return dom && dom->getResultDropProbability() > 0.0;
}


double
Call::dropProbability(unsigned int p) const
{
    const LQIO::DOM::Call * dom = getDOM(p);
    return dom->getResultDropProbability();
}


Call&
Call::dropProbability( const unsigned p, const double value )
{
    LQIO::DOM::Call * dom = const_cast<LQIO::DOM::Call *>(getDOM(p));
    if ( dom && value > 0.0 ) {
	dom->setResultDropProbability( value );
    }
    return *this;
}


/* --- */

bool
Call::hasDropProbability() const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	if ( hasDropProbability(p) ) return true;
    }
    return false;
}

bool
Call::hasInfiniteWait() const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	const LQIO::DOM::Call * dom = getDOM(p);
	if ( dom && !std::isfinite( dom->getResultWaitingTime() ) )  return true;
    }
    return false;
}

/*
 * Return true if the destination task is selected for drawing.
 */

bool
Call::isSelected() const
{
    return (dstTask()->isSelected()
	    && (((submodel_output() || queueing_output())
		 && (isPseudoCall() || (!isPseudoCall() && !hasForwarding())))
		|| (!submodel_output() && !queueing_output() && !isPseudoCall())))
	|| queueing_output()
	|| (!partial_output()
	    && (!queueing_output()
		&& Flags::print[CHAIN].opts.value.i == 0
		&& !isPseudoCall())
	    || (Flags::print[CHAIN].opts.value.i != 0
		&& dstTask()->isSelectedIndirectly()));
}



bool
Call::isLoopBack() const
{
    return dynamic_cast<const Task *>(srcTask()) == dstTask();
}


/*
 * Return the name of the destination entry.
 */

const std::string&
Call::dstName() const
{
    return dstEntry()->name();
}



double
Call::dstIndex() const
{
    return dstEntry()->index();
}



unsigned
Call::dstLevel() const
{
    return dstTask()->level();
}


/*
 * Return true if a rendezvous has been specified (though not necessarily set)
 */

bool
Call::hasRendezvousForPhase( const unsigned p ) const
{
    return hasRendezvous() && (p-1) < _calls.size() && _calls[p-1] != nullptr;
}


bool
Call::hasSendNoReplyForPhase( const unsigned p ) const
{
    return hasSendNoReply() && (p-1) < _calls.size() && _calls[p-1] != nullptr;
}


Graphic::colour_type
Call::colour() const
{
    switch ( Flags::print[COLOUR].opts.value.i ) {
    case COLOUR_RESULTS:
    case COLOUR_DIFFERENCES:
	if ( hasDropProbability() || hasInfiniteWait() ) {
	    return Graphic::RED;
	} else {
	    return dstTask()->colour();
	}
    case COLOUR_SERVER_TYPE:
    case COLOUR_OFF:
	return GenericCall::colour();
    }
    if ( hasAncestorLevel() || hasForwardingLevel() ) {
	return dstEntry()->colour();
    } else {
	return Graphic::RED;
    }
}


Call&
Call::moveDst( const Point& aPoint )
{
    GenericCall::moveDst( aPoint );
    const double delta_y = Flags::print[Y_SPACING].opts.value.f / 3.0;
    /* Hack to stagger labels */
    const std::vector<GenericCall *>& callers = dstEntry()->callers();
    unsigned int even = 1;
    for ( unsigned int i = 0; i < callers.size(); ++i ) {
	if ( callers[i] == this ) {
	    even = (i % 2) + 1;
	    break;
	}
    }
    if ( Flags::label_justification == ABOVE_JUSTIFY ) {
	/* Move all labels above entry */
	Point tempPoint = _arc->dstPoint();
	tempPoint.moveBy( 0, delta_y * even );
	_label->moveTo( tempPoint ).justification( CENTER_JUSTIFY );
    } else {
	double offset;
	const Point& p1 = _arc->penultimatePoint();
	const Point& p2 = _arc->dstPoint();
	if ( p1.y() > p2.y() ) {
	    offset = delta_y * even;
	    if ( hasForwardingLevel() ) {
		std::vector<GenericCall *>::const_iterator pos = find_if( dstEntry()->callers().begin(), dstEntry()->callers().end(), EQ<GenericCall>( this ) );
		if ( pos != dstEntry()->callers().end() && (pos - dstEntry()->callers().begin()) % 2 ) {
		    offset = delta_y * 2;
		}
	    }
	} else {
	    offset = delta_y + Flags::icon_height;
	}
	Point tempPoint = _arc->pointFromDst( offset );
	_label->moveTo( tempPoint );
    }
    return *this;
}



Call&
Call::label()
{
    if ( Flags::print[INPUT_PARAMETERS].opts.value.b ) {
	if ( hasNoCall() && Flags::print[COLOUR].opts.value.i != COLOUR_OFF ) {
	    _label->colour( Graphic::RED );
	}
	if ( Flags::print[AGGREGATION].opts.value.i != AGGREGATE_ENTRIES ) {
	    *_label << '(' << print_calls(*this) << ')';
	}
	const LQIO::DOM::ExternalVariable& fan_out = srcTask()->fanOut( dstTask() );
	if ( LQIO::DOM::ExternalVariable::isPresent( &fan_out, 1.0 )  ) {
	    *_label << ", O=" << fan_out;
	}
	const LQIO::DOM::ExternalVariable& fan_in = dstTask()->fanIn( srcTask() );
	if ( LQIO::DOM::ExternalVariable::isPresent( &fan_in, 1.0 ) ) {
	    *_label << ", I=" << fan_in;
	}
    }
    if ( Flags::have_results ) {
	Graphic::colour_type c = (hasDropProbability() || hasInfiniteWait()) ? Graphic::RED : Graphic::DEFAULT_COLOUR;
	if ( Flags::print[WAITING].opts.value.b && hasWaiting() ) {
	    _label->newLine().colour(c) << begin_math() << print_wait(*this) << end_math();
	}
	if ( Flags::print[LOSS_PROBABILITY].opts.value.b && hasDropProbability() ) {
	    _label->newLine().colour(c) << begin_math( &Label::epsilon ) << "=" << print_drop_probability(*this) << end_math();
	}
    }
    return *this;
}


#if defined(REP2FLAT)
/*
 * Clone the call from srcEntry
 */

Call&
Call::expand( const Entry& srcEntry )
{
    const unsigned int num_replicas = srcEntry.owner()->replicasValue();

    unsigned int next_dst_id = 1;
    const unsigned int dst_replicas = dstEntry()->owner()->replicasValue();
    for ( unsigned src_replica = 1; src_replica <= num_replicas; src_replica++ ) {
	assert( fanOut() <= dst_replicas );
	Entry *src_entry = Entry::find_replica( srcEntry.name(), src_replica );
	LQIO::DOM::Entry * src_dom = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(src_entry->getDOM()));

	const unsigned int fan_out = fanOut();
	for ( unsigned int k = 1; k <= fan_out; k++ ) {
	    /* divide the destination entries equally between calling entries. */
	    const int dst_replica = (next_dst_id++ - 1) % dst_replicas + 1;
	    Entry *dst_entry = Entry::find_replica(dstEntry()->name(), dst_replica);
	    LQIO::DOM::Entry * dst_dom = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(dst_entry->getDOM()));

	    for (unsigned int p = 1; p <= MAX_PHASES; p++) {
		LQIO::DOM::Phase * dom_phase = const_cast<LQIO::DOM::Phase *>(src_entry->getPhaseDOM(p));
		if ( !dom_phase || (!hasRendezvousForPhase(p) && !hasSendNoReplyForPhase(p)) ) continue;
		LQIO::DOM::Call * dom_call = getDOM(p)->clone();
		dom_call->setDestinationEntry( dst_dom );
#if BUG_299
		dom_call->setCallMeanValue( dom_call->getCallMeanValue() / fan_out );			    /*+ BUG 299 */
#endif
		if ( hasRendezvousForPhase(p) ) {
		    src_entry->rendezvous(dst_entry, p, dom_call );
		} else if ( hasSendNoReplyForPhase(p) ) {
		    src_entry->sendNoReply(dst_entry, p, dom_call );
		}
		dom_phase->addCall( dom_call );

		if ( src_replica == 1 ) {
		    Element::cloneObservations( getDOM(p), dom_call );
		}
	    }
	    if ( hasForwarding() ) {
		LQIO::DOM::Call * dom_call = getFwdDOM()->clone();
		dom_call->setDestinationEntry( dst_dom );
#if BUG_299
		dom_call->setCallMeanValue( dom_call->getCallMeanValue() / fan_out );			    /*+ BUG 299 */
#endif
		src_entry->forward( dst_entry, dom_call );
		src_dom->addForwardingCall( dom_call );
		if ( src_replica == 1 ) {
		    Element::cloneObservations( getFwdDOM(), dom_call );
		}
	    }
	}
    }
    return *this;
}



Call&
Call::replicateCall( std::vector<Call *>& calls, Call ** root )
{
    const static struct {
	set_function first;
	get_function second;
    } call_mean[] = {
// static std::pair<set_function,get_function> call_mean[] = {
	{ &LQIO::DOM::DocumentObject::setResultWaitingTime, &LQIO::DOM::DocumentObject::getResultWaitingTime },
	{ &LQIO::DOM::DocumentObject::setResultDropProbability, &LQIO::DOM::DocumentObject::getResultDropProbability },
	{ &LQIO::DOM::DocumentObject::setResultVarianceWaitingTime, &LQIO::DOM::DocumentObject::getResultVarianceWaitingTime },
	{ nullptr, nullptr }
    };

    const static struct {
	set_function first;
	get_function second;
    } call_variance[] = {
//static std::pair<set_function,get_function> call_variance[] = {
	{ &LQIO::DOM::DocumentObject::setResultDropProbabilityVariance, &LQIO::DOM::DocumentObject::getResultDropProbabilityVariance },
	{ &LQIO::DOM::DocumentObject::setResultVarianceWaitingTimeVariance, &LQIO::DOM::DocumentObject::getResultVarianceWaitingTimeVariance },
	{ &LQIO::DOM::DocumentObject::setResultWaitingTimeVariance, &LQIO::DOM::DocumentObject::getResultWaitingTimeVariance },
	{ nullptr, nullptr }
    };

    const Entry * dst = dstEntry();
    unsigned int replica = 0;
    dst->baseReplicaName( replica );
    if ( replica == 1 ) {
	*root = this;
	calls.push_back( this );
    } else {
	/* DOM Copy will be a little trickier. */
	for ( unsigned p = 1; p <= (*root)->maxPhase(); ++p ) {
	    LQIO::DOM::Call * dst = const_cast<LQIO::DOM::Call *>((*root)->getDOM(p));
	    LQIO::DOM::Call * src = const_cast<LQIO::DOM::Call *>(getDOM(p));
	    if ( !dst || !src ) continue;
	    for ( unsigned int i = 0; call_mean[i].first != nullptr; ++i ) {
		update_mean( dst, call_mean[i].first, src, call_mean[i].second, replica );
		update_variance( dst, call_variance[i].first, src, call_variance[i].second );
	    }
	}
    }
    return *this;
}
#endif

std::ostream&
Call::print( std::ostream& output ) const
{
    if ( isPseudoCall() ) return output;

    if ( hasRendezvous() ) {
	printSRVNLine( output, 'y', print_rendezvous );
    }
    if ( hasSendNoReply() ) {
	printSRVNLine( output, 'z', print_sendnoreply );
    }
    if ( hasForwarding() ) {
	printSRVNLine( output, 'F', print_forwarding );
    }
    return output;
}



std::ostream&
Call::printSRVNLine( std::ostream& output, char code, print_func_ptr func ) const
{
    output << "  " << code << " " 
	   << srcName() << " " 
	   << dstName() << " " 
	   << (*func)( *this ) << " -1" << std::endl;
    return output;
}


void
Call::dump() const
{
    std::cout << "Call";
    GenericCall::dump();
    std::cout << "(";
    for ( std::vector<const LQIO::DOM::Call *>::const_iterator p = _calls.begin(); p != _calls.end(); ++p ) {
	if ( p != _calls.begin() ) std::cout << ",";
	if ( *p != nullptr ) std::cout << *(*p)->getCallMean();
	else std::cout << "NULL";
    }
    std::cout << ")";
    if ( _forwarding ) {
	std::cout << ", FWD" << *_forwarding->getCallMean();
    }
    std::cout << std::endl;
}

/* ------------------------ Exception Handling ------------------------ */

Call::cycle_error::cycle_error( const CallStack& callStack )
    : std::runtime_error( std::accumulate( callStack.rbegin(), callStack.rend(), callStack.back()->dstName(), Call::cycle_error::fold ) ),
      _depth(callStack.size())
{
}

std::string
Call::cycle_error::fold( const std::string& s1, const Call * c2 )
{
    if ( c2 != nullptr ) {		/* Top of stack may be null */
	return s1 + ", " + c2->srcName();
    } else {
	return s1;
    }
}

/* ------------------------ Exception Handling ------------------------ */
EntryCall::EntryCall( const Entry * fromEntry, const Entry * toEntry )
    : Call( toEntry, MAX_PHASES ), _source(fromEntry)
{
}


/*
 * Deep copy.
 */

EntryCall::EntryCall( const EntryCall& src )
    : Call(src),
      _source(src._source)
{
}


EntryCall::~EntryCall()
{
    _source = nullptr;			/* Calling entry.		*/
}



/*
 * Check replication
 */

bool
EntryCall::check() const
{
    bool rc =true;
    /* Check */

    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	double value = 0.;
	char p_str[2];
	p_str[0] = "0123"[p];
	p_str[1] = '\0';
	if ( (hasRendezvousForPhase(p) && rendezvous(p).wasSet() && rendezvous(p).getValue( value ))
	     || (hasSendNoReplyForPhase(p) && sendNoReply(p).wasSet() && sendNoReply(p).getValue( value )) ) {
	    try {
		if ( std::isinf(value) ) throw std::domain_error( "infinity" );
		if ( value < 0.0 ) {
		    std::stringstream ss;
		    ss << value << " < " << 0;
		    throw std::domain_error( ss.str() );
		}
		if ( srcEntry()->phaseTypeFlag(p) == LQIO::DOM::Phase::Type::DETERMINISTIC && value != rint(value) ) throw std::domain_error( "invalid integer" );
	    }
	    catch ( const std::domain_error& e ) {
		LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "entry", srcName().c_str(), "phase", p_str, dstName().c_str(), e.what() );
		rc = false;
	    }
	}
    }

    if ( !checkReplication() ) rc = false;

    return rc;
}



/*
 * Return the name of the source entry.
 */

const std::string& 
EntryCall::srcName() const
{
    return srcEntry()->name();
}



/*
 * Return the source task of this call.  Sources are always tasks.
 */

const Task *
EntryCall::srcTask() const
{
    return srcEntry()->owner();
}


double
EntryCall::srcIndex() const
{
    return srcEntry()->index();
}

unsigned
EntryCall::maxPhase() const
{
    return srcEntry()->maxPhase();
}


LQIO::DOM::Phase::Type
EntryCall::phaseTypeFlag( const unsigned p ) const
{
    return srcEntry()->phaseTypeFlag(p);
}


Call *
EntryCall::addForwardingCall( Entry * toEntry, const double rate )
{
    Call * aCall = 0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	aCall = const_cast<Entry *>(srcEntry())->forwardingRendezvous( toEntry, p, rate * LQIO::DOM::to_double(rendezvous(p)) );
    }

    return aCall;
}


EntryCall&
EntryCall::setChain( const unsigned k )
{
    if ( hasSendNoReply() ) {
	const_cast<Entry *>(srcEntry())->setClientOpenChain( k );
    } else {
	const_cast<Entry *>(srcEntry())->setClientClosedChain( k );
    }
    const_cast<Entry *>(dstEntry())->setServerChain( k );
    return *this;
}


Graphic::colour_type
EntryCall::colour() const
{
    switch ( Flags::print[COLOUR].opts.value.i ) {
    case COLOUR_CLIENTS:
	return srcEntry()->colour();

    }
    return Call::colour();
}

ProxyEntryCall::ProxyEntryCall( const Entry * fromEntry, const Entry * toEntry )
    : EntryCall( fromEntry, toEntry ), myProxy(0)
{
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
    }
}

PseudoEntryCall::PseudoEntryCall( const Entry * fromEntry, const Entry * toEntry )
    : EntryCall( fromEntry, toEntry )
{
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED_DOTTED);
    }
}

ActivityCall::ActivityCall( const Activity * fromActivity, const Entry * toEntry )
    : Call( toEntry, 1 ),
      _source(fromActivity)
{
}


ActivityCall::ActivityCall( const ActivityCall& src )
    : Call( src ),
      _source( src._source )
{
}


ActivityCall::~ActivityCall()
{
    _source = nullptr;
}



/*
 * Check replication
 */

bool
ActivityCall::check() const
{
    bool rc = true;

    /* Check */

    double value = 0.0;
    if ( ( hasRendezvous() && rendezvous().wasSet() && rendezvous().getValue(value) )
	 || ( sendNoReply().wasSet() && sendNoReply().getValue(value) ) ) {
	try {
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value < 0.0 ) {
		std::stringstream ss;
		ss << value << " < " << 0;
		throw std::domain_error( ss.str() );
	    }
	    if ( srcActivity()->phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC && value != rint(value) ) throw std::domain_error( "invalid integer" );
	}
	catch ( const std::domain_error& e ) {
	    LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER, "task", srcTask()->name().c_str(), "activity", srcName().c_str(), dstName().c_str(), e.what() );
	    rc = false;
	}
    }

    if ( !checkReplication() ) rc = false;

    return rc;
}



/*
 * Return the name of the source entry.
 */

const std::string&
ActivityCall::srcName() const
{
    return srcActivity()->name();
}

/*
 * Return the source task of this call.  Sources are always tasks.
 */

const Task *
ActivityCall::srcTask() const
{
    return srcActivity()->owner();
}


double
ActivityCall::srcIndex() const
{
    return srcActivity()->index();
}

ActivityCall&
ActivityCall::setChain( const unsigned k )
{
    if ( hasSendNoReply() ) {
	const_cast<Activity *>(srcActivity())->setClientOpenChain( k );
    } else {
	const_cast<Activity *>(srcActivity())->setClientClosedChain( k );
    }
    const_cast<Entry *>(dstEntry())->setServerChain( k );
    return *this;
}


LQIO::DOM::Phase::Type
ActivityCall::phaseTypeFlag( const unsigned ) const
{
    return srcActivity()->phaseTypeFlag();
}


Call *
ActivityCall::addForwardingCall( Entry * toEntry, const double rate )
{
    return const_cast<Activity *>(srcActivity())->forwardingRendezvous( toEntry, rate * LQIO::DOM::to_double(rendezvous()) );
}

Graphic::colour_type
ActivityCall::colour() const
{
    switch ( Flags::print[COLOUR].opts.value.i ) {
    case COLOUR_CLIENTS:
	return srcActivity()->colour();

    }
    if ( hasAncestorLevel() || hasForwardingLevel() ) {
	return dstEntry()->colour();
    } else {
	return Graphic::RED;
    }
}


/*
 * Don't print the -1.
 */

std::ostream&
ActivityCall::printSRVNLine( std::ostream& output, char code, print_func_ptr func ) const
{
    if ( isPseudoCall() ) return output;

    output << "  " << code << " " 
	   << srcName() << " " 
	   << dstName() << " " 
	   << (*func)( *this ) << std::endl;
    return output;
}

ProxyActivityCall::ProxyActivityCall( const Activity * fromActivity, const Entry * toEntry )
    : ActivityCall( fromActivity, toEntry ), myProxy(0)
{
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
    }
}

Reply::Reply( const Activity * fromActivity, const Entry * toEntry )
    : ActivityCall( fromActivity, toEntry )
{
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
    }
    const_cast<Entry *>(toEntry)->addActivityReplyArc( this );
}


Reply::~Reply()
{
    const_cast<Entry *>(dstEntry())->deleteActivityReplyArc( this );
}


Graphic::colour_type
Reply::colour() const
{
    return dstEntry()->colour();
}

/* ------------------ Calls to entities from tasks. ------------------- */

EntityCall::EntityCall( const EntityCall& src )
    : GenericCall( src ),
      _srcTask(src._srcTask),
      _dstEntity(src._dstEntity)
{
}


const std::string&
EntityCall::srcName() const
{
    return srcTask()->name();
}

const std::string &
EntityCall::dstName() const
{
    return dstEntity()->name();
}

double
EntityCall::dstIndex() const
{
    return dstEntity()->index();
}

unsigned
EntityCall::dstLevel() const
{
    return dstEntity()->level();
}


#if defined(BUG_270)
EntityCall&
EntityCall::updateRateFrom( const Call& src )
{
    return *this;
}
#endif

/* -------------------- Calls to tasks from tasks. -------------------- */

TaskCall::TaskCall( const Task * fromTask, const Task * toTask )
    : EntityCall( fromTask, toTask ),
      _rendezvous(0.),
      _sendNoReply(0.),
      _forwarding(0.)
{
}


TaskCall::~TaskCall()
{
}


TaskCall::TaskCall( const TaskCall& src )
    : EntityCall( src._srcTask, src._dstEntity ),
      _rendezvous(src._rendezvous),
      _sendNoReply(src._sendNoReply),
      _forwarding(src._forwarding)
{
}


int
TaskCall::operator==( const TaskCall& item ) const
{
    return this == &item;
}


TaskCall&
TaskCall::rendezvous( const LQIO::DOM::ConstantExternalVariable& value )
{
    _rendezvous = value;
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW);
    }
    return *this;
}

const LQIO::DOM::ExternalVariable *
TaskCall::sumOfRendezvous() const
{
    return &rendezvous();
}



TaskCall&
TaskCall::sendNoReply( const LQIO::DOM::ConstantExternalVariable& value )
{
    _sendNoReply = value;
    if ( !hasRendezvous() && !hasForwarding() && _arc ) {
	_arc->arrowhead(Graphic::OPEN_ARROW);
    }
    return *this;
}



double
TaskCall::sumOfSendNoReply() const
{
    return LQIO::DOM::to_double( sendNoReply() );
}



unsigned
TaskCall::fanIn() const
{
    return dstEntity()->fanInValue( srcTask() );
}

unsigned
TaskCall::fanOut() const
{
    return srcTask()->fanOutValue( dstEntity() );
}


TaskCall&
TaskCall::taskForward( const LQIO::DOM::ConstantExternalVariable& value)
{
    _forwarding = value;
    if ( !hasRendezvous() && _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	if ( !Flags::print_forwarding_by_depth && _arc->nPoints() == 2 ) {
	    _arc->resize( 4 );
	}
    }
    return *this;
}


bool
TaskCall::isSelected() const
{
    return (dstEntity()->isSelected()
	    && ( ((submodel_output() || queueing_output())
		  && (isPseudoCall() || (!isPseudoCall() && !hasForwarding())))
		|| (!submodel_output()
		    && !queueing_output()
		    && !isPseudoCall()) ))
	|| (!partial_output()
	    && (!queueing_output()
		&& Flags::print[CHAIN].opts.value.i == 0
		&& !isPseudoCall())
	    || (Flags::print[CHAIN].opts.value.i != 0
		&& dstEntity()->isSelectedIndirectly()));
}

bool TaskCall::hasRendezvous() const
{
    double value;
    return _rendezvous.wasSet() && _rendezvous.getValue( value ) && value > 0.0;
}

bool TaskCall::hasSendNoReply() const
{
    double value;
    return _sendNoReply.wasSet() && _sendNoReply.getValue( value ) && value > 0.0;
}

bool TaskCall::hasForwarding() const
{
    double value;
    return _forwarding.wasSet() && _forwarding.getValue( value ) && value > 0.0;
}


bool
TaskCall::isLoopBack() const
{
    return dynamic_cast<const Task *>(srcTask()) == dstEntity();
}



TaskCall&
TaskCall::setChain( const unsigned k )
{
    if ( hasSendNoReply() ) {
	const_cast<Task *>(srcTask())->setClientOpenChain( k );
    } else {
	const_cast<Task *>(srcTask())->setClientClosedChain( k );
    }
    const_cast<Entity *>(dstEntity())->setServerChain( k );
    return *this;
}


Graphic::colour_type
TaskCall::colour() const
{
    switch ( Flags::print[COLOUR].opts.value.i ) {
    case COLOUR_CLIENTS:
	return srcTask()->colour();
    case COLOUR_SERVER_TYPE:
	return GenericCall::colour();

    }
    if ( hasAncestorLevel() || hasForwardingLevel() ) {
	return dstEntity()->colour();
    } else {
	return Graphic::RED;
    }
}

TaskCall&
TaskCall::moveSrc( const Point& aPoint )
{
    GenericCall::moveSrc( aPoint );		// Move to center of circle

    Point tempPoint = _arc->pointFromSrc(10);
    _label->moveTo( tempPoint );
    return *this;
}



TaskCall&
TaskCall::moveSrcBy( const double dx, const double dy )
{
    GenericCall::moveSrcBy( dx, dy );		// Move to center of circle

    Point tempPoint = _arc->pointFromSrc(10);
    _label->moveTo( tempPoint );

    return *this;
}



TaskCall&
TaskCall::label()
{
    if ( Flags::print[INPUT_PARAMETERS].opts.value.b ) {
	bool print_goop = false;
	if ( hasRendezvous() && Flags::print[PRINT_AGGREGATE].opts.value.b ) {
	    *_label << "(" << rendezvous() << ")";
	    print_goop = true;
	}
	if ( fanOut() != 1.0  ) {
	    if ( print_goop ) {
		*_label << " ";
	    } else {
		print_goop = true;
	    }
	    *_label << "O="<< fanOut();
	    print_goop = true;
	}
	if ( fanIn() != 1.0 ) {
	    if ( print_goop ) {
		*_label << ", ";
	    }
	    *_label << "I=" << fanIn();
	}
    }
    if ( dynamic_cast<const Task *>(srcTask())->hasQueueingTime() &&
	 Flags::have_results && Flags::print[PROCESSOR_QUEUEING].opts.value.b ) {
	bool print = false;
	const std::vector<Entry *>& entries = dynamic_cast<const Task *>(srcTask())->entries();
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    if ( !(*entry)->hasQueueingTime() ) continue;
	    if ( print ) _label->newLine();
	    *_label << (*entry)->name() << " w=" << print_queueing_time(**entry);
	    print = true;
	}
    }
    return *this;
}



void
TaskCall::dump() const
{
    std::cout << "EntityCall";
    GenericCall::dump();
    std::cout << "RNV" << _rendezvous;
    std::cout << ", SNR" << _sendNoReply;
    std::cout << ", FWD" << _forwarding;
    std::cout << std::endl;
}

/* -------------------- Calls to tasks from tasks. -------------------- */

ProxyTaskCall::ProxyTaskCall( const Task * fromTask, const Task * toTask )
    : TaskCall( fromTask, toTask )
{
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
    }
}


PseudoTaskCall::PseudoTaskCall( const Task * fromTask, const Task * toTask )
    : TaskCall( fromTask, toTask )
{
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED_DOTTED);
    }
}



/* ----------------- Calls to processors from tasks. ------------------ */

/*
 * A call from a task to its processor.  Normally, a task only makes
 * one call to the processor, but if we are pruning, other tasks can
 * also call the processor (via the tasks that were "pruned").  For
 * the latter case, one call is made per entry with the _visits and
 * _serviceTime set.
 */

ProcessorCall::ProcessorCall( const Task * fromTask, const Processor * toProcessor )
    : EntityCall( fromTask, toProcessor ),
      _callType(LQIO::DOM::Call::Type::NULL_CALL),	/* Not null if cloned BUG_270 */
      _demand(),				/* Not null if cloned BUG_270 */
      _source(nullptr)				/* Not null if cloned BUG_270 */
{
}


ProcessorCall::ProcessorCall( const ProcessorCall& src )
    : EntityCall( src._srcTask, src._dstEntity ),
      _callType(src._callType),
      _demand(src._demand)
{
}


ProcessorCall::~ProcessorCall()
{
}

int
ProcessorCall::operator==( const ProcessorCall& item ) const
{
    return this == &item;
}

ProcessorCall&
ProcessorCall::rendezvous( const LQIO::DOM::ExternalVariable * value )
{
    _callType = LQIO::DOM::Call::Type::RENDEZVOUS;
    _demand.setVisits(value);
    return *this;
}

const LQIO::DOM::ExternalVariable&
ProcessorCall::rendezvous() const
{
    if ( !hasRendezvous() ) return Element::ZERO;
    else if ( _demand.visits() == nullptr ) return Element::ONE;	/* Default processor call. */
    else return *_demand.visits();
}


const LQIO::DOM::ExternalVariable *
ProcessorCall::sumOfRendezvous() const
{
    if ( !hasRendezvous() ) return nullptr;
    else if ( _demand.visits() == nullptr ) return &Element::ONE;	/* Default processor call. */
    else return _demand.visits();
}


const LQIO::DOM::ExternalVariable&
ProcessorCall::sendNoReply() const
{
    if ( !hasSendNoReply() || _demand.visits() == nullptr ) return Element::ZERO;
    else return *_demand.visits();
}


double
ProcessorCall::sumOfSendNoReply() const
{
    if ( !hasSendNoReply() || _demand.visits() == nullptr ) return 0.0;
    else return to_double( *_demand.visits() );
}


double
ProcessorCall::sum_of_extvar( double augend, const LQIO::DOM::ConstantExternalVariable& addend )
{
    return augend + to_double( addend );
}

const LQIO::DOM::ExternalVariable&
ProcessorCall::forward() const
{
    throw should_not_implement( "ProcessorCall::forward()", __FILE__, __LINE__ );
}


unsigned
ProcessorCall::fanIn() const
{
    return dstEntity()->fanInValue( srcTask() );
}

unsigned
ProcessorCall::fanOut() const
{
    return 1;
}


bool
ProcessorCall::isSelected() const
{
    return ( dstEntity()->isSelected() || Flags::print[INCLUDE_ONLY].opts.value.r )
	&& ( dynamic_cast<const Processor *>(dstEntity())->isInteresting()
	     || (Flags::print[CHAIN].opts.value.i != 0 && dstEntity()->isSelectedIndirectly())
	     || submodel_output()
	     || queueing_output() );
}


ProcessorCall&
ProcessorCall::setChain( const unsigned k )
{
    if ( hasSendNoReply() ) {
	const_cast<Task *>(srcTask())->setClientOpenChain( k );
    } else {
	const_cast<Task *>(srcTask())->setClientClosedChain( k );
    }
    const_cast<Entity *>(dstEntity())->setServerChain( k );
    return *this;
}


Graphic::colour_type
ProcessorCall::colour() const
{
    switch ( Flags::print[COLOUR].opts.value.i ) {
    case COLOUR_CLIENTS:	return srcTask()->colour();
    case COLOUR_SERVER_TYPE:	return GenericCall::colour();
    }
    return dstEntity()->colour();
}

ProcessorCall&
ProcessorCall::moveSrc( const Point& aPoint )
{
    GenericCall::moveSrc( aPoint );		// Move to center of circle
    moveLabel();

    return *this;
}



ProcessorCall&
ProcessorCall::moveSrcBy( const double dx, const double dy )
{
    GenericCall::moveSrcBy( dx, dy );		// Move to center of circle
    moveLabel();
    return *this;
}



ProcessorCall&
ProcessorCall::moveDst( const Point& aPoint )
{
    GenericCall::moveDst( aPoint );		// Move to center of circle
    const Point intersect = _arc->dstIntersectsCircle( aPoint, fabs( dstEntity()->height() ) / 2 );
    GenericCall::moveDst( intersect );		// Now move to edge.

    return *this;
}



ProcessorCall&
ProcessorCall::label()
{
    if ( Flags::print[INPUT_PARAMETERS].opts.value.b && Flags::prune ) {
	if ( (hasRendezvous() || hasSendNoReply()) && _demand.visits() != nullptr ) {	/* Ignore the default */
	    *_label << '(' << *_demand.visits() << ')';
	}
    } 
    if ( !Flags::have_results ) return *this;
    const Task& src = *srcTask();
    const std::vector<Entry *>& entries = src.entries();
    const std::vector<Activity *>& activities = src.activities();
    bool do_newline = false;
    if ( Flags::print[ENTRY_UTILIZATION].opts.value.b && Flags::print[PROCESSOR_UTILIZATION].opts.value.b ) {
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    if ( !(*entry)->hasQueueingTime() || (*entry)->isActivityEntry() ) continue;
	    if ( do_newline ) _label->newLine();
	    *_label << "U[" << (*entry)->name() << "]=" << opt_pct((*entry)->processorUtilization());
	    do_newline = true;
	}
	for ( std::vector<Activity *>::const_iterator activity = activities.begin(); activity != activities.end(); ++activity ) {
	    if ( !(*activity)->hasQueueingTime() ) continue;
	    if ( do_newline ) _label->newLine();
	    *_label << "U[" << (*activity)->name() << "]=" << opt_pct( (*activity)->processorUtilization() );
	    do_newline = true;
	}
    }
    if ( src.hasQueueingTime() && Flags::print[PROCESSOR_QUEUEING].opts.value.b ) {
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    if ( !(*entry)->hasQueueingTime() || (*entry)->isActivityEntry() ) continue;
	    if ( do_newline ) _label->newLine();
	    *_label << "W[" << (*entry)->name() << "]=" << print_queueing_time(**entry);
	    do_newline = true;
	}
	for ( std::vector<Activity *>::const_iterator activity = activities.begin(); activity != activities.end(); ++activity ) {
	    if ( !(*activity)->hasQueueingTime() ) continue;
	    if ( do_newline ) _label->newLine();
	    *_label << "W[" << (*activity)->name() << "]=" << opt_pct( (*activity)->queueingTime() );
	    do_newline = true;
	}
    }
    return *this;
}


void
ProcessorCall::moveLabel()
{
    const double delta_y = Flags::print[Y_SPACING].opts.value.f / 3.0;
    /* Hack to stagger labels */
    const std::vector<GenericCall *>& callers = dstEntity()->callers();
    unsigned int even = 1;
    for ( unsigned int i = 0; i < callers.size(); ++i ) {
	if ( callers[i] == this ) {
	    even = (i % 2) + 1;
	    break;
	}
    }
    const Point tempPoint = _arc->pointFromSrc(delta_y * even);
    _label->moveTo( tempPoint );
}



#if defined(BUG_270)
ProcessorCall&
ProcessorCall::updateRateFrom( const Call& call )
{
    if ( callType() == LQIO::DOM::Call::Type::NULL_CALL ) {
	/* First time for this call.  Save visits */
	_demand.setVisits( &Element::ONE );
	/* Get service time from the phase */
	const EntryCall * entry_call = dynamic_cast<const EntryCall *>(&call);
	if ( entry_call != nullptr ) {
	    const Entry * entry = entry_call->srcEntry();
	    _demand.setServiceTime(entry->serviceTime());
	}
	_callType = call.callType();
    } else if ( callType() != call.callType() ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntity()->name().c_str() );
	return *this;
    }
    /* Preserve variable if multiplier is one (1) (which is likely is) */
    const LQIO::DOM::ExternalVariable * multiplier = call.sumOfRendezvous();
    if ( LQIO::DOM::ExternalVariable::isDefault( _demand.visits(), 1.0 ) ) {
	_demand.setVisits( multiplier );
    } else if ( LQIO::DOM::ExternalVariable::isDefault( multiplier, 1.0 ) ) {
	return *this;
    } else if ( dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(_demand.visits()) && dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(multiplier) ) {
	_demand.setVisits( new LQIO::DOM::ConstantExternalVariable( to_double(*_demand.visits()) * to_double(*multiplier) ) );
    } else {	/* More complicated... */
	_demand.setVisits( Entity::multiplyExternalVariables( _demand.visits(), multiplier ) );
    }
    return *this;
}
#endif



void
ProcessorCall::dump() const
{
    std::cout << "ProcessorCall";
    GenericCall::dump();
    std::cout << "(";
    std::cout << *_demand.visits();
    std::cout << ")";
    std::cout << std::endl;
}

/* ----------------- Calls to processors from tasks. ------------------ */

PseudoProcessorCall::PseudoProcessorCall( const Task * fromTask, const Processor * toProcessor )
    : ProcessorCall( fromTask, toProcessor )
{
    if ( _arc ) {
	_arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED_DOTTED);
    }
}

/* ---------------- Calls from Open arrivals to tasks ----------------- */

OpenArrival::OpenArrival( const OpenArrivalSource * from, const Entry * to )
    : GenericCall(),
      _source(from),
      _destination(to)
{
    _arc->arrowhead(Graphic::OPEN_ARROW);
}


OpenArrival::OpenArrival( const OpenArrival& src )
    : GenericCall(),
      _source(src._source),
      _destination(src._destination)
{
}


OpenArrival::~OpenArrival()
{
}


int
OpenArrival::operator==( const OpenArrival& item ) const
{
    return this == &item;
}


const Task *
OpenArrival::srcTask() const
{
    return _source;
}



const std::string&
OpenArrival::srcName() const
{
    return srcTask()->name();
}


const std::string&
OpenArrival::dstName() const
{
    return _destination->name();
}


double
OpenArrival::srcIndex() const
{
    return srcTask()->index();
}


double
OpenArrival::dstIndex() const
{
    return _destination->index();
}


const Task *
OpenArrival::dstTask() const
{
    return _destination->owner();
}

unsigned
OpenArrival::dstLevel() const
{
    return dstTask()->level();
}


const LQIO::DOM::ExternalVariable&
OpenArrival::openArrivalRate() const
{
    return _destination->openArrivalRate();
}


double
OpenArrival::sumOfSendNoReply() const
{
    return LQIO::DOM::to_double( sendNoReply() );
}


const LQIO::DOM::ExternalVariable&
OpenArrival::rendezvous() const
{
    throw should_not_implement( "OpenArrival::rendezvous()", __FILE__, __LINE__ );
}



const LQIO::DOM::ExternalVariable&
OpenArrival::sendNoReply() const
{
    throw should_not_implement( "OpenArrival::sendNoReply()", __FILE__, __LINE__ );
}

const LQIO::DOM::ExternalVariable&
OpenArrival::forward() const
{
    throw should_not_implement( "OpenArrival::forward()", __FILE__, __LINE__ );
}


unsigned
OpenArrival::fanIn() const
{
    should_not_implement( "OpenArrival::fanIn()", __FILE__, __LINE__ );
    return 1;
}

unsigned
OpenArrival::fanOut() const
{
    should_not_implement( "OpenArrival::fanOut()", __FILE__, __LINE__ );
    return 1;
}


double
OpenArrival::openWait() const
{
    return _destination->openWait();
}

OpenArrival&
OpenArrival::setChain( const unsigned k )
{
    const_cast<Task *>(dynamic_cast<const Task *>(srcTask()))->setClientOpenChain( k );
    const_cast<Entry *>(_destination)->setServerChain( k );
    return *this;
}

Graphic::colour_type
OpenArrival::colour() const
{
    switch ( Flags::print[COLOUR].opts.value.i ) {
    case COLOUR_SERVER_TYPE:
	return Graphic::RED;
    }

    return dstTask()->colour();
}


OpenArrival&
OpenArrival::moveDst( const Point& aPoint )
{
    GenericCall::moveDst( aPoint );

    GenericCall::moveSrc( _source->center() );	// Move to center of circle
    Point intersect = _arc->srcIntersectsCircle( _source->center(), _source->radius() );
    GenericCall::moveSrc( intersect );

    Point tempPoint = _arc->pointFromSrc(30);
    _label->moveTo( tempPoint );

    return *this;
}



OpenArrival&
OpenArrival::label()
{
    bool print = false;
    if ( Flags::print[INPUT_PARAMETERS].opts.value.b ) {
	*_label << begin_math( &Label::lambda ) << "=" << _destination->openArrivalRate() << end_math();
	print = true;
    }
    if ( _destination->openWait()
	 && Flags::have_results
	 && Flags::print[OPEN_WAIT].opts.value.b ) {
	if ( print ) _label->newLine();
	Graphic::colour_type c = std::isfinite( _destination->openWait() ) ? Graphic::DEFAULT_COLOUR : Graphic::RED;
	_label->colour(c) << begin_math() << _destination->openWait() << end_math();
	print = true;
    }
    return *this;
}


void
OpenArrival::dump() const
{
    std::cout << "OpenArrival";
    GenericCall::dump();
    std::cout << std::endl;
}

/*----------------------------------------------------------------------*/
/*                      Functions for manipulators                      */
/*----------------------------------------------------------------------*/

/*
 * We are looking for matching tasks for calls.  For lqn2ps, we only
 * stop on cycle errors that loop back to an entry, not a task like
 * with lqns, because we always want to traverse the entire tree (see
 * BUG_301).
 */

std::deque<const Call *>::const_iterator
CallStack::find( const Call * dstCall, const bool direct_path )
{
    const Entry * dstEntry = dstCall->dstEntry();
    bool broken = false;
    for ( std::deque<const Call *>::const_reverse_iterator call = rbegin(); call != rend(); ++call ) {
	if ( !(*call) ) continue;
	if ( (*call)->hasSendNoReply() ) broken = true;		/* Cycle broken - async call */

	if ( (*call)->dstEntry() == dstEntry ) {		/* Cycle detected. */
	    if ( (*call)->hasRendezvous() && dstCall->hasRendezvous() && !broken ) {
		throw Call::cycle_error( *this );		/* Dead lock */
	    } if ( (*call)->dstEntry() == dstEntry && direct_path ) {
		throw Call::cycle_error( *this );		/* Live lock */
	    } else {
		return (call+1).base();
	    }
	}
    }
    return end();
}



/*
 * We may skip back over forwarded calls when computing the size.
 */

size_t
CallStack::size() const
{
    const size_t sz = std::deque<const Call *>::size();
    if ( Flags::print_forwarding_by_depth ) {
	return sz;
    } else {
	size_t k = 0;
	for ( std::deque<const Call *>::const_iterator call = begin(); call != end(); ++call ) {
	    if ( !(*call) || (*call)->hasRendezvous() || (*call)->hasSendNoReply() ) {
		k += 1;
	    }
	}
	return k;
    }
}

/*----------------------------------------------------------------------*/
/*                      Functions for manipulators                      */
/*----------------------------------------------------------------------*/

static std::ostream&
format_prologue( std::ostream& output, const Call& aCall, int p )
{
    switch( Flags::print[OUTPUT_FORMAT].opts.value.o ) {
    case file_format::EEPIC:
    case file_format::PSTEX:
	if ( p != 1 ) {
	    output << ',';
	}
	if ( aCall.phaseTypeFlag(p) == LQIO::DOM::Phase::Type::DETERMINISTIC ) {
	    output << "\\fbox{";
	}
	break;
    case file_format::OUTPUT:
    case file_format::PARSEABLE:
    case file_format::RTF:
	output << std::setw( maxDblLen );
	break;
    case file_format::POSTSCRIPT:
    case file_format::FIG:
    case file_format::SVG:
	if ( p != 1 ) {
	    output << ',';
	}
	break;
    default:
	if ( p != 1 ) {
	    output << ' ';
	}
	break;
    }
    return output;
}

static std::ostream&
format_epilogue( std::ostream& output, const Call& aCall, int p )
{
    switch( Flags::print[OUTPUT_FORMAT].opts.value.o ) {
    case file_format::EEPIC:
    case file_format::PSTEX:
	if ( aCall.phaseTypeFlag(p) == LQIO::DOM::Phase::Type::DETERMINISTIC ) {
	    output << "}";
	}
	break;
    case file_format::POSTSCRIPT:
    case file_format::SVG:
    case file_format::FIG:
	if ( aCall.phaseTypeFlag(p) == LQIO::DOM::Phase::Type::DETERMINISTIC ) {
	    output << ":D";
	}
	break;
    }
    return output;
}

static std::ostream&
rendezvous_of_str( std::ostream& output, const Call& aCall )
{
    for ( unsigned p = 1; p <= aCall.maxPhase(); ++p ) {
	format_prologue( output, aCall, p );
	output << aCall.rendezvous(p);
	format_epilogue( output, aCall, p );
    }
    return output;
}


static std::ostream&
sendnoreply_of_str( std::ostream& output, const Call& aCall )
{
    for ( unsigned p = 1; p <= aCall.maxPhase(); ++p ) {
	format_prologue( output, aCall, p );
	output << aCall.sendNoReply(p);
	format_epilogue( output, aCall, p );
    }
    return output;
}


static std::ostream&
forwarding_of_str( std::ostream& output, const Call& aCall )
{
    output << aCall.forward();
    return output;
}


static std::ostream&
fanin_of_str( std::ostream& output, const Call& aCall )
{
    output << aCall.fanIn();
    return output;
}


static std::ostream&
fanout_of_str( std::ostream& output, const Call& aCall )
{
    output  << aCall.fanOut();
    return output;
}


static Label&
wait_of_str( Label& aLabel, const Call& aCall )
{
    for ( unsigned p = 1; p <= aCall.maxPhase(); ++p ) {
	if ( p != 1 ) {
	    aLabel << ',';
	}
	aLabel << opt_pct(aCall.waiting(p));
    }
    return aLabel;
}



static Label&
drop_probability_of_str( Label& aLabel, const Call& aCall )
{
    for ( unsigned p = 1; p <= aCall.maxPhase(); ++p ) {
	if ( !aCall.hasDropProbability( p ) ) break;
	if ( p != 1 ) {
	    aLabel << ',';
	}
	aLabel << opt_pct(aCall.dropProbability(p));
    }
    return aLabel;
}



static std::ostream&
calls_of_str( std::ostream& output, const Call& aCall )
{

    if ( aCall.hasRendezvous() ) {
	rendezvous_of_str( output, aCall );
    } else if ( aCall.hasSendNoReply() ) {
	sendnoreply_of_str( output, aCall );
    } else {
	forwarding_of_str( output, aCall );
    }
    return output;
}


static SRVNCallManip
print_calls( const Call& aCall )
{
    return SRVNCallManip( &calls_of_str, aCall );
}

SRVNCallManip
print_rendezvous( const Call& aCall )
{
    return SRVNCallManip( &rendezvous_of_str, aCall );
}

SRVNCallManip
print_sendnoreply( const Call& aCall )
{
    return SRVNCallManip( &sendnoreply_of_str, aCall );
}

SRVNCallManip
print_forwarding( const Call& aCall )
{
    return SRVNCallManip( &forwarding_of_str, aCall );
}


SRVNCallManip
print_fanin( const Call& aCall )
{
    return SRVNCallManip( &fanin_of_str, aCall );
}


SRVNCallManip
print_fanout( const Call& aCall )
{
    return SRVNCallManip( &fanout_of_str, aCall );
}


LabelCallManip
print_wait( const Call& aCall )
{
    return LabelCallManip( &wait_of_str, aCall );
}


static LabelCallManip
print_drop_probability( const Call& aCall )
{
    return LabelCallManip( &drop_probability_of_str, aCall );
}
