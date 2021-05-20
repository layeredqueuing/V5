/* pragma.cc	-- Greg Franks Tue Sep  1 2009
 *
 * ------------------------------------------------------------------------
 * $Id: pragma.cc 14655 2021-05-17 15:16:53Z greg $
 * ------------------------------------------------------------------------
 */

#include "petrisrvn.h"
#include "pragma.h"
#include <iomanip>
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
#include <lqio/glblerr.h>

Pragma * Pragma::__pragmas = nullptr;
const std::map<const std::string,Pragma::fptr> Pragma::__set_pragma = {
    { LQIO::DOM::Pragma::_processor_scheduling_,	&Pragma::set_processor_scheduling },
    { LQIO::DOM::Pragma::_reschedule_on_async_send_, 	&Pragma::set_reschedule_on_async_send },
    { LQIO::DOM::Pragma::_severity_level_, 		&Pragma::set_severity_level },
    { LQIO::DOM::Pragma::_stop_on_message_loss_,	&Pragma::set_stop_on_message_loss },
    { LQIO::DOM::Pragma::_task_scheduling_,		&Pragma::set_task_scheduling },
    { LQIO::DOM::Pragma::_spex_header_, 		&Pragma::set_spex_header }
};

#define N_SCHEDULING_MODELS	4

Pragma::Pragma() :
    _processor_scheduling(SCHEDULE_FIFO),
    _reschedule_on_async_send(false),
    _severity_level(LQIO::NO_ERROR),
    _spex_header(true),
    _stop_on_message_loss(false),
    _task_scheduling(SCHEDULE_FIFO),
    _default_processor_scheduling(true),
    _default_task_scheduling(true)
{
}

void
Pragma::set( const std::map<std::string,std::string>& list )
{
    if ( __pragmas != nullptr ) delete __pragmas;
    __pragmas = new Pragma();

    for ( std::map<std::string,std::string>::const_iterator i = list.begin(); i != list.end(); ++i ) {
	const std::string& param = i->first;
	const std::map<std::string,fptr>::const_iterator j = __set_pragma.find(param);
	if ( j != __set_pragma.end() ) {
	    try {
		fptr f = j->second;
		(__pragmas->*f)(i->second);
	    }
	    catch ( const std::domain_error& e ) {
		LQIO::solution_error( LQIO::WRN_PRAGMA_ARGUMENT_INVALID, param.c_str(), e.what() );
	    }
	}
    }
}

/* ------------------------------------------------------------------------ */

void
Pragma::set_processor_scheduling( const std::string& value )
{
    _processor_scheduling = str_to_scheduling_type( value );
    _default_processor_scheduling = false;
}

void
Pragma::set_reschedule_on_async_send( const std::string& value )
{
    _reschedule_on_async_send = LQIO::DOM::Pragma::isTrue( value );
}

void
Pragma::set_severity_level(const std::string& value)
{
    _severity_level = LQIO::DOM::Pragma::getSeverityLevel( value );
}

void
Pragma::set_spex_header( const std::string& value )
{
    _spex_header = LQIO::DOM::Pragma::isTrue( value );
}

void
Pragma::set_stop_on_message_loss( const std::string& value )
{
    _stop_on_message_loss = LQIO::DOM::Pragma::isTrue( value );
}

void
Pragma::set_task_scheduling( const std::string& value )
{
    _task_scheduling = str_to_scheduling_type( value );
    _default_task_scheduling = false;
}

/*
 * Print out available pragmas.
 */

void
Pragma::usage( std::ostream& output )
{
    output << "Valid pragmas:" << std::endl;
    std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );

    for ( std::map<const std::string,Pragma::fptr>::const_iterator p = Pragma::__set_pragma.begin(); p != Pragma::__set_pragma.end(); ++p  ) {
	output << "\t" << std::setw(20) << p->first;
	const std::set<const std::string>* args = LQIO::DOM::Pragma::getValues( p->first );
	if ( args && args->size() > 1 ) {
	    output << " = {";

	    for ( std::set<const std::string>::const_iterator q = args->begin(); q != args->end(); ++q ) {
		if ( q != args->begin() ) output << ",";
		output << *q;
	    }
	    output << "}" << std::endl;
	} else {
	    output << " = <arg>" << std::endl;
	}
    }
    output.setf( flags );
}



/*
 * Convert aStr to scheduling type.
 */

scheduling_type
Pragma::str_to_scheduling_type( const std::string& s )
{
    for ( unsigned i = 0; i < N_SCHEDULING_TYPES; ++i ) {
	if ( s.compare( ::scheduling_label[i].XML ) == 0 ) {
	    return static_cast<scheduling_type>(i);
	}
    }
    throw std::domain_error( s );
}
