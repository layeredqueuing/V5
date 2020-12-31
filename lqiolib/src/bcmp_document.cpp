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
#include "bcmp_to_lqn.h"
#include "input.h"

namespace BCMP {
    bool Model::insertClass( const std::string& name, Class::Type type, unsigned int customers, double think_time )
    {
	std::pair<Class::map_t::iterator,bool> result = _classes.insert( Class::pair_t( name, Class( type, customers, think_time ) ) );
	return result.second;
    }
    
    bool Model::insertStation( const std::string& name, const Station& station )
    {
	std::pair<Station::map_t::iterator,bool> result = _stations.insert( Station::pair_t( name, station ) );
	return result.second;
    }
    
    bool Model::insertDemand( const std::string& station_name, const std::string& class_name, const Station::Demand& demands )
    {
	Station::map_t::iterator station = _stations.find( station_name );
	if ( station == _stations.end() ) return false;
	return station->second.insertDemand( class_name, demands );
    }
    
    /* 
     * Sum service time over all clients and visits over all servers.
     * The demand at the reference station is the demand (service
     * time) at the reference station but the visits over all
     * non-customer stations.
     */
	
    Model::Station::Demand::map_t
    Model::computeCustomerDemand( const std::string& name ) const
    {
	const Station::Demand::map_t visits = std::accumulate( stations().begin(), stations().end(), Station::Demand::map_t(), Station::select( &Station::isServer ) );
	const Station::Demand::map_t service_times = std::accumulate( stations().begin(), stations().end(), Station::Demand::map_t(), Station::select( &Station::isCustomer ) );
	Station::Demand::map_t demands = std::accumulate( service_times.begin(), service_times.end(), Station::Demand::map_t(), sum_visits(visits) );
	return demands;
    }
    
    Model::Station::Demand::map_t
    Model::sum_visits::operator()( const Station::Demand::map_t& input, const Station::Demand::pair_t& demand ) const
    {
	Station::Demand::map_t output = input;
	std::pair<Station::Demand::map_t::iterator,bool>result = output.insert( Station::Demand::pair_t(demand.first, demand.second) );
	result.first->second.setVisits( _visits.at(demand.first).visits() );
	return output;
    }


    bool
    Model::convertToLQN( LQIO::DOM::Document& dom ) const
    {
	return LQIO::DOM::BCMP_to_LQN( *this, dom ).convert();
    }

    
    std::ostream&
    Model::print( std::ostream& output ) const
    {
	return output;
    }
    

    /*
     * JMVA insists that service time/visits exist for --all-- classes for --all--stations
     * so pad the demand_map to make it so.
     */

    void
    Model::pad_demand::operator()( const Station::pair_t& m ) const
    {
	for ( Class::map_t::const_iterator k = _classes.begin(); k != _classes.end(); ++k ) {
	    const std::string& class_name = k->first;
	    Station& station = const_cast<Station&>(m.second);
	    station.insertDemand( class_name, BCMP::Model::Station::Demand() );
	}
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

    bool
    Model::Station::insertDemand( const std::string& class_name, const Demand& demand )
    {
	return _demands.insert( Demand::pair_t( class_name, demand ) ).second;
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
    Model::Station::select::operator()( const Station::Demand::map_t& augend, const Station::pair_t& m ) const
    {
	const Station& station = m.second;
	if ( (*_test)( m ) ) {
	    const Demand::map_t& demands = station.demands();
	    return std::accumulate( demands.begin(), demands.end(), augend, &Station::Demand::collect );
	} else {
	    return augend;
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
