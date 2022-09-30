/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/openmodel.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: openmodel.cc 15918 2022-09-27 17:12:59Z greg $
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
#include "openmodel.h"
#include "closedmodel.h"
#include <mva/fpgoop.h>
#include <mva/mva.h>
#include <mva/prob.h>
#include <mva/multserv.h>
#include <mva/open.h>

OpenModel::OpenModel( Model& parent, QNIO::Document& input ) : Model(input,Model::Solver::OPEN), _parent(parent), _solver(nullptr)
{
    const size_t K = _model.n_chains(type());
    const size_t M = _model.n_stations(type());
    _result = K > 0 && M > 0;
    if ( !_result ) return;
}



OpenModel::~OpenModel()
{
    delete _solver;
}


bool
OpenModel::construct()
{
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
    _solver = new Open( Q );
    return true;
}

/*
 * Nothing to do.  Arrival rates are changed in Model::InstatiateStation.
 */

bool
OpenModel::instantiate()
{
    assert( !isParent() );
    return true;
}


bool
OpenModel::convert( const ClosedModel* closed )
{
    try {
	_solver->convert( closed->customers() );
    }
    catch ( const std::range_error& e ) {
	return false;
    }
    return true;
}


/*
 * Mixed model MVA 
 */

bool
OpenModel::solve( const ClosedModel* closed )
{
    try {
	if ( closed ) {
	    _solver->solve( *closed->solver(), closed->customers() );
	} else {
	    _solver->solve();
	}
    }
    catch ( const std::range_error& e ) {
	return false;
    }
    return true;
}


void
OpenModel::saveResults()
{
    static const size_t k = 0;
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const size_t m = _index.m.at(mi->first);
	for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	    if ( !ki->second.isOpen() ) continue;
	    const size_t e = _index.k.at(ki->first);
	    const double lambda = _solver->entryThroughput( *Q[m], e );
	    const double residence_time = Q[m]->R(e,k);
	    const_cast<BCMP::Model::Station&>(mi->second).classes()[ki->first].setResults( lambda,
											   residence_time * lambda,
											   residence_time,
											   _solver->entryUtilization( *Q[m], e ) );
	}
    }
}

std::ostream&
OpenModel::debug( std::ostream& output ) const
{
    for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	if ( !ki->second.isOpen() ) continue;
	output << ": customers=" << *ki->second.customers() << ": arrival rate=" << *ki->second.arrival_rate() << std::endl;
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
