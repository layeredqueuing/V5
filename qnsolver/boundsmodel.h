/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/boundsmodel.h $
 *
 * Bounds solver.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * February 2022.
 *
 * $Id: boundsmodel.h 15918 2022-09-27 17:12:59Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(BOUNDSMODEL_H)
#define BOUNDSMODEL_H
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <map>
#include <string>
#include <lqio/bcmp_document.h>
#include "model.h"

class BoundsModel : public Model {
    typedef std::pair<const std::string,std::pair<const std::string,double>> result_pair_t;
    typedef std::multimap<const std::string,std::pair<const std::string,double>> result_map_t;

    static double plus( double augend, const std::pair<const std::string,double>& addend ) { return augend + addend.second; }

public:
    friend class Model;
    
    BoundsModel( Model& parent, QNIO::Document& input );
    virtual ~BoundsModel();

    explicit operator bool() const { return _result == true; }
    bool construct();
    bool solve();
    virtual void saveResults();
    const std::map<const std::string,BCMP::Model::Bound>& bounds() const { return _bounds; }	/* Chain, Bounds */
    
private:
    virtual BCMP::Model::Chain::Type type() const { return BCMP::Model::Chain::Type::UNDEFINED; }
    virtual bool isParent() const { return false; }

private:
    Model& _parent;
    std::map<const std::string,BCMP::Model::Bound> _bounds;			/* Chain, Bounds */
    result_map_t _results;	/* Station, classes */
};
#endif
