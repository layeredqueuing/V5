/* -*- C++ -*-
 *  $Id: qnap2_document.h 14318 2021-01-02 00:55:54Z greg $
 *
 *  Created by Greg Franks 2020/12/28
 */

#ifndef __LQIO_QNAP2_DOCUMENT__
#define __LQIO_QNAP2_DOCUMENT__

#include "bcmp_document.h"
#include "xml_output.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
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
	    getVariables( const Model& model ) : _model(model) {}
	    std::string operator()( const std::string&, const Model::Class::pair_t& k ) const;
	    std::string operator()( const std::string&, const Model::Station::pair_t& m ) const;
	    std::string operator()( const std::string&, const Model::Station::Demand::pair_t& d ) const;
	private:
	    const Model& model() const { return _model; }
	    const Model::Class::map_t& classes() const { return _model.classes(); }	/* For demand */
	    const Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	private:
	    const Model& _model;
	};
	    
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

	static std::map<scheduling_type,std::string> __scheduling_str;
    };

    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document& doc ) { return doc.print(output); }
}
#endif
