/* pragma.cc	-- Greg Franks Tue Sep  1 2009
 *
 * ------------------------------------------------------------------------
 * $Id: pragma.cc 17400 2024-10-28 20:52:36Z greg $
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
#include <lqio/dom_pragma.h>
#include <lqio/common_io.h>
#include <lqio/labels.h>
#include <lqio/input.h>
#include <lqio/glblerr.h>
#include "task.h"

Pragma * Pragma::__pragmas = nullptr;
const std::map<const std::string,Pragma::fptr> Pragma::__set_pragma = {
    { LQIO::DOM::Pragma::_disjoint_customers_,		&Pragma::set_disjoint_customers },
    { LQIO::DOM::Pragma::_force_random_queueing_, 	&Pragma::set_force_random_queueing },
    { LQIO::DOM::Pragma::_processor_scheduling_,	&Pragma::set_processor_scheduling },
    { LQIO::DOM::Pragma::_queue_size_,			&Pragma::set_queue_size },
    { LQIO::DOM::Pragma::_reschedule_on_async_send_, 	&Pragma::set_reschedule_on_async_send },
    { LQIO::DOM::Pragma::_save_marginal_probabilities_, &Pragma::set_save_marginal_probabilities },
    { LQIO::DOM::Pragma::_severity_level_, 		&Pragma::set_severity_level },
    { LQIO::DOM::Pragma::_spex_comment_, 		&Pragma::set_spex_comment },
    { LQIO::DOM::Pragma::_spex_convergence_, 		&Pragma::set_spex_convergence },
    { LQIO::DOM::Pragma::_spex_header_, 		&Pragma::set_spex_header },
    { LQIO::DOM::Pragma::_spex_iteration_limit_,	&Pragma::set_spex_iteration_limit },
    { LQIO::DOM::Pragma::_spex_underrelaxation_,	&Pragma::set_spex_underrelaxation },
    { LQIO::DOM::Pragma::_stop_on_message_loss_,	&Pragma::set_stop_on_message_loss },
    { LQIO::DOM::Pragma::_task_scheduling_,		&Pragma::set_task_scheduling }
};

Pragma::Pragma() :
    _disjoint_customers(false),
    _processor_scheduling(SCHEDULE_FIFO),
    _reschedule_on_async_send(false),
    _save_marginal_probabilities(false),
    _severity_level(LQIO::error_severity::ALL),
    _spex_comment(true),
    _spex_convergence(0.0),
    _spex_header(true),
    _spex_iteration_limit(0),
    _spex_underrelaxation(1.0),
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
	const std::map<const std::string,fptr>::const_iterator j = __set_pragma.find(param);
	if ( j != __set_pragma.end() ) {
	    try {
		fptr f = j->second;
		(__pragmas->*f)(i->second);
	    }
	    catch ( const std::domain_error& e ) {
		LQIO::runtime_error( LQIO::WRN_PRAGMA_ARGUMENT_INVALID, param.c_str(), e.what() );
	    }
	}
    }
}

/* ------------------------------------------------------------------------ */

void
Pragma::set_disjoint_customers( const std::string& value )
{
    _disjoint_customers =LQIO::DOM::Pragma::isTrue( value );
}


void
Pragma::set_force_random_queueing( const std::string& value )
{
    const bool isTrue = LQIO::DOM::Pragma::isTrue( value );
    if ( isTrue ) {
	_processor_scheduling = SCHEDULE_RAND;
	_task_scheduling = SCHEDULE_RAND;
    }
    _default_processor_scheduling = !isTrue;
    _default_task_scheduling = !isTrue;
}

void
Pragma::set_processor_scheduling( const std::string& value )
{
    _processor_scheduling = str_to_scheduling_type( value );
    _default_processor_scheduling = false;
}


void
Pragma::set_queue_size( const std::string& value )
{
    char * endptr = nullptr;
    Task::__open_model_tokens = std::strtol( value.c_str(), &endptr, 10 );
    if ( Task::__open_model_tokens < 1 || OPEN_MODEL_TOKENS*2 < Task::__open_model_tokens || *endptr != '\0' ) {
	throw std::domain_error( value );
    }
}

void
Pragma::set_reschedule_on_async_send( const std::string& value )
{
    _reschedule_on_async_send = LQIO::DOM::Pragma::isTrue( value );
}

void
Pragma::set_save_marginal_probabilities( const std::string& value )
{
    _save_marginal_probabilities = LQIO::DOM::Pragma::isTrue( value );
}


void
Pragma::set_severity_level(const std::string& value)
{
    _severity_level = LQIO::DOM::Pragma::getSeverityLevel( value );
}

void
Pragma::set_spex_comment( const std::string& value )
{
    _spex_comment = LQIO::DOM::Pragma::isTrue( value );
}

void
Pragma::set_spex_header( const std::string& value )
{
    _spex_header = LQIO::DOM::Pragma::isTrue( value );
}

void
Pragma::set_spex_convergence( const std::string& value )
{
    char * endptr = nullptr;
    _spex_convergence = std::strtod( value.c_str(), &endptr );
    if ( _spex_convergence <= 0  || *endptr != '\0' ) throw std::domain_error( value );
}

void
Pragma::set_spex_iteration_limit(  const std::string& value )
{
    char * endptr = nullptr;
    _spex_iteration_limit = std::strtol( value.c_str(), &endptr, 10 );
    if ( _spex_iteration_limit < 0 || *endptr != '\0' ) throw std::domain_error( value );
}


void
Pragma::set_spex_underrelaxation( const std::string& value )
{
    char * endptr = nullptr;
    _spex_underrelaxation = std::strtod( value.c_str(), &endptr );
    if ( (_spex_underrelaxation <= 0 || 1 < _spex_underrelaxation ) || *endptr != '\0' ) throw std::domain_error( value );
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
	const std::set<std::string>* args = LQIO::DOM::Pragma::getValues( p->first );
	if ( args && args->size() > 1 ) {
	    output << " = {";

	    for ( std::set<std::string>::const_iterator q = args->begin(); q != args->end(); ++q ) {
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
    static const std::map<const std::string,const scheduling_type> scheduling_pragma = {
	{ LQIO::SCHEDULE::DELAY,  SCHEDULE_DELAY },
	{ LQIO::SCHEDULE::FIFO,   SCHEDULE_FIFO },
	{ LQIO::SCHEDULE::RAND,   SCHEDULE_RAND }
    };
    std::map<const std::string, const scheduling_type>::const_iterator i = scheduling_pragma.find(s);
    if ( i != scheduling_pragma.end() ) return i->second;
    throw std::domain_error( s );
}
