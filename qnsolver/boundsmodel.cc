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
 * $Id: boundsmodel.cc 16204 2022-12-27 15:42:27Z greg $
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
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const BCMP::Model::Station& station = mi->second;
	std::map<const std::string,double> utilization;
	for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	    const std::string& chain = ki->first;
	    const BCMP::Model::Bound& bound = bounds().at(chain);

	    if ( BCMP::Model::Bound::D( station, *ki ) == bound.D_max() ) {
		/* Hard bound */
		_results.emplace(result_pair_t(mi->first,std::pair<const std::string,double>(chain,1.0)));	/* Insert chain for station */

	    } else {
		LQX::MathExpression demand( LQX::MathExpression::MULTIPLY, station.classAt(chain).service_time(), station.classAt(chain).visits() );
		LQX::ConstantValueExpression demand_max( bound.D_max() );
		utilization[chain] = LQX::MathExpression( LQX::MathExpression::DIVIDE, &demand, &demand_max ).invoke(nullptr)->getDoubleValue();
	    }
	}

	const double sum = std::accumulate( utilization.begin(), utilization.end(), 0.0, &BoundsModel::plus );
	if ( sum >= 1.0 ) {
	    for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
		const std::string& chain = ki->first;
		_results.emplace( result_pair_t( mi->first, std::pair<const std::string,double>( chain, utilization.at(chain) / sum ) ) );
	    }
	}
    }
    return true;
}


void
BoundsModel::saveResults()
{
    for ( result_map_t::const_iterator result = _results.begin(); result != _results.end(); ++result ) {
	const std::string& station = result->first;
	const std::string& chain = result->second.first;
	const BCMP::Model::Bound& bound = bounds().at(chain);
	const BCMP::Model::Station::Class& k = stations().at(station).classes().at(chain);

	const double utilization = result->second.second;
	const double throughput = 1.0 / bound.D_max()->invoke(nullptr)->getDoubleValue();
	const double residence_time = utilization / throughput;

	const_cast<BCMP::Model::Station::Class&>(k).setResults( throughput,
								1.0,
								residence_time,
								utilization );
    }
}
