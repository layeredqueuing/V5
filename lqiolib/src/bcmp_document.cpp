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

    /* ---------------------------------------------------------------- */
    /*			           Model				*/
    /* ---------------------------------------------------------------- */

    Model::~Model()
    {
	_chains.clear();
	_stations.clear();
    }

    std::pair<Model::Chain::map_t::iterator,bool>
    Model::insertClosedChain( const std::string& name, const LQIO::DOM::ExternalVariable * customers, const LQIO::DOM::ExternalVariable * think_time )
    {
	return _chains.emplace( name, Chain( Chain::Type::CLOSED, customers, think_time )  );
    }

    std::pair<Model::Chain::map_t::iterator,bool>
    Model::insertOpenChain( const std::string& name, const LQIO::DOM::ExternalVariable * arrival_rate )
    {
	return _chains.emplace( name, Chain( Chain::Type::OPEN, arrival_rate )  );
    }

    std::pair<Model::Station::map_t::iterator,bool>
    Model::insertStation( const std::string& name, const Station& station )
    {
	return _stations.emplace( name, station );
    }

    size_t
    Model::n_chains(Model::Chain::Type type) const
    {
	return std::count_if( chains().begin(), chains().end(), Model::Chain::is_a(type) );
    }

    size_t
    Model::n_stations(Model::Chain::Type type) const
    {
	return std::count_if( stations().begin(), stations().end(), Model::Station::is_a(*this,type) );
    }

    /*
     * Find the station in the map
     */

    Model::Station::map_t::const_iterator
    Model::findStation( const Station* m ) const
    {
	for ( Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	    if ( m == &mi->second ) return mi;
	}
	return stations().end();
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
	const Station::Class::map_t servers = std::accumulate( stations().begin(), stations().end(), Station::Class::map_t(), Station::select( &Station::isServer ) );
	const Station::Class::map_t clients = std::accumulate( stations().begin(), stations().end(), Station::Class::map_t(), Station::select( &Station::isCustomer ) );
	return std::accumulate( clients.begin(), clients.end(), Station::Class::map_t(), sum_visits(servers) );
    }

    /*
     * Compute reponse time for class "name"
     */

    double
    Model::response_time( const std::string& name ) const
    {
	return std::accumulate( stations().begin(), stations().end(), 0.0, sum_residence_time( name ) );
    }

    double
    Model::throughput( const std::string& name ) const
    {
	Station::map_t::const_iterator m = std::find_if( stations().begin(), stations().end(), &Station::isCustomer );
	if ( m == stations().end() ) return 0.0;
	const Model::Station& station = m->second;
	if ( name.empty() ) {
	    return station.throughput();
	} else if ( station.hasClass(name) ) {
	    const Station::Class& clasx = station.classAt(name);
	    return clasx.throughput();
	} else {
	    return 0.0;
	}
    }


    Model::Station::Class::map_t
    Model::sum_visits::operator()( const Station::Class::map_t& input, const Station::Class::pair_t& clasx ) const
    {
	Station::Class::map_t output = input;
	std::pair<Station::Class::map_t::iterator,bool>result = output.emplace( clasx.first, clasx.second );
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

    /* Check for a valid variable (if set) and NOT the default value (0). */

    bool
    Model::isSet( const LQIO::DOM::ExternalVariable * var, double default_value )
    {
	double value;
	return  var != nullptr && var->wasSet() && var->getValue(value) && value != default_value;
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

    double Model::sum_residence_time::operator()( double augend, const Station::pair_t& m ) const
    {
	const Model::Station& station = m.second;

	if ( !Station::isServer( m ) ) {
	    return augend;
	} else if ( _name.empty() ) {
	    return augend + station.residence_time();
	} else if ( station.hasClass( _name ) ) {
	    return augend + station.classAt( _name ).residence_time();
	} else {
	    return augend;
	}
    }

    /* ---------------------------------------------------------------- */
    /*			           Chains				*/
    /* ---------------------------------------------------------------- */

    const char * const Model::Chain::__typeName = "chain";

    std::string
    Model::Chain::fold::operator()( const std::string& s1, const Chain::pair_t& c2 ) const
    {
	if ( !s1.empty() ) {
	    return s1 + "," + c2.first + _suffix;
	} else {
	    return c2.first + _suffix;
	}
    }

    /* ---------------------------------------------------------------- */
    /*			         Station				*/
    /* ---------------------------------------------------------------- */

    const char * const Model::Station::__typeName = "station";

    Model::Station::~Station()
    {
	_classes.clear();
    }


    std::pair<Model::Station::Class::map_t::iterator,bool>
    Model::Station::insertClass( const std::string& class_name, const Class& clasx )
    {
	return _classes.emplace( class_name, clasx );
    }

    std::pair<Model::Station::Class::map_t::iterator,bool>
    Model::Station::insertClass( const std::string& class_name, const DOM::ExternalVariable* visits, const DOM::ExternalVariable* service_time )
    {
	return _classes.emplace( class_name, Class( visits, service_time ) );
    }

    /*
     * Find the station in the map
     */

    Model::Station::Class::map_t::const_iterator
    Model::Station::findClass( const Class* k ) const
    {
	for ( Class::map_t::const_iterator ki = classes().begin(); ki != classes().end(); ++ki ) {
	    if ( k == &ki->second ) return ki;
	}
	return classes().end();
    }

    double Model::Station::throughput() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_throughput );
    }

    double Model::Station::queue_length() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_queue_length );
    }

    double Model::Station::residence_time() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_residence_time );
    }

    double Model::Station::utilization() const
    {
	return std::accumulate( classes().begin(), classes().end(), 0.0, &sum_utilization );
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
     * Sum results except for Residence Time.  Those have to be computed after
     * the sum is found.
     */

    Model::Station::Class
    Model::Station::sumResults( const Model::Station::Class& augend, const Model::Station::Class::pair_t& addend )
    {
	Class sum = augend;
	sum._results[Result::Type::THROUGHPUT]     += addend.second._results.at(Result::Type::THROUGHPUT);
	sum._results[Result::Type::QUEUE_LENGTH]   += addend.second._results.at(Result::Type::QUEUE_LENGTH);
	sum._results[Result::Type::RESIDENCE_TIME]  = 0.0;	/* Need to derive 	*/
	sum._results[Result::Type::UTILIZATION]    += addend.second._results.at(Result::Type::UTILIZATION);
	return sum;
    }


    void
    Model::Station::insertResultVariable( Result::Type type, const std::string& name  )
    {
	if ( type == Result::Type::RESPONSE_TIME ) {
	    throw std::runtime_error( "Invalid Result::Type" );
	} else if ( !_result_vars.emplace(type,name).second ) {
	    throw std::runtime_error( "Duplicate Result Variable" );
	}
    }


    /*
     * Return true if the station is in the open or closed model
     * (type).
     */

    bool
    Model::Station::any_of( const Model::Chain::map_t& chains, Model::Chain::Type type ) const
    {
	if ( type == Chain::Type::UNDEFINED ) return true;
	for ( Station::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
	    const Chain::map_t::const_iterator& chain = chains.find(k->first);
	    if ( chain != chains.end() && chain->second.type() == type ) return true;
	}
	return false;
    }


    size_t
    Model::Station::count_if( const Model::Chain::map_t& chains, Model::Chain::Type type ) const
    {
	size_t count = 0;
	for ( Station::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
	    const Chain::map_t::const_iterator& chain = chains.find(k->first);
	    if ( chain != chains.end() && ( type == Chain::Type::UNDEFINED || chain->second.type() == type ) ) count += 1;
	}
	return count;
    }


    Model::Station::Class::map_t
    Model::Station::select::operator()( const Class::map_t& augend, const Station::pair_t& m ) const
    {
	const Station& station = m.second;
	if ( (*_test)( m ) ) {
	    const Class::map_t& classes = station.classes();
	    return std::accumulate( classes.begin(), classes.end(), augend, &Station::Class::collect );
	} else {
	    return augend;
	}
    }


    /* ---------------------------------------------------------------- */
    /*			           Classes				*/
    /* ---------------------------------------------------------------- */

    const char * const Model::Station::Class::__typeName = "Class";

    Model::Station::Class::Class( const DOM::ExternalVariable* visits, const DOM::ExternalVariable* service_time ) :
	_visits(visits), _service_time(service_time), _results(), _result_vars()
    {
	_results[Result::Type::THROUGHPUT] = 0.;
	_results[Result::Type::QUEUE_LENGTH] = 0.;
	_results[Result::Type::RESIDENCE_TIME] = 0.;
	_results[Result::Type::UTILIZATION] = 0.;
    }

    void
    Model::Station::Class::setResults( double throughput, double queue_length, double residence_time, double utilization )
    {
	_results[Result::Type::THROUGHPUT] = throughput;
	_results[Result::Type::QUEUE_LENGTH] = queue_length;
	_results[Result::Type::RESIDENCE_TIME] = residence_time;
	_results[Result::Type::UTILIZATION] = utilization;
    }

    void
    Model::Station::Class::insertResultVariable( Result::Type type, const std::string& name  )
    {
	if ( type == Result::Type::RESPONSE_TIME ) {
	    throw std::runtime_error( "Invalid Result::Type" );
	} else if ( !_result_vars.emplace(type,name).second ) {
	    throw std::runtime_error( "Duplicate Result Variable" );
	}
    }

    /*
     * Derive waiting time over all chains.  Usually after summing in
     * an external application such as qnap2_output.
     */

    Model::Station::Class&
    Model::Station::Class::deriveResidenceTime()
    {
	if ( _results.at(Result::Type::THROUGHPUT) == 0. ) return *this;
	_results.at(Result::Type::RESIDENCE_TIME) = _results.at(Result::Type::QUEUE_LENGTH) / _results.at(Result::Type::THROUGHPUT);
	return *this;
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

    /* ---------------------------------------------------------------- */
    /*			           Bound				*/
    /* ---------------------------------------------------------------- */

    Model::Bound::Bound( const Model::Chain::pair_t& chain, const Model::Station::map_t& stations )
	: _chain(chain), _stations(stations), _D_max(0.0), _D_sum(0.0), _Z(0.0)
    {
	compute();
    }



    void
    Model::Bound::compute()
    {
	_D_max = std::accumulate( stations().begin(), stations().end(), 0., max_demand( chain() ) );
	_D_sum = std::accumulate( stations().begin(), stations().end(), 0., sum_demand( chain() ) );
	_Z = std::accumulate( stations().begin(), stations().end(), think_time(), sum_think_time( chain() ) );
    }


    double
    Model::Bound::think_time() const
    {
	const Model::Chain& chain = _chain.second;
	if ( chain.isClosed() ) return to_double( *chain.think_time() );
	else return 0.0;
    }

    /*
     * Find the largest demand at a station that forms queues.  Adjust
     * for multiplicity.
     */

    double
    Model::Bound::max_demand::operator()( double a1, const Model::Station::pair_t& m2 )
    {
	const Model::Station& m = m2.second;
	if ( (    m.type() != Model::Station::Type::LOAD_INDEPENDENT
	       && m.type() != Model::Station::Type::MULTISERVER )
	     || !m.hasClass( _class ) ) return a1;
	const Model::Station::Class& k = m.classAt( _class );
	double demand = to_double( *k.visits() ) * to_double( *k.service_time() );
	if ( m.type() == Model::Station::Type::MULTISERVER ) {
	    demand = demand / to_double( *m.copies() );
	}
	return std::max( a1, demand );
    }



    double
    Model::Bound::sum_demand::operator()( double a1, const Model::Station::pair_t& m2 )
    {
	const Model::Station& m = m2.second;
	if ( (    m.type() != Model::Station::Type::DELAY
	       && m.type() != Model::Station::Type::LOAD_INDEPENDENT
	       && m.type() != Model::Station::Type::MULTISERVER )
	     || !m.hasClass( _class ) ) return a1;
	const Model::Station::Class& k = m.classAt( _class );
	return a1 += to_double( *k.visits() ) * to_double( *k.service_time() );
    }



    double
    Model::Bound::sum_think_time::operator()( double a1, const Model::Station::pair_t& m2 )
    {
	const Model::Station& m = m2.second;
	if ( !m.reference() || !m.hasClass( _class ) ) return a1;
	const Model::Station::Class& k = m.classAt( _class );
	return a1 += to_double( *k.visits() ) * to_double( *k.service_time() );
    }

}
