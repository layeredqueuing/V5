/* -*- C++ -*-
 *  $Id: qnap2_document.h 15930 2022-09-30 00:22:54Z greg $
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
#include "qnio_document.h"
#include "xml_output.h"

extern "C" {
#else
#include <stdbool.h>
#endif
    void qnap2error( const char * fmt, ... );
    void qnap2_default_class();
    void qnap2_define_station();
    void qnap2_map_transit_to_visits();
    void * qnap2_append_identifier( void *, const char * );
    void * qnap2_append_node( void *, void * );
    void qnap2_add_queue( void * );
    void qnap2_add_class( void * );
    void qnap2_add_field( int, void * );
    void qnap2_add_variable( int, void * );
    void qnap2_declare_scalar( const char * );
    void qnap2_declare_vector( const char *, const void * );
    void * qnap2_get_integer( long );				/* Returns LQX */
    void * qnap2_get_real( double );				/* Returns LQX */
    void * qnap2_get_string( const char * );			/* Returns LQX */
    void * qnap2_get_variable( const char * );			/* Returns LQX */
    void * qnap2_get_function( const char * , void * );		/* Returns LQX */
    void * qnap2_get_transit_pair( const char *, void * );
    void * qnap2_get_service_distribution( int, const void *, const void * );
    void qnap2_set_station_init( const void *, const void * );
    void qnap2_set_station_name( const void * );
    void qnap2_set_station_prio( const void *, const void * );
    void qnap2_set_program( void * );
    void qnap2_set_station_quantum( const void *, const void * );
    void qnap2_set_station_rate( void * );
    void qnap2_set_station_sched( const void * );
    void qnap2_set_station_service( const void *, const void * );
    void qnap2_set_station_transit( const void *, const void * );
    void qnap2_set_station_type( int, int );
    void * qnap2_assignment( void *, void * );
    void * qnap2_add( void *, void * );
    void * qnap2_divide( void *, void * );
    void * qnap2_modulus( void *, void * );
    void * qnap2_multiply( void *, void * );
    void * qnap2_negate( void * );
    void * qnap2_power( void *, void * );
    void * qnap2_subtract( void *, void * );


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

namespace QNIO {
    class QNAP2_Document : public QNIO::Document {
	friend void ::qnap2error( const char * fmt, ... );
	friend void ::qnap2_default_class();
	friend void ::qnap2_define_station();
	friend void ::qnap2_map_transit_to_visits();
	friend void ::qnap2_add_class( void * );
	friend void ::qnap2_add_queue( void * );
	friend void ::qnap2_add_field( int, void * );
	friend void ::qnap2_add_variable( int, void * );
	friend void ::qnap2_declare_scalar( const char * );
	friend void ::qnap2_declare_vector( const char *, const void * );
	friend void ::qnap2_set_station_init( const void *, const void * );
	friend void ::qnap2_set_station_name( const void * );
	friend void ::qnap2_set_station_prio( const void *, const void * );
	friend void ::qnap2_set_station_quantum( const void *, const void * );
	friend void ::qnap2_set_station_sched( const void * );
	friend void ::qnap2_set_station_service( const void *, const void * );
	friend void ::qnap2_set_station_transit( const void *, const void * );
	friend void ::qnap2_set_station_type( int, int );
	friend void * ::qnap2_get_variable( const char * variable );
	friend void * ::qnap2_get_service_distribution( int code, const void *, const void * );
	
	enum class Type { undefined, boolean, integer, real, string, boolean_field, integer_field, real_field, string_field, function };
	enum class Distribution { constant, exponential, erlang, hyperexponential, coxian };
	enum class Structure { scalar, vector };

	class ServiceDistribution {
	public:
	    ServiceDistribution( Distribution distribution, const LQX::SyntaxTreeNode * mean, const LQX::SyntaxTreeNode * shape=nullptr ) : _distribution(distribution), _mean(mean), _shape(shape) {}
	    const Distribution distribution() const { return _distribution; }
	    const LQX::SyntaxTreeNode * mean() const { return _mean; }
	    const LQX::SyntaxTreeNode * shape() const { return _shape; }

	private:
	    const Distribution _distribution;
	    const LQX::SyntaxTreeNode * _mean;
	    const LQX::SyntaxTreeNode * _shape;
	};
	
    public:
	QNAP2_Document( const std::string& input_file_name );					/* For input */
	QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model );		/* For output */
	virtual ~QNAP2_Document();

	virtual bool load();
	
	virtual expr_list * getSPEXProgram() const { return nullptr; }
	virtual expr_list * getGNUPlotProgram() { return nullptr; }

	virtual std::ostream& print( std::ostream& ) const;
	std::ostream& printInput( std::ostream& ) const;

	const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }
	bool multiclass() const { return chains().size() > 1; }

	/* QNAP2 interface */
    private:
	bool declareClass( const std::string& );
	bool declareStation( const std::string& );
	bool declareSymbol( Type type, const std::string& );
	bool defineStation();
	bool findVariable( const std::string& ) const;
	bool mapTransitToVisits();
	bool setClassInit( const std::string&, const LQX::SyntaxTreeNode * );
	bool setStationName( const std::string& ); 
	bool setStationScheduling( const std::string& );
	bool setStationService( const std::string&, const QNAP2_Document::ServiceDistribution * );	/* arg2 May have to be identifier list */
	bool setStationType( BCMP::Model::Station::Type, int );
	bool setStationTransit( const std::string&, const std::vector<const std::string,LQX::SyntaxTreeNode *>& );
	void defaultClass();

    private:
	static LQIO::DOM::ExternalVariable * getExternalVariable( const LQX::SyntaxTreeNode * );
	const std::string& getClassName( const std::string& ) const;
	
	void printClassVariables( std::ostream& ) const;

	static std::ostream& printKeyword( std::ostream&, const std::string& s1, const std::string& s2 );
	static std::ostream& printStatement( std::ostream&, const std::string& s1, const std::string& s2 );
	static XML::StringManip qnap2_statement( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printStatement, s1, s2 ); }
	static XML::StringManip qnap2_keyword( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printKeyword, s1, s2 ); }
	static std::string to_real( const LQIO::DOM::ExternalVariable* v );
	static std::string to_unsigned( const LQIO::DOM::ExternalVariable* v );

    private:
	struct getIntegerVariables {
	    getIntegerVariables( const BCMP::Model& model, std::set<const LQIO::DOM::ExternalVariable *>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
	    std::string operator()( const std::string&, const BCMP::Model::Station::pair_t& m ) const;
	    std::string operator()( const std::string&, const BCMP::Model::Chain::pair_t& k ) const;
	private:
	    const BCMP::Model& model() const { return _model; }
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }	/* For demand */
	    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    const BCMP::Model& _model;
	    std::set<const LQIO::DOM::ExternalVariable *>& _symbol_table;
	};
	    
	struct getRealVariables {
	    getRealVariables( const BCMP::Model& model, std::set<const LQIO::DOM::ExternalVariable *>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
	    std::string operator()( const std::string&, const BCMP::Model::Chain::pair_t& k ) const;
	    std::string operator()( const std::string&, const BCMP::Model::Station::pair_t& m ) const;
	    std::string operator()( const std::string&, const BCMP::Model::Station::Class::pair_t& d ) const;
	private:
	    const BCMP::Model& model() const { return _model; }
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }	/* For demand */
	    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    const BCMP::Model& _model;
	    std::set<const LQIO::DOM::ExternalVariable *>& _symbol_table;
	};
	    
	static std::string getDeferredVariables( const std::string&, const std::pair<const LQIO::DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& );

	struct getResultVariables {
	    getResultVariables( const std::set<const LQIO::DOM::ExternalVariable *>& symbol_table );
	    std::string operator()( const std::string&, const LQIO::Spex::var_name_and_expr& ) const;
	private:
	    std::set<std::string> _symbol_table;	/* Converted from arg. */
	};
	
	struct printStation {
	    printStation( std::ostream& output, const BCMP::Model& model ) : _output(output), _model(model) {}
	    void operator()( const BCMP::Model::Station::pair_t& m ) const;
	private:
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }	/* For demand */
	    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	    bool multiclass() const { return chains().size() > 1; }
	    void printCustomerTransit() const;
	    void printServerTransit( const BCMP::Model::Station::pair_t& m ) const;
	    void printInterarrivalTime() const;
	private:
	    std::ostream& _output;
	    const BCMP::Model& _model;
	};

	struct fold_transit {
	    fold_transit( const std::string& name ) : _name(name) {}
	    std::string operator()( const std::string&, const BCMP::Model::Station::pair_t& ) const;
	private:
	    const std::string& _name;
	};
    
	struct printStationVariables {
	    printStationVariables( std::ostream& output, const BCMP::Model& model ) : _output(output), _model(model) {}
	    void operator()( const BCMP::Model::Station::pair_t& m ) const;
	private:
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    std::ostream& _output;
	    const BCMP::Model& _model;
	};

	struct printSPEXScalars {
	    printSPEXScalars( std::ostream& output ) : _output(output) {}
	    void operator()( const std::string& ) const;
	private:
	    std::ostream& _output;
	};
	
	struct printSPEXDeferred {
	    printSPEXDeferred( std::ostream& output ) : _output(output) {}
	    void operator()( const std::pair<const LQIO::DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& ) const;
	private:
	    std::ostream& _output;
	};
	
	class getObservations {
	public:
	    typedef std::pair<std::string,std::string> (getObservations::*f)( const std::string&, const std::string& ) const;
	    
	    getObservations( std::ostream& output, const BCMP::Model& model ) : _output(output), _model(model) {}
	    void operator()( const LQIO::Spex::var_name_and_expr& ) const;
	    std::pair<std::string,std::string> get_throughput( const std::string&, const std::string& ) const;
	    std::pair<std::string,std::string> get_utilization( const std::string&, const std::string& ) const;
	    std::pair<std::string,std::string> get_service_time( const std::string&, const std::string& ) const;
	    std::pair<std::string,std::string> get_waiting_time( const std::string&, const std::string& ) const;
	private:
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }
	    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    std::ostream& _output;
	    const BCMP::Model& _model;
	};

	void printResultsHeader( std::ostream& output, const std::vector<LQIO::Spex::var_name_and_expr>& vars ) const;
	void printResults( std::ostream& output, const std::vector<LQIO::Spex::var_name_and_expr>& ) const;

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
	    std::string operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m ) const;
	private:
	    const std::string& _suffix;
	};

	struct fold_class {
	    fold_class( const BCMP::Model::Chain::map_t& chains, BCMP::Model::Chain::Type type ) : _chains(chains), _type(type) {}
	    std::string operator()( const std::string& s1, const BCMP::Model::Station::Class::pair_t& k2 ) const;
	private:
	    const BCMP::Model::Chain::map_t& _chains;
	    const BCMP::Model::Chain::Type _type;
	};
	    
	struct fold_visits {
	    fold_visits( const std::string& name ) : _name(name) {}
	    std::string operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m2 ) const;
	private:
	    const std::string& _name;
	};

	struct fold_mresponse {
	    fold_mresponse( const std::string& name, const BCMP::Model::Chain::map_t& chains ) : _name(name), _chains(chains) {}
	    std::string operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m2 ) const;
	private:
	    bool multiclass() const { return _chains.size() > 1; }
	private:
	    const std::string& _name;
	    const BCMP::Model::Chain::map_t& _chains;
	};

	class Output {
	    
	public:
	    Output( const BCMP::Model& model ) : _model(model) {}
	    
	    std::ostream& print( std::ostream& ) const;

	private:
	    static std::streamsize __width;
	    static std::streamsize __precision;
	    static std::string __separator;

	    static std::string header();
	    static std::string blankline();
	    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }

	    std::ostream& print( std::ostream& output, double service_time, const BCMP::Model::Station::Result& item ) const;

	    const BCMP::Model& _model;
	};

	friend std::ostream& operator<<( std::ostream& output, const QNAP2_Document::Output& doc );

    private:
	std::map<const std::string,Type> _symbol_table;
	std::map<const std::string,int> _transit;

    public:
	static const std::map<const scheduling_type,const std::string> __scheduling_str;	/* Maps scheduling_type to qnap2 keyword */
	static const std::map<const std::string,const scheduling_type> __scheduling_type;
	static const std::map<const int,const BCMP::Model::Station::Type> __station_type;
	
    private:
	static QNAP2_Document * __document;
	static std::pair<std::string,BCMP::Model::Station> __station;	/* Station under construction */
    };

    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document& doc ) { return doc.print(output); }
    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document::Output& doc ) { return doc.print(output); }
}
#endif
#endif
