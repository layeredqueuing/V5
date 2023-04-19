/* -*- c++ -*-
 * $Id: expat_document.cpp 11503 2013-08-30 23:53:06Z greg $
 *
 * Read in JSON input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * September 2013
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <fstream>
#include <iomanip>
#include "json_document.h"
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cassert>
#include <cmath>
#include <fcntl.h>
#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "srvn_results.h"
#include "error.h"
#include "srvndiff.h"

namespace LQIO {
    namespace DOM {

	std::map<const char*,Json_Document::ImportModel,Json_Document::ImportModel>  Json_Document::model_table;
	bool Json_Document::__debugJSON = false;
	int Json_Document::__indent = 0;
    }
}
 
/* -------------------------------------------------------------------- */
/* DOM input.                                                       	*/
/* -------------------------------------------------------------------- */

namespace LQIO {
    namespace DOM {

	Json_Document::Json_Document()
	    : _conf_95( LQIO::ConfidenceIntervals::CONF_95 )
	{
	    init_tables();
	}


	Json_Document::~Json_Document()
	{
	}

	bool
	Json_Document::load( const std::string& input_filename )
	{
	    Json_Document json_document;
	    return json_document.parse( input_filename );
	}

	/*
	 * Parse the XML file, catching any XML exceptions that might
	 * propogate out of it.
	 */

	bool
	Json_Document::parse( const std::string& file_name )
	{
	    struct stat statbuf;
	    bool rc = true;
	    int input_fd = -1;

	    if ( file_name ==  "-" ) {
		input_fd = fileno( stdin );
		input_file_name = lq_toolname;
	    } else if ( ( input_fd = open( file_name.c_str(), O_RDONLY ) ) < 0 ) {
		std::cerr << lq_toolname << ": Cannot open input file " << file_name << " - " << strerror( errno ) << std::endl;
		return false;
	    }

	    if ( isatty( input_fd ) ) {
		std::cerr << lq_toolname << ": Input from terminal is not allowed." << std::endl;
		return false;
	    } else if ( fstat( input_fd, &statbuf ) != 0 ) {
		std::cerr << lq_toolname << ": Cannot stat " << input_file_name << " - " << strerror( errno ) << std::endl;
		return false;
#if defined(S_ISSOCK)
	    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) ) {
#else
	    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) ) {
#endif
		std::cerr << lq_toolname << ": Input from " << input_file_name << " is not allowed." << std::endl;
		return false;
	    }

	    //            initialize();		/* Creates parser */

#if HAVE_MMAP
	    char *buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, input_fd, 0 ));
	    if ( buffer != MAP_FAILED ) {
		std::string err = picojson::parse( _dom, buffer, buffer + statbuf.st_size );
		if ( err.empty() ) {
		    try {
			handleModel();
		    }
		    catch ( std::runtime_error& e ) {
			input_error( "%s - %s", file_name.c_str(), e.what() );
			rc = false;
		    }
		} else {
		    input_error( "%s - %s", file_name.c_str(), err.c_str() );
		    rc = false;
		}
		munmap( buffer, statbuf.st_size );
	    } else {
		std::cerr << lq_toolname << ": Read error on " << input_file_name << " - " << strerror( errno ) << std::endl;
	    }
#else
	    char * buffer = static_cast<char *>(malloc( statbuf.st_size ) );
	    if ( read( input_fd, buffer, statbuf.st_size ) == statbuf.st_size ) {
		std::string err = picojson::parse( _dom, buffer, buffer + statbuf.st_size );
		if ( err.empty() ) {
		    try {
			handleModel();
		    }
		    catch ( std::runtime_error& e ) {
			input_error( "%s - %s", file_name.c_str(), e.what() );
			rc = false;
		    }
		} else {
		    input_error( "%s - %s", file_name.c_str(), err.c_str() );
		    rc = false;
		}
	    } else {
		std::cerr << lq_toolname << ": Read error on " << input_file_name << " - " << strerror( errno ) << std::endl;
	    }
	    free( buffer );
#endif
	    close( input_fd );
	    return rc;
	}

	void
	Json_Document::input_error( const char * fmt, ... ) const
	{
	    va_list args;
	    va_start( args, fmt );
	    verrprintf( stderr, RUNTIME_ERROR, input_file_name,  0, 0, fmt, args );
	    va_end( args );
	}
 
	/* ---------------------------------------------------------------- */
	/* DOM importation						    */
	/* ---------------------------------------------------------------- */

	void
	Json_Document::handleModel()
	{
	    if ( !_dom.is<picojson::object>() ) throw std::runtime_error( "JSON object expected" );
	    const picojson::value::object& obj = _dom.get<picojson::object>();

	    /* Check valid attributes */
	    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		const std::string& attr = i->first;
		std::map<const char *,ImportModel>::const_iterator j = std::find_if( model_table.begin(), model_table.end(), ImportModel::Match( attr ) );
		if ( j != model_table.end() ) {
		    try {
			j->second(attr.c_str(),*this,i->second);		/* Handle attribute */
		    }
		    catch ( missing_attribute& err ) {
			input_error( "Missing attribute \"%s\" for element <%s>", err.what(), attr.c_str() );
		    }
		}
	    }
	}

	void
	Json_Document::handleGeneral( const picojson::value& value )
	{
	    if ( !value.is<picojson::object>() ) {
		throw std::invalid_argument( value.to_str().c_str() );
	    }
	    const picojson::value::object& results = get_object_attribute( Xresult, value.get<picojson::object>() );
	    const int iterations = get_long_attribute( Xiterations, results );
	    if ( iterations > 1 ) {
		_conf_95.set_t_value( iterations );
	    }
	    set_general( get_bool_attribute( Xvalid, results ),
			 get_double_attribute( Xconv_val_result, results ),
			 iterations,
			 0,
			 1 );
	    try {
		add_elapsed_time( get_time_attribute( Xelapsed_time, results ) );
		add_user_time( get_time_attribute( Xuser_cpu_time, results ) );
		add_system_time( get_time_attribute( Xsystem_cpu_time, results ) );
		const std::string& version = get_string_attribute( Xsolver_info, results );
		add_solver_info( version.data() );
	    }
	    catch ( missing_attribute& e ) {
	    }

	    try {
		const picojson::value::object& mva_info = get_object_attribute( Xmva_info, results );
		add_mva_solver_info( get_long_attribute( Xsubmodels, mva_info ),
				     get_long_attribute( Xcore, mva_info ),
				     get_double_attribute( Xstep, mva_info ),
				     get_double_attribute( Xstep_squared, mva_info ),
				     get_double_attribute( Xwait, mva_info ),
				     get_double_attribute( Xwait_squared, mva_info ),
				     get_long_attribute( Xfaults, mva_info ) );
	    }
	    catch ( missing_attribute& e ) {
	    }
	}

	void
	Json_Document::handleProcessor( const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleProcessor( *i );
		}

	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& name = get_string_attribute( Xname, obj );
		    if ( Json_Document::__debugJSON ) std::cerr << begin_object( name );
		    const unsigned int p = find_or_add_processor( name.c_str() );

		    handleProcessorResults( p, get_object_attribute( Xresult, obj ) );		/* throws (up) if no results */
		}
		catch ( missing_attribute& attr ) {
		}

		/* For XML style schema */
		std::map<std::string, picojson::value>::const_iterator group = obj.find( Xgroup );
		if ( group != obj.end() ) {
		    handleGroup( group->second );
		}
		std::map<std::string, picojson::value>::const_iterator task = obj.find( Xtask );
		if ( task != obj.end() ) {
		    handleTask( task->second );
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
	    } else {
		argument_error( Xprocessor, value.to_str() );
	    }
	}


	void
	Json_Document::handleGroup( const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleGroup( *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& name = get_string_attribute( Xname, obj );
		    if ( Json_Document::__debugJSON ) std::cerr << begin_object( name );
		    const unsigned int g = find_or_add_group( name.c_str() );

		    handleGroupResults( g, get_object_attribute( Xresult, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}

		/* For XML style schema */
		std::map<std::string, picojson::value>::const_iterator task = obj.find( Xtask );
		if ( task != obj.end() ) {
		    handleTask( task->second );
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
	    } else {
		argument_error( Xgroup, value.to_str() );
	    }
	}


	void
	Json_Document::handleTask( const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleTask( *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const std::string& task = get_string_attribute( Xname, obj );
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( task );
		try {
		    const unsigned int t = find_or_add_task( task.c_str() );

		    handleTaskResults( t, get_object_attribute( Xresult, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}

		/* For XML style schema */
		std::map<std::string, picojson::value>::const_iterator entry = obj.find( Xentry );
		if ( entry != obj.end() ) {
		    handleEntry( entry->second );
		}
		std::map<std::string, picojson::value>::const_iterator activity = obj.find( Xactivity );
		if ( activity != obj.end() ) {
		    handleActivity( task, activity->second );
		}
		std::map<std::string, picojson::value>::const_iterator precedence = obj.find( Xprecedence );
		if ( precedence != obj.end() ) {
		    handlePrecedence( task, precedence->second );
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();

	    } else {
		argument_error( Xtask, value.to_str() );
	    }
	}



	void
	Json_Document::handleEntry( const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleEntry( *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const std::string& name = get_string_attribute( Xname, obj );
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( name );
		const unsigned int e = find_or_add_entry( name.c_str() );
		try {
		    handleEntryResults( e, get_object_attribute( Xresult, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		
		std::map<std::string, picojson::value>::const_iterator phase = obj.find( Xphase );
		if ( phase != obj.end() ) {
		    handlePhase( e, phase->second );
		}
		try {
		    handleEntryCalls( e, FORWARD, get_value_attribute( Xforwarding, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
		
	    } else if ( !value.is<std::string>() ) {
		argument_error( Xentry, value.to_str() );
	    }
	}


	void
	Json_Document::handlePhase( const unsigned e, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handlePhase( e, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const unsigned int p = get_long_attribute( Xphase, obj );
		if ( p == 0 ) throw missing_attribute( Xphase );

		try {
		    handlePhaseResults( e, p-1, get_object_attribute( Xresult, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		try {
		    handlePhaseCalls( e, p-1, RENDEZVOUS, get_value_attribute( Xsynch_call, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		try {
		    handlePhaseCalls( e, p-1, SEND_NO_REPLY, get_value_attribute( Xasynch_call, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
	    }
	}

	void
	Json_Document::handleTaskActivity( const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleTaskActivity( *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const std::string& task = get_string_attribute( Xtask, obj );
		/* Should have an activity and precedence array */
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( task );
		for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		    if ( i->second.is<picojson::array>() ) {
			if ( strcasecmp( Xactivity, i->first.c_str() ) == 0 ) {
			    handleActivity( task, i->second );
			} else if ( strcasecmp( Xprecedence, i->first.c_str() ) == 0 ) {
			    handlePrecedence( task, i->second );
			}
		    } else if ( !i->second.is<std::string>() ) {
			argument_error( Xactivity, i->second.to_str() );
		    }
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
	    } else {
		argument_error( Xactivity, value.to_str() );
	    }
	}


	void
	Json_Document::handleActivity( const std::string& task, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleActivity( task, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const std::string& activity = get_string_attribute( Xname, obj );
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( activity );
		const unsigned int a = find_or_add_activity( task.c_str(), activity.c_str() );
		try {
		    handleActivityCalls( a, RENDEZVOUS, get_value_attribute( Xsynch_call, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		try {
		    handleActivityCalls( a, SEND_NO_REPLY, get_value_attribute( Xasynch_call, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		try {
		    handleActivityResults( a, get_object_attribute( Xresult, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
	    } else {
		argument_error( Xactivity, value.to_str() );
	    }
	}


	void
	Json_Document::handlePrecedence( const std::string& task, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handlePrecedence( task, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( Xprecedence );
		/* Need the and-join, rest don't have results */
		try {
		    const picojson::value::array& list = get_array_attribute( Xand_join, obj );
		    const char * first = 0;
		    const char * last  = 0;
		    for ( picojson::value::array::const_iterator i = list.begin(); i != list.end(); ++i ) {
			if ( !i->is<std::string>() ) continue;
			if ( i == list.begin() ) {
			    first = i->get<std::string>().c_str();
			} else {
			    last = i->get<std::string>().c_str();
			}
		    }
		    if ( !first || !last ) return;

		    unsigned first_activity = find_or_add_activity( task.c_str(), first );
		    unsigned last_activity  = find_or_add_activity( task.c_str(), last );

		    handlePrecedenceResults( first_activity, last_activity, get_object_attribute( Xresult, obj ) );
		}
		catch ( missing_attribute& attr ) {
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
	    }
	}


	void
	Json_Document::handleEntryCalls( const unsigned e, call_type t, const picojson::value& value )
	{
	    if ( !e ) return;
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleEntryCalls( e, t, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const std::string& name = get_string_attribute( Xname, obj );
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( name );
		const unsigned int d = find_or_add_entry( name.c_str() );
		const picojson::value::object& results = get_object_attribute( Xresult, obj );
		try {
		    switch ( t ) {
		    case FORWARD:
			handleEntryCallResults( e, d, get_value_attribute( Xwaiting, results ), &Json_Document::save_entry_fwd_wait );
			handleEntryCallResults( e, d, get_value_attribute( Xwaiting_variance, results ), &Json_Document::save_entry_fwd_wait_var );
			break;
		    default:
			break;
		    }
		}
		catch ( missing_attribute& ) {
		}
		if ( Json_Document::__debugJSON ) std::cerr << end_object();
	    } else if ( !value.is<std::string>() ) {
		argument_error( Xentry, value.to_str() );
	    }
	}


	void
	Json_Document::handlePhaseCalls( unsigned int e, unsigned int p, call_type t, const picojson::value& value ) const
	{
	    if ( !e || p >= MAX_PHASES ) return;
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handlePhaseCalls( e, p, t, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const std::string& name = get_string_attribute( Xname, obj );
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( name );
		const unsigned int d = find_or_add_entry( name.c_str() );
		const picojson::value::object& results = get_object_attribute( Xresult, obj );
		try {
		    switch ( t ) {
		    case RENDEZVOUS:
			handlePhaseCallResults( e, p, d, get_value_attribute( Xwaiting, results ), &Json_Document::save_phase_rnv_wait );
			handlePhaseCallResults( e, p, d, get_value_attribute( Xwaiting_variance, results ), &Json_Document::save_phase_rnv_wait_var );
			break;
		    case SEND_NO_REPLY:
			handlePhaseCallResults( e, p, d, get_value_attribute( Xwaiting, results ), &Json_Document::save_phase_snr_wait );
			handlePhaseCallResults( e, p, d, get_value_attribute( Xwaiting_variance, results ), &Json_Document::save_phase_snr_wait_var );
			/* Drop probability? */
			break;
		    default:
			abort();
		    }
		}
		catch ( missing_attribute& ) {
		}
	    }
	}



	void
	Json_Document::handleActivityCalls( const unsigned int a, call_type t, const picojson::value& value ) const
	{
	    if ( !a ) return;
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleActivityCalls( a, t, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		const std::string& name = get_string_attribute( Xname, obj );
		if ( Json_Document::__debugJSON ) std::cerr << begin_object( name );
		const unsigned int d = find_or_add_entry( name.c_str() );
		const picojson::value::object& results = get_object_attribute( Xresult, obj );
		try {
		    switch ( t ) {
		    case RENDEZVOUS:
			handleActivityCallResults( a, d, get_value_attribute( Xwaiting, results ), &Json_Document::save_activity_rnv_wait );
			handleActivityCallResults( a, d, get_value_attribute( Xwaiting_variance, results ), &Json_Document::save_activity_rnv_wait_var );
			break;
		    case SEND_NO_REPLY:
			handleActivityCallResults( a, d, get_value_attribute( Xwaiting, results ), &Json_Document::save_activity_snr_wait );
			handleActivityCallResults( a, d, get_value_attribute( Xwaiting_variance, results ), &Json_Document::save_activity_snr_wait_var );
			/* Drop probability? */
			break;
		    default:
			abort();
		    }
		}
		catch ( missing_attribute& ) {
		}
	    }
	}
	
	/* ------------------------------------------------------------------------ */
	/* Functions used to extract results.  Erroneous or superfluous attributes  */
	/* are ignored.								    */
	/* ------------------------------------------------------------------------ */

	bool
	Json_Document::get_results( const picojson::value::object& results, const char * attribute, double& mean, double& conf_95 ) const
	{
	    std::map<std::string, picojson::value>::const_iterator attr = results.find( attribute );
	    if ( attr != results.end() ) {
		const picojson::value& value = attr->second;
		if ( value.is<double>() ) {
		    mean = value.get<double>();
		} else if ( value.is<picojson::object>() ) {
		    const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		    mean = get_double_attribute( Xmean, obj );
		    conf_95 = get_double_attribute( Xconf_95, obj, 0.0 );
		}
		return true;
	    } else {
		return false;
	    }
	}


	void
	Json_Document::handleProcessorResults( unsigned int p, const picojson::value::object& results ) const
	{
	    if ( !p ) return;
	    processor_tab[pass][p].has_results = get_results( results, Xutilization, processor_tab[pass][p].utilization, processor_tab[pass][p].utilization_conf );
	}

	void
	Json_Document::handleGroupResults( unsigned int g, const picojson::value::object& results ) const
	{
	    if ( !g ) return;
	    group_tab[pass][g].has_results = get_results( results, Xutilization, group_tab[pass][g].utilization, group_tab[pass][g].utilization_conf );
	}

	void
	Json_Document::handleTaskResults( unsigned int t, const picojson::value::object& results ) const
	{
	    if ( !t ) return;

            get_results( results, Xthroughput,                 task_tab[pass][t].throughput,                task_tab[pass][t].throughput_conf                );
            get_results( results, Xutilization,                task_tab[pass][t].utilization,               task_tab[pass][t].utilization_conf               );
            get_results( results, Xsemaphore_waiting,          task_tab[pass][t].semaphore_waiting,         task_tab[pass][t].semaphore_waiting_conf         );
            get_results( results, Xsemaphore_utilization,      task_tab[pass][t].semaphore_utilization,     task_tab[pass][t].semaphore_utilization_conf     );
            get_results( results, Xrwlock_reader_waiting,      task_tab[pass][t].rwlock_reader_waiting,     task_tab[pass][t].rwlock_reader_waiting_conf     );
            get_results( results, Xrwlock_reader_holding,      task_tab[pass][t].rwlock_reader_holding,     task_tab[pass][t].rwlock_reader_holding_conf     );
            get_results( results, Xrwlock_reader_utilization,  task_tab[pass][t].rwlock_reader_utilization, task_tab[pass][t].rwlock_reader_utilization_conf );
            get_results( results, Xrwlock_writer_waiting,      task_tab[pass][t].rwlock_writer_waiting,     task_tab[pass][t].rwlock_writer_waiting_conf     );
            get_results( results, Xrwlock_writer_holding,      task_tab[pass][t].rwlock_writer_holding,     task_tab[pass][t].rwlock_writer_holding_conf     );
            get_results( results, Xrwlock_writer_utilization,  task_tab[pass][t].rwlock_writer_utilization, task_tab[pass][t].rwlock_writer_utilization_conf );
	    task_tab[pass][t].has_results = true;
	}

	void
	Json_Document::handleEntryResults( unsigned int e, const picojson::value::object& results ) const
	{
	    if ( !e ) return;
	    double dummy = 0.;	/* ignored value */
	    
	    entry_tab[pass][e].open_arrivals = get_results( results, Xopen_wait_time, entry_tab[pass][e].open_waiting, entry_tab[pass][e].open_wait_conf );
	    entry_tab[pass][e].bounds = get_results( results, Xthroughput, entry_tab[pass][e].throughput, entry_tab[pass][e].throughput_conf );
	    get_results( results, Xthroughput_bound, entry_tab[pass][e].throughput, dummy );

	    /* For case where we have activities... results specified at entry level */

	    try {
		handlePhaseResults( e, get_array_attribute( Xservice_time, results ), save_entry_service );
		handlePhaseResults( e, get_array_attribute( Xservice_time_variance, results ), save_entry_variance );
		handlePhaseResults( e, get_array_attribute( Xproc_waiting, results ), save_entry_proc_waiting );
		handlePhaseResults( e, get_array_attribute( Xphase_utilization, results ), save_entry_utilization );
	    }
	    catch ( missing_attribute& ) {
	    }
	}


	void
	Json_Document::handlePhaseResults( unsigned int e, unsigned int p, const picojson::value::object& results ) const
	{
	    if ( !e || p >= MAX_PHASES ) return;

	    activity_info& phase = entry_tab[pass][e].phase[p];
	    get_results( results, Xservice_time, phase.service, phase.serv_conf );
	    get_results( results, Xservice_time_variance, phase.variance, phase.var_conf );
	    get_results( results, Xproc_waiting, phase.processor_waiting, phase.processor_waiting_conf );
	}

	void
	Json_Document::handlePhaseResults( unsigned int e, const picojson::value::array& results, phs_save_fptr f ) const
	{
	    unsigned int p = 0;
	    for ( picojson::value::array::const_iterator x = results.begin(); x != results.end() && p < MAX_PHASES; ++x, ++p ) {
		(*f)( e, p, *x );
	    }
	}


	void
	Json_Document::handleActivityResults( unsigned int a, const picojson::value::object& results ) const
	{
	    if ( !a ) return;

	    activity_info& act = activity_tab[pass][a];
	    get_results( results, Xservice_time, act.service, act.serv_conf );
	    get_results( results, Xservice_time_variance, act.variance, act.var_conf );
	    get_results( results, Xproc_waiting, act.processor_waiting, act.processor_waiting_conf );
	}

	void
	Json_Document::handlePrecedenceResults( unsigned int first_activity, unsigned int last_activity, const picojson::value::object& results ) const
	{
	    if ( first_activity && last_activity ) {
		get_results( results, Xjoin_waiting,  join_tab[pass][first_activity][last_activity].mean    , join_tab[pass][first_activity][last_activity].mean_conf    );
		get_results( results, Xjoin_variance, join_tab[pass][first_activity][last_activity].variance, join_tab[pass][first_activity][last_activity].variance_conf );
	    }
	}

	/* static */ void
	Json_Document::save_entry_service( unsigned int e, unsigned int p, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][e].phase[p].service = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][e].phase[p].service = get_double_attribute( Xmean, obj );
		entry_tab[pass][e].phase[p].serv_conf = get_double_attribute( Xconf_95, obj, 0.0 );

	    }
	}

	/* static */ void
	Json_Document::save_entry_variance( unsigned int e, unsigned int p, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][e].phase[p].variance = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][e].phase[p].variance = get_double_attribute( Xmean, obj );
		entry_tab[pass][e].phase[p].var_conf = get_double_attribute( Xconf_95, obj );

	    }
	}

	/* static */ void
	Json_Document::save_entry_proc_waiting( unsigned int e, unsigned int p, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][e].phase[p].processor_waiting = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][e].phase[p].processor_waiting = get_double_attribute( Xmean, obj );
		entry_tab[pass][e].phase[p].processor_waiting_conf = get_double_attribute( Xconf_95, obj, 0.0 );

	    }
	}

	/* static */ void
	Json_Document::save_entry_utilization( unsigned int e, unsigned int p, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][e].phase[p].utilization = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][e].phase[p].utilization = get_double_attribute( Xmean, obj );
		entry_tab[pass][e].phase[p].utilization_conf = get_double_attribute( Xconf_95, obj, 0.0 );
	    }
	}

	/* static */ void
	Json_Document::save_entry_fwd_wait( unsigned int entry, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][entry].fwd_to[destination].waiting = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][entry].fwd_to[destination].waiting = get_double_attribute( Xmean, obj );
		entry_tab[pass][entry].fwd_to[destination].wait_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ void
	Json_Document::save_entry_fwd_wait_var( unsigned int entry, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][entry].fwd_to[destination].wait_var = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][entry].fwd_to[destination].wait_var = get_double_attribute( Xmean, obj );
		entry_tab[pass][entry].fwd_to[destination].wait_var_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ void
	Json_Document::save_phase_rnv_wait( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][entry].phase[phase].to[destination].waiting = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][entry].phase[phase].to[destination].waiting = get_double_attribute( Xmean, obj );
		entry_tab[pass][entry].phase[phase].to[destination].wait_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/*static */ void
	Json_Document::save_phase_rnv_wait_var( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][entry].phase[phase].to[destination].wait_var = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][entry].phase[phase].to[destination].wait_var = get_double_attribute( Xmean, obj );
		entry_tab[pass][entry].phase[phase].to[destination].wait_var_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ void
	Json_Document::save_phase_snr_wait( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][entry].phase[phase].to[destination].snr_waiting = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][entry].phase[phase].to[destination].snr_waiting = get_double_attribute( Xmean, obj );
		entry_tab[pass][entry].phase[phase].to[destination].snr_wait_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/*static */ void
	Json_Document::save_phase_snr_wait_var( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		entry_tab[pass][entry].phase[phase].to[destination].snr_wait_var = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		entry_tab[pass][entry].phase[phase].to[destination].snr_wait_var = get_double_attribute( Xmean, obj );
		entry_tab[pass][entry].phase[phase].to[destination].snr_wait_var_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ void
	Json_Document::save_activity_rnv_wait( unsigned int activity, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		activity_tab[pass][activity].to[destination].waiting = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		activity_tab[pass][activity].to[destination].waiting = get_double_attribute( Xmean, obj );
		activity_tab[pass][activity].to[destination].wait_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ void
	Json_Document::save_activity_rnv_wait_var( unsigned int activity, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		activity_tab[pass][activity].to[destination].wait_var = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		activity_tab[pass][activity].to[destination].wait_var = get_double_attribute( Xmean, obj );
		activity_tab[pass][activity].to[destination].wait_var_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ void
	Json_Document::save_activity_snr_wait( unsigned int activity, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		activity_tab[pass][activity].to[destination].snr_waiting = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		activity_tab[pass][activity].to[destination].snr_waiting = get_double_attribute( Xmean, obj );
		activity_tab[pass][activity].to[destination].snr_wait_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ void
	Json_Document::save_activity_snr_wait_var( unsigned int activity, unsigned int destination, const picojson::value& value )
	{
	    if ( value.is<double>() ) {
		activity_tab[pass][activity].to[destination].snr_wait_var = value.get<double>();
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		activity_tab[pass][activity].to[destination].snr_wait_var = get_double_attribute( Xmean, obj );
		activity_tab[pass][activity].to[destination].snr_wait_var_conf = get_double_attribute( Xconf_95, obj );
	    }
	}

	/* static */ const std::string
	Json_Document::get_string_attribute( const char * name, const std::map<std::string, picojson::value>& obj ) 
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		throw missing_attribute( name );
	    } else {
		return attr->second.to_str();
	    }
	}

	/* static */ const long
	Json_Document::get_long_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		throw missing_attribute( name );
	    } else if ( attr->second.is<double>() ) {
		const double value = attr->second.get<double>();
		if ( value >= 0 && rint(value) == value ) {
		    return static_cast<long>(value);
		}
	    } else if ( attr->second.is<std::string>() ) {
		char * end_ptr = 0;
		const double value = strtod( attr->second.get<std::string>().c_str(), &end_ptr );
		if ( value >= 0 && rint(value) == value && ( !end_ptr || *end_ptr == '\0' ) ) {
		    return static_cast<long>(value);
		}
	    }
	    throw std::invalid_argument( attr->second.to_str().c_str() );		// throws.
	    return 0;
	}

	/* static */ const bool
	Json_Document::get_bool_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		throw missing_attribute( name );
	    } else if ( attr->second.is<bool>() ) {
		return attr->second.get<bool>();
	    } else if ( attr->second.is<std::string>() ) {
		const std::string& value = attr->second.get<std::string>();
		return value == "true" || value == "TRUE";
	    }
	    throw std::invalid_argument( attr->second.to_str().c_str() );		// throws.
	    return 0;
	}

	/* static */ const double
	Json_Document::get_double_attribute( const char * name, const std::map<std::string, picojson::value>& obj, double default_value )
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
	        if ( default_value >= 0. ) {
	            return default_value;
                } else {
		    throw missing_attribute( name );
                }
	    } else if ( attr->second.is<double>() ) {
		return attr->second.get<double>();
	    } else if ( attr->second.is<std::string>() ) {
		char * end_ptr = 0;
		const double value = strtod( attr->second.get<std::string>().c_str(), &end_ptr );
		if ( !end_ptr || *end_ptr == '\0' ) {
		    return value;
		}
	    }
	    argument_error( name, attr->second.to_str() );		// throws.
	    return 0;
	}


	/* static */ const picojson::value::object&
	Json_Document::get_object_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		throw missing_attribute( name );
	    } else if ( !attr->second.is<picojson::object>() ) {
		argument_error( name, attr->second.to_str() );		// throws.
	    }
	    return attr->second.get<picojson::object>();
	}



	/* static */ const picojson::value::array&
	Json_Document::get_array_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		throw missing_attribute( name );
	    } else if ( !attr->second.is<picojson::array>() ) {
		argument_error( name, attr->second.to_str() );		// throws.
	    }
	    return attr->second.get<picojson::array>();
	}

	/* static */ const picojson::value&
	Json_Document::get_value_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		throw missing_attribute( name );
	    }
	    return attr->second;
	}

	/* static */ const double
	Json_Document::get_time_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    unsigned long hrs   = 0;
	    unsigned long mins  = 0;
	    unsigned long secs  = 0;
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		return 0;
	    } else if ( !attr->second.is<std::string>() || ::sscanf( attr->second.get<std::string>().c_str(), "%ld:%ld:%ld", &hrs, &mins, &secs ) != 3 ) {
		argument_error( name, attr->second.to_str().c_str() );
	    }
	    return hrs * 3600 + mins * 60 + secs;
	}
    }
}

namespace LQIO {
    namespace DOM {

	/* static */ std::ostream&
	Json_Document::printIndent( std::ostream& output, int i )
	{
	    if ( i < 0 ) {
		__indent += i;
		if ( __indent < 0 ) {
		    __indent = 0;
		}
	    }
	    output << std::endl << std::setw( __indent * 4 ) << " ";
	    if ( i >= 0 ) {
		__indent += i;
	    }
	    return output;
	}

	/* static */ std::ostream&
	Json_Document::printAttribute( std::ostream& output, const std::string& name, const picojson::value& value )
	{
	    output << indent(0) << "\"" << name << "\": " ;
	    if ( value.is<std::string>() ) {
		output << "\"" << value.get<std::string>() << "\": " ;
	    } else {
		output << value.to_str();
	    }
	    return output;
	}


	/* static */ std::ostream&
	Json_Document::beginAttribute( std::ostream& output, const std::string& attribute, const picojson::value& value )
	{
	    if ( value.is<picojson::object>() ) {
		output << indent( +1 ) << attribute << ": {";
	    } else if ( value.is<picojson::array>() ) {
		output << indent( +1 ) << attribute << ": [";
	    } else {
		output << indent( 0 ) << attribute << ": " << value.to_str();
	    }
	    return output;
	}

	/* static */ std::ostream&
	Json_Document::endAttribute( std::ostream& output, const std::string&, const picojson::value& value )
	{
	    if ( value.is<picojson::object>() ) {
		output << indent( -1 ) << "}";
	    } else if ( value.is<picojson::array>() ) {
		output << indent( -1 ) << "]";
	    }
	    return output;
	}

	/* static */ std::ostream&
	Json_Document::printObjectBegin( std::ostream& output, const std::string& name )
	{
	    output << indent( +1 ) << "\"" << name << "\": {";
	    return output;
	}

	/* static */ std::ostream&
	Json_Document::printObjectEnd( std::ostream& output )
	{
	    output << indent( -1 ) << "}";
	    if ( __indent == 0 ) {
		output << std::endl;
	    }
	    return output;
	}
    }
}

namespace LQIO {
    namespace DOM {

	/*
	 * This method corresponds to the "set" method used in the constructor.
	 */

	bool
	Json_Document::ImportModel::operator()( const std::string& attribute, Json_Document& document, const picojson::value& value ) const
	{
	    if ( Json_Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    if ( value.is<picojson::object>() || value.is<picojson::array>() ) {
		(document.*getFptr())( value );
	    } else {
		argument_error( attribute, value.to_str() );
	    }

	    if ( Json_Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	    return true;
	}

	bool
	Json_Document::ImportModel::Match::operator()( const std::pair<const char*,ImportModel>& p ) const
	{
	    const ImportModel& obj = p.second;
	    return strncasecmp( p.first, _s.c_str(), _s.size() > obj._min_match ? _s.size() : obj._min_match ) == 0;
	}

    }
}


namespace LQIO {
    namespace DOM {

	void
	Json_Document::init_tables()
	{
	    if ( model_table.size() != 0 ) return;

	    model_table[Xgeneral]		= ImportModel( 1, &Json_Document::handleGeneral );
	    model_table[Xprocessor]		= ImportModel( 1, &Json_Document::handleProcessor );
	    model_table[Xgroup]			= ImportModel( 1, &Json_Document::handleGroup );
	    model_table[Xtask]     		= ImportModel( 1, &Json_Document::handleTask );
	    model_table[Xentry]			= ImportModel( 1, &Json_Document::handleEntry );
	    model_table[Xactivity]		= ImportModel( 1, &Json_Document::handleTaskActivity );
	}


        const char * Json_Document::Xactivity           = "activity";
        const char * Json_Document::Xand_fork           = "and-fork";
        const char * Json_Document::Xand_join           = "and-join";
        const char * Json_Document::Xasynch_call        = "asynch-call";
        const char * Json_Document::Xbegin              = "begin";
        const char * Json_Document::Xcap                = "cap";
        const char * Json_Document::Xcoeff_of_var_sq    = "host-demand-cvsq";
        const char * Json_Document::Xcomment            = "comment";
        const char * Json_Document::Xconf_95            = "conf-95";
        const char * Json_Document::Xconf_99            = "conf-99";
        const char * Json_Document::Xconv_val           = "conv-val";
        const char * Json_Document::Xconv_val_result    = "convergence-value";
        const char * Json_Document::Xcore               = "core";
        const char * Json_Document::Xdeterministic      = "determinstic-calls";
        const char * Json_Document::Xelapsed_time       = "elapsed-time";
        const char * Json_Document::Xend                = "end";
        const char * Json_Document::Xentry              = "entry";
        const char * Json_Document::Xfanin              = "fan-in";
        const char * Json_Document::Xfanout             = "fan-out";
        const char * Json_Document::Xfaults             = "faults";
        const char * Json_Document::Xforwarding         = "forwarding";
        const char * Json_Document::Xgeneral            = "general";
        const char * Json_Document::Xgroup              = "group";
        const char * Json_Document::Xhistogram          = "histogram";
        const char * Json_Document::Xhistogram_bin      = "bin";
        const char * Json_Document::Xinitially          = "initially";
        const char * Json_Document::Xit_limit           = "it-limit";
        const char * Json_Document::Xiterations         = "iterations";
        const char * Json_Document::Xjoin_variance      = "join-variance";
        const char * Json_Document::Xjoin_waiting       = "join-waiting";
        const char * Json_Document::Xloop               = "loop";
        const char * Json_Document::Xlqx                = "lqx";
	const char * Json_Document::Xmean   		= "mean";
	const char * Json_Document::Xmean_calls		= "mean-calls";
        const char * Json_Document::Xmax                = "max";
        const char * Json_Document::Xmax_service_time   = "max-service-time";
        const char * Json_Document::Xmin                = "min";
        const char * Json_Document::Xmultiplicity       = "multiplicity";
        const char * Json_Document::Xmva_info           = "mva-info";
        const char * Json_Document::Xname               = "name";
        const char * Json_Document::Xnumber_bins        = "bins";
        const char * Json_Document::Xopen_arrival_rate  = "open-arrival-rate";
        const char * Json_Document::Xopen_wait_time     = "open-wait-time";
        const char * Json_Document::Xor_fork            = "or-fork";
        const char * Json_Document::Xor_join            = "or-join";
        const char * Json_Document::Xoverflow_bin       = "overflow";
	const char * Json_Document::Xphase		= "phase";
        const char * Json_Document::Xphase_type_flag    = "phase-type";
        const char * Json_Document::Xphase_utilization  = "phase-utilization";
        const char * Json_Document::Xplatform_info      = "platform-info";
        const char * Json_Document::Xpost               = "post";
        const char * Json_Document::Xpragma             = "pragma";
        const char * Json_Document::Xpre                = "pre";
        const char * Json_Document::Xprecedence         = "precedence";
        const char * Json_Document::Xprint_int          = "print-int";
        const char * Json_Document::Xpriority           = "priority";
        const char * Json_Document::Xprob               = "prob";
        const char * Json_Document::Xprob_exceed_max    = "prob_exceed_max_service_time";
        const char * Json_Document::Xproc_utilization   = "proc-utilization";
        const char * Json_Document::Xproc_waiting       = "proc-waiting";
        const char * Json_Document::Xprocessor          = "processor";
        const char * Json_Document::Xquantum            = "quantum";
        const char * Json_Document::Xqueue_length       = "queue-length";
        const char * Json_Document::Xquorum             = "quorum";
        const char * Json_Document::Xr_lock             = "read-lock";
        const char * Json_Document::Xr_unlock           = "read-unlock";
        const char * Json_Document::Xreplication        = "replication";
        const char * Json_Document::Xreply_to           = "reply-to";
        const char * Json_Document::Xresult             = "results";
        const char * Json_Document::Xrwlock             = "rwlock";
        const char * Json_Document::Xrwlock_reader_holding              = "rwlock-reader-holding";
        const char * Json_Document::Xrwlock_reader_holding_variance     = "rwlock-reader-holding-variance";
        const char * Json_Document::Xrwlock_reader_utilization          = "rwlock-reader-utilization";
        const char * Json_Document::Xrwlock_reader_waiting              = "rwlock-reader-waiting";
        const char * Json_Document::Xrwlock_reader_waiting_variance     = "rwlock-reader-waiting-variance";
        const char * Json_Document::Xrwlock_writer_holding              = "rwlock-writer-holding";
        const char * Json_Document::Xrwlock_writer_holding_variance     = "rwlock-writer-holding-variance";
        const char * Json_Document::Xrwlock_writer_utilization          = "rwlock-writer-utilization";
        const char * Json_Document::Xrwlock_writer_waiting              = "rwlock-writer-waiting";
        const char * Json_Document::Xrwlock_writer_waiting_variance     = "rwlock-writer-waiting-variance";
        const char * Json_Document::Xscheduling         = "scheduling";
        const char * Json_Document::Xsemaphore          = "semaphore";
        const char * Json_Document::Xsemaphore_utilization              = "semaphore-waiting";
        const char * Json_Document::Xsemaphore_waiting                  = "semaphore-waiting-variance";
        const char * Json_Document::Xsemaphore_waiting_variance         = "semaphore-utilization";
        const char * Json_Document::Xservice_time       = "service-time";
        const char * Json_Document::Xservice_time_variance   		= "service-time-variance";
        const char * Json_Document::Xservice_type       = "service-time";
        const char * Json_Document::Xshare              = "share";
        const char * Json_Document::Xsignal             = "signal";
	const char * Json_Document::Xsolver_info	= "solver-info";
        const char * Json_Document::Xspeed_factor       = "rate";
        const char * Json_Document::Xsquared_coeff_variation = 		"squared-coeff-variation";
        const char * Json_Document::Xstart_activity     = "start-activity";
        const char * Json_Document::Xstep               = "step";
        const char * Json_Document::Xstep_squared       = "step-squared";
        const char * Json_Document::Xsubmodels          = "submodels";
        const char * Json_Document::Xsynch_call         = "synch-call";
        const char * Json_Document::Xsystem_cpu_time    = "system-cpu-time";
        const char * Json_Document::Xtask               = "task";
        const char * Json_Document::Xthink_time         = "think-time";
        const char * Json_Document::Xthroughput         = "throughput";
        const char * Json_Document::Xthroughput_bound   = "throughput-bound";
        const char * Json_Document::Xunderflow_bin      = "underflow";
        const char * Json_Document::Xunderrelax_coeff   = "underrelax-coeff";
        const char * Json_Document::Xuser_cpu_time      = "user-cpu-time";
        const char * Json_Document::Xutilization        = "utilization";
        const char * Json_Document::Xvalid              = "valid";
        const char * Json_Document::Xw_lock             = "write-lock";
        const char * Json_Document::Xw_unlock           = "write-unlock";
        const char * Json_Document::Xwait               = "wait";
        const char * Json_Document::Xwait_squared       = "wait-squared";
        const char * Json_Document::Xwaiting            = "waiting";
        const char * Json_Document::Xwaiting_variance   = "waiting-variance";
    }
}
