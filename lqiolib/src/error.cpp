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
 * $Id: error.cpp 15735 2022-06-30 03:18:14Z greg $
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
    { LQIO::ERR_NO_OBJECT,			{ LQIO::error_severity::FATAL,    "Model has no %s." } },
    { LQIO::ERR_NOT_A_CONSTANT, 		{ LQIO::error_severity::ERROR,    "%s is not a constant.\n" } },
    { LQIO::ERR_DUPLICATE_SYMBOL,		{ LQIO::error_severity::ERROR,    "%s \"%s\" previously defined." } },
    { LQIO::ERR_LQX_COMPILATION,		{ LQIO::error_severity::ERROR,    "An error occurred while compiling the LQX program found in file: %s." } },
    { LQIO::ERR_LQX_EXECUTION,			{ LQIO::error_severity::ERROR,    "An error occurred executing the LQX program found in file: %s." } },
    { LQIO::ERR_LQX_SPEX,			{ LQIO::error_severity::ERROR,    "Both LQX and SPEX found in file %s.  Use one or the other." } },
    { LQIO::ERR_CANT_OPEN_DIRECTORY,		{ LQIO::error_severity::ERROR,    "Cannot open directory \"%s\" -- %s." } },
    { LQIO::ERR_CANT_OPEN_FILE,			{ LQIO::error_severity::ERROR,    "Cannot open file \"%s\" -- %s." } },
    { LQIO::ERR_INVALID_DECISION_TYPE,		{ LQIO::error_severity::ERROR,    "Decision %s has a decision type %s, which is not supported in this version." } },
    { LQIO::ERR_SRC_EQUALS_DST,			{ LQIO::error_severity::ERROR,    "Destination entry \"%s\" must be different from source entry \"%s\"." } },
    { LQIO::ERR_MISSING_ATTRIBUTE,		{ LQIO::error_severity::ERROR,    "Element \"%s\", missing attribute: \"%s\"."} },
    { LQIO::ERR_UNEXPECTED_ATTRIBUTE,		{ LQIO::error_severity::ERROR,    "Element \"%s\", unexpected attribute: \"%s\"." } },
    { LQIO::ERR_INVALID_ARGUMENT,		{ LQIO::error_severity::ERROR,    "Element \"%s\", invalid argument to attribute: \"%s\"." } },
    { LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES,	{ LQIO::error_severity::ERROR,    "Entry \"%s\" specified as both a lock and a unlock." } },
    { LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES,	{ LQIO::error_severity::ERROR,    "Entry \"%s\" specified as both a signal and a wait." } },
    { LQIO::ERR_MIXED_ENTRY_TYPES,		{ LQIO::error_severity::ERROR,    "Entry \"%s\" specified using both activity and phase methods." } },
    { LQIO::ERR_LQX_VARIABLE_RESOLUTION,	{ LQIO::error_severity::ERROR,    "External variables are present in file \"%s\", but there is no LQX program to resolve them." } },
    { LQIO::ERR_HISTOGRAM_INVALID_MIN,		{ LQIO::error_severity::ERROR,    "Invalid lower range value for histogram of %g." } },
    { LQIO::ERR_INVALID_PROBABILITY,		{ LQIO::error_severity::ERROR,    "Invalid probability of %g." } },
    { LQIO::ERR_HISTOGRAM_INVALID_MAX,		{ LQIO::error_severity::ERROR,    "Invalid upper range value for histogram of %g." } },
    { LQIO::ERR_INVALID_PARAMETER,		{ LQIO::error_severity::ERROR,    "Invalid %s for %s \"%s\": %s." } },
    { LQIO::ERR_NO_GROUP_SPECIFIED,		{ LQIO::error_severity::ERROR,    "No group specified for task \"%s\" running on processor \"%s\" using fair share scheduling." } },
    { LQIO::ERR_NO_QUANTUM_SCHEDULING,		{ LQIO::error_severity::ERROR,    "No quantum is specified for processor \"%s\" with  \"%s\" scheduling."} },
    { LQIO::ERR_NO_REFERENCE_TASKS,		{ LQIO::error_severity::ERROR,    "No reference tasks have been specified in this model." } },
    { LQIO::ERR_NO_RWLOCK,			{ LQIO::error_severity::ERROR,    "No lock or unlock specified for rwlock task \"%s\"." } },
    { LQIO::ERR_NON_REF_THINK_TIME,		{ LQIO::error_severity::ERROR,    "Non-reference task \"%s\" cannot have think time." } },
    { LQIO::ERR_TOO_MANY_X,			{ LQIO::error_severity::ERROR,    "Number of %s is outside of program limits of (1,%d)." } },
    { LQIO::ERR_DUPLICATE_X_LIST,		{ LQIO::error_severity::ERROR,    "Precedence \"%s\" list previously defined." } },
    { LQIO::ERR_REFERENCE_TASK_OPEN_ARRIVALS,	{ LQIO::error_severity::ERROR,    "Reference task \"%s\", entry \"%s\" cannot have open arrival stream." } },
    { LQIO::ERR_REFERENCE_TASK_IS_RECEIVER,	{ LQIO::error_severity::ERROR,    "Reference task \"%s\", entry \"%s\" receives requests." } },
    { LQIO::ERR_REFERENCE_TASK_IS_INFINITE,	{ LQIO::error_severity::ERROR,    "Reference task \"%s\" must have a finite number of copies." } },
    { LQIO::ERR_ASYNC_REQUEST_TO_WAIT,		{ LQIO::error_severity::ERROR,    "Semaphore \"wait\" entry \"%s\" cannot accept send-no-reply requests." } },
    { LQIO::ERR_DUPLICATE_START_ACTIVITY,	{ LQIO::error_severity::ERROR,    "Start activity for entry \"%s\" is already defined.  Activity \"%s\" is a duplicate." } },
    { LQIO::ERR_NOT_DEFINED,			{ LQIO::error_severity::ERROR,    "Symbol \"%s\" not previously defined." } },
    { LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH,	{ LQIO::error_severity::ERROR,    "Task \"%s\" has a cycle in activity graph.  Backtrace is \"%s\"." } },
    { LQIO::ERR_NO_TASK_FOR_ENTRY,		{ LQIO::error_severity::ERROR,    "Task for entry \"%s\" has not been defined." } },
    { LQIO::ADV_LQX_IMPLICIT_SOLVE,		{ LQIO::error_severity::ADVISORY, "No solve() call found in the lqx program in file: %s.  solve() was invoked implicitly." } },
    { LQIO::ADV_SPEX_UNUSED_RESULT_VARIABLE,	{ LQIO::error_severity::ADVISORY, "SPEX result variable \"%s\" was not used as an array or in an observation." } },
    { LQIO::ADV_TOO_MANY_GNUPLOT_VARIABLES,	{ LQIO::error_severity::ADVISORY, "Too many dependent variables to plot from \"%s\" onwards." } },
    { LQIO::WRN_INFINITE_MULTI_SERVER,		{ LQIO::error_severity::WARNING,  "%s \"%s\" is an infinite server with a multiplicity of %d." } },
    { LQIO::WRN_SCHEDULING_NOT_SUPPORTED,	{ LQIO::error_severity::WARNING,  "%s scheduling specified for %s \"%s\" is not supported." } },
    { LQIO::WRN_ENTRY_TYPE_MISMATCH,		{ LQIO::error_severity::WARNING,  "Entry \"%s\" attribute type=\"%s\" - entry type should be \"%s\"." } },
    { LQIO::WRN_NO_SERVICE_TIME,		{ LQIO::error_severity::WARNING,  "Entry \"%s\" has no service time specified for any phase." } },
    { LQIO::WRN_INVALID_SPEX_RESULT_PHASE,	{ LQIO::error_severity::WARNING,  "Invalid phase, %d, specified for SPEX result \"%%%s\" for entry \"%s\"." } },
    { LQIO::WRN_NO_SPEX_OBSERVATIONS,		{ LQIO::error_severity::WARNING,  "No SPEX Observations were specified in the input model." } },
    { LQIO::WRN_MULTIPLE_SPECIFICATION,		{ LQIO::error_severity::WARNING,  "Parameter is specified multiple times." } },
    { LQIO::WRN_PRAGMA_UNKNOWN,			{ LQIO::error_severity::WARNING,  "Pragma \"%s\" is not recognized." } },
    { LQIO::WRN_PRAGMA_ARGUMENT_INVALID,	{ LQIO::error_severity::WARNING,  "Pragma \"%s\": invalid argument \"%s\"." } },
    { LQIO::WRN_QUANTUM_SCHEDULING,		{ LQIO::error_severity::WARNING,  "using \"%s\" scheduling has a non-zero quantum specified." } },
    { LQIO::WRN_NO_SENDS_FROM_REF_TASK,		{ LQIO::error_severity::WARNING,  "Reference task \"%s\" does not send any messages." } },
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
LQIO::input_error2( unsigned err, ... )
{
    va_list args;
    va_start( args, err );
    verrprintf( stderr, error_messages.at(err).severity, LQIO::DOM::Document::__input_file_name.c_str(), LQIO_lineno, 0,
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
