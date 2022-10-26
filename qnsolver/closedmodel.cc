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
 * $Id: closedmodel.cc 16034 2022-10-25 23:20:32Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include <algorithm>
#include <numeric>
#include <lqio/jmva_document.h>
#include <lqio/dom_extvar.h>
#include <lqx/SyntaxTree.h>
#include <mva/fpgoop.h>
#include <mva/prob.h>
#include <mva/multserv.h>
#include "closedmodel.h"

ClosedModel::ClosedModel( Model& parent, QNIO::Document& input, Model::Solver mva )
    : Model(input,mva), _parent(parent), _solver(nullptr), _mva(mva), N(), Z(), priority()
{
    const size_t K = _model.n_chains(BCMP::Model::Chain::Type::CLOSED);
    const size_t M = _model.n_stations(BCMP::Model::Chain::Type::CLOSED);
    _result = K > 0 && M > 0;
    if ( !_result ) return;

    /* Dimension the parameters */
    
    N.resize(K);
    Z.resize(K);
    priority.resize(K);
}


ClosedModel::~ClosedModel()
{
    delete _solver;
}


bool
ClosedModel::construct()
{
    static const std::map<const Model::Solver, MVA::new_solver> solvers = {
	{ Model::Solver::EXACT_MVA,	    ExactMVA::create },
	{ Model::Solver::BARD_SCHWEITZER,   Schweitzer::create },
	{ Model::Solver::LINEARIZER,	    Linearizer::create },
	{ Model::Solver::LINEARIZER2,	    Linearizer2::create }
    };

    assert( !isParent() );

    Model::construct();
    
    /* Copy over only the stations we care about */

    for ( BCMP::Model::Station::map_t::const_iterator m = _parent.stations().begin(); m != _parent.stations().end(); ++m ) {
	if ( m->second.any_of( chains(), type() ) ) {
	    const size_t src = _parent._index.m.at(m->first);
	    const size_t dst = _index.m.at(m->first);
	    Q[dst] = _parent.Q[src];
	}
    }

    /* Create the solver */

    const MVA::new_solver solver = solvers.at(_mva);
    _solver = (*solver)( Q, N, Z, priority, nullptr );
    return _result;
}

/* 
 * Populate the population and thing times prior to solution.
 * Service times and visits are done by model (the superclass).
 */

bool
ClosedModel::instantiate()
{
    assert( !isParent() );
    if ( !_result ) return _result;		/* Nothing to do. */

    try {
	std::for_each( chains().begin(), chains().end(), InstantiateChain( *this ) );
	_result = true;
    }
    catch ( const std::domain_error& e ) {
	_result = false;
    }
    return _result;
}


bool
ClosedModel::solve()
{
    if ( _solver->solve() ) {
	return true;
    } else {
	return false;
    }
}


void
ClosedModel::saveResults()
{
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const size_t m = _index.m.at(mi->first);
	for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	    if ( !ki->second.isClosed() ) continue;
	    const size_t k = _index.k.at(ki->first);
	    const_cast<BCMP::Model::Station&>(mi->second).classes()[ki->first].setResults( _solver->throughput( m, k, N ),
											   _solver->queueLength( m, k, N ),
											   Q[m]->R(k),
											   _solver->utilization( m, k, N ) );
	}
    }
}

void
ClosedModel::InstantiateChain::operator()( const BCMP::Model::Chain::pair_t& input ) 
{
    if ( !input.second.isClosed() ) return;
    const size_t k = indexAt(input.first);
    N(k) = getUnsignedValue( input.second.customers() );
    Z(k) = getDoubleValue( input.second.think_time() );
    priority(k) = 0;
}

std::ostream&
ClosedModel::debug( std::ostream& output ) const
{
    for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	if ( !ki->second.isClosed() ) continue;
	const unsigned int customers = getUnsignedValue( ki->second.customers(), 0 );
	output << "Class " << ki->first << ": customers=" << customers << std::endl;
    }
    return Model::debug( output );
}

std::ostream&
ClosedModel::print( std::ostream& output ) const
{
    output << *_solver << std::endl;
    return output;
}
