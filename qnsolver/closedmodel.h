/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/closedmodel.h $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: closedmodel.h 15967 2022-10-13 15:23:20Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(CLOSEDMODEL_H)
#define CLOSEDMODEL_H
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <map>
#include <string>
#include <lqio/bcmp_document.h>
#include <mva/mva.h>
#include <mva/pop.h>
#include <mva/vector.h>
#include "model.h"

class ClosedModel : public Model {
public:
    class InstantiateChain {
    public:
	InstantiateChain( ClosedModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Chain::pair_t& pair );

    private:
	size_t indexAt( const std::string& name ) { return _model._index.k.at(name); }
	unsigned& N(size_t k) { return _model.N[k]; }
	double& Z(size_t k) { return _model.Z[k]; }
	unsigned& priority(size_t k) { return _model.priority[k]; }
	double getDoubleValue( LQX::SyntaxTreeNode * variable ) const { return _model.getDoubleValue( variable ); }
	unsigned getUnsignedValue( LQX::SyntaxTreeNode * variable, unsigned int default_value=0 ) const { return _model.getUnsignedValue( variable, default_value ); }
	
    private:
	ClosedModel& _model;
    };

public:
    ClosedModel( Model& parent, QNIO::Document& input, Model::Solver mva );
    virtual ~ClosedModel();

    bool construct();
    bool instantiate();
    bool solve();
    void saveResults();

    std::ostream& debug( std::ostream& output ) const;
    const Population& customers() const { return N; }
    const MVA* solver() const { return _solver; }
	
private:
    virtual BCMP::Model::Chain::Type type() const { return BCMP::Model::Chain::Type::CLOSED; }
    virtual bool isParent() const { return false; }

private:
    Model& _parent;
    MVA * _solver;
    Model::Solver _mva;
    Population N;				/* Population (by class) 	*/
    VectorMath<double> Z;			/* Think Time */
    VectorMath<unsigned> priority;		/* Priority */
};
#endif
