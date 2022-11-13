/* -*- c++ -*-
 * $Id: qnap2_document.cpp 16098 2022-11-12 20:26:41Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * December 2020.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <regex>
#include <sstream>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include <lqx/SyntaxTree.h>
#include <lqx/Program.h>
#include "bcmp_bindings.h"
#include "common_io.h"
#include "dom_document.h"
#include "error.h"
#include "filename.h"
#include "glblerr.h"
#include "input.h"
#include "qnap2_document.h"

extern "C" {
    #include "qnap2_gram.h"

    struct yy_buffer_state;
    extern int qnap2parse();
    extern void qnap2error( const char *, ... );

    extern FILE * qnap2in;		/* from srvn_gram.y, implicitly */
    extern yy_buffer_state * qnap2_scan_string( const char * );
    extern void qnap2_delete_buffer( yy_buffer_state * );
}


namespace QNIO {

    QNAP2_Document * QNAP2_Document::__document = nullptr;
    std::pair<std::string,BCMP::Model::Station> QNAP2_Document::__station;		/* Station under construction */
    std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>> QNAP2_Document::__transit;	/* to, chain, value under construction */

    const std::map<const scheduling_type,const std::string> QNAP2_Document::__scheduling_str = {
	{ SCHEDULE_CUSTOMER,"infinite" },
	{ SCHEDULE_DELAY,   "infinite" },
	{ SCHEDULE_FIFO,    "fifo" },
	{ SCHEDULE_PPR,     "preempt" },
	{ SCHEDULE_HOL,     "prior" },
	{ SCHEDULE_PS, 	    "ps" }
    };

    const std::map<const std::string,const scheduling_type> QNAP2_Document::__scheduling_type = {
	{ "fcfs", 	SCHEDULE_FIFO },
	{ "fifo",     	SCHEDULE_FIFO },
	{ "lifo",     	SCHEDULE_LIFO },
	{ "preempt",   	SCHEDULE_PPR },
	{ "prior",    	SCHEDULE_HOL },
	{ "ps",       	SCHEDULE_PS }
    };

    const std::map<const int,const BCMP::Model::Station::Type> QNAP2_Document::__station_type = {
	{ QNAP_INFINITE,    BCMP::Model::Station::Type::DELAY },
	{ QNAP_SINGLE, 	    BCMP::Model::Station::Type::LOAD_INDEPENDENT },
	{ QNAP_MULTIPLE,    BCMP::Model::Station::Type::MULTISERVER },
	{ QNAP_SOURCE, 	    BCMP::Model::Station::Type::SOURCE }
    };

    const std::map<int,const QNIO::QNAP2_Document::Type> QNAP2_Document::__parser_to_type =
    {
	{ QNAP_BOOLEAN, QNIO::QNAP2_Document::Type::Boolean },
	{ QNAP_CLASS, 	QNIO::QNAP2_Document::Type::Class },
	{ QNAP_INTEGER, QNIO::QNAP2_Document::Type::Integer },
	{ QNAP_QUEUE,	QNIO::QNAP2_Document::Type::String },
	{ QNAP_REAL, 	QNIO::QNAP2_Document::Type::Real },
	{ QNAP_STRING, 	QNIO::QNAP2_Document::Type::String },
    };
  
    const std::map<const std::string,std::pair<QNIO::QNAP2_Document::option_fptr,bool>> QNIO::QNAP2_Document::__option = {
	{ "debug",	{ &QNIO::QNAP2_Document::setDebug,	true }  },
	{ "ndebug",	{ &QNIO::QNAP2_Document::setDebug,  	false } },
	{ "result",	{ &QNIO::QNAP2_Document::setResult, 	true }  },
	{ "nresult",	{ &QNIO::QNAP2_Document::setResult, 	false } },
	{ "warn",	{ &QNIO::QNAP2_Document::setWarning,	true }  },
	{ "nwarn",	{ &QNIO::QNAP2_Document::setWarning, 	false } },
    };
    
}

static const bool is_external = false;

/* ------------------------------------------------------------------------ */
/* Parser interface.							    */
/* ------------------------------------------------------------------------ */

/*
 * Append item to list.  Create list if not present.
 */

void *
qnap2_append_pointer( void * list, void * item )
{
    if ( item == nullptr ) return list;
    if ( list == nullptr ) list = static_cast<void *>(new std::vector<void *>());
    static_cast<std::vector<void *>*>(list)->emplace_back( item );
    return list;
}

    
void *
qnap2_append_string( void * list, const char * name )
{
    if ( name == nullptr ) return list;
    if ( list == nullptr ) list = static_cast<void *>(new std::vector<std::string>());
    static_cast<std::vector<std::string>*>(list)->emplace_back(std::string(name));
    return list;
}


void
qnap2_declare_attribute( int code1, int code2, void * list )
{
    if ( list == nullptr ) return;
    const std::map<int,const QNIO::QNAP2_Document::Type>::const_iterator type1 = QNIO::QNAP2_Document::__parser_to_type.find(code1);
    const std::map<int,const QNIO::QNAP2_Document::Type>::const_iterator type2 = QNIO::QNAP2_Document::__parser_to_type.find(code2);
    if ( type1 == QNIO::QNAP2_Document::__parser_to_type.end() ) {
	qnap2error( "type error." );
    } else if ( type2 == QNIO::QNAP2_Document::__parser_to_type.end() ) {
	qnap2error( "type error." );
    } else {
	std::vector<QNIO::QNAP2_Document::Symbol *>* symbol_list = static_cast<std::vector<QNIO::QNAP2_Document::Symbol *>*>(list);
	for ( std::vector<QNIO::QNAP2_Document::Symbol *>::const_iterator symbol = symbol_list->begin(); symbol != symbol_list->end(); ++symbol ) {
	    if ( !QNIO::QNAP2_Document::__document->declareAttribute( type1->second, type2->second, **symbol ) ) {
		qnap2error( "duplicate attribute: %s.", (*symbol)->name().c_str() );
	    }
	}
    }
}



/*
 * Add each item in queue_list to model._chains, but don't populate.
 * That's done by the station.
 */

void
qnap2_declare_class( void * list )
{
    if ( list == nullptr ) return;
    std::vector<QNIO::QNAP2_Document::Symbol *>* class_list = static_cast<std::vector<QNIO::QNAP2_Document::Symbol *>*>(list);
    for ( std::vector<QNIO::QNAP2_Document::Symbol *>::const_iterator symbol = class_list->begin(); symbol != class_list->end(); ++symbol ) {
	if ( !QNIO::QNAP2_Document::__document->declareClass( **symbol ) ) {
	    qnap2error( "duplicate class: %s.", (*symbol)->name().c_str() );
	}
    }
}


/*
 * Add each item in queue_list to model._stations, but don't populate
 */

void
qnap2_declare_queue( void * list )
{
    if ( list == nullptr ) return;
    std::vector<QNIO::QNAP2_Document::Symbol *>* queue_list = static_cast<std::vector<QNIO::QNAP2_Document::Symbol *>*>(list);
    for ( std::vector<QNIO::QNAP2_Document::Symbol *>::const_iterator symbol = queue_list->begin(); symbol != queue_list->end(); ++symbol ) {
	if ( !QNIO::QNAP2_Document::__document->declareStation( **symbol ) ) {
	    qnap2error( "duplicate queue: %s.", (*symbol)->name().c_str() );
	}
    }
}


void
qnap2_declare_reference( int code, void * list )
{
    if ( list == nullptr ) return;
    const std::map<int,const QNIO::QNAP2_Document::Type>::const_iterator type = QNIO::QNAP2_Document::__parser_to_type.find(code);
    if ( type == QNIO::QNAP2_Document::__parser_to_type.end() ) abort();
    std::vector<std::string>* identifier_list = static_cast<std::vector<std::string>*>(list);
    for ( std::vector<std::string>::const_iterator identifier = identifier_list->begin(); identifier != identifier_list->end(); ++identifier ) {
	if ( !QNIO::QNAP2_Document::__document->defineSymbol( type->second, *identifier, true ) ) {
	    qnap2error( "duplicate symbol: %s.", (*identifier).c_str() );
	}
    }
}

void
qnap2_declare_variable( int code, void * list )
{
    if ( list == nullptr ) return;
    const std::map<int,const QNIO::QNAP2_Document::Type>::const_iterator type = QNIO::QNAP2_Document::__parser_to_type.find(code);
    if ( type == QNIO::QNAP2_Document::__parser_to_type.end() ) abort();
    std::vector<QNIO::QNAP2_Document::Symbol *>* symbol_list = static_cast<std::vector<QNIO::QNAP2_Document::Symbol *>*>(list);
    for ( std::vector<QNIO::QNAP2_Document::Symbol *>::const_iterator symbol = symbol_list->begin(); symbol != symbol_list->end(); ++symbol ) {
	if ( !QNIO::QNAP2_Document::__document->defineSymbol( type->second, **symbol ) ) {
	    qnap2error( "duplicate symbol: %s.", (*symbol)->name().c_str() );
	}
    }
}


void *
qnap2_define_variable( const char * name, void * begin, void * end, void * init )
{
    return new QNIO::QNAP2_Document::Symbol( std::string(name), QNIO::QNAP2_Document::Type::Undefined, static_cast<LQX::SyntaxTreeNode *>(begin), 
					     static_cast<LQX::SyntaxTreeNode *>(end), static_cast<LQX::SyntaxTreeNode *>(init) );
}


void qnap2_define_station()
{
    if ( !QNIO::QNAP2_Document::__document->defineStation() ) {
	qnap2error( "station name not set." );
    }
}


void qnap2_declare_objects()
{
    QNIO::QNAP2_Document::__document->declareObjects();
}


void qnap2_map_transit_to_visits()
{
    try {
	QNIO::QNAP2_Document::__document->mapTransitToVisits();
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
}



void * qnap2_get_all_objects( int code )
{
    static const std::map<int,const QNIO::QNAP2_Document::Type> parser_to_array = {
	{ QNAP_CLASS, 	QNIO::QNAP2_Document::Type::Queue },
	{ QNAP_QUEUE, 	QNIO::QNAP2_Document::Type::Class },
    };

    try {
	return QNIO::QNAP2_Document::__document->getAllObjects( parser_to_array.at(code) );
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
    return nullptr;
}

void * qnap2_get_attribute( void * arg1, const char * arg2 )
{
    return new LQX::ObjectPropertyReadNode( static_cast<LQX::SyntaxTreeNode *>(arg1), arg2 );
}


/*
 * Checks for class name and throws if not found
 */

const char * qnap2_get_class_name( const char * class_name )
{
    try {
	QNIO::QNAP2_Document::__document->getChain( class_name );
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
    return class_name;
}

/*
 * Both functions and arrays have the same syntax, so is it a variable or is it a function.
 */

void * qnap2_get_function( const char * name, void * arg2 )
{
    try {
	return QNIO::QNAP2_Document::__document->getFunction( name, static_cast<std::vector<LQX::SyntaxTreeNode *>*>(arg2) );
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
    return nullptr;
}


void * qnap2_get_integer( long value )
{
    /* add to symbol table */
    return new LQX::ConstantValueExpression( static_cast<double>( value ) );
}

/* checks for station name */
const char *
qnap2_get_station_name( const char * station_name )
{
#if 0
    /*!!! Check the symbol table.  add symbolIsDefined( name ) const {} */
    try {
	QNIO::QNAP2_Document::__document->getStation( station_name );
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
#endif
    return station_name;
    
}

void * qnap2_get_procedure( const char * name, void * arg2 )
{
    if ( arg2 != nullptr ) {
	return new LQX::MethodInvocationExpression( name, static_cast<std::vector<LQX::SyntaxTreeNode *>*>(arg2) );
    } else {
	return new LQX::MethodInvocationExpression( name );
    }
}


void * qnap2_get_real( double value )
{
    /* add to symbol table */
    return new LQX::ConstantValueExpression( value );
}

void * qnap2_get_service_distribution( int code, void * mean, void * shape )
{
    static const std::map<int,const QNIO::QNAP2_Document::Distribution> parser_to_distribution = {
	{ QNAP_COX, 	QNIO::QNAP2_Document::Distribution::Coxian },
	{ QNAP_CST, 	QNIO::QNAP2_Document::Distribution::Constant },
	{ QNAP_ERLANG, 	QNIO::QNAP2_Document::Distribution::Erlang },
	{ QNAP_EXP, 	QNIO::QNAP2_Document::Distribution::Exponential },
	{ QNAP_HEXP, 	QNIO::QNAP2_Document::Distribution::Hyperexponential }
    };

    if ( mean == nullptr ) return nullptr;
    return new QNIO::QNAP2_Document::ServiceDistribution( parser_to_distribution.at(code), static_cast<LQX::SyntaxTreeNode *>(mean), static_cast<LQX::SyntaxTreeNode *>(shape) );
}

void * qnap2_get_station_type_pair( int code, int copies )
{
    return new std::pair<int,int>(code,copies);
}

void * qnap2_get_string( const char * string )
{
    return new LQX::ConstantValueExpression( string );
}

void * qnap2_get_transit_pair( const char * name, void * value )
{
    if ( value == nullptr ) {
	return new std::pair<const std::string,LQX::SyntaxTreeNode *>( std::string(name), new LQX::ConstantValueExpression(1.0) );
    } else {
	return new std::pair<const std::string,LQX::SyntaxTreeNode *>( std::string(name), static_cast<LQX::SyntaxTreeNode *>(value) );
    }
}

    
void * qnap2_get_variable( const char * name )
{
    try {
	return QNIO::QNAP2_Document::__document->getVariable( name );
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
    return nullptr;
}


void qnap2_set_program( void * program )
{
    QNIO::QNAP2_Document::__document->setProgram( static_cast<LQX::SyntaxTreeNode*>(program) );
}


void qnap2_set_option( const void * list )
{
    const std::vector<std::string>* options = static_cast<const std::vector<std::string>*>(list);
    std::for_each( options->begin(), options->end(), QNIO::QNAP2_Document::SetOption( *QNIO::QNAP2_Document::__document ) );
}


/*
 * Can be init(class) = value;  If only one class, init=value;.
 * Or init=var name.
 */

void qnap2_set_station_init( const void * list, void * value )
{
    if ( value == nullptr ) return;
    try {
	QNIO::QNAP2_Document::SetChainCustomers set_station_init( *QNIO::QNAP2_Document::__document, static_cast<LQX::SyntaxTreeNode*>(value) );
	if ( list == nullptr ) {
	    BCMP::Model::Chain::map_t& chains = QNIO::QNAP2_Document::__document->model().chains();
	    std::for_each( chains.begin(), chains.end(), set_station_init );
	} else {
	    const std::vector<std::string>* classes = static_cast<const std::vector<std::string>*>(list);
	    std::for_each( classes->begin(), classes->end(), set_station_init );
	}
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
}


/*
 * Set the name of the station under construction.
 */
 
void qnap2_set_station_name( const char * name )
{
    QNIO::QNAP2_Document::__station.first = name;
}

void qnap2_set_station_sched( const char * scheduling )
{
    if ( !QNIO::QNAP2_Document::__document->setStationScheduling( scheduling ) ) {
	qnap2error( "undefined station scheduling: %s.", scheduling );
    }
}

void qnap2_set_station_service( const void * list, const void * arg2 )
{
    /* Service list is Service::Distribution. */
    if ( arg2 == nullptr ) return;
    const QNIO::QNAP2_Document::ServiceDistribution* service = static_cast<const QNIO::QNAP2_Document::ServiceDistribution*>(arg2);
    try {
	QNIO::QNAP2_Document::SetStationService set_service( *QNIO::QNAP2_Document::__document, *service );
	if ( list == nullptr ) {
	    const BCMP::Model::Chain::map_t& chains = QNIO::QNAP2_Document::__document->chains();
	    std::for_each( chains.begin(), chains.end(), set_service );
	} else {
	    const std::vector<std::string>* classes = static_cast<const std::vector<std::string>*>(list);
	    std::for_each( classes->begin(), classes->end(), set_service );
	}
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
}

void qnap2_set_station_transit( const void * list, const void * arg2 )
{
    if ( arg2 == nullptr ) return;
    const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>* transit = static_cast<const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>*>(arg2);
    try {
	QNIO::QNAP2_Document::SetStationTransit set_transit( *transit );
	if ( list == nullptr ) {
	    const BCMP::Model::Chain::map_t& chains = QNIO::QNAP2_Document::__document->chains();
	    std::for_each( chains.begin(), chains.end(), set_transit );
	} else {
	    const std::vector<std::string>* classes = static_cast<const std::vector<std::string>*>(list);
	    std::for_each( classes->begin(), classes->end(), set_transit );
	}
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
}

void qnap2_set_station_type( const void * arg )
{
    const std::pair<int,int>* type = static_cast<const std::pair<int,int>*>( arg );
    QNIO::QNAP2_Document::__document->setStationType( QNIO::QNAP2_Document::__station_type.at(type->first), type->second );
}

void qnap2_set_station_prio( const void *, const void * ){}
void qnap2_set_station_quantum( const void *, const void * ){}
void qnap2_set_station_rate( void * ){}

void * qnap2_assignment( void * arg1, void * arg2 )
{
    return new LQX::AssignmentStatementNode(static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2));
}


void * qnap2_compound_statement( void * list )
{
    return new LQX::CompoundStatementNode( static_cast<std::vector<LQX::SyntaxTreeNode*>*>( list ) );
}


void * qnap2_if_statement( void * condition, void * if_stmt, void * else_stmt )
{
    return new LQX::ConditionalStatementNode( static_cast<LQX::SyntaxTreeNode *>(condition), static_cast<LQX::SyntaxTreeNode *>(if_stmt), static_cast<LQX::SyntaxTreeNode *>(else_stmt) );
}

/*
 * for variable := sublist do loop_stmt
 *  LoopStatementNode::LoopStatementNode(SyntaxTreeNode* onStart, SyntaxTreeNode* stop, SyntaxTreeNode* onEachRun, SyntaxTreeNode* action) :
 *    _onBegin(onStart), _stopCondition(stop), _onEachRun(onEachRun), _action(action)
 */

void * qnap2_for_statement( void * arg1, void * arg2, void * loop_body ) 
{
    LQX::SyntaxTreeNode * variable = static_cast<LQX::SyntaxTreeNode *>(arg1);
    const QNIO::QNAP2_Document::List * sublist = static_cast<QNIO::QNAP2_Document::List *>(arg2);
    return new LQX::LoopStatementNode( new LQX::AssignmentStatementNode( variable, sublist->initial_value() ),
				       new LQX::ComparisonExpression( LQX::ComparisonExpression::LESS_OR_EQUAL, variable, sublist->until() ),
				       new LQX::AssignmentStatementNode( variable, new LQX::MathExpression( LQX::MathExpression::ADD, variable, sublist->step() ) ),
				       static_cast<LQX::SyntaxTreeNode *>(loop_body) );
}



/*
 * Loop through either all of the objects in "list".
 */

void * qnap2_foreach_statement( void * arg1, void * arg2, void * loop_body )
{
    LQX::SyntaxTreeNode * variable = static_cast<LQX::SyntaxTreeNode *>(arg1);
    LQX::SyntaxTreeNode * array = static_cast<LQX::SyntaxTreeNode *>(arg2);
    if ( dynamic_cast<LQX::VariableExpression *>(variable) != nullptr ) {
	return new LQX::ForeachStatementNode( "", dynamic_cast<LQX::VariableExpression *>(variable)->getName(), false, is_external,
					      array, static_cast<LQX::SyntaxTreeNode *>(loop_body) );	/* Key, value, key_is_ext, var_is_ext, array, loop_body */
    } else {
	throw std::logic_error( "invalid foreach loop variable." );
    }
    return nullptr;
}


void * qnap2_list( void * initial_value, void * step, void * until )
{
    return new QNIO::QNAP2_Document::List( static_cast<LQX::SyntaxTreeNode *>(initial_value), static_cast<LQX::SyntaxTreeNode *>(step), static_cast<LQX::SyntaxTreeNode *>(until) );
}

void * qnap2_logic( int operation, void * arg1, void * arg2 )
{
    static const std::map<int,LQX::LogicExpression::LogicOperation> parser_to_logic = {
	{ QNAP_AND,		LQX::LogicExpression::AND },
	{ QNAP_OR,		LQX::LogicExpression::OR },
	{ QNAP_NOT,		LQX::LogicExpression::NOT },
    };

    return new LQX::LogicExpression( parser_to_logic.at(operation), static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2) );
}


void * qnap2_math( int operation, void * arg1, void * arg2 )
{
    static const std::map<int,LQX::MathExpression::MathOperation> parser_to_math = {
	{ QNAP_PLUS,		LQX::MathExpression::ADD },
	{ QNAP_DIVIDE,		LQX::MathExpression::DIVIDE },
	{ QNAP_MODULUS,		LQX::MathExpression::MODULUS },
	{ QNAP_MULTIPLY,	LQX::MathExpression::MULTIPLY },
	{ QNAP_POWER,		LQX::MathExpression::POWER },                                
	{ QNAP_MINUS,		LQX::MathExpression::SUBTRACT },
    };

    if ( operation == QNAP_MINUS && arg1 == nullptr ) {
	return new LQX::MathExpression( LQX::MathExpression::NEGATE, static_cast<LQX::SyntaxTreeNode *>(arg2), nullptr );
    } else {
	return new LQX::MathExpression( parser_to_math.at(operation), static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2) );
    }
}

	

/* Math */

void * qnap2_relation( int operation, void * arg1, void * arg2 )
{
    static const std::map<int,LQX::ComparisonExpression::CompareMode> parser_to_relation = {
	{ QNAP_EQUAL,		LQX::ComparisonExpression::EQUALS },
	{ QNAP_GREATER,		LQX::ComparisonExpression::GREATER_THAN },
	{ QNAP_GREATER_EQUAL,	LQX::ComparisonExpression::GREATER_OR_EQUAL },
	{ QNAP_LESS,		LQX::ComparisonExpression::LESS_THAN },
	{ QNAP_NOT_EQUAL,	LQX::ComparisonExpression::NOT_EQUALS },
 	{ QNAP_LESS_EQUAL,	LQX::ComparisonExpression::LESS_OR_EQUAL },
    };

    return new LQX::ComparisonExpression( parser_to_relation.at(operation), static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2) );
}



void * qnap2_while_statement( void * condition, void * loop_stmt )
{
    return new LQX::LoopStatementNode( nullptr, static_cast<LQX::SyntaxTreeNode *>(condition), nullptr, static_cast<LQX::SyntaxTreeNode *>(loop_stmt) );
}



/*
 * Print out and error message (and the line number on which it
 * occurred.
 */

void
qnap2error( const char * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    LQIO::verrprintf( stderr, LQIO::error_severity::ERROR, QNIO::QNAP2_Document::__document->getInputFileName().c_str(), qnap2lineno, 0, fmt, args );
    va_end( args );
}

namespace QNIO {
    /*
     * The Symbol table is partitioned into three categories.
     */
    bool
    QNAP2_Document::Symbol::operator<( const Symbol& dst ) const
    {
	return _name < dst._name;
    }
}

namespace QNIO {
    /* ------------------------------------------------------------------------ */
    /*                                  Input                                   */
    /* ------------------------------------------------------------------------ */

    QNAP2_Document::QNAP2_Document( const std::string& input_file_name ) :
	Document(input_file_name, BCMP::Model()),
 	_attributes(), _transit(), _program(), _lqx(LQX::Program::loadRawProgram( &_program )), _env(_lqx->getEnvironment()), 
	_debug(false), _result(true)
    {
	registerMethods();
	__document = this;
    }

    QNAP2_Document::QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model ) :
	Document(input_file_name, model),
 	_attributes(), _transit(), _program(), _lqx(LQX::Program::loadRawProgram( &_program )), _env(_lqx->getEnvironment()),
	_debug(false), _result(true)
    {
	registerMethods();
	__document = this;
    }

    QNAP2_Document::~QNAP2_Document()
    {
	__document = nullptr;
    }

    /*
     * Load the model.
     */

    bool
    QNAP2_Document::load()
    {
	unsigned errorCode = 0;
	if ( !LQIO::Filename::isFileName( getInputFileName() ) ) {
	    qnap2in = stdin;
	} else if (!( qnap2in = fopen( getInputFileName().c_str(), "r" ) ) ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open input file " << getInputFileName() << " - " << strerror( errno ) << std::endl;
	    return false;
	} 
	int qnap2in_fd = fileno( qnap2in );

	struct stat statbuf;
	if ( isatty( qnap2in_fd ) ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Input from terminal is not allowed." << std::endl;
	    return false;
	} else if ( fstat( qnap2in_fd, &statbuf ) != 0 ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot stat " << getInputFileName() << " - " << strerror( errno ) << std::endl;
	    return false;
#if defined(S_ISSOCK)
	} else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) ) {
#else
	} else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) ) {
#endif
	    std::cerr << LQIO::io_vars.lq_toolname << ": Input from " << getInputFileName() << " is not allowed." << std::endl;
	    return false;
	} 

//	qnap2_lineno = 1;

#if HAVE_MMAP
	char * buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, qnap2in_fd, 0 ));
	if ( buffer != MAP_FAILED ) {
	    yy_buffer_state * yybuf = qnap2_scan_string( buffer );
	    try {
		errorCode = qnap2parse();
	    }
	    catch ( const std::domain_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": " << e.what() << "." << std::endl;
		errorCode = 1;
	    }
	    catch ( const std::runtime_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": " << e.what() << "." << std::endl;
		errorCode = 1;
	    }
	    qnap2_delete_buffer( yybuf );
	    munmap( buffer, statbuf.st_size );
	} else {
#endif
	    /* Try the old way (for pipes) */
	    try {
		errorCode = qnap2parse();
	    }
	    catch ( const std::domain_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": " << e.what() << "." << std::endl;
		errorCode = 1;
	    }
	    catch ( const std::runtime_error& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": " << e.what() << "." << std::endl;
		errorCode = 1;
	    }
#if HAVE_MMAP
	}
#endif
	if ( qnap2in && qnap2in != stdin ) {
	    fclose( qnap2in );
	}

	return errorCode == 0;
    }


    bool
    QNAP2_Document::declareClass( const Symbol& symbol )
    {
	return symbol.isScalar() && defineSymbol( QNAP2_Document::Type::Class, symbol ) && model().chains().emplace( BCMP::Model::Chain::pair_t( symbol.name(), BCMP::Model::Chain() ) ).second;
    }


    bool
    QNAP2_Document::declareAttribute( QNAP2_Document::Type object_type, QNAP2_Document::Type attribute_type, Symbol& symbol )
    {
	return symbol.isScalar() && defineSymbol( QNAP2_Document::Type::Attribute, symbol ) && _attributes.emplace( symbol.name(), object_type ).second
	    && BCMP::Attributes::addAttribute( symbol.name(), getInitialValue( attribute_type, symbol ) );
    }

    bool
    QNAP2_Document::declareStation( const Symbol& symbol )
    {
	return defineSymbol( QNAP2_Document::Type::Queue, symbol );
    }

    bool
    QNAP2_Document::defineSymbol( QNAP2_Document::Type type, const Symbol& symbol, bool is_reference )
    {
	if ( symbol.name().empty() ) return true;
	_lqx->defineExternalVariable( symbol.name() );

	if ( !is_reference ) {
	    LQX::VariableExpression * lvalue = new LQX::VariableExpression( symbol.name(), is_external );
	    LQX::SyntaxTreeNode * initial_value = getInitialValue( type, symbol );
	    if ( initial_value != nullptr ) {
		_program.push_back( new LQX::AssignmentStatementNode( lvalue, initial_value ) );
	    }
	}
	return true;
    }


    /*
     * Create LQX objects for all symbols (and classes).
     */
    
    void
    QNAP2_Document::declareObjects()
    {
	BCMP::Model::Chain::map_t& chains = model().chains();
	if ( chains.empty() ) {
	    declareClass( Symbol( std::string("") ) );
	}
    }



    /*
     * Since we don't know the station name ahead of time to find the
     * object in the map, copy the station constructed by the parser
     * to the BCMP::Document model once everything has been defined.
     * Fill in defaults as necessary.
     */
    
    bool
    QNAP2_Document::defineStation()
    {
	const std::string& station_name = __station.first;
	/* This will have to be a loop if station is an array */
	std::pair<BCMP::Model::Station::map_t::iterator,bool> insertion = model().stations().emplace( station_name, __station.second );
	if ( !insertion.second ) return false;

	BCMP::Model::Station& station =	insertion.first->second;

	/* Copy over constructed values */
	
	_transit.emplace( station_name, __transit );	/* copy */
	    
	/* Set defaults if not set */

	if ( station.type() == BCMP::Model::Station::Type::NOT_DEFINED ) {	/* Default */
	    if ( station.scheduling() == SCHEDULE_DELAY ) {
		station.setType( BCMP::Model::Station::Type::DELAY );
	    } else if ( station.copies() != nullptr ) {
		station.setType( BCMP::Model::Station::Type::MULTISERVER );
	    } else {
		station.setType( BCMP::Model::Station::Type::LOAD_INDEPENDENT );
	    }
	}
	if ( station.copies() == nullptr ) {
	    station.setCopies( new LQX::ConstantValueExpression( 1. ) );	/* Default 1 */
	}

	/* Set the name attribute of the station. */
	_program.push_back( new LQX::AssignmentStatementNode( new LQX::ObjectPropertyReadNode( new LQX::VariableExpression( station_name, is_external ), "name" ), new LQX::ConstantValueExpression( station_name ) ) );

	/* Reset */
	
	__transit.clear();
	__station.first.clear();
	__station.second.clear();
	
	return true;
    }



    /*
     * starting from all reference or source stations, follow the
     * transit graph and convert the transit chain to visits to the
     * station.
     */
    
    bool
    QNAP2_Document::mapTransitToVisits()
    {
	// for each chain, locate the reference station.  There should be exactly one.
	// follow the transit list from the reference station (recursive).  We should
	// end up at the reference station (or out).

	const BCMP::Model::Chain::map_t& chains = model().chains();
	BCMP::Model::Station::map_t& stations = model().stations();
	for ( BCMP::Model::Chain::map_t::const_iterator chain = chains.begin(); chain != chains.end(); ++chain ) {
	    BCMP::Model::Station::map_t::iterator start = stations.end();
	    for ( BCMP::Model::Station::map_t::iterator station = stations.begin(); station != stations.end(); ++station ) {
		if ( !station->second.reference() && station->second.type() != BCMP::Model::Station::Type::SOURCE ) continue;
		if ( start != stations.end() ) throw std::logic_error( std::string( "Duplicate source station: " ) + station->first );
		start = station;
	    }
	    if ( start == stations.end() ) {
		throw std::logic_error( std::string( "No reference or source stations." ) );
	    }
	    const std::string& station_name = start->first;
	    const std::string& class_name = chain->first;
	    std::deque<std::string> stack;
	    mapTransitToVisits( station_name, class_name, new LQX::ConstantValueExpression( 1.0 ), stack );	    // set visits to reference station to 1 */
	    assert( stack.empty() );
	}
	return true;
    }


    /*
     * If we find we have reached a node which is on the stack, then we are done.  Set the visits to that node to the sum of what so far plus the value on the top of stack.
     * Otherwise, if the station transits out for this class, set the visits to the current vists, then recurse down the transit list.  If the station doesn't transit out, then simply return.
     */

    bool
    QNAP2_Document::mapTransitToVisits( const std::string& station_name, const std::string& class_name, LQX::SyntaxTreeNode * visits, std::deque<std::string>& stack )
    {
	BCMP::Model::Station::Class& k = model().stationAt( station_name ).classAt( class_name );
	
	/* have we completed the cycle? */
	
	if ( k.visits() != nullptr || std::find( stack.begin(), stack.end(), station_name ) != stack.end() ) return true;	/* Yes, return */

	/* Do we transit out from this station? */

	const size_t outgoing_transits = countOutgoingTransits( station_name, class_name );
	const size_t incomming_transits = countIncommingTransits( station_name, class_name );
	if ( outgoing_transits == 0 || incomming_transits == 0 ) return true;		/* No visits here.	*/
#if 0
	/* Fails on regression/2-class, though qnap2 needs the definition */
	if ( outgoing_transits != incomming_transits ) {
	    throw std::logic_error( std::string( "Transits in do not match transits out for station " ) + station_name );
	}
#endif	
	/* Ok, we transit out from here and we have transits in */

	const std::map<std::string,LQX::SyntaxTreeNode *>& transits = getTransits( station_name, class_name  );
	LQX::SyntaxTreeNode * sum = std::accumulate( transits.begin(), transits.end(), static_cast<LQX::SyntaxTreeNode *>(nullptr), fold_transit( class_name ) );	// class name not needed here.
	k.setVisits( BCMP::Model::multiply( visits, sum ) );
	stack.push_back( station_name );
	for ( std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator transit = transits.begin(); transit != transits.end(); ++transit ) {
	    mapTransitToVisits( transit->first, class_name, BCMP::Model::multiply( visits, transit->second ), stack );
	}
	stack.pop_back();
	return true;
    }


    void
    QNAP2_Document::SetOption::operator()( const std::string& arg ) const
    {
	std::map<const std::string,std::pair<QNIO::QNAP2_Document::option_fptr,bool>>::const_iterator option = QNIO::QNAP2_Document::__option.find(arg);
	if ( option != QNIO::QNAP2_Document::__option.end() ) {
	    (_document.*option->second.first)(option->second.second);
	}
    }


    void
    QNAP2_Document::SetChainCustomers::operator()( BCMP::Model::Chain::pair_t& chain ) const
    {
	if ( chain.second.type() != BCMP::Model::Chain::Type::UNDEFINED ) return;	/* Skip those that are set */
	if ( chain.second.type() == BCMP::Model::Chain::Type::OPEN ) throw std::invalid_argument( std::string( "mixed type: " ) + chain.first );
	chain.second.setType( BCMP::Model::Chain::Type::CLOSED );
	chain.second.setCustomers( getCustomers( chain.first ) );
	QNAP2_Document::__station.second.setReference( true );
    }

    
    void
    QNAP2_Document::SetChainCustomers::operator()( const std::string& class_name ) const
    {
	BCMP::Model::Chain& chain = QNAP2_Document::getChain( class_name );
	if ( chain.type() == BCMP::Model::Chain::Type::OPEN ) throw std::invalid_argument( std::string( "mixed type: " ) + class_name );
	chain.setType( BCMP::Model::Chain::Type::CLOSED );
	chain.setCustomers( getCustomers( class_name ) );
	QNAP2_Document::__station.second.setReference( true );
    }

    
    /*
     * get the serivce time.  If _service.mean() is an run of the mill variable, then just use that.  If it is an attribute, then it should be attached to a class.
     */

    LQX::SyntaxTreeNode *
    QNIO::QNAP2_Document::SetChainCustomers::getCustomers( const std::string& class_name ) const
    {
	LQX::VariableExpression * variable = dynamic_cast<LQX::VariableExpression *>(_customers);
	if ( isAttribute( variable ) ) {
	    return new LQX::ObjectPropertyReadNode( new LQX::VariableExpression( class_name, is_external ), variable->getName() );
	} else {
	    return _customers;
	}
    }


    void
    QNIO::QNAP2_Document::SetStationService::operator()( const BCMP::Model::Chain::pair_t& chain ) const
    {
	BCMP::Model::Station::Class& k = QNIO::QNAP2_Document::__station.second.classes().emplace( chain.first, BCMP::Model::Station::Class() ).first->second;
	k.setServiceTime( getServiceTime( chain.first ) );
    }


    void
    QNIO::QNAP2_Document::SetStationService::operator()( const std::string& class_name ) const
    {
	BCMP::Model::Station::Class& k = QNIO::QNAP2_Document::__station.second.classes().emplace( class_name, BCMP::Model::Station::Class() ).first->second;
	if ( k.service_time() != nullptr ) throw std::domain_error( "service time previously set." );
	k.setServiceTime( getServiceTime( class_name ) );		// !!!
    }

    /* get the serivce time.  If _service.mean() is an run of the mill variable, then just use that.  If it is an attribute (!) */

    LQX::SyntaxTreeNode *
    QNIO::QNAP2_Document::SetStationService::getServiceTime( const std::string& class_name ) const
    {
	LQX::VariableExpression * variable = dynamic_cast<LQX::VariableExpression *>(_service.mean());
	if ( isAttribute( variable ) ) {
	    return new LQX::ObjectPropertyReadNode( new LQX::VariableExpression( class_name, is_external ), variable->getName() );
	} else {
	    return _service.mean();
	}
    }

    
    /*
     * Save the transit information ( from station, class, to station, value )
     * If class is empty, then do all classes that have not been assigned 
     */
    
    void
    QNAP2_Document::SetStationTransit::operator()( const std::string& class_name ) const
    {
	set( class_name );
    }
    
    void
    QNAP2_Document::SetStationTransit::operator()( const BCMP::Model::Chain::pair_t& chain ) const
    {
	set( chain.first );
    }

    void
    QNAP2_Document::SetStationTransit::set( const std::string& class_name ) const
    {
//!!!	const BCMP::Model::Station::map_t& stations = QNAP2_Document::__document->model().stations();
	for ( std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>::const_iterator transit = _transit.begin(); transit != _transit.end(); ++transit ) {
	    const std::string& station_name = (*transit)->first;
//!!!	    if ( stations.find( station_name ) == stations.end() ) throw std::invalid_argument( std::string( "undefined station: " ) + station_name );
	    __transit.emplace( class_name, std::map<std::string,LQX::SyntaxTreeNode *>() ).first->second.emplace( station_name, (*transit)->second );
	}
    }


    
    bool
    QNAP2_Document::setStationScheduling( const std::string& value )
    {
	const std::map<const std::string,const scheduling_type>::const_iterator scheduling = __scheduling_type.find( value );
	if ( scheduling == __scheduling_type.end() ) return false;
	BCMP::Model::Station& station = __station.second;
	station.setScheduling( scheduling->second );
	return true;
    }
    


    bool
    QNAP2_Document::setStationType( const BCMP::Model::Station::Type type, int copies )
    {
	BCMP::Model::Station& station = __station.second;
	if ( station.type() != BCMP::Model::Station::Type::NOT_DEFINED && station.type() != type ) return false;
	station.setType( type );
	if ( type == BCMP::Model::Station::Type::DELAY ) {
	    station.setScheduling( SCHEDULE_DELAY );
	}
	return true;
    }


    
    /*
     * Locate the class/chain from the name.  Throw if not found.
     */
    
    BCMP::Model::Chain&
    QNAP2_Document::getChain( const std::string& class_name )
    {
	BCMP::Model::Chain::map_t& chains = QNAP2_Document::__document->model().chains();
	BCMP::Model::Chain::map_t::iterator chain = chains.find( class_name );
	if ( chain == chains.end() ) throw std::invalid_argument( std::string( "undefined class: " ) + class_name );
	return chain->second;
    }


    /*
     * Return an array with all of the items of type.
     */
    
    LQX::SyntaxTreeNode *
    QNAP2_Document::getAllObjects( QNIO::QNAP2_Document::Type type ) const
    {
	std::vector<LQX::SyntaxTreeNode *>* items = new std::vector<LQX::SyntaxTreeNode *>();
	if ( type == QNIO::QNAP2_Document::Type::Queue ) {
	    for ( BCMP::Model::Station::map_t::const_iterator station = stations().begin(); station != stations().end(); ++station ) {
		items->push_back( new LQX::MethodInvocationExpression( "station", new LQX::ConstantValueExpression( station->first ), nullptr ) );
	    }
	} else if ( type == QNIO::QNAP2_Document::Type::Class ) {
	    /* chain names are the classes in the document */
	    for ( BCMP::Model::Chain::map_t::const_iterator chain = chains().begin(); chain != chains().end(); ++chain ) {
		items->push_back( new LQX::VariableExpression( chain->first, is_external ) );
	    }
	} else {
	    throw std::logic_error( "???" );
	}
	return new LQX::MethodInvocationExpression( "array_create", items );
    }


    
    /*
     * Both functions and arrays have the same syntax, so is it a
     * variable or is it a function.
     */
    
    LQX::SyntaxTreeNode *
    QNAP2_Document::getFunction( const std::string& name, std::vector<LQX::SyntaxTreeNode *>* args )
    {
	if ( getLQXEnvironment()->getSpecialSymbolTable()->isDefined( name ) ) {
	    /* array */
	} else if ( getLQXEnvironment()->getMethodTable()->isDefined( name ) ) {
	    if ( args != nullptr ) {
		return new LQX::MethodInvocationExpression( name, args );
	    } else {
		return new LQX::MethodInvocationExpression( name );
	    }
	}
	throw std::invalid_argument( std::string( "undefined function or array: " ) + name );
	return nullptr;
    }

    
    LQX::SyntaxTreeNode * QNAP2_Document::getInitialValue( QNAP2_Document::Type type, const Symbol& symbol ) 
    {
    	LQX::SyntaxTreeNode * initial_value = symbol.initial_value();
	if ( initial_value == nullptr ) {
	    switch ( type ) {
	    case Type::Boolean:	    initial_value = new LQX::ConstantValueExpression( false ); break;
	    case Type::Integer:	    /* Fall through, no integers, use real. */
	    case Type::Real:	    initial_value = new LQX::ConstantValueExpression( 0. ); break;
	    case Type::String:	    initial_value = new LQX::ConstantValueExpression( "" ); break;
	    case Type::Queue:	    initial_value = new LQX::MethodInvocationExpression( "station", new LQX::ConstantValueExpression( symbol.name() ), nullptr ); break;
	    case Type::Class:	    initial_value = new LQX::MethodInvocationExpression( "chain", new LQX::ConstantValueExpression( symbol.name() ), nullptr ); break;
	    case Type::Attribute:   initial_value = new LQX::ConstantValueExpression( symbol.name() ); break;
	    default: abort();
	    }
	}
	return initial_value;
    }

    
    /*
     * Locate the station from the name.  Throw if not found.
     */
    
    BCMP::Model::Station& 
    QNAP2_Document::getStation( const std::string& station_name )
    {
	BCMP::Model::Station::map_t& stations = QNAP2_Document::__document->model().stations();
	BCMP::Model::Station::map_t::iterator station = stations.find( station_name );
	if ( station == stations.end() ) throw std::invalid_argument( std::string( "undefined queue: " ) + station_name );
	return station->second;
    }


    /*
     * Variables are reals, booleans, strings, queues and classes.
     */
    
    LQX::VariableExpression *
    QNAP2_Document::getVariable( const std::string& name )
    {
	LQX::SymbolTable* symbol_table = getLQXEnvironment()->getSpecialSymbolTable();
	if ( !symbol_table->isDefined( name ) ) {
	    throw std::invalid_argument( std::string( "undefined variable: " ) + name );
	}
	return new LQX::VariableExpression( name, is_external );
    }


    bool
    QNAP2_Document::hasAttribute( LQX::VariableExpression * variable ) const
    {
	return variable != nullptr && _attributes.find( variable->getName() ) != _attributes.end();
    }

    /*
     * Append exec statement to the program
     */
    
    std::vector<LQX::SyntaxTreeNode *>*
    QNAP2_Document::setProgram( LQX::SyntaxTreeNode * statement )
    {
	if ( statement != nullptr ) {
	    _program.push_back( statement );
	}
	return &_program;
    }


    /*
     * Return true is there are any transit out from "from, chain";
     */
    
    size_t
    QNAP2_Document::countOutgoingTransits( const std::string& from, const std::string& chain ) const
    {
	std::map<std::string,std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>>::const_iterator m1 = _transit.find(from);
	if ( m1 == _transit.end() ) throw std::logic_error( std::string( "Missing from station: " ) + from );
	if ( m1->second.find(chain) == m1->second.end() ) return 0;
	std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>::const_iterator k = m1->second.find(chain);
	if ( k == m1->second.end() ) return 0;
	const std::map<std::string,LQX::SyntaxTreeNode *>& transits = k->second;
	return transits.size();
    }


    /*
     * Return true it there are any transits from any station (including "to") to "to".
     */
    
    size_t
    QNAP2_Document::countIncommingTransits( const std::string& to, const std::string& chain ) const
    {
	size_t count = 0;
	for ( std::map<std::string,std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>>::const_iterator m1 = _transit.begin(); m1 != _transit.end(); ++m1 ) {
	    std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>::const_iterator k = m1->second.find(chain);
	    if ( k == m1->second.end() ) continue;	/* No chain from this from station */
	    std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator m2 = k->second.find(to);
	    if ( m2 != k->second.end() ) count += 1;
	}
	return count;
    }
    
    const std::map<std::string,LQX::SyntaxTreeNode *>&
    QNAP2_Document::getTransits( const std::string& from, const std::string& chain ) const
    {
	std::map<std::string,std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>>::const_iterator m1 = _transit.find(from);
	if ( m1 == _transit.end() ) throw std::logic_error( std::string( "Missing from station: " ) + from );
	std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>::const_iterator k = m1->second.find(chain);
	if ( k == m1->second.end() ) throw std::logic_error( std::string( "Missing chain: " ) + from );
	return k->second;
    }

    /*
     * The output printer is actually the qnap2 function "output", so just redirect.
     */
    
    std::ostream&
    QNAP2_Document::print( std::ostream& output ) const
    {
	std::vector<LQX::SymbolAutoRef> args;
	getLQXEnvironment()->invokeGlobalMethod( "output", &args );
	return output;
    }

    void
    QNAP2_Document::registerMethods()
    {
	LQX::MethodTable * method_table = getLQXEnvironment()->getMethodTable();
	method_table->registerMethod(new BCMP::Mbusypct(model()));
	method_table->registerMethod(new BCMP::Mcustnb(model()));
	method_table->registerMethod(new BCMP::Mresponse(model()));
	method_table->registerMethod(new BCMP::Mservice(model()));
	method_table->registerMethod(new BCMP::Mthruput(model()));
	method_table->registerMethod(new Print);
	method_table->registerMethod(new Output(model()));
    }
}

namespace QNIO {
    /* ------------------------------------------------------------------------ */
    /*                            LQX Functions                                 */
    /* ------------------------------------------------------------------------ */

    LQX::SymbolAutoRef
    QNAP2_Document::Print::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef>& args)
    {
	for ( std::vector<LQX::SymbolAutoRef>::iterator item = args.begin(); item != args.end(); ++item ) {
	    std::cout << *item;
	}
	std::cout << std::endl;
	return LQX::Symbol::encodeNull();
    }


    LQX::SymbolAutoRef
    QNAP2_Document::Output::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef>& args)
    {
	print( std::cout );
	return LQX::Symbol::encodeNull();
    }

    std::streamsize QNAP2_Document::Output::__width = 10;
    std::streamsize QNAP2_Document::Output::__precision = 5;
    std::string QNAP2_Document::Output::__separator = "*";

    std::ostream&
    QNAP2_Document::Output::print( std::ostream& output ) const
    {
	const std::streamsize old_precision = output.precision(__precision);
//    output << " - (" << _solver << ") - " << std::endl;
	output.fill('*');
	output << std::setw(__width*6+7) << "*" << std::endl;
	output.fill(' ');
	output.setf(std::ios::left, std::ios::adjustfield);
	output << __separator << " " << std::setw(__width-1) << "name" << header() << __separator << std::endl;
	output.fill('*');
	output << std::setw(__width*6+7) << "*" << std::endl;
	output.fill(' ');
	output << __separator << std::setw(__width) << " ";
	output.setf(std::ios::right, std::ios::adjustfield);
	output << QNAP2_Document::Output::blankline() << __separator << std::endl;
	for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	    const BCMP::Model::Station::Class::map_t& results = mi->second.classes();
	    if ( results.empty() ) continue;
	    const BCMP::Model::Station::Class sum = std::accumulate( std::next(results.begin()), results.end(), results.begin()->second, &BCMP::Model::Station::sumResults );
	
	    /* Sum will work for single class too. */
	    output.setf(std::ios::left, std::ios::adjustfield);
	    output << __separator << std::setw(__width) << ( " " + mi->first );
	    print( output, sum );
	    output << __separator << std::endl;
	    if ( results.size() > 1 ) {
		for ( BCMP::Model::Station::Class::map_t::const_iterator result = results.begin(); result != results.end(); ++result ) {
		    if (result->second.throughput() == 0 ) continue;
		    output << __separator << std::setw(__width) <<  ( "(" + result->first + ")");
		    print( output, result->second );
		    output << __separator << std::endl;
		}
	    }
	    output << __separator << std::setw(__width) << " " << QNAP2_Document::Output::blankline() << __separator << std::endl;
	}
	output.fill('*');
	output << std::setw(__width*6+7) << "*" << std::endl;
	output.fill(' ');
	output.precision(old_precision);
	return output;
    }
    
    std::ostream&
    QNAP2_Document::Output::print( std::ostream& output, const BCMP::Model::Station::Result& item ) const
    {
	output.unsetf( std::ios::floatfield );
	output << __separator << " " << std::setw(__width-1) << item.mean_service()
	       << __separator << " " << std::setw(__width-1) << item.utilization()
	       << __separator << " " << std::setw(__width-1) << item.queue_length()
	       << __separator << " " << std::setw(__width-1) << item.residence_time()		// per visit.
	       << __separator << " " << std::setw(__width-1) << item.throughput();
	return output;
    }


    std::string
    QNAP2_Document::Output::header()
    {
	std::ostringstream output;
	output << __separator << std::setw(__width) << "service "
	       << __separator << std::setw(__width) << "busy pct "
	       << __separator << std::setw(__width) << "cust nb "
	       << __separator << std::setw(__width) << "response "
	       << __separator << std::setw(__width) << "thruput ";
	return output.str();
    }

    std::string
    QNAP2_Document::Output::blankline()
    {
	std::ostringstream output;
	for ( unsigned i = 0; i < 5; ++i ) {
	    output << std::setfill(' ');
	    output << __separator << std::setw(__width) << " ";
	}
	return output.str();
    }
}

namespace QNIO {
    /* ------------------------------------------------------------------------ */
    /*                            QNAP2 Export                                  */
    /* ------------------------------------------------------------------------ */

    /*
     * "&" is a comment.
     * identifiers are limited to the first 8 characters.
     * input is limited to 80 characters.
     * statements are terminated with ";"
     * If there are SPEX variables, then use those.
     */

    std::ostream&
    QNAP2_Document::exportModel( std::ostream& output ) const
    {
	std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );
	output << "& " << LQIO::DOM::Common_IO::svn_id() << std::endl;
	if ( LQIO::io_vars.lq_command_line.size() > 0 ) {
	    output << "& " << LQIO::io_vars.lq_command_line << std::endl;
	}


	/* Special stations for closed and open classes */
	bool has_closed_chain = false;
	bool has_open_chain = false;
	std::string customers;		/* Special station names */
	BCMP::Model::Station::map_t::const_iterator customer = std::find_if( stations().begin(), stations().end(), &BCMP::Model::Station::isCustomer );
	if ( customer != stations().end() ) {
	    has_closed_chain = true;
	    const std::string name = customer->first;
	    model().computeCustomerDemand( name );	/* compute terminal service times based on visits */
	    customers = name;
	}
	BCMP::Model::Chain::map_t::const_iterator chain = std::find_if( chains().begin(), chains().end(), &BCMP::Model::Chain::openChain );
	if ( chain != chains().end() ) {
	    has_open_chain = true;
	    const std::string name = "source";
	    BCMP::Model::Station& source = const_cast<BCMP::Model&>(model()).insertStation( name, BCMP::Model::Station::Type::SOURCE ).first->second;
	    source.insertClass( chain->first, nullptr, chain->second.arrival_rate() );
	    if ( !customers.empty() ) customers += ',';
	    customers += name;
	}


	/* 1) Declare all SPEX variables */
	output << qnap2_keyword( "declare" ) << std::endl;
	std::set<const LQX::VariableExpression *> symbol_table;
	const std::string customer_vars = std::accumulate( chains().begin(), chains().end(), std::string(""), getIntegerVariables( model(), symbol_table ) );
	if ( !customer_vars.empty() )   output << qnap2_statement( "integer " + customer_vars, "Customer vars." ) << std::endl;
	const std::string integer_vars  = std::accumulate( stations().begin(), stations().end(), std::string(""), getIntegerVariables( model(), symbol_table ) );
	if ( !integer_vars.empty() )    output << qnap2_statement( "integer " + integer_vars, "Multiplicity var." ) << std::endl;
	const std::string real_vars    	= std::accumulate( stations().begin(), stations().end(), std::string(""), getRealVariables( model(), symbol_table ) );
	if ( !real_vars.empty() )       output << qnap2_statement( "real " + real_vars, "Service time vars." ) << std::endl;
	std::set<std::string> symbol_table2;
	const std::string result_vars  	= std::accumulate( stations().begin(), stations().end(), std::string(""), getResultVariables( model(), symbol_table2 ) );
	if ( !result_vars.empty() )     output << qnap2_statement( "real " + result_vars, "Result vars." ) << std::endl;

	/* 2) Declare all stations */
	output << qnap2_statement( "queue " + std::accumulate( stations().begin(), stations().end(), customers, fold_station() ), "Station identifiers" ) << std::endl;

	/* 3) Declare the chains */
	if ( !multiclass() ) {
	    if ( has_closed_chain ) {
		output << qnap2_statement( "integer n_users", "Population" ) << std::endl
		       << qnap2_statement( "real think_t", "Think time." ) << std::endl;
	    }
	    if ( has_open_chain ) {
		output << qnap2_statement( "real arrivals", "Arrival Rate." ) << std::endl;
	    }
	    output << qnap2_statement( "real " + std::accumulate( stations().begin(), stations().end(), std::string(""), fold_station( "_t") ), "Station service time" ) << std::endl;
	} else {
	    /* Variables */
	    output << qnap2_statement( "class string name", "Name (for output)" ) << std::endl;		// LQN client.
	    if ( has_closed_chain ) {
		output << qnap2_statement( "class integer n_users", "Population." ) << std::endl
		       << qnap2_statement( "class real think_t", "Think time." ) << std::endl;
	    }
	    if ( has_open_chain ) {
		output << qnap2_statement( "real arrivals", "Arrival Rate." ) << std::endl;
	    }
	    output << qnap2_statement( "class real " + std::accumulate( stations().begin(), stations().end(), std::string(""), fold_station( "_t") ), "Station service time" ) << std::endl;
	    /* Chains */
	    output << qnap2_statement( "class " + std::accumulate( chains().begin(), chains().end(), std::string(""), BCMP::Model::Chain::fold() ), "Class names" ) << std::endl;
	}

	/* 4) output the statations */
	std::for_each( stations().begin(), stations().end(), printStation( output, model() ) );		// Stations.

	/* Output control stuff if necessary */
	if ( multiclass() || !result_vars.empty() ) {
	    output << "&" << std::endl
		   << qnap2_keyword( "control" ) << std::endl;
	    if ( multiclass() ) {
		output << qnap2_statement( "class=all queue", "Compute for all classes" ) << std::endl;	// print for all classes.
	    }
	    if ( !result_vars.empty() ) {
		output << qnap2_statement( "option=nresult", "Suppress default output" ) << std::endl;
	    }
	}

	/* 6) Finally, assign the parameters defined in steps 2 & 3 from the constants and variables in the model */

	output << "&" << std::endl
	       << qnap2_keyword( "exec" ) << std::endl
	       << "   begin" << std::endl;

	if ( !result_vars.empty() ) {
	    printResults( output, "\"", result_vars, "\"" );
	}

	/* Insert QNAP for statements for arrays and completions. */
	if ( !comprehensions().empty() ) {
	    output << "&  -- Comprehensions --" << std::endl;
	    std::for_each( comprehensions().begin(), comprehensions().end(), for_loop( output ) );
	}

	output << "&  -- Class variables --" << std::endl;
        printClassVariables( output );

	output << "&  -- Station variables --" << std::endl;
	std::for_each( stations().begin(), stations().end(), printStationVariables( output, model() ) );

	/* Let 'er rip! */
	output << "&  -- Let 'er rip! --" << std::endl;
	output << qnap2_statement( "solve" ) << std::endl;
	if ( !result_vars.empty() ) {
	    output << "&  -- SPEX results for QNAP2 solutions are converted to" << std::endl
		   << "&  -- the LQN output for throughput, service and waiting time." << std::endl
		   << "&  -- QNAP2 throughput for a reference task is per-slice," << std::endl
		   << "&  -- and not the aggregate so divide by the number of transits." << std::endl
		   << "&  -- Service time is mservice() + sum of mresponse()." << std::endl
		   << "&  -- Waiting time is mresponse() - mservice()." << std::endl;
	    printResults( output, "", result_vars, "" );
	}

	/* insert end's for each for. */
	std::for_each( comprehensions().rbegin(), comprehensions().rend(), end_for( output ) );

	/* End of program */
	output << qnap2_statement( "end" ) << std::endl;
        output.flags(flags);

	output << qnap2_keyword( "end" ) << std::endl;

	return output;
    }

    /*
     * Collect all variables.  Only add an itemm to the string if it's
     * a variable (the name), and the variable has not been seen
     * before (insert will return true).
     */

    std::string
    QNAP2_Document::getIntegerVariables::operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m ) const
    {
	std::string s = s1;
	const LQX::VariableExpression * copies = dynamic_cast<const LQX::VariableExpression *>(m.second.copies());
	if ( BCMP::Model::Station::isServer(m) && copies != nullptr && _symbol_table.insert(copies).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += copies->getName();
	}
	return s;
    }
    
    std::string
    QNAP2_Document::getRealVariables::operator()( const std::string& s1, const BCMP::Model::Station::Class::pair_t& demand ) const
    {
	std::string s = s1;
	const LQX::VariableExpression * service_time = dynamic_cast<const LQX::VariableExpression *>(demand.second.service_time());
	if ( service_time != nullptr && _symbol_table.insert(service_time).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += service_time->getName();
	}
	return s;
    }


    /*
     * Think times. Only add an item to the string if it's a variable (the name), and the variable
     * has not been seen before.
     */

    std::string
    QNAP2_Document::getIntegerVariables::operator()( const std::string& s1, const BCMP::Model::Chain::pair_t& k ) const
    {
	std::string s = s1;
	const LQX::VariableExpression * customers = dynamic_cast<LQX::VariableExpression *>(k.second.customers());
	if ( k.second.isClosed() && customers != nullptr && _symbol_table.insert(customers).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += customers->getName();
	}
	return s;
    }

    /*
     *
     */
    
    std::string
    QNAP2_Document::getRealVariables::operator()( const std::string& s1, const BCMP::Model::Chain::pair_t& k ) const
    {
	std::string s = s1;
	const LQX::VariableExpression * think_time = dynamic_cast<const LQX::VariableExpression *>(k.second.think_time());
	const LQX::VariableExpression * arrival_rate = dynamic_cast<const LQX::VariableExpression *>(k.second.arrival_rate());
	if ( k.second.isClosed() && think_time != nullptr && _symbol_table.insert(think_time).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += think_time->getName();
	} else if ( k.second.isOpen() && arrival_rate != nullptr && _symbol_table.insert(arrival_rate).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += arrival_rate->getName();
	}
	return s;
    }

    std::string
    QNAP2_Document::getRealVariables::operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m ) const
    {
	
	const BCMP::Model::Station::Class::map_t& classes = m.second.classes();
	return std::accumulate( classes.begin(), classes.end(), s1, getRealVariables( model(), _symbol_table ) );
    }

    /*
     *
     */
    
    std::string
    QNAP2_Document::getResultVariables::operator()( const std::string& s1, const BCMP::Model::Chain::pair_t& k ) const
    {
	return s1;
    }

    
    std::string
    QNAP2_Document::getResultVariables::operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m ) const
    {
	/* Get results per class */
	const BCMP::Model::Station::Class::map_t& classes = m.second.classes();
	std::string s = std::accumulate( classes.begin(), classes.end(), s1, getResultVariables( model(), _symbol_table ) );

	/* Get station results */
	const BCMP::Model::Station::Result::map_t& results = m.second.resultVariables();
	for ( BCMP::Model::Station::Result::map_t::const_iterator result = results.begin(); result != results.end(); ++result ) {
	    const std::string& name = result->second;
	    if ( _symbol_table.insert(name).second == true ) {
		if ( !s.empty() ) s += ",";
		s += name;
	    }
	}

	return s;
	
    }



    std::string
    QNAP2_Document::getResultVariables::operator()( const std::string& s1, const BCMP::Model::Station::Class::pair_t& k ) const
    {
	std::string s = s1;
	const BCMP::Model::Station::Class::Result::map_t& results = k.second.resultVariables();
	for ( BCMP::Model::Station::Class::Result::map_t::const_iterator result = results.begin(); result != results.end(); ++result ) {
	    const std::string& name = result->second;
	    if ( _symbol_table.insert(name).second == true ) {
		if ( !s.empty() ) s += ",";
		s += name;
	    }
	}
	return s;
    }

    /*
     * Prints stations.
     */

    void
    QNAP2_Document::printStation::operator()( const BCMP::Model::Station::pair_t& m ) const
    {
	const BCMP::Model::Station& station = m.second;
	_output << "&" << std::endl
		<< "/station/ name=" << m.first << ";" << std::endl;
	switch ( station.type() ) {
	case BCMP::Model::Station::Type::SOURCE:
	    _output << qnap2_statement( "type=source" ) << std::endl;
	    printInterarrivalTime();
	    printCustomerTransit();
	    return;
	case BCMP::Model::Station::Type::DELAY:
	    _output << qnap2_statement( "type=" + __scheduling_str.at(SCHEDULE_DELAY) ) << std::endl;
	    break;
	case BCMP::Model::Station::Type::LOAD_INDEPENDENT:
	case BCMP::Model::Station::Type::MULTISERVER:
	    try {
		_output << qnap2_statement( "sched=" + __scheduling_str.at(station.scheduling()) ) << std::endl;
	    }
	    catch ( const std::out_of_range& ) {
		LQIO::runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED,
				     scheduling_label.at(station.scheduling()).str.c_str(),
				     "station",
				     m.first.c_str() );
	    }
	    if ( station.type() == BCMP::Model::Station::Type::MULTISERVER ) {
		_output << qnap2_statement( "type=multiple(" + to_unsigned(station.copies()) + ")" ) << std::endl;
	    } 
	    break;
	case BCMP::Model::Station::Type::NOT_DEFINED:
	    throw std::range_error( "QNAP2_Document::printStation::operator(): Undefined station type." );
	}

	/* Print out service time variables and visits for non-special stations */

	if ( station.reference() ) {
	    _output << qnap2_statement( "init=n_users", "Population by class" ) << std::endl
		    << qnap2_statement( "service=exp(think_t)" ) << std::endl;
	    printCustomerTransit();
	} else {
	    _output << qnap2_statement( "service=exp(" + m.first + "_t)" ) << std::endl;
	    printServerTransit( m );
	}
    }

    void
    QNAP2_Document::printStation::printCustomerTransit() const
    {
	if ( !multiclass() ) {
	    std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_transit(chains().begin()->first) );
	    if ( visits.empty() ) return;
	    _output << qnap2_statement("transit=" + visits, "visits to servers" ) << std::endl;
	} else {
	    for ( BCMP::Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
		std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_transit(k->first) );
		if ( visits.empty() ) continue;
		_output << qnap2_statement("transit(" + k->first + ")=" + visits, "visits to servers" ) << std::endl;
	    }
	}
    }

    /*
     * If I have mixed models, then I have to transit by class
     */

    void
    QNAP2_Document::printStation::printServerTransit( const BCMP::Model::Station::pair_t& m ) const
    {
	const BCMP::Model::Station::Class::map_t& classes = m.second.classes();
	const std::string closed_classes = std::accumulate( classes.begin(), classes.end(), std::string(), fold_class( chains(), BCMP::Model::Chain::Type::CLOSED ) );
	const std::string open_classes = std::accumulate( classes.begin(), classes.end(), std::string(), fold_class( chains(), BCMP::Model::Chain::Type::OPEN ) );
	const BCMP::Model::Station::map_t::const_iterator terminal = std::find_if( stations().begin(), stations().end(), &BCMP::Model::Station::isCustomer );

	if ( !closed_classes.empty() & !open_classes.empty() ) {
	    _output << qnap2_statement( "transit( " + closed_classes + ")=" + terminal->first );
	    _output << qnap2_statement( "transit( " + open_classes + ")=out" );
	} else {
	    std::string name;
	    if ( terminal != stations().end() ) name = terminal->first;
	    else name = "out";
	    if ( multiclass() ) {
		_output << qnap2_statement( "transit(all class)=" + name ) << std::endl;
	    } else {
		_output << qnap2_statement( "transit=" + name ) << std::endl;
	    }
	}
    }

    /*
     * Convert arrival rate to inter-arrival time, then adjust for the
     * fact that the visits for open chains are converted to a
     * probability in QNAP2 by multipling the rate by the total visits
     * to all open classes.
     */

    void
    QNAP2_Document::printStation::printInterarrivalTime() const
    {
	if ( !multiclass() ) {
	    const std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( chains().begin()->first ) );
	    _output << qnap2_statement( "service=exp(1./((" + visits + ")*arrivals))", "Convert to inter-arrival time" ) << std::endl;
	} else {
	    for ( BCMP::Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
		const std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( k->first ) );
		_output << qnap2_statement( "service=(" + k->first + ")=exp(1./((" + visits + ")*arrivals))", "Convert to inter-arrival time." ) << std::endl;
	    }
	}
    }

    std::string
    QNAP2_Document::fold_transit::operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m2 ) const
    {
	std::string s = s1;
	const BCMP::Model::Station& station = m2.second;
	LQX::SyntaxTreeNode * visits = station.classAt(_name).visits();
	if ( !station.reference() && station.hasClass(_name) && !BCMP::Model::isDefault(visits) ) {
	    if ( !s.empty() ) s += ",";
	    s += m2.first + "," + to_unsigned(visits);
	}
	return s;
    }

    LQX::SyntaxTreeNode * QNAP2_Document::fold_transit::operator()( const LQX::SyntaxTreeNode* t1, const std::pair<std::string,LQX::SyntaxTreeNode *>& t2 ) const
    {
	return BCMP::Model::add( const_cast<LQX::SyntaxTreeNode *>(t1), t2.second );
    }


    void
    QNAP2_Document::printClassVariables( std::ostream& output ) const
    {
	/*
	 * Sum service time over all clients and visits over all
	 * servers.  The demand at the reference station is the demand
	 * (service time) at the reference station but the visits over
	 * all non-customer stations.
	 */

	const BCMP::Model::Station::Class::map_t visits = std::accumulate( stations().begin(), stations().end(), BCMP::Model::Station::Class::map_t(), BCMP::Model::Station::select( &BCMP::Model::Station::isServer ) );
	const BCMP::Model::Station::Class::map_t service_times = std::accumulate( stations().begin(), stations().end(), BCMP::Model::Station::Class::map_t(), BCMP::Model::Station::select( &BCMP::Model::Station::isCustomer ) );
	BCMP::Model::Station::Class::map_t classes = std::accumulate( service_times.begin(), service_times.end(), BCMP::Model::Station::Class::map_t(), BCMP::Model::sum_visits(visits) );

	/*
	 * QNAP Does everything by "transit", so the service time at
	 * the terminal station has to be adjusted by the number of
	 * visits to all other stations.  Let QNAP do it as it's
	 * easier to see the values from the origial file. Force
	 * floating point math.
	 */

	for ( BCMP::Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
	    if ( !k->second.isClosed() ) continue;
	    std::string comment;
	    LQX::SyntaxTreeNode * think_time = classes.at(k->first).service_time();
	    LQX::SyntaxTreeNode * customers = k->second.customers();
	    if ( dynamic_cast<LQX::VariableExpression *>(k->second.customers()) ) {
		comment = "SPEX variable " + dynamic_cast<LQX::VariableExpression *>(k->second.customers())->getName();
	    }

	    if ( multiclass() ) {
		output << qnap2_statement( k->first + ".name:=\"" + k->first + "\"", "Class (client) name" ) << std::endl;
		output << qnap2_statement( k->first + ".n_users:=" + to_unsigned(customers), comment  ) << std::endl;
	    } else {
		output << qnap2_statement( "n_users:=" + to_unsigned(customers), comment  ) << std::endl;
	    }

	    const std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( k->first ) );
	    if ( !think_visits.empty() ) {
		if ( multiclass() ) {
		    output << qnap2_statement( k->first + ".think_t:=" + to_real(think_time) + "/(" + think_visits + ")", "Slice time at client" ) << std::endl;
		} else {
		    output << qnap2_statement( "think_t:=" + to_real(think_time) + "/(" + think_visits + ")", "Slice time at client" ) << std::endl;
		}
	    }
	}
    }


    /*
     * Print out all services times.  Since they are all external
     * variables *var will print either the value or the name of the
     * variable.
     */

    void
    QNAP2_Document::printStationVariables::operator()( const BCMP::Model::Station::pair_t& m ) const
    {
	const BCMP::Model::Station& station = m.second;
	for ( BCMP::Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
	    std::string name;

	    if ( station.type() == BCMP::Model::Station::Type::SOURCE ) {
		name = "arrivals";
	    } else if ( station.reference() ) {
		continue;
	    } else {
		name = m.first + "_t";
	    }

	    std::string comment;
	    LQX::SyntaxTreeNode * service_time = station.classAt(k->first).service_time();
	    
	    if ( !station.hasClass(k->first) || BCMP::Model::isDefault( service_time ) ) {
		comment = "QNAP does not like zero (0)";
	    } else {
		if ( dynamic_cast<LQX::VariableExpression *>(service_time) != nullptr ) {
		    comment = "SPEX variable " + dynamic_cast<LQX::VariableExpression *>(service_time)->getName();
		}
	    }
	    if ( multiclass() ) {
		_output << qnap2_statement( k->first + "." + name + ":=" + to_real(service_time), comment ) << std::endl;
	    } else {
		_output << qnap2_statement( name + ":=" + to_real(service_time), comment ) << std::endl;
	    }
	}
    }


    /*
     * Print out the result variables in chunks as QNAP2 doesn't like BIG print statements.
     */

    void
    QNAP2_Document::printResults( std::ostream& output, const std::string& prefix, const std::string& variables, const std::string& postfix ) const
    {
	std::string comment = "Results";
	std::string s;
	bool continuation = false;
	size_t count = 0;
	const char delim = ',';		/* Tokeninze the input string on ',' */
	size_t start;
	size_t finish = 0;
	while ( (start = variables.find_first_not_of( delim, finish )) != std::string::npos ) {
	    finish = variables.find(delim, start);
	    const std::string name = variables.substr( start, finish - start  );
	    ++count;
	    if ( s.empty() && continuation ) s += "\",\",";	/* second print statement, signal continuation with "," */
	    else if ( !s.empty() ) s += ",\",\",";		/* between vars. */
	    s += name;
	    if ( count > 6 ) {
		output << qnap2_statement( "print(" + s + ")", comment ) << std::endl;
		s.clear();
		count = 0;
		continuation = true;
		comment = "... continued";
	    }
	}
	if ( !s.empty() ) {
	    output << qnap2_statement( "print(" + s + ")", comment ) << std::endl;
	}
    }


    /*
     * Generate for loops;
     */

    void
    QNAP2_Document::for_loop::operator()( const Comprehension& comprehension ) const
    {
	_output << "   " << "for "<< comprehension.name() << ":=" << comprehension.begin() << " step " << comprehension.step() << " until " << comprehension.max() << " do begin" << std::endl;
    }

    /*
     * Sequence through the stations and classes looking for the result variables.
     */

    /* mservice, mbusypct, mcustnb, vcustnb, mresponse, mthruput, custnb */


    void
    QNAP2_Document::getObservations::operator()( const std::pair<const std::string,LQX::SyntaxTreeNode *>& var ) const
    {
	static const std::map<const BCMP::Model::Result::Type,QNAP2_Document::getObservations::f> key_map = {
	    { BCMP::Model::Result::Type::QUEUE_LENGTH,   &getObservations::get_waiting_time },
	    { BCMP::Model::Result::Type::RESIDENCE_TIME, &getObservations::get_service_time },
	    { BCMP::Model::Result::Type::THROUGHPUT,     &getObservations::get_throughput },
	    { BCMP::Model::Result::Type::UTILIZATION,    &getObservations::get_utilization }
	};

	for ( BCMP::Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {

	    /* Check for station results */

	    const BCMP::Model::Result::map_t& station_variables = m->second.resultVariables();
	    for ( BCMP::Model::Result::map_t::const_iterator r = station_variables.begin(); r != station_variables.end(); ++r ) {
		if ( r->second != var.first ) continue;
		const std::map<const BCMP::Model::Result::Type,f>::const_iterator key = key_map.find( r->first );
		if ( key != key_map.end() ) {
		    std::pair<std::string,std::string> expression;
		    expression = (this->*(key->second))( m->first, std::string() );
		    _output << qnap2_statement( var.first + ":=" + expression.first, expression.second ) << std::endl;
		}
	    }

	    /* Check for class results */

	    for ( BCMP::Model::Station::Class::map_t::const_iterator k = m->second.classes().begin(); k != m->second.classes().end(); ++k ) {
		const BCMP::Model::Result::map_t& class_variables = k->second.resultVariables();
		for ( BCMP::Model::Result::map_t::const_iterator r = class_variables.begin(); r != class_variables.end(); ++r ) {
		    if ( r->second != var.first ) continue;
		    const std::map<const BCMP::Model::Result::Type,f>::const_iterator key = key_map.find( r->first );
		    if ( key != key_map.end() ) {
			std::pair<std::string,std::string> expression;
			expression = (this->*(key->second))( m->first, k->first );
			_output << qnap2_statement( var.first + ":=" + expression.first, expression.second ) << std::endl;
		    }
		}
	    }
	}
    }

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_throughput( const std::string& station_name, const std::string& class_name ) const
    {
	std::string result;
	std::string comment;
	const BCMP::Model::Station& station = stations().at(station_name);
	result = "mthruput(" + station_name;
	if ( station.reference() ) {
	    /* Report class results for the customers; the station name is the class name */
	    if ( multiclass() ) result += "," + class_name;
	    result += ")";
	    const std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( class_name ) );
	    if ( !think_visits.empty() ) result += "/(" + think_visits + ")";
	    comment = "Convert to LQN throughput";
	} else {
	    if ( !class_name.empty() ) result += "," + class_name;
	    result += ")";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * Derive for a multiserver.  QNAP gives odd numbers.
     */
    
    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_utilization( const std::string& station_name, const std::string& class_name ) const
    {
	std::string result;
	std::string comment;
	const BCMP::Model::Station& station = stations().at(station_name);
	if ( station.type() == BCMP::Model::Station::Type::MULTISERVER ) {
	    result = "mthruput";
	} else {
	    result = "mbusypct";
	}
	result += "(" + station_name;
	if ( !class_name.empty() ) result += "," + class_name;
	result += ")";
	if ( station.type() == BCMP::Model::Station::Type::MULTISERVER ) {
	    result += "*(";
	    if ( !class_name.empty() ) result += to_real(station.classAt(class_name).service_time());
	    else {
		const BCMP::Model::Station::Class::map_t classes = station.classes();
		result += to_real(classes.begin()->second.service_time());
	    }
	    result += ")";
	    comment = "Dervived for multiserver.";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * KEY_SERVICE is mservice + sum_of mresponse
     */

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_service_time( const std::string& station_name, const std::string& class_name ) const
    {
	std::string result;
	std::string comment;
	const BCMP::Model::Station& station = stations().at(station_name);
	if ( station.reference() ) {
	    result =  "mservice(" + station_name;
	    if ( multiclass() ) result += "," + class_name;
	    result += ")";
	    const std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( class_name ) );
	    if ( !visits.empty() ) result += "*(" + visits + ")";	// Over all visits
	    const std::string response = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_mresponse( class_name, chains() ) );
	    if ( !response.empty() ) result += "+(" + response +")";
	    comment = "Convert to LQN service time";
	} else {
	    result =  "mresponse(" + station_name;
	    if ( multiclass() && !class_name.empty() ) result += "," + class_name;
	    result += ")";
	    comment = "Convert to LQN service time.";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * KEY_WAITING is mresponse-mservice.
     */

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_waiting_time( const std::string& station_name, const std::string& class_name  ) const
    {
	std::string result;
	std::string comment;
	result = "mresponse(" + station_name + ")-mservice(" + station_name;
	if ( multiclass() && !class_name.empty() ) result += "," + class_name;
	result += ")";
	comment = "Convert to LQN queueing time";
	return std::pair<std::string,std::string>(result,comment);
    }

    void
    QNAP2_Document::end_for::operator()( const Comprehension& comprehension ) const
    {
	std::string comment = "for " + comprehension.name();
	std::replace( comment.begin(), comment.end(), '$', '_'); 	// Make variables acceptable for QNAP2.
	_output << qnap2_statement( "end", comment ) << std::endl;
    }

    std::string
    QNAP2_Document::fold_station::operator()( const std::string& s1, const BCMP::Model::Station::pair_t& s2 ) const
    {
	if ( s2.second.reference() || s2.second.type() == BCMP::Model::Station::Type::SOURCE ) return s1;
	else if ( s1.empty() ) {
	    return s2.first + _suffix;
	} else {
	    return s1 + "," + s2.first + _suffix;
	}
    }

    std::string
    QNAP2_Document::fold_class::operator()( const std::string& s1, const BCMP::Model::Station::Class::pair_t& k2 ) const
    {
	if ( _chains.at(k2.first).type() != _type ) return s1;
	else if ( s1.empty() ) {
	    return k2.first;
	} else {
	    return s1 + "," + k2.first;
	}
    }

    std::string
    QNAP2_Document::fold_visits::operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m2 ) const
    {
	const BCMP::Model::Station& station = m2.second;
	if ( !station.hasClass(_name) || station.reference() ) return s1;	/* Don't visit self */
	LQX::SyntaxTreeNode * visits = station.classAt(_name).visits();
	if ( BCMP::Model::isDefault( visits ) ) return s1;	/* ignore zeros */
	std::string s2 = to_unsigned( visits );
	if ( s1.empty() ) {
	    return s2;
	} else {
	    return s1 + "+" + s2;
	}
    }

    std::string
    QNAP2_Document::fold_mresponse::operator()( const std::string& s1, const BCMP::Model::Station::pair_t& m2 ) const
    {
	const BCMP::Model::Station& station = m2.second;
	if ( !station.hasClass(_name) || station.reference() ) return s1;	/* Don't visit self */
	std::string s2 = "mresponse(" + m2.first;
	if ( multiclass() ) {
	    s2 += "," + _name;
	}
	s2 += ")";
	if ( s1.empty() ) {
	    return s2;
	} else {
	    return s1 + "+" + s2;
	}
    }


    /*
     * Output either the name of the variable, or a real
     * constant for QNAP2 (append a . if none found).
     */

    std::string
    QNAP2_Document::to_real( LQX::SyntaxTreeNode* v )
    {
	std::string str;
	if ( dynamic_cast<LQX::VariableExpression *>(v) ) {
	    str = dynamic_cast<LQX::VariableExpression *>(v)->getName();
	} else {
	    char buf[16];
	    snprintf( buf, 15, "%g", v->invoke(nullptr)->getDoubleValue() );
	    str = buf;
	    if ( str.find( '.' ) == std::string::npos ) {
		str += ".";	/* Force real */
	    }
	}
	return str;
    }

    std::string
    QNAP2_Document::to_unsigned( LQX::SyntaxTreeNode* v )
    {
	std::string str;
	if ( dynamic_cast<LQX::VariableExpression *>(v) ) {
	    str = dynamic_cast<LQX::VariableExpression *>(v)->getName();
	} else {
	    char buf[16];
	    snprintf( buf, 15, "%g", v->invoke(nullptr)->getDoubleValue() );
	    str = buf;
	    if ( str.find( '.' ) != std::string::npos ) {
		throw std::domain_error( "Invalid integer" );
	    }
	}
	return str;
    }

    /* static */ std::ostream&
    QNAP2_Document::printKeyword( std::ostream& output, const std::string& s1, const std::string& s2 )
    {
	output << "/" << s1 << "/";
	if ( !s2.empty() ) {
	    output << " " << s2 << ";";
	}
	return output;
    }

    /*
     * Format for QNAP output.  Swap all $ to _. I may have to line
     * wrap as lines are limited to 80 characters (fortran).  If s2 is
     * present, it's a comment.
     */

    /* static */ std::ostream&
    QNAP2_Document::printStatement( std::ostream& output, const std::string& s1, const std::string& s2 )
    {
	bool first = true;
	std::string::const_iterator brk = s1.end();
	for ( std::string::const_iterator begin = s1.begin(); begin != s1.end(); begin = brk ) {
	    bool in_quote = false;

	    if ( s1.end() - begin < 60 ) {
		brk = s1.end();
	    } else {
		/* look for ',' not in quotes. */
		for ( std::string::const_iterator curr = begin; curr - begin < 60 && curr != s1.end(); ++curr ) {
		    if ( *curr == '"' ) in_quote = !in_quote;
		    if ( !in_quote && (*curr == ',' || *curr == '+' || *curr == '*' || *curr == ')' ) ) brk = curr;
		}
	    }

	    /* Take everthing up to brk and make variables variables acceptable for QNAP2. */
	    std::string buffer( brk - begin + (first ? 0 : 1), ' ' );	/* reserve space */
	    std::replace_copy( begin, brk, (first ? buffer.begin(): std::next(buffer.begin())), '$', '_');
//	    std::replace_copy( begin, brk, std::back_inserter<std::string>(buffer.begin()), '$', '_');

	    /* Output the buffer */
	    output << "   ";
	    if ( brk != s1.end() ) {
		output << buffer << std::endl;		/* break the line 	*/
	    } else if ( s2.empty() ) {
		output << buffer << ";";		/* Don't fill 		*/
	    } else {
		output << std::setw(60) << (buffer + ';') << "& " << s2;
	    }
	    first = false;
	}
	return output;
    }
}
