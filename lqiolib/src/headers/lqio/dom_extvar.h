/*
 *  $Id: dom_extvar.h 12338 2015-12-01 17:12:23Z greg $
 *
 *  Created by Martin Mroz on 02/03/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_EXTVAR__
#define __LQIO_DOM_EXTVAR__

#include <string>
#include <ostream>
#include <stdexcept>
#include <lqx/SymbolTable.h>
#include <lqx/Program.h>

namespace LQIO {
    namespace DOM {
    
	class ConstantExternalVariable;
	
	class ExternalVariable {
	    friend std::ostream& operator<<( std::ostream&, const LQIO::DOM::ExternalVariable& );
	    
	public:
      
	    /* Designated Initializers for Variables */
	    ExternalVariable();
	    ExternalVariable& operator=( const ExternalVariable& )  throw (std::domain_error);
	    virtual ExternalVariable * clone() const = 0;
	    virtual ~ExternalVariable();

	protected:
	    ExternalVariable( const ExternalVariable& );

	public:
	    ExternalVariable& operator*=( const ExternalVariable& ) throw (std::domain_error);
	    ExternalVariable& operator*=( const double ) throw (std::domain_error);
	    ExternalVariable& operator+=( const ExternalVariable& ) throw (std::domain_error);
	    ExternalVariable& operator+=( const double ) throw (std::domain_error);
	    
	    /* Obtaining the Value */
	    virtual void set(double value) = 0;
	    virtual bool getValue(double& result) const = 0;
	    virtual const std::string& getName() const = 0;
	    virtual bool wasSet() const = 0;

	protected:
	    virtual std::ostream& print( std::ostream& ) const = 0;
	};
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	class ConstantExternalVariable : public ExternalVariable {
	public:
	    friend LQIO::DOM::ConstantExternalVariable operator*( const LQIO::DOM::ExternalVariable&, const double ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator*( const double, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator*( const LQIO::DOM::ExternalVariable&, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator/( const LQIO::DOM::ExternalVariable&, const double ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator/( const double, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator/( const LQIO::DOM::ExternalVariable&, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator+( const LQIO::DOM::ExternalVariable&, const double ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator+( const double, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	    friend LQIO::DOM::ConstantExternalVariable operator+( const LQIO::DOM::ExternalVariable&, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);

	public:
	    /* Initializing the constant external variable */
	    ConstantExternalVariable(double constant);
	    ConstantExternalVariable& operator=( const ConstantExternalVariable& ) throw (std::domain_error);
	    ConstantExternalVariable( const ExternalVariable& );
	    virtual ConstantExternalVariable * clone() const;
	    virtual ~ConstantExternalVariable();
      

	public:
	    /* Obtaining the Value */
	    virtual void set(double value);
	    virtual bool getValue(double& result) const;
	    virtual const std::string& getName() const;
	    virtual bool wasSet() const;
      
	protected:
	    virtual std::ostream& print( std::ostream& ) const;

	private:
      
	    /* The stored value */
	    double _value;
      
	};
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	class SymbolExternalVariable : public ExternalVariable {
	public:
      
	    /* Designated initializers once again */
	    SymbolExternalVariable(const std::string& name);
	    SymbolExternalVariable& operator=( const SymbolExternalVariable& ) throw (std::domain_error);
	    virtual SymbolExternalVariable * clone() const;
	    virtual ~SymbolExternalVariable();
      
	protected:
	    SymbolExternalVariable( const SymbolExternalVariable& );

	public:
	    /* Registering in an environment */
	    bool registerInEnvironment(LQX::Program* pgm);
      
	    /* Obtaining the Value */
	    virtual void set(double value);
	    virtual bool getValue(double& result) const;
	    virtual bool wasSet() const;
	    virtual const std::string& getName() const { return _name; }
      
	protected:
	    virtual std::ostream& print( std::ostream& ) const;

	private:
      
	    /* This one's a bit more complicated */
	    LQX::SymbolAutoRef _externalSymbol;
	    std::string _name;
	    double _initial;
      
	};
    
	double to_double( const LQIO::DOM::ExternalVariable& );
	LQIO::DOM::ConstantExternalVariable operator*( const LQIO::DOM::ExternalVariable&, const double ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator*( const double, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator*( const LQIO::DOM::ExternalVariable&, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator/( const LQIO::DOM::ExternalVariable&, const double ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator/( const double, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator/( const LQIO::DOM::ExternalVariable&, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator+( const LQIO::DOM::ExternalVariable&, const double ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator+( const double, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
	LQIO::DOM::ConstantExternalVariable operator+( const LQIO::DOM::ExternalVariable&, const LQIO::DOM::ExternalVariable& ) throw (std::domain_error);
    };
};
#endif /* __LQIO_DOM_EXTVAR__ */
