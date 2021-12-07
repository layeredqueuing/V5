/* element.cc	-- Greg Franks Wed Feb 12 2003
 *
 * $Id: element.cc 15170 2021-12-07 23:33:05Z greg $
 */

#include "element.h"
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <map>
#include <lqio/error.h>
#include "errmsg.h"
#include "entry.h"
#include "task.h"
#include "label.h"
#include "processor.h"
#include "activity.h"

const LQIO::DOM::ConstantExternalVariable Element::ZERO(0.);
const LQIO::DOM::ConstantExternalVariable Element::ONE(1.);


Element::Element( const LQIO::DOM::DocumentObject * dom, const size_t id )
    : _documentObject( dom ),
      _elementId( id ),
      myLabel(nullptr),
      myNode(nullptr)
{
}


Element::~Element()
{
}



/*
 * Squish the name (hopefully keeping it unique in the first six
 * characters).  There are three global name spaces, processors,
 * tasks, and entries (groups too maybe?) and on local name space
 * (activities).
 */

Element&
Element::squish( std::map<std::string,unsigned>& key_table, std::map<std::string,std::string>& symbol_table )
{
    std::string key = name().substr(0,5);
    std::pair<std::map<std::string,unsigned>::const_iterator,bool> hit = key_table.insert( std::pair<std::string,unsigned>( key, 0 ) );
    if ( hit.second == true ) return *this;	/* Unique in first 6 chars.  OK for qnap */
    /* Take first two caps, or convert _z to _Z and treat as caps.  Strip numbers. */

    std::string new_name = name().substr( 0, 3 );
    char first =  '_';
    char second = '_';
    for ( std::string::const_iterator c = name().begin() + 3; c != name().end(); ++c ) {
	if ( std::isupper(*c) ) {
	    if ( std::islower(first) ) first = *c;
	    else if ( std::islower(second) ) second = *c;
	} else if ( *c == '_' && std::next(c) != name().end() && isalpha(*std::next(c)) ) {
	    ++c;
	    if ( std::islower(first) ) first = toupper(*c);
	    else if ( std::islower(second) ) second = toupper(*c);
	} else if ( std::isalpha(*c) ) {
	    if ( first == '_' ) first = *c;
	    else if ( second == '_' ) second = *c;
	}
    }
    new_name += first + second;
    std::pair< std::map<std::string,unsigned>::iterator,bool> item = key_table.insert( std::pair<std::string,unsigned>( new_name, 1 ) );
    item.first->second += 1;			/* will get item in table if existed before */
    new_name += std::to_string(item.first->second);
    std::pair<std::map<std::string,std::string>::const_iterator,bool> result = symbol_table.insert( std::pair<std::string,std::string>( new_name, name() ) );
    if ( result.second == false ) {
	throw std::logic_error( "Duplicate symbol" );
    }
    const_cast<LQIO::DOM::DocumentObject *>(_documentObject)->setName( new_name );
    return *this;
}



Element&
Element::addPath( const unsigned aPath ) 
{
    myPaths.insert( aPath );
    return *this;
}



/*
 * Return true if no path checking or path exists.
 */

bool
Element::pathTest() const
{
    return (Flags::print[CHAIN].opts.value.i == 0 && isReachable()) || hasPath( Flags::print[CHAIN].opts.value.i ) != 0;
}



/*
 * Chase calls looking for cycles and the depth in the call tree.  
 * The return value reflects the deepest depth in the call tree.
 */

size_t
Element::followCalls( std::pair<std::vector<Call *>::const_iterator,std::vector<Call *>::const_iterator> callList, CallStack& callStack, const unsigned path ) const
{
    size_t max_depth = callStack.size();
    if ( path != 0 ) {
	const_cast<Element *>(this)->addPath( path );
    }

    for ( std::vector<Call *>::const_iterator call = callList.first; call != callList.second; ++call ) {
	const Task * dstTask = (*call)->dstTask();

	/* Ignore async calls to tasks that accept rendezvous requests. */

	if ( (*call)->hasSendNoReply() && !Flags::async_topological_sort && dstTask->isCalled( request_type::RENDEZVOUS ) ) continue;

	try {

	    /* 
	     * Chase the call if there is no loop, and if following the path results in pushing
	     * tasks down.  Open class requests can loop up.  Always check the stack because of the
	     * short-circuit test with path.
	     */

	    if ( (callStack.find( (*call), path != 0 ) == callStack.end() 
		  && ( callStack.size() >= dstTask->level() || Flags::exhaustive_toplogical_sort ))
		 || path != 0 ) {						/* always (for forwarding)	*/

		callStack.push_back( (*call) );
		if ( path != 0 && (*call)->hasForwarding() && partial_output() && Flags::surrogates ) {
		    addForwardingRendezvous( callStack );			/* only necessary when transforming the model. */
		}
		max_depth = std::max( const_cast<Task *>(dstTask)->findChildren( callStack, path ), max_depth );

		callStack.pop_back();
	    } 
	}
	catch ( const Call::cycle_error& error ) {
	    if ( path != 0 ) {
		LQIO::solution_error( LQIO::ERR_CYCLE_IN_CALL_GRAPH, error.what() );
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

    for ( CallStack::const_reverse_iterator call = callStack.rbegin(); call != callStack.rend(); ++call ) {
	if ( (*call)->hasRendezvous() ) {
	    rate *= to_double( *(*call)->sumOfRendezvous() ); 
	    Call * psuedo = const_cast<Call *>((*call))->addForwardingCall( const_cast<Entry *>(callStack.back()->dstEntry()), rate );
	    if ( psuedo ) {
		psuedo->proxy( const_cast<Call *>(callStack.back()) );
	    }
	    break;
	} else if ( (*call)->hasSendNoReply() ) {
	    break;
	} else if ( (*call)->hasForwarding() ) {
	    rate *= LQIO::DOM::to_double( (*call)->forward() );
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
    myClientClosedChains.insert( k );
    return *this;
}


/*
 * Set the client chain to k.
 */

Element&
Element::setClientOpenChain( unsigned k )
{
    myClientOpenChains.insert( k );
    return *this;
}


/*
 * Set the server chain k.
 */

Element&
Element::setServerChain( unsigned k )
{
    myServerChains.insert(k);
    return *this;
}


/*
 * Return true is this server has chain k.
 */

bool 
Element::hasServerChain( unsigned k ) const
{
    return myServerChains.find( k ) != myServerChains.end();
}



/*
 * Return true is this client has chain k.
 */

bool 
Element::hasClientChain( unsigned k ) const
{
    return myClientClosedChains.find( k ) != myClientClosedChains.end() || myClientOpenChains.find( k ) != myClientOpenChains.end();
}


/*
 * Return true is this client has chain k.
 */

bool 
Element::hasClientClosedChain( unsigned k ) const
{
    return myClientClosedChains.find( k ) != myClientClosedChains.end();
}


/*
 * Return true is this client has chain k.
 */

bool 
Element::hasClientOpenChain( unsigned k ) const
{
    return myClientOpenChains.find( k ) != myClientOpenChains.end();
}


/*
 * Move the object.  We simply change the origin, then invoke moveTo to do the work.
 */

Element&
Element::moveBy( const double dx, const double dy )
{
    moveTo( myNode->left() + dx, myNode->bottom() + dy );
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
    myNode->depth( depth );
    myLabel->depth( depth-1 );
    return *this;
}


/*
 * Generic compare functions
 */

bool
Element::compare( const Element * e1, const Element * e2 )
{
    if ( Flags::sort == Sorting::NONE ) {
	return e1->getDOM()->getSequenceNumber() < e2->getDOM()->getSequenceNumber();
    } else if ( e1->getIndex() != e2->getIndex() ) {
	return e1->getIndex() < e2->getIndex();
    } else if ( e1->span() != e2->span() ) {
	if ( e1->index() >= e1->getIndex() ) {
	    return e1->span() < e2->span();
	} else {
	    return e2->span() < e1->span();
	}
    }

    switch ( Flags::sort ) {
    case Sorting::REVERSE: return e2->name() > e1->name();
    case Sorting::FORWARD: return e1->name() < e2->name();
    default: return false;
    }
}


/* static */ double 
Element::adjustForSlope( double y )
{
    return y * Flags::icon_slope;
}

Graphic::colour_type Element::colourForUtilization( double utilization ) const
{
    if ( Flags::use_colour ) {
	if ( utilization == 0.0 ) {
	    return Graphic::DEFAULT_COLOUR;
	} else if ( utilization < 0.1 ) {
	    return Graphic::VIOLET;
	} else if ( utilization < 0.2 ) {
	    return Graphic::BLUE;
	} else if ( utilization < 0.3 ) {
	    return Graphic::INDIGO;
	} else if ( utilization < 0.4 ) {
	    return Graphic::CYAN;
	} else if ( utilization < 0.5 ) {
	    return Graphic::TURQUOISE;
	} else if ( utilization < 0.6 ) {
	    return Graphic::GREEN;
	} else if ( utilization < 0.7 ) {
	    return Graphic::SPRINGGREEN;
	} else if ( utilization < 0.8 ) {
	    return Graphic::YELLOW;
	} else if ( utilization < 0.9 ) {
	    return Graphic::ORANGE;
	} else { 
	    return Graphic::RED;
	}
    } else if ( utilization >= 0.8 ) {
	return Graphic::GREY_10;
    } else {
	return Graphic::DEFAULT_COLOUR;
    }
}

/*
 * Set colour based on difference (as a percentage).
 */

Graphic::colour_type Element::colourForDifference( double difference ) const
{
    if ( difference == 0.0 ) {
	return Graphic::DEFAULT_COLOUR;
    } else if ( difference < 0.7813 ) {
	return Graphic::BLUE;
    } else if ( difference < 1.5625 ) {
	return Graphic::INDIGO;
    } else if ( difference < 3.125 ) {
	return Graphic::CYAN;
    } else if ( difference < 6.25 ) {
	return Graphic::TURQUOISE;
    } else if ( difference < 12.5 ) {
	return Graphic::GREEN;
    } else if ( difference < 25.0 ) {
	return Graphic::SPRINGGREEN;
    } else if ( difference < 50.0 ) {
	return Graphic::YELLOW;
    } else if ( difference < 100. ) {
	return Graphic::ORANGE;
    } else { 
	return Graphic::RED;
    }
}

#if defined(REP2FLAT)
std::string
Element::baseReplicaName( unsigned int& replica ) const
{
    const std::string& name = this->name();
    const size_t pos = name.rfind( '_' );
    if ( pos == std::string::npos ) {
	replica = 1;
	return name;
    } else { 
	char * end_ptr = NULL;
	replica = strtol( &name[pos+1], &end_ptr, 10 );
	if ( *end_ptr != '\0' || replica <= 0 ) throw std::runtime_error( "Can't find replica number" );
	return name.substr( 0, pos );
    }
}
#endif



#if defined(REP2FLAT)
/*
 * Clone the observation saved by the old_DOM to the new DOM (this).
 * It is a two step process because we can't whack the entries in the
 * multimap without screwing up the iterators.
 */

/* static */ void 
Element::cloneObservations( const LQIO::DOM::DocumentObject * old_DOM, const LQIO::DOM::DocumentObject * new_DOM )
{
    LQIO::Spex::obs_var_tab_t& observations = const_cast<LQIO::Spex::obs_var_tab_t&>(LQIO::Spex::observations());
    const std::pair<LQIO::Spex::obs_var_tab_t::const_iterator, LQIO::Spex::obs_var_tab_t::const_iterator> range = LQIO::Spex::observations().equal_range( old_DOM );
    if ( range.first == range.second ) return;

    LQIO::Spex::obs_var_tab_t new_observations;
    for ( LQIO::Spex::obs_var_tab_t::const_iterator obs = range.first; obs != range.second; ++obs ) {
	new_observations.emplace( new_DOM, obs->second );			/* Make a copy.		*/
    }
    observations.erase( range.first, range.second );				/* Out with the old 	*/
    observations.insert( new_observations.begin(), new_observations.end() );	/* and in with the new 	*/
}
#endif
