/* -*- c++ -*-
 * $Id: qnap2_document.cpp 14330 2021-01-04 11:51:36Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * December 2020.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <numeric>
#include "qnap2_document.h"
#include "error.h"
#include "glblerr.h"
#include "input.h"
#include "common_io.h"

namespace BCMP {

    std::map<scheduling_type,std::string> QNAP2_Document::__scheduling_str;
    
    
    QNAP2_Document::QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model ) :
	_input_file_name(input_file_name), _model(model)
    {
	if ( __scheduling_str.empty() ) {
	    __scheduling_str[SCHEDULE_CUSTOMER] = "infinite";
	    __scheduling_str[SCHEDULE_DELAY]    = "infinite";
	    __scheduling_str[SCHEDULE_FIFO]     = "fifo";
	    __scheduling_str[SCHEDULE_HOL]      = "";
	    __scheduling_str[SCHEDULE_PPR]      = "prior";
	    __scheduling_str[SCHEDULE_RAND]     = "";
	    __scheduling_str[SCHEDULE_PS]       = "ps";
	    __scheduling_str[SCHEDULE_PS_HOL]   = "";
	    __scheduling_str[SCHEDULE_PS_PPR]   = "";
	    __scheduling_str[SCHEDULE_POLL]     = "";
	    __scheduling_str[SCHEDULE_BURST]    = "";
	    __scheduling_str[SCHEDULE_UNIFORM]  = "";
	    __scheduling_str[SCHEDULE_SEMAPHORE]= "";
	    __scheduling_str[SCHEDULE_CFS]      = "";
	    __scheduling_str[SCHEDULE_RWLOCK]   = "";
	}
    }

    
    /*
     * "&" is a comment.
     * identifiers are limited to the first 8 characters.
     * input is limited to 80 characters.
     * statements are terminated with ";"
     * If there are SPEX variables, then use those.
     */
    
    std::ostream&
    QNAP2_Document::print( std::ostream& output ) const
    {
	std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );
	output << "& " << LQIO::DOM::Common_IO::svn_id() << std::endl;
	if ( LQIO::io_vars.lq_command_line.size() > 0 ) {
	    output << "& " << LQIO::io_vars.lq_command_line << std::endl;
	}
	/* compute terminal service times based on visits */
	const std::string name = std::find_if( stations().begin(), stations().end(), &Model::Station::isCustomer )->first;
	const std::pair<const std::string,const Model::Station::Demand::map_t> terminal( name, _model.computeCustomerDemand( name ) );

	/* 1) Declare all SPEX variables */
	output << qnap2_keyword( "declare" ) << std::endl;
	const std::string integer_vars = std::accumulate( classes().begin(), classes().end(), std::string(""), getVariables( model() ) );
	if ( !integer_vars.empty() )     output << qnap2_statement( "integer " + integer_vars, "SPEX customers vars." ) << std::endl;
	const std::string real_vars    = std::accumulate( stations().begin(), stations().end(), std::string(""), getVariables( model() ) );
	if ( !real_vars.empty() )        output << qnap2_statement( "real " + real_vars,       "SPEX service time vars." ) << std::endl;

	/* 2) Declare all stations */
	output << qnap2_statement( "queue " + std::accumulate( stations().begin(), stations().end(), terminal.first, Model::Station::fold() ), "Station identifiers" ) << std::endl;

	/* 3) Declare the classes */
	if ( classes().size() ==  1 ) {
	    output << qnap2_statement( "integer n_users", "Population" ) << std::endl;
	} else {
	    /* Variables */
	    output << qnap2_statement( "class string name", "Name (for output)" ) << std::endl		// LQN client.
		   << qnap2_statement( "class real " + std::accumulate( stations().begin(), stations().end(), std::string(""), Model::Station::fold( "_t") ), "Station service time" ) << std::endl
		   << qnap2_statement( "class real think_t", "Think time." ) << std::endl
		   << qnap2_statement( "class integer n_users", "Population." ) << std::endl;
	    /* Classes */
	    output << qnap2_statement( "class " + std::accumulate( classes().begin(), classes().end(), std::string(""), Model::Class::fold() ), "Class names" ) << std::endl;
	}

	/* 4) output the statations */
	std::for_each( stations().begin(), stations().end(), printStation( output, model() ) );	// Stations.

	if ( classes().size() > 1 ) {
	    output << qnap2_keyword( "control", "class=all queue" ) << std::endl;	// print for all classes.
	}

	/* 5) Finally, assign the parameters defined in steps 2 & 3 from the constants and variables in the model */
	output << qnap2_keyword( "exec" ) << std::endl
	       << "   begin" << std::endl;
        printClassVariables( output );
	std::for_each( stations().begin(), stations().end(), printStationVariables( output, model() ) );
	output << qnap2_statement( "solve" ) << std::endl
	       << qnap2_statement( "end" ) << std::endl;
        output.flags(flags);

	output << "& For custom output, one needs: " << std::endl
	       << "&   /control/ option=nresult;" << std::endl
	       << "&   /exec/ exit=begin ... end" << std::endl;

	output << qnap2_keyword( "terminal" ) << std::endl;

	return output;
    }

    /* 
     * Think times. 
     */

    std::string
    QNAP2_Document::getVariables::operator()( const std::string& s1, const Model::Class::pair_t& k ) const
    {
	if ( !Model::Class::has_constant_customers( k ) ) {
	    std::ostringstream ss;
	    ss << s1;
	    if ( !s1.empty() ) ss << ",";
	    ss << *k.second.customers();		/* Will print out name */
	    return ss.str();
	} else {
	    return s1;
	}
    }
    
    std::string
    QNAP2_Document::getVariables::operator()( const std::string& s1, const Model::Station::pair_t& m ) const
    {
	const Model::Station::Demand::map_t& demands = m.second.demands();
	return std::accumulate( demands.begin(), demands.end(), s1, getVariables( model() ) );
    }
    
    /*
     * Collect all variables.
     */
    
    std::string
    QNAP2_Document::getVariables::operator()( const std::string& s1, const Model::Station::Demand::pair_t& demand ) const
    {
	if ( !Model::Station::Demand::has_constant_service_time( demand ) ){
	    std::ostringstream ss;
	    ss << s1;
	    if ( !s1.empty() ) ss << ",";
	    ss << *demand.second.service_time();		/* Will print out name */
	    return ss.str();
	} else {
	    return s1;
	}
    }


    /*
     * Prints stations.
     */
    
    void
    QNAP2_Document::printStation::operator()( const Model::Station::pair_t& m ) const
    {
	const Model::Station& station = m.second;
	_output << "&" << std::endl
		<< "/station/ name=" << m.first << ";" << std::endl;
	switch ( station.type() ) {
	case Model::Station::CUSTOMER:
	    _output << qnap2_statement( "init=n_users", "Population by class" ) << std::endl
		    << qnap2_statement( "type=" + __scheduling_str.at(SCHEDULE_CUSTOMER) ) << std::endl
		    << qnap2_statement( "service=exp(think_t)" ) << std::endl;
	    break;
	case Model::Station::DELAY:
	    _output << qnap2_statement( "type=" + __scheduling_str.at(SCHEDULE_DELAY) ) << std::endl;
	    break;
	case Model::Station::LOAD_INDEPENDENT:
	    _output << qnap2_statement( "sched=" + __scheduling_str.at(station.scheduling()) ) << std::endl;
	    break;
	case Model::Station::MULTISERVER:
	    _output << qnap2_statement( "type=multiple(" + to_unsigned(station.copies()) + ")" ) << std::endl;
	default:
	    abort();
	}
	if ( station.type() != Model::Station::CUSTOMER ) {

	    /* Print out service time variables. */

	    _output << qnap2_statement( "service=exp(" + m.first + "_t)" ) << std::endl;

	    const Model::Station::map_t::const_iterator terminal = std::find_if( stations().begin(), stations().end(), &Model::Station::isCustomer );
	    if ( classes().size() == 1 ) {
		_output << qnap2_statement( "transit=" + terminal->first + ",1" ) << std::endl;
	    } else {
		_output << qnap2_statement( "transit(all class)=" + terminal->first + ",1" ) << std::endl;
	    }
	} else {
	    if ( classes().size() == 1 ) {
		_output << qnap2_statement("transit=" + std::accumulate( stations().begin(), stations().end(), std::string(""), printTransit(classes().begin()->first) ), "visits to servers" ) << std::endl;
	    } else {
		for ( Model::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
		    _output << qnap2_statement("transit(" + k->first + ")=" + std::accumulate( stations().begin(), stations().end(), std::string(""), printTransit(k->first) ), "visits to servers" ) << std::endl;
		}
	    }
	}
    }

    std::string
    QNAP2_Document::printTransit::operator()( const std::string& str, const Model::Station::pair_t& m ) const
    {
	std::ostringstream out;
	const Model::Station& station = m.second;

	out << str;
	if ( station.type() != Model::Station::CUSTOMER && station.hasClass(_name) ) {
	    if ( !str.empty() ) out << ",";
	    out << m.first << "," << *station.demandAt(_name).visits();
	}
	return out.str();
    }

    void
    QNAP2_Document::printClassVariables( std::ostream& output ) const
    {
	/* 
	 * Sum service time over all clients and visits over all
	 * servers.  The demand at the reference station is the demand
	 * (service time) at the reference station but the visits over
	 * all non-customer stations.  
	 */
	
	const Model::Station::Demand::map_t visits = std::accumulate( stations().begin(), stations().end(), Model::Station::Demand::map_t(), Model::Station::select( &Model::Station::isServer ) );
	const Model::Station::Demand::map_t service_times = std::accumulate( stations().begin(), stations().end(), Model::Station::Demand::map_t(), Model::Station::select( &Model::Station::isCustomer ) );
	Model::Station::Demand::map_t demands = std::accumulate( service_times.begin(), service_times.end(), Model::Station::Demand::map_t(), Model::sum_visits(visits) );

	/*
	 * QNAP Does everything by "transit", so the service time at
	 * the terminal station has to be adjusted by the number of
	 * visits to all other stations.  Let QNAP do it as it's
	 * easier to see the values from the origial file. Force
	 * floating point math.
	 */
	
	const bool multiclass = classes().size() > 1;
	for ( Model::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
	    std::ostringstream think_time;
	    std::ostringstream customers;
	    std::string comment;
	    think_time << *demands.at(k->first).service_time();
	    customers  << *k->second.customers();
	    if ( !k->second.customers()->wasSet() ) {
		comment = "SPEX Variable";
	    }
	    std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visit( k->first ) );
	    if ( multiclass ) {
		output << qnap2_statement( k->first + ".name:=\"" + k->first + "\"", "Class (client) name" ) << std::endl;
		output << qnap2_statement( k->first + ".think_t:=(" + think_time.str() + ")/(" + think_visits + ")", "Service time per visit" ) << std::endl;
		output << qnap2_statement( k->first + ".n_users:=" + customers.str(), comment  ) << std::endl;
	    } else {
		output << qnap2_statement( "think_t:=(" + think_time.str() + ")/(" + think_visits + ")", "Service time per visit" ) << std::endl;
		output << qnap2_statement( "n_users:=" + customers.str(), comment  ) << std::endl;
	    }
	}
    }


    /*
     * Print out all services times.  Since they are all external
     * variables *var will print either the value or the name of the
     * variable.
     */

    void
    QNAP2_Document::printStationVariables::operator()( const Model::Station::pair_t& m ) const
    {
	const bool multiclass = classes().size() > 1;
	const Model::Station& station = m.second;
	for ( Model::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
	    if ( station.type() == Model::Station::CUSTOMER ) continue;
	    const LQIO::DOM::ExternalVariable * service_time = station.hasClass(k->first) ? station.demandAt(k->first).service_time() : nullptr;
	    std::ostringstream time;
	    std::string comment;
	    if ( LQIO::DOM::ExternalVariable::isDefault(service_time,0.) ) {
		time << "0.000001";		// Qnap doesn't like zero for service time.
		comment = "QNAP does not like zero (0)";
	    } else {
		time << *service_time;		// Might have to strip off $
		if ( !service_time->wasSet() ) {
		    comment = "SPEX variable";
		}
	    }
	    if ( multiclass ) {
		_output << qnap2_statement( k->first + "." + m.first + "_t" + ":=" + time.str(), comment ) << std::endl;
	    } else {
		_output << qnap2_statement(  m.first + "_t" + ":=" + time.str(), comment ) << std::endl;
	    }
	}
    }

    std::string
    QNAP2_Document::fold_visit::operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const
    {
	const Model::Station& station = m2.second;
	if ( !station.hasClass(_name) || station.type() == Model::Station::CUSTOMER ) return s1;	/* Don't visit self */
	std::string s2 = to_real( station.demandAt(_name).visits() );
	if ( s1.empty() ) {
	    return s2;
	} else {
	    return s1 + "+" + s2;
	}
    }

    std::string
    QNAP2_Document::to_real( const LQIO::DOM::ExternalVariable* v )
    {
	char buf[16];
	snprintf( buf, 16, "%g", to_double(*v) );
	std::string str = buf;
	if ( str.find( '.' ) == std::string::npos ) {
	    str += ".";	/* Force real */
	}
	return str;
    }

    std::string
    QNAP2_Document::to_unsigned( const LQIO::DOM::ExternalVariable* v )
    {
	char buf[16];
	snprintf( buf, 16, "%g", to_double(*v) );
	std::string str = buf;
	if ( str.find( '.' ) != std::string::npos ) {
	    throw std::domain_error( "Invalid integer" );
	}
	return str;
    }

    /* static */ std::ostream&
    QNAP2_Document::printKeyword( std::ostream& output, const std::string& s1, const std::string& s2 )
    {
	output << "/" << s1 << "/";
	if ( !s2.empty() ) {
	    output << " " << s2 << ";";
	}
	return output;
    }
    
    /*
     * Format for QNAP output.  I may have to line wrap as lines are
     * limited to 80 characters (fortran).  If s2 is present, it's a
     * comment.
     */
    
    /* static */ std::ostream&
    QNAP2_Document::printStatement( std::ostream& output, const std::string& s1, const std::string& s2 )
    {
	output << "   ";
	if ( s2.empty() ) {
	    output << s1 << ";";
	} else {
	    output << std::setw(60) << (s1 + ";") << "& " << s2;
	}
	return output;
    }
}
