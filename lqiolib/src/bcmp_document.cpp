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

    const char * const Model::Station::__typeName = "station";

    std::pair<Model::Chain::map_t::iterator,bool>
    Model::insertChain( const std::string& name, Chain::Type type, const LQIO::DOM::ExternalVariable * customers, const LQIO::DOM::ExternalVariable * think_time )
    {
	return _chains.emplace( name, Chain( type, customers, think_time )  );
    }

    std::pair<Model::Station::map_t::iterator,bool>
    Model::insertStation( const std::string& name, const Station& station )
    {
	return _stations.insert( Station::pair_t( name, station ) );
    }

    bool Model::insertClass( const std::string& station_name, const std::string& class_name, const Station::Class& classes )
    {
	Station::map_t::iterator station = _stations.find( station_name );
	if ( station == _stations.end() ) return false;
	return station->second.insertClass( class_name, classes );
    }

    bool
    Model::hasConstantCustomers() const
    {
	return std::all_of( chains().begin(), chains().end(), &Model::Chain::has_constant_customers );
    }

    /*
     * Sum service time over all clients and visits over all servers.
     * The clasx at the reference station is the clasx (service
     * time) at the reference station but the visits over all
     * non-customer stations.
     */
	
    Model::Station::Class::map_t
    Model::computeCustomerDemand( const std::string& name ) const
    {
	const Station::Class::map_t visits = std::accumulate( stations().begin(), stations().end(), Station::Class::map_t(), Station::select( &Station::isServer ) );
	const Station::Class::map_t service_times = std::accumulate( stations().begin(), stations().end(), Station::Class::map_t(), Station::select( &Station::isCustomer ) );
	Station::Class::map_t classes = std::accumulate( service_times.begin(), service_times.end(), Station::Class::map_t(), sum_visits(visits) );
	return classes;
    }

    Model::Station::Class::map_t
    Model::sum_visits::operator()( const Station::Class::map_t& input, const Station::Class::pair_t& clasx ) const
    {
	Station::Class::map_t output = input;
	std::pair<Station::Class::map_t::iterator,bool>result = output.insert( Station::Class::pair_t(clasx.first, clasx.second) );
	result.first->second.setVisits( _visits.at(clasx.first).visits() );
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
     * JMVA insists that service time/visits exist for --all-- chains for --all--stations
     * so pad the class_map to make it so.
     */

    void
    Model::pad_demand::operator()( const Station::pair_t& m ) const
    {
	for ( Chain::map_t::const_iterator k = _chains.begin(); k != _chains.end(); ++k ) {
	    const std::string& class_name = k->first;
	    Station& station = const_cast<Station&>(m.second);
	    station.insertClass( class_name, BCMP::Model::Station::Class() );
	}
    }

    const char * const Model::Chain::__typeName = "class";

    /* static */ bool
    Model::Chain::has_constant_customers( const Chain::pair_t& k )
    {
	return k.second.customers() == nullptr || dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(k.second.customers()) != nullptr;
    }

    std::string
    Model::Chain::fold::operator()( const std::string& s1, const Chain::pair_t& c2 ) const
    {
	if ( !s1.empty() ) {
	    return s1 + "," + c2.first + _suffix;
	} else {
	    return c2.first + _suffix;
	}
    }

    Model::~Model()
    {
	_chains.clear();
	_stations.clear();
    }

    Model::Station::~Station()
    {
	_classes.clear();
    }

    bool
    Model::Station::insertClass( const std::string& class_name, const Class& clasx )
    {
	return _classes.insert( Class::pair_t( class_name, clasx ) ).second;
    }

    bool
    Model::Station::hasConstantServiceTime() const
    {
	return std::all_of( classes().begin(), classes().end(), &Class::has_constant_service_time );
    }

    bool
    Model::Station::hasConstantVisits() const
    {
	return std::all_of( classes().begin(), classes().end(), &Class::has_constant_visits );
    }

    double Model::Station::throughput() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_throughput );
    }

    double Model::Station::queue_length() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_utilization );
    }

    double Model::Station::residence_time() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_residence_time );
    }

    double Model::Station::utilization() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_queue_length );
    }

    double Model::Station::sum_throughput( double augend, const BCMP::Model::Station::Class::pair_t& addend )
    {
	return augend + addend.second.throughput();
    }

    double Model::Station::sum_utilization( double augend, const BCMP::Model::Station::Class::pair_t& addend )
    {
	return augend + addend.second.utilization();
    }

    double Model::Station::sum_residence_time( double augend, const BCMP::Model::Station::Class::pair_t& addend )
    {
	return augend + addend.second.residence_time();
    }

    double Model::Station::sum_queue_length( double augend, const BCMP::Model::Station::Class::pair_t& addend )
    {
	return augend + addend.second.queue_length();
    }

    /*
     * Sum results except for S and R.  Those have to be computed after
     * the sum is found.
     */

    Model::Station::Class
    Model::Station::sumResults( const Model::Station::Class& augend, const Model::Station::Class::pair_t& addend )
    {
	Class sum = augend;
	sum._throughput     += addend.second._throughput;	/* Throughput		*/
	sum._queue_length   += addend.second._queue_length;	/* Length (n cust)	*/
	sum._residence_time = 0.0;				/* Need to derive 	*/
	sum._utilization    += addend.second._utilization;	/* Utilization		*/
	return sum;
    }


    std::string
    Model::Station::fold::operator()( const std::string& s1, const Station::pair_t& s2 ) const
    {
	if ( s2.second.type() == Type::CUSTOMER ) return s1;
	else if ( s1.empty() ) {
	    return s2.first + _suffix;
	} else {
	    return s1 + "," + s2.first + _suffix;
	}
    }

    Model::Station::Class::map_t
    Model::Station::select::operator()( const Station::Class::map_t& augend, const Station::pair_t& m ) const
    {
	const Station& station = m.second;
	if ( (*_test)( m ) ) {
	    const Class::map_t& classes = station.classes();
	    return std::accumulate( classes.begin(), classes.end(), augend, &Station::Class::collect );
	} else {
	    return augend;
	}
    }

    const char * const Model::Station::Class::__typeName = "Class";

    /*
     * Return true if my class is constant
     */

    bool Model::Station::Class::has_constant_service_time( const Station::Class::pair_t& clasx )
    {
	return clasx.second.service_time() == nullptr || dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(clasx.second.service_time()) != nullptr;
    }

    bool Model::Station::Class::has_constant_visits( const Station::Class::pair_t& clasx )
    {
	return clasx.second.visits() == nullptr || dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(clasx.second.visits()) != nullptr;
    }

    Model::Station::Class::map_t
    Model::Station::Class::collect( const Class::map_t& augend, const Class::pair_t& addend )
    {
	Class::map_t sum = augend;
	std::pair<Class::map_t::iterator,bool> result = sum.insert( addend );
	if ( result.second == false ) {
	    Class& sum_ref = result.first->second;
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

    Model::Station::Class
    Model::Station::Class::operator+( const Model::Station::Class& addend ) const
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
	return Class( visits, service_time );
    }

    Model::Station::Class&
    Model::Station::Class::operator+=( const Model::Station::Class& addend )
    {
	return accumulate( addend );
    }

    Model::Station::Class&
    Model::Station::Class::accumulate( double visits, double service_time )
    {
	return accumulate( Class(new LQIO::DOM::ConstantExternalVariable(visits), new LQIO::DOM::ConstantExternalVariable(service_time) ) );
    }

    /*
     * Accumlate.  Copy the addend (might be a var) if the addend is the default value (0).
     */

    Model::Station::Class&
    Model::Station::Class::accumulate( const Class& addend )
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

    /*
     * Derive waiting time over all chains.
     */

    Model::Station::Class&
    Model::Station::Class::deriveStationAverage()
    {
	if ( _throughput == 0 ) return *this;
	_residence_time = _queue_length / _throughput;
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
