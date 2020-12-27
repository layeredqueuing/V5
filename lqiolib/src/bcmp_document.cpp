/* -*- c++ -*-
 * $Id: expat_document.cpp 13764 2020-08-17 19:50:05Z greg $
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
#include <algorithm>
#include <numeric>
#include "bcmp_document.h"
#include "input.h"
#include "common_io.h"
#include "xml_output.h"

namespace BCMP {

    const std::string JMVA::__client_name = "Reference";
    const std::string QNAP2::__client_name = "terminal";


    bool Model::insertClass( const std::string& name, Class::Type type, unsigned int customers, double think_time )
    {
	std::pair<Class::map_t::iterator,bool> result = _classes.insert( Class::pair_t( name, Class( type, customers, think_time ) ) );
	return result.second;
    }
    
    bool Model::insertStation( const std::string& name, Station::Type type, unsigned int copies)
    {
	std::pair<Station::map_t::iterator,bool> result = _stations.insert( Station::pair_t( name, Station( type, copies ) ) );
	return result.second;
    }
    
    bool Model::insertDemand( const std::string& station_name, const std::string& class_name, const Station::Demand& demands )
    {
	Station::map_t::iterator station = _stations.find( station_name );
	if ( station == _stations.end() ) return false;
	return station->second.insertDemand( class_name, demands );
    }
    
    void
    Model::computeCustomerVisits( const std::string& name )
    {
	const Station::Demand::map_t visits = std::accumulate( stations().begin(), stations().end(), Station::Demand::map_t(), Station::select( &Station::isServer ) );
	Station::Demand& demand = stationAt(name).demandAt(name);
	demand.setVisits( visits.at(name).visits() );
    }
    
    Model::Station::Demand::map_t
    Model::sumVisits::operator()( const Station::Demand::map_t& input, const Station::Demand::pair_t& demand ) const
    {
	Station::Demand::map_t output = input;
	std::pair<Station::Demand::map_t::iterator,bool>result = output.insert( Station::Demand::pair_t(demand.first, demand.second) );
	result.first->second.setVisits( _visits.at(demand.first).visits() );
	return output;
    }

    void JMVA::print( std::ostream& output ) const
    {
	std::for_each( _stations.begin(), _stations.end(), Station::pad_demand( classes() ) );	/* JMVA want's zeros */
	
	XML::set_indent(0);
	output << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl
	       << "<!-- " << LQIO::DOM::Common_IO::svn_id() << " -->" << std::endl;
	if ( LQIO::io_vars.lq_command_line.size() > 0 ) {
	    output << "<!-- " << LQIO::io_vars.lq_command_line << " -->" << std::endl;
	}

	output << XML::start_element( "model" )
	       << XML::attribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" )
	       << XML::attribute( "xsi:noNamespaceSchemaLocation", "JMTmodel.xsd" )
	       << ">" << std::endl;
	      
	output << XML::start_element( "parameters" ) << ">" << std::endl;

	output << XML::start_element( "classes" ) << XML::attribute( "number", nClasses() ) << ">" << std::endl;
	std::for_each( classes().begin(), classes().end(), printClass( output ) );
	output << XML::end_element( "classes" ) << std::endl;

	const unsigned int n_stations = std::count_if( stations().begin(), stations().end(), &Station::isServer ) + 1;
	output << XML::start_element( "stations" ) << XML::attribute( "number", n_stations ) << ">" << std::endl;
	printClientStation( output );
	std::for_each( stations().begin(), stations().end(), printStation( output ) );
	output << XML::end_element( "stations" ) << std::endl;

	output << XML::start_element( "ReferenceStation" ) << XML::attribute( "number", nClasses() ) << ">" << std::endl;
	std::for_each( classes().begin(), classes().end(), printReference( output ) );
	output << XML::end_element( "ReferenceStation" ) << std::endl;

	output << XML::end_element( "parameters" ) << std::endl;
	output << XML::start_element( "algParams" ) << ">" << std::endl
	       << XML::simple_element( "algType" ) << XML::attribute( "maxSamples", "10000" ) << XML::attribute( "name", "MVA" ) << XML::attribute( "tolerance", "1.0E-7" ) << "/>" << std::endl
	       << XML::simple_element( "compareAlgs" ) << XML::attribute( "value", "false" ) << "/>" << std::endl
	       << XML::end_element( "algParams" ) << std::endl;


	output << XML::end_element( "model" ) << std::endl;
    }

    /*
     * Print out the special "reference" station.
     */
    
    void
    JMVA::printClientStation( std::ostream& output ) const
    {
	/* 
	 * Sum service time over all clients and visits over all
	 * servers.  The demand at the reference station is the demand
	 * (service time) at the reference station but the visits over
	 * all non-customer stations.  
	 */
	
	const Station::Demand::map_t visits = std::accumulate( stations().begin(), stations().end(), Station::Demand::map_t(), Station::select( &Station::isServer ) );
	const Station::Demand::map_t service_times = std::accumulate( stations().begin(), stations().end(), Station::Demand::map_t(), Station::select( &Station::isCustomer ) );
	Station::Demand::map_t demands = std::accumulate( service_times.begin(), service_times.end(), Station::Demand::map_t(), sumVisits(visits) );

	const std::string element( "delaystation" );
	output << XML::start_element( element ) << XML::attribute( "name", "Reference" );
	output << ">" << std::endl;
	output << XML::start_element( "servicetimes" ) << ">" << std::endl;
	std::for_each( demands.begin(), demands.end(), printService( output ) );
	output << XML::end_element( "servicetimes" ) << std::endl;
	output << XML::start_element( "visits" ) << ">" << std::endl;
	std::for_each( demands.begin(), demands.end(), printVisits( output ) );
	output << XML::end_element( "visits" ) << std::endl;
	output << XML::end_element( element ) << std::endl;
    }

    void
    JMVA::printStation::operator()( const Station::pair_t& m ) const
    {
	const Station& station = m.second;
	std::string element;
	switch ( station.type() ) {
	case Station::CUSTOMER:	return;		/* Don't do these.  JMVA wants only ONE reference station */
	case Station::DELAY:		element = "delaystation"; break;
	case Station::LOAD_INDEPENDENT:	element = "listation"; break;
	default: abort();
	}
	_output << XML::start_element( element ) << XML::attribute( "name", m.first );
	if ( station.copies() > 1 ) _output << XML::attribute( "servers", station.copies() );
	_output << ">" << std::endl;
	_output << XML::start_element( "servicetimes" ) << ">" << std::endl;
	std::for_each( station.demands().begin(), station.demands().end(), printService( _output ) );
	_output << XML::end_element( "servicetimes" ) << std::endl;
	_output << XML::start_element( "visits" ) << ">" << std::endl;
	std::for_each( station.demands().begin(), station.demands().end(), printVisits( _output ) );
	_output << XML::end_element( "visits" ) << std::endl;
	_output << XML::end_element( element ) << std::endl;
    }

    void
    JMVA::printClass::operator()( const std::pair<const std::string&,Class>& k ) const
    {
	if ( k.second.isInClosedModel() ) {
	    _output << XML::simple_element( "closedclass" )
		    << XML::attribute( "name", k.first )
		    << XML::attribute( "population", k.second.customers() )
		    << "/>" << std::endl;
	}
	if ( k.second.isInOpenModel() ) {
	}
    }
    
    void
    JMVA::printReference::operator()( const std::pair<const std::string&,Class>& k ) const
    {
	if ( k.second.isInClosedModel() ) {
	    _output << XML::simple_element( "Class" )
		    << XML::attribute( "name", k.first )
		    << XML::attribute( "refStation", "Reference" )
		    << "/>" << std::endl;
	}
	if ( k.second.isInOpenModel() ) {
	}
    }


    void
    JMVA::printService::operator()( const std::pair<const std::string, Station::Demand>& d ) const
    {
	_output << XML::inline_element( "servicetime", "customerclass", d.first, d.second.service_time() ) << std::endl;
    }

    void
    JMVA::printVisits::operator()( const std::pair<const std::string, Station::Demand>& d ) const
    {
	_output << XML::inline_element( "visits", "customerclass", d.first, d.second.visits() ) << std::endl;
    }

    void
    QNAP2::print( std::ostream& output ) const
    {
	std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );
	output << "& " << LQIO::DOM::Common_IO::svn_id() << std::endl;
	if ( LQIO::io_vars.lq_command_line.size() > 0 ) {
	    output << "& " << LQIO::io_vars.lq_command_line << std::endl;
	}
	    
	output << "/declare/" << std::endl;
	output << qnap2_output( "queue " + std::accumulate( stations().begin(), stations().end(), __client_name, Station::fold() ), "Station identifiers" ) << std::endl;
	
	if ( classes().size() ==  1 ) {
	    output << qnap2_output( "real " + std::accumulate( stations().begin(), stations().end(), std::string("think_t"), Station::fold( "_t") ), "Station service time" ) << std::endl
		   << qnap2_output( "integer n_users" ) << std::endl;
	} else {
	    /* Variables */
	    output << qnap2_output( "class string name" ) << std::endl
		   << qnap2_output( "class real " + std::accumulate( stations().begin(), stations().end(), std::string(""), Station::fold( "_t") ), "Station service time" ) << std::endl
		   << qnap2_output( "class real think_t" ) << std::endl
		   << qnap2_output( "class integer n_users" ) << std::endl;
	    /* Classes */
	    output << qnap2_output( "class " + std::accumulate( classes().begin(), classes().end(), std::string(""), Class::fold() ), "Class names" ) << std::endl;
	}

	printClientStation( output );	// Customers.
	std::for_each( stations().begin(), stations().end(), printStation( output, classes() )   );	// Stations.

	if ( classes().size() > 1 ) {
	    output << "/control/ class=all queue;" << std::endl;	// print for all classes.
	}

	output << "/exec/" << std::endl
	       << qnap2_output( "begin" ) << std::endl;
        printClassVariables( output );
	std::for_each( stations().begin(), stations().end(), printStationVariables( output, classes() ) );
	output << qnap2_output( "solve" ) << std::endl
	       << qnap2_output( "end" ) << std::endl;
	output << "/terminal/" << std::endl;
        output.flags(flags);

	/* for custom output, one needs /control/ option=nresult; and /exec/ exit=begin ... end */
    }

    /*
     * Prints stations.
     */
    
    void
    QNAP2::printStation::operator()( const Station::pair_t& m ) const
    {
	const Station& station = m.second;
	if ( station.type() == Station::CUSTOMER ) return;
	_output << "&" << std::endl
		<< "/station/ name=" << m.first << ";" << std::endl;
	if ( station.type() == Station::DELAY ) {
	    _output << qnap2_output( "type=infinite" );
	}
	_output << qnap2_output( "service=exp(" + m.first + "_t)" ) << std::endl;
	if ( _classes.size() == 1 ) {
	    _output << qnap2_output( "transit=" + __client_name + ",1" ) << std::endl;
	} else {
	    _output << qnap2_output( "transit(all class)=" + __client_name + ",1" ) << std::endl;
	} 
    }

    void
    QNAP2::printClientStation( std::ostream& output ) const
    {
	output << "&" << std::endl;
	output << "/station/ name=" << std::setw(64-15) << (__client_name + ";") << "& Customer Class" << std::endl
	       << qnap2_output("type=infinite") << std::endl
	       << qnap2_output("init=n_users", "Population by class" ) << std::endl
	       << qnap2_output("service=exp(think_t)", "Think time by class" ) << std::endl;
	if ( classes().size() == 1 ) {
	    output << qnap2_output("transit=" + std::accumulate( stations().begin(), stations().end(), std::string(""), Station::printQNAP2Transit(classes().begin()->first) ), "visits to servers" ) << std::endl;
	} else {
	    for ( Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
		output << qnap2_output("transit(" + k->first + ")=" + std::accumulate( stations().begin(), stations().end(), std::string(""), Station::printQNAP2Transit(k->first) ), "visits to servers" ) << std::endl;
	    }
	}
    }


    void
    QNAP2::printClassVariables( std::ostream& output ) const
    {
	/* 
	 * Sum service time over all clients and visits over all
	 * servers.  The demand at the reference station is the demand
	 * (service time) at the reference station but the visits over
	 * all non-customer stations.  
	 */
	
	const Station::Demand::map_t visits = std::accumulate( stations().begin(), stations().end(), Station::Demand::map_t(), Station::select( &Station::isServer ) );
	const Station::Demand::map_t service_times = std::accumulate( stations().begin(), stations().end(), Station::Demand::map_t(), Station::select( &Station::isCustomer ) );
	Station::Demand::map_t demands = std::accumulate( service_times.begin(), service_times.end(), Station::Demand::map_t(), sumVisits(visits) );

	const bool multiclass = classes().size() > 1;
	for ( Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
	    if ( multiclass ) {
		output << qnap2_output( k->first + ".name:=" + k->first, "Class (client) name" );
		output << qnap2_output( k->first + ".think_t:=" + std::to_string(demands.at(k->first).service_time()) ) << std::endl;
		output << qnap2_output( k->first + ".n_users:=" + std::to_string(k->second.customers()) ) << std::endl;
	    } else {
		output << qnap2_output( "think_t:=" + std::to_string(demands.at(k->first).service_time()) ) << std::endl;
		output << qnap2_output( "n_users:=" + std::to_string(k->second.customers()) ) << std::endl;
	    }
	}
    }


    void
    QNAP2::printStationVariables::operator()( const Station::pair_t& m ) const
    {
	const bool multiclass = _classes.size() > 1;
	const Station& station = m.second;
	for ( Class::map_t::const_iterator k = _classes.begin(); k != _classes.end(); ++k ) {
	    if ( station.type() == Station::CUSTOMER ) continue;
	    double time = 0.0;
	    if ( station.hasClass(k->first) ) {
		time = station.demandAt(k->first).service_time();
	    }
	    if ( time == 0 ) time = 0.000001;		// Qnap doesn't like zero for service time.
	    if ( multiclass ) {
		_output << qnap2_output( k->first + "." + m.first + "_t" + ":=" + std::to_string(time) ) << std::endl;
	    } else {
		_output << qnap2_output(  m.first + "_t" + ":=" + std::to_string(time) ) << std::endl;
	    }
	}
    }


    /*
     * Format for QNAP output.  I may have to line wrap as lines are
     * limited to 80 characters (fortran).  If s2 is present, it's a
     * comment.
     */
    
    /* static */ std::ostream&
    QNAP2::printOutput( std::ostream& output, const std::string& s1, const std::string& s2 )
    {
	output << "   ";
	if ( s2.empty() ) {
	    output << s1 << ";";
	} else {
	    output << std::setw(60) << (s1 + ";") << "& " << s2;
	}
	return output;
    }

    bool
    Model::Station::insertDemand( const std::string& class_name, const Demand& demand )
    {
	return _demands.insert( Demand::pair_t( class_name, demand ) ).second;
    }


    /*
     * JMVA insists that service time/visits exist for --all-- classes for --all--stations
     * so pad the demand_map to make it so.
     */

    void
    Model::Station::pad_demand::operator()( const Station::pair_t& m ) const
    {
	for ( Class::map_t::const_iterator k = _classes.begin(); k != _classes.end(); ++k ) {
	    const std::string& class_name = k->first;
	    Station& station = const_cast<Station&>(m.second);
	    station.insertDemand( class_name, BCMP::Model::Station::Demand() );
	}
    }

    Model::Station::Demand::map_t
    Model::Station::select::operator()( const Model::Station::Demand::map_t& augend, const std::pair<const std::string,Model::Station>& m ) const
    {
	const Station& station = m.second;
	if ( (*_test)( m ) ) {
	    const Demand::map_t& demands = station.demands();
	    return std::accumulate( demands.begin(), demands.end(), augend, &Station::Demand::collect );
	} else {
	    return augend;
	}
    }

    std::string
    Model::Station::printQNAP2Transit::operator()( const std::string& str, const Station::pair_t& m ) const
    {
	std::ostringstream out;
	const Station& station = m.second;

	out << str;
	if ( station.type() != CUSTOMER && station.hasClass(_class_name) ) {
	    if ( !str.empty() ) out << ",";
	    out << m.first << "," << station.demandAt(_class_name).visits();
	}
	return out.str();
    }

    std::string
    Model::Class::fold::operator()( const std::string& s1, const Class::pair_t& c2 ) const
    {
	if ( !s1.empty() ) {
	    return s1 + "," + c2.first + _suffix;
	} else {
	    return c2.first + _suffix;
	}
    }

    std::string 
    Model::Station::fold::operator()( const std::string& s1, const Station::pair_t& s2 ) const
    {
	if ( s2.second.type() == CUSTOMER ) return s1;
	else if ( s1.empty() ) {
	    return s2.first + _suffix;
	} else {
	    return s1 + "," + s2.first + _suffix;
	}
    }
    

    Model::Station::Demand::map_t
    Model::Station::Demand::collect( const Demand::map_t& augend, const Demand::pair_t& addend )
    {
	Demand::map_t sum = augend;
	std::pair<Demand::map_t::iterator,bool> result = sum.insert( addend );
	if ( result.second == false ) {
	    Demand& sum_ref = result.first->second;
	    sum_ref += addend.second;
	}
	return sum;

    }
}
