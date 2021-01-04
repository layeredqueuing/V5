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
 * $Id: bcmpresult.h 14324 2021-01-03 04:11:49Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(BCMPRESULT_H)
#define BCMPRESULT_H
#include "bcmpmodel.h"
#include <mva/mva.h>
#include <mva/vector.h>

class BCMPResult {

    struct Q_t {						/* Stations	*/
	Q_t( const Model& model ) : _model(model) {}
	const Server& operator[]( size_t m ) const { return *_model.Q()[m]; }
    private:
	const Model& _model;
    };

public:
    class Item {
	friend std::ostream& operator<<( std::ostream&, const BCMPResult::Item& );

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

	std::ostream& print( std::ostream& ) const;

	static std::string header();
	static std::string blankline();

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
    BCMPResult( Model& model ) : _model(model), Q(model) {}
    void get( const MVA& solver );
    std::ostream& print( std::ostream& ) const;

private:
    /* inputs */
    const std::map<std::string,size_t>& classes() const { return _model.index().k; }
    const std::map<std::string,size_t>& stations() const { return _model.index().m; }
    const std::map<size_t,std::string>& class_names() const { return _model.reverse().k; }
    /* Outputs */
    const Population& N() const { return _model.N(); }		/* Populations	*/

private:
    /* input */
    Model& _model;
    /* ouptut */
    result_map _results;
    const Q_t Q;

    static std::streamsize __width;
    static std::streamsize __precision;
    static std::string __separator;	/* Column separator	*/
};

inline std::ostream& operator<<( std::ostream& output, const BCMPResult::Item& item ) { return item.print( output ); }
#endif
