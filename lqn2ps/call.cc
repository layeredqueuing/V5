/*  -*- c++ -*-
 * $Id: call.cc 14149 2020-11-27 13:16:29Z greg $
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

bool Call::hasVariance = false;

/*----------------------------------------------------------------------*/
/*                            Generic  Calls                            */
/*----------------------------------------------------------------------*/

GenericCall::GenericCall() 
    : myLabel(0), myArc(0)
{
    myArc = Arc::newArc();
    myLabel = Label::newLabel();
    myLabel->justification( Flags::label_justification );
}


GenericCall::~GenericCall()
{
    delete myArc;
    delete myLabel;
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
    myArc->moveSrc( aPoint ); 
    return *this; 
} 



GenericCall&
GenericCall::moveSrcBy( const double dx, const double dy )
{ 
    myArc->moveSrcBy( dx, dy ); 
    return *this; 
} 



GenericCall&
GenericCall::moveDst( const Point& aPoint )
{
    myArc->moveDst( aPoint );
    return *this;
}



GenericCall&
GenericCall::moveSecond( const Point& aPoint )
{ 
    myArc->moveSecond( aPoint ); 
    return *this; 
} 



GenericCall&
GenericCall::movePenultimate( const Point& aPoint )
{
    myArc->movePenultimate( aPoint ); 
    return *this;
}



GenericCall& 
GenericCall::scaleBy( const double sx, const double sy )
{
    myArc->scaleBy( sx, sy );
    myLabel->scaleBy( sx, sy );
    return *this;
}



GenericCall& 
GenericCall::translateY( const double dy )
{
    myArc->translateY( dy );
    myLabel->translateY( dy );
    return *this;
}



GenericCall& 
GenericCall::depth( const unsigned curDepth )
{
    myArc->depth( curDepth );
    myLabel->depth( curDepth-1 );
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
    myArc->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() );
    myArc->comment( output, aComment.str() );
    output << *myArc;

    myLabel->backgroundColour( Graphic::DEFAULT_COLOUR ).comment( output, aComment.str() );
    output << *myLabel;
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

/*----------------------------------------------------------------------*/
/*                          Calls between Entries                       */
/*----------------------------------------------------------------------*/

Call::Call()
    : GenericCall(),
      _destination(NULL),
      _rendezvous(),
      _sendNoReply(),
      _forwarding(NULL)
{
}

/*
 * Initialize and zero fields.   Reverse links are set here.  Forward
 * links are done by subclass.  Processor calls are linked specially.
 */

Call::Call( const Entry * toEntry, const unsigned nPhases )
    : GenericCall(),
      _destination(toEntry),
      _rendezvous(nPhases,NULL),		/* rendezvous.			*/
      _sendNoReply(nPhases,NULL),		/* send no reply.		*/
      _forwarding(NULL)
{
}


/*
 * Clean up the mess.
 */

Call::~Call()
{
    _destination = NULL;			/* to whom I am referring to	*/
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
	if ( fan_out != NULL && fan_out->wasSet() ) fan_out->getValue( fanOutValue );
	const LQIO::DOM::ExternalVariable * fan_in = dynamic_cast<const LQIO::DOM::Task *>(dstTask()->getDOM())->getFanIn( srcName );
	if ( fan_in != NULL && fan_in->wasSet() ) fan_in->getValue( fanInValue );
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

    if ( src.hasRendezvousForPhase(1) ) {
	const LQIO::DOM::Call * call = src._rendezvous[0];	/* Phases go from 1-3, vector goes from 0-2. */
	if ( !hasRendezvousForPhase(p) ) {
	    if ( p > _rendezvous.size() ) {
		_rendezvous.resize(p);				/* Make it big enough if neccessary */
	    }
	    LQIO::DOM::Call * newCall = call->clone();		/* Copy call. */
	    _rendezvous[p-1] = newCall;
	    newCall->setSourceObject(const_cast<LQIO::DOM::Phase *>(phase.getDOM()));
	    newCall->setDestinationEntry(const_cast<LQIO::DOM::Entry *>(call->getDestinationEntry()));
	    const_cast<LQIO::DOM::Phase *>(phase.getDOM())->addCall( newCall );
	} else {
	    rendezvous( p, to_double(rendezvous(p)) + call->getCallMeanValue() * rate );
	}
	deleteCall( const_cast<LQIO::DOM::Call *>( call ) );
    } else if ( src.hasSendNoReplyForPhase(1) ) {
	const LQIO::DOM::Call * call = src._sendNoReply[0];	/* Phases go from 1-3, vector goes from 0-2. */
	if ( !hasSendNoReplyForPhase(p) ) {
	    if ( p > _sendNoReply.size() ) {
		_sendNoReply.resize(p);
	    }
	    LQIO::DOM::Call * newCall = call->clone();		/* Copy call. */
	    _sendNoReply[p-1] = newCall;
	    newCall->setSourceObject(const_cast<LQIO::DOM::Phase *>(phase.getDOM()));
	    newCall->setDestinationEntry(const_cast<LQIO::DOM::Entry *>(call->getDestinationEntry()));
	    const_cast<LQIO::DOM::Phase *>(phase.getDOM())->addCall( newCall );
	} else {
	    sendNoReply( p, to_double(sendNoReply(p)) + call->getCallMeanValue() * rate );
	}
	deleteCall( const_cast<LQIO::DOM::Call *>( call ) );
    }
    /* Forwarding? */

    if ( (hasRendezvous() || hasForwarding()) && hasSendNoReply() ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    }
    if ( myArc ) {
	if ( hasSendNoReply() ) {
	    myArc->arrowhead(Graphic::OPEN_ARROW);
	} else {
	    myArc->arrowhead(Graphic::CLOSED_ARROW);
	    if ( hasForwarding() ) {
		myArc->linestyle(Graphic::DASHED);
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
	if ( _rendezvous.at(p) ) {
	    const LQIO::DOM::Call * call = _rendezvous[p];
	    if ( _rendezvous[0] == nullptr ) {
		LQIO::DOM::Call * clone = call->clone();		/* copy call to phase 1 */
		_rendezvous[0] = clone;
		clone->setSourceObject( const_cast<LQIO::DOM::DocumentObject *>(call->getSourceObject()) );
		clone->setDestinationEntry( const_cast<LQIO::DOM::Entry *>(call->getDestinationEntry()) );
		src.addCall( clone );
	    } else {
		const double rnv_src = to_double(*call->getCallMean());
		const_cast<LQIO::DOM::Call *>(_rendezvous[0])->setCallMeanValue(to_double(*_rendezvous[0]->getCallMean()) + rnv_src);
	    }
	    deleteCall( const_cast<LQIO::DOM::Call *>( call ) );
	}
	if ( _sendNoReply[p] ) {
	    const LQIO::DOM::Call * call = _sendNoReply[p];
	    if ( !_sendNoReply[0] ) {
		LQIO::DOM::Call * clone = call->clone();		/* copy call to phase 1 */
		_rendezvous[0] = clone;
		clone->setSourceObject( const_cast<LQIO::DOM::DocumentObject *>(call->getSourceObject()) );
		clone->setDestinationEntry( const_cast<LQIO::DOM::Entry *>(call->getDestinationEntry()) );
		src.addCall( clone );
	    } else {
		double snr_src = to_double(*call->getCallMean());
		const_cast<LQIO::DOM::Call *>(_sendNoReply[0])->setCallMeanValue(to_double(*_sendNoReply[0]->getCallMean()) + snr_src);
	    }
	    deleteCall( const_cast<LQIO::DOM::Call *>( call ) );
	}
    }

    setArcType();
    return *this;
}


/* 
 * Remove reference from source phase and delete the call.
 */

Call&
Call::deleteCall( LQIO::DOM::Call * call )
{
    const LQIO::DOM::Phase * phase = dynamic_cast<const LQIO::DOM::Phase *>(call->getSourceObject());
    if ( phase ) {
	const_cast<LQIO::DOM::Phase *>(phase)->eraseCall( const_cast<LQIO::DOM::Call*>(call) );
    }
    delete call;
    return *this;
}


/*
 * Set the arc type (ie., arrow and line style)
 */

Call&
Call::setArcType()
{
    if ( myArc ) {
	if ( hasSendNoReply() ) {
	    myArc->arrowhead(Graphic::OPEN_ARROW);
	} else {
	    myArc->arrowhead(Graphic::CLOSED_ARROW);
	    if ( hasForwarding() ) {
		myArc->linestyle(Graphic::DASHED);
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
    hasVariance = false;
}



double
Call::sumOfRendezvous() const
{
    double sum = 0;
    for ( std::vector<const LQIO::DOM::Call *>::const_iterator call = _rendezvous.begin(); call != _rendezvous.end(); ++call ) {
	double result = 0.0;
	if ( !(*call) ) continue;
	const LQIO::DOM::ExternalVariable * value = (*call)->getCallMean();
	if ( !value ) continue;
	else if ( !value->getValue(result) ) abort(); 		/* throw not_defined */
	else sum += result;
    }
    return sum;
}



const LQIO::DOM::ExternalVariable & 
Call::rendezvous( const unsigned p ) const
{
    if ( 0 < p && p <= _rendezvous.size() && _rendezvous[p-1] ) return *_rendezvous[p-1]->getCallMean();
    else return Element::ZERO;
}


Call& 
Call::rendezvous( const unsigned p, const LQIO::DOM::Call * value )
{
    if ( !_sendNoReply.at(p-1) ) {
	_rendezvous[p-1] = value;
	if ( myArc ) {
	    myArc->arrowhead(Graphic::CLOSED_ARROW);
	}
    } else {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    }
    return *this;
}


Call&
Call::rendezvous( const unsigned p, const double value ) {
    if ( !_sendNoReply.at(p-1) ) {
	const_cast<LQIO::DOM::Call *>(_rendezvous[p-1])->setCallMeanValue(value);
	if ( myArc ) {
	    myArc->arrowhead(Graphic::CLOSED_ARROW);
	}
    } else {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    }
    return *this;
}


double
Call::sumOfSendNoReply() const
{
    double sum = 0;
    for ( std::vector<const LQIO::DOM::Call *>::const_iterator call = _sendNoReply.begin(); call != _sendNoReply.end(); ++call ) {
	if ( !(*call) ) continue;
	const LQIO::DOM::ExternalVariable * value = (*call)->getCallMean();
	double result = 0.0;
	if ( !value ) continue;
	else if ( !value->getValue(result) ) abort(); 		/* throw not_defined */
	else sum += result;
    }
    return sum;
}



const LQIO::DOM::ExternalVariable & 
Call::sendNoReply( const unsigned p ) const
{
    if ( 0 < p && p <= _sendNoReply.size() && _sendNoReply[p-1] ) return *_sendNoReply[p-1]->getCallMean();
    else return Element::ZERO;
}


Call& 
Call::sendNoReply( const unsigned p, const LQIO::DOM::Call * value )
{ 
    if ( !_rendezvous.at(p-1) && !_forwarding ) {
	_sendNoReply[p-1] = value;
	if ( myArc ) {
	    myArc->arrowhead(Graphic::OPEN_ARROW);
	}
    } else {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    }
    return *this; 
} 

Call& 
Call::sendNoReply( const unsigned p, const double value )
{ 
    if ( !_rendezvous.at(p-1) && !_forwarding ) {
	const_cast<LQIO::DOM::Call *>(_sendNoReply[p-1])->setCallMeanValue(value);
	if ( myArc ) {
	    myArc->arrowhead(Graphic::OPEN_ARROW);
	}
    } else {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
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
    if ( !_sendNoReply[0] ) {
	_forwarding = value;
	if ( myArc ) {
	    myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	    if ( !Flags::print_forwarding_by_depth && myArc->nPoints() == 2 ) {
		myArc->resize( 4 );
	    }
	}
    } else {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, dstEntry()->name().c_str() );
    }
    return *this; 
}

/* --- */

const LQIO::DOM::Call *
Call::getDOM( const unsigned int p ) const
{
    if ( 0 < p && p <= _rendezvous.size() ) {
	if ( _rendezvous.at(p-1) ) {
	    return _rendezvous[p-1];
	} else if ( _sendNoReply.at(p-1) ) {
	    return _sendNoReply[p-1];
	} else {
	    return _forwarding;
	}
    }
    return NULL;
}


const LQIO::DOM::Call *
Call::getDOMFwd() const
{
    return _forwarding;
}

bool
Call::hasWaiting() const 
{ 
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	LQIO::DOM::Call * dom = const_cast<LQIO::DOM::Call *>(getDOM(p));
	if ( dom && dom->getCallMean() != NULL ) return true;
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
	hasVariance = true;
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
		&& Flags::print[CHAIN].value.i == 0
		&& !isPseudoCall())
	    || (Flags::print[CHAIN].value.i != 0 
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
    return (p-1) < _rendezvous.size() && _rendezvous[p-1] != NULL;
}


bool 
Call::hasRendezvous() const
{
    for ( std::vector<const LQIO::DOM::Call *>::const_iterator call = _rendezvous.begin(); call != _rendezvous.end(); ++call ) {
	if ( *call != NULL ) return true;
    }
    return false;
}



bool 
Call::hasSendNoReplyForPhase( const unsigned p ) const
{
    return (p-1) < _sendNoReply.size() && _sendNoReply[p-1] != NULL;
}


bool
Call::hasSendNoReply() const
{
    for ( std::vector<const LQIO::DOM::Call *>::const_iterator call = _sendNoReply.begin(); call != _sendNoReply.end(); ++call ) {
	if ( *call != NULL ) return true;
    }
    return false;
}


bool
Call::hasForwarding() const
{
    return _forwarding != NULL;
}


Graphic::colour_type 
Call::colour() const
{
    switch ( Flags::print[COLOUR].value.i ) {
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
    const double delta_y = Flags::print[Y_SPACING].value.f / 3.0;
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
	Point tempPoint = myArc->dstPoint();
	tempPoint.moveBy( 0, delta_y * even );
	myLabel->moveTo( tempPoint ).justification( CENTER_JUSTIFY );
    } else {
	double offset;
	const Point& p1 = myArc->penultimatePoint();
	const Point& p2 = myArc->dstPoint();
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
	Point tempPoint = myArc->pointFromDst( offset );
	myLabel->moveTo( tempPoint );
    }
    return *this;
}



Call&
Call::label()
{
    if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	if ( hasNoCall() && Flags::print[COLOUR].value.i != COLOUR_OFF ) {
	    myLabel->colour( Graphic::RED );
	}
	if ( Flags::print[AGGREGATION].value.i != AGGREGATE_ENTRIES ) {
	    *myLabel << '(' << print_calls(*this) << ')';
	}
	if ( fanOut() != 1.0  ) {
	    *myLabel << ", O="<< fanOut();
	}
	if ( fanIn() != 1.0 ) {
	    *myLabel << ", I=" << fanIn();
	}
    }
    if ( Flags::have_results ) {
	Graphic::colour_type c = (hasDropProbability() || hasInfiniteWait()) ? Graphic::RED : Graphic::DEFAULT_COLOUR;
	if ( Flags::print[WAITING].value.b && hasWaiting() ) {
	    myLabel->newLine().colour(c) << begin_math() << print_wait(*this) << end_math();
	} 
	if ( Flags::print[LOSS_PROBABILITY].value.b && hasDropProbability() ) {
	    myLabel->newLine().colour(c) << begin_math( &Label::epsilon ) << "=" << print_drop_probability(*this) << end_math();
	}
    }
    return *this;
}


#if defined(REP2FLAT)
static struct {
    set_function first;
    get_function second;
} call_mean[] = { 
// static std::pair<set_function,get_function> call_mean[] = {
    { &LQIO::DOM::DocumentObject::setResultWaitingTime, &LQIO::DOM::DocumentObject::getResultWaitingTime },
    { &LQIO::DOM::DocumentObject::setResultDropProbability, &LQIO::DOM::DocumentObject::getResultDropProbability },
    { &LQIO::DOM::DocumentObject::setResultVarianceWaitingTime, &LQIO::DOM::DocumentObject::getResultVarianceWaitingTime },
    { NULL, NULL }
};

static struct {
    set_function first;
    get_function second;
} call_variance[] = { 
//static std::pair<set_function,get_function> call_variance[] = {
    { &LQIO::DOM::DocumentObject::setResultDropProbabilityVariance, &LQIO::DOM::DocumentObject::getResultDropProbabilityVariance },
    { &LQIO::DOM::DocumentObject::setResultVarianceWaitingTimeVariance, &LQIO::DOM::DocumentObject::getResultVarianceWaitingTimeVariance },
    { &LQIO::DOM::DocumentObject::setResultWaitingTimeVariance, &LQIO::DOM::DocumentObject::getResultWaitingTimeVariance },
    { NULL, NULL }
};


Call&
Call::replicateCall( std::vector<Call *>& calls, Call ** root )
{
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
	    for ( unsigned int i = 0; call_mean[i].first != NULL; ++i ) {
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
    : Call( toEntry, MAX_PHASES ), source(fromEntry) 
{
}


EntryCall::~EntryCall()
{
    source = 0;			/* Calling entry.		*/
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
		if ( srcEntry()->phaseTypeFlag(p) == PHASE_DETERMINISTIC && value != rint(value) ) throw std::domain_error( "invalid integer" );
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


phase_type
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
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_CLIENTS:
	return srcEntry()->colour();

    }
    return Call::colour();
}

ProxyEntryCall::ProxyEntryCall( const Entry * fromEntry, const Entry * toEntry ) 
    : EntryCall( fromEntry, toEntry ), myProxy(0)
{
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
    }
}

PseudoEntryCall::PseudoEntryCall( const Entry * fromEntry, const Entry * toEntry ) 
    : EntryCall( fromEntry, toEntry )
{
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED_DOTTED);
    }
}

ActivityCall::ActivityCall( const Activity * fromActivity, const Entry * toEntry )
    : Call( toEntry, 1 ), source(fromActivity) 
{
}


ActivityCall::~ActivityCall()
{
    source = 0;
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
	    if ( srcActivity()->phaseTypeFlag() == PHASE_DETERMINISTIC && value != rint(value) ) throw std::domain_error( "invalid integer" );
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


phase_type
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
    switch ( Flags::print[COLOUR].value.i ) {
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
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
    }
}

Reply::Reply( const Activity * fromActivity, const Entry * toEntry ) 
    : ActivityCall( fromActivity, toEntry )
{
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
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

/* -------------------- Calls to tasks from tasks. -------------------- */

TaskCall::TaskCall( const Task * fromTask, const Task * toTask )
    : EntityCall( fromTask, toTask ), _rendezvous(0.), _sendNoReply(0.), _forwarding(0.)
{
}


TaskCall::~TaskCall()
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
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW);
    }
    return *this;
}

double 
TaskCall::sumOfRendezvous() const
{
    return LQIO::DOM::to_double( rendezvous() );
}



TaskCall& 
TaskCall::sendNoReply( const LQIO::DOM::ConstantExternalVariable& value ) 
{ 
    _sendNoReply = value;
    if ( !hasRendezvous() && !hasForwarding() && myArc ) {
	myArc->arrowhead(Graphic::OPEN_ARROW);
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
    if ( !hasRendezvous() && myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	if ( !Flags::print_forwarding_by_depth && myArc->nPoints() == 2 ) {
	    myArc->resize( 4 );
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
		&& Flags::print[CHAIN].value.i == 0
		&& !isPseudoCall())
	    || (Flags::print[CHAIN].value.i != 0 
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
    switch ( Flags::print[COLOUR].value.i ) {
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

    Point tempPoint = myArc->pointFromSrc(10);
    myLabel->moveTo( tempPoint );
    return *this;
}



TaskCall& 
TaskCall::moveSrcBy( const double dx, const double dy )
{
    GenericCall::moveSrcBy( dx, dy );		// Move to center of circle 

    Point tempPoint = myArc->pointFromSrc(10);
    myLabel->moveTo( tempPoint );

    return *this;
}



TaskCall&
TaskCall::label()
{
    if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	bool print_goop = false;
	if ( hasRendezvous() && Flags::print[PRINT_AGGREGATE].value.b ) {
	    *myLabel << "(" << rendezvous() << ")";
	    print_goop = true;
	}
	if ( fanOut() != 1.0  ) {
	    if ( print_goop ) {
		*myLabel << " ";
	    } else {
		print_goop = true;
	    }
	    *myLabel << "O="<< fanOut();
	    print_goop = true;
	}
	if ( fanIn() != 1.0 ) {
	    if ( print_goop ) {
		*myLabel << ", ";
	    }
	    *myLabel << "I=" << fanIn();
	}
    }
    if ( dynamic_cast<const Task *>(srcTask())->hasQueueingTime() &&
	 Flags::have_results && Flags::print[PROCESSOR_QUEUEING].value.b ) {
	bool print = false;
	const std::vector<Entry *>& entries = dynamic_cast<const Task *>(srcTask())->entries();
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    if ( !(*entry)->hasQueueingTime() ) continue;
	    if ( print ) myLabel->newLine();
	    *myLabel << (*entry)->name() << " w=" << print_queueing_time(**entry);
	    print = true;
	}
    }
    return *this;
}

/* -------------------- Calls to tasks from tasks. -------------------- */

ProxyTaskCall::ProxyTaskCall( const Task * fromTask, const Task * toTask )
    : TaskCall( fromTask, toTask )
{
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DOTTED);
    }
}


PseudoTaskCall::PseudoTaskCall( const Task * fromTask, const Task * toTask )
    : TaskCall( fromTask, toTask )
{
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED_DOTTED);
    }
}



/* ----------------- Calls to processors from tasks. ------------------ */

ProcessorCall::ProcessorCall( const Task * fromTask, const Processor * toProcessor )
    : EntityCall( fromTask, toProcessor )
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


const LQIO::DOM::ExternalVariable&
ProcessorCall::rendezvous() const
{
    abort();
}

const LQIO::DOM::ExternalVariable&
ProcessorCall::sendNoReply() const
{
    abort();
}

const LQIO::DOM::ExternalVariable&
ProcessorCall::forward() const
{
    abort();
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
    return ( dstEntity()->isSelected() 
#if HAVE_REGEX_T
	     || Flags::print[INCLUDE_ONLY].value.r 
#endif
	)
	&& ( dynamic_cast<const Processor *>(dstEntity())->isInteresting()
	     || (Flags::print[CHAIN].value.i != 0 && dstEntity()->isSelectedIndirectly())
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
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_CLIENTS:	return srcTask()->colour();
    case COLOUR_SERVER_TYPE:	return GenericCall::colour();
    }
    return dstEntity()->colour();
}

ProcessorCall& 
ProcessorCall::moveSrc( const Point& aPoint )
{
    GenericCall::moveSrc( aPoint );		// Move to center of circle 

    Point tempPoint = myArc->pointFromSrc(10);
    myLabel->moveTo( tempPoint );

    return *this;
}



ProcessorCall& 
ProcessorCall::moveSrcBy( const double dx, const double dy )
{
    GenericCall::moveSrcBy( dx, dy );		// Move to center of circle 

    Point tempPoint = myArc->pointFromSrc(10);
    myLabel->moveTo( tempPoint );

    return *this;
}



ProcessorCall& 
ProcessorCall::moveDst( const Point& aPoint )
{
    GenericCall::moveDst( aPoint );		// Move to center of circle 
    Point intersect = myArc->dstIntersectsCircle( aPoint, fabs( dstEntity()->height() ) / 2 );
    GenericCall::moveDst( intersect );		// Now move to edge.

    return *this;
}



ProcessorCall&
ProcessorCall::label()
{
    if ( !Flags::have_results ) return *this;
    const Task& src = *srcTask();
    const std::vector<Entry *>& entries = src.entries();
    const std::vector<Activity *>& activities = src.activities();
    bool do_newline = false;
    if ( Flags::print[ENTRY_UTILIZATION].value.b && Flags::print[PROCESSOR_UTILIZATION].value.b ) {
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    if ( !(*entry)->hasQueueingTime() || (*entry)->isActivityEntry() ) continue;
	    if ( do_newline ) myLabel->newLine();
	    *myLabel << "U[" << (*entry)->name() << "]=" << opt_pct((*entry)->processorUtilization());
	    do_newline = true;
	}
	for ( std::vector<Activity *>::const_iterator activity = activities.begin(); activity != activities.end(); ++activity ) {
	    if ( !(*activity)->hasQueueingTime() ) continue;
	    if ( do_newline ) myLabel->newLine();
	    *myLabel << "U[" << (*activity)->name() << "]=" << opt_pct( (*activity)->processorUtilization() );
	    do_newline = true;
	}
    }
    if ( src.hasQueueingTime() && Flags::print[PROCESSOR_QUEUEING].value.b ) {
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    if ( !(*entry)->hasQueueingTime() || (*entry)->isActivityEntry() ) continue;
	    if ( do_newline ) myLabel->newLine();
	    *myLabel << "W[" << (*entry)->name() << "]=" << print_queueing_time(**entry);
	    do_newline = true;
	}
	for ( std::vector<Activity *>::const_iterator activity = activities.begin(); activity != activities.end(); ++activity ) {
	    if ( !(*activity)->hasQueueingTime() ) continue;
	    if ( do_newline ) myLabel->newLine();
	    *myLabel << "W[" << (*activity)->name() << "]=" << opt_pct( (*activity)->queueingTime() );
	    do_newline = true;
	}
    }
    return *this;
}

/* ----------------- Calls to processors from tasks. ------------------ */

PseudoProcessorCall::PseudoProcessorCall( const Task * fromTask, const Processor * toProcessor )
    : ProcessorCall( fromTask, toProcessor )
{
    if ( myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED_DOTTED);
    }
}

/* ---------------- Calls from Open arrivals to tasks ----------------- */

OpenArrival::OpenArrival( const OpenArrivalSource * from, const Entry * to )
    : GenericCall(), source(from), _destination(to)
{
    myArc->arrowhead(Graphic::OPEN_ARROW);
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
    return source; 
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
    abort();
}



const LQIO::DOM::ExternalVariable&
OpenArrival::sendNoReply() const
{
    abort();
}

const LQIO::DOM::ExternalVariable&
OpenArrival::forward() const
{
    abort();
}


unsigned
OpenArrival::fanIn() const
{
    abort();
    return 1;
}

unsigned
OpenArrival::fanOut() const
{
    abort();
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
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_SERVER_TYPE:
	return Graphic::RED;
    }

    return dstTask()->colour();
}


OpenArrival& 
OpenArrival::moveDst( const Point& aPoint )
{
    GenericCall::moveDst( aPoint );	

    GenericCall::moveSrc( source->center() );	// Move to center of circle 
    Point intersect = myArc->srcIntersectsCircle( source->center(), source->radius() );
    GenericCall::moveSrc( intersect );

    Point tempPoint = myArc->pointFromSrc(30);
    myLabel->moveTo( tempPoint );

    return *this;
}



OpenArrival&
OpenArrival::label()
{
    bool print = false;
    if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	*myLabel << begin_math( &Label::lambda ) << "=" << _destination->openArrivalRate() << end_math();
	print = true;
    }
    if ( _destination->openWait()
	 && Flags::have_results 
	 && Flags::print[OPEN_WAIT].value.b ) {
	if ( print ) myLabel->newLine();
	Graphic::colour_type c = std::isfinite( _destination->openWait() ) ? Graphic::DEFAULT_COLOUR : Graphic::RED;
	myLabel->colour(c) << begin_math() << _destination->openWait() << end_math();
	print = true;
    }
    return *this;
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
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
    case FORMAT_PSTEX:
	if ( p != 1 ) {
	    output << ',';
	}
	if ( aCall.phaseTypeFlag(p) == PHASE_DETERMINISTIC ) {
	    output << "\\fbox{";
	}
	break;
    case FORMAT_OUTPUT:
    case FORMAT_PARSEABLE:
    case FORMAT_RTF:
	output << std::setw( maxDblLen );
	break;
    case FORMAT_POSTSCRIPT:
    case FORMAT_FIG:
    case FORMAT_SVG:
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
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
    case FORMAT_PSTEX:
	if ( aCall.phaseTypeFlag(p) == PHASE_DETERMINISTIC ) {
	    output << "}";
	}
	break;
    case FORMAT_POSTSCRIPT:
    case FORMAT_SVG:
    case FORMAT_FIG:
	if ( aCall.phaseTypeFlag(p) == PHASE_DETERMINISTIC ) {
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
