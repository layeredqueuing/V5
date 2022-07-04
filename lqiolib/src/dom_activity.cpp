/*
 *  $Id: dom_activity.cpp 15751 2022-07-04 21:39:22Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cstdarg>
#include "dom_activity.h"
#include "dom_actlist.h"
#include "dom_task.h"
#include "dom_document.h"
#include "glblerr.h"

namespace LQIO {
    namespace DOM {

	const char * Activity::__typeName = "activity";
	
	Activity::Activity(const Document * document, const std::string& name)
	    : Phase(document,nullptr),
	      _replyList(), _isSpecified(false), _task(nullptr),
	      _outputList(nullptr), _inputList(nullptr),
	      _resultCVSquared(0.0),
	      _resultThroughput(0.0), _resultThroughputVariance(0.0),
	      _resultProcessorUtilization(0.0), _resultProcessorUtilizationVariance(0.0)
	{
	    setName(name);
	}

	Activity::Activity( const LQIO::DOM::Activity & src )
	    : Phase( src ),
	      _replyList(), _isSpecified(src.isSpecified()), _task(src._task),
	      _outputList(nullptr), _inputList(nullptr),
	      _resultCVSquared(0.0),
	      _resultThroughput(0.0), _resultThroughputVariance(0.0),
	      _resultProcessorUtilization(0.0), _resultProcessorUtilizationVariance(0.0)
	{
	}

      
	Activity::~Activity()
	{
	}

	/*
	 * Error detected during input processing.  Line number if found from parser.
	 */
	
	void Activity::input_error( unsigned code, ... ) const
	{
	    const error_message_type& error = DocumentObject::__error_messages.at(code);
	    std::string buf = LQIO::DOM::Document::__input_file_name + ":" + std::to_string(LQIO_lineno)
		+ ": " + severity_table.at(error.severity)
		+ ": Task \"" + getTask()->getName() + "\", activity \"" + getName() + "\" " + error.message;
	    if ( code == LQIO::ERR_DUPLICATE_SYMBOL && getLineNumber() != LQIO_lineno ) {
		buf += std::string( " at line " ) + std::to_string(getLineNumber());
	    }
	    buf += std::string( ".\n" );

	    va_list args;
	    va_start( args, code );
	    vfprintf( stderr, buf.c_str(), args );
	    va_end( args );

	    if ( LQIO::io_vars.severity_action != nullptr ) LQIO::io_vars.severity_action( error.severity );
	}

	/*
	 * Error detected during runtime.  Line number is found from object.
	 */
	
	void Activity::runtime_error( unsigned code, ... ) const
	{
	    const error_message_type& error = __error_messages.at(code);

	    if ( !output_error_message( error.severity ) ) return;

	    std::string object_name = getTypeName();
	    object_name[0] = std::toupper( object_name[0] );
	    std::string buf = LQIO::DOM::Document::__input_file_name + ":" + std::to_string(getLineNumber())
		+ ": " + severity_table.at(error.severity)
		+ ": Task \"" + getTask()->getName() + "\", activity \"" + getName() + "\" " + error.message + ".\n";
	    
	    va_list args;
	    va_start( args, code );
	    vfprintf( stderr, buf.c_str(), args );
	    va_end( args );

	    if ( LQIO::io_vars.severity_action != nullptr ) LQIO::io_vars.severity_action( error.severity );
	}

    
	std::vector<DOM::Entry*>& Activity::getReplyList() 
	{
	    /* Returns the ReplyList of the Activity */
	    return _replyList;
	}

	const bool Activity::isSpecified() const
	{
	    /* Returns the IsSpecified of the Activity */
	    return _isSpecified;
	}

	void Activity::setIsSpecified(const bool isSpecified)
	{
	    /* Stores the given IsSpecified of the Activity */
	    _isSpecified = isSpecified;
	}

	const Task * Activity::getTask() const
	{
	    return _task;
	}

	void Activity::setTask( Task * task ) 
	{
	    _task = task;
	}
		
	void Activity::outputTo(ActivityList* outputList)
	{
	    if (_outputList != nullptr && outputList != nullptr) {
		input_error( ERR_DUPLICATE_ACTIVITY_LVALUE, _outputList->getLineNumber() );
	    } else {
		_outputList = outputList;
	    }
	}

	void Activity::inputFrom(ActivityList* inputList)
	{
	    if (_inputList != nullptr && inputList != nullptr) {
		input_error( ERR_DUPLICATE_ACTIVITY_RVALUE, _inputList->getLineNumber() );
	    } else if ( isStartActivity() ) {
		input_error( ERR_IS_START_ACTIVITY );
	    } else {
		_inputList = inputList;
	    }
	}

	double Activity::getResultSquaredCoeffVariation() const
	{
	    /* Returns the ResultCVSquared of the Activity */
	    return _resultCVSquared;
	}

	Activity& Activity::setResultSquaredCoeffVariation(const double resultCVSquared)
	{
	    /* Stores the given ResultCVSquared of the Activity */
	    _resultCVSquared = resultCVSquared;
	    return *this;
	}

	double Activity::getResultThroughput() const
	{
	    /* Returns the ResultThroughput of the Class */
	    return _resultThroughput;
	}

	Activity& Activity::setResultThroughput(const double resultThroughput)
	{
	    /* Stores the given ResultThroughput of the Class */
	    _resultThroughput = resultThroughput;
	    return *this;
	}

	double Activity::getResultThroughputVariance() const
	{
	    return _resultThroughputVariance;
	}

	Activity& Activity::setResultThroughputVariance(const double resultThroughputVariance)
	{
	    /* Stores the given ResultThroughput of the Class */
	    _resultThroughputVariance = resultThroughputVariance;
	    return *this;
	}

	double Activity::getResultProcessorUtilization() const
	{
	    /* Returns the ResultProcessorUtilization of the Activity */
	    return _resultProcessorUtilization;
	}

	Activity& Activity::setResultProcessorUtilization(const double resultProcessorUtilization)
	{
	    /* Stores the given ResultProcessorUtilization of the Activity */
	    _resultProcessorUtilization = resultProcessorUtilization;
	    return *this;
	}

	double Activity::getResultProcessorUtilizationVariance() const
	{
	    /* Stores the given ResultProcessorUtilization of the Activity */
	    return _resultProcessorUtilizationVariance;
	}

	Activity& Activity::setResultProcessorUtilizationVariance(const double resultProcessorUtilizationVariance)
	{
	    /* Stores the given ResultProcessorUtilization of the Activity */
	    _resultProcessorUtilizationVariance = resultProcessorUtilizationVariance;
	    return *this;
	}
    }
}
