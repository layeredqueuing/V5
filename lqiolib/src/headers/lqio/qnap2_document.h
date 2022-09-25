/* -*- C++ -*-
 *  $Id: qnap2_document.h 15907 2022-09-25 13:18:18Z greg $
 *
 *  Created by Greg Franks 2020/12/28
 */

#ifndef __LQIO_QNAP2_DOCUMENT__
#define __LQIO_QNAP2_DOCUMENT__

#if defined(__cplusplus)
#include <iostream>
#include <map>
#include <set>
#include <string>
#include "bcmp_document.h"
#include "srvn_spex.h"
#include "xml_output.h"

extern "C" {
#else
#include <stdbool.h>
#endif
    void qnap2error( const char * fmt, ... );
    void qnap2_default_class();
    void * qnap2_append_identifier( void *, void * );
    void qnap2_add_queue( void * );
    void qnap2_add_class( void * );
    void qnap2_add_field( int, void * );
    void qnap2_add_variable( int, void * );
    void qnap2_define_station();
    char * qnap2_get_station_type( const void *, const void * );
    void * qnap2_get_integer( long );			/* Returns LQX */
    void * qnap2_get_real( double );			/* Returns LQX */
    void * qnap2_get_string( const char * );		/* Returns LQX */
    void * qnap2_get_variable( const char * );		/* Returns LQX */
    void qnap2_set_station_init( const void * );
    void qnap2_set_station_name( const void * );
    void qnap2_set_station_prio( void * );
    void qnap2_set_station_quantum( void * );
    void qnap2_set_station_rate( void * );
    void qnap2_set_station_sched( const void * );
    void qnap2_set_station_service( void * );
    void qnap2_set_station_transit( const char *, void * );
    void qnap2_set_station_type( const void *, int );

    extern int qnap2lineno;

#if defined(__cplusplus)
}

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
	friend void ::qnap2error( const char * fmt, ... );
	friend void ::qnap2_default_class();
	friend void ::qnap2_add_class( void * );
	friend void ::qnap2_add_queue( void * );
	friend void ::qnap2_add_field( int, void * );
	friend void ::qnap2_add_variable( int, void * );
	friend void ::qnap2_define_station();
	friend void ::qnap2_set_station_init( const void * );
	friend void ::qnap2_set_station_name( const void * );
	friend void ::qnap2_set_station_sched( const void * );
	friend void ::qnap2_set_station_type( const void *, int );
	friend void ::qnap2_set_station_transit( const char *, void * );
	friend void * ::qnap2_get_variable( const char * variable );

	enum class Type { boolean, integer, real, string, boolean_field, integer_field, real_field, string_field, function };
	
    public:
	QNAP2_Document( const std::string& input_file_name );					/* For input */
	QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model );		/* For output */
	virtual ~QNAP2_Document();

	bool load();
	const char * getInputFileName() const { return _input_file_name.c_str(); }
	
	std::ostream& print( std::ostream& ) const;

	const Model::Model::Station::map_t& stations() const { return _model.stations(); }
	const Model::Chain::map_t& chains() const { return _model.chains(); }
	const Model& model() const { return _model; }
	bool multiclass() const { return chains().size() > 1; }

	/* QNAP2 interface */
    private:
	bool declareStation( const std::string& );
	bool declareClass( const std::string& );
	bool declareSymbol( Type type, const std::string& );
	void defaultClass();
	bool defineStation();
	bool findVariable( const std::string& ) const;
	bool setClassInit( const LQIO::DOM::ExternalVariable * );
	bool setStationName( const std::string& ); 
	bool setStationScheduling( const std::string& );
	bool setStationType( const std::string&, int );

    private:
	void printClassVariables( std::ostream& ) const;

	static std::ostream& printKeyword( std::ostream&, const std::string& s1, const std::string& s2 );
	static std::ostream& printStatement( std::ostream&, const std::string& s1, const std::string& s2 );
	static XML::StringManip qnap2_statement( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printStatement, s1, s2 ); }
	static XML::StringManip qnap2_keyword( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printKeyword, s1, s2 ); }
	static std::string to_real( const LQIO::DOM::ExternalVariable* v );
	static std::string to_unsigned( const LQIO::DOM::ExternalVariable* v );

    private:
	struct getIntegerVariables {
	    getIntegerVariables( const Model& model, std::set<const LQIO::DOM::ExternalVariable *>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
	    std::string operator()( const std::string&, const Model::Station::pair_t& m ) const;
	    std::string operator()( const std::string&, const Model::Chain::pair_t& k ) const;
	private:
	    const Model& model() const { return _model; }
	    const Model::Chain::map_t& chains() const { return _model.chains(); }	/* For demand */
	    const Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    const Model& _model;
	    std::set<const LQIO::DOM::ExternalVariable *>& _symbol_table;
	};
	    
	struct getRealVariables {
	    getRealVariables( const Model& model, std::set<const LQIO::DOM::ExternalVariable *>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
	    std::string operator()( const std::string&, const Model::Chain::pair_t& k ) const;
	    std::string operator()( const std::string&, const Model::Station::pair_t& m ) const;
	    std::string operator()( const std::string&, const Model::Station::Class::pair_t& d ) const;
	private:
	    const Model& model() const { return _model; }
	    const Model::Chain::map_t& chains() const { return _model.chains(); }	/* For demand */
	    const Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    const Model& _model;
	    std::set<const LQIO::DOM::ExternalVariable *>& _symbol_table;
	};
	    
	static std::string getDeferredVariables( const std::string&, const std::pair<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& );

	struct getResultVariables {
	    getResultVariables( const std::set<const LQIO::DOM::ExternalVariable *>& symbol_table );
	    std::string operator()( const std::string&, const Spex::var_name_and_expr& ) const;
	private:
	    std::set<std::string> _symbol_table;	/* Converted from arg. */
	};
	
	struct printStation {
	    printStation( std::ostream& output, const Model& model ) : _output(output), _model(model) {}
	    void operator()( const Model::Station::pair_t& m ) const;
	private:
	    const Model::Chain::map_t& chains() const { return _model.chains(); }	/* For demand */
	    const Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	    bool multiclass() const { return chains().size() > 1; }
	    void printCustomerTransit() const;
	    void printServerTransit( const Model::Station::pair_t& m ) const;
	    void printInterarrivalTime() const;
	private:
	    std::ostream& _output;
	    const Model& _model;
	};

	struct fold_transit {
	    fold_transit( const std::string& name ) : _name(name) {}
	    std::string operator()( const std::string&, const Model::Station::pair_t& ) const;
	private:
	    const std::string& _name;
	};
    
	struct printStationVariables {
	    printStationVariables( std::ostream& output, const Model& model ) : _output(output), _model(model) {}
	    void operator()( const Model::Station::pair_t& m ) const;
	private:
	    const Model::Chain::map_t& chains() const { return _model.chains(); }
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    std::ostream& _output;
	    const Model& _model;
	};

	struct printSPEXScalars {
	    printSPEXScalars( std::ostream& output ) : _output(output) {}
	    void operator()( const std::string& ) const;
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
	    typedef std::pair<std::string,std::string> (getObservations::*f)( const std::string&, const std::string& ) const;
	    
	    getObservations( std::ostream& output, const Model& model ) : _output(output), _model(model) {}
	    void operator()( const Spex::var_name_and_expr& ) const;
	    std::pair<std::string,std::string> get_throughput( const std::string&, const std::string& ) const;
	    std::pair<std::string,std::string> get_utilization( const std::string&, const std::string& ) const;
	    std::pair<std::string,std::string> get_service_time( const std::string&, const std::string& ) const;
	    std::pair<std::string,std::string> get_waiting_time( const std::string&, const std::string& ) const;
	private:
	    const Model::Chain::map_t& chains() const { return _model.chains(); }
	    const Model::Model::Station::map_t& stations() const { return _model.stations(); }
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    std::ostream& _output;
	    const Model& _model;
	};

	void printResultsHeader( std::ostream& output, const std::vector<Spex::var_name_and_expr>& vars ) const;
	void printResults( std::ostream& output, const std::vector<Spex::var_name_and_expr>& ) const;

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
	
	struct fold_station {
	    fold_station( const std::string& suffix="" ) : _suffix(suffix) {}
	    std::string operator()( const std::string& s1, const Model::Station::pair_t& m ) const;
	private:
	    const std::string& _suffix;
	};

	struct fold_class {
	    fold_class( const Model::Chain::map_t& chains, Model::Chain::Type type ) : _chains(chains), _type(type) {}
	    std::string operator()( const std::string& s1, const Model::Station::Class::pair_t& k2 ) const;
	private:
	    const Model::Chain::map_t& _chains;
	    const Model::Chain::Type _type;
	};
	    
	struct fold_visits {
	    fold_visits( const std::string& name ) : _name(name) {}
	    std::string operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const;
	private:
	    const std::string& _name;
	};

	struct fold_mresponse {
	    fold_mresponse( const std::string& name, const Model::Chain::map_t& chains ) : _name(name), _chains(chains) {}
	    std::string operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const;
	private:
	    bool multiclass() const { return _chains.size() > 1; }
	private:
	    const std::string& _name;
	    const Model::Chain::map_t& _chains;
	};

    private:
	const std::string _input_file_name;
	std::map<const std::string,Type> _symbol_table;
	BCMP::Model _model;

    public:
	static std::map<const scheduling_type,const std::string> __scheduling_str;	/* Maps scheduling_type to qnap2 keyword */
	static std::map<const std::string,const scheduling_type> __scheduling_type;
	static std::map<const std::string,const BCMP::Model::Station::Type> __station_type;
	
    private:
	static QNAP2_Document * __document;
	static std::pair<std::string,BCMP::Model::Station> __station;	/* Station under construction */
    };

    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document& doc ) { return doc.print(output); }
}
#endif
#endif
