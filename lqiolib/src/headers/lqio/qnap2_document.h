/* -*- C++ -*-
 *  $Id: qnap2_document.h 14370 2021-01-16 19:40:56Z greg $
 *
 *  Created by Greg Franks 2020/12/28
 */

#ifndef __LQIO_QNAP2_DOCUMENT__
#define __LQIO_QNAP2_DOCUMENT__

#include <iostream>
#include <map>
#include <set>
#include <string>
#include "bcmp_document.h"
#include "srvn_spex.h"
#include "xml_output.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}
namespace LQX {
    class SyntaxTreeNode;
}

namespace BCMP {
    class QNAP2_Document {
    public:
	QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model );
	virtual ~QNAP2_Document() {}

	std::ostream& print( std::ostream& ) const;

    private:
	const Model::Model::Station::map_t& stations() const { return _model.stations(); }
	const Model::Class::map_t& classes() const { return _model.classes(); }
	const Model& model() const { return _model; }

	void printClassVariables( std::ostream& ) const;

	static std::ostream& printKeyword( std::ostream&, const std::string& s1, const std::string& s2 );
	static std::ostream& printStatement( std::ostream&, const std::string& s1, const std::string& s2 );
	static XML::StringManip qnap2_statement( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printStatement, s1, s2 ); }
	static XML::StringManip qnap2_keyword( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printKeyword, s1, s2 ); }
	static std::string to_real( const LQIO::DOM::ExternalVariable* v );
	static std::string to_unsigned( const LQIO::DOM::ExternalVariable* v );

    private:
	struct getVariables {
	    getVariables( const Model& model, std::set<const LQIO::DOM::ExternalVariable *>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
	    std::string operator()( const std::string&, const Model::Class::pair_t& k ) const;
	    std::string operator()( const std::string&, const Model::Station::pair_t& m ) const;
	    std::string operator()( const std::string&, const Model::Station::Demand::pair_t& d ) const;
	private:
	    const Model& model() const { return _model; }
	    const Model::Class::map_t& classes() const { return _model.classes(); }	/* For demand */
	    const Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	private:
	    const Model& _model;
	    mutable std::set<const LQIO::DOM::ExternalVariable *> _symbol_table;
	};
	    
	static std::string getDeferredVariables( const std::string&, const std::pair<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& );
	static std::string getResultVariables( const std::string&, const Spex::var_name_and_expr& );

	struct printStation {
	    printStation( std::ostream& output, const Model& model ) : _output(output), _model(model) {}
	    void operator()( const Model::Station::pair_t& m ) const;
	private:
	    const Model::Class::map_t& classes() const { return _model.classes(); }	/* For demand */
	    const Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	private:
	    std::ostream& _output;
	    const Model& _model;
	};

	struct printTransit {
	    printTransit( const std::string& name ) : _name(name) {}
	    std::string operator()( const std::string&, const Model::Station::pair_t& ) const;
	private:
	    const std::string& _name;
	};
    
	struct printStationVariables {
	    printStationVariables( std::ostream& output, const Model& model ) : _output(output), _model(model) {}
	    void operator()( const Model::Station::pair_t& m ) const;
	private:
	    const Model::Class::map_t& classes() const { return _model.classes(); }
	private:
	    std::ostream& _output;
	    const Model& _model;
	};

	struct printSPEXScalars {
	    printSPEXScalars( std::ostream& output ) : _output(output) {}
	    void operator()( const std::pair<std::string, const LQX::SyntaxTreeNode *>& ) const;
	private:
	    std::ostream& _output;
	};
	
	struct printSPEXDeferred {
	    printSPEXDeferred( std::ostream& output ) : _output(output) {}
	    void operator()( const std::pair<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& ) const;
	private:
	    std::ostream& _output;
	};
	
	class getObservations {
	public:
	    typedef std::string (getObservations::*f)( const std::string& ) const;
	    static std::map<int,f> __key_map;	/* Maps srvn_gram.h KEY_XXX to qnap2 function */
	    
	    getObservations( const Model& model ) : _model(model) {}
	    std::string operator()( const std::string&, const Spex::var_name_and_expr& ) const;
	    std::string get_throughput( const std::string& name ) const;
	    std::string get_utilization( const std::string& name ) const;
	    std::string get_service_time( const std::string& name ) const;
	    std::string get_waiting_time( const std::string& name ) const;
	private:
	    const Model::Class::map_t& classes() const { return _model.classes(); }
	    const Model::Model::Station::map_t& stations() const { return _model.stations(); }
	private:
	    const Model& _model;
	};

	struct for_loop {
	    for_loop( std::ostream& output ) : _output(output) {}
	    void operator()( const std::string& ) const;
	private:
	    std::ostream& _output;
	};
	
	struct end_for {
	    end_for( std::ostream& output ) : _output(output) {}
	    void operator()( const std::string& ) const;
	private:
	    std::ostream& _output;
	};
	
	struct fold_service_time {
	    fold_service_time() {}
	    std::string operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const;
	};
	    
	struct fold_visit {
	    fold_visit( const std::string& name ) : _name(name) {}
	    std::string operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const;
	private:
	    const std::string& _name;
	};

    private:
	const std::string _input_file_name;
	BCMP::Model _model;

	static std::map<scheduling_type,std::string> __scheduling_str;	/* Maps scheduling_type to qnap2 keyword */
    };

    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document& doc ) { return doc.print(output); }
}
#endif
