/* -*- C++ -*-
 *  $Id: expat_document.h 13717 2020-08-03 00:04:28Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO_BCMP_DOCUMENT__
#define __LQIO_BCMP_DOCUMENT__

#include <map>
#include <string>
#include <iostream>
#include "input.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}

namespace BCMP {
    class Model {
	

    public:
	class Object {
	public:
	    Object() : _comment() {}
	    virtual ~Object() {}

	    std::string& getComment() { return _comment; }

	private:
	    std::string _comment;
	};
	

	/* ------------------------------------------------------------ */
	/*                           Station                            */
	/* ------------------------------------------------------------ */

    public:
	class Class : public Object {
	    
	public:
	    typedef std::map<const std::string,Class> map_t;
	    typedef std::pair<const std::string,Class> pair_t;

	    typedef enum { NONE, CLOSED, OPEN, MIXED } Type;
	    
	public:
	    Class() : _type(NONE), _customers(0), _think_time(0.0) {}
	    Class( Type type, unsigned int customers, double think_time ) : _type(type), _customers(customers), _think_time(think_time) {}

	    Type type() const { return _type; }
	    unsigned int customers() const { return _customers; }
	    void setCustomers( unsigned int customers ) { _customers = customers; }
	    double think_time() const { return _think_time; }
	    void setThinkTime( double think_time ) { _think_time = think_time; }
	    bool isInClosedModel() const { return _type == CLOSED || _type == MIXED; }
	    bool isInOpenModel() const { return _type == OPEN || _type == MIXED; }

	private:
	    Type _type;
	    unsigned int _customers;
	    double _think_time;

	public:
	    struct fold {
		fold( const std::string& suffix="" ) : _suffix(suffix) {}
		std::string operator()( const std::string& s1, const Class::pair_t& c2 ) const;
	    private:
		const std::string& _suffix;
	    };
	};

	/* ------------------------------------------------------------ */
	/*                           Station                            */
	/* ------------------------------------------------------------ */

	class Station : public Object {
	    
	public:
	    typedef std::map<const std::string,Station> map_t;
	    typedef std::pair<const std::string,Station> pair_t;

	    typedef enum { NOT_DEFINED, DELAY, LOAD_INDEPENDENT, MULTISERVER, CUSTOMER } Type;

	    class Demand {
	    public:
		typedef std::map<const std::string,Demand> map_t;
		typedef std::pair<const std::string,Demand> pair_t;

		Demand() : _visits(0.0), _service_time(0.0) {}
		Demand( double visits, double service_time ) : _visits(visits), _service_time(service_time) {}
		
		double visits() const { return _visits; }
		double service_time() const { return _service_time; }
		double demand() const { return _service_time * _visits; }
		void setVisits( double visits ) { _visits = visits; }
		void setServiceTime( double service_time ) { _service_time = service_time; }
		
		Demand operator+( const Demand& augend ) const { return Demand( _visits + augend._visits, _service_time + augend._service_time ); }
		Demand& operator+=( const Demand& addend ) { _visits += addend._visits; _service_time += addend._service_time; return *this; }
		Demand& accumulate( double visits, double demand ) { _visits += visits; _service_time += demand; return *this; }
		Demand& accumulate( const Demand& addend ) { _visits += addend._visits; _service_time += addend._service_time; return *this; }
		static Demand::map_t collect( const Demand::map_t& augend_t, const Demand::pair_t& );

	    private:
		double _visits;
		double _service_time;
	    };
	
	/* -------------------------- Station ------------------------- */

	public:
	    Station() : _type(NOT_DEFINED), _scheduling(SCHEDULE_DELAY), _copies(1), _demands() {}
	    Station( Type type, scheduling_type scheduling=SCHEDULE_DELAY, unsigned int copies=1 ) : _type(type), _scheduling(scheduling), _copies(copies) {}

	    bool insertDemand( const std::string&, const Demand& );
	    
	    Type type() const { return _type; }
	    void setType(Type type) { _type = type; }
	    scheduling_type scheduling() const { return _scheduling; }
	    unsigned int copies() const { return _copies; }
	    Demand::map_t& demands() { return _demands; }
	    const Demand::map_t& demands() const { return _demands; }
	    Demand& demandAt( const std::string& name ) { return _demands.at(name); }
	    const Demand& demandAt( const std::string& name ) const { return _demands.at(name); }

	    bool hasClass( const std::string& name ) const { return _demands.find(name) != _demands.end(); }

	    static bool isCustomer( const Station::pair_t& m ) { return m.second.type() == CUSTOMER; }
	    static bool isServer( const Station::pair_t& m ) { return m.second.type() != CUSTOMER && m.second.type() != NOT_DEFINED; }

	public:
	    struct select {
		typedef bool (*predicate)( const Station::pair_t& );
		select( const predicate test ) : _test(test) {}
		Demand::map_t operator()( const Demand::map_t& augend, const Station::pair_t& m ) const;
	    private:
		const predicate _test;
	    };

	    struct test {
		typedef bool (*predicate)( const Station::pair_t& m );
		test( const predicate t ) : _test(t) {}
		bool operator()( const Station::pair_t& m ) const { return (*_test)( m ); }
	    private:
		const predicate _test;
	    };
	    
	    struct fold {
		fold( const std::string& suffix="" ) : _suffix(suffix) {}
		std::string operator()( const std::string& s1, const Station::pair_t& m ) const;
	    private:
		const std::string& _suffix;
	    };
  
	private:
	    Type _type;
	    scheduling_type _scheduling;
	    unsigned int _copies;
	    Demand::map_t _demands;
	};

	/* ------------------------------------------------------------ */
	/*                            Model                             */
	/* ------------------------------------------------------------ */

    public:
	Model() : _comment(), _classes(), _stations() {}
	virtual ~Model() {}

	bool empty() const { return _classes.size() == 0 || _stations.size() == 0; }
	const std::string& comment() const { return _comment; }
	Class::map_t& classes() { return _classes; }
	const Class::map_t& classes() const { return _classes; }
	Station::map_t& stations() { return _stations; }
	const Station::map_t& stations() const { return _stations; }
	Station& stationAt( const std::string& name ) { return _stations.at(name); }
	const Station& stationAt( const std::string& name ) const { return _stations.at(name); }
	Class& classAt( const std::string& name ) { return _classes.at(name); }
	const Class& classAt( const std::string& name ) const { return _classes.at(name); }

	bool insertComment( const std::string comment ) { _comment = comment; return true; }
	bool insertClass( const std::string&, Class::Type, unsigned int, double=0.0 );
	bool insertStation( const std::string&, const Station& ); 
	bool insertStation( const std::string& name, Station::Type type, scheduling_type scheduling=SCHEDULE_DELAY, unsigned int copies=1 ) { return insertStation( name, Station( type, scheduling, copies ) ); }
	bool insertDemand( const std::string&, const std::string&, const Station::Demand& );

	Station::Demand::map_t computeCustomerDemand( const std::string& ) const;
	bool convertToLQN( LQIO::DOM::Document& ) const;
	
	virtual std::ostream& print( std::ostream& output ) const;	/* NOP (lqn2ps will render) */

	struct pad_demand {
	    pad_demand( const Class::map_t& classes ) : _classes(classes) {}
	    void operator()( const Station::pair_t& station ) const;
	private:
	    const Class::map_t& _classes;	
	};

	struct sum_visits {
	    sum_visits( const Station::Demand::map_t& visits ) : _visits(visits) {}
	    Station::Demand::map_t operator()( const Station::Demand::map_t& input, const Station::Demand::pair_t& visit ) const;
	private:
	    const Station::Demand::map_t& _visits;
	};

    private:
	struct update_demand {
	    update_demand( Station& station, Station::Demand::map_t& demands ) : _station(station), _demands(demands) {}
	    void operator()( const Class::pair_t& k ) { _station.demandAt( k.first ) = _demands.at( k.first ); }
	private:
	    Station& _station;
	    const Station::Demand::map_t& _demands;
	};

    private:
	std::string _comment;
	Class::map_t _classes;
	Station::map_t _stations;
    };
}
#endif /* */
