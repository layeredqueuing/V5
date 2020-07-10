/*
 *  $Id: dom_object.cpp 13675 2020-07-10 15:29:36Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_object.h"
#include "dom_document.h"
#include <sstream>
#include <cassert>
#include <cmath>

namespace LQIO {
    namespace DOM {

	unsigned long DocumentObject::sequenceNumber = 0;

	DocumentObject::DocumentObject() 
	    : _document(nullptr), _sequenceNumber(0xDEAD0000DEAD0000), _name("null"), _comment()
	{
	}

	DocumentObject::DocumentObject(const Document * document, const std::string& name ) 
	    : _document(document), _sequenceNumber(sequenceNumber), _name(name), _comment()
	{
	    assert( document );
	    sequenceNumber += 1;
	}

	DocumentObject::DocumentObject(const DocumentObject& src ) 
	    : _document(src._document), _sequenceNumber(sequenceNumber), _name(), _comment()
	{
	    assert( _document );
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

	const std::string& DocumentObject::getComment() const
	{
	    return _comment;
	}

	void DocumentObject::setComment( const std::string& comment )
	{
	    _comment = comment;
	}

	bool DocumentObject::hasResults() const
	{
	    return getDocument()->hasResults();
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

	ExternalVariable * DocumentObject::checkIntegerVariable( ExternalVariable * var, int floor_value ) const
	{
	    /* Check for a valid variable (if set).  Return the var */
	    double value = floor_value;
	    if ( var != NULL && var->wasSet() && ( var->getValue(value) != true || std::isinf(value) || value != rint(value) || value < floor_value ) ) {
		throw std::domain_error( "invalid integer" );
	    }
	    return var;
	}

	int DocumentObject::getIntegerValue( const ExternalVariable * var, int floor_value ) const
	{
	    /* Return a valid integer */
	    double value = floor_value;
	    if ( var == NULL ) return floor_value;
	    if ( var->wasSet() != true ) throw std::domain_error( "not set" );
	    if ( var->getValue(value) != true ) throw std::domain_error( "not a number" );	/* Sets value for return! */
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value != rint(value) ) throw std::domain_error( "invalid integer" );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    return value;
	}

	ExternalVariable * DocumentObject::checkDoubleVariable( ExternalVariable * var, double floor_value, double ceiling_value ) const
	{
	    /* Check for a valid variable (if set).  Return the var */
	    double value = floor_value;
	    if ( var != NULL && var->wasSet() && ( var->getValue(value) != true || std::isinf(value) || value < floor_value || (floor_value < ceiling_value && ceiling_value < value) ) ){
		throw std::domain_error( "invalid double" );
	    }
	    return var;
	}

	double DocumentObject::getDoubleValue( const ExternalVariable * var, double floor_value, double ceiling_value ) const
	{
	    /* Return a valid double */
	    double value = floor_value;
	    if ( var == NULL ) return floor_value;
	    if ( var->wasSet() != true ) throw std::domain_error( "not set" );
	    if ( var->getValue(value) != true ) throw std::domain_error( "not a number" );	/* Sets value for return! */
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    if ( floor_value < ceiling_value && ceiling_value < value ) {
		std::stringstream ss;
		ss << value << " > " << ceiling_value;
		throw std::domain_error( ss.str() );
	    }
	    return value;
	}
    }
}
