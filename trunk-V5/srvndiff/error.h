/* -*- C++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 1991.								*/
/************************************************************************/

/*
 * $Id$
 */

#if	!defined(SRVNIO_LIB_ERROR_H)
#define	SRVNIO_LIB_ERROR_H

/*
 *	The user of error.c must supply several things:
 *
 *	1) A list of error codes.  This list must start at one and contain an
 *	   entry for each unique error.  An enum statement is good for this.
 *	   This list of error codes is used to index the error message list. (More
 *	   on the latter later)  The global variable "max_error" must be set to
 *	   maximum error number.
 *
 *	2) A list of severity codes.  This list should start at zero and contain
 *	   an entry for each distinct severity of error.  An enum statement is
 *	   good for this.  The severity at zero indicates that no error occured.
 *	   These codes are used to (a) index a severity table and (b) specify
 *	   an action to be carried out after the printing of a message. This
 *	   is discussed furthur below.
 *
 *	3) A table of error messages in the form of an array of "struct
 *	   error_message_type" which includes an unsigned  severity code 
 *	   and the message text itself itself.  This array is indexed by the
 *	   error codes.
 *
 *	4) A table of severity labels.  This is an array of strings which contains
 *	   text which informs the user of the severity of the error.  An example
 *	   might be "fatal error".  This array is indexed by the severity codes.
 *
 *	5) A function which takes as an argument a severity code and performs an
 *	   action based on this code.  For example, an action for a "fatal error"
 *	   could be "abort()".  Switch statements are ideal for this.
 *
 *	6) The name of an input file.  This is the filename for a file that's
 *	   being read (perhaps an error occured during the reading).  Using the
 *	   the name of the program is also an alternative.
 *
 *	7) A variable of type unsigned called anError.  This variable is
 *	   modified by yyerror() as a side effect.
 */

#include <cstdio>
#include <cstdarg>
#include <stdexcept>

extern const char * input_file_name;

namespace LQIO {


    /*
     * The following list are the error codes which MUST be implemented by the user
     * if either the input.h or symtbl.h modules are to be used.
     *
     * enum {
     *      NER_NO_ERROR,
     *      FTL_NO_MEMORY,
     *      ERR_SYMBOL_TABLE_FULL,
     *      ERR_TOO_MANY_X,
     *      ERR_UNDEFINED_PROCESSOR,
     *      ERR_NO_ENTRIES_DEFINED,
     *      ERR_TOO_MANY_ENTRIES_FOR_REF_TASK
     * }
     *
     * max_error = ERR_TOO_MANY_ENTRIES_FOR_REF_TASK;
     */

    /*
     * The following error codes MUST be implemented by the user of error.h if
     * if the data and functions defined in "input.h" or "symtbl.h" are used.
     * This refers to #2 on the list.
     *
     * enum {
     *      NO_ERROR,
     *      WARNING_ONLY,
     *      ADVISORY_ONLY,
     *      RUNTIME_ERROR,
     *      FATAL_ERROR
     * } severity;
     *
     */

    typedef enum error_severity {
	NO_ERROR,
	WARNING_ONLY,
	ADVISORY_ONLY,
	RUNTIME_ERROR,
	FATAL_ERROR
    } severity_t;

    typedef struct error_message_type {     /* This is number three on the above */
	severity_t severity;            /* list of things to provide */
	const char * message;
    } error_message_type;

    /*
     * The following messages must be implemented by the user of error.h if
     * input.h or symtbl.h are to be used.  This is #3 on the list above.
     *
     * struct error_message_type error_messages[] = {
     *      (unsigned)NO_ERROR,     "** NO ERROR **",       -- always define this one
     *      (unsigned)FATAL_ERROR,  "No more memory.",      -- for the symbol table symtbl.h
     *      (unsigned)RUNTIME_ERROR,"Symbol table full.",   -- for symtbl.h
     *      (unsigned)RUNTIME_ERROR,"Number of %s is outside of program limits of (1,%d).",         -- for input.y
     *      (unsigned)RUNTIME_ERROR,"Processor \"%s\" for task \"%s\" has not been defined.",       -- input.y
     *      (unsigned)RUNTIME_ERROR,"No entries have been defined for task \"%s\".",                -- input.y
     *      (unsigned)RUNTIME_ERROR,"Reference task \"%s\" has more than one entry defined.",       -- input.y
     * };
     *
     * This has been moved into input.h
     */

    extern const char *severity_table[];    /* This is item number 4 on the above */
    /* list of requirements. */

    extern severity_t severity_level;


    /*
     * The following is the minimum severity table that should be implemented
     * by the user of error.h if the symtbl.h or input.h modules are to be used.
     *
     * char * severity_table[] = {
     *      "no error",
     *      "error",
     *      "fatal error"
     * };
     */

    /* extern void severity_action();               This is number 5 on the above list */

    /*
     * The following is the minimum severity action function required if
     * the input.h or symtbl.h routines are needed.
     *
     * void
     * severity_action( severity )
     * unsigned severity;
     * {
     *      switch( severity ) {
     *      case FATAL_ERROR:
     *              exit( 1 );
     *              break; *
     *
     *      case RUNTIME_ERROR:
     *              anError = true;
     *              break;
     *
     *      default:
     *              break;
     *      }
     * }
     */

    /*
     *      The following are the functions which are provided by
     *      error.c  The function runtime error, yyerror, and yywarning are
     *      all available provided the above conditions are all met.
     */

    /*
     *      The function runtime_error prints an error message to the
     *      standard output device. The message consists of the following items.
     *
     *      o  The value of the string "input_file_name" within quotes followed
     *         by a colon.
     *
     *      o  The severity (severity_table[error_messages[err].severity]) string
     *         is displayed next, within parentheses surrounded by blanks.
     *
     *      o  The error message (error_messages[err].message) in the format of
     *         "printf()". The arguments "args..." are used in replacing the printf
     *         style place holders.
     */
         

    /* error.c */

    /*
     * yyerror() and yywarning() are both used in same way as "printf()".
     * The format and arguments must be supplied in the same way that
     * they are for printf().
     */


    void init_errmsg();
    void solution_error ( unsigned err, ... );
    void internal_error ( const char * filename, const unsigned lineno, const char *, ... );
    void severity_action ( unsigned severity );
    void yywarning (const char * fmt, ...);
    void yyerror (const char * fmt);
    void verrprintf ( FILE *, const severity_t, const char *, unsigned int, unsigned int, const char *, va_list );
                
    class element_error : public std::runtime_error
    {
    public:
	explicit element_error( const std::string& s ) : runtime_error( s ) {}
    };


    class missing_attribute : public std::runtime_error
    {
    public:
	explicit missing_attribute( const std::string& attribute ) : runtime_error( attribute ) {}
    };


    class unexpected_attribute : public std::runtime_error
    {
    public:
	explicit unexpected_attribute( const std::string& attribute ) : runtime_error( attribute ) {}
    };
}
#endif
