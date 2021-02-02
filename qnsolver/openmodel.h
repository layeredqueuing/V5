/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/openmodel.h $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: openmodel.h 14440 2021-02-02 12:44:31Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(OPENMODEL_H)
#define OPENMODEL_H
#include <config.h>
#include <string>
#include <map>
#include <lqio/bcmp_document.h>
#include <mva/vector.h>


class Server;
class Open;
class ClosedModel;

class OpenModel {
private:
    class CreateChainIndex {
    public:
	CreateChainIndex( OpenModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Chain::pair_t& pair );

    private:
	std::map<const std::string,size_t>& index() { return _model._index.k; }
	
    private:
	OpenModel& _model;
    };

    class CreateStationIndex {
    public:
	CreateStationIndex( OpenModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Station::pair_t& pair );
	std::map<const std::string,size_t>& index() { return _model._index.m; }

    private:
	OpenModel& _model;
    };

    class InstantiateChain {
    public:
	InstantiateChain( OpenModel& model ) : _model(model) {}
	void operator()( const BCMP::Model::Chain::pair_t& pair );

    private:
	size_t indexAt( const std::string& name ) { return _model._index.k.at(name); }
	
    private:
	OpenModel& _model;
    };

    class InstantiateStation {
    private:
	class InstantiateClass {
	public:
	    InstantiateClass( OpenModel& model, Server& server ) : _model(model), _server(server) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& );

	private:
	    const BCMP::Model::Chain& chainAt( const std::string& name ) const { return _model._model.chainAt(name); }
	    size_t indexAt(const std::string& name) const { return _model._index.k.at(name); }
	    
	private:
	    const OpenModel& _model;
	    Server& _server;
	};

    public:
	InstantiateStation( OpenModel& model ) : _model(model), E(model._index.k.size()) {}
	void operator()( const BCMP::Model::Station::pair_t& pair );
	size_t indexAt(const std::string& name ) const { return _model._index.m.at(name); }

    private:
	Server*& Q(size_t m) { return _model.Q[m]; }

    private:
	OpenModel& _model;
	size_t E;
    };

    struct Index {					/* Map string to int for chains/stations */
	std::map<const std::string,size_t> k;
	std::map<const std::string,size_t> m;
    };

    
public:
    OpenModel( const BCMP::Model& model );
    ~OpenModel();

    explicit operator bool() const { return _result == true; }
    bool instantiate();
    std::ostream& debug( std::ostream& output ) const;
    bool solve( ClosedModel& );
    
private:
    void saveResults( const Open& open );

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
    
    Vector<Server *> Q;				/* Stations. */
    bool _result;
};
#endif
