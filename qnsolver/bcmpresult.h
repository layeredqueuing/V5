/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/bcmpresult.h $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: bcmpresult.h 14333 2021-01-04 23:17:05Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(BCMPRESULT_H)
#define BCMPRESULT_H
#include <map>
#include <mva/mva.h>
#include <mva/vector.h>
#include <mva/server.h>

class BCMPResult {

public:
    class Item {

    public:
	Item() : _S(0.), _V(0.), _X(0.), _L(0.), _R(0.), _U(0.) {}
	Item(double S, double V, double X, double L, double R, double U) : _S(S), _V(V), _X(X), _L(L), _R(V>0.?R/V:0), _U(U) {}
	double service_time() const { return _S; }
	double visits() const { return _V; }
	double throughput() const { return _X; }
	double customers() const { return _L; }
	double residence_time() const { return _R; }
	double utilization() const { return _U; }

	Item& operator+=( const Item& );
	Item& deriveStationAverage();
//	Item& adjustTerminalForQNAPOutput();

    private:
	double _S;			/* Service Time (QNAP) 	*/
	double _V;			/* Visits.		*/
	double _X;			/* Throughput		*/
	double _L;			/* Length (n cust)	*/
	double _R;			/* Residence time (W)	*/
	double _U;			/* Utilization		*/
    };

    typedef std::pair<size_t,BCMPResult::Item> item_pair;
    typedef std::multimap<size_t,item_pair> result_map;	/* m,k,Item */
    typedef std::pair<size_t,item_pair> result_pair;	/* m,k,Item */

public:
    BCMPResult( const Population& N, const Vector<Server *>& Q, const MVA& solver );

    const result_map& results() const { return _results; }
    BCMPResult::Item sum( size_t m ) const;	/* Return sum over all stations */

private:
    /* ouptut */
    result_map _results;
};
#endif
