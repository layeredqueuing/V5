/*
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_object.h"
#include "dom_document.h"
#include <cassert>

namespace LQIO {
    namespace DOM {

	unsigned long DocumentObject::sequenceNumber = 0;

	DocumentObject::DocumentObject(const Document * document, const char * name, const void * xmlDOMElement ) 
	    : _document(document), _sequenceNumber(sequenceNumber), _name(name), _xmlDOMElement(xmlDOMElement) 
	{
	    assert( document );
	    sequenceNumber += 1;
	}

	DocumentObject::~DocumentObject() 
	{
	}

	const std::string& DocumentObject::getName() const
	{
	    return _name;
	}

	void DocumentObject::setName(const std::string& newName)
	{
	    /* Set the new entity name */
	    _name = newName;
	}

	void DocumentObject::subclass() const
	{
	    if ( dynamic_cast<const Activity *>(this) ) {
		throw should_implement( "Activity" );
	    } else if ( dynamic_cast<const ActivityList *>(this) ) {
		throw should_implement( "ActivityList" );
	    } else if ( dynamic_cast<const AndJoinActivityList *>(this) ) {
		throw should_implement( "AndJoinActivityList" );
	    } else if ( dynamic_cast<const Call *>(this) ) {
		throw should_implement( "Call" );
	    } else if ( dynamic_cast<const Entry *>(this) ) {
		throw should_implement( "Entry" );
	    } else if ( dynamic_cast<const Group *>(this) ) {
		throw should_implement( "Group" );
	    } else if ( dynamic_cast<const Phase *>(this) ) {
		throw should_implement( "Phase" );
	    } else if ( dynamic_cast<const Processor *>(this) ) {
		throw should_implement( "Processor" );
	    } else if ( dynamic_cast<const SemaphoreTask *>(this) ) {
		throw should_implement( "SemaphoreTask" );
		} else if ( dynamic_cast<const RWLockTask *>(this) ) {
		throw should_implement( "RWLockTask" );
	    } else if ( dynamic_cast<const Task *>(this) ) {
		throw should_implement( "Task" );
	    } else {
		throw should_implement( "unknown" );
	    }
	}

    }
}
