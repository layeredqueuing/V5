/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/openmodel.h $
 *
 * Open Model solver.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: openmodel.h 15918 2022-09-27 17:12:59Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(OPENMODEL_H)
#define OPENMODEL_H
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <map>
#include <string>
#include <lqio/bcmp_document.h>
#include <mva/vector.h>
#include <mva/open.h>
#include "model.h"

class ClosedModel;
class Open;

class OpenModel : public Model {
public:
    friend class Model;
    
    OpenModel( Model& parent, QNIO::Document& input );
    virtual ~OpenModel();

    explicit operator bool() const { return _result == true; }
    bool construct();
    bool instantiate();
    bool convert( const ClosedModel* );
    bool solve( const ClosedModel* );
    virtual void saveResults();
    
    std::ostream& debug( std::ostream& output ) const;

private:
    virtual BCMP::Model::Chain::Type type() const { return BCMP::Model::Chain::Type::OPEN; }
    virtual bool isParent() const { return false; }
    
private:
    Model& _parent;
    Open* _solver;
};
#endif
