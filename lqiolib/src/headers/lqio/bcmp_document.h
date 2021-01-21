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
#include "dom_extvar.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}

namespace BCMP {
    using namespace LQIO;
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
	    friend class Model;

	public:
	    typedef std::map<const std::string,Class> map_t;
	    typedef std::pair<const std::string,Class> pair_t;

	    typedef enum { NONE, CLOSED, OPEN, MIXED } Type;
	    
	public:
	    Class() : _type(NONE), _customers(nullptr), _think_time(new DOM::ConstantExternalVariable(0.)) {}
	    Class( Type type, const DOM::ExternalVariable* customers, const DOM::ExternalVariable* think_time ) : _type(type), _customers(customers), _think_time(think_time) {}

	    Type type() const { return _type; }
	    const DOM::ExternalVariable* customers() const { return _customers; }
	    void setCustomers( DOM::ExternalVariable* customers ) { _customers = customers; }
	    const DOM::ExternalVariable* think_time() const { return _think_time; }
	    void setThinkTime( DOM::ExternalVariable* think_time ) { _think_time = think_time; }
	    bool isInClosedModel() const { return _type == CLOSED || _type == MIXED; }
	    bool isInOpenModel() const { return _type == OPEN || _type == MIXED; }

	    struct fold {
		fold( const std::string& suffix="" ) : _suffix(suffix) {}
		std::string operator()( const std::string& s1, const Class::pair_t& c2 ) const;
	    private:
		const std::string& _suffix;
	    };

	    static bool has_constant_customers( const Class::pair_t& );

	private:
	    Type _type;
	    const DOM::ExternalVariable * _customers;
	    const DOM::ExternalVariable * _think_time;

	};

	/* ------------------------------------------------------------ */
	/*                           Station                            */
	/* ------------------------------------------------------------ */

	class Station : public Object {
	    friend class Model;
	    
	public:
	    typedef std::map<const std::string,Station> map_t;
	    typedef std::pair<const std::string,Station> pair_t;

	    typedef enum { NOT_DEFINED, DELAY, LOAD_INDEPENDENT, MULTISERVER, CUSTOMER } Type;

	    class Demand {
		friend class Station;

	    public:
		typedef std::map<const std::string,Demand> map_t;
		typedef std::pair<const std::string,Demand> pair_t;

		Demand() : _visits(nullptr), _service_time(nullptr) {}
		Demand( const DOM::ExternalVariable* visits, const DOM::ExternalVariable* service_time ) : _visits(visits), _service_time(service_time) {}
		~Demand() {}
		
		const DOM::ExternalVariable* visits() const { return _visits; }
		const DOM::ExternalVariable* service_time() const { return _service_time; }
		void setVisits( const DOM::ExternalVariable* visits ) { _visits = visits; }
		void setServiceTime( const DOM::ExternalVariable* service_time ) { _service_time = service_time; }
		
		Demand operator+( const Demand& augend ) const;
		Demand& operator+=( const Demand& addend );
		Demand& accumulate( double visits, double demand );
		Demand& accumulate( const Demand& addend );

		static bool has_constant_service_time( const Demand::pair_t& );
		static bool has_constant_visits( const Demand::pair_t& );

	    private:
		static Demand::map_t collect( const Demand::map_t& augend_t, const Demand::pair_t& );

	    private:
		const DOM::ExternalVariable* _visits;
		const DOM::ExternalVariable* _service_time;
	    };
	
	/* -------------------------- Station ------------------------- */

	public:
	    Station() : _type(NOT_DEFINED), _scheduling(SCHEDULE_DELAY), _copies(nullptr), _demands() {}
	    Station( Type type, scheduling_type scheduling=SCHEDULE_DELAY, const DOM::ExternalVariable* copies=nullptr ) : _type(type), _scheduling(scheduling), _copies(copies) {}
	    ~Station();
	    
	    bool insertDemand( const std::string&, const Demand& );
	    
	    Type type() const { return _type; }
	    void setType(Type type) { _type = type; }
	    scheduling_type scheduling() const { return _scheduling; }
	    const DOM::ExternalVariable* copies() const { return _copies; }
	    Demand::map_t& demands() { return _demands; }
	    const Demand::map_t& demands() const { return _demands; }
	    Demand& demandAt( const std::string& name ) { return _demands.at(name); }
	    const Demand& demandAt( const std::string& name ) const { return _demands.at(name); }

	    bool hasClass( const std::string& name ) const { return _demands.find(name) != _demands.end(); }

	    static bool isCustomer( const Station::pair_t& m ) { return m.second.type() == CUSTOMER; }
	    static bool isServer( const Station::pair_t& m ) { return m.second.type() != CUSTOMER && m.second.type() != NOT_DEFINED; }
	    bool hasConstantServiceTime() const;
	    bool hasConstantVisits() const;

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
	    const DOM::ExternalVariable* _copies;
	    Demand::map_t _demands;
	};

	/* ------------------------------------------------------------ */
	/*                            Model                             */
	/* ------------------------------------------------------------ */

    public:
	Model() : _comment(), _classes(), _stations() {}
	virtual ~Model();

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

	bool hasConstantCustomers() const;

	bool insertComment( const std::string comment ) { _comment = comment; return true; }
	bool insertClass( const std::string&, Class::Type, const DOM::ExternalVariable *, const DOM::ExternalVariable * service_time=nullptr );
	bool insertStation( const std::string&, const Station& ); 
	bool insertStation( const std::string& name, Station::Type type, scheduling_type scheduling=SCHEDULE_DELAY, const DOM::ExternalVariable* copies=nullptr ) { return insertStation( name, Station( type, scheduling, copies ) ); }
	bool insertDemand( const std::string&, const std::string&, const Station::Demand& );

	Station::Demand::map_t computeCustomerDemand( const std::string& ) const;
	bool convertToLQN( DOM::Document& ) const;
	
	virtual std::ostream& print( std::ostream& output ) const;	/* NOP (lqn2ps will render) */
	static bool isSet( const LQIO::DOM::ExternalVariable * var, double default_value=0.0 );

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
