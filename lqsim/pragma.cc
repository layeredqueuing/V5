/* pragma.cc	-- Greg Franks Tue Sep  1 2009
 *
 * ------------------------------------------------------------------------
 * $Id: pragma.cc 12147 2014-09-29 17:10:36Z greg $
 * ------------------------------------------------------------------------
 */

#include "lqsim.h"
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <algorithm>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include "pragma.h"

using namespace std;

std::map<const char *, Pragma::pragma_info, lt_str> Pragma::__pragmas;

#define N_SCHEDULING_MODELS	4

static const char * scheduling_model_str[] = {
    "default",
    "custom",
    "default-natural",
    "custom-natural",
    0
};

Pragma::Pragma()
    : _abort_on_dropped_message(true),  /* halt on dropped msgs.	*/
      _quorum_delayed_calls(false),	/* Quorum reply (BUG_311)	*/
      _reschedule_on_async_send(false),	/* force schedule after snr.	*/
      _scheduling_model(SCHEDULE_SLICE)
{
}

Pragma&
Pragma::operator=( const Pragma& src )
{
    _abort_on_dropped_message = src._abort_on_dropped_message;
    _quorum_delayed_calls     = src._quorum_delayed_calls;
    _reschedule_on_async_send = src._reschedule_on_async_send;
    _scheduling_model         = src._scheduling_model;
    return *this;
}

void
Pragma::operator()( const char * p )
{
    if ( !p ) return;

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
	(*this)( param, value );
    } while ( *p++ == ',' );
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


bool
Pragma::set_scheduling_model( const string& value )
{
    _scheduling_model = str_to_scheduling_type( value, SCHEDULE_SLICE );
    if ( _scheduling_model < 0 ) {
	(void) fprintf( stderr, "%s: #Pragma scheduling=%s is not supported, \"keep-all\" assumed\n", io_vars.lq_toolname, value.c_str() );
	_scheduling_model = SCHEDULE_SLICE;
	return false;
    }
    return true;
}

const char *
Pragma::get_scheduling_model() const
{
    return scheduling_model_str[_scheduling_model];
}

bool
Pragma::set_reschedule_on_async_send( const string& value )
{
    _reschedule_on_async_send = true_or_false( value );
    return true;
}

const char *
Pragma::get_reschedule_on_async_send() const
{
    return _reschedule_on_async_send ? "true" : "false";
}

bool
Pragma::set_abort_on_dropped_message( const string& value )
{
    _abort_on_dropped_message = true_or_false( value );
    return true;
}

const char * 
Pragma::get_abort_on_dropped_message() const
{
    return _abort_on_dropped_message ? "true" : "false";
}


bool
Pragma::set_nice( const string& value )
{
    nice_value = atoi( value.c_str() );
    if ( nice_value < 0 || 20 < nice_value  ) {
	(void) fprintf( stderr, "%s: #pragma nice=%s is invalid.\n", io_vars.lq_toolname, value.c_str() );
	nice_value = 20;
	return false;
    } else {
	_nice_value = value;
	return true;
    }
}

const char * 
Pragma::get_nice() const 
{
    return _nice_value.c_str();
}


bool
Pragma::set_quorum_delayed_calls( const string& value ) 
{
    _quorum_delayed_calls = true_or_false( value );
    return true;
}

const char * 
Pragma::get_quorum_delayed_calls() const 
{
    return _quorum_delayed_calls ? "true" : "false";
}


bool
Pragma::set_xml_schema( const std::string& value )
{
    return true;
}

const char * 
Pragma::get_xml_schema() const
{
    return 0;
}

void
Pragma::initialize()
{
    if ( __pragmas.size() > 0 ) return;

    __pragmas["nice"] =                     pragma_info( NICE,                 &Pragma::set_nice,    	       	      &Pragma::get_nice,                     &Pragma::eq_nice );
    __pragmas["quorum-reply"] =             pragma_info( PROCESSOR_SCHEDULING, &Pragma::set_quorum_delayed_calls,     &Pragma::get_quorum_delayed_calls,     &Pragma::eq_quorum_delayed_calls );
    __pragmas["reschedule-on-async-send"] = pragma_info( QUORUM_REPLY,         &Pragma::set_reschedule_on_async_send, &Pragma::get_reschedule_on_async_send, &Pragma::eq_reschedule_on_async_send );
    __pragmas["scheduling"] =               pragma_info( RESCHEDULE_ON_SNR,    &Pragma::set_scheduling_model,         &Pragma::get_scheduling_model,         &Pragma::eq_scheduling_model );
    __pragmas["stop-on-message-loss"] =     pragma_info( STOP_ON_MESSAGE_LOSS, &Pragma::set_abort_on_dropped_message, &Pragma::get_abort_on_dropped_message, &Pragma::eq_abort_on_dropped_message );
    __pragmas["xml-schema"] =		    pragma_info( XML_SCHEMA,	       &Pragma::set_xml_schema,		      &Pragma::get_xml_schema,		     &Pragma::eq_xml_schema );
}



/*
 * Print out available pragmas.
 */

void
Pragma::usage(void)
{
    (void) fprintf( stderr, "Valid pragmas:\n" );

    for ( std::map<const char *, Pragma::pragma_info>::const_iterator p = Pragma::__pragmas.begin(); p != Pragma::__pragmas.end(); ++p  ) {
	unsigned j;

	(void) fprintf( stderr, "\t%12s = ", p->first );
	switch ( p->second._p ) {
	case PROCESSOR_SCHEDULING:
	    (void) fprintf( stderr, "{" );
	    for ( j = 0; j < N_SCHEDULING_MODELS; ++j ) {
		if ( j > 0 ) {
		    (void) fputc( ',', stderr );
		}
		(void) fprintf( stderr, "%s", scheduling_model_str[j] );
	    }
	    (void) fprintf( stderr, "}" );
	    break;

	case STOP_ON_MESSAGE_LOSS:
	case RESCHEDULE_ON_SNR:
	case QUORUM_REPLY:
	    (void) fprintf( stderr, "{true,false}" );
	    break;

	case NICE:
	    (void) fprintf( stderr, "<nn>" );
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

int
Pragma::str_to_scheduling_type( const string& s, int default_sched )
{
    if ( s.size() == 0 ) {
	return default_sched;
    } else {
	for ( unsigned i = 0; i < N_SCHEDULING_MODELS; ++i ) {
	    if ( s.compare( scheduling_model_str[i] ) == 0 ) {
		return i;
	    }
	}
    }
    return -1;
}



/*
 * Convert s to scheduling type.
 */

bool
Pragma::true_or_false( const string& s ) const
{
    return s.compare( "true" ) == 0
	|| s.compare( "TRUE" )  == 0
	|| s.compare( "yes" ) == 0
	|| s.compare( "YES" ) == 0;
}



