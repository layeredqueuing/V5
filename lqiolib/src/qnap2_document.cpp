/* -*- c++ -*-
 * $Id: qnap2_document.cpp 15982 2022-10-16 20:52:42Z greg $
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
#include "common_io.h"
#include "dom_document.h"
#include "error.h"
#include "filename.h"
#include "glblerr.h"
#include "input.h"
#include "qnap2_document.h"
#include "srvn_spex.h"

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
	{ QNAP_STRING, QNIO::QNAP2_Document::Type::String },
	{ QNAP_INTEGER, QNIO::QNAP2_Document::Type::Integer },
	{ QNAP_REAL, QNIO::QNAP2_Document::Type::Real },
	{ QNAP_BOOLEAN, QNIO::QNAP2_Document::Type::Boolean },
    };
  
}

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


/*
 * Add each item in queue_list to model._chains, but don't populate.
 * That's done by the station.
 */

void
qnap2_declare_class( void * list )
{
    if ( list == nullptr ) return;
    std::vector<QNIO::QNAP2_Document::Variable *>* class_list = static_cast<std::vector<QNIO::QNAP2_Document::Variable *>*>(list);
    for ( std::vector<QNIO::QNAP2_Document::Variable *>::const_iterator variable = class_list->begin(); variable != class_list->end(); ++variable ) {
	if ( !QNIO::QNAP2_Document::__document->declareClass( **variable ) ) {
	    qnap2error( "duplicate class: %s.", (*variable)->name().c_str() );
	}
    }
}


void
qnap2_declare_field( int code, void * list )
{
    if ( list == nullptr ) return;
    const std::map<int,const QNIO::QNAP2_Document::Type>::const_iterator type = QNIO::QNAP2_Document::__parser_to_type.find(code);
    if ( type == QNIO::QNAP2_Document::__parser_to_type.end() ) {
	qnap2error( "type error." );
    }
    std::vector<QNIO::QNAP2_Document::Variable *>* variable_list = static_cast<std::vector<QNIO::QNAP2_Document::Variable *>*>(list);
    for ( std::vector<QNIO::QNAP2_Document::Variable *>::const_iterator variable = variable_list->begin(); variable != variable_list->end(); ++variable ) {
	if ( !QNIO::QNAP2_Document::__document->declareField( type->second, **variable ) ) {
	    qnap2error( "duplicate field: %s.", (*variable)->name().c_str() );
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
    std::vector<QNIO::QNAP2_Document::Variable *>* queue_list = static_cast<std::vector<QNIO::QNAP2_Document::Variable *>*>(list);
    for ( std::vector<QNIO::QNAP2_Document::Variable *>::const_iterator variable = queue_list->begin(); variable != queue_list->end(); ++variable ) {
	if ( !QNIO::QNAP2_Document::__document->declareStation( **variable ) ) {
	    qnap2error( "duplicate queue: %s.", (*variable)->name().c_str() );
	}
    }
}


void
qnap2_define_variable( int code, void * list )
{
    if ( list == nullptr ) return;
    const std::map<int,const QNIO::QNAP2_Document::Type>::const_iterator type = QNIO::QNAP2_Document::__parser_to_type.find(code);
    if ( type == QNIO::QNAP2_Document::__parser_to_type.end() ) abort();
    std::vector<QNIO::QNAP2_Document::Variable *>* variable_list = static_cast<std::vector<QNIO::QNAP2_Document::Variable *>*>(list);
    for ( std::vector<QNIO::QNAP2_Document::Variable *>::const_iterator variable = variable_list->begin(); variable != variable_list->end(); ++variable ) {
	if ( !QNIO::QNAP2_Document::__document->defineVariable( type->second, **variable ) ) {
	    qnap2error( "duplicate variable: %s.", (*variable)->name().c_str() );
	}
    }
}


void *
qnap2_declare_variable( const char * name, void * begin, void * end )
{
    return new QNIO::QNAP2_Document::Variable( std::string(name), QNIO::QNAP2_Document::Type::Undefined, static_cast<LQX::SyntaxTreeNode *>(begin), static_cast<LQX::SyntaxTreeNode *>(end) );
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
    QNIO::QNAP2_Document::__document->mapTransitToVisits();
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

/* checks for station name */
const char *
qnap2_get_station_name( const char * station_name )
{
    try {
	QNIO::QNAP2_Document::__document->getStation( station_name );
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
    return station_name;
    
}

void * qnap2_get_function( const char * arg1, void * arg2 )
{
    if ( arg2 != nullptr ) {
	return new LQX::MethodInvocationExpression(arg1, static_cast<LQX::SyntaxTreeNode *>(arg2));
    } else {
	return new LQX::MethodInvocationExpression(arg1);
    }
}


void * qnap2_get_integer( long value )
{
    /* add to symbol table */
    return new LQX::ConstantValueExpression( static_cast<double>( value ) );
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
    const std::map<int,const QNIO::QNAP2_Document::Distribution>::const_iterator distribution_type = parser_to_distribution.find(code);
    if ( distribution_type == parser_to_distribution.end() ) abort();
    return new QNIO::QNAP2_Document::ServiceDistribution( distribution_type->second, static_cast<LQX::SyntaxTreeNode *>(mean), static_cast<LQX::SyntaxTreeNode *>(shape) );
}

void * qnap2_get_string( const char * string )
{
    return new LQX::ConstantValueExpression( string );
}

void * qnap2_get_transit_pair( const char * name, void * value )
{
    return new std::pair<const std::string,LQX::SyntaxTreeNode *>( std::string(name), static_cast<LQX::SyntaxTreeNode *>(value) );
}

    
void * qnap2_get_variable( const char * variable )
{
    if ( !QNIO::QNAP2_Document::__document->findVariable( variable ) ) {
	qnap2error( "undefined variable:: %s.", variable );
    }
    return new LQX::VariableExpression( variable, true );
}


void * qnap2_get_scalar_lvalue( const char * variable )
{
    try {
	return QNIO::QNAP2_Document::__document->getLValue( std::string(variable ) );
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "undefined variable: %s.", variable );
	return nullptr;
    }
}
void * qnap2_get_vector_lvalue( const char *, const char * ) { return nullptr; }
void * qnap2_get_object_lvalue( const char *, const char * ) { return nullptr; }



/*
 * Can be init(class) = value;  If only one class, init=value;.
 * Or init=var name.
 */

void qnap2_set_station_init( const void * list, void * value )
{
    if ( value == nullptr ) return;
    try {
	if ( list == nullptr ) {
	    BCMP::Model::Chain::map_t& chains = QNIO::QNAP2_Document::__document->model().chains();
	    std::for_each( chains.begin(), chains.end(), QNIO::QNAP2_Document::SetClassInit( static_cast<LQX::SyntaxTreeNode*>(value) ) );
	} else {
	    const std::vector<std::string>* class_list = static_cast<const std::vector<std::string>*>(list);
	    std::for_each( class_list->begin(), class_list->end(), QNIO::QNAP2_Document::SetClassInit( static_cast<LQX::SyntaxTreeNode*>(value) ) );
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
    try {
	QNIO::QNAP2_Document::__document->getStation( std::string( name ) );
	QNIO::QNAP2_Document::__station.first = name;
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
}

void qnap2_set_program( void * program )
{
    LQX::Program::loadRawProgram( QNIO::QNAP2_Document::__document->appendProgram( static_cast<std::vector<LQX::SyntaxTreeNode*>*>(program) ) );
}

void qnap2_set_station_sched( const void * scheduling )
{
    if ( !QNIO::QNAP2_Document::__document->setStationScheduling( static_cast<const char *>(scheduling) ) ) {
	qnap2error( "undefined station scheduling: %s.", static_cast<const char *>(scheduling) );
    }
}

void qnap2_set_station_service( const void * list, const void * service )
{
    /* Service list is Service::Distribution. */

    assert ( service != nullptr );
    try {
	if ( list == nullptr ) {
	    const BCMP::Model::Chain::map_t& chains = QNIO::QNAP2_Document::__document->chains();
	    std::for_each( chains.begin(), chains.end(), QNIO::QNAP2_Document::SetStationService( *static_cast<const QNIO::QNAP2_Document::ServiceDistribution*>(service) ) );
	} else {
	    const std::vector<std::string>* class_list = static_cast<const std::vector<std::string>*>(list);
	    std::for_each( class_list->begin(), class_list->end(), QNIO::QNAP2_Document::SetStationService( *static_cast<const QNIO::QNAP2_Document::ServiceDistribution*>(service) ) );
	}
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
}

void qnap2_set_station_transit( const char * class_name, const void * transit )
{
    assert( transit != nullptr );
    try {
	if ( class_name == nullptr ) {
	    const BCMP::Model::Chain::map_t& chains = QNIO::QNAP2_Document::__document->chains();
	    std::for_each( chains.begin(), chains.end(), QNIO::QNAP2_Document::SetStationTransit( *static_cast<const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>*>(transit) ) );
	} else {
	    QNIO::QNAP2_Document::SetStationTransit( *static_cast<const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>*>(transit) )( std::string( class_name ) );
	}
    }
    catch ( const std::logic_error& error ) {
	qnap2error( "%s.", error.what() );
    }
}

void qnap2_set_station_type( int code, int copies )
{
    QNIO::QNAP2_Document::__document->setStationType( QNIO::QNAP2_Document::__station_type.at(code), copies );
}

void qnap2_set_station_prio( const void *, const void * ){}
void qnap2_set_station_quantum( const void *, const void * ){}
void qnap2_set_station_rate( void * ){}

void * qnap2_assignment( void * arg1, void * arg2 )
{
    return new LQX::AssignmentStatementNode(static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2));
}


void * qnap2_if_statement( void * condition, void * if_stmt, void * else_stmt )
{
    return new LQX::ConditionalStatementNode( static_cast<LQX::SyntaxTreeNode *>(condition), static_cast<LQX::SyntaxTreeNode *>(if_stmt), static_cast<LQX::SyntaxTreeNode *>(else_stmt) );
}

/*
 * for variable := sublist do loop_stmt
 */

void * qnap2_for_statement( void * variable, void * sublist, void * loop_stmt ) 
{
//    #warning All wrong.
//    return new LQX::ForeachStatementNode( "", static_cast<LQX::SyntaxTreeNode *>(variable), static_cast<LQX::SyntaxTreeNode *>(nullpr), 
//					  static_cast<LQX::SyntaxTreeNode *>(loop_stmt), nullptr, nullptr );	// !!! args all wrong.
	abort();
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
    /* ------------------------------------------------------------------------ */
    /*                                  Input                                   */
    /* ------------------------------------------------------------------------ */

    QNAP2_Document::QNAP2_Document( const std::string& input_file_name ) :
	Document(input_file_name, BCMP::Model()),
 	_symbol_table(), _field_table(), _transit()
    {
	__document = this;
    }

    QNAP2_Document::QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model ) :
	Document(input_file_name, model),
	_symbol_table(), _field_table(), _transit()
    {
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
    QNAP2_Document::declareClass( const Variable& variable )
    {
	assert( variable.isScalar() );
	if ( !_symbol_table.emplace( variable, nullptr ).second ) return false;
	return model().chains().emplace( BCMP::Model::Chain::pair_t( variable.name(), BCMP::Model::Chain() ) ).second;
    }


    bool
    QNAP2_Document::declareField( QNAP2_Document::Type, const Variable& variable )
    {
	return false;
    }

    bool
    QNAP2_Document::declareStation( const Variable& variable )
    {
	assert( variable.isScalar() );
	return model().stations().emplace( BCMP::Model::Station::pair_t( variable.name(), BCMP::Model::Station() ) ).second;
    }

    bool
    QNAP2_Document::defineVariable( QNAP2_Document::Type type, const Variable& variable )
    {
	std::pair<std::map<const Variable,LQX::SyntaxTreeNode *>::iterator,bool> result = _symbol_table.emplace( variable, nullptr );
	if ( !result.second ) return false;			// Duplicate.
	LQX::SyntaxTreeNode * lvalue = new LQX::VariableExpression( variable.name(), false );
	result.first->second = lvalue;

	LQX::SyntaxTreeNode * initially = nullptr;
	switch ( type ) {
	case Type::Boolean:	initially = new LQX::ConstantValueExpression( false ); break;
	case Type::Integer:
	case Type::Real:	initially = new LQX::ConstantValueExpression( 0. ); break;
	case Type::String:	initially = new LQX::ConstantValueExpression( "" ); break;
	default: abort();
	}
	_program.push_back( new LQX::AssignmentStatementNode( lvalue, initially ) );

	return true;
    }
    

    
    /*
     * Create LQX objects for all variables (and classes).
     */
    
    void
    QNAP2_Document::declareObjects()
    {
	BCMP::Model::Chain::map_t& chains = model().chains();
	if ( chains.empty() ) {
	    declareClass( Variable( std::string("") ) );
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
	BCMP::Model::Station& station =	getStation( station_name );

	/* Copy over constructed values */
	
	station = __station.second;			/* copy */
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
    
	// chain is the name.
	// __transit{destination,chain,LQX::Expression}.  I
	// if expression is constant, and rate is constant, compute and save as constant.
	// otherwise, save expresion.
	// recurse with transitfrom this station.... setStationTransit( ..., transit * current visits ).
//	static std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>> __transit; /* to, chain, value under construction */

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


    bool
    QNAP2_Document::mapTransitToVisits( const std::string& station_name, const std::string& class_name, LQX::SyntaxTreeNode * visits, std::deque<std::string>& stack )
    {
	BCMP::Model::Station::Class& the_class = model().stationAt( station_name ).classAt( class_name );
	if ( the_class.visits() != nullptr ) return false;			// Already set, so done.
	the_class.setVisits( visits );
	stack.push_back( station_name );

	/*
	 * If stack empty, then start... otherwise, look for one and
	 * only one transit to an item on the top.  That's the return
	 * and it should be one.  If the sum of visits is <= 1.0,
	 * renormalize to the return value
	 */

	/* Recurse */

	const std::map<std::string,std::map<std::string,LQX::SyntaxTreeNode *>>& from = _transit.at( station_name );	
	const std::map<std::string,LQX::SyntaxTreeNode *>& transit = from.at( class_name );
	for ( std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator to = transit.begin(); to != transit.end(); ++to ) {
	    mapTransitToVisits( to->first, class_name, getProduct( visits, to->second ), stack );
	}
	stack.pop_back();
	return true;
    }


    bool
    QNAP2_Document::findVariable( const std::string& name ) const
    {
	return _symbol_table.find( name ) != _symbol_table.end();
    }

    
    void
    QNAP2_Document::SetClassInit::operator()( BCMP::Model::Chain::pair_t& chain ) const
    {
	if ( chain.second.type() != BCMP::Model::Chain::Type::UNDEFINED ) return;	/* Skip those that are set */
	if ( chain.second.type() == BCMP::Model::Chain::Type::OPEN ) throw std::invalid_argument( std::string( "mixed type: " ) + chain.first );
	chain.second.setType( BCMP::Model::Chain::Type::CLOSED );
	chain.second.setCustomers( _customers );
	QNAP2_Document::__station.second.setReference( true );
    }

    
    void
    QNAP2_Document::SetClassInit::operator()( const std::string& class_name ) const
    {
	BCMP::Model::Chain& chain = QNAP2_Document::getChain( class_name );

	if ( chain.type() == BCMP::Model::Chain::Type::OPEN ) throw std::invalid_argument( std::string( "mixed type: " ) + class_name );
	chain.setType( BCMP::Model::Chain::Type::CLOSED );
	chain.setCustomers( _customers );
	QNAP2_Document::__station.second.setReference( true );
    }

    
    void
    QNIO::QNAP2_Document::SetStationService::operator()( const BCMP::Model::Chain::pair_t& chain ) const
    {
	BCMP::Model::Station::Class& the_class = QNIO::QNAP2_Document::__station.second.classes().emplace( chain.first, BCMP::Model::Station::Class() ).first->second;
	the_class.setServiceTime( _service.mean() );
    }


    void
    QNIO::QNAP2_Document::SetStationService::operator()( const std::string& class_name ) const
    {
	BCMP::Model::Station::Class& the_class = QNIO::QNAP2_Document::__station.second.classes().emplace( class_name, BCMP::Model::Station::Class() ).first->second;
	if ( the_class.service_time() != nullptr ) throw std::domain_error( "service time previously set." );
	the_class.setServiceTime( _service.mean()  );
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
	const BCMP::Model::Station::map_t& stations = QNAP2_Document::__document->model().stations();
	for ( std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>*>::const_iterator transit = _transit.begin(); transit != _transit.end(); ++transit ) {
	    const std::string& station_name = (*transit)->first;
	    if ( stations.find( station_name ) == stations.end() ) throw std::invalid_argument( std::string( "undefined station: " ) + station_name );
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



    LQX::SyntaxTreeNode *
    QNAP2_Document::getLValue( const std::string& scalar ) 
    {
	std::map<const Variable,LQX::SyntaxTreeNode *>::iterator symbol = _symbol_table.find( scalar );
	if ( symbol == _symbol_table.end() ) throw std::invalid_argument( std::string( "undefined variable: " ) + scalar );

	return symbol->second;
    }


    /*
     * Append statements to the program
     */
    
    std::vector<LQX::SyntaxTreeNode *>*
    QNAP2_Document::appendProgram( std::vector<LQX::SyntaxTreeNode *>* statements )
    {
	if ( statements != nullptr && !statements->empty() ) {
	    _program.insert( _program.end(), statements->begin(), statements->end() );
	}
	return &_program;
    }


#if 0
    /* Find class, instantiate a map if not present, then insert an unnamed variable which can be assigned */
    /* return an LQX expression. */

    void *
    QNAP2_Document::assignmentStatement( const std::string& class_name, const std::string& field_name, LQX::SyntaxTreeNode * value )
    {
//    std::map<const Variable,void *> _symbol_table;		/* All symbols defined in Declare */
	std::map<const Variable,void *>::iterator symbol = _symbol_table.find( class_name );
	if ( symbol == _symbol_table.end() ) throw std::invalid_argument( std::string( "undefined class: " ) + class_name );
	std::map<const std::string,Type>::iterator field = _field_table.find( field_name );
	if ( field == _field_table.end() ) throw std::invalid_argument( std::string( "undefined field: " ) + field_name );

	_symbol_table.emplace( class_name, nullptr )
	return nullptr;
    }
#endif
	
    /*
     * Multiply the multiplier with the multiplicand.  Simplify where possible.
     */
    
    LQX::SyntaxTreeNode *
    QNAP2_Document::getProduct( LQX::SyntaxTreeNode * multiplier, LQX::SyntaxTreeNode * multiplicand )
    {
	if ( dynamic_cast<LQX::ConstantValueExpression *>( multiplier ) && dynamic_cast<LQX::ConstantValueExpression *>( multiplicand ) ) {
	    return new LQX::ConstantValueExpression( multiplier->invoke(nullptr)->getDoubleValue() * multiplicand->invoke(nullptr)->getDoubleValue() );
	} else if ( dynamic_cast<LQX::ConstantValueExpression *>( multiplier ) && multiplier->invoke(nullptr)->getDoubleValue() == 1. ) {
	    return multiplicand;
	} else if ( dynamic_cast<LQX::ConstantValueExpression *>( multiplicand ) && multiplicand->invoke(nullptr)->getDoubleValue() == 1. ) {
	    return multiplier;
	} else {
	    return new LQX::MathExpression( LQX::MathExpression::MULTIPLY, multiplier, multiplicand );
	}
    }

    /* ------------------------------------------------------------------------ */
    /*                                  Output                                  */
    /* ------------------------------------------------------------------------ */

    /*
     * Print the results in the old-style qnap format.
     */
    
    std::ostream&
    QNAP2_Document::print( std::ostream& output ) const
    {
	output << Output( model() );
	return output;
    }
    
    std::streamsize QNAP2_Document::Output::__width = 10;
    std::streamsize QNAP2_Document::Output::__precision = 6;
    std::string QNAP2_Document::Output::__separator = "*";

    std::ostream&
    QNAP2_Document::Output::print( std::ostream& output ) const
    {
	const std::streamsize old_precision = output.precision(__precision);
//    output << " - (" << _solver << ") - " << std::endl;
	output.fill('*');
	output << std::setw(__width*6+7) << "*" << std::endl;
	output.fill(' ');
	output << __separator << std::setw(__width) << "name " << header() << __separator << std::endl;
	output.fill('*');
	output << std::setw(__width*6+7) << "*" << std::endl;
	output.fill(' ');
	output << __separator << std::setw(__width) << " " << QNAP2_Document::Output::blankline() << __separator << std::endl;
	for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	    const BCMP::Model::Station::Class::map_t& results = mi->second.classes();
	    if ( results.empty() ) continue;
	    const BCMP::Model::Station::Class sum = std::accumulate( std::next(results.begin()), results.end(), results.begin()->second, &BCMP::Model::Station::sumResults ).deriveResidenceTime();
	    const double service = sum.throughput() > 0 ? sum.utilization() / sum.throughput() : 0.0;
	
	    /* Sum will work for single class too. */
	    output.setf(std::ios::left, std::ios::adjustfield);
	    output << __separator << std::setw(__width) << ( " " + mi->first );
	    print(output,service,sum);
	    output << __separator << std::endl;
	    if ( results.size() > 1 ) {
		for ( BCMP::Model::Station::Class::map_t::const_iterator result = results.begin(); result != results.end(); ++result ) {
		    if (result->second.throughput() == 0 ) continue;
		    output << __separator << std::setw(__width) <<  ( "(" + result->first + ")");
		    const double service_time = BCMP::Model::getDoubleValue( mi->second.classAt(result->first).service_time() );
		    const double visits = BCMP::Model::getDoubleValue( mi->second.classAt(result->first).visits() );
		    print( output, service_time * visits, result->second );
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
    QNAP2_Document::Output::print( std::ostream& output, double service_time, const BCMP::Model::Station::Result& item ) const
    {
	output.unsetf( std::ios::floatfield );
	output << __separator << std::setw(__width) << service_time
	       << __separator << std::setw(__width) << item.utilization()
	       << __separator << std::setw(__width) << item.queue_length()
	       << __separator << std::setw(__width) << item.residence_time()		// per visit.
	       << __separator << std::setw(__width) << item.throughput();
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

/*
 - mean value analysis ("mva") -
 *******************************************************************
 *  name    *  service * busy pct *  cust nb * response *  thruput *
 *******************************************************************
 *          *          *          *          *          *          *
 * terminal *0.2547    * 0.000    * 1.579    *0.2547    * 6.201    *
 *(c1      )*0.3333    * 0.000    * 1.417    *0.3333    * 4.250    *
 *(c2      )*0.8333E-01* 0.000    *0.1625    *0.8333E-01* 1.951    *
 *          *          *          *          *          *          *
 * p1       *0.4000    *0.8267    * 2.223    * 1.076    * 2.067    *
 *(c1      )*0.4000    *0.5667    * 1.512    * 1.067    * 1.417    *
 *(c2      )*0.4000    *0.2601    *0.7109    * 1.093    *0.6502    *
 *          *          *          *          *          *          *
 * p2       *0.2000    *0.6317    * 1.215    *0.3848    * 3.158    *
 *(c1      )*0.2000    *0.5667    * 1.071    *0.3780    * 2.833    *
 *(c2      )*0.2000    *0.6502E-01*0.1442    *0.4436    *0.3251    *
 *          *          *          *          *          *          *
 * p3       *0.7000    *0.6827    *0.9823    * 1.007    *0.9753    *
 *(c2      )*0.7000    *0.6827    *0.9823    * 1.007    *0.9753    *
 *          *          *          *          *          *          *
 *******************************************************************
              memory used:       4024 words of 4 bytes
               (  1.55  % of total memory)     
*/

    /*
     * "&" is a comment.
     * identifiers are limited to the first 8 characters.
     * input is limited to 80 characters.
     * statements are terminated with ";"
     * If there are SPEX variables, then use those.
     */

    std::ostream&
    QNAP2_Document::printInput( std::ostream& output ) const
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
	if ( !customer_vars.empty() )   output << qnap2_statement( "integer " + customer_vars, "SPEX customers vars." ) << std::endl;
	const std::string integer_vars  = std::accumulate( stations().begin(), stations().end(), std::string(""), getIntegerVariables( model(), symbol_table ) );
	if ( !integer_vars.empty() )    output << qnap2_statement( "integer " + integer_vars, "SPEX multiplicity vars." ) << std::endl;
	const std::string real_vars    	= std::accumulate( stations().begin(), stations().end(), std::string(""), getRealVariables( model(), symbol_table ) );
	if ( !real_vars.empty() )       output << qnap2_statement( "real " + real_vars, "SPEX service time vars." ) << std::endl;
	const std::string deferred_vars	= std::accumulate( LQIO::Spex::inline_expressions().begin(), LQIO::Spex::inline_expressions().end(), std::string(""), &getDeferredVariables );
	if ( !deferred_vars.empty() )   output << qnap2_statement( "real " + deferred_vars, "SPEX deferred vars." ) << std::endl;
	std::set<const LQIO::DOM::ExternalVariable *> symbol_table_2;
	const std::string result_vars	= std::accumulate( LQIO::Spex::result_variables().begin(), LQIO::Spex::result_variables().end(), std::string(""), getResultVariables(symbol_table_2) );
	if ( !result_vars.empty() )     output << qnap2_statement( "real " + result_vars, "SPEX result vars." ) << std::endl;

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
	if ( multiclass() || !LQIO::Spex::result_variables().empty() ) {
	    output << "&" << std::endl
		   << qnap2_keyword( "control" ) << std::endl;
	    if ( multiclass() ) {
		output << qnap2_statement( "class=all queue", "Compute for all classes" ) << std::endl;	// print for all classes.
	    }
	    if ( !LQIO::Spex::result_variables().empty() ) {
		output << qnap2_statement( "option=nresult", "Suppress default output" ) << std::endl;
	    }
	}

	/* 6) Finally, assign the parameters defined in steps 2 & 3 from the constants and variables in the model */

	output << "&" << std::endl
	       << qnap2_keyword( "exec" ) << std::endl
	       << "   begin" << std::endl;

	if ( !LQIO::Spex::result_variables().empty() && !LQIO::Spex::__no_header ) {
	    printResultsHeader( output, LQIO::Spex::result_variables() );
	}

	if ( LQIO::Spex::input_variables().size() > LQIO::Spex::array_variables().size() ) {	// Only care about scalars
	    output << "&  -- SPEX scalar variables --" << std::endl;
	    std::for_each( LQIO::Spex::scalar_variables().begin(), LQIO::Spex::scalar_variables().end(), printSPEXScalars( output ) );	/* Scalars */
	}

	/* Insert QNAP for statements for arrays and completions. */
	if ( !LQIO::Spex::array_variables().empty() ) {
	    output << "&  -- SPEX arrays and completions --" << std::endl;
	    std::for_each( LQIO::Spex::array_variables().begin(), LQIO::Spex::array_variables().end(), for_loop( output ) );
	}

	if ( !LQIO::Spex::inline_expressions().empty() ) {
	    output << "&  -- SPEX deferred assignments --" << std::endl;
	    std::for_each( LQIO::Spex::inline_expressions().begin(), LQIO::Spex::inline_expressions().end(), printSPEXDeferred( output ) );	/* Arrays and completions */
	}

	output << "&  -- Class variables --" << std::endl;
        printClassVariables( output );

	output << "&  -- Station variables --" << std::endl;
	std::for_each( stations().begin(), stations().end(), printStationVariables( output, model() ) );

	/* Let 'er rip! */
	output << "&  -- Let 'er rip! --" << std::endl;
	output << qnap2_statement( "solve" ) << std::endl;
	if ( !LQIO::Spex::result_variables().empty() ) {
	    output << "&  -- SPEX results for QNAP2 solutions are converted to" << std::endl
		   << "&  -- the LQN output for throughput, service and waiting time." << std::endl
		   << "&  -- QNAP2 throughput for a reference task is per-slice," << std::endl
		   << "&  -- and not the aggregate so divide by the number of transits." << std::endl
		   << "&  -- Service time is mservice() + sum of mresponse()." << std::endl
		   << "&  -- Waiting time is mresponse() - mservice()." << std::endl;
	    /* Output statements to have Qnap2 compute LQN results */
	    std::for_each( LQIO::Spex::result_variables().begin(), LQIO::Spex::result_variables().end(), getObservations( output, model() ) );
	    /* Print them */
	    printResults( output, LQIO::Spex::result_variables() );
	}

	/* insert end's for each for. */
	std::for_each( LQIO::Spex::array_variables().rbegin(), LQIO::Spex::array_variables().rend(), end_for( output ) );

	/* End of program */
	output << qnap2_statement( "end" ) << std::endl;
        output.flags(flags);

	output << qnap2_keyword( "end" ) << std::endl;

	return output;
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


    std::string
    QNAP2_Document::getDeferredVariables( const std::string& s1, const std::pair<const LQIO::DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& p2 )
    {
	std::string s3;
	if ( !s1.empty() ) {
	    return s1 + "," + p2.first->getName();
	} else {
	    return p2.first->getName();
	}
    }


    /*
     * Convert External Variables to Strings.  Exclude them from the
     * list as they have been declared earlier.
     */

    QNAP2_Document::getResultVariables::getResultVariables( const std::set<const LQIO::DOM::ExternalVariable *>& symbol_table ) : _symbol_table()
    {
	for ( std::set<const LQIO::DOM::ExternalVariable *>::const_iterator var = symbol_table.begin(); var != symbol_table.end(); ++var ) {
	    if ( !dynamic_cast<const LQX::VariableExpression *>(*var) ) continue;
	    _symbol_table.insert(dynamic_cast<const LQX::VariableExpression *>(*var)->getName());
	}
    }

    std::string
    QNAP2_Document::getResultVariables::operator()( const std::string& s1, const LQIO::Spex::var_name_and_expr& var ) const
    {
	if ( _symbol_table.find(var.first) != _symbol_table.end() ) {
	    return s1;
	} else if ( !s1.empty() ) {
	    return s1 + "," + var.first;
	} else {
	    return var.first;
	}
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
	if ( !station.reference() && station.hasClass(_name) && static_cast<unsigned int>(visits->invoke(nullptr)->getDoubleValue()) > 0 ) {
	    if ( !s.empty() ) s += ",";
	    s += m2.first + "," + to_real(visits);
	}
	return s;
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
	    
	    if ( !station.hasClass(k->first) || service_time->invoke(nullptr)->getDoubleValue() == 0. ) {
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
     * Print out all deferred assignment variables (they go inside all loop bodies that
     * arise for array and completion assignements
     */

    void
    QNAP2_Document::printSPEXScalars::operator()( const std::string& var ) const
    {
	const std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator expr = LQIO::Spex::input_variables().find(var);

	if ( !expr->second ) return;		/* Comprehension or array.  I could check array_variables. */
	std::ostringstream ss;
	ss << expr->first << ":=";
	expr->second->print(ss,0);
	std::string s(ss.str());
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());	/* Strip blanks */
	_output << qnap2_statement( s ) << std::endl;	/* Swaps $ to _ and appends ;	*/
    }

    void
    QNAP2_Document::printSPEXDeferred::operator()( const std::pair<const LQIO::DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& expr ) const
    {
	std::ostringstream ss;
	ss << expr.first->getName() << ":=";
	expr.second->print(ss);
	std::string s(ss.str());
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());	/* Strip blanks */
	_output << qnap2_statement( s ) << std::endl;	/* Swaps $ to _ and appends ;	*/
    }


    /*
     * Print out the Header for the results.   Format the same as printResults (next)
     */

    void
    QNAP2_Document::printResultsHeader( std::ostream& output, const std::vector<LQIO::Spex::var_name_and_expr>& vars ) const
    {
	std::string s;
	std::string comment = "SPEX results";
	bool continuation = false;
	size_t count = 0;
	for ( std::vector<LQIO::Spex::var_name_and_expr>::const_iterator var = vars.begin(); var != vars.end(); ++var ) {
	    ++count;
	    if ( s.empty() && continuation ) s += "\",\",";	/* second print statement, signal continuation with "," */
	    else if ( !s.empty() ) s += ",\",\",";		/* between vars. */
	    s += "\"" + var->first + "\"";
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
     * Print out the SPEX result variables in chunks as QNAP2 doesn't like BIG print statements.
     */

    void
    QNAP2_Document::printResults( std::ostream& output, const std::vector<LQIO::Spex::var_name_and_expr>& vars ) const
    {
	std::string s;
	std::string comment = "SPEX results";
	bool continuation = false;
	size_t count = 0;
	for ( std::vector<LQIO::Spex::var_name_and_expr>::const_iterator var = vars.begin(); var != vars.end(); ++var ) {
	    ++count;
	    if ( s.empty() && continuation ) s += "\",\",";	/* second print statement, signal continuation with "," */
	    else if ( !s.empty() ) s += ",\",\",";		/* between vars. */
	    s += var->first;
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
    QNAP2_Document::for_loop::operator()( const std::string& var ) const
    {
	const std::map<std::string,LQIO::Spex::ComprehensionInfo>::const_iterator comprehension = LQIO::Spex::comprehensions().find( var );
	std::string loop;
	std::ostringstream ss;
	if ( comprehension != LQIO::Spex::comprehensions().end() ) {
	    /* Comprehension */
	    /* FOR i:=1 STEP n UNTIL m DO... */
	    ss << comprehension->second.getInit() << " step " << comprehension->second.getStep() << " until " << comprehension->second.getTest();
	    loop = ss.str();
	} else {
	    /* Array variable */
	    /* FOR i:=1,2,3... DO */
	    const std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator array = LQIO::Spex::input_variables().find(var);
	    array->second->print(ss);
	    loop = ss.str();
	    /* Get rid of brackets and spaces from array_create(0.5, 1, 1.5) */
	    loop.erase(loop.begin(), loop.begin()+13);	/* "array_create(" */
	    loop.erase(loop.end()-1,loop.end());	/* ")" */
	    loop.erase(std::remove(loop.begin(), loop.end(), ' '), loop.end());	/* Strip blanks */
	}
	std::string name = var;
	std::replace( name.begin(), name.end(), '$', '_'); 		// Make variables acceptable for QNAP2.
	_output << "   " << "for "<< name << ":=" << loop << " do begin" << std::endl;
    }

    /*
     * Sequence through the stations and classes looking for the result variables.
     */

    /* mservice, mbusypct, mcustnb, vcustnb, mresponse, mthruput, custnb */


    void
    QNAP2_Document::getObservations::operator()( const LQIO::Spex::var_name_and_expr& var ) const
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
    QNAP2_Document::end_for::operator()( const std::string& var ) const
    {
	std::string comment = "for " + var;
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
	if ( visits->invoke(nullptr)->getDoubleValue() == 0. ) return s1;	/* ignore zeros */
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
