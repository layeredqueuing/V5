/*
 *  $Id: dom_extvar.cpp 16817 2023-11-01 19:40:11Z greg $
 *
 *  Created by Martin Mroz on 02/03/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <lqx/Program.h>
#include "dom_extvar.h"

// #define BUG_315 0

namespace LQIO {
    namespace DOM {
	class ExternalVariable;
	
	double to_double( const LQIO::DOM::ExternalVariable& arg )
	{
	    double value;
	    if ( !arg.getValue( value ) ) throw std::domain_error( "unassigned variable" );
	    return value;
	}


	double to_unsigned( const LQIO::DOM::ExternalVariable& arg )
	{
	    double value;
	    if ( !arg.getValue( value ) ) throw std::domain_error( "unassigned variable" );
	    return value;
	}


	const char * to_string( const LQIO::DOM::ExternalVariable& arg )
	{
	    const char * value;
	    if ( !arg.getString( value ) ) throw std::domain_error( "unassigned variable" );
	    return value;
	}

	std::ostream&
	operator<<( std::ostream& output, const LQIO::DOM::ExternalVariable& self )
	{
	    return self.print( output );
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	ExternalVariable::ExternalVariable()
	{
	}

	ExternalVariable::ExternalVariable( const ExternalVariable& )
	{
	    /* Nothing to copy */
	}

	ExternalVariable& ExternalVariable::operator=( const ExternalVariable& )
	{
	    /* Nothing to copy */
	    return *this;
	}

	ExternalVariable::~ExternalVariable()
	{
	}

	/* static */ ExternalVariable * ExternalVariable::clone( const ExternalVariable * src )
	{
	    if ( src == nullptr ) return nullptr;
	    return src->clone();
	}

	/* static */ std::ostream& ExternalVariable::printVariableName( std::ostream& output, const ExternalVariable& var )
	{
	    return var.printVariableName( output );
	}

	/* static */ bool ExternalVariable::isPresent( const ExternalVariable * var )
	{
	    /* Any non-negative number */
	    double value = 0.0;
#if BUG_315
	    /* expanded out for debugging and return false on var == 0.  */ 
	    if ( var == nullptr ) return false;
	    if ( !var->wasSet() || !var->getValue(value) ) return true;
	    if ( !std::isfinite(value) ) return false;
//	    if ( value != 0 ) return true;
//	    return false;
	    if ( value > 0. ) return true;
	    if ( value < 0. ) return false;
	    return true;
#else
	    return var != nullptr && (!var->wasSet() || !var->getValue(value) || (std::isfinite(value) && value >= 0. ));
#endif
	}


	/* static */ bool ExternalVariable::isPresent( const ExternalVariable * var, double default_value )
	{
	    /* Any number greater than the default value */
	    double value = 0.0;
	    return var != nullptr && (!var->wasSet() || !var->getValue(value) || (std::isfinite(value) && value != default_value));
	}


	/* static */ bool ExternalVariable::isDefault( const ExternalVariable * var, double default_value )
	{
	    double value = 0.0;
	    return var == nullptr || (var->wasSet() && var->getValue(value) && value == default_value);
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	ConstantExternalVariable::ConstantExternalVariable(double constant) : _variableType( Type::DOUBLE )
	{	
	    _value.d = constant;
	}

	ConstantExternalVariable::ConstantExternalVariable(const char * string) : _variableType( Type::STRING )
	{	
	    _value.s = strdup(string);
	}

	ConstantExternalVariable::ConstantExternalVariable( const ExternalVariable& src )
	{
	    _variableType = src.getType();
	    if ( src.getType() == Type::DOUBLE ) {
		if ( !src.getValue( _value.d ) ) throw std::domain_error( "unassigned variable" );
	    } else if ( src.getType() == Type::STRING ) {
		const char * s = 0;
		if ( !src.getString( s ) )  throw std::domain_error( "unassigned variable" );
		_value.s = strdup( s );
	    }
	}

	ConstantExternalVariable& ConstantExternalVariable::operator=( const ConstantExternalVariable& src )
	{
	    if ( _variableType == Type::STRING ) {
		free ( const_cast<char *>(_value.s) );
	    }
	    _variableType = src._variableType;
	    if ( src._variableType == Type::DOUBLE ) {
		_value.d = src._value.d;
	    } else if ( src._variableType == Type::STRING ) {
		_value.s = strdup( src._value.s );
	    }
	    return *this;
	}

	ConstantExternalVariable& ConstantExternalVariable::operator=( double value )
	{
	    if ( _variableType == Type::STRING ) {
		free ( const_cast<char *>(_value.s) );
	    }
	    _variableType = Type::DOUBLE;
	    _value.d = value;
	    return *this;
	}

	ConstantExternalVariable * ConstantExternalVariable::clone() const
	{
	    return new ConstantExternalVariable(*this);
	}

	ConstantExternalVariable::~ConstantExternalVariable()
	{
	    if ( _variableType == Type::STRING ) {
		free ( const_cast<char *>(_value.s) );
	    }
	}

	void ConstantExternalVariable::set(double value)
	{
	    if ( _variableType == Type::STRING ) {
		free ( const_cast<char *>(_value.s) );
	    }
	    _variableType = Type::DOUBLE;
	    _value.d = value;
	}

	void ConstantExternalVariable::setString(const char * string)
	{
	    if ( _variableType == Type::STRING ) {
		free ( const_cast<char *>(_value.s) );
	    }
	    _variableType = Type::STRING;
	    _value.s = string;
	}

	bool ConstantExternalVariable::getValue(double& result) const
	{
	    if ( _variableType == Type::DOUBLE ) {
		result = _value.d;
		return true;
	    } else {
		return false;
	    }
	}

	bool ConstantExternalVariable::getString(const char *& result) const
	{
	    if ( _variableType == Type::STRING ) {
		result = _value.s;
		return true;
	    } else {
		return false;
	    }
	}

	const std::string ConstantExternalVariable::getName() const
	{
	    switch ( getType() ) {
	    case Type::DOUBLE: std::to_string( _value.d );
	    case Type::STRING: return _value.s;
	    default: return "<<unassigned>>";
	    }
	}

	bool ConstantExternalVariable::wasSet() const
	{
	    return _variableType != Type::UNASSIGNED;
	}


	std::ostream& ConstantExternalVariable::print( std::ostream& output ) const
	{
	    switch ( getType() ) {
	    case Type::DOUBLE: output << _value.d; break;
	    case Type::STRING: output << _value.s; break;
	    default: output << "<<unassigned>>"; break;
	    }
	    return output;
	}

	std::ostream& ConstantExternalVariable::printVariableName( std::ostream& output ) const
	{
	    return print( output );
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	SymbolExternalVariable::SymbolExternalVariable(const std::string & name) :
	    _externalSymbol(nullptr), _name(name)
	{
	}

	SymbolExternalVariable::SymbolExternalVariable(const SymbolExternalVariable& src ) :
	    _externalSymbol(src._externalSymbol), _name(src._name)
	{
	}


	SymbolExternalVariable& SymbolExternalVariable::operator=(const SymbolExternalVariable& src )
	{
	    double v;
	    const char *s;
	    if (src.getValue(v)) {
		_externalSymbol->assignDouble(v);
	    } else if (src.getString(s)) {
		_externalSymbol->assignString(s);
	    } else {
		throw std::domain_error("unassigned variable");
	    }
	    return *this;
	}


	SymbolExternalVariable * SymbolExternalVariable::clone() const
	{
	    return new SymbolExternalVariable(*this);
	}

	SymbolExternalVariable::~SymbolExternalVariable()
	{
	}


	bool SymbolExternalVariable::registerInEnvironment(LQX::Program* pgm)
	{
	    /* Obtain a symbol for registration and set the double */
	    _externalSymbol = pgm->defineExternalVariable(_name);
	    if (_externalSymbol == nullptr) {
		return false;
	    } else {
		_externalSymbol->assignNull();
		return true;
	    }
	}

	ExternalVariable::Type SymbolExternalVariable::getType() const
	{
	    if ( !(_externalSymbol == nullptr) ) {
		switch ( _externalSymbol->getType() ) {
		case LQX::Symbol::SYM_DOUBLE: return Type::DOUBLE;
		case LQX::Symbol::SYM_STRING: return Type::STRING;
		default: break;		/* Ignore */
		}
	    }
	    return Type::UNASSIGNED;
	}

	void SymbolExternalVariable::set(double value)
	{
	    /* If unregistered set the initial */
	    if (_externalSymbol == nullptr) {
		throw std::domain_error("unassigned variable");
	    } else {
		_externalSymbol->assignDouble(value);
	    }
	}

	bool SymbolExternalVariable::getValue(double& result) const
	{
	    /* If unregistered return the initial */
	    if (_externalSymbol == nullptr) {
		throw std::domain_error("unassigned variable");
	    } else if (_externalSymbol->getType() == LQX::Symbol::SYM_DOUBLE) {
		result = _externalSymbol->getDoubleValue();
		return true;
	    } else {
		return false;
	    }
	}

	void SymbolExternalVariable::setString(const char * value)
	{
	    /* If unregistered set the initial */
	    if (_externalSymbol == nullptr) {
		throw std::domain_error("unassigned variable");
	    } else {
		_externalSymbol->assignString(value);
	    }
	}

	bool SymbolExternalVariable::getString(const char *& result) const
	{
	    /* If unregistered return the initial */
	    if (_externalSymbol == nullptr) {
		throw std::domain_error("unassigned variable");
	    } else if (_externalSymbol->getType() == LQX::Symbol::SYM_STRING) {
		result = _externalSymbol->getStringValue();
		return true;
	    } else {
		return false;
	    }
	}

	bool SymbolExternalVariable::wasSet() const
	{
	    /* This is just a very basic check */
	    return _externalSymbol != nullptr && (_externalSymbol->getType() == LQX::Symbol::SYM_DOUBLE || _externalSymbol->getType() == LQX::Symbol::SYM_STRING);
	}

	std::ostream& SymbolExternalVariable::print( std::ostream& output ) const
	{
	    switch (  getType() ) {
	    case LQIO::DOM::ExternalVariable::Type::DOUBLE: output << to_double( *this ); break;
	    case LQIO::DOM::ExternalVariable::Type::STRING: output << to_string( *this ); break;
	    default: output << _name; break;
	    }
	    return output;
	}

	std::ostream& SymbolExternalVariable::printVariableName( std::ostream& output ) const
	{
	    output << _name;
	    return output;
	}

    }
}
