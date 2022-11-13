/* -*- C++ -*-
 *  $Id: qnap2_document.h 16097 2022-11-12 20:01:52Z greg $
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
#include <lqx/MethodTable.h>

extern "C" {
#else
#include <stdbool.h>
#endif
    void qnap2error( const char * fmt, ... );
    void * qnap2_append_pointer( void *, void * );
    void * qnap2_append_string( void * list, const char * name );
    void * qnap2_define_variable( const char *, void *, void *, void * );
    void qnap2_declare_class( void * );
    void qnap2_declare_attribute( int, int, void * );
    void qnap2_declare_queue( void * );
    void qnap2_declare_objects();
    void qnap2_declare_reference( int, void * list );
    void qnap2_declare_variable( int, void * );
    void qnap2_define_station();
    void qnap2_map_transit_to_visits();
    const char * qnap2_get_class_name( const char * );		/* checks for class name */
    const char * qnap2_get_station_name( const char * );	/* checks for station name */
    void * qnap2_get_all_objects( int code );
    void * qnap2_get_transit_pair( const char *, void * );
    void * qnap2_get_service_distribution( int, void *, void * );
    void qnap2_set_option( const void * );
    void qnap2_set_station_init( const void *, void * );
    void qnap2_set_station_name( const char * );
    void qnap2_set_station_prio( const void *, const void * );
    void qnap2_set_program( void * );
    void qnap2_set_station_quantum( const void *, const void * );
    void qnap2_set_station_rate( void * );
    void qnap2_set_station_sched( const char * );
    void qnap2_set_station_service( const void *, const void * );
    void qnap2_set_station_transit( const void *, const void * );
    void qnap2_set_station_type( const void * );
    /* LQX */
    void * qnap2_get_attribute( void *, const char * );
    void * qnap2_get_function( const char * , void * );			/* Returns LQX */
    void * qnap2_get_integer( long );					/* Returns LQX */
    void * qnap2_get_procedure( const char *, void * );			/* Returns LQX */
    void * qnap2_get_real( double );					/* Returns LQX */
    void * qnap2_get_station_type_pair( int, int );
    void * qnap2_get_string( const char * );				/* Returns LQX */
    void * qnap2_get_variable( const char * ); 

    void * qnap2_assignment( void *, void * );
    void * qnap2_compound_statement( void * );
    void * qnap2_for_statement( void *, void *, void * ); 
    void * qnap2_foreach_statement( void *, void *, void * ); 
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
    class Program;
    class Environment;
}

namespace QNIO {
    class QNAP2_Document : public QNIO::Document {
	class SetStationService;
	typedef void (QNAP2_Document::*option_fptr)( bool );
	
	friend class SetStationService;
	friend const char * ::qnap2_get_class_name( const char * );		/* checks for class name */
	friend const char * ::qnap2_get_station_name( const char * );		/* checks for station name */
	friend void * ::qnap2_define_variable( const char * name, void * begin, void * end, void * init );
	friend void ::qnap2_declare_attribute( int, int, void * );
	friend void ::qnap2_declare_class( void * );
	friend void ::qnap2_declare_objects();
	friend void ::qnap2_declare_queue( void * );
	friend void ::qnap2_declare_reference( int, void * );
	friend void ::qnap2_declare_variable( int, void * );
	friend void ::qnap2_define_station();
	friend void ::qnap2_map_transit_to_visits();
	friend void ::qnap2_set_option( const void * );
	friend void ::qnap2_set_program( void * );
	friend void ::qnap2_set_station_init( const void *, void * );
	friend void ::qnap2_set_station_name( const char * );
	friend void ::qnap2_set_station_prio( const void *, const void * );
	friend void ::qnap2_set_station_quantum( const void *, const void * );
	friend void ::qnap2_set_station_sched( const char * );
	friend void ::qnap2_set_station_service( const void *, const void * );
	friend void ::qnap2_set_station_transit( const void *, const void * );
	friend void ::qnap2_set_station_type( const void * );
	friend void ::qnap2error( const char * fmt, ... );
	friend void * ::qnap2_get_all_objects( int code );
	friend void * ::qnap2_get_attribute( void *, const char * );
	friend void * ::qnap2_get_function( const char * , void * );
	friend void * ::qnap2_get_procedure( const char * symbol, void * );
	friend void * ::qnap2_get_service_distribution( int code, void *, void * );
	friend void * ::qnap2_get_variable( const char * ); 
	friend void * ::qnap2_list( void * initial_value, void * step, void * until );
	friend void * ::qnap2_for_statement( void * variable, void * arg2, void * loop_body );
	friend void * ::qnap2_foreach_statement( void * variable, void * list, void * loop_body );
	
	enum class Type { Undefined, Attribute, Boolean, Class, Integer, Queue, Real, Reference, String };
	enum class Distribution { Constant, Exponential, Erlang, Hyperexponential, Coxian };
	enum class Dimension { Scalar, Vector };

	class Symbol {
	public:
	    Symbol( const std::string& name, Type type=Type::Undefined, LQX::SyntaxTreeNode * begin=nullptr, LQX::SyntaxTreeNode * end=nullptr, LQX::SyntaxTreeNode * initial_value=nullptr ) :
		_name(name), _type(type), _begin(begin), _end(end), _initial_value(initial_value) {}
	    bool operator<( const Symbol& dst ) const;
	    const std::string& name() const { return _name; }
	    Type type() const { return _type; }
	    void setType( Type type ) { _type = type; }
	    LQX::SyntaxTreeNode * begin() const { return _begin; }
	    LQX::SyntaxTreeNode * end() const { return _end; }
	    LQX::SyntaxTreeNode * initial_value() const { return _initial_value; }
	    bool isScalar() const { return _begin == _end; }
	    bool isVector() const { return _begin != _end; }
	private:
	    const std::string _name;
	    Type _type;
	    LQX::SyntaxTreeNode * _begin;
	    LQX::SyntaxTreeNode * _end;
	    LQX::SyntaxTreeNode * _initial_value;
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
	
	virtual bool disableDefaultOutputWithLQX() const { return !_result; }
	virtual LQX::Program * getLQXProgram() { return _lqx; }
	LQX::Environment * getLQXEnvironment() const { return _env; }

	virtual std::vector<LQX::SyntaxTreeNode*> * gnuplot() { return nullptr; }

	virtual std::ostream& print( std::ostream& ) const;
	std::ostream& exportModel( std::ostream& ) const;

	const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }
	bool multiclass() const { return chains().size() > 1; }

    private:
	LQX::SyntaxTreeNode * getAllObjects( QNIO::QNAP2_Document::Type ) const;
	LQX::SyntaxTreeNode * getFunction( const std::string& name, std::vector<LQX::SyntaxTreeNode *>* args );
	LQX::VariableExpression * getVariable( const std::string& name );

	bool hasAttribute( LQX::VariableExpression * ) const;

	/* QNAP2 interface */
	bool declareClass( const Symbol& );
	bool declareAttribute( Type, Type, Symbol& );
	bool declareStation( const Symbol& );
	bool defineSymbol( Type, const Symbol&, bool=false );
	bool defineStation();
	bool mapTransitToVisits();
	bool mapTransitToVisits( const std::string&, const std::string&, LQX::SyntaxTreeNode *, std::deque<std::string>& );
	void setDebug( bool value ) { _debug = value; }
	void setResult( bool value ) { _result = value; }
	void setWarning( bool value ) {}
	bool setStationScheduling( const std::string& );
	bool setStationType( BCMP::Model::Station::Type, int );
	bool setStationTransit( const std::string&, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>& );
	void declareObjects();
	std::vector<LQX::SyntaxTreeNode *>* setProgram( LQX::SyntaxTreeNode * );
	void registerMethods();
	size_t countOutgoingTransits( const std::string& from, const std::string& chain ) const;
	size_t countIncommingTransits( const std::string& from, const std::string& chain ) const;
	const std::map<std::string,LQX::SyntaxTreeNode *>& getTransits( const std::string& from, const std::string& chain ) const;
	
    private:
	static BCMP::Model::Chain& getChain( const std::string& ); /* throws if not found */
	static BCMP::Model::Station& getStation( const std::string& );
	static LQX::SyntaxTreeNode * getInitialValue( QNAP2_Document::Type type, const Symbol& symbol );

	class SetOption {
	public:
	    SetOption( QNAP2_Document& document ) : _document(document) {}
	    void operator()( const std::string& ) const;
	private:
	    QNAP2_Document& _document;
	};

	class SetChainCustomers {
	public:
	    SetChainCustomers( const QNAP2_Document& document, LQX::SyntaxTreeNode* customers ) : _document(document), _customers(customers) {}
	    void operator()( const std::string& ) const;
	    void operator()( BCMP::Model::Chain::pair_t& ) const;
	private:
	    LQX::SyntaxTreeNode * getCustomers( const std::string& ) const;
	    bool isAttribute( LQX::VariableExpression* variable ) const { return _document.hasAttribute( variable ); }

	    const QNAP2_Document& _document;
	    LQX::SyntaxTreeNode * _customers;
	};
	
	class SetStationService {
	public:
	    SetStationService( const QNAP2_Document& document, const QNIO::QNAP2_Document::ServiceDistribution& service ) : _document(document), _service(service) {}
	    void operator()( const std::string& ) const;
	    void operator()( const BCMP::Model::Chain::pair_t& ) const;
	private:
	    LQX::SyntaxTreeNode * getServiceTime( const std::string& ) const;
	    bool isAttribute( LQX::VariableExpression* variable ) const { return _document.hasAttribute( variable ); }
	    
	    const QNAP2_Document& _document;
	    const QNAP2_Document::ServiceDistribution& _service;
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

	class Print : public LQX::Method {
	public:
	    /* maps print() to LQX::println(); */
	    Print() {}
	    virtual ~Print() {}
		
	    /* All of the glue code to make sure LQX can call print() */
	    virtual std::string getName() const { return "print"; } 
	    virtual const char* getParameterInfo() const { return "+"; } 
	    virtual std::string getHelp() const { return "Print the arguments."; } 
	    virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
	};

	class Output : public LQX::Method {
	public:
	    /* maps print() to LQX::println(); */
	    Output( BCMP::Model& model ) : _model(model) {}
	    virtual ~Output() {}
		
	    /* All of the glue code to make sure LQX can call output() */
	    virtual std::string getName() const { return "output"; } 
	    virtual const char* getParameterInfo() const { return ""; } 
	    virtual std::string getHelp() const { return "Print the arguments."; } 
	    virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);

	private:
	    const BCMP::Model& model() const { return _model; }
	    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }

	    std::ostream& print( std::ostream& output ) const;
	    std::ostream& print( std::ostream& output, const BCMP::Model::Station::Result& item ) const;

	    static std::string header();
	    static std::string blankline();

	private:
	    static std::streamsize __width;
	    static std::streamsize __precision;
	    static std::string __separator;

	    const BCMP::Model& _model;
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
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }		/* For demand */
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
	    
	struct getResultVariables {
	    getResultVariables( const BCMP::Model& model, std::set<std::string>& symbol_table ) : _model(model), _symbol_table(symbol_table) {}
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
	    std::set<std::string>& _symbol_table;
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

	void printResults( std::ostream& output, const std::string& prefix, const std::string& vars, const std::string& postfix  ) const;

	struct for_loop {
	    for_loop( std::ostream& output ) : _output(output) {}
	    void operator()( const Comprehension& ) const;
	private:
	    std::ostream& _output;
	};
	
	struct end_for {
	    end_for( std::ostream& output ) : _output(output) {}
	    void operator()( const Comprehension& ) const;
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

	/* -------------------------------------------------------------------- */
	/* */
	/* -------------------------------------------------------------------- */
    private:
	std::map<const std::string,const Type> _attributes;			/* Object, Scalar */
	std::map<std::string,std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>> _transit; /* from, chain, to, value under construction */
	std::vector<LQX::SyntaxTreeNode *> _program;
	LQX::Program * _lqx;
	LQX::Environment * _env;
	bool _debug;
	bool _result;										/* suppress default output.	*/

    public:
	static const std::map<const scheduling_type,const std::string> __scheduling_str;	/* Maps scheduling_type to qnap2 keyword */
	static const std::map<const std::string,const scheduling_type> __scheduling_type;
	static const std::map<const int,const BCMP::Model::Station::Type> __station_type;
	static const std::map<int,const QNIO::QNAP2_Document::Type> __parser_to_type;
	static const std::map<const std::string,std::pair<option_fptr,bool>> __option;
	
    private:
	static QNAP2_Document * __document;
	static std::pair<std::string,BCMP::Model::Station> __station;				/* Station under construction */
	static std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>> __transit; 	/* chain, to-station, value under construction */
    };

    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document& doc ) { return doc.print(output); }
}
#endif
#endif
