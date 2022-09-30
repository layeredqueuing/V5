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
 * December 2020
 *
 * $Id: model.h 15921 2022-09-28 20:49:00Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(QNSOLVER_MODEL_H)
#define QNSOLVER_MODEL_H
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <lqio/bcmp_document.h>
#include <mva/vector.h>
#include <mva/pop.h>
#include "runlqx.h"

extern bool print_spex;				/* Print LQX program		*/
extern bool debug_flag;;

namespace QNIO {
    class Document;
}

namespace SolverInterface {
    class Solve;
}

class Server;
class BoundsModel;
class OpenModel;
class ClosedModel;

class Model {
    friend class SolverInterface::Solve;
    friend class OpenModel;
    friend class ClosedModel;
    
public:
    enum class Solver { BOUNDS, OPEN, EXACT_MVA, LINEARIZER, LINEARIZER2, BARD_SCHWEITZER, EXPERIMENTAL };
    enum class Multiserver { DEFAULT, CONWAY, REISER, REISER_PS, ROLIA, ROLIA_PS, BRUELL, SCHMIDT, SURI, ZHOU };
    
public:
    Model( QNIO::Document& input, Model::Solver mva, const std::string& );
    Model( QNIO::Document& input, Model::Solver mva );
    Model();
    virtual ~Model();

    explicit operator bool() const { return _result == true; }
    std::ostream& debug( std::ostream& output ) const;
    bool construct();
    bool instantiate();
    virtual bool solve();
    void bounds();
    Solver solver() const { return _solver; }
    std::ostream& print( std::ostream& output ) const;
    
protected:
    virtual BCMP::Model::Chain::Type type() const { return BCMP::Model::Chain::Type::UNDEFINED; }
    virtual bool isParent() const { return true; }
    
    /* inputs */
    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }
    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }
    const BCMP::Model::Station::Class::map_t& classesAt( const std::string& name ) const { return _model.stationAt(name).classes(); }

private:
    size_t indexAt(BCMP::Model::Chain::Type, const std::string& name) const;
    bool compute();
    void saveResults();

private:
    class CreateChainIndex {
    public:
	CreateChainIndex( Model& model, const BCMP::Model::Chain::Type type ) : _model(model), _type(type) {}
	void operator()( const BCMP::Model::Chain::pair_t& pair );

    private:
	std::map<const std::string,size_t>& index() { return _model._index.k; }
	
    private:
	Model& _model;
	const BCMP::Model::Chain::Type _type;
    };

    class CreateStationIndex {
    public:
	CreateStationIndex( Model& model, BCMP::Model::Chain::Type type ) : _model(model), _type(type) {}
	void operator()( const BCMP::Model::Station::pair_t& pair );

    private:
	std::map<const std::string,size_t>& index() { return _model._index.m; }
	const BCMP::Model::Chain::map_t& chains() { return _model.chains(); }
	
    private:
	Model& _model;
	const BCMP::Model::Chain::Type _type;
    };

    class InstantiateStation {
    private:
	class InstantiateClass {
	public:
	    InstantiateClass( const Model& model, Server& server ) : _model(model), _server(server) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& );

	private:
	    const BCMP::Model::Chain& chainAt( const std::string& name ) const { return _model._model.chainAt(name); }
	    size_t indexAt(BCMP::Model::Chain::Type type, const std::string& name) const { return _model.indexAt( type, name ); }
	    const ClosedModel * closed_model() const { return _model._closed_model; }
	    const OpenModel * open_model() const { return _model._open_model; }
	    
	private:
	    const Model& _model;
	    Server& _server;
	};

    public:
	InstantiateStation( const Model& model );
	void operator()( const BCMP::Model::Station::pair_t& pair );

    private:
	const BCMP::Model& model() const { return _model._model; }
	size_t indexAt(const std::string& name ) const { return _model._index.m.at(name); }
	size_t indexAt(BCMP::Model::Chain::Type type, const std::string& name) const  { return _model.indexAt( type, name ); }
	const BCMP::Model::Chain::map_t& chains() { return _model.chains(); }
	Server*& Q(size_t m) { return _model.Q[m]; }	/* Will choose the subclass version */
	Server * replace_server( const std::string&, Server *, Server * ) const;
	const ClosedModel * closed_model() const { return _model._closed_model; }
	const OpenModel * open_model() const { return _model._open_model; }

    private:
	const Model& _model;
	Population N;
    };

    struct Index {					/* Map string to int for chains/stations */
	std::map<const std::string,size_t> k;
	std::map<const std::string,size_t> m;
    };

protected:
    const BCMP::Model& _model;			/* Input */
    const Model::Solver _solver;
    Index _index;				/* Map name to station/class no. */
    bool _result;

private:
    QNIO::Document& _input;		/* Input */
    const std::string _output_file_name;
    /* mixed model */  /* Might change to a vector */
    ClosedModel * _closed_model;
    OpenModel * _open_model;
    BoundsModel * _bounds_model;
    Vector<Server *> Q;				/* Stations. */
};
#endif
