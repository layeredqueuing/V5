/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqiolib/src/headers/lqio/qnio_document.h $
 *
 * Proxy for the input model.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November 2022
 *
 * $Id: qnio_document.h 17136 2024-03-22 01:40:35Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef QNIO_DOCUMENT_H
#define QNIO_DOCUMENT_H
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "bcmp_document.h"
#include "dom_pragma.h"
#include "gnuplot.h"

namespace LQX {
    class Program;
    class VariableExpression;
}
namespace BCMP {
    class Model;
}

namespace QNIO {
    class Document {

    protected:
	class Comprehension
	{
	public:
	    enum class Type { ARRIVAL_RATES, CUSTOMERS, SERVERS, DEMANDS, SCALE };

	    /* Variable = begin(); variable < end(); variable += step() */
	public:
	    friend std::ostream& operator<<( std::ostream& output, const QNIO::Document::Comprehension& comprehension ) { return comprehension.print( output ); }
	    
	    Comprehension( const std::string& name, Type type, const std::string& s ) : _name(name), _type(type), _begin(0.), _step(0.), _size(0) { convert(s); }
	    Comprehension& operator=( const Comprehension& );

	    LQX::VariableExpression * getVariable() const;
	    const std::string& name() const { return _name; }
	    const std::string& typeName() const { return __type_name.at(type()); }
	    Type type() const { return _type; }
	    size_t size() const { return _size; }
	    double begin() const { return _begin; }				// Like an iterator
	    double end() const { return begin() + size() * step(); }		// Like an iterator
	    double step() const { return _step; }
	    double max() const { return begin() + step() * (size() - 1); }	// Largest possible value
	    bool isInteger() const { return type() == Type::CUSTOMERS || type() == Type::SERVERS; }

	    LQX::SyntaxTreeNode * collect( std::vector<LQX::SyntaxTreeNode *>* ) const;

	    std::ostream& print( std::ostream& ) const;

	    struct find {
		find( const std::string& name ) : _name(name) {}
		bool operator()( const Comprehension& comprehension ) const { return comprehension._name == _name; }
	    private:
		const std::string _name;
	    };

	private:
	    void convert( const std::string& );

	    std::string _name;
	    const Type _type;
	    double _begin;
	    double _step;
	    size_t _size;

	public:
	    const static std::map<Type,const std::string> __type_name;
	};

    public:
	enum class InputFormat { JMVA, QNAP };
	
	Document( const std::string& input_file_name, const BCMP::Model& model );
	Document( const BCMP::Model& model );
	Document( const Document& );
	virtual ~Document();

	virtual bool load() = 0;

	const BCMP::Model& model() const { return _model; }
	BCMP::Model& model() { return _model; }
	const std::string& getComment() const { return _comment; }
	void setComment( const std::string& comment ) { _comment = comment; }
	bool boundsOnly() const { return _bounds_only; }
	void setBoundsOnly( bool value ) { _bounds_only = value; }
	const std::string& getInputFileName() const { return _input_file_name; }
	virtual InputFormat getInputFormat() const = 0;
	const std::deque<Comprehension>& comprehensions() const { return _comprehensions; }		/* For loops from WhatIf */
	bool hasVariable( const std::string& name ) const { return _input_variables.find(name) != _input_variables.end(); }
	const std::map<const std::string,LQX::SyntaxTreeNode*>::iterator insertInputVariable( const std::string& name, LQX::SyntaxTreeNode * init=nullptr ) { return _input_variables.emplace(name,init).first; }
	const std::map<const std::string,LQX::SyntaxTreeNode *>& input_variables() const { return _input_variables; }
	
	void setLQXEnvironment( LQX::Environment * environment ) { _model.setEnvironment( environment ); }
	LQX::Environment * getLQXEnvironment() const { return _model.environment(); }
	virtual LQX::Program * getLQXProgram() = 0;
	virtual bool preSolve() { return true; }
	virtual bool postSolve() { return true; }
    protected:
	void insertComprehension( const Comprehension& comprehension ) { _comprehensions.emplace_front( comprehension ); }
    public:
	virtual void registerExternalSymbolsWithProgram( LQX::Program * ) {}	/* Might hoist */
	virtual std::vector<std::string> getUndefinedExternalVariables() const { return std::vector<std::string>(); }
	virtual unsigned getSymbolExternalVariableCount() const { return 0; }
	const std::map<std::string,std::string>& getPragmaList() const { return _pragmas.getList(); }

	bool hasPragmas() const { return !_pragmas.empty(); }

	void insertPragma( const std::string param, const std::string value ) { _pragmas.insert( param, value ); }
	void mergePragmas( const std::map<std::string,std::string>& list ) { _pragmas.merge( list ); }

	virtual bool disableDefaultOutputWithLQX() const { return false; }
	virtual void saveResults( size_t, const std::string&, size_t, const std::string&, const std::string&, const std::map<BCMP::Model::Result::Type,double>& ) {}
	virtual void plot( BCMP::Model::Result::Type, const std::string&, LQIO::GnuPlot::Format format=LQIO::GnuPlot::Format::TERMINAL ) {}

	bool convertToLQN( LQIO::DOM::Document& ) const;

	virtual std::ostream& print( std::ostream& output ) const = 0;
	virtual std::ostream& exportModel( std::ostream& output ) const = 0;
    
    private:
	const std::string _input_file_name;
	std::string _comment;
	LQIO::DOM::Pragma _pragmas;
	bool _bounds_only;
	BCMP::Model _model;
	std::map<const std::string,LQX::SyntaxTreeNode *> _input_variables;	/* Spex vars and inital values	*/
	std::deque<Comprehension> _comprehensions; 				/* For loops from WhatIf */
    };
}
#endif
