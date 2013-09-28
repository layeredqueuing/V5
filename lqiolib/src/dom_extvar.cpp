/*
 *  $Id$
 *
 *  Created by Martin Mroz on 02/03/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_extvar.h"
#include <cstdlib>

namespace LQIO {
    namespace DOM {
    
	double to_double( const LQIO::DOM::ExternalVariable& arg )
	{
	    double value;
	    if ( !arg.getValue( value ) ) throw std::domain_error( "" );
	    return value;
	}


	std::ostream& 
	operator<<( std::ostream& output, const LQIO::DOM::ExternalVariable& self )
	{
	    return self.print( output );
	}

	LQIO::DOM::ConstantExternalVariable operator*( const LQIO::DOM::ExternalVariable& multiplicand, const double multiplier_d ) throw (std::domain_error)
	{
	    double multiplicand_d;
	    if ( !multiplicand.getValue( multiplicand_d ) ) throw std::domain_error( "" );
	    ConstantExternalVariable product( multiplicand_d * multiplier_d );
	    return product;
	}

	LQIO::DOM::ConstantExternalVariable operator*( const double multiplicand_d, const LQIO::DOM::ExternalVariable& multiplier ) throw (std::domain_error)
	{
	    double multiplier_d;
	    if ( !multiplier.getValue( multiplier_d ) ) throw std::domain_error( "" );
	    ConstantExternalVariable product( multiplicand_d * multiplier_d );
	    return product;
	}

	LQIO::DOM::ConstantExternalVariable operator*( const LQIO::DOM::ExternalVariable& multiplicand, const LQIO::DOM::ExternalVariable& multiplier ) throw (std::domain_error)
	{
	    double multiplier_d, multiplicand_d;
	    if ( !multiplier.getValue( multiplier_d ) ) throw std::domain_error( "" );
	    if ( !multiplicand.getValue( multiplicand_d ) ) throw std::domain_error( "" );
	    ConstantExternalVariable product( multiplicand_d * multiplier_d );
	    return product;
	}

	LQIO::DOM::ConstantExternalVariable operator/( const LQIO::DOM::ExternalVariable& dividend, const double divisor ) throw (std::domain_error)
	{
	    double dividend_d;
	    if ( !dividend.getValue( dividend_d ) ) throw std::domain_error( "" );
	    ConstantExternalVariable quotient( dividend_d / divisor );
	    return quotient;
	}

	LQIO::DOM::ConstantExternalVariable operator/( const double dividend, const LQIO::DOM::ExternalVariable& divisor ) throw (std::domain_error)
	{
	    double divisor_d;
	    if ( !divisor.getValue( divisor_d ) ) throw std::domain_error( "" );
	    ConstantExternalVariable quotient( dividend / divisor_d );
	    return quotient;
	}

	LQIO::DOM::ConstantExternalVariable operator/( const LQIO::DOM::ExternalVariable& dividend, const LQIO::DOM::ExternalVariable& divisor ) throw (std::domain_error)
	{
	    double dividend_d, divisor_d;
	    if ( !dividend.getValue( dividend_d ) ) throw std::domain_error( "" );
	    if ( !divisor.getValue( divisor_d ) ) throw std::domain_error( "" );
	    ConstantExternalVariable quotient( dividend_d / divisor_d );
	    return quotient;
	}

	LQIO::DOM::ConstantExternalVariable operator+( const LQIO::DOM::ExternalVariable& arg1, const double arg2 ) throw (std::domain_error)
	{
	    double value;
	    if ( !arg1.getValue( value ) ) throw std::domain_error( "" );
	    ConstantExternalVariable sum( value + arg2 );
	    return sum;
	}

	LQIO::DOM::ConstantExternalVariable operator+( const double arg1, const LQIO::DOM::ExternalVariable& arg2 ) throw (std::domain_error)
	{
	    double value;
	    if ( !arg2.getValue( value ) ) throw std::domain_error( "" );
	    ConstantExternalVariable sum( arg1 + value );
	    return sum;
	}

	LQIO::DOM::ConstantExternalVariable operator+( const LQIO::DOM::ExternalVariable& arg1, const LQIO::DOM::ExternalVariable& arg2 ) throw (std::domain_error)
	{
	    double value1, value2;
	    if ( !arg1.getValue( value1 ) ) throw std::domain_error( "" );
	    if ( !arg2.getValue( value2 ) ) throw std::domain_error( "" );
	    ConstantExternalVariable sum( value1 + value2 );
	    return sum;
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
    
	ExternalVariable& ExternalVariable::operator=( const ExternalVariable& )  throw (std::domain_error)
	{
	    /* Nothing to copy */
	    return *this;
	}

	ExternalVariable::~ExternalVariable()
	{
	}
    
	ExternalVariable&
	ExternalVariable::operator*=( const ExternalVariable& arg ) throw (std::domain_error)
	{
	    double multiplier, multiplicand;
	    
	    if ( !arg.getValue( multiplier ) ) throw std::domain_error( "" );
	    if ( !getValue( multiplicand ) ) throw std::domain_error( "" );
	    set( multiplier * multiplicand );
	    return *this;
	}

	ExternalVariable&
	ExternalVariable::operator*=( const double multiplier ) throw (std::domain_error)
	{
	    double multiplicand;
	    
	    if ( !getValue( multiplicand ) ) throw std::domain_error( "" );
	    set( multiplier * multiplicand );
	    return *this;
	}

	ExternalVariable&
	ExternalVariable::operator+=( const ExternalVariable& arg ) throw (std::domain_error)
	{
	    double addend, augend;
	    
	    if ( !getValue( addend ) ) throw std::domain_error( "" );
	    if ( !arg.getValue( augend ) ) throw std::domain_error( "" );
	    set( addend + augend );
	    return *this;
	}

	ExternalVariable&
	ExternalVariable::operator+=( const double addend ) throw (std::domain_error)
	{
	    double augend;
	    
	    if ( !getValue( augend ) ) throw std::domain_error( "" );
	    set( addend + augend );
	    return *this;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	ConstantExternalVariable::ConstantExternalVariable(double constant) : _value(constant)
	{
	}
    
	ConstantExternalVariable::ConstantExternalVariable( const ExternalVariable& src )
	{
	    if ( !src.getValue( _value ) ) throw std::domain_error( "" );
	}

	ConstantExternalVariable& ConstantExternalVariable::operator=( const ConstantExternalVariable& src )  throw (std::domain_error)
	{
	    _value = src._value;
	    return *this;
	}

	ConstantExternalVariable * ConstantExternalVariable::clone() const
	{
	    return new ConstantExternalVariable(*this);
	}

	ConstantExternalVariable::~ConstantExternalVariable()
	{
	}
    
	void ConstantExternalVariable::set(double value)
	{
	    _value = value;
	}
    
	bool ConstantExternalVariable::getValue(double& result) const
	{
	    result = _value;
	    return true;
	}
    
	bool ConstantExternalVariable::wasSet() const
	{
	    return true;
	}

	std::ostream& ConstantExternalVariable::print( std::ostream& output ) const
	{
	    output << _value;
	    return output;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	SymbolExternalVariable::SymbolExternalVariable(const std::string & name) : 
	    _externalSymbol(NULL), _name(name), _initial(0.0)
	{
	}
    
	SymbolExternalVariable::SymbolExternalVariable(const SymbolExternalVariable& src ) : 
	    _externalSymbol(src._externalSymbol), _name(src._name), _initial(src._initial)
	{
	    abort();
	}


	SymbolExternalVariable& SymbolExternalVariable::operator=(const SymbolExternalVariable& src )  throw (std::domain_error)
	{
	    double v;
	    if (src.getValue(v)) {
		_externalSymbol->assignDouble(v);
	    } else {
		throw std::domain_error("");
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
	    if (_externalSymbol == NULL) {
		return false;
	    } else {
		_externalSymbol->assignNull();
		return true;
	    }
	}
    
	void SymbolExternalVariable::set(double value)
	{
	    /* If unregistered set the initial */
	    if (_externalSymbol == NULL) {
		abort();
		_initial = value;
		return;
	    } else {
		_externalSymbol->assignDouble(value);
	    }
	}
    
	bool SymbolExternalVariable::getValue(double& result) const
	{
	    /* If unregistered return the initial */
	    if (_externalSymbol == NULL) {
		abort();
		result = _initial;
		return true;
	    } else {
		if (_externalSymbol->getType() != LQX::Symbol::SYM_DOUBLE) {
		    return false;
		} else {
		    result = _externalSymbol->getDoubleValue();
		    return true;
		}
	    }
	}
    
	bool SymbolExternalVariable::wasSet() const
	{
	    /* This is just a very basic check */
	    if (_externalSymbol == NULL) {
		return false;
	    } else if (_externalSymbol->getType() != LQX::Symbol::SYM_DOUBLE) {
		return false;
	    } else {
		return true;
	    }
	}
    
	std::ostream& SymbolExternalVariable::print( std::ostream& output ) const
	{
	    output << _name;
	    return output;
	}

    };
};
