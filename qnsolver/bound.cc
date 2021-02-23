/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/closedmodel.cc $
 *
 * Compute bounds for model.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: closedmodel.cc 14440 2021-02-02 12:44:31Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include "config.h"
#include <algorithm>
#include <numeric>
#include "bound.h"

Bound::Bound( const BCMP::Model::Chain::pair_t& chain, const BCMP::Model::Station::map_t& stations )
    : _chain(chain), _stations(stations), _D_max(0.0), _D_sum(0.0), _Z(0.0)
{
    compute();
}



void
Bound::compute()
{
    _D_max = std::accumulate( stations().begin(), stations().end(), 0., max_demand( chain() ) );
    _D_sum = std::accumulate( stations().begin(), stations().end(), 0., sum_demand( chain() ) );
   _Z = std::accumulate( stations().begin(), stations().end(), think_time(), sum_think_time( chain() ) );
}


double
Bound::think_time() const
{
    const BCMP::Model::Chain& chain = _chain.second;
    if ( chain.isClosed() ) return to_double( *chain.think_time() );
    else return 0.0;
}

double
Bound::max_demand::operator()( double a1, const BCMP::Model::Station::pair_t& m2 )
{
    const BCMP::Model::Station& m = m2.second;
    if ( (    m.type() != BCMP::Model::Station::Type::DELAY
	   && m.type() != BCMP::Model::Station::Type::LOAD_INDEPENDENT
	   && m.type() != BCMP::Model::Station::Type::MULTISERVER )
	 || !m.hasClass( _class ) ) return a1;
    const BCMP::Model::Station::Class& k = m.classAt( _class );
    return std::max( a1, to_double( *k.visits() ) * to_double( *k.service_time() ) );
}



double
Bound::sum_demand::operator()( double a1, const BCMP::Model::Station::pair_t& m2 )
{
    const BCMP::Model::Station& m = m2.second;
    if ( (    m.type() != BCMP::Model::Station::Type::DELAY
	   && m.type() != BCMP::Model::Station::Type::LOAD_INDEPENDENT
	   && m.type() != BCMP::Model::Station::Type::MULTISERVER )
	 || !m.hasClass( _class ) ) return a1;
    const BCMP::Model::Station::Class& k = m.classAt( _class );
    return a1 += to_double( *k.visits() ) * to_double( *k.service_time() );
}



double
Bound::sum_think_time::operator()( double a1, const BCMP::Model::Station::pair_t& m2 )
{
    const BCMP::Model::Station& m = m2.second;
    if ( m.type() != BCMP::Model::Station::Type::CUSTOMER
	 || !m.hasClass( _class ) ) return a1;
    const BCMP::Model::Station::Class& k = m.classAt( _class );
    return a1 += to_double( *k.visits() ) * to_double( *k.service_time() );
}
