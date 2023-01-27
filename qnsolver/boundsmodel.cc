/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/boundsmodel.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: boundsmodel.cc 16369 2023-01-26 19:21:33Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include "config.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <lqio/qnio_document.h>
#include <lqx/SyntaxTree.h>
#include "boundsmodel.h"

BoundsModel::BoundsModel( Model& parent, QNIO::Document& input ) : Model(input,Model::Solver::BOUNDS), _parent(parent), _bounds()
{
    const size_t K = _model.n_chains(type());
    const size_t M = _model.n_stations(type());
    _result = K > 0 && M > 0;
    if ( !_result ) return;
}



BoundsModel::~BoundsModel()
{
}


/*
 * For each chain, find the station with the highest demand, and find
 * the total demand.  Highest demand gives throughput bound, total
 * demand gives response bound.  I suppose I can calculate the number
 * of customers per class too so that I can find the x-range.
 */

bool
BoundsModel::construct()
{
    assert( !isParent() );
    
    for ( BCMP::Model::Chain::map_t::const_iterator chain = chains().begin(); chain != chains().end(); ++chain ) {
	_bounds.emplace( chain->first, BCMP::Model::Bound(*chain,stations()) );
    }
    return true;
}


/*
 * Find bounds.  For all chains, find D_max.  Find N.
 */

bool
BoundsModel::solve()
{
    return true;
}


/*
 * Save throughput/utilization at the bounds for every class
 */

void
BoundsModel::saveResults()
{
    for ( BCMP::Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	const BCMP::Model::Station& station = m->second;
	for ( BCMP::Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
	    const std::string& chain = k->first;
	    const BCMP::Model::Bound& bound = bounds().at( chain );

	    const double Dmax_k = BCMP::Model::getDoubleValue( bound.D_max() );
	    if ( Dmax_k == 0 ) continue;
	    const double D_k = BCMP::Model::getDoubleValue( m->second.demand( station.classAt( chain ) ) );
	    const double X_k = 1.0 / Dmax_k;
	    const double U_k = D_k * X_k;
	    const_cast<BCMP::Model::Station&>(m->second).classes()[chain].setResults( X_k, U_k, D_k, U_k );
	}
    }
}
