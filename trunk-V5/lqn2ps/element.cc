/* element.cc	-- Greg Franks Wed Feb 12 2003
 *
 * $Id$
 */

#include "element.h"
#include <cmath>
#include <cstdlib>
#include <limits.h>
#include <lqio/error.h>
#include "errmsg.h"
#include "stack.h"
#include "entry.h"
#include "task.h"
#include "label.h"
#include "processor.h"
#include "activity.h"

const LQIO::DOM::ConstantExternalVariable Element::ZERO(0);

Element::Element( const LQIO::DOM::DocumentObject * dom, const size_t id )
    : _documentObject( dom ),
      myElementId( id ),
      myLabel(0),
      myNode(0)
{
}


Element::~Element()
{
    myLabel = 0;
    myNode = 0;
}



void
Element::rename()
{
    char new_name[16];
    if ( dynamic_cast<Activity *>(this) ) {
	snprintf( new_name, 16, "a%ld", static_cast<unsigned long>(elementId()) );
    } else if ( dynamic_cast<Entry *>(this) ) {
	snprintf( new_name, 16, "e%ld", static_cast<unsigned long>(elementId()) );
    } else if ( dynamic_cast<Task *>(this) ) {
	snprintf( new_name, 16, "t%ld", static_cast<unsigned long>(elementId()) );
    } else if ( dynamic_cast<Processor *>(this) ) {
	snprintf( new_name, 16, "p%ld", static_cast<unsigned long>(elementId()) );
    } else {
	abort();
    }
    
    const_cast<LQIO::DOM::DocumentObject *>(_documentObject)->setName( new_name );
}



/*
 * Squish the name (hopefully keeping it unique).
 */

void
Element::squishName()
{
    const unsigned n = elementId();
    const string& old_name = name();
    if ( old_name.size() > 5 ) {
	ostringstream new_name;
	new_name << old_name.substr(0,5);
	new_name << "_" << n;
	const_cast<LQIO::DOM::DocumentObject *>(_documentObject)->setName( new_name.str() );
    }
}



Element&
Element::addPath( const unsigned aPath ) 
{
    if ( aPath && !myPaths.find( aPath ) ) {
	myPaths.grow(1);
	myPaths[myPaths.size()] = aPath;
    }
    return *this;
}



/*
 * Return true if no path checking or path exists.
 */

bool
Element::pathTest() const
{
    return (Flags::print[CHAIN].value.i == 0 && isReachable()) || hasPath( Flags::print[CHAIN].value.i ) != 0;
}



/*
 * Chase calls looking for cycles and the depth in the call tree.  
 * The return value reflects the deepest depth in the call tree.
 */

unsigned
Element::followCalls( const Task * parent, Sequence<Call *>& nextCall, CallStack& callStack, const unsigned directPath ) const
{
    unsigned max_depth = callStack.size();
    if ( directPath ) {
	const_cast<Element *>(this)->addPath( directPath );
    }

    Call * aCall;

    while ( aCall = nextCall() ) {
	const Task * dstTask = aCall->dstTask();

	/* Ignore async calls to tasks that accept rendezvous requests. */

	if ( aCall->hasSendNoReply() && !Flags::async_topological_sort && dstTask->isCalled( RENDEZVOUS_REQUEST ) ) continue;

	try {

	    /* 
	     * Chase the call if there is no loop, and if following the path results in pushing
	     * tasks down.  Open class requests can loop up.  Always check the stack because of the
	     * short-circuit test with directPath.
	     */

	    if ( (callStack.find( aCall, directPath != 0 ) == 0 
		  && ( callStack.size() >= dstTask->level() || Flags::exhaustive_toplogical_sort ))
		 || directPath != 0 ) {						/* always (for forwarding)	*/

		callStack.push( aCall );
		if ( directPath && aCall->hasForwarding() && partial_output() && Flags::surrogates ) {
		    addForwardingRendezvous( callStack );			/* only necessary when transforming the model. */
		}
		max_depth = max( const_cast<Task *>(dstTask)->findChildren( callStack, directPath ), max_depth );

		callStack.pop();
	    } 
	}
	catch ( call_cycle& error ) {
	    if ( directPath ) {
		LQIO::solution_error( LQIO::ERR_CYCLE_IN_CALL_GRAPH, error.what() );
	    }
	}
	catch ( activity_cycle& error ) {
	    if ( directPath ) {
		LQIO::solution_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, name().c_str(), error.what() ); 
	    }
	}
    }
    return max_depth;
}


/* 
 * We have to add a psuedo forwarding arc.  Search back for on the
 * call stack for a rendezvous.  On drawing, we may have to do some
 * fancy labelling.  We may have to have more than one proxy.
 */

Element const& 
Element::addForwardingRendezvous( CallStack& callStack ) const
{
    double rate = 1.0;

    for ( unsigned i = callStack.size2(); i > 0 && callStack[i]; --i ) {
	const Call * aCall = callStack[i];
	if ( aCall->hasRendezvous() ) {
	    rate *= aCall->sumOfRendezvous(); 
	    Call * psuedo = aCall->addForwardingCall( const_cast<Entry *>(callStack.top()->dstEntry()), rate );
	    if ( psuedo ) {
		psuedo->proxy( const_cast<Call *>(callStack.top()) );
	    }
	    break;
	} else if ( aCall->hasSendNoReply() ) {
	    break;
	} else if ( aCall->hasForwarding() ) {
	    rate *= LQIO::DOM::to_double( aCall->forward() );
	} else {
	    abort();
	}
    }
    return *this;
}


/*
 * Set the client chain to k.
 */

Element&
Element::setClientClosedChain( unsigned k )
{
    if ( !myClientClosedChains.find( k ) ) {
	myClientClosedChains.grow(1);
	myClientClosedChains[myClientClosedChains.size()] = k;
    }

    return *this;
}


/*
 * Set the client chain to k.
 */

Element&
Element::setClientOpenChain( unsigned k )
{
    if ( !myClientOpenChains.find( k ) ) {
	myClientOpenChains.grow(1);
	myClientOpenChains[myClientOpenChains.size()] = k;
    }

    return *this;
}


/*
 * Set the server chain k.
 */

Element&
Element::setServerChain( unsigned k )
{
    if ( !myServerChains.find( k ) ) {
	myServerChains.grow(1);
	myServerChains[myServerChains.size()] = k;
    }

    return *this;
}


/*
 * Return true is this server has chain k.
 */

bool 
Element::hasServerChain( unsigned k ) const
{
    return myServerChains.find( k ) != 0;
}



/*
 * Return true is this client has chain k.
 */

bool 
Element::hasClientChain( unsigned k ) const
{
    return myClientClosedChains.find( k ) != 0 || myClientOpenChains.find( k ) != 0;
}


/*
 * Return true is this client has chain k.
 */

bool 
Element::hasClientClosedChain( unsigned k ) const
{
    return myClientClosedChains.find( k ) != 0;
}


/*
 * Return true is this client has chain k.
 */

bool 
Element::hasClientOpenChain( unsigned k ) const
{
    return myClientOpenChains.find( k ) != 0;
}


unsigned 
Element::closedChainAt( unsigned i ) const
{
    if ( 1 <= i && i <= myClientClosedChains.size() ) {
	return myClientClosedChains[i];
    } else {
	return 0;
    }
}

unsigned 
Element::openChainAt( unsigned i ) const
{
    if ( 1 <= i && i <= myClientOpenChains.size() ) {
	return myClientOpenChains[i];
    } else {
	return 0;
    }
}

unsigned
Element::serverChainAt( unsigned i ) const
{
    if ( 1 <= i && i <= myServerChains.size() ) {
	return myServerChains[i];
    } else {
	return 0;
    }
}


/*
 * Move the object.  We simply change the origin, then invoke moveTo to do the work.
 */

Element&
Element::moveBy( const double dx, const double dy )
{
    moveTo( myNode->origin.x() + dx, myNode->origin.y() + dy );
    return *this;
}



Element&
Element::scaleBy( const double sx, const double sy )
{
    myNode->scaleBy( sx, sy );
    myLabel->scaleBy( sx, sy );
    return *this;
}



Element&
Element::translateY( const double dy  )
{
    myNode->translateY( dy );
    myLabel->translateY( dy );
    return *this;
}



Element&
Element::depth( const unsigned depth  ) 
{
    myNode->depth( depth  );
    myLabel->depth( depth-1 );
    return *this;
}


/*
 * Generic compare functions
 */

int
Element::compare( const Element * e1, const Element * e2 )
{
    if ( Flags::sort == NO_SORT ) {
	return e1->getDOM()->getSequenceNumber() > e2->getDOM()->getSequenceNumber();
    } else if ( e1->getIndex() != e2->getIndex() ) {
	return static_cast<int>(copysign( 1.0, e1->getIndex() - e2->getIndex() ) );
    } else if ( e1->span() != e2->span() ) {
	if ( e1->index() > e1->getIndex() ) {
	    return e1->span() - e2->span();
	} else {
	    return e2->span() - e1->span();
	}
    }

    switch ( Flags::sort ) {
    case REVERSE_SORT: return e2->name() < e1->name();
    case FORWARD_SORT: return e1->name() > e2->name();
    default: return 0;
    }
}


/* static */ double 
Element::adjustForSlope( double y )
{
    return y * Flags::icon_slope;
}

#if defined(QNAP_OUTPUT) || defined(PMIF_OUTPUT)
static ostream&
server_chain_of_str( ostream& output, const Element& anElement, const bool multi_class, const unsigned i )
{
    if ( multi_class ) {
	output << "(k" << anElement.serverChainAt(i) << ")";
    } 
    return output;
}

static ostream&
closed_chain_of_str( ostream& output, const Element& anElement, const bool multi_class, const unsigned i )
{
    if ( multi_class ) {
	output << "(k" << anElement.closedChainAt(i) << ")";
    } 
    return output;
}

static ostream&
open_chain_of_str( ostream& output, const Element& anElement, const bool multi_class, const unsigned i )
{
    if ( multi_class ) {
	output << "(k" << anElement.openChainAt(i) << ")";
    } 
    return output;
}

QNAPElementManip 
server_chain( const Element& anElement, const bool multi_class, const unsigned i )
{
    return QNAPElementManip( &server_chain_of_str, anElement, multi_class, i );
}

QNAPElementManip 
closed_chain( const Element& anElement, const bool multi_class, const unsigned i )
{
    return QNAPElementManip( &closed_chain_of_str, anElement, multi_class, i );
}

QNAPElementManip 
open_chain( const Element& anElement, const bool multi_class, const unsigned i )
{
    return QNAPElementManip( &open_chain_of_str, anElement, multi_class, i );
}

#endif
