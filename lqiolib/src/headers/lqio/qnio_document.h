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
 * $Id: qnio_document.h 16149 2022-12-01 02:58:03Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(QN_DOCUMENT_H)
#define QN_DOCUMENT_H
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <deque>
#include "bcmp_document.h"
#include "dom_pragma.h"

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
	    /* Variable = begin(); variable < end(); variable += step() */
	public:
	    friend std::ostream& operator<<( std::ostream& output, const QNIO::Document::Comprehension& comprehension ) { return comprehension.print( output ); }
	    
	    Comprehension( const std::string name, const std::string& s, bool integer ) : _name(name), _begin(0.), _step(0.), _size(0) { convert(s,integer); }
	    Comprehension& operator=( const Comprehension& );

	    LQX::VariableExpression * getVariable() const;
	    const std::string& name() const { return _name; }
	    size_t size() const { return _size; }
	    double begin() const { return _begin; }				// Like an iterator
	    double end() const { return begin() + size() * step(); }		// Like an iterator
	    double step() const { return _step; }
	    double max() const { return begin() + step() * (size() - 1); }	// Largest possible value

	    LQX::SyntaxTreeNode * collect( std::vector<LQX::SyntaxTreeNode *>* ) const;

	    std::ostream& print( std::ostream& ) const;

	    struct find {
		find( const std::string& name ) : _name(name) {}
		bool operator()( const Comprehension& comprehension ) const { return comprehension._name == _name; }
	    private:
		const std::string _name;
	    };
	    
	private:
	    void convert( const std::string&, bool );

	    std::string _name;
	    double _begin;
	    double _step;
	    size_t _size;
	};

    public:
	Document( const std::string& input_file_name, const BCMP::Model& model );
	virtual ~Document();

	virtual bool load() = 0;

	const BCMP::Model& model() const { return _model; }
	BCMP::Model& model() { return _model; }
	const std::string& getInputFileName() const { return _input_file_name; }
	const std::deque<Comprehension>& comprehensions() const { return _comprehensions; }		/* For loops from WhatIf */
	
	void setEnvironment( LQX::Environment * environment ) { _model.setEnvironment( environment ); }
	virtual LQX::Program * getLQXProgram() = 0;
	virtual bool preSolve() { return true; }
	virtual bool postSolve() { return true; }
    protected:
	void insertComprehension( const Comprehension& comprehension ) { _comprehensions.emplace_front( comprehension); }
    public:
	virtual void registerExternalSymbolsWithProgram( LQX::Program * ) {}	/* Might hoist */
	virtual std::vector<std::string> getUndefinedExternalVariables() const { return std::vector<std::string>(); }
	const std::map<std::string,std::string>& getPragmaList() const { return _pragmas.getList(); }

	bool hasPragmas() const { return !_pragmas.empty(); }

	void insertPragma( const std::string param, const std::string value ) { _pragmas.insert( param, value ); }
	void mergePragmas( const std::map<std::string,std::string>& list ) { _pragmas.merge( list ); }

	virtual bool disableDefaultOutputWithLQX() const { return false; }
	virtual void plot( BCMP::Model::Result::Type, const std::string& ) {}

	virtual std::ostream& print( std::ostream& output ) const = 0;
    
    private:
	const std::string _input_file_name;
	LQIO::DOM::Pragma _pragmas;
	BCMP::Model _model;
	std::deque<Comprehension> _comprehensions; 			/* For loops from WhatIf */
    };
}
#endif
