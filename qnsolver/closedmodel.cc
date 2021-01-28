/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/closedmodel.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: closedmodel.cc 14427 2021-01-28 23:13:01Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <lqio/jmva_document.h>
#include <lqio/dom_extvar.h>
#include "closedmodel.h"
#include <mva/fpgoop.h>
#include <mva/mva.h>
#include <mva/prob.h>
#include <mva/multserv.h>

ClosedModel::ClosedModel( const BCMP::Model& model ) : _model(model), _index(), N(), Z(), priority(), _solver("")
{
    /* Dimension the parameters */
    
    const size_t n_chains = chains().size();		/* !!!This will have to be changed to only count closed chains !!!*/
    const size_t n_stations = stations().size();
    N.resize(n_chains);
    Z.resize(n_chains);
    priority.resize(n_chains);
    Q.resize(n_stations);

    try {
	std::for_each( chains().begin(), chains().end(), CreateChainIndex( *this ) );
	std::for_each( stations().begin(), stations().end(), CreateStationIndex( *this ) );
	_result = true;
    }
    catch ( const std::domain_error& e ) {
	_result = false;
    }
}


ClosedModel::~ClosedModel()
{
}


bool
ClosedModel::instantiate()
{
    try {
	std::for_each( chains().begin(), chains().end(), InstantiateChain( *this ) );
	std::for_each( stations().begin(), stations().end(), InstantiateStation( *this ) );
	_result = true;
    }
    catch ( const std::domain_error& e ) {
	_result = false;
    }
    return _result;
}


bool
ClosedModel::solve( Using solver )
{
    /* Run the solver and return its success as a boolean value */
    MVA * mva = nullptr;
    switch ( solver ) {
    case Using::EXACT_MVA:	    mva = new ExactMVA(   Q, N, Z, priority ); break;
    case Using::BARD_SCHWEITZER:    mva = new Schweitzer( Q, N, Z, priority ); break;
    case Using::LINEARIZER:	    mva = new Linearizer( Q, N, Z, priority ); break;
    }
    assert( mva != nullptr );
    bool ok = mva->solve();
    if ( ok ) {
	saveResults( *mva );
    }
    delete mva;
    return ok;
}


void
ClosedModel::saveResults( const MVA& solver )
{
    _solver = solver.getTypeName();
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const size_t m = _index.m.at(mi->first);
	for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	    const size_t k = _index.k.at(ki->first);
	    const_cast<BCMP::Model::Station&>(mi->second).classes()[ki->first].setResults( solver.throughput( m, k, N ),
											   solver.queueLength( m, k, N ),
											   Q[m]->R(k),
											   solver.utilization( m, k, N ) );
	}
    }
}

void
ClosedModel::CreateChainIndex::operator()( const BCMP::Model::Chain::pair_t& input ) 
{
    if ( !input.second.isClosed() ) return;
    index().emplace( input.first, index().size() + 1 );
}

void
ClosedModel::CreateStationIndex::operator()( const BCMP::Model::Station::pair_t& input )
{
    const size_t m = index().size() + 1;
    index().emplace( input.first, m );
}

void
ClosedModel::InstantiateChain::operator()( const BCMP::Model::Chain::pair_t& input ) 
{
    if ( !input.second.isClosed() ) return;
    const size_t k = indexAt(input.first);
    N(k) = LQIO::DOM::to_unsigned(*input.second.customers());
    Z(k) = LQIO::DOM::to_double(*input.second.think_time());
    priority(k) = 0;
}

void
ClosedModel::InstantiateStation::InstantiateClass::operator()( const BCMP::Model::Station::Class::pair_t& input )
{
    try {
	const size_t k = indexAt(input.first);
	const BCMP::Model::Station::Class& demand = input.second;	// From BCMP model.
	_server.setService( k, LQIO::DOM::to_double( *demand.service_time()) );
	_server.setVisits( k, LQIO::DOM::to_double( *demand.visits()) );
    }
    catch ( const std::out_of_range& e ) {
	/* Open class, ignore */
    }
}

void
ClosedModel::InstantiateStation::operator()( const BCMP::Model::Station::pair_t& input )
{
    const size_t m = indexAt(input.first);
    if ( Q(m) != nullptr ) delete Q(m);		/* out with the old... */

    const BCMP::Model::Station& station = input.second;
    const size_t K = n_chains();
    Server * server = nullptr;
    const unsigned int copies = LQIO::DOM::to_double( *station.copies() );
    switch ( station.scheduling() ) {
    case SCHEDULE_FIFO:
	if ( station.type() == BCMP::Model::Station::Type::DELAY ) {
	    server = new Infinite_Server(K);
	} else if ( copies == 1 && station.type() == BCMP::Model::Station::Type::LOAD_INDEPENDENT ) {
	    server = new FCFS_Server(copies,K);
	} else {
	    server = new Reiser_Multi_Server(copies,K);
	}
	break;
    case SCHEDULE_PS:
	if ( station.type() == BCMP::Model::Station::Type::DELAY ) {
	    server = new Infinite_Server(K);
	} else if ( copies == 1 && station.type() == BCMP::Model::Station::Type::LOAD_INDEPENDENT ) {
	    server = new PS_Server(K);
	} else {
	    server = new Reiser_PS_Multi_Server(copies,K);
	}
	break;
    case SCHEDULE_DELAY:
	server = new Infinite_Server(K);
	break;
    default:
	abort();
	break;
    }
	
    Q(m) = server;				/* ...and in with the new */

    const BCMP::Model::Station::Class::map_t& classes = station.classes();
    std::for_each ( classes.begin(), classes.end(), InstantiateClass( _model, *server ) );
}

std::ostream&
ClosedModel::debug( std::ostream& output ) const
{
    for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	output << "Class "  << ki->first;
	if ( ki->second.isClosed() ) {
	    output << ": customers=" << *ki->second.customers() << std::endl;
	} else {
	    output << ": arrival rate=" << *ki->second.arrival_rate() << std::endl;
	}
    }
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const BCMP::Model::Station::Class::map_t& classes = mi->second.classes();
	output << "Station " << mi->first << ": servers=" << *mi->second.copies() << std::endl;
	for ( BCMP::Model::Station::Class::map_t::const_iterator ki = classes.begin(); ki != classes.end(); ++ki ) {
	    output << "    Class " << ki->first << ": visits=" << *ki->second.visits() << ", service time=" << *ki->second.service_time() << std::endl;
	}
    }
    return output;
}


std::streamsize ClosedModel::__width = 10;
std::streamsize ClosedModel::__precision = 6;
std::string ClosedModel::__separator = "*";


std::ostream&
ClosedModel::print( std::ostream& output ) const
{
    const std::streamsize old_precision = output.precision(__precision);
    output << " - (" << _solver << ") - " << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.fill(' ');
    output << __separator << std::setw(__width) << "name " << header() << __separator << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const size_t m = _index.m.at(mi->first);

	const BCMP::Model::Station::Class::map_t& results = mi->second.classes();
	const BCMP::Model::Station::Class sum = std::accumulate( std::next(results.begin()), results.end(), results.begin()->second, &BCMP::Model::Station::sumResults ).deriveResidenceTime();
	const double service = sum.throughput() > 0 ? sum.utilization() / sum.throughput() : 0.0;
	
	/* Sum will work for single class too. */
	output.setf(std::ios::left, std::ios::adjustfield);
	output.fill(' ');
	if ( results.size() > 1 ) {
	    output << __separator << std::setw(__width) << " " << ClosedModel::blankline() << __separator << std::endl;
	}
	output << __separator << std::setw(__width) << ( " " + mi->first );
	print(output,service,sum);
	output << __separator << std::endl;
	if ( results.size() > 1 ) {
	    for ( BCMP::Model::Station::Class::map_t::const_iterator result = results.begin(); result != results.end(); ++result ) {
		if (result->second.throughput() == 0 ) continue;
		const size_t k = _index.k.at(result->first);
		output << __separator << std::setw(__width) <<  ( "(" + result->first + ")");
		print(output,Q[m]->S(k),result->second);
		output << __separator << std::endl;
	    }
	}
    }
    output << __separator << std::setw(__width) << " " << ClosedModel::blankline() << __separator << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.precision(old_precision);
    return output;
}

std::ostream&
ClosedModel::print( std::ostream& output, double service_time, const BCMP::Model::Station::Result& item ) const
{
    output.unsetf( std::ios::floatfield );
    output << __separator << std::setw(__width) << service_time
	   << __separator << std::setw(__width) << item.utilization()
	   << __separator << std::setw(__width) << item.queue_length()
	   << __separator << std::setw(__width) << item.residence_time()		// per visit.
	   << __separator << std::setw(__width) << item.throughput();
    return output;
}


std::string
ClosedModel::header()
{
    std::ostringstream output;
    output << __separator << std::setw(__width) << "service "
	   << __separator << std::setw(__width) << "busy pct "
	   << __separator << std::setw(__width) << "cust nb "
	   << __separator << std::setw(__width) << "response "
	   << __separator << std::setw(__width) << "thruput ";
    return output.str();
}

std::string
ClosedModel::blankline()
{
    std::ostringstream output;
    for ( unsigned i = 0; i < 5; ++i ) {
	output << std::setfill(' ');
	output << __separator << std::setw(__width) << " ";
    }
    return output.str();
}

/*
 - mean value analysis ("mva") -
 *******************************************************************
 *  name    *  service * busy pct *  cust nb * response *  thruput *
 *******************************************************************
 *          *          *          *          *          *          *
 * terminal *0.2547    * 0.000    * 1.579    *0.2547    * 6.201    *
 *(c1      )*0.3333    * 0.000    * 1.417    *0.3333    * 4.250    *
 *(c2      )*0.8333E-01* 0.000    *0.1625    *0.8333E-01* 1.951    *
 *          *          *          *          *          *          *
 * p1       *0.4000    *0.8267    * 2.223    * 1.076    * 2.067    *
 *(c1      )*0.4000    *0.5667    * 1.512    * 1.067    * 1.417    *
 *(c2      )*0.4000    *0.2601    *0.7109    * 1.093    *0.6502    *
 *          *          *          *          *          *          *
 * p2       *0.2000    *0.6317    * 1.215    *0.3848    * 3.158    *
 *(c1      )*0.2000    *0.5667    * 1.071    *0.3780    * 2.833    *
 *(c2      )*0.2000    *0.6502E-01*0.1442    *0.4436    *0.3251    *
 *          *          *          *          *          *          *
 * p3       *0.7000    *0.6827    *0.9823    * 1.007    *0.9753    *
 *(c2      )*0.7000    *0.6827    *0.9823    * 1.007    *0.9753    *
 *          *          *          *          *          *          *
 *******************************************************************
              memory used:       4024 words of 4 bytes
               (  1.55  % of total memory)     
*/

