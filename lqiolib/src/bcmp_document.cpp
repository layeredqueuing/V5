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
    const LQIO::DOM::ConstantExternalVariable _ZERO_(0.);

    bool Model::insertClass( const std::string& name, Class::Type type, const LQIO::DOM::ExternalVariable * customers, const LQIO::DOM::ExternalVariable * think_time )
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
    
    bool
    Model::hasConstantCustomers() const
    {
	return std::all_of( classes().begin(), classes().end(), &Model::Class::has_constant_customers );
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

    /* static */ bool
    Model::Class::has_constant_customers( const Class::pair_t& k )
    {
	return k.second.customers() == nullptr || dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(k.second.customers()) != nullptr;
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

    bool
    Model::Station::hasConstantServiceTime() const
    {
	return std::all_of( demands().begin(), demands().end(), &Demand::has_constant_service_time );
    }

    bool
    Model::Station::hasConstantVisits() const
    {
	return std::all_of( demands().begin(), demands().end(), &Demand::has_constant_visits );
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

    /* 
     * Return true if my demand is constant 
     */

    bool Model::Station::Demand::has_constant_service_time( const Station::Demand::pair_t& demand )
    {
	return demand.second.service_time() == nullptr || dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(demand.second.service_time()) != nullptr;
    }

    bool Model::Station::Demand::has_constant_visits( const Station::Demand::pair_t& demand )
    {
	return demand.second.visits() == nullptr || dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(demand.second.visits()) != nullptr;
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

    /*
     * Add this (augend) to addend and return the sum.  If the addend
     * is null or the default value (0), then copy over the augend.
     * Adding symbol variable to another variable will throw (from
     * to_double).
     */
    
    Model::Station::Demand
    Model::Station::Demand::operator+( const Model::Station::Demand& addend ) const
    {
	const LQIO::DOM::ExternalVariable * visits = this->visits();
	if ( isSet(visits) && isSet(addend.visits()) ) {
	    visits       = new LQIO::DOM::ConstantExternalVariable( to_double(*this->visits()) + to_double(*addend.visits()) );
	} else if (!isSet(this->visits()) ) {
	    visits       = addend.visits();
	} /* No operation */
	const LQIO::DOM::ExternalVariable * service_time = this->service_time();
	if ( isSet(service_time) && isSet(addend.service_time()) ) {
	    service_time = new LQIO::DOM::ConstantExternalVariable( to_double(*this->service_time()) + to_double(*addend.service_time()) );
	} else if (!isSet(this->service_time()) ) {
	    service_time = addend.service_time();
	}
	assert( service_time != nullptr );
	return Demand( visits, service_time );
    }

    Model::Station::Demand&
    Model::Station::Demand::operator+=( const Model::Station::Demand& addend )
    {
	return accumulate( addend );
    }

    Model::Station::Demand&
    Model::Station::Demand::accumulate( double visits, double service_time )
    {
	return accumulate( Demand(new LQIO::DOM::ConstantExternalVariable(visits), new LQIO::DOM::ConstantExternalVariable(service_time) ) );
    }

    /*
     * Accumlate.  Copy the addend (might be a var) if the addend is the default value (0).
     */
    
    Model::Station::Demand&
    Model::Station::Demand::accumulate( const Demand& addend ) 
    {
	if ( isSet(visits()) && isSet(addend.visits()) ) {
	    _visits       = new LQIO::DOM::ConstantExternalVariable( to_double(*visits()) + to_double(*addend.visits()) );
	} else if (!isSet(visits()) ) {
	    _visits       = addend.visits();
	} /* No operation */
	if ( isSet(service_time()) && isSet(addend.service_time()) ) {
	    _service_time = new LQIO::DOM::ConstantExternalVariable( to_double(*service_time()) + to_double(*addend.service_time()) );
	} else if (!isSet(service_time()) ) {
	    _service_time = addend.service_time();
	}
	assert( _service_time != nullptr );
	return *this;
    }


    /* Check for a valid variable (if set) and NOT the default value (0). */

    bool 
    Model::isSet( const LQIO::DOM::ExternalVariable * var, double default_value )
    {
	double value;
	return  var != nullptr && var->wasSet() && var->getValue(value) && value != default_value;
    }
	
}
