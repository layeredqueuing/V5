//
//  call.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-07.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include "model.h"
#include "call.h"
#include "entry.h"
#include <lqio/dom_call.h>

Call::Call( const LQIO::DOM::Call& call, const Phase* source, const Model& model ) 
    : _call( call ), _model( model ), _source(source), _destination(0) 
{
}

Call::~Call() 
{
}


void Call::connect() 
{
    _destination = _model.findEntry( _call.getDestinationEntry()->getName() );
}

void Call::findChildren( std::set<const Task *>& call_chain ) const throw( std::runtime_error )
{
    if ( _destination ) _destination->findChildren( call_chain );
}

