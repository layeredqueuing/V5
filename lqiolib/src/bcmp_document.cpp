/* -*- c++ -*-
 * $Id: bcmp_document.cpp 16331 2023-01-15 22:58:45Z greg $
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
#include <lqx/SyntaxTree.h>
#include <lqx/SymbolTable.h>

namespace BCMP {

    /* ---------------------------------------------------------------- */
    /*			           Model				*/
    /* ---------------------------------------------------------------- */

    Model::~Model()
    {
	_chains.clear();
	_stations.clear();
    }

    std::pair<Model::Chain::map_t::iterator,bool>
    Model::insertClosedChain( const std::string& name, LQX::SyntaxTreeNode * customers, LQX::SyntaxTreeNode * think_time )
    {
	return _chains.emplace( name, Chain( Chain::Type::CLOSED, customers, think_time )  );
    }

    std::pair<Model::Chain::map_t::iterator,bool>
    Model::insertOpenChain( const std::string& name, LQX::SyntaxTreeNode * arrival_rate )
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
	return std::accumulate( stations().begin(), stations().end(), 0.0, sum_response_time( name ) );
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


    /*
     * Plotting will insert new variables, so erase all those currently set.
     */

    void
    Model::clearAllResultVariables()
    {
	std::for_each( stations().begin(), stations().end(), &Station::clear_all_result_variables );
    }

    std::ostream&
    Model::print( std::ostream& output ) const
    {
	static const std::string heading = ", \"utilization\", \"queue length \", \"residence time\", \"throughput\"";
	output << "\"Name\"";
	if ( chains().size() == 1 ) {
	    output << heading;
	} else {
	    for ( Chain::map_t::const_iterator k = _chains.begin(); k != _chains.end(); ++k ) {
		output << ", " << k->first << ", , , ";
	    }
	    output << ", \"Aggregate\", , ," << std::endl;
	    for ( Chain::map_t::const_iterator k = _chains.begin(); k != _chains.end(); ++k ) {
		output << heading;
	    }
	    output << heading;
	}
	output << std::endl;

	std::for_each( stations().begin(), stations().end(), Station::print( output ) );
	return output;
    }

    /*
     * Return true is var evaluates to the default value.
     */
    
    bool
    Model::isDefault( LQX::SyntaxTreeNode * var, double default_value )
    {
	if ( var == nullptr ) return true;
	if ( !dynamic_cast<LQX::ConstantValueExpression *>(var) ) return false;
	return getDoubleValue(var) == default_value;
    }


    double
    Model::getDoubleValue( LQX::SyntaxTreeNode * var )
    {
	if ( var == nullptr || !dynamic_cast<LQX::ConstantValueExpression *>(var) ) return 0.0;
	return var->invoke(nullptr)->getDoubleValue();
    }

    LQX::SyntaxTreeNode *
    Model::add( LQX::SyntaxTreeNode * a1, LQX::SyntaxTreeNode * a2 )
    {
	if ( a1 == nullptr ) return a2;
	else if ( a2 == nullptr ) return a1;
	if ( dynamic_cast<LQX::ConstantValueExpression *>(a1) && dynamic_cast<LQX::ConstantValueExpression *>(a2) ) return new LQX::ConstantValueExpression( Model::getDoubleValue(a1) + Model::getDoubleValue(a2) );
	else return new LQX::MathExpression( LQX::MathExpression::ADD, a1, a2 );
    }

    LQX::SyntaxTreeNode *
    Model::subtract( LQX::SyntaxTreeNode * a1, LQX::SyntaxTreeNode * a2 )
    {
	if ( a2 == nullptr ) return a1;
	else if ( a1 == nullptr ) return subtract( new LQX::ConstantValueExpression( 0. ), a2 );
	else if ( dynamic_cast<LQX::ConstantValueExpression *>(a1) && dynamic_cast<LQX::ConstantValueExpression *>(a2) ) return new LQX::ConstantValueExpression( Model::getDoubleValue(a1) - Model::getDoubleValue(a2) );
	else return new LQX::MathExpression( LQX::MathExpression::SUBTRACT, a1, a2 );
    }

    LQX::SyntaxTreeNode *
    Model::divide( LQX::SyntaxTreeNode * a1, LQX::SyntaxTreeNode * a2 )
    {
	if ( isDefault( a1 ) ) return new LQX::ConstantValueExpression( 0. );
	else if ( isDefault( a2, 1.0 ) ) return a1;
	else if ( dynamic_cast<LQX::ConstantValueExpression *>(a1) && dynamic_cast<LQX::ConstantValueExpression *>(a2) ) return new LQX::ConstantValueExpression( Model::getDoubleValue(a1) / Model::getDoubleValue(a2) );
	else return new LQX::MathExpression( LQX::MathExpression::DIVIDE, a1, a2 );
    }

    LQX::SyntaxTreeNode *
    Model::multiply( LQX::SyntaxTreeNode * a1, LQX::SyntaxTreeNode * a2 )
    {
	if ( isDefault( a1 ) || isDefault( a2 ) ) return new LQX::ConstantValueExpression( 0. );
	else if ( isDefault( a1, 1.0 ) ) return a2;
	else if ( isDefault( a2, 1.0 ) ) return a1;
	else if ( dynamic_cast<LQX::ConstantValueExpression *>(a1) && dynamic_cast<LQX::ConstantValueExpression *>(a2) ) return new LQX::ConstantValueExpression( Model::getDoubleValue(a1) * Model::getDoubleValue(a2) );
	else return new LQX::MathExpression( LQX::MathExpression::MULTIPLY, a1, a2 );
    }

    LQX::SyntaxTreeNode *
    Model::max( LQX::SyntaxTreeNode * a1, LQX::SyntaxTreeNode * a2 )
    {
	if ( a1 == nullptr ) return a2;
	else if ( a2 == nullptr ) return a1;
	else if ( dynamic_cast<LQX::ConstantValueExpression *>(a1) && dynamic_cast<LQX::ConstantValueExpression *>(a2) ) return new LQX::ConstantValueExpression( std::max( Model::getDoubleValue(a1), Model::getDoubleValue(a2) ) );
	else return new LQX::MethodInvocationExpression( "max", a1, a2, nullptr );
    }

    LQX::SyntaxTreeNode *
    Model::reciprocal( LQX::SyntaxTreeNode * divisor )
    {
	if ( divisor == nullptr ) return nullptr;
	else if ( dynamic_cast<LQX::ConstantValueExpression *>(divisor) ) return new LQX::ConstantValueExpression( 1. / Model::getDoubleValue(divisor) );
	return new LQX::MathExpression( LQX::MathExpression::DIVIDE, new LQX::ConstantValueExpression( 1. ), divisor );
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

    double Model::sum_response_time::operator()( double augend, const Station::pair_t& m ) const
    {
	const Model::Station& station = m.second;

	if ( !Station::isServer( m ) ) {
	    return augend;
	} else if ( _name.empty() ) {
	    return augend + station.response_time();
	} else if ( station.hasClass( _name ) ) {
	    return augend + station.classAt( _name ).response_time();
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


    void
    Model::Chain::insertResultVariable( Result::Type type, const std::string& name  )
    {
	if ( type != Result::Type::RESPONSE_TIME && type != Result::Type::THROUGHPUT ) {
	    throw std::runtime_error( std::string("Invalid Result::Type: ") + name );
	} else if ( !_result_vars.emplace(type,name).second ) {
	    throw std::runtime_error( std::string("Duplicate Result Variable: ") + name );
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


    Model::Station&
    Model::Station::operator=( const Station& src )
    {
	_type = src._type;
	_scheduling = src._scheduling;
	_copies = src._copies;
	_reference = src._reference;
	_classes.clear();
	std::copy(src._classes.begin(), src._classes.end(), std::inserter(_classes, _classes.begin()));
	_result_vars = src._result_vars;
	return *this;
    }

    void
    Model::Station::clear()
    {
	_type = Model::Station::Type::NOT_DEFINED;
	_scheduling = SCHEDULE_PS;
	_copies = nullptr;
	_reference = false;
	_classes.clear();
	_result_vars.clear();
    }

    std::pair<Model::Station::Class::map_t::iterator,bool>
    Model::Station::insertClass( const std::string& class_name, const Class& clasx )
    {
	return _classes.emplace( class_name, clasx );
    }

    std::pair<Model::Station::Class::map_t::iterator,bool>
    Model::Station::insertClass( const std::string& class_name, LQX::SyntaxTreeNode * visits, LQX::SyntaxTreeNode * service_time )
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

    double Model::Station::response_time() const
    {
	const double x = throughput();
	if ( x > 0. ) return queue_length() / x;		/* Must be derived. */
	else return 0.0;
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

    double Model::Station::sum_response_time( double augend, const BCMP::Model::Station::Class::pair_t& addend )
    {
	return augend + addend.second.response_time();
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
	sum._results[Result::Type::RESIDENCE_TIME] += addend.second._results.at(Result::Type::RESIDENCE_TIME);
	sum._results[Result::Type::UTILIZATION]    += addend.second._results.at(Result::Type::UTILIZATION);
	return sum;
    }


    void
    Model::Station::insertResultVariable( Result::Type type, const std::string& name  )
    {
	if ( type == Result::Type::RESPONSE_TIME ) {
	    throw std::runtime_error( std::string("Invalid Result::Type: ") + name );
	} else if ( !_result_vars.emplace(type,name).second ) {
	    throw std::runtime_error( std::string("Duplicate Result Variable: ") + name );
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


    void
    Model::Station::clear_all_result_variables( BCMP::Model::Station::pair_t& mi )
    {
	Model::Station& m = mi.second;
	m.resultVariables().clear();
	std::for_each( m.classes().begin(), m.classes().end(), &Model::Station::Class::clear_all_result_variables );
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

    /*
     * Generic result printer
     */
    
    void
    Model::Station::print::operator()( const Model::Station::pair_t& mi ) const
    {
	const BCMP::Model::Station::Class::map_t& classes = mi.second.classes();
	if ( classes.empty() ) return;

	_output << mi.first;		/* Station name */

	std::for_each( classes.begin(), classes.end(), Class::print( _output ) );
	if ( classes.size() > 1 ) {
	    const BCMP::Model::Station::Class sum = std::accumulate( std::next(classes.begin()), classes.end(), classes.begin()->second, &BCMP::Model::Station::sumResults );
	    _output << ", "<< sum.utilization()
		    << ", "<< sum.queue_length()
		    << ", "<< sum.residence_time()		// per visit.
		    << ", "<< sum.throughput();
		
	}
	_output << std::endl;
    }

    /* ---------------------------------------------------------------- */
    /*			           Classes				*/
    /* ---------------------------------------------------------------- */

    const char * const Model::Station::Class::__typeName = "class";

    Model::Station::Class::Class( LQX::SyntaxTreeNode * visits, LQX::SyntaxTreeNode * service_time ) :
	_visits(visits), _service_time(service_time), _results(), _result_vars()
    {
	_results[Result::Type::THROUGHPUT] = 0.;
	_results[Result::Type::QUEUE_LENGTH] = 0.;
	_results[Result::Type::RESIDENCE_TIME] = 0.;
	_results[Result::Type::UTILIZATION] = 0.;
    }

    Model::Station::Class&
    Model::Station::Class::operator=( const Model::Station::Class& src )
    {
	_visits = src._visits;
	_service_time = src._service_time;
	_results = src._results;
	_result_vars = src._result_vars;
	return *this;
    }


    void
    Model::Station::Class::clear()
    {
	_visits = nullptr;
	_service_time = nullptr;
	_results.clear();
	_result_vars.clear();
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
	    throw std::runtime_error( std::string("Invalid Result::Type: ") + name );
	} else if ( !_result_vars.emplace(type,name).second ) {
	    throw std::runtime_error( std::string("Duplicate Result Variable") + name );
	}
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
	LQX::SyntaxTreeNode * visits = this->visits();
	if ( !isDefault(visits) && !isDefault(addend.visits()) ) {
	    visits       = new LQX::MathExpression( LQX::MathExpression::ADD, visits, addend.visits() );
	} else if ( isDefault(visits) ) {
	    visits       = addend.visits();
	} /* No operation */
	LQX::SyntaxTreeNode * service_time = this->service_time();
	if ( !isDefault(service_time) && !isDefault(addend.service_time()) ) {
	    service_time  = new LQX::MathExpression( LQX::MathExpression::ADD, service_time, addend.service_time() );
	} else if ( isDefault(service_time) ) {
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
	return accumulate( Class(new LQX::ConstantValueExpression(visits), new LQX::ConstantValueExpression(service_time) ) );
    }

    /*
     * Accumlate.  Copy the addend (might be a var) if the addend is the default value (0).
     */

    Model::Station::Class&
    Model::Station::Class::accumulate( const Class& addend )
    {
	if ( !isDefault(visits()) && !isDefault(addend.visits()) ) {
	    _visits       = new LQX::MathExpression( LQX::MathExpression::ADD, _visits, addend.visits() );
	} else if ( isDefault(visits()) ) {
	    _visits       = addend.visits();
	} /* No operation */
	if ( !isDefault(service_time()) && !isDefault(addend.service_time()) ) {
	    _service_time  = new LQX::MathExpression( LQX::MathExpression::ADD, _service_time, addend.service_time() );
	} else if ( isDefault(service_time()) ) {
	    _service_time = addend.service_time();
	}
	assert( _service_time != nullptr );
	return *this;
    }



    Model::Station::Class&
    Model::Station::Class::accumulateResults( const Class& addend )
    {
	_results[Result::Type::THROUGHPUT]     += addend._results.at(Result::Type::THROUGHPUT);
	_results[Result::Type::QUEUE_LENGTH]   += addend._results.at(Result::Type::QUEUE_LENGTH);
	_results[Result::Type::RESIDENCE_TIME] += addend._results.at(Result::Type::RESIDENCE_TIME);
	_results[Result::Type::UTILIZATION]    += addend._results.at(Result::Type::UTILIZATION);
	return *this;
    }

    void
    Model::Station::Class::print::operator()( const Model::Station::Class::pair_t& ki ) const
    {
	_output << ", " << ki.second.throughput()
		<< ", " << ki.second.queue_length()
		<< ", " << ki.second.residence_time()
		<< ", " << ki.second.utilization();
    }

    /* ---------------------------------------------------------------- */
    /*			           Bound				*/
    /* ---------------------------------------------------------------- */

    /*
     * Find the demand at a station that forms queues.  Adjust for
     * multiplicity.
     */

    /* static */ LQX::SyntaxTreeNode *
    Model::Bound::D( const Model::Station& m, const Model::Chain::pair_t& chain )
    {
	return demand( m, chain.first );
    }


    Model::Bound::Bound( const Model::Chain::pair_t& chain, const Model::Station::map_t& stations )
	: _chain(chain), _stations(stations), _D_max(nullptr), _D_sum(nullptr), _Z_sum(nullptr)
    {
	compute();
    }



    void
    Model::Bound::compute()
    {
	_D_max = std::accumulate( stations().begin(), stations().end(), static_cast<LQX::SyntaxTreeNode *>(nullptr), max_demand( chain() ) );
	_D_sum = std::accumulate( stations().begin(), stations().end(), static_cast<LQX::SyntaxTreeNode *>(nullptr), sum_demand( chain() ) );
	_Z_sum = std::accumulate( stations().begin(), stations().end(), Z(), sum_think_time( chain() ) );
    }


    LQX::SyntaxTreeNode *
    Model::Bound::Z() const
    {
	const Model::Chain& chain = _chain.second;
	if ( chain.isClosed() ) return chain.think_time();
	else return nullptr;
    }

    
    LQX::SyntaxTreeNode *
    Model::Bound::N() const
    {
	if ( Model::isDefault( D_sum(), 0.0 ) || Model::isDefault( D_max(), 1.0) ) return D_sum();
	return Model::divide( D_sum(), D_max() );
    }

    
    LQX::SyntaxTreeNode *
    Model::Bound::N_star() const
    {
	return divide( Model::add( D_sum(), Z_sum() ), D_max() );
    }
    
    /*
     * Is this station the one with the higest demand?
     */
    
    bool
    Model::Bound::is_D_max( const Model::Station& m ) const
    {
	LQX::SyntaxTreeNode * demand = this->demand( m, chain() );
	if ( dynamic_cast<LQX::ConstantValueExpression *>(demand) && dynamic_cast<LQX::ConstantValueExpression *>(_D_max) ) return Model::getDoubleValue(demand) == Model::getDoubleValue(_D_max);
	return demand == _D_max;
    }


    /*
     * Find the demand at a station that forms queues.  Adjust for
     * multiplicity.
     */

    LQX::SyntaxTreeNode *
    Model::Bound::demand( const Model::Station& m, const std::string& chain )
    {
	if ( (   m.type() != Model::Station::Type::LOAD_INDEPENDENT
	      && m.type() != Model::Station::Type::MULTISERVER)
	     || !m.hasClass( chain ) ) return nullptr;

	const Model::Station::Class& k = m.classAt( chain );
	if ( isDefault( k.visits() ) || isDefault( k.service_time() ) ) return nullptr;

	return divide( Bound::demand( k ), m.copies() );
    }


    LQX::SyntaxTreeNode *
    Model::Bound::demand( const Model::Station::Class& k )
    {
	return Model::multiply( k.visits(), k.service_time() );
    }


    /*
     * Find the largest demand at a station that forms queues.  Adjust
     * for multiplicity.
     */

    LQX::SyntaxTreeNode *
    Model::Bound::max_demand::operator()( LQX::SyntaxTreeNode * a1, const Model::Station::pair_t& m2 )
    {
	return Model::max( a1,  Bound::demand( m2.second, _class ) );
    }



    /*
     * Add up all of the demand for _class.  Do not adjust for multiplicity.
     */

    LQX::SyntaxTreeNode *
    Model::Bound::sum_demand::operator()( LQX::SyntaxTreeNode * a1, const Model::Station::pair_t& m2 )
    {
	const Model::Station& m = m2.second;
	if ( (    m.type() != Model::Station::Type::DELAY
	       && m.type() != Model::Station::Type::LOAD_INDEPENDENT
	       && m.type() != Model::Station::Type::MULTISERVER )
	     || m.reference() 
	     || !m.hasClass( _class ) ) return a1;

	const Model::Station::Class& k = m.classAt( _class );
	if ( isDefault( k.visits() ) || isDefault( k.service_time() ) ) return a1;
	else return Model::add( a1, Bound::demand( k ) );
    }



    LQX::SyntaxTreeNode *
    Model::Bound::sum_think_time::operator()( LQX::SyntaxTreeNode * a1, const Model::Station::pair_t& m2 )
    {
	const Model::Station& m = m2.second;
	if ( !m.reference() || !m.hasClass( _class ) ) return a1;
	const Model::Station::Class& k = m.classAt( _class );
	if ( isDefault( k.visits() ) || isDefault( k.service_time() ) ) return a1;
	else return Model::add( a1, Bound::demand( k ) );
    }

}
