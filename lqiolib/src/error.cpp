/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 1991.								*/
/************************************************************************/

/*
 * Error processing for srvn program.
 * Written by Greg Franks.  August, 1991.
 *
 * $Id: error.cpp 16736 2023-06-08 16:11:47Z greg $
 * ------------------------------------------------------------------------
 */

#include "error.h"
#include <string.h>
#include <cstdarg>
#include "input.h"
#include "glblerr.h"
#include "dom_document.h"

/*
 * Types of errors.
 */

const std::map<const LQIO::error_severity, const std::string> LQIO::severity_table =
{
    { error_severity::ALL, "no error" },
    { error_severity::ADVISORY, "advisory" },
    { error_severity::WARNING ,"warning" },
    { error_severity::ERROR, "error" },
    { error_severity::FATAL, "fatal error" }
};

/*
 * Error messages shared
 */

std::map<unsigned int, LQIO::error_message_type> LQIO::error_messages = {
    { LQIO::FTL_INTERNAL_ERROR,			{ LQIO::error_severity::FATAL,    "Internal error." } },
    { LQIO::FTL_NO_MEMORY,			{ LQIO::error_severity::FATAL,    "No more memory." } },
    { LQIO::ERR_ARRIVAL_RATE,			{ LQIO::error_severity::ERROR,    "Arrival rate of %g exceeds service rate of %g at entry \"%s\"." } },
    { LQIO::ERR_CANT_OPEN_DIRECTORY,		{ LQIO::error_severity::ERROR,    "Cannot open directory \"%s\" -- %s." } },
    { LQIO::ERR_CANT_OPEN_FILE,			{ LQIO::error_severity::ERROR,    "Cannot open file \"%s\" -- %s." } },
    { LQIO::ERR_DUPLICATE_SYMBOL,		{ LQIO::error_severity::ERROR,    "%s \"%s\" previously defined." } },
    { LQIO::ERR_HISTOGRAM_INVALID_MAX,		{ LQIO::error_severity::ERROR,    "Invalid upper range value for histogram of %g." } },
    { LQIO::ERR_HISTOGRAM_INVALID_MIN,		{ LQIO::error_severity::ERROR,    "Invalid lower range value for histogram of %g." } },
    { LQIO::ERR_INVALID_ARGUMENT,		{ LQIO::error_severity::ERROR,    "Element \"%s\", invalid argument to attribute: \"%s\"." } },
    { LQIO::ERR_INVALID_DECISION_TYPE,		{ LQIO::error_severity::ERROR,    "Decision %s has a decision type %s, which is not supported in this version." } },
    { LQIO::ERR_INVALID_PARAMETER,		{ LQIO::error_severity::ERROR,    "Invalid %s for %s \"%s\": %s." } },
    { LQIO::ERR_LQX_COMPILATION,		{ LQIO::error_severity::ERROR,    "An error occurred while compiling the LQX program found in file: %s." } },
    { LQIO::ERR_LQX_EXECUTION,			{ LQIO::error_severity::ERROR,    "An error occurred executing the LQX program found in file: %s." } },
    { LQIO::ERR_LQX_SPEX,			{ LQIO::error_severity::ERROR,    "Both LQX and SPEX found in file %s.  Use one or the other." } },
    { LQIO::ERR_LQX_VARIABLE_RESOLUTION,	{ LQIO::error_severity::ERROR,    "External variables are present in file \"%s\", but there is no LQX program to resolve them." } },
    { LQIO::ERR_MISSING_ATTRIBUTE,		{ LQIO::error_severity::ERROR,    "Element \"%s\", missing attribute: \"%s\"."} },
    { LQIO::ERR_NOT_A_CONSTANT, 		{ LQIO::error_severity::ERROR,    "%s is not a constant.\n" } },
    { LQIO::ERR_NOT_DEFINED,			{ LQIO::error_severity::ERROR,    "Symbol \"%s\" not previously defined." } },
    { LQIO::ERR_NO_OBJECT,			{ LQIO::error_severity::FATAL,    "Model has no %s." } },
    { LQIO::ERR_NO_REFERENCE_TASKS,		{ LQIO::error_severity::ERROR,    "No reference tasks have been specified in this model." } },
    { LQIO::ERR_REPLICATION,		    	{ LQIO::error_severity::ERROR,    "Fan-out of %d from task \"%s\" with %d replicas does not match the fan-in of %d to task \"%s\" with %d replicas." } },
    { LQIO::ERR_REPLICATION_PROCESSOR,    	{ LQIO::error_severity::ERROR,    "The number of replicas (%d) for task \"%s\" is not an integer multiple of the number of replicas (%d) for processor \"%s\"." } },
    { LQIO::ERR_SRC_EQUALS_DST,			{ LQIO::error_severity::ERROR,    "Destination entry \"%s\" must be different from source entry \"%s\"." } },
    { LQIO::ERR_TASK_HAS_NO_ENTRIES,	 	{ LQIO::error_severity::ERROR, 	  "No entries were defined for task \"%s\"." } },
    { LQIO::ERR_TOO_MANY_X,			{ LQIO::error_severity::ERROR,    "Number of %s is outside of program limits of (1,%d)." } },
    { LQIO::ERR_UNEXPECTED_ATTRIBUTE,		{ LQIO::error_severity::ERROR,    "Element \"%s\", unexpected attribute: \"%s\"." } },
    { LQIO::ADV_LQX_IMPLICIT_SOLVE,		{ LQIO::error_severity::ADVISORY, "No solve() call found in the lqx program in file: %s.  solve() was invoked implicitly." } },
    { LQIO::ADV_SPEX_UNUSED_RESULT_VARIABLE,	{ LQIO::error_severity::ADVISORY, "SPEX result variable \"%s\" was not used as an array or in an observation." } },
    { LQIO::ADV_TOO_MANY_GNUPLOT_VARIABLES,	{ LQIO::error_severity::ADVISORY, "Too many dependent variables to plot from \"%s\" onwards." } },
    { LQIO::WRN_SCHEDULING_NOT_SUPPORTED,	{ LQIO::error_severity::WARNING,  "%s scheduling specified for %s \"%s\" is not supported." } },
    { LQIO::WRN_INVALID_SPEX_RESULT_PHASE,	{ LQIO::error_severity::WARNING,  "Invalid phase, %d, specified for SPEX result \"%%%s\" for entry \"%s\"." } },
    { LQIO::WRN_NO_SPEX_OBSERVATIONS,		{ LQIO::error_severity::WARNING,  "No SPEX Observations were specified in the input model." } },
    { LQIO::WRN_MULTIPLE_SPECIFICATION,		{ LQIO::error_severity::WARNING,  "Parameter is specified multiple times." } },
    { LQIO::WRN_PRAGMA_UNKNOWN,			{ LQIO::error_severity::WARNING,  "Pragma \"%s\" is not recognized." } },
    { LQIO::WRN_PRAGMA_ARGUMENT_INVALID,	{ LQIO::error_severity::WARNING,  "Pragma \"%s\": invalid argument \"%s\"." } },
    { LQIO::ERR_NOT_SUPPORTED,			{ LQIO::error_severity::ERROR,    "The %s feature is not supported in this version." } },
};

/*
 *	Error(): print error message
 */

void
LQIO::runtime_error( unsigned err, ... )
{
    va_list args;
    va_start( args, err );
    verrprintf( stderr, 
		error_messages.at(err).severity, 
		LQIO::DOM::Document::__input_file_name.c_str(), 0, 0,
		error_messages.at(err).message, 
		args );
    va_end( args );
}


/*
 * Error(): print error message
 */

void
LQIO::internal_error( const char * filename, const unsigned lineno, const char * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    verrprintf( stderr, error_severity::FATAL, filename, lineno, 0, fmt, args );
    va_end( args );
}



void
LQIO::input_error( unsigned err, ... )
{
    va_list args;
    va_start( args, err );
    verrprintf( stderr, error_messages.at(err).severity, LQIO::DOM::Document::__input_file_name.c_str(), srvnlineno, 0,
		error_messages.at(err).message, args );
    va_end( args );
}


/*
 * Return true if the message's error_severity is high enough
 */
    
bool
LQIO::output_error_message( error_severity level )
{
    return LQIO::io_vars.severity_level == error_severity::ALL	/* Always */
	|| level == error_severity::FATAL				/* Always */
	|| (LQIO::io_vars.severity_level == error_severity::WARNING && (level == error_severity::WARNING || level == error_severity::ERROR) )
	|| (LQIO::io_vars.severity_level == error_severity::ADVISORY && (level == error_severity::ADVISORY || level == error_severity::ERROR ) )
	|| (LQIO::io_vars.severity_level == error_severity::ERROR && (level == error_severity::ERROR ) );
}

void
LQIO::verrprintf( FILE * output, error_severity level, const char * file_name, unsigned int linenumber, unsigned int column, const char * fmt, va_list args )
{
    if ( !output_error_message( level ) ) return;
    if ( file_name && strlen( file_name ) > 0 ) {
	(void) fprintf( output, "%s:", file_name );
	if ( linenumber ) {
	    (void) fprintf( output, "%d:", linenumber );
	}
    } else {
	(void) fprintf( output, "%s: ", LQIO::io_vars.toolname() );
    }
    (void) fprintf( output, " %s: ", severity_table.at(level).c_str() );
    if ( args ) {
	(void) vfprintf( output, fmt, args );
	(void) fprintf( output, "\n" );
    } else {
	(void) fprintf( output, "%s\n", fmt );	/* Don't interpret % in fmt because there are no args. */
    }

    if ( LQIO::io_vars.severity_action != nullptr ) LQIO::io_vars.severity_action( level );
}
