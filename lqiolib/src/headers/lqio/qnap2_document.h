/* -*- C++ -*-
 *  $Id: qnap2_document.h 16036 2022-10-26 10:57:40Z greg $
 *
 *  Created by Greg Franks 2020/12/28
 */

#ifndef __LQIO_QNAP2_DOCUMENT__
#define __LQIO_QNAP2_DOCUMENT__

#if defined(__cplusplus)
#include <deque>
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
    void * qnap2_append_pointer( void *, void * );
    void * qnap2_append_string( void * list, const char * name );
    void * qnap2_declare_variable( const char *, void *, void * );
    void qnap2_declare_class( void * );
    void qnap2_declare_field( int, void * );
    void qnap2_declare_queue( void * );
    void qnap2_declare_objects();
    void qnap2_define_station();
    void qnap2_define_variable( int, void * );
    void qnap2_map_transit_to_visits();
    const char * qnap2_get_class_name( const char * );		/* checks for class name */
    const char * qnap2_get_station_name( const char * );	/* checks for station name */
    void * qnap2_get_transit_pair( const char *, void * );
    void * qnap2_get_service_distribution( int, void *, void * );
    void qnap2_set_option( const char * );
    void qnap2_set_station_init( const void *, void * );
    void qnap2_set_station_name( const char * );
    void qnap2_set_station_prio( const void *, const void * );
    void qnap2_set_program( void * );
    void qnap2_set_station_quantum( const void *, const void * );
    void qnap2_set_station_rate( void * );
    void qnap2_set_station_sched( const char * );
    void qnap2_set_station_service( const void *, const void * );
    void qnap2_set_station_transit( const char *, const void * );
    void qnap2_set_station_type( int, int );
    /* LQX */
    void * qnap2_get_integer( long );					/* Returns LQX */
    void * qnap2_get_real( double );					/* Returns LQX */
    void * qnap2_get_string( const char * );				/* Returns LQX */
    void * qnap2_get_variable( const char * );				/* Returns LQX */
    void * qnap2_get_vector_lvalue( const char *, const char * );       /* Returns LQX */
    void * qnap2_get_object_lvalue( const char *, const char * );       /* Returns LQX */ 
    void * qnap2_get_scalar_lvalue( const char * ); 		        /* Returns LQX */
    void * qnap2_get_function( const char * , void * );			/* Returns LQX */

    void * qnap2_assignment( void *, void * );
    void * qnap2_compound_statement( void * );
    void * qnap2_for_statement( void *, void *, void * );
    void * qnap2_if_statement( void *, void *, void * );
    void * qnap2_list( void * initial_value, void * step, void * until );
    void * qnap2_logic( int operation, void * arg1, void * arg2 );
    void * qnap2_math( int operation, void * arg1, void * arg2 );
    void * qnap2_relation( int operation, void * arg1, void * arg2 );
    void * qnap2_while_statement( void *, void * );

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
	friend const char * ::qnap2_get_class_name( const char * );		/* checks for class name */
	friend const char * ::qnap2_get_station_name( const char * );		/* checks for station name */
	friend void * ::qnap2_declare_variable( const char * name, void * begin, void * end );
	friend void ::qnap2_declare_class( void * );
	friend void ::qnap2_declare_field( int, void * );
	friend void ::qnap2_declare_queue( void * );
	friend void ::qnap2_declare_objects();
	friend void ::qnap2_define_station();
	friend void ::qnap2_define_variable( int, void * );
	friend void ::qnap2_map_transit_to_visits();
	friend void ::qnap2_set_option( const char * );
	friend void ::qnap2_set_program( void * );
	friend void ::qnap2_set_station_init( const void *, void * );
	friend void ::qnap2_set_station_name( const char * );
	friend void ::qnap2_set_station_prio( const void *, const void * );
	friend void ::qnap2_set_station_quantum( const void *, const void * );
	friend void ::qnap2_set_station_sched( const char * );
	friend void ::qnap2_set_station_service( const void *, const void * );
	friend void ::qnap2_set_station_transit( const char *, const void * );
	friend void ::qnap2_set_station_type( int, int );
	friend void ::qnap2error( const char * fmt, ... );
	friend void * ::qnap2_get_variable( const char * variable );
	friend void * ::qnap2_get_service_distribution( int code, void *, void * );
	friend void * ::qnap2_get_vector_lvalue( const char *, const char * ); 
	friend void * ::qnap2_get_object_lvalue( const char *, const char * ); 
	friend void * ::qnap2_get_scalar_lvalue( const char * ); 
	friend void * ::qnap2_list( void * initial_value, void * step, void * until );
	friend void * ::qnap2_for_statement( void * variable, void * arg2, void * loop_body );
	
	enum class Type { Undefined, Boolean, Class, Integer, Queue, Real, String };
	enum class Distribution { Constant, Exponential, Erlang, Hyperexponential, Coxian };
	enum class Dimension { Scalar, Vector };

	class Variable {
	public:
	    Variable( const std::string& name, Type type=Type::Undefined, LQX::SyntaxTreeNode * begin=nullptr, LQX::SyntaxTreeNode * end=nullptr ) : _name(name), _type(type), _begin(begin), _end(end) {}
	    bool operator<( const Variable& dst ) const { return name() < dst.name(); }
	    const std::string& name() const { return _name; }
	    Type type() const { return _type; }
	    void setType( Type type ) { _type = type; }
	    LQX::SyntaxTreeNode * begin() const { return _begin; }
	    LQX::SyntaxTreeNode * end() const { return _end; }
	    bool isScalar() const { return _begin == _end; }
	    bool isVector() const { return _begin != _end;  }
	private:
	    const std::string _name;
	    Type _type;
	    LQX::SyntaxTreeNode * _begin;
	    LQX::SyntaxTreeNode * _end;
	};

	class List {
	public:
	    List( LQX::SyntaxTreeNode * initial_value, LQX::SyntaxTreeNode * step, LQX::SyntaxTreeNode * until ) : _initial_value(initial_value), _step(step), _until(until) {}
	    LQX::SyntaxTreeNode * initial_value() const { return _initial_value; }
	    LQX::SyntaxTreeNode * step() const { return _step; }
	    LQX::SyntaxTreeNode * until() const { return _until; }
	private:
	    LQX::SyntaxTreeNode * _initial_value;
	    LQX::SyntaxTreeNode * _step;
	    LQX::SyntaxTreeNode * _until;
	};

	class ServiceDistribution {
	public:
	    ServiceDistribution( Distribution distribution, LQX::SyntaxTreeNode * mean, LQX::SyntaxTreeNode * shape=nullptr ) : _distribution(distribution), _mean(mean), _shape(shape) {}
	    const Distribution distribution() const { return _distribution; }
	    LQX::SyntaxTreeNode * mean() const { return _mean; }
	    LQX::SyntaxTreeNode * shape() const { return _shape; }

	private:
	    const Distribution _distribution;
	    LQX::SyntaxTreeNode * _mean;
	    LQX::SyntaxTreeNode * _shape;
	};
	
    public:
	QNAP2_Document( const std::string& input_file_name );					/* For input */
	QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model );		/* For output */
	virtual ~QNAP2_Document();

	virtual bool load();
	
	virtual bool disableDefaultOutputWithLQX() const { return _nresult; }

	virtual std::vector<LQX::SyntaxTreeNode*> * gnuplot() { return nullptr; }

	virtual std::ostream& print( std::ostream& ) const;
	std::ostream& printInput( std::ostream& ) const;

	const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }
	bool multiclass() const { return chains().size() > 1; }

	/* QNAP2 interface */
    private:
	bool declareClass( const Variable& );
	bool declareField( Type type, const Variable& );
	bool declareStation( const Variable& );
	bool defineSymbol( Type type, const Variable& );
	bool defineStation();
	bool findVariable( const std::string& ) const;
	bool mapTransitToVisits();
	bool mapTransitToVisits( const std::string&, const std::string&, LQX::SyntaxTreeNode *, std::deque<std::string>& );
	void setNResult( bool value ) { _nresult = value; }
	bool setStationScheduling( const std::string& );
	bool setStationType( BCMP::Model::Station::Type, int );
	bool setStationTransit( const std::string&, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>& );
	void declareObjects();
	LQX::SyntaxTreeNode * getLValue( const std::string& );
	std::vector<LQX::SyntaxTreeNode *>* setProgram( LQX::SyntaxTreeNode * );
	
    private:
	static BCMP::Model::Chain& getChain( const std::string& ); /* throws if not found */
	static BCMP::Model::Station& getStation( const std::string& ); 
	static LQX::SyntaxTreeNode * getProduct( LQX::SyntaxTreeNode * multiplier, LQX::SyntaxTreeNode * multiplicand );

	class SetClassInit {
	public:
	    SetClassInit( LQX::SyntaxTreeNode* customers ) : _customers(customers) {}
	    void operator()( const std::string& ) const;
	    void operator()( BCMP::Model::Chain::pair_t& ) const;
	private:
	    LQX::SyntaxTreeNode * _customers;
	};
	
	class SetStationService {
	public:
	    SetStationService( const QNIO::QNAP2_Document::ServiceDistribution& service ) : _service(service) {}
	    void operator()( const std::string& ) const;
	    void operator()( const BCMP::Model::Chain::pair_t& ) const;
	private:
	    const QNIO::QNAP2_Document::ServiceDistribution& _service;
	};
	
	class SetStationTransit {
	public:
	    SetStationTransit( const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>& transit ) : _transit(transit) {}
	    void operator()( const std::string& ) const;
	    void operator()( const BCMP::Model::Chain::pair_t& ) const;
	private:
	    void set( const std::string& class_name ) const;
	    
	    const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>& _transit;
	};

	/* -------------------------------------------------------------------- */
	/*                              output                                  */
	/* -------------------------------------------------------------------- */
    private:
	void printClassVariables( std::ostream& ) const;

	static std::ostream& printKeyword( std::ostream&, const std::string& s1, const std::string& s2 );
	static std::ostream& printStatement( std::ostream&, const std::string& s1, const std::string& s2 );
	static XML::StringManip qnap2_statement( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printStatement, s1, s2 ); }
	static XML::StringManip qnap2_keyword( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2_Document::printKeyword, s1, s2 ); }
	static std::string to_real( LQX::SyntaxTreeNode* v );
	static std::string to_unsigned( LQX::SyntaxTreeNode* v );

    private:
	struct getIntegerVariables {
	    getIntegerVariables( const BCMP::Model& model, std::set<const LQX::VariableExpression *>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
	    std::string operator()( const std::string&, const BCMP::Model::Station::pair_t& m ) const;
	    std::string operator()( const std::string&, const BCMP::Model::Chain::pair_t& k ) const;
	private:
	    const BCMP::Model& model() const { return _model; }
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }	/* For demand */
	    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }	/* For visits */
	    bool multiclass() const { return chains().size() > 1; }
	private:
	    const BCMP::Model& _model;
	    std::set<const LQX::VariableExpression *>& _symbol_table;
	};
	    
	struct getRealVariables {
	    getRealVariables( const BCMP::Model& model, std::set<const LQX::VariableExpression *>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
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
	    std::set<const LQX::VariableExpression *>& _symbol_table;
	};
	    
	static std::string getDeferredVariables( const std::string&, const std::pair<const LQIO::DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& );

	struct getResultVariables {
	    getResultVariables( const std::set<const LQIO::DOM::ExternalVariable *>& symbol_table );
	    std::string operator()( const std::string&, const std::pair<const std::string,LQX::SyntaxTreeNode *>& ) const;
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
	    LQX::SyntaxTreeNode * operator()( const LQX::SyntaxTreeNode*, const std::pair<std::string,LQX::SyntaxTreeNode *>& t2 ) const;
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
	    void operator()( const std::pair<const std::string,LQX::SyntaxTreeNode *>& ) const;
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

	void printResultsHeader( std::ostream& output, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>& vars ) const;
	void printResults( std::ostream& output, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>& ) const;

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

	    std::ostream& print( std::ostream& output, const BCMP::Model::Station::Result& item ) const;

	    const BCMP::Model& _model;
	};

	friend std::ostream& operator<<( std::ostream& output, const QNAP2_Document::Output& doc );

	/* -------------------------------------------------------------------- */
	/*                              output                                  */
	/* -------------------------------------------------------------------- */
    private:
	std::map<const Variable,LQX::SyntaxTreeNode *> _symbol_table;	/* All symbols defined in Declare */
	std::map<const std::string,Type> _field_table;			/* Fields for classes.	*/
	std::map<std::string,std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>> _transit; /* from, chain, to, value under construction */

	std::vector<LQX::SyntaxTreeNode *> _program;
	bool _nresult;							/* suppress default output.	*/

    public:
	static const std::map<const scheduling_type,const std::string> __scheduling_str;	/* Maps scheduling_type to qnap2 keyword */
	static const std::map<const std::string,const scheduling_type> __scheduling_type;
	static const std::map<const int,const BCMP::Model::Station::Type> __station_type;
	static const std::map<int,const QNIO::QNAP2_Document::Type> __parser_to_type;
	
    private:
	static QNAP2_Document * __document;
	static std::pair<std::string,BCMP::Model::Station> __station;	/* Station under construction */
	static std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>> __transit; /* chain, to-station, value under construction */
    };

    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document& doc ) { return doc.print(output); }
    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document::Output& doc ) { return doc.print(output); }
}
#endif
#endif
