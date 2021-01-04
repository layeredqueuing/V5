/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/bcmpmodel.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: bcmpmodel.cc 14321 2021-01-02 15:35:47Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include <string>
#include <lqio/jmva_document.h>
#include <lqio/dom_extvar.h>
#include "bcmpmodel.h"
#include <mva/fpgoop.h>
#include <mva/mva.h>
#include <mva/vector.h>
#include <mva/server.h>
#include <mva/prob.h>

Model::Model( const BCMP::Model& model ) : _model(model), _index(), _reverse(), _N(), _Z(), _priority()
{
    try {
	_result = construct();
    }
    catch ( const std::domain_error& e ) {
	_result = false;
    }
}



void
Model::CreateClass::operator()( const BCMP::Model::Class::pair_t& input ) 
{
    const size_t k = index().size() + 1;
    index().insert( std::pair<std::string,size_t>( input.first, k ) );
    reverse().insert( std::pair<size_t,std::string>( k, input.first ) );

    N(k) = LQIO::DOM::to_unsigned(*input.second.customers());
    Z(k) = LQIO::DOM::to_double(*input.second.think_time());
    priority(k) = 0;
}

void
Model::CreateStation::CreateDemand::operator()( const BCMP::Model::Station::Demand::pair_t& input )
{
    const size_t k = indexAt(input.first);
    const BCMP::Model::Station::Demand& demand = input.second;	// From BCMP model.
    _server.setService( k, LQIO::DOM::to_double(*demand.service_time()) );
    _server.setVisits( k, LQIO::DOM::to_double(*demand.visits()) );
}

void
Model::CreateStation::operator()( const BCMP::Model::Station::pair_t& input )
{
    const size_t m = index().size() + 1;
    index().insert( std::pair<std::string,size_t>( input.first, m ) );
    reverse().insert( std::pair<size_t,std::string>( m, input.first ) );

    const BCMP::Model::Station& station = input.second;
    const size_t K = n_classes();
    Server * server = nullptr;
    switch ( station.scheduling() ) {
    case SCHEDULE_FIFO:		server = new FCFS_Server(K);	break;
    case SCHEDULE_DELAY:	server = new Infinite_Server(K);break;
    case SCHEDULE_PS:		server = new PS_Server(K);	break;
    default: abort(); break;
    }
    Q(m) = server;

    const BCMP::Model::Station::Demand::map_t& demands = station.demands();
    for_each ( demands.begin(), demands.end(), CreateDemand( _model, *server ) );
}

bool
Model::construct()
{
    /* Dimension the parameters */
    
    const size_t n_classes =  classes().size();
    const size_t n_stations = stations().size();
    _N.resize(n_classes);
    _Z.resize(n_classes);
    _priority.resize(n_classes);
    _Q.resize(n_stations);

    std::for_each( classes().begin(), classes().end(), CreateClass( *this ) );
    std::for_each( stations().begin(), stations().end(), CreateStation( *this ) );
    
    return true;
}
