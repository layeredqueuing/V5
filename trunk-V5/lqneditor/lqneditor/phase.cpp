//
//  phase.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-07.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include "model.h"
#include "entry.h"
#include "phase.h"
#include "call.h"
#include "arc.h"
#include <algorithm>
#include <lqio/dom_phase.h>
#include <lqio/dom_call.h>

Phase::Phase( const unsigned int p, const LQIO::DOM::Phase& phase, const Entry& entry, const Model& model ) 
    : _calls(), _p(p), _model(model), _entry(entry), _phase( phase )
{
    for ( std::vector<LQIO::DOM::Call*>::const_iterator next_call = phase.getCalls().begin(); next_call != phase.getCalls().end(); ++next_call ) {
	_calls.push_back( new Call( **next_call, this, model ) );
    }
}

Phase::~Phase() 
{
}

void Phase::connectCalls( std::vector<ArcForEntry *>& arcs ) 
{
    for ( std::vector<Call*>::const_iterator next_call = _calls.begin(); next_call != _calls.end(); ++next_call ) {
	Call * call = *next_call;
	call->connect();
	ArcForEntry * arc = 0;
	std::vector<ArcForEntry *>::const_iterator the_arc = std::find_if( arcs.begin(), arcs.end(), ArcForEntry::eqDestination(call->getDestination()) );
	if ( the_arc == arcs.end() ) {
	    arc = new ArcForEntry( _model );		    /* create a new arc */
	    arcs.push_back( arc );
	    const Entry * destination = call->getDestination();
	    const_cast<Entry *>(destination)->addAsDestination( arc );	/* connect to destination entry. */

	} else {
	    arc = *the_arc;
	}
	arc->addCall( _p, call );					/* Add the phase to the arc 	*/
    }
}


void Phase::findChildren( std::set<const Task *>& call_chain ) const throw( std::runtime_error )
{
    for ( std::vector<Call*>::const_iterator next_call = _calls.begin(); next_call != _calls.end(); ++next_call ) {
	Call * call = *next_call;
	call->findChildren( call_chain );
    }
}

