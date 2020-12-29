/* -*- c++ -*-
 * $Id: qnap2_document.cpp 14288 2020-12-29 13:24:52Z greg $
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
	    
	output << qnap2_keyword( "declare" ) << std::endl;
	output << qnap2_statement( "queue " + std::accumulate( stations().begin(), stations().end(), terminal.first, Model::Station::fold() ), "Station identifiers" ) << std::endl;
	
	if ( classes().size() ==  1 ) {
	    output << qnap2_statement( "real " + std::accumulate( stations().begin(), stations().end(), std::string(""), Model::Station::fold( "_t") ), "Station service time" ) << std::endl
		   << qnap2_statement( "integer n_users", "Population" ) << std::endl;
	} else {
	    /* Variables */
	    output << qnap2_statement( "class string name", "Name (for output)" ) << std::endl		// LQN client.
		   << qnap2_statement( "class real " + std::accumulate( stations().begin(), stations().end(), std::string(""), Model::Station::fold( "_t") ), "Station service time" ) << std::endl
		   << qnap2_statement( "class real think_t", "Think time." ) << std::endl
		   << qnap2_statement( "class integer n_users", "Population." ) << std::endl;
	    /* Classes */
	    output << qnap2_statement( "class " + std::accumulate( classes().begin(), classes().end(), std::string(""), Model::Class::fold() ), "Class names" ) << std::endl;
	}

	std::for_each( stations().begin(), stations().end(), printStation( output, classes(), stations() ) );	// Stations.

	if ( classes().size() > 1 ) {
	    output << qnap2_keyword( "control", "class=all queue" ) << std::endl;	// print for all classes.
	}

	output << qnap2_keyword( "exec" ) << std::endl
	       << "   begin" << std::endl;
        printClassVariables( output );
	std::for_each( stations().begin(), stations().end(), printStationVariables( output, classes() ) );
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
	case Model::Station::LOAD_INDEPENDENT:
	    _output << qnap2_statement( "type=" + __scheduling_str.at(station.scheduling()) ) << std::endl
		    << qnap2_statement( "service=exp(" + m.first + "_t)" ) << std::endl;
	    break;
	case Model::Station::MULTISERVER:
	    _output << qnap2_statement( "type=multiple(" + std::to_string(station.copies()) + ")" ) << std::endl
		    << qnap2_statement( "service=exp(" + m.first + "_t)" ) << std::endl;
	default:
	    abort();
	}
	if ( station.type() != Model::Station::CUSTOMER ) {
	    const Model::Station::map_t::const_iterator terminal = std::find_if( _stations.begin(), _stations.end(), &Model::Station::isCustomer );
	    if ( _classes.size() == 1 ) {
		_output << qnap2_statement( "transit=" + terminal->first + ",1" ) << std::endl;
	    } else {
		_output << qnap2_statement( "transit(all class)=" + terminal->first + ",1" ) << std::endl;
	    }
	} else {
	    if ( _classes.size() == 1 ) {
		_output << qnap2_statement("transit=" + std::accumulate( _stations.begin(), _stations.end(), std::string(""), printTransit(_classes.begin()->first) ), "visits to servers" ) << std::endl;
	    } else {
		for ( Model::Class::map_t::const_iterator k = _classes.begin(); k != _classes.end(); ++k ) {
		    _output << qnap2_statement("transit(" + k->first + ")=" + std::accumulate( _stations.begin(), _stations.end(), std::string(""), printTransit(k->first) ), "visits to servers" ) << std::endl;
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
	    out << m.first << "," << station.demandAt(_name).visits();
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
	    std::string think_time = to_real( demands.at(k->first).service_time() );
	    std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visit( k->first ) );
	    std::ostringstream customers;
	    customers  << k->second.customers();
	    if ( multiclass ) {
		output << qnap2_statement( k->first + ".name:=\"" + k->first + "\"", "Class (client) name" ) << std::endl;
		output << qnap2_statement( k->first + ".think_t:=(" + think_time + ")/(" + think_visits + ")", "Service time per visit" ) << std::endl;
		output << qnap2_statement( k->first + ".n_users:=" + customers.str()  ) << std::endl;
	    } else {
		output << qnap2_statement( "think_t:=(" + think_time + ")/(" + think_visits + ")", "Service time per visit" ) << std::endl;
		output << qnap2_statement( "n_users:=" + customers.str()  ) << std::endl;
	    }
	}
    }


    void
    QNAP2_Document::printStationVariables::operator()( const Model::Station::pair_t& m ) const
    {
	const bool multiclass = _classes.size() > 1;
	const Model::Station& station = m.second;
	for ( Model::Class::map_t::const_iterator k = _classes.begin(); k != _classes.end(); ++k ) {
	    if ( station.type() == Model::Station::CUSTOMER ) continue;
	    double temp = station.hasClass(k->first) ? station.demandAt(k->first).service_time() : 0.;
	    std::string comment;
	    if ( temp == 0 ) {
		temp = 0.000001;		// Qnap doesn't like zero for service time.
		comment = "QNAP does not like zero (0)";
	    }
	    std::string time = to_real( temp );
	    if ( multiclass ) {
		_output << qnap2_statement( k->first + "." + m.first + "_t" + ":=" + time, comment ) << std::endl;
	    } else {
		_output << qnap2_statement(  m.first + "_t" + ":=" + time, comment ) << std::endl;
	    }
	}
    }


    std::string
    QNAP2_Document::fold_visit::operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const
    {
	const Model::Station& station = m2.second;
	if ( !station.hasClass(_name) ) return s1;
	std::string s2 = to_real( station.demandAt(_name).visits() );
	if ( s1.empty() ) {
	    return s2;
	} else {
	    return s1 + "+" + s2;
	}
    }

    std::string
    QNAP2_Document::to_real( double v )
    {
	char buf[16];
	snprintf( buf, 16, "%g", v );
	std::string str = buf;
	if ( str.find( '.' ) == std::string::npos ) {
	    str += ".";	/* Force real */
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
