/* -*- C++ -*-
 *  $Id: qnap2_document.h 14288 2020-12-29 13:24:52Z greg $
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

	void printClassVariables( std::ostream& ) const;

	static std::ostream& printKeyword( std::ostream&, const std::string& s1, const std::string& s2 );
	static std::ostream& printStatement( std::ostream&, const std::string& s1, const std::string& s2 );
	static XML::StringManip qnap2_statement( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printStatement, s1, s2 ); }
	static XML::StringManip qnap2_keyword( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printKeyword, s1, s2 ); }
	static std::string to_real( double v );

    private:
	struct printStation {
	    printStation( std::ostream& output, const Model::Class::map_t& classes, const Model::Station::map_t& stations ) : _output(output), _classes(classes), _stations(stations) {}
	    void operator()( const Model::Station::pair_t& m ) const;
	private:
	    std::ostream& _output;
	    const Model::Class::map_t& _classes;	/* For demand */
	    const Model::Station::map_t& _stations;	/* For visits */
	};

	struct printTransit {
	    printTransit( const std::string& name ) : _name(name) {}
	    std::string operator()( const std::string&, const Model::Station::pair_t& ) const;
	private:
	    const std::string& _name;
	};
	    

	struct printStationVariables {
	    printStationVariables( std::ostream& output, const Model::Class::map_t& classes ) : _output(output), _classes(classes) {}
	    void operator()( const Model::Station::pair_t& m ) const;
	private:
	    std::ostream& _output;
	    const Model::Class::map_t& _classes;
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
