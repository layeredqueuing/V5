/*
 *  $Id: dom_object.cpp 17411 2024-10-31 21:18:36Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cassert>
#include <cmath>
#include <cstdarg>
#include <sstream>
#include "dom_document.h"
#include "dom_object.h"
#include "dom_task.h"
#include "glblerr.h"

namespace LQIO {
    namespace DOM {

	size_t DocumentObject::__sequenceNumber = 0;

	DocumentObject::DocumentObject() 
	    : _document(nullptr), _sequenceNumber(0xDEAD0000DEAD0000), _line_number(0), _name("null"), _comment()
	{
	}

	DocumentObject::DocumentObject(const Document * document, const std::string& name ) 
	    : _document(document), _sequenceNumber(__sequenceNumber), _line_number(srvnlineno), _name(name), _comment()
	{
	    assert( document );
	    __sequenceNumber += 1;
	}

	DocumentObject::DocumentObject(const DocumentObject& src ) 
	    : _document(src._document), _sequenceNumber(__sequenceNumber), _line_number(srvnlineno), _name(), _comment()
	{
	    assert( _document );
	    __sequenceNumber += 1;
	}

	DocumentObject::~DocumentObject() 
	{
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Error Handling] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

 	/*
	 * Same as glblerr.cpp, except without the name.
	 */
	
	std::map<unsigned, LQIO::error_message_type> DocumentObject::__error_messages = {
	    { LQIO::ADV_MESSAGES_DROPPED,		{ LQIO::error_severity::ADVISORY, "dropped messages for open-class queues" } },
	    { LQIO::ERR_ARRIVAL_RATE,			{ LQIO::error_severity::ERROR,    "arrival rate of %g exceeds service rate of %g." } },
	    { LQIO::ERR_ASYNC_REQUEST_TO_WAIT,		{ LQIO::error_severity::ERROR,    "(wait) cannot accept send-no-reply requests" } },
	    { LQIO::ERR_BAD_PATH_TO_JOIN,		{ LQIO::error_severity::ERROR,    "activity \"%s\" is not reachable" } },
	    { LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH,	{ LQIO::error_severity::ERROR, 	  "has a cycle in activity graph.  Backtrace is \"%s\"" } },
	    { LQIO::ERR_CYCLE_IN_CALL_GRAPH,		{ LQIO::error_severity::ERROR,    "has a cycle in call graph,  backtrace is \"%s\"" } },
	    { LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE,	{ LQIO::error_severity::ERROR,    "previously used in the join at line %d" } },
	    { LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE,	{ LQIO::error_severity::ERROR,    "previously used in the fork at line %d" } },
	    { LQIO::ERR_DUPLICATE_SEMAPHORE_ENTRY_TYPES,{ LQIO::error_severity::ERROR,    "both entry \"%s\" and entry \"%s\" are of semaphore type %s" } },
	    { LQIO::ERR_DUPLICATE_START_ACTIVITY,	{ LQIO::error_severity::ERROR,    "has a start activity defined.  Activity \"%s\" is a duplicate" } },
	    { LQIO::ERR_DUPLICATE_SYMBOL,		{ LQIO::error_severity::ERROR,    "previously defined" } },
	    { LQIO::ERR_FORK_JOIN_MISMATCH,		{ LQIO::error_severity::ERROR,    "does not match %s \"%s\" at line %d" } },
	    { LQIO::ERR_INFINITE_SERVER, 		{ LQIO::error_severity::ERROR, 	  "cannot be an infinite server" } },
	    { LQIO::ERR_INVALID_FORWARDING_PROBABILITY,	{ LQIO::error_severity::ERROR,    "has a total forwarding probability of %g" } },
	    { LQIO::ERR_INVALID_OR_BRANCH_PROBABILITY,	{ LQIO::error_severity::ERROR,    "activity \"%s\" has invalid probability of %g" } },
	    { LQIO::ERR_INVALID_PARAMETER,		{ LQIO::error_severity::ERROR,    "invalid %s: %s" } },
	    { LQIO::ERR_INVALID_REPLY_DUPLICATE,	{ LQIO::error_severity::ERROR,    "makes a duplicate reply for entry \"%s\"" } },
	    { LQIO::ERR_INVALID_REPLY_FOR_SNR_ENTRY,	{ LQIO::error_severity::ERROR,    "makes an invalid reply for entry \"%s\" which does not accept rendezvous requests" } },
	    { LQIO::ERR_INVALID_REPLY_FROM_BRANCH,	{ LQIO::error_severity::ERROR,    "makes an invalid reply from a branch for entry \"%s\"" } },
	    { LQIO::ERR_INVALID_VISIT_PROBABILITY,	{ LQIO::error_severity::ERROR,    "has a total visit probability of %g (it should be 1.0)" } },
	    { LQIO::ERR_IS_START_ACTIVITY,		{ LQIO::error_severity::ERROR,    "is a start activity" } },
	    { LQIO::ERR_MIXED_ENTRY_TYPES,		{ LQIO::error_severity::ERROR,    "is specified using both activity and phase methods" } },
	    { LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES,	{ LQIO::error_severity::ERROR,    "is specified as both a lock and a unlock" } },
	    { LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES,	{ LQIO::error_severity::ERROR,    "is specified as both a signal and a wait" } },
	    { LQIO::ERR_NON_REF_THINK_TIME,		{ LQIO::error_severity::ERROR,    "is not a reference task; it cannot have think time" } },
	    { LQIO::ERR_NON_UNITY_REPLIES,		{ LQIO::error_severity::ERROR,    "generates %4.2f replies" } },
	    { LQIO::ERR_NOT_REACHABLE,			{ LQIO::error_severity::ERROR,    "is not reachable"} },
	    { LQIO::ERR_NOT_RWLOCK_TASK,		{ LQIO::error_severity::ERROR,    "cannot have a %s for entry \"%s\"" } },
	    { LQIO::ERR_NOT_SEMAPHORE_TASK,		{ LQIO::error_severity::ERROR,    "cannot have a %s for entry \"%s\"" } },
	    { LQIO::ERR_NOT_SPECIFIED,			{ LQIO::error_severity::ERROR,    "has neither service time nor any outgoing requests" } },
	    { LQIO::ERR_NOT_SUPPORTED,			{ LQIO::error_severity::ERROR,    "does not support the %s feature" } },
	    { LQIO::ERR_NO_GROUP_SPECIFIED,		{ LQIO::error_severity::ERROR,    "running on processor \"%s\" using fair share scheduling has no group specified" } },
	    { LQIO::ERR_NO_QUANTUM_SCHEDULING,		{ LQIO::error_severity::ERROR,    "with \"%s\" scheduling has no quantum specified"} },
	    { LQIO::ERR_NO_SEMAPHORE,			{ LQIO::error_severity::ERROR,    "has neither a signal nor a wait entry specified" } },
	    { LQIO::ERR_NO_START_ACTIVITIES,		{ LQIO::error_severity::ERROR,    "has activities but none are reachable" } },
	    { LQIO::ERR_OPEN_AND_CLOSED_CLASSES,	{ LQIO::error_severity::ERROR,    "accepts both rendezvous and send-no-reply messages" } },
	    { LQIO::ERR_OR_BRANCH_PROBABILITIES,	{ LQIO::error_severity::ERROR,    "branch probabilities do not sum to 1.0; sum is %4.2f" }, },
	    { LQIO::ERR_REFERENCE_TASK_FORWARDING,	{ LQIO::error_severity::ERROR,    "entry \"%s\" cannot forward requests" } },
	    { LQIO::ERR_REFERENCE_TASK_IS_INFINITE,	{ LQIO::error_severity::ERROR,    "must have a finite number of copies" } },
	    { LQIO::ERR_REFERENCE_TASK_IS_RECEIVER,	{ LQIO::error_severity::ERROR,    "entry \"%s\" receives requests" } },
	    { LQIO::ERR_REFERENCE_TASK_OPEN_ARRIVALS,	{ LQIO::error_severity::ERROR,    "entry \"%s\" cannot have open arrivals" } },
	    { LQIO::ERR_REFERENCE_TASK_REPLIES,		{ LQIO::error_severity::ERROR,    "replies to entry \"%s\"" } },
	    { LQIO::ERR_REPLY_NOT_GENERATED,		{ LQIO::error_severity::ERROR,    "must reply; the reply is not specified in the activity graph" } },
	    { LQIO::ERR_TASK_ENTRY_COUNT, 		{ LQIO::error_severity::ERROR, 	  "has %d entries defined, exactly %d are required" } },
	    { LQIO::ERR_TASK_HAS_NO_ENTRIES,	 	{ LQIO::error_severity::ERROR, 	  "has no entries defined" } },
	    { LQIO::ERR_WRONG_TASK_FOR_ENTRY,		{ LQIO::error_severity::ERROR,    "is not part of task \"%s\""} },
	    { LQIO::WRN_ENTRY_HAS_NO_REQUESTS,		{ LQIO::error_severity::WARNING,  "does not receive any requests" } },
	    { LQIO::WRN_MIXED_ENTRY_TYPES,		{ LQIO::error_severity::WARNING,  "is specified using both activity and phase methods" } },
	    { LQIO::WRN_INFINITE_MULTI_SERVER, 		{ LQIO::error_severity::WARNING,  "is an infinite server with a multiplicity of %d" } },
	    { LQIO::WRN_INFINITE_SERVER_OPEN_ARRIVALS,	{ LQIO::error_severity::WARNING,  "is an infinite server that accepts either asynchronous messages or open arrivals" } },
	    { LQIO::WRN_NON_CFS_PROCESSOR,		{ LQIO::error_severity::WARNING,  "is a processor which is not running fair share scheduling" } },
	    { LQIO::WRN_NOT_USED,			{ LQIO::error_severity::WARNING,  "is not used" } },
	    { LQIO::WRN_PRIO_TASK_ON_FIFO_PROC,		{ LQIO::error_severity::WARNING,  "with priority is running on processor \"%s\" which does not have priority scheduling" } },
	    { LQIO::WRN_PROCESSOR_HAS_NO_TASKS,		{ LQIO::error_severity::WARNING,  "has no tasks" } },
	    { LQIO::WRN_QUANTUM_SCHEDULING,		{ LQIO::error_severity::WARNING,  "using \"%s\" scheduling has a non-zero quantum specified" } },
	    { LQIO::WRN_SCHEDULING_NOT_SUPPORTED,	{ LQIO::error_severity::WARNING,  "with %s scheduling is not supported" } },
	    { LQIO::WRN_TASK_HAS_VISIT_PROBABILITY,	{ LQIO::error_severity::WARNING,  "is not a reference task; visit probabilities for entries are ignored." } },
	    { LQIO::WRN_XXXX_DEFINED_BUT_ZERO,		{ LQIO::error_severity::WARNING,  "has %s defined, but its value is zero" } },
	};
	
	void DocumentObject::input_error( unsigned code, ... ) const
	{
	    const error_message_type& error = __error_messages.at(code);
	    const std::string buf = inputErrorPreamble( code );

	    va_list args;
	    va_start( args, code );
	    vfprintf( stderr, buf.c_str(), args );
	    va_end( args );

	    if ( LQIO::io_vars.severity_action != nullptr ) LQIO::io_vars.severity_action( error.severity );
	}
	
	void DocumentObject::runtime_error( unsigned code, ... ) const
	{
	    const error_message_type& error = __error_messages.at(code);

	    if ( !output_error_message( error.severity ) ) return;

	    const std::string buf = runtimeErrorPreamble( code );
	    
	    va_list args;
	    va_start( args, code );
	    vfprintf( stderr, buf.c_str(), args );
	    va_end( args );

	    if ( LQIO::io_vars.severity_action != nullptr ) LQIO::io_vars.severity_action( error.severity );
	}

    	void DocumentObject::throw_invalid_parameter( const std::string& parameter, const std::string& error ) const
	{
	    runtime_error( LQIO::ERR_INVALID_PARAMETER, parameter.c_str(), error.c_str() );
	    throw std::domain_error( std::string( "invalid " ) + parameter + ": " + error );
	}

	std::string DocumentObject::inputErrorPreamble( unsigned int code ) const
	{
	    const error_message_type& error = __error_messages.at(code);

	    std::string object_name = getTypeName();
	    if ( dynamic_cast<const Task *>(this) && dynamic_cast<const Task *>(this)->getSchedulingType() == SCHEDULE_CUSTOMER ) {
		object_name.insert( 0, "Reference " );
	    } else {
		object_name[0] = std::toupper( object_name[0] );
	    }
	    std::string buf = LQIO::DOM::Document::__input_file_name.string() + ":" + std::to_string(srvnlineno)
		+ ": " + severity_table.at(error.severity) 
		+ ": " + object_name + " \"" + getName() + "\" " + error.message;
	    if ( code == LQIO::ERR_DUPLICATE_SYMBOL && getLineNumber() != static_cast<size_t>(srvnlineno) ) {
		buf += std::string( " at line " ) + std::to_string(getLineNumber());
	    }
	    buf += std::string( ".\n" );
	    return buf;
	}

	std::string DocumentObject::runtimeErrorPreamble( unsigned int code ) const
	{
	    const error_message_type& error = __error_messages.at(code);

	    std::string object_name = getTypeName();
	    if ( dynamic_cast<const Task *>(this) && dynamic_cast<const Task *>(this)->getSchedulingType() == SCHEDULE_CUSTOMER ) {
		object_name.insert( 0, "Reference " );
	    } else {
		object_name[0] = std::toupper( object_name[0] );
	    }
	    std::string buf = LQIO::DOM::Document::__input_file_name.string() + ":" + std::to_string(getLineNumber())
		+ ": " + severity_table.at(error.severity)
		+ ": " + object_name + " \"" + getName() + "\" " + error.message + ".\n";
	    return buf;
	}


	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	const std::string& DocumentObject::getName() const
	{
	    return _name;
	}

	void DocumentObject::setName(const std::string& newName)
	{
	    /* Set the new entity name */
	    _name = newName;
	}

	const std::string& DocumentObject::getComment() const
	{
	    return _comment;
	}

	void DocumentObject::setComment( const std::string& comment )
	{
	    _comment = comment;
	}

	bool DocumentObject::hasResults() const
	{
	    return getDocument()->hasResults();
	}

	void DocumentObject::subclass() const
	{
	    throw should_implement( getTypeName() );
	}

	error_severity DocumentObject::getSeverity( unsigned code )
	{
	    return __error_messages.at(code).severity;
	}

	error_severity DocumentObject::setSeverity( unsigned code, error_severity severity )
	{
	    error_severity previous = __error_messages.at(code).severity;
	    __error_messages.at(code).severity = severity;
	    return previous;
	}

	const ExternalVariable * DocumentObject::checkIntegerVariable( const ExternalVariable * var, int floor_value ) const
	{
	    /* Check for a valid variable (if set).  Return the var */
	    double value = floor_value;
	    if ( var != nullptr && var->wasSet() && ( var->getValue(value) != true || std::isinf(value) || value != rint(value) || value < floor_value ) ) {
		throw std::domain_error( "invalid integer" );
	    }
	    return var;
	}

	int DocumentObject::getIntegerValue( const ExternalVariable * var, int floor_value ) const
	{
	    /* Return a valid integer */
	    double value = floor_value;
	    if ( var == nullptr ) return floor_value;
	    if ( var->wasSet() != true ) throw std::domain_error( "not set" );
	    if ( var->getValue(value) != true ) throw std::domain_error( "not a number" );	/* Sets value for return! */
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value != rint(value) ) throw std::domain_error( std::string("invalid integer: ") + std::to_string(value)  );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    return value;
	}

	const ExternalVariable * DocumentObject::checkDoubleVariable( const ExternalVariable * var, double floor_value, double ceiling_value ) const
	{
	    /* Check for a valid variable (if set).  Return the var */
	    double value = floor_value;
	    if ( var != nullptr && var->wasSet() && ( var->getValue(value) != true || std::isinf(value) || value < floor_value || (floor_value < ceiling_value && ceiling_value < value) ) ){
		throw std::domain_error( "invalid double" );
	    }
	    return var;
	}

	double DocumentObject::getDoubleValue( const ExternalVariable * var, double floor_value, double ceiling_value ) const
	{
	    /* Return a valid double */
	    double value = floor_value;
	    if ( var == nullptr ) return floor_value;
	    if ( var->wasSet() != true ) throw std::domain_error( "not set" );
	    if ( var->getValue(value) != true ) throw std::domain_error( "not a number" );	/* Sets value for return! */
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    if ( floor_value < ceiling_value && ceiling_value < value ) {
		std::stringstream ss;
		ss << value << " > " << ceiling_value;
		throw std::domain_error( ss.str() );
	    }
	    return value;
	}
    }
}
