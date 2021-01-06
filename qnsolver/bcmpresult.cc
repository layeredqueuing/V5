/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/bcmpresult.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: bcmpresult.cc 14333 2021-01-04 23:17:05Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include "bcmpresult.h"
#include "bcmpmodel.h"

BCMPResult::BCMPResult( const Population& N, const Vector<Server *>& Q, const MVA& solver) : _results()
{
    for ( Vector<Server *>::const_iterator m = Q.begin(); m != Q.end(); ++m ) {
	const size_t mx = (*m)->closedIndex;
	for ( size_t k = 1; k <= N.size(); ++k ) {
	    Item item( (*m)->S(k),
		       (*m)->V(k),
		       solver.throughput( mx, k, N ),
		       solver.queueLength(mx, k, N ),
		       (*m)->R(k),
		       solver.utilization( mx, k, N ) );
	    _results.insert( result_pair( mx, item_pair(k, item ) ) );
	}
    }
}


BCMPResult::Item
BCMPResult::sum( const size_t m ) const
{
    Item sum;
    const std::pair<result_map::const_iterator,result_map::const_iterator> results = _results.equal_range( m );
    for ( result_map::const_iterator result = results.first; result != results.second; ++result ) {
	sum += result->second.second;
    }
    sum.deriveStationAverage();
    return sum;
}


/*
 * Sum results except for S and R.  Those have to be computed after
 * the sum is found.
 */

BCMPResult::Item& BCMPResult::Item::operator+=( const Item& addend )
{
    _S = 0.0;
    _V = _V + addend._V;		/* Visits		*/
    _X = _X + addend._X;		/* Throughput		*/
    _L = _L + addend._L;		/* Length (n cust)	*/
    _R = 0.0;				/* Need to derive R	*/
    _U = _U + addend._U;		/* Utilization		*/
    return *this;
}


/*
 * Derive waiting time over all classes.
 */

BCMPResult::Item&
BCMPResult::Item::deriveStationAverage()
{
    if ( _X == 0 ) return *this;
    _S = _U / _X;
    _R = _L / _X;
    return *this;
}
