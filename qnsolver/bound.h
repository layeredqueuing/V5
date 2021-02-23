/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/model.h $
 *
 * Composite Class for models (There might be a closed and/or an open model).  Stations are
 * shared between the closed and open models.  Output is common.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * February 2021
 *
 * $Id: model.h 14468 2021-02-09 11:57:04Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(QNSOLVER_BOUND_H)
#define QNSOLVER_BOUND_H

#include <map>
#include <string>
#include <lqio/bcmp_document.h>

class Bound {
public:
    Bound( const BCMP::Model::Chain::pair_t& chain, const BCMP::Model::Station::map_t& stations );
    
    double think_time() const;

    double D_max() const { return _D_max; }
    double D_sum() const { return _D_sum; }
    double Z() const { return _Z; }
	    
private:
    const std::string& chain() const { return _chain.first; }
    const BCMP::Model::Station::map_t& stations() const { return _stations; }
    
    void compute();

    struct max_demand {
    max_demand( const std::string& chain ) : _class(chain) {}
	double operator()( double, const BCMP::Model::Station::pair_t& );
    private:
	const std::string& _class;
    };
	
    struct sum_demand {
    sum_demand( const std::string& chain ) : _class(chain) {}
	double operator()( double, const BCMP::Model::Station::pair_t& );
    private:
	const std::string& _class;
    };
	
    struct sum_think_time {
    sum_think_time( const std::string& chain ) : _class(chain) {}
	double operator()( double, const BCMP::Model::Station::pair_t& );
    private:
	const std::string& _class;
    };

    const BCMP::Model::Chain::pair_t _chain;
    const BCMP::Model::Station::map_t& _stations;
    double _D_max;
    double _D_sum;
    double _Z;
};
#endif
