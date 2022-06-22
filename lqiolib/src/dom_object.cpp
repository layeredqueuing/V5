/*
 *  $Id: dom_object.cpp 15691 2022-06-22 18:04:24Z greg $
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

	size_t DocumentObject::sequenceNumber = 0;

	DocumentObject::DocumentObject() 
	    : _document(nullptr), _sequenceNumber(0xDEAD0000DEAD0000), _line_number(0), _name("null"), _comment()
	{
	}

	DocumentObject::DocumentObject(const Document * document, const std::string& name ) 
	    : _document(document), _sequenceNumber(sequenceNumber), _line_number(LQIO_lineno), _name(name), _comment()
	{
	    assert( document );
	    sequenceNumber += 1;
	}

	DocumentObject::DocumentObject(const DocumentObject& src ) 
	    : _document(src._document), _sequenceNumber(sequenceNumber), _line_number(LQIO_lineno), _name(), _comment()
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
	    throw should_implement( getTypeName() );
	}

	const ExternalVariable * DocumentObject::checkIntegerVariable( const ExternalVariable * var, int floor_value ) const
	{
	    /* Check for a valid variable (if set).  Return the var */
	    double value = floor_value;
	    if ( var != nullptr && var->wasSet() && ( var->getValue(value) != true || std::isinf(value) || value != rint(value) || value < floor_value ) ) {
		throw std::domain_error( "invalid integer" );
	    }
	    return var;
	}

	int DocumentObject::getIntegerValue( const ExternalVariable * var, int floor_value ) const
	{
	    /* Return a valid integer */
	    double value = floor_value;
	    if ( var == nullptr ) return floor_value;
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

	const ExternalVariable * DocumentObject::checkDoubleVariable( const ExternalVariable * var, double floor_value, double ceiling_value ) const
	{
	    /* Check for a valid variable (if set).  Return the var */
	    double value = floor_value;
	    if ( var != nullptr && var->wasSet() && ( var->getValue(value) != true || std::isinf(value) || value < floor_value || (floor_value < ceiling_value && ceiling_value < value) ) ){
		throw std::domain_error( "invalid double" );
	    }
	    return var;
	}

	double DocumentObject::getDoubleValue( const ExternalVariable * var, double floor_value, double ceiling_value ) const
	{
	    /* Return a valid double */
	    double value = floor_value;
	    if ( var == nullptr ) return floor_value;
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
