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
 * $Id: qnio_document.h 16003 2022-10-19 17:22:13Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(QN_DOCUMENT_H)
#define QN_DOCUMENT_H
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "bcmp_document.h"
#include "dom_pragma.h"

namespace LQX {
    class Program;
}
namespace BCMP {
    class Model;
}

namespace QNIO {
    class Document {
    public:
	Document( const std::string& input_file_name, const BCMP::Model& model );
	virtual ~Document();

	virtual bool load() = 0;

	const BCMP::Model& model() const { return _model; }
	BCMP::Model& model() { return _model; }
	const std::string& getInputFileName() const { return _input_file_name; }
	
	virtual LQX::Program * getLQXProgram() const { return _lqx_program; }
    protected:
	void setLQXProgram( LQX::Program * program ) { _lqx_program = program; }
    public:
	virtual void registerExternalSymbolsWithProgram( LQX::Program * ) {}	/* Might hoist */
	virtual std::vector<std::string> getUndefinedExternalVariables() const { return std::vector<std::string>(); }
	const std::map<std::string,std::string>& getPragmaList() const { return _pragmas.getList(); }

	bool hasPragmas() const { return !_pragmas.empty(); }

	void insertPragma( const std::string param, const std::string value ) { _pragmas.insert( param, value ); }
	void mergePragmas( const std::map<std::string,std::string>& list ) { _pragmas.merge( list ); }

	virtual void plot( BCMP::Model::Result::Type, const std::string& ) {}

	virtual std::ostream& print( std::ostream& output ) const = 0;
    
    private:
	const std::string _input_file_name;
	LQIO::DOM::Pragma _pragmas;
	BCMP::Model _model;
	LQX::Program * _lqx_program;
    };
}
#endif
