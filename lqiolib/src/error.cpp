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
 * $Id: error.cpp 13727 2020-08-04 14:06:18Z greg $
 *
 */
#include "error.h"
#include <string.h>
#include <cstdarg>
#include "input.h"
#include "dom_document.h"

namespace LQIO {

    /*
     * List of severity messages.
     */

    const char * severity_table[] =
    {
	"no error",
	"warning",
	"advisory",
	"error",
	"fatal error"
    };
	
    /*
     *	Error(): print error message
     */

    void
    solution_error( unsigned err, ... )
    {
	va_list args;
	va_start( args, err );
	verrprintf( stderr, 
		    LQIO::io_vars.error_messages[err].severity, 
		    LQIO::DOM::Document::__input_file_name.c_str(), 0, 0,
		    LQIO::io_vars.error_messages[err].message, 
		    args );
	va_end( args );
    }



    /*
     * Error(): print error message
     */

    void
    internal_error( const char * filename, const unsigned lineno, const char * fmt, ... )
    {
	va_list args;
	va_start( args, fmt );
	verrprintf( stderr, FATAL_ERROR, filename, lineno, 0, fmt, args );
	va_end( args );
    }



    void
    input_error( const char * fmt, ... )
    {
	va_list args;
	va_start( args, fmt );
	verrprintf( stderr, RUNTIME_ERROR, LQIO::DOM::Document::__input_file_name.c_str(), LQIO_lineno, 0, fmt, args );
	va_end( args );
    }



    void
    input_error2( unsigned err, ... )
    {
	va_list args;
	va_start( args, err );
	verrprintf( stderr, LQIO::io_vars.error_messages[err].severity, LQIO::DOM::Document::__input_file_name.c_str(), LQIO_lineno, 0,
		    LQIO::io_vars.error_messages[err].message, args );
	va_end( args );
    }


    void
    verrprintf( FILE * output, const severity_t level, const char * file_name, unsigned int linenumber, unsigned int column, const char * fmt, va_list args )
    {
	if ( level >= LQIO::io_vars.severity_level ) {
	    if ( file_name && strlen( file_name ) > 0 ) {
		(void) fprintf( output, "%s:", file_name );
		if ( linenumber ) {
		    (void) fprintf( output, "%d:", linenumber );
		}
	    } else {
		(void) fprintf( output, "%s: ", LQIO::io_vars.toolname() );
	    }
	    (void) fprintf( output, " %s: ", severity_table[level] );
	    if ( args ) {
		(void) vfprintf( output, fmt, args );
		(void) fprintf( output, "\n" );
	    } else {
		(void) fprintf( output, "%s\n", fmt );	/* Don't interpret % in fmt because there are no args. */
	    }

	    if ( LQIO::io_vars.severity_action != nullptr ) LQIO::io_vars.severity_action( level );
	}
    }
}
