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
 * $Id: bcmpmodel.h 14407 2021-01-25 13:56:07Z greg $
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

class Server;
class MVA;

class BCMPModel {
public:
    	enum class Using {EXACT_MVA, LINEARIZER, LINEARIZER2, BARD_SCHWEITZER, EXPERIMENTAL };
    
private:
    class CreateChainIndex {
    public:
	CreateChainIndex( BCMPModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Chain::pair_t& pair );

    private:
	std::map<const std::string,size_t>& index() { return _model._index.k; }
	
    private:
	BCMPModel& _model;
    };

    class CreateStationIndex {
    public:
	CreateStationIndex( BCMPModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Station::pair_t& pair );
	std::map<const std::string,size_t>& index() { return _model._index.m; }

    private:
	BCMPModel& _model;
    };

    class InstantiateChain {
    public:
	InstantiateChain( BCMPModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Chain::pair_t& pair );

    private:
	size_t indexAt( const std::string& name ) { return _model._index.k.at(name); }
	unsigned& N(size_t k) { return _model.N[k]; }
	double& Z(size_t k) { return _model.Z[k]; }
	unsigned& priority(size_t k) { return _model.priority[k]; }
	
    private:
	BCMPModel& _model;
    };

    class InstantiateStation {
    private:
	class InstantiateClass {
	public:
	    InstantiateClass( BCMPModel& model, Server& server ) : _model(model), _server(server) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& );

	private:
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }
	    size_t indexAt(const std::string& name) const { return _model._index.k.at(name); }
	    
	private:
	    BCMPModel& _model;
	    Server& _server;
	};

    public:
	InstantiateStation( BCMPModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Station::pair_t& pair );
	size_t indexAt(const std::string& name ) const { return _model._index.m.at(name); }

    private:
	size_t n_chains() const { return _model._index.k.size(); }
	Server*& Q(size_t m) { return _model.Q[m]; }

    private:
	BCMPModel& _model;
    };

    struct Index {					/* Map string to int for chains/stations */
	std::map<const std::string,size_t> k;
	std::map<const std::string,size_t> m;
    };

    
public:
    BCMPModel( const BCMP::Model& model );
    ~BCMPModel();

    bool operator!() const { return _result == false; }
    bool instantiate();
    std::ostream& debug( std::ostream& output ) const;
    bool solve( Using );
    
private:
    void saveResults( const MVA& mva );

    /* inputs */
    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }
    const BCMP::Model::Station::map_t& stations() const { return _model.stations(); }
    const BCMP::Model::Station::Class::map_t& classesAt( const std::string& name ) const { return _model.stationAt(name).classes(); }

    const Index& index() const { return _index; }


#if 0
    class OutputQNAP {
#endif
	static std::streamsize __width;
	static std::streamsize __precision;
	static std::string __separator;

	static std::string header();
	static std::string blankline();

	std::ostream& print( std::ostream& output, double service_time, const BCMP::Model::Station::Result& item ) const;
    public:
	std::ostream& print( std::ostream& ) const;
    private:
#if 0
    };

    class OutputLQN {
    };
#endif

private:
    const BCMP::Model& _model;			/* Input */
    Index _index;				/* Map name to station/class no. */
    
    Population N;				/* Population (by class) */
    Vector<Server *> Q;				/* Stations. */
    VectorMath<double> Z;			/* Think Time */
    VectorMath<unsigned> priority;		/* Priority */
    bool _result;
    std::string _solver;
};
#endif
