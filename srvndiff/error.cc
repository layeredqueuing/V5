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
 * $Id: error.cc 13200 2018-03-05 22:48:55Z greg $
 *
 */
#include "error.h"
#include <string.h>
#include <string>
#include <cstdarg>

const char * lq_toolname;
const char * input_file_name;

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
	
    severity_t severity_level = NO_ERROR;

    /*
     * Error(): print error message
     */

    void
    internal_error( const char * filename, const unsigned lineno, const char * fmt, ... )
    {
	va_list args;
	va_start( args, fmt );
	verrprintf( stdout, FATAL_ERROR, filename, lineno, 0, fmt, args );
	va_end( args );
    }



    void
    verrprintf( FILE * output, const severity_t level, const char * file_name, unsigned int linenumber, unsigned int column, const char * fmt, va_list args )
    {
	if ( level >= severity_level ) {
	    (void) fprintf( output, " %s: ", lq_toolname );
	    if ( file_name && strlen( file_name ) > 0 ) {
		(void) fprintf( output, "%s:", file_name );
		if ( linenumber ) {
		    (void) fprintf( output, "%d:", linenumber );
		}
	    }
	    (void) fprintf( output, " %s: ", severity_table[level] );
	    if ( args ) {
		(void) vfprintf( output, fmt, args );
		(void) fprintf( output, "\n" );
	    } else {
		(void) fprintf( output, "%s\n", fmt );	/* Don't interpret % in fmt because there are no args. */
	    }
	}
    }

    void
    argument_error( const std::string& attr, const std::string& arg )
    {
	std::string err = attr;
	err += "\"=\"";
	err += arg;
	err += "\"";
	throw std::invalid_argument( err.c_str() );
    }
};
