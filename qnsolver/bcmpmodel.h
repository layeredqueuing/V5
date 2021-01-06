/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/bcmpmodel.h $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: bcmpmodel.h 14333 2021-01-04 23:17:05Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(BCMPSOLVE_H)
#define BCMPSOLVE_H
#include <config.h>
#include <string>
#include <map>
#include <lqio/bcmp_document.h>
#include <mva/pop.h>
#include "bcmpresult.h"

class Server;

class Model {
    
private:
    class CreateClass {
    public:
	CreateClass( Model& model ) : _model(model) {}
	void operator()( const BCMP::Model::Class::pair_t& pair );

    private:
	std::map<std::string,size_t>& index() { return _model._index.k; }
	std::map<size_t,std::string>& reverse() { return _model._reverse.k; }
	unsigned& N(size_t k) { return _model._N[k]; }
	double& Z(size_t k) { return _model._Z[k]; }
	unsigned& priority(size_t k) { return _model._priority[k]; }
	
    private:
	Model& _model;
    };

    class CreateStation {
    private:
	class CreateDemand {
	public:
	    CreateDemand( Model& model, Server& server ) : _model(model), _server(server) {}
	    void operator()( const BCMP::Model::Station::Demand::pair_t& );

	private:
	    const BCMP::Model::Class::map_t& classes() const { return _model.classes(); }
	    size_t indexAt(const std::string& name) const { return _model._index.k.at(name); }
	    
	private:
	    Model& _model;
	    Server& _server;
	};

    public:
	CreateStation( Model& model ) : _model(model) {}
	void operator()( const BCMP::Model::Station::pair_t& pair );
	std::map<std::string,size_t>& index() { return _model._index.m; }
	std::map<size_t,std::string>& reverse() { return _model._reverse.m; }

    private:
	size_t n_classes() const { return _model._index.k.size(); }
	Server*& Q(size_t m) { return _model._Q[m]; }

    private:
	Model& _model;
    };

    struct Index {					/* Map string to int for classes/stations */
	std::map<std::string,size_t> k;
	std::map<std::string,size_t> m;
    };
    struct Reverse {
	std::map<size_t,std::string> k;
	std::map<size_t,std::string> m;
    };

    
public:
    Model( const BCMP::Model& model );

    void insertResult( BCMPResult * result ) { _results = result; }	// Will change to vector.

    bool operator!() const { return _result == false; }
    
    const Population& N() const { return _N; }
    const Vector<Server *>& Q() const { return _Q; }
    const VectorMath<double>& Z() const { return _Z; }
    const VectorMath<unsigned>& priority() const { return _priority; }
    Population& N() { return _N; }
    Vector<Server *>& Q() { return _Q; }
    VectorMath<double>& Z() { return _Z; }
    VectorMath<unsigned>& priority() { return _priority; }

private:
    const Index& index() const { return _index; }
    const Reverse& reverse() const { return _reverse; }

#if 0
    class OutputQNAP {
#endif
	static std::streamsize __width;
	static std::streamsize __precision;
	static std::string __separator;

	static std::string header();
	static std::string blankline();

	std::ostream& print( std::ostream& output, const BCMPResult::Item& item ) const;
    public:
	std::ostream& print( std::ostream& ) const;
    private:
#if 0
    };

    class OutputLQN {
    };
#endif

    bool construct();
    /* inputs */
    const BCMP::Model::Class::map_t& classes() const { return _model.classes(); }
    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }

private:
    const BCMP::Model& _model;				/* Input */
    Index _index;
    Reverse _reverse;
    
    Population _N;					/* Population (by class) */
    Vector<Server *> _Q;				/* Stations. */
    VectorMath<double> _Z;				/* Think Time */
    VectorMath<unsigned> _priority;			/* Priority */
    bool _result;
    BCMPResult * _results;			// will change to vector
};
#endif
