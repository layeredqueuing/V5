/* pragma.cc	-- Greg Franks Tue Sep  1 2009
 *
 * $HeadURL$
 * ------------------------------------------------------------------------
 * $Id: pragma.cc 13533 2020-03-12 22:09:07Z greg $
 * ------------------------------------------------------------------------
 */

#include "petrisrvn.h"
#include "pragma.h"
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <algorithm>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include <lqio/labels.h>
#include <lqio/input.h>
#include <lqio/expat_document.h>

using namespace std;

std::map<const char *, Pragma::pragma_info, lt_str> Pragma::__pragmas;

#define N_SCHEDULING_MODELS	4

Pragma::Pragma()
    : _processor_scheduling(SCHEDULE_FIFO),
      _reschedule_on_async_send(false),
      _stop_on_message_loss(false),
      _task_scheduling(SCHEDULE_FIFO)
{
}

Pragma&
Pragma::operator=( const Pragma& src )
{
    _processor_scheduling = src._processor_scheduling;
    _reschedule_on_async_send = src._reschedule_on_async_send;
    _stop_on_message_loss = src._stop_on_message_loss;
    _task_scheduling = src._task_scheduling;
    return *this;
}

bool
Pragma::operator()( const char * p )
{
    if ( !p ) return false;

    bool rc = true;
    do {
	while ( isspace( *p ) ) ++p;		/* Skip leading whitespace. */
	string param;
	string value;
	while ( *p && !isspace( *p ) && *p != '=' && *p != ',' ) {
	    param += *p++;			/* get parameter */
	}
	while ( isspace( *p ) ) ++p;
	if ( *p == '=' ) {
	    ++p;
	    while ( isspace( *p ) ) ++p;
	    while ( *p && !isspace( *p ) && *p != ',' ) {
		value += *p++;
	    }
	}
	while ( isspace( *p ) ) ++p;
	if ( ! (*this)( param, value ) ) rc = false;
    } while ( *p++ == ',' );
    return rc;
}


/*
 * Process the pragma string.  The string is automagically freed.
 */

bool
Pragma::operator()( const string& param, const string& value )
{
    initialize();

    std::map<const char *, Pragma::pragma_info, lt_str>::const_iterator p = Pragma::__pragmas.find( param.c_str() );
    if ( p == Pragma::__pragmas.end() ) return false;
    Pragma::set_pragma_fptr set = p->second._set;
    if ( !set ) return true;		/* ignored pragma */
    return (this->*set)( value );
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

bool
Pragma::set_processor_scheduling( const string& value )
{
    try {
	_processor_scheduling = str_to_scheduling_type( value, SCHEDULE_FIFO );
	return true;
    }
    catch ( const invalid_argument& arg ) {
	(void) fprintf( stderr, "%s: #Pragma scheduling=%s is not supported, \"FIFO\" assumed\n", io_vars.lq_toolname, arg.what() );
	_processor_scheduling = SCHEDULE_FIFO;
	return false;
    }
}

const char *
Pragma::get_processor_scheduling() const
{
    return scheduling_label[_processor_scheduling].XML;
}


bool
Pragma::set_reschedule_on_async_send( const string& value )
{
    _reschedule_on_async_send = is_true( value );
    return true;
}

const char *
Pragma::get_reschedule_on_async_send() const
{
    return _reschedule_on_async_send ? "true" : "false";
}

bool
Pragma::set_stop_on_message_loss( const string& value )
{
    _stop_on_message_loss = is_true( value );
    return true;
}

const char * 
Pragma::get_stop_on_message_loss() const
{
    return _stop_on_message_loss ? "true" : "false";
}


bool
Pragma::set_task_scheduling( const string& value )
{
    try {
	_task_scheduling = str_to_scheduling_type( value, SCHEDULE_FIFO );
	return true;
    }
    catch ( const invalid_argument& arg ) {
	(void) fprintf( stderr, "%s: #Pragma scheduling=%s is not supported, \"FIFO\" assumed\n", io_vars.lq_toolname, arg.what() );
	_task_scheduling = SCHEDULE_FIFO;
	return false;
    }
}

const char *
Pragma::get_task_scheduling() const
{
    return scheduling_label[_task_scheduling].XML;
}


bool
Pragma::set_spex_header( const std::string& value )
{
    LQIO::Spex::__no_header = is_true( value );
    return true;
}

const char *
Pragma::get_spex_header() const
{
    return LQIO::Spex::__no_header ? "true" : "false";
}


void
Pragma::initialize()
{
    if ( __pragmas.size() > 0 ) return;

    __pragmas["processor-scheduling"] =	      pragma_info( PROCESSOR_SCHEDULING,       &Pragma::set_processor_scheduling,	&Pragma::get_processor_scheduling,	 &Pragma::eq_processor_scheduling );
    __pragmas["reschedule-on-async-send"] =   pragma_info( RESCHEDULE_ON_ASYNC_SEND,   &Pragma::set_reschedule_on_async_send,	&Pragma::get_reschedule_on_async_send,	 &Pragma::eq_reschedule_on_async_send );
    __pragmas["stop-on-message-loss"] =	      pragma_info( STOP_ON_MESSAGE_LOSS,       &Pragma::set_stop_on_message_loss,	&Pragma::get_stop_on_message_loss,	 &Pragma::eq_stop_on_message_loss );
    __pragmas["task-scheduling"] =	      pragma_info( TASK_SCHEDULING,	       &Pragma::set_task_scheduling,		&Pragma::get_task_scheduling,		 &Pragma::eq_task_scheduling );
    __pragmas["no-header"] =		      pragma_info( SPEX_HEADER,		       &Pragma::set_spex_header,		&Pragma::get_spex_header,		 &Pragma::eq_spex_header );
}



/*
 * Print out available pragmas.
 */

void
Pragma::usage(void)
{
    (void) fprintf( stderr, "Valid pragmas:\n" );

    for ( std::map<const char *, Pragma::pragma_info>::const_iterator p = Pragma::__pragmas.begin(); p != Pragma::__pragmas.end(); ++p  ) {

	(void) fprintf( stderr, "\t%12s = ", p->first );
	switch ( p->second._p ) {
	case TASK_SCHEDULING:
	case PROCESSOR_SCHEDULING:
	    (void) fprintf( stderr, "{fcfs,rand,...}" );
	    break;

	case STOP_ON_MESSAGE_LOSS:
	case RESCHEDULE_ON_ASYNC_SEND:
	case XML_SCHEMA:
	case SPEX_HEADER:
	    (void) fprintf( stderr, "{true,false}" );
	    break;
	}
	(void) fprintf( stderr, "\n" );
    }
}



/*
 * Update the DOM to current state. 
 */

void
Pragma::updateDOM( LQIO::DOM::Document* document ) const
{
    Pragma pragma_default;

    initialize();

    // Reset DOM __pragmass 
    document->clearPragmaList();

    /* General cases */
    std::map<const char *, Pragma::pragma_info, lt_str>::const_iterator param;
    for ( param = Pragma::__pragmas.begin(); param != Pragma::__pragmas.end(); ++param  ) {
	const pragma_info& value = param->second;
	if ( !value._eq || (this->*(value._eq))( pragma_default ) ) continue;		// No change 

	const char * p = (this->*(value._get))();
	if ( p ) {
	    document->addPragma( param->first, p );
	}
    }
}


/*
 * Convert aStr to scheduling type.
 */

scheduling_type
Pragma::str_to_scheduling_type( const string& s, scheduling_type default_sched )
{
    if ( s.size() == 0 ) {
	return default_sched;
    } else {
	for ( unsigned i = 0; i < N_SCHEDULING_TYPES; ++i ) {
	    if ( s.compare( ::scheduling_label[i].XML ) == 0 ) {
		return static_cast<scheduling_type>(i);
	    }
	}
    }
    throw invalid_argument(s);
}



/*
 * Convert s to scheduling type.
 */

bool
Pragma::is_true( const string& s ) 
{
    return s.compare( "true" ) == 0
	|| s.compare( "TRUE" ) == 0
	|| s.compare( "yes" ) == 0
	|| s.compare( "YES" ) == 0;
}



