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
 * $Id: boundsmodel.cc 15421 2022-02-02 01:10:15Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include <string>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <lqio/jmva_document.h>
#include <lqio/dom_extvar.h>
#include "boundsmodel.h"

BoundsModel::BoundsModel( Model& parent, BCMP::JMVA_Document& input ) : Model(input,Model::Solver::BOUNDS), _parent(parent), _bounds()
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
 * Mixed model MVA 
 */

bool
BoundsModel::solve()
{
    return true;
}


void
BoundsModel::saveResults()
{
    static const size_t k = 0;
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const size_t m = _index.m.at(mi->first);
	for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
//	    const_cast<BCMP::Model::Station&>(mi->second).classes()[ki->first].setResults( lambda,
//											   residence_time * lambda,
//											   residence_time,
//											   _solver->entryUtilization( *Q[m], e ) );
	}
    }
}
