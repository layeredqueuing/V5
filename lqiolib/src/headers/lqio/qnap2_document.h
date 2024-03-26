/* -*- C++ -*-
 *  $Id: qnap2_document.h 17142 2024-03-22 19:47:05Z greg $
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
    void * qnap2_comprehension( const void * );
    void * qnap2_define_variable( const char *, void *, void *, void * );
    void qnap2_declare_attribute( int, int, void * );
    void qnap2_declare_object( int, void * );
    void qnap2_declare_reference( int, void * list );
    void qnap2_declare_variable( int, void * );
    void qnap2_delete_station_pair( void * );
    void qnap2_construct_chains();
    void qnap2_construct_station();
    const char * qnap2_get_class_name( const char * );		/* checks for class name */
    const char * qnap2_get_station_name( const char * );	/* checks for station name */
    void * qnap2_get_all_objects( int code );
    void * qnap2_get_transit_pair( const char *, const void *, void *, const void * );
    void * qnap2_get_service_distribution( int, void *, void * );
    void qnap2_set_option( const void * );
    void qnap2_set_station_init( const void *, void * );
    void qnap2_set_station_name( const char * );
    void qnap2_set_station_prio( const void *, void * );
    void qnap2_set_entry( void * );
    void qnap2_set_main( void * );
    void qnap2_set_exit( void * );
    void qnap2_set_station_quantum( const void *, const void * );
    void qnap2_set_station_rate( void * );
    void qnap2_set_station_sched( const char * );
    void qnap2_set_station_service( const void *, const void * );
    void qnap2_set_station_transit( const void *, const void * );
    void qnap2_set_station_type( const void * );
    /* LQX */
    void * qnap2_get_array( void * );
    void * qnap2_get_attribute( void *, const char *, void * );
    void * qnap2_get_function( const char * , void * );			/* Returns LQX */
    void * qnap2_get_integer( long );					/* Returns LQX */
    void * qnap2_get_procedure( const char *, void * );			/* Returns LQX */
    void * qnap2_get_real( double );					/* Returns LQX */
    void * qnap2_get_station_type_pair( int, void * );
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
    class ArrayObject;
    class Environment;
    class Program;
    class SyntaxTreeNode;
}

namespace QNIO {
    class QNAP2_Document : public QNIO::Document {
	class SetStationService;
	typedef void (QNAP2_Document::*option_fptr)( bool );

	friend class SetStationService;
	friend const char * ::qnap2_get_class_name( const char * );		/* checks for class name */
	friend const char * ::qnap2_get_station_name( const char * );		/* checks for station name */
	friend void * ::qnap2_comprehension( const void * );
	friend void * ::qnap2_define_variable( const char * name, void * begin, void * end, void * init );
	friend void ::qnap2_declare_attribute( int, int, void * );
	friend void ::qnap2_declare_object( int, void * );
	friend void ::qnap2_declare_reference( int, void * );
	friend void ::qnap2_declare_variable( int, void * );
	friend void ::qnap2_construct_chains();
	friend void ::qnap2_construct_station();
	friend void ::qnap2_set_option( const void * );
	friend void ::qnap2_set_entry( void * );
	friend void ::qnap2_set_main( void * );
	friend void ::qnap2_set_exit( void * );
	friend void ::qnap2_set_station_init( const void *, void * );
	friend void ::qnap2_set_station_name( const char * );
	friend void ::qnap2_set_station_prio( const void *, void * );
	friend void ::qnap2_set_station_quantum( const void *, const void * );
	friend void ::qnap2_set_station_sched( const char * );
	friend void ::qnap2_set_station_service( const void *, const void * );
	friend void ::qnap2_set_station_transit( const void *, const void * );
	friend void ::qnap2_set_station_type( const void * );
	friend void ::qnap2error( const char * fmt, ... );
	friend void * ::qnap2_get_all_objects( int code );
	friend void * ::qnap2_get_array( void * );
	friend void * ::qnap2_get_attribute( void *, const char *, void * );
	friend void * ::qnap2_get_function( const char * , void * );
	friend void * ::qnap2_get_procedure( const char * symbol, void * );
	friend void * ::qnap2_get_service_distribution( int code, void *, void * );
	friend void * ::qnap2_get_transit_pair( const char *, const void *, void *, const void * );
	friend void * ::qnap2_get_variable( const char * );
	friend void * ::qnap2_list( void * initial_value, void * step, void * until );
	friend void * ::qnap2_assignment( void *, void * );
	friend void * ::qnap2_for_statement( void * variable, void * arg2, void * loop_body );
	friend void * ::qnap2_foreach_statement( void * variable, void * list, void * loop_body );

	enum class Type { Undefined, Boolean, Class, Integer, Queue, Real, Reference, String };
	enum class Distribution { Constant, Exponential, Erlang, Hyperexponential, Coxian };

	
	class Symbol {
	public:
	    Symbol( const std::string& name, Type type=Type::Undefined, LQX::SyntaxTreeNode * begin=nullptr, LQX::SyntaxTreeNode * end=nullptr, LQX::SyntaxTreeNode * initial_value=nullptr ) :
		_name(name), _type(type), _object_type(Type::Undefined), _begin(begin), _end(end), _initial_value(initial_value) {}
	    bool operator<( const Symbol& dst ) const { return _name < dst._name; }
	    const std::string& name() const { return _name; }
	    Type type() const { return _type; }
	    void setType( Type type ) { _type = type; }
	    Type objectType() const { return _object_type; }
	    void setObjectType( Type type ) { _object_type = type; }
	    LQX::SyntaxTreeNode * begin() const { return _begin; }
	    LQX::SyntaxTreeNode * end() const { return _end; }
	    LQX::SyntaxTreeNode * initial_value() const { return _initial_value; }
	    bool isScalar() const { return _begin == _end; }
	    bool isVector() const { return _begin != _end; }
	    bool isAttribute() const { return _object_type == QNAP2_Document::Type::Class || _object_type == QNAP2_Document::Type::Queue; }
	    bool isReference() const { return _object_type == QNAP2_Document::Type::Reference; }
	private:
	    const std::string _name;
	    Type _type;
	    Type _object_type;
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
	QNAP2_Document( const std::string& input_file_name );			/* For input */
	QNAP2_Document( const BCMP::Model& model );				/* For output */
	QNAP2_Document( const QNIO::Document& );
	virtual ~QNAP2_Document();

	virtual bool load();
	static bool load( LQIO::DOM::Document&, const std::string& );		// Factory.
	virtual InputFormat getInputFormat() const { return InputFormat::QNAP; }

	virtual bool disableDefaultOutputWithLQX() const { return !_result; }
	virtual LQX::Program * getLQXProgram() { return _lqx; }
	LQX::Environment * getLQXEnvironment() const { return _env; }
	LQX::SymbolAutoRef getLQXSymbol( const std::string& ) const;
	virtual bool preSolve();
	virtual bool postSolve();

	virtual std::vector<LQX::SyntaxTreeNode*> * gnuplot() { return nullptr; }

	virtual std::ostream& print( std::ostream& ) const;
	std::ostream& exportModel( std::ostream& ) const;

	const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }
	bool multiclass() const { return chains().size() > 1; }

    private:
	bool debug() const { return _debug; }

	LQX::SyntaxTreeNode * getAllObjects( QNIO::QNAP2_Document::Type ) const;
	LQX::SyntaxTreeNode * getArray( const Symbol& symbol, QNAP2_Document::Type ) const;
	LQX::ArrayObject* getArrayObject( LQX::SyntaxTreeNode * variable ) const;
	LQX::SyntaxTreeNode * getFunction( const std::string& name, std::vector<LQX::SyntaxTreeNode *>* args );
	LQX::VariableExpression * getVariable( const std::string& name );

	const std::set<Symbol>::const_iterator findAttribute( LQX::VariableExpression * ) const;
	double getDouble( LQX::SyntaxTreeNode * ) const;

	/* QNAP2 interface */
	bool declareClass( const Symbol& );
	bool declareAttribute( Type, Type, const Symbol& );
	bool declareStation( const Symbol& );
	bool defineSymbol( Symbol, Type, Type=Type::Undefined );	// Create copy of Symbol
	void constructChains();
	bool constructStation();
	void setDebug( bool value ) { _debug = value; }
	void setResult( bool value ) { _result = value; }
	void setWarning( bool value ) {}
	bool setStationScheduling( const std::string& );
	bool setStationType( BCMP::Model::Station::Type, LQX::SyntaxTreeNode * );
	bool setStationTransit( const std::string&, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>& );
	void setEntry( LQX::SyntaxTreeNode * );
	void setMain( LQX::SyntaxTreeNode * );
	void setExit( LQX::SyntaxTreeNode * );
	void registerMethods();
	bool hasOutgoingTransits( const std::string& from, const std::string& chain ) const;
	bool hasIncommingTransits( const std::string& from, const std::string& chain ) const;
	const std::map<std::string,LQX::SyntaxTreeNode *>& getTransits( const std::string& from, const std::string& chain ) const;


    private:
	bool isDefined( const std::string& ) const;
	static BCMP::Model::Chain& getChain( const std::string& ); /* throws if not found */
	static BCMP::Model::Station& getStation( const std::string& );
	LQX::SyntaxTreeNode * getInitialValue( const Symbol& symbol, QNAP2_Document::Type type=QNAP2_Document::Type::Undefined, bool=false ) const;

	bool mapTransitToVisits();
	bool mapTransitToVisits( const std::string&, const std::string&, LQX::SyntaxTreeNode *, std::deque<std::string>& );

	class SetOption {
	public:
	    SetOption( QNAP2_Document& document ) : _document(document) {}
	    void operator()( const std::string& ) const;
	private:
	    QNAP2_Document& _document;
	};

	class SetParameter {
	protected:
	    SetParameter( const QNAP2_Document& document ) : _document(document) {}

	    const std::set<Symbol>::const_iterator findAttribute( LQX::VariableExpression * variable ) const;
	    const std::set<Symbol>::const_iterator endAttribute() const { return _document._symbolTable.end(); }

	private:
	    const QNAP2_Document& _document;
	};

	class SetChainCustomers : public SetParameter {
	public:
	    SetChainCustomers( const QNAP2_Document& document, LQX::SyntaxTreeNode* customers ) : SetParameter(document), _customers(customers) {}
	    void operator()( const std::string& ) const;
	    void operator()( BCMP::Model::Chain::pair_t& ) const;
	private:
	    LQX::SyntaxTreeNode * getCustomers( const std::string& ) const;

	    LQX::SyntaxTreeNode * _customers;
	};

	class SetStationPriority : public SetParameter {
	public:
	    SetStationPriority( const QNAP2_Document& document, LQX::SyntaxTreeNode* priority ) : SetParameter(document), _priority(priority) {}
	    void operator()( const std::string& ) const;
	    void operator()( BCMP::Model::Chain::pair_t& ) const;
	private:
	    LQX::SyntaxTreeNode * _priority;
	};
	
	class SetStationService : public SetParameter {
	public:
	    SetStationService( const QNAP2_Document& document, const QNIO::QNAP2_Document::ServiceDistribution& service ) : SetParameter(document), _service(service) {}
	    void operator()( const std::string& ) const;
	    void operator()( const BCMP::Model::Chain::pair_t& ) const;
	private:
	    const QNAP2_Document::ServiceDistribution& _service;
	};

	class SetServiceTime : public SetParameter {
	public:
	    SetServiceTime( const QNAP2_Document& document, const std::string& name, size_t index ) : SetParameter(document), _name(name), _has_index(true), _index(index) {}
	    SetServiceTime( const QNAP2_Document& document, const std::string& name ) : SetParameter(document), _name(name), _has_index(false), _index(0) {}
	    void operator()( BCMP::Model::Station::Class::pair_t& ) const;

	private:
	    const std::string& _name;
	    const bool _has_index;
	    const size_t _index;
	};

	class SetStationTransit {
	public:
	    SetStationTransit( const QNAP2_Document& document, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>& transit ) : _document(document), _transit(transit) {}
	    void operator()( const std::string& class_name ) const { set( class_name ); }
	    void operator()( const BCMP::Model::Chain::pair_t& chain ) const { set( chain.first ); }
	private:
	    void set( const std::string& class_name ) const;
	    const std::set<Symbol>::const_iterator findSymbol( const std::string& name ) const { return _document._symbolTable.find( name ); }
	    const std::set<Symbol>::const_iterator symbolTableEnd() const { return _document._symbolTable.end(); }
	    LQX::SymbolAutoRef getLQXSymbol( const std::string& name ) const { return _document.getLQXSymbol( name ); }

	    const QNAP2_Document& _document;
	    const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>& _transit;

	    struct insert {
		insert( const std::string& base_name, const std::string& class_name, LQX::SyntaxTreeNode * src ) : _base_name(base_name), _class_name(class_name), _src(src) {}

		void operator()() const;
		void operator()( std::pair<LQX::SymbolAutoRef,LQX::SymbolAutoRef> dst ) const;

		const std::string& _base_name;
		const std::string& _class_name;
		LQX::SyntaxTreeNode * _src;
	    };
	};

	class ConstructStation {
	public:
	    ConstructStation( QNAP2_Document& document, const std::string& name, BCMP::Model::Station& station ) : _document(document), _name(name), _station(station) {}
	    bool operator()( const std::pair<LQX::SymbolAutoRef,LQX::SymbolAutoRef>& item ) const;
	    bool operator()() const;

	private:
	    BCMP::Model& model() const { return _document.model(); }
	    BCMP::Model::Station& station() const { return _station; }
	    std::vector<LQX::SyntaxTreeNode *>& program() const { return _document._main; }
	    std::map<std::string,std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>>& transit() const { return _document._transit; }

	private:
	    LQX::SyntaxTreeNode * getServiceTime( const std::string& ) const;

	    QNAP2_Document& _document;
	    const std::string& _name;
	    BCMP::Model::Station& _station;
	};

	class DeepCopy : public LQX::Method {
	public:
	    DeepCopy() {}
	    virtual ~DeepCopy() {}
	    
	    /* All of the glue code to make sure LQX can call print() */
	    virtual std::string getName() const { return "deep_copy"; }
	    virtual const char* getParameterInfo() const { return "aa"; }
	    virtual std::string getHelp() const { return "Copy the contents of the second argument to the first."; }
	    virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);

	private:
	    bool isArray( const LQX::SymbolAutoRef& ) const;

	    struct copy_item {
		copy_item( LQX::ArrayObject * src ) : _src(src) {}
		void operator()( std::pair<LQX::SymbolAutoRef,LQX::SymbolAutoRef> dst ) const;

		LQX::ArrayObject * _src;
	    };
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
	    static const std::streamsize __width;
	    static const std::streamsize __precision;
	    static const std::string __separator;

	    const BCMP::Model& _model;
	};

	class PreSolve : public LQX::Method {
	public:
	    /* maps solve() to QNAP2_Document::preSolve() to set visit ration. Used by BCMP_to_LQN transformation. */
	    PreSolve(QNAP2_Document& model ) : _model(model) {}
	    virtual ~PreSolve() {}
		
	    /* All of the glue code to make sure LQX can call solve() */
	    virtual std::string getName() const { return "solve"; } 
	    virtual const char* getParameterInfo() const { return "1"; } 
	    virtual std::string getHelp() const { return "Solves the model."; } 
	    virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
	
	private:
	    QNAP2_Document& _model;
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
	static std::string to_identifier( const std::string& );
	static std::string to_identifier( const std::string&, const std::string& );

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
	    for_loop( std::ostream& output, const std::map<const std::string,LQX::SyntaxTreeNode *>& input_variables ) : _output(output), _input_variables(input_variables) {}
	    void operator()( const Comprehension& ) const;
	private:
	    const std::map<const std::string,LQX::SyntaxTreeNode *>& input_variables() const { return _input_variables; }

	    std::ostream& _output;
	    const std::map<const std::string,LQX::SyntaxTreeNode *>& _input_variables;	/* Spex vars and inital values	*/
	};

	struct end_for {
	    end_for( std::ostream& output ) : _output(output) {}
	    void operator()( const Comprehension& ) const;
	private:
	    std::ostream& _output;
	};

	static std::string fold( const std::string& s1, const std::string& s2 );
	
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
	/* QNAP2_Document attributes.						*/
	/* -------------------------------------------------------------------- */
    public:
	typedef std::map<std::string,std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>> TransitFromClassToValue;
	
	static const std::map<const scheduling_type,const std::string> __scheduling_str;	/* Maps scheduling_type to qnap2 keyword */
	static const std::map<const std::string,const scheduling_type> __scheduling_type;
	static const std::map<const int,const BCMP::Model::Station::Type> __station_type;
	static const std::map<int,const QNIO::QNAP2_Document::Type> __parser_to_type;
	static const std::map<const std::string,std::pair<option_fptr,bool>> __option;

    private:
	static QNAP2_Document * __document;
	static std::pair<std::string,BCMP::Model::Station> __station;				/* Station under construction */
	static std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>> __transit; 	/* chain, to-station, value under construction */

    private:
	std::set<Symbol> _symbolTable;
	TransitFromClassToValue _transit; /* from, chain, to, value under construction */
	std::vector<LQX::SyntaxTreeNode *> _entry;
	std::vector<LQX::SyntaxTreeNode *> _main;
	std::vector<LQX::SyntaxTreeNode *> _exit;
	LQX::Program * _lqx;
	LQX::Environment * _env;
	bool _debug;
	bool _result;										/* suppress default output.	*/
    };

    inline std::ostream& operator<<( std::ostream& output, const QNAP2_Document& doc ) { return doc.print(output); }
}
#endif
#endif
