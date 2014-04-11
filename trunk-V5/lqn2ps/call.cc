/*  -*- c++ -*-
 * $Id$
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
#include <cassert>
#include <cstdlib>
#include <cmath>
#if HAVE_IEEEFP_H && !defined(MSDOS)
#include <ieeefp.h>
#endif
#include <lqio/input.h>
#include <lqio/dom_call.h>
#include <lqio/dom_entry.h>
#include "call.h"
#include "arc.h"
#include "label.h"
#include "entry.h"
#include "cltn.h"
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

/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Print out the number of rendezvous in standard output format.
 */

ostream&
operator<<( ostream& output, const GenericCall& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_SRVN:
	self.print( output );
	break;
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
	break;
#endif
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
	break;
#endif
    default:
	self.draw( output );
	break;
    }
    return output;
}

/*----------------------------------------------------------------------*/
/*                            Generic  Calls                            */
/*----------------------------------------------------------------------*/

GenericCall::GenericCall() 
    : myLabel(0), myArc(0)
{
    myArc = Arc::newArc();
    myLabel = Label::newLabel();
    if ( myLabel ) {
	myLabel->justification( Flags::label_justification );
    }
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
    (*myArc)[2] =  aPoint; 
    return *this; 
} 



GenericCall&
GenericCall::movePenultimate( const Point& aPoint )
{
    (*myArc)[myArc->size()-1] = aPoint; 
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



ostream&
GenericCall::draw( ostream& output ) const
{
    ostringstream aComment;
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
    return output;
}

/*----------------------------------------------------------------------*/
/*                          Calls between Entries                       */
/*----------------------------------------------------------------------*/

Call::Call()
    : GenericCall(), destination(0),
      myForwarding(0)
{
}

/*
 * Initialize and zero fields.   Reverse links are set here.  Forward
 * links are done by subclass.  Processor calls are linked specially.
 */

Call::Call( const Entry * toEntry, const unsigned nPhases )
    : GenericCall(), destination(toEntry),
      myForwarding(0)
{
    myRendezvous.grow(nPhases);			/* rendezvous.			*/
    mySendNoReply.grow(nPhases);		/* send no reply.		*/
    for ( unsigned p = 1; p <= nPhases; ++p ) {
	myRendezvous[p] = 0;
	mySendNoReply[p] = 0;
    }
}


/*
 * Clean up the mess.
 */

Call::~Call()
{
    destination = 0;			/* to whom I am referring to	*/
}


int
Call::operator==( const Call& item ) const
{
    return (dstEntry() == item.dstEntry());
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
 * Add all phases of src to the receiver.  multiply by rate.
 */

Call&
Call::merge( const Call& src, const double rate )
{
    for ( unsigned p = 1; p <= myRendezvous.size(); ++p ) {
	merge( p, src, rate );
    }
    return *this;
}



/*
 * Add all phases of src to the receiver.  multiply by rate.
 */

Call&
Call::merge( const int p, const Call& src, const double rate )
{
    assert( (isPseudoCall() && src.isPseudoCall() ) || (!isPseudoCall() && !src.isPseudoCall() ) );

    if ( src.myRendezvous[p] ) {
	const LQIO::DOM::ExternalVariable& rnv_src = *(src.myRendezvous[p]->getCallMean());
	if ( myRendezvous[p] ) {
	    LQIO::DOM::ExternalVariable& rnv_dst = *(const_cast<LQIO::DOM::ExternalVariable *>(myRendezvous[p]->getCallMean()));
	    rnv_dst += rnv_src * rate;
	} else {
	    myRendezvous[p] = src.myRendezvous[p]->clone();		/* Copy call. */
	}
    }

    if ( src.mySendNoReply[p] ) {
	const LQIO::DOM::ExternalVariable& snr_src = *(src.mySendNoReply[p]->getCallMean());
	if ( mySendNoReply[p] ) {
	    LQIO::DOM::ExternalVariable& snr_dst = *(const_cast<LQIO::DOM::ExternalVariable *>(mySendNoReply[p]->getCallMean()));
	    snr_dst += snr_src * rate;
	} else {
	    mySendNoReply[p] = src.mySendNoReply[p]->clone();
	}
    }

    setArcType();
    return *this;
}


/*
 * Move all phases to phase 1.
 */

Call&
Call::aggregatePhases()
{
    for ( unsigned p = 2; p <= maxPhase(); ++p ) {
	if ( myRendezvous[p] ) {
	    const LQIO::DOM::ExternalVariable& rnv_src = *(myRendezvous[p]->getCallMean());
	    if ( !myRendezvous[1] ) {
		myRendezvous[1] = myRendezvous[p]->clone();
	    } else {
		*const_cast<LQIO::DOM::ExternalVariable *>(myRendezvous[p]->getCallMean()) += rnv_src;
	    }
	}
	if ( mySendNoReply[p] ) {
	    const LQIO::DOM::ExternalVariable& snr_src = *(mySendNoReply[p]->getCallMean());
	    if ( !mySendNoReply[1] ) {
		mySendNoReply[1] = mySendNoReply[p]->clone();
	    } else {
		*const_cast<LQIO::DOM::ExternalVariable *>(mySendNoReply[p]->getCallMean()) += snr_src;
	    }
	}
    }

    setArcType();
    return *this;
}


/*
 * Set the arc type (ie., arrow and line style)
 */

Call&
Call::setArcType()
{
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
    for ( unsigned p = 1; p <= myRendezvous.size(); ++p ) {
	double result = 0.0;
	if ( !hasRendezvousForPhase(p) ) continue;
	const LQIO::DOM::ExternalVariable * value = myRendezvous[p]->getCallMean();
	if ( !value ) continue;
	else if ( !value->getValue(result) ) abort(); 		/* throw not_defined */
	else sum += result;
    }
    return sum;
}



const LQIO::DOM::ExternalVariable & 
Call::rendezvous( const unsigned p ) const
{
    if ( myRendezvous[p] ) return *myRendezvous[p]->getCallMean();
    else return Element::ZERO;
}


Call& 
Call::rendezvous( const unsigned p, const LQIO::DOM::Call * value )
{
    if ( !mySendNoReply[p] ) {
	myRendezvous[p] = value;
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
    for ( unsigned p = 1; p <= mySendNoReply.size(); ++p ) {
	if ( !hasSendNoReplyForPhase(p) ) continue;
	const LQIO::DOM::ExternalVariable * value = mySendNoReply[p]->getCallMean();
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
    if ( mySendNoReply[p] ) return *mySendNoReply[p]->getCallMean();
    else return Element::ZERO;
}


Call& 
Call::sendNoReply( const unsigned p, const LQIO::DOM::Call * value )
{ 
    if ( !myRendezvous[p] && !myForwarding ) {
	mySendNoReply[p] = value;
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
    if ( myForwarding ) return *myForwarding->getCallMean();
    else return Element::ZERO;
}


Call& 
Call::forward( const LQIO::DOM::Call * value )
{ 
    if ( !mySendNoReply[1] ) {
	myForwarding = value;
	if ( myArc ) {
	    myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	    if ( !Flags::print_forwarding_by_depth && myArc->size() == 2 ) {
		myArc->grow( 2 );
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
    if ( hasRendezvousForPhase(p) ) {
	return myRendezvous[p];
    } else if ( hasSendNoReplyForPhase(p) ) {
	return mySendNoReply[p];
    } else {
	return 0;
    }
}


const LQIO::DOM::Call *
Call::getDOMFwd() const
{
    return myForwarding;
}

bool
Call::hasWaiting() const 
{ 
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	LQIO::DOM::Call * dom = const_cast<LQIO::DOM::Call *>(getDOM(p));
	if ( dom && dom->getResultWaitingTime() > 0. ) return true;
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
	if ( dom && !isfinite( dom->getResultWaitingTime() ) )  return true;
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

const string&
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
    return myRendezvous[p] != NULL;
}


bool 
Call::hasRendezvous() const
{
    for ( unsigned p = 1; p <= myRendezvous.size(); ++p ) {
	if ( myRendezvous[p] ) return true;
    }
    return false;
}



bool 
Call::hasSendNoReplyForPhase( const unsigned p ) const
{
    return mySendNoReply[p] != NULL;
}


bool
Call::hasSendNoReply() const
{
    for ( unsigned p = 1; p <= mySendNoReply.size(); ++p ) {
	if ( mySendNoReply[p] ) return true;
    }
    return false;
}


bool
Call::hasForwarding() const
{
    return myForwarding != 0;
}


Graphic::colour_type 
Call::colour() const
{
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_RESULTS:
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

    if ( Flags::label_justification == ABOVE_JUSTIFY ) {
	/* Move all labels above entry */
	Point tempPoint = myArc->dstPoint();
	tempPoint.moveBy( 0, Flags::print[Y_SPACING].value.f / 3.0 );
	myLabel->moveTo( tempPoint ).justification( CENTER_JUSTIFY );
    } else {
	double offset;
	const Point& p1 = myArc->penultimatePoint();
	const Point& p2 = myArc->dstPoint();
	if ( p1.y() > p2.y() ) {
	    offset = Flags::print[Y_SPACING].value.f / 3.0;
	    if ( hasForwardingLevel() && dstEntry()->callerList().find( this ) % 2 ) {
		offset *= 2;
	    }
	} else {
	    offset = Flags::print[Y_SPACING].value.f / 3.0 + Flags::icon_height;
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



ostream&
Call::print( ostream& output ) const
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



ostream&
Call::printSRVNLine( ostream& output, char code, print_func_ptr func ) const
{
    output << "  " << code << " " 
	   << srcName() << " " 
	   << dstName() << " " 
	   << (*func)( *this ) << " -1" << endl;
    return output;
}


/*
 * Compare destinations of the arcs leaving the source entry (for sorting).
 */

int
Call::compareSrc( const void * n1, const void *n2 )
{
    const Call * call1 = (*static_cast<Call **>(const_cast<void *>(n1)));
    const Call * call2 = (*static_cast<Call **>(const_cast<void *>(n2)));

    if ( call1->dstEntry() == call2->dstEntry() ) {
	if ( call1->hasForwarding() && !call2->hasForwarding() ) {
	    return 1;
	} else if ( !call1->hasForwarding() && call2->hasForwarding() ) {
	    return -1;
	}
    }

    const double diff = call1->dstIndex() - call2->dstIndex();
    if ( fabs( diff ) < 1.0 ) {
	if ( (call1->dstLevel() <= call2->dstLevel() && call1->srcIndex() <= call1->dstIndex())
	    || (call1->dstLevel() >= call2->dstLevel() && call1->srcIndex() >= call1->dstIndex())) {
	    return 1;
	} else {
	    return -1;
	}
    }
    return static_cast<int>(copysign( 1.0, diff ) );
}


/*
 * Compare sources of the calls entering entry (for sorting).
 */

int
Call::compareDst( const void * n1, const void *n2 )
{
    const Call * call1 = (*static_cast<Call **>(const_cast<void *>(n1)));
    const Call * call2 = (*static_cast<Call **>(const_cast<void *>(n2)));

    if ( call1->dstEntry() == call2->dstEntry() ) {
	if ( call1->hasForwarding() && !call2->hasForwarding() ) {
	    return 1;
	} else if ( !call1->hasForwarding() && call2->hasForwarding() ) {
	    return -1;
	}
    }

    const double diff = call1->srcIndex() - call2->srcIndex();
    if ( fabs( diff ) < 1.0 ) {
	if ( (call1->srcLevel() < call2->srcLevel() && call1->srcIndex() > call1->dstIndex())
	    || (call1->srcLevel() > call2->srcLevel() && call1->srcIndex() < call1->dstIndex()) ) {
	    return -1;
	} else {
	    return 1;
	}
    }
    return static_cast<int>(copysign( 1.0, diff ) );
}

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

void
EntryCall::check()
{
    /* Check */

    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	double value;
	char p_str[2];
	p_str[0] = p + '0';
	p_str[1] = '\0';
	if ( hasRendezvousForPhase(p) && rendezvous(p).wasSet() && rendezvous(p).getValue( value ) ) {
	    if ( value < 0.0 ) {
		LQIO::solution_error( LQIO::ERR_NEGATIVE_CALLS_FOR, "Entry", srcName().c_str(), "phase", p_str, value, dstName().c_str() );
	    }
	    if ( srcEntry()->phaseTypeFlag(p) == PHASE_DETERMINISTIC && fmod( value, 1.0 ) > 1e-6 ) {
		LQIO::solution_error( LQIO::ERR_NON_INTEGRAL_CALLS_FOR, "Entry", srcName().c_str(), "phase", p_str, value, dstName().c_str() );
	    }
	}
	if ( hasSendNoReplyForPhase(p) && sendNoReply(p).wasSet() && sendNoReply(p).getValue( value ) ) {
	    if ( value < 0.0 ) {
		LQIO::solution_error( LQIO::ERR_NEGATIVE_CALLS_FOR, "Entry", srcName().c_str(), "phase", p_str, value, dstName().c_str() );
	    }
	    if ( srcEntry()->phaseTypeFlag(p) == PHASE_DETERMINISTIC && fmod( value, 1.0 ) > 1e-6 ) {
		LQIO::solution_error( LQIO::ERR_NON_INTEGRAL_CALLS_FOR, "Entry", srcName().c_str(), "phase", p_str, value, dstName().c_str() );
	    }
	}
    }

    const unsigned n_src = srcTask()->replicas();
    const unsigned n_dst = dstTask()->replicas();
    if ( n_src * fanOut() != n_dst * fanIn() ) {	/* Fully specified and valid */
	LQIO::solution_error( ERR_REPLICATION, 
			      "entry", srcName().c_str(), fanOut(), n_src,
			      "entry", dstName().c_str(), fanIn() , n_dst );
    }
}



/*
 * Return the name of the source entry.
 */

const string& 
EntryCall::srcName() const
{
    return srcEntry()->name();
}



/*
 * Return the source task of this call.  Sources are always tasks.
 */

const Entity *
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
EntryCall::addForwardingCall( Entry * toEntry, const double rate ) const
{
    Call * aCall = 0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	aCall = const_cast<Entry *>(srcEntry())->forwardingRendezvous( toEntry, p, rate * LQIO::DOM::to_double(rendezvous(p)) );
    }
    
    return aCall;
}


const EntryCall&
EntryCall::setChain( const unsigned k ) const
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

void
ActivityCall::check()
{
    /* Check */

    double value = 0.0;
    if ( hasRendezvous() ) {
	rendezvous().wasSet() && rendezvous().getValue(value);
    } else if ( hasSendNoReply() ) {
	sendNoReply().wasSet() && sendNoReply().getValue(value);
    }
    if ( value < 0. ) {
	LQIO::solution_error( LQIO::ERR_NEGATIVE_CALLS_FOR, "Task", srcTask()->name().c_str(), "activity", srcName().c_str(), value, dstName().c_str() );
    } else if ( srcActivity()->phaseTypeFlag() == PHASE_DETERMINISTIC && fmod( value, 1.0 ) > 1e-6 ) {
	LQIO::solution_error( LQIO::ERR_NON_INTEGRAL_CALLS_FOR, "Task", srcTask()->name().c_str(), "activity", srcName().c_str(), value, dstName().c_str() );
    }

    /* Check */

    const unsigned n_src = srcTask()->replicas();
    const unsigned n_dst = dstTask()->replicas();
    if ( n_src * fanOut() != n_dst * fanIn() ) {	/* Fully specified and valid */
	LQIO::solution_error( ERR_REPLICATION, 
			      "activity", srcName().c_str(), fanOut(), n_src,
			      "entry", dstName().c_str(), fanIn() , n_dst );
    }
}



/*
 * Return the name of the source entry.
 */

const string& 
ActivityCall::srcName() const
{
    return srcActivity()->name();
}

/*
 * Return the source task of this call.  Sources are always tasks.
 */

const Entity *
ActivityCall::srcTask() const
{
    return srcActivity()->owner();
}


double
ActivityCall::srcIndex() const
{
    return srcActivity()->index();
}

const ActivityCall&
ActivityCall::setChain( const unsigned k ) const
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
ActivityCall::addForwardingCall( Entry * toEntry, const double rate ) const
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

ostream&
ActivityCall::printSRVNLine( ostream& output, char code, print_func_ptr func ) const
{
    if ( isPseudoCall() ) return output;

    output << "  " << code << " " 
	   << srcName() << " " 
	   << dstName() << " " 
	   << (*func)( *this ) << endl;
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

const Entity * 
EntityCall::srcTask() const 
{ 
    return mySrcTask; 
}



const string& 
EntityCall::srcName() const
{
    return srcTask()->name();
}

/* -------------------- Calls to tasks from tasks. -------------------- */

TaskCall::TaskCall( const Task * fromTask, const Task * toTask )
    : EntityCall( fromTask ), myDstTask(toTask), myRendezvous(0), mySendNoReply(0), myForwarding(0)
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


const string &
TaskCall::dstName() const
{
    return dstTask()->name();
}

double
TaskCall::dstIndex() const
{
    return dstTask()->index();
}

unsigned 
TaskCall::dstLevel() const
{
    return dstTask()->level();
}


TaskCall& 
TaskCall::rendezvous( const LQIO::DOM::ConstantExternalVariable& value ) 
{ 
    myRendezvous = value;
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
    mySendNoReply = value;
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
    return srcTask()->fanIn( dstTask() );
}

unsigned
TaskCall::fanOut() const
{
    return srcTask()->fanOut( dstTask() );
}


TaskCall& 
TaskCall::taskForward( const LQIO::DOM::ConstantExternalVariable& value) 
{ 
    myForwarding = value;
    if ( !hasRendezvous() && myArc ) {
	myArc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	if ( !Flags::print_forwarding_by_depth && myArc->size() == 2 ) {
	    myArc->grow( 2 );
	}
    }
    return *this;
}

double
TaskCall::serviceTime( const unsigned k ) const
{
    return dstTask()->serviceTime( k );
}


bool
TaskCall::isSelected() const
{
    return (dstTask()->isSelected() 
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
		&& dstTask()->isSelectedIndirectly()));
}

bool TaskCall::hasRendezvous() const 
{ 
    double value;
    return myRendezvous.wasSet() && myRendezvous.getValue( value ) && value > 0.0;
}

bool TaskCall::hasSendNoReply() const 
{ 
    double value;
    return mySendNoReply.wasSet() && mySendNoReply.getValue( value ) && value > 0.0;
}

bool TaskCall::hasForwarding() const 
{ 
    double value;
    return myForwarding.wasSet() && myForwarding.getValue( value ) && value > 0.0;
}


bool
TaskCall::isLoopBack() const
{
    return dynamic_cast<const Task *>(srcTask()) == dstTask();
}



const TaskCall&
TaskCall::setChain( const unsigned k ) const
{
    if ( hasSendNoReply() ) {
	const_cast<Entity *>(srcTask())->setClientOpenChain( k );
    } else {
	const_cast<Entity *>(srcTask())->setClientClosedChain( k );
    }
    const_cast<Task *>(dstTask())->setServerChain( k );
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
	return dstTask()->colour();
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
	 Flags::have_results && Flags::print[PROCESS_QUEUEING].value.b ) {
	Sequence<Entry *> nextEntry(dynamic_cast<const Task *>(srcTask())->entries());
	Entry * anEntry;
	bool print = false;
	while ( anEntry = nextEntry() ) {
	    if ( !anEntry->hasQueueingTime() ) continue;
	    if ( print ) myLabel->newLine();
	    *myLabel << anEntry->name() << ": " << print_queueing_time(*anEntry);
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
    : EntityCall( fromTask ), myProcessor(toProcessor)
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


const string& 
ProcessorCall::dstName() const
{
    return dstProcessor()->name();
}

double
ProcessorCall::dstIndex() const
{
    return dstProcessor()->index();
}

unsigned
ProcessorCall::dstLevel() const
{
    return dstProcessor()->level();
}


double
ProcessorCall::serviceTime( const unsigned k ) const
{
    return dstProcessor()->serviceTime( k );
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
    return dstProcessor()->fanIn( srcTask() );
}

unsigned
ProcessorCall::fanOut() const
{
    return 1;
}


bool
ProcessorCall::isSelected() const
{
    return ( dstProcessor()->isSelected() 
#if HAVE_REGEX_T
	     || Flags::print[INCLUDE_ONLY].value.r 
#endif
	     )
	&& ( dstProcessor()->isInteresting()
	     || (Flags::print[CHAIN].value.i != 0 && dstProcessor()->isSelectedIndirectly())
	     || submodel_output() 
	     || queueing_output() );
}


const ProcessorCall&
ProcessorCall::setChain( const unsigned k ) const
{
    if ( hasSendNoReply() ) {
	const_cast<Entity *>(srcTask())->setClientOpenChain( k );
    } else {
	const_cast<Entity *>(srcTask())->setClientClosedChain( k );
    }
    const_cast<Processor *>(dstProcessor())->setServerChain( k );
    return *this;
}


Graphic::colour_type 
ProcessorCall::colour() const
{
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_CLIENTS:	return srcTask()->colour();
    case COLOUR_SERVER_TYPE:	return GenericCall::colour();
    }
    return myProcessor->colour();
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
    Point intersect = myArc->dstIntersectsCircle( aPoint, fabs( myProcessor->height() ) / 2 );
    GenericCall::moveDst( intersect );		// Now move to edge.

    return *this;
}



ProcessorCall&
ProcessorCall::label()
{
    if ( !Flags::have_results ) return *this;
    const Task * aTask = dynamic_cast<const Task *>(srcTask());
    Sequence<Entry *> nextEntry(aTask->entries());
    Entry * anEntry;

    bool print = false;
    if ( Flags::print[PROCESS_UTIL].value.b ) {
	while ( anEntry = nextEntry() ) {
	    if ( !anEntry->hasQueueingTime() || anEntry->isActivityEntry() ) continue;
	    *myLabel << (!print ? "Ue=" : ";") << anEntry->processorUtilization();
	    print = true;
	}
    }
    if ( aTask->hasQueueingTime() && Flags::print[PROCESS_QUEUEING].value.b ) {
	if ( print ) myLabel->newLine();
	print = false;
	while ( anEntry = nextEntry() ) {
	    if ( !anEntry->hasQueueingTime() || anEntry->isActivityEntry() ) continue;
	    *myLabel << (!print ? "We=" : ";") << print_queueing_time(*anEntry);
	    print = true;
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
    : GenericCall(), source(from), destination(to)
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


const Entity * 
OpenArrival::srcTask() const 
{ 
    return source; 
}



const string&
OpenArrival::srcName() const
{
    return srcTask()->name();
}


const string&
OpenArrival::dstName() const
{
    return destination->name();
}


double
OpenArrival::srcIndex() const
{
    return srcTask()->index();
}


double
OpenArrival::dstIndex() const
{
    return destination->index();
}


const Task * 
OpenArrival::dstTask() const
{
    return destination->owner();
}

unsigned 
OpenArrival::dstLevel() const
{
    return dstTask()->level();
}


const LQIO::DOM::ExternalVariable&
OpenArrival::openArrivalRate() const
{
    return destination->openArrivalRate();
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
    return destination->openWait();
}

const OpenArrival&
OpenArrival::setChain( const unsigned k ) const
{
    const_cast<Entity *>(srcTask())->setClientOpenChain( k );
    const_cast<Entry *>(destination)->setServerChain( k );
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
	*myLabel << begin_math( &Label::lambda ) << "=" << destination->openArrivalRate() << end_math();
	print = true;
    }
    if ( destination->openWait()
	 && Flags::have_results 
	 && Flags::print[OPEN_WAIT].value.b ) {
	if ( print ) myLabel->newLine();
	Graphic::colour_type c = isfinite( destination->openWait() ) ? Graphic::DEFAULT_COLOUR : Graphic::RED;
	myLabel->colour(c) << begin_math() << destination->openWait() << end_math();
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

unsigned 
CallStack::find( const Call * dstCall, const bool direct_path ) const
{
    const Entry * dstEntry = dstCall->dstEntry();
    const unsigned sz = Stack<const Call *>::size();
    bool broken = false; 
    for ( unsigned j = sz; j > 0; --j ) {
	const Call * aCall = (*this)[j];
	if ( !aCall ) continue;
	if ( aCall->hasSendNoReply() ) broken = true;		/* Cycle broken - async call */

	if ( aCall->dstEntry() == dstEntry ) {			/* Cycle detected. */
	    if ( aCall->hasRendezvous() && dstCall->hasRendezvous() && !broken ) {
		throw call_cycle( dstCall, *this );		/* Dead lock */
	    } if ( aCall->dstEntry() == dstEntry && direct_path ) {
		throw call_cycle( dstCall, *this );		/* Live lock */
	    } else {
		return j;
	    }
	}
    }
    return 0;
}



/*
 * We may skip back over forwarded calls when computing the size.
 */

unsigned 
CallStack::size() const
{
    const unsigned sz = Stack<const Call *>::size();
    if ( Flags::print_forwarding_by_depth ) {
	return sz;
    } else {
	unsigned k = 0;
	for ( unsigned j = 1; j <= sz; ++j ) {
	    const Call * aCall = (*this)[j];
	    if ( !aCall || aCall->hasRendezvous() || aCall->hasSendNoReply() ) {
		k += 1;
	    }
	}
	return k;
    }
}


/* 
 * Return size of stack, regarless of model printing type.
 */

unsigned 
CallStack::size2() const
{
    return Stack<const Call *>::size();
}

/* ------------------------ Exception Handling ------------------------ */

call_cycle::call_cycle( const Call * aCall, const CallStack& callStack )
    : path_error( callStack.size() )
{
    myMsg = aCall->dstName();
    for ( unsigned i = callStack.size(); i > 0; --i ) {
	if ( !callStack[i] ) continue;
	myMsg += ", ";
	myMsg += callStack[i]->dstName();
    }
}

/*----------------------------------------------------------------------*/
/*                      Functions for manipulators                      */
/*----------------------------------------------------------------------*/

static ostream&
format_prologue( ostream& output, const Call& aCall, int p )
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
	output << setw( maxDblLen );
	break;
    case FORMAT_POSTSCRIPT:
    case FORMAT_FIG:
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

static ostream&
format_epilogue( ostream& output, const Call& aCall, int p )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
    case FORMAT_PSTEX:
	if ( aCall.phaseTypeFlag(p) == PHASE_DETERMINISTIC ) {
	    output << "}";
	}
	break;
    case FORMAT_POSTSCRIPT:
    case FORMAT_FIG:
	if ( aCall.phaseTypeFlag(p) == PHASE_DETERMINISTIC ) {
	    output << ":D";
	}
	break;
    }
    return output;
}

static ostream&
rendezvous_of_str( ostream& output, const Call& aCall )
{
    for ( unsigned p = 1; p <= aCall.maxPhase(); ++p ) {
	format_prologue( output, aCall, p );
	output << instantiate( aCall.rendezvous(p) );
	format_epilogue( output, aCall, p );
    }
    return output;
}


static ostream&
sendnoreply_of_str( ostream& output, const Call& aCall )
{
    for ( unsigned p = 1; p <= aCall.maxPhase(); ++p ) {
	format_prologue( output, aCall, p );
	output << instantiate( aCall.sendNoReply(p) );
	format_epilogue( output, aCall, p );
    }
    return output;
}


static ostream&
forwarding_of_str( ostream& output, const Call& aCall )
{
    output << instantiate( aCall.forward() );
    return output;
}


static ostream&
fanin_of_str( ostream& output, const Call& aCall )
{
    output << aCall.fanIn();
    return output;
}


static ostream&
fanout_of_str( ostream& output, const Call& aCall )
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
	aLabel << aCall.waiting(p);
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
	aLabel << aCall.dropProbability(p);
    }
    return aLabel;
}



static ostream&
calls_of_str( ostream& output, const Call& aCall )
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


/*
 * No need for fancy forwarding like print_calls because we never use
 * this to generate srvn input/output.
 */

#if defined(QNAP_OUTPUT)
static ostream&
qnap_visits_str( ostream& output, const EntityCall& aCall )
{
    output << "v_" << aCall.srcName() << "_" << aCall.dstName();
    return output;
}
#endif


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


#if defined(QNAP_OUTPUT)
EntityCallManip
qnap_visits( const EntityCall& aCall )
{
    return EntityCallManip( &qnap_visits_str, aCall );
}
#endif
