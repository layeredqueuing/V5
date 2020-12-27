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
#include "xml_output.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}

namespace BCMP {
    class Model {
	
    public:
	class Class {
	    friend class Model;
	    
	public:
	    typedef std::map<const std::string,Class> map_t;
	    typedef std::pair<const std::string,Class> pair_t;

	    typedef enum { NONE, CLOSED, OPEN, MIXED } Type;
	    
	private:
	    Class() : _type(NONE), _customers(0), _think_time(0.0) {}
	    Class( Type type, unsigned int customers, double think_time ) : _type(type), _customers(customers), _think_time(think_time) {}

	public:
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

	/* -------------------------- Station ------------------------- */

	class Station {
	    friend class JMVA;
	    
	public:
	    typedef std::map<const std::string,Station> map_t;
	    typedef std::pair<const std::string,Station> pair_t;

	    typedef enum { NOT_DEFINED, DELAY, LOAD_INDEPENDENT, MULTISERVER, CUSTOMER } Type;

	    class Demand {
	    public:
		typedef std::map<const std::string,Demand> map_t;
		typedef std::pair<const std::string,Demand> pair_t;

		Demand() : _visits(0.0), _demand(0.0) {}
		Demand( double visits, double demand ) : _visits(visits), _demand(demand) {}
		
		Demand operator+( const Demand& augend ) const { return Demand( _visits + augend._visits, _demand + augend._demand ); }
		Demand& operator+=( const Demand& addend ) { _visits += addend._visits; _demand += addend._demand; return *this; }
		double visits() const { return _visits; }
		double service_time() const { return _visits > 0 ? _demand / _visits : 0.; }
		double demand() const { return _demand; }
		Demand& accumulate( double visits, double demand ) { _visits += visits; _demand += demand; return *this; }
		Demand& accumulate( const Demand& addend ) { _visits += addend._visits; _demand += addend._demand; return *this; }
		void setVisits( double visits ) { _visits = visits; }
		void setDemand( double demand ) { _demand = demand; }
		
		static Demand::map_t collect( const Demand::map_t& augend_t, const Demand::pair_t& );
	    private:
		double _visits;
		double _demand;
	    };
	
	    Station() : _type(NOT_DEFINED), _copies(1), _demands() {}
	    Station( Type type, unsigned int copies ) : _type(type), _copies(copies) {}

	public:
	    bool insertDemand( const std::string&, const Demand& );
	    
	    Type type() const { return _type; }
	    unsigned int copies() const { return _copies; }
	    const Demand::map_t& demands() const { return _demands; }
	    Demand& demandAt( const std::string& name ) { return _demands.at(name); }
	    const Demand& demandAt( const std::string& name ) const { return _demands.at(name); }

	    bool hasClass( const std::string& name ) const { return _demands.find(name) != _demands.end(); }

	    static bool isCustomer( const Station::pair_t& m ) { return m.second.type() == CUSTOMER; }
	    static bool isServer( const Station::pair_t& m ) { return m.second.type() != CUSTOMER && m.second.type() != NOT_DEFINED; }

	private:
	    Type _type;
	    unsigned int _copies;
	    Demand::map_t _demands;

	public:
	    struct select {
		typedef bool (*predicate)( const std::pair<const std::string,Model::Station>& );
		select( const predicate test ) : _test(test) {}
		Demand::map_t operator()( const Demand::map_t& augend, const Station::pair_t& ) const;
	    private:
		const predicate _test;
	    };

	    struct fold {
		fold( const std::string& suffix="" ) : _suffix(suffix) {}
		std::string operator()( const std::string& s1, const Station::pair_t& s2 ) const;
	    private:
		const std::string& _suffix;
	    };

	    struct printQNAP2Transit {
		printQNAP2Transit( const std::string& class_name ) : _class_name(class_name) {}
		std::string operator()( const std::string&, const Station::pair_t& ) const;
	    private:
		const std::string _class_name;
	    };
	    
	private:
	    struct pad_demand {
		pad_demand( const Class::map_t& classes ) : _classes(classes) {}
		void operator()( const Station::pair_t& station ) const;
	    private:
		const Class::map_t& _classes;	
	    };
	};

	/* --------------------------- Model -------------------------- */

    public:
	Model() : _classes(), _stations() {}
	virtual ~Model() {}

	bool empty() const { return _classes.size() == 0 || _stations.size() == 0; }
	const Class::map_t& classes() const { return _classes; }
	const Station::map_t& stations() const { return _stations; }
	unsigned int nClasses() const { return _classes.size(); }
	unsigned int nStations() const { return _stations.size(); }
	Station& stationAt( const std::string& name ) { return _stations.at(name); }
	const Station& stationAt( const std::string& name ) const { return _stations.at(name); }
	Class& classAt( const std::string& name ) { return _classes.at(name); }
	const Class& classAt( const std::string& name ) const { return _classes.at(name); }

	bool insertClass( const std::string&, Class::Type, unsigned int, double=0.0 );
	bool insertStation( const std::string&, Station::Type, unsigned int=1 );
	bool insertDemand( const std::string&, const std::string&, const Station::Demand& );
	void computeCustomerVisits( const std::string& );
	LQIO::DOM::Document * convertToLQN() const;
	
	virtual void print( std::ostream& ) const = 0;

    protected:
	Class::map_t _classes;
	Station::map_t _stations;

	struct sumVisits {
	    sumVisits( const Station::Demand::map_t& visits ) : _visits(visits) {}
	    Station::Demand::map_t operator()( const Station::Demand::map_t& input, const Station::Demand::pair_t& visit ) const;
	private:
	    const Station::Demand::map_t& _visits;
	};
    };

    class JMVA : public Model {
    public:
	JMVA() : Model() {}

	void print( std::ostream& ) const;

    private:
	void printClientStation( std::ostream& ) const;

    private:
	static const std::string __client_name;

	struct printClass {
	    printClass( std::ostream& output ) : _output(output) {}
	    void operator()( const std::pair<const std::string&,Class>& k ) const;
	private:
	    std::ostream& _output;
	};

	struct printStation {
	    printStation( std::ostream& output ) : _output(output) {}
	    void operator()( const Station::pair_t& m ) const;
	private:
	    std::ostream& _output;
	};

	struct printReference {
	    printReference( std::ostream& output ) : _output(output) {}
	    void operator()( const std::pair<const std::string&,Class>& k ) const;
	private:
	    std::ostream& _output;
	};

	struct printService {
	    printService( std::ostream& output ) : _output(output) {}
	    void operator()( const Station::Demand::pair_t& d ) const;
	private:
	    std::ostream& _output;
	};

	struct printVisits {
	    printVisits( std::ostream& output ) : _output(output) {}
	    void operator()( const Station::Demand::pair_t& d ) const;
	private:
	    std::ostream& _output;
	};
    };

    class QNAP2 : public Model {
    public:
	QNAP2() : Model() {}

	void print( std::ostream& ) const;

    private:
	void printClientStation( std::ostream& ) const;
	void printClassVariables( std::ostream& ) const;

	static std::ostream& printOutput( std::ostream&, const std::string& s1, const std::string& s2 );
	static XML::StringManip qnap2_output( const std::string& s1, const std::string& s2="" ) { return XML::StringManip( &QNAP2::printOutput, s1, s2 ); }

    private:
	static const std::string __client_name;

	struct printStation {
	    printStation( std::ostream& output, const Class::map_t& classes ) : _output(output), _classes(classes) {}
	    void operator()( const Station::pair_t& m ) const;
	private:
	    std::ostream& _output;
	    const Class::map_t& _classes;
	};

	struct printStationVariables {
	    printStationVariables( std::ostream& output, const Class::map_t& classes ) : _output(output), _classes(classes) {}
	    void operator()( const Station::pair_t& m ) const;
	private:
	    std::ostream& _output;
	    const Class::map_t& _classes;
	};
    };
}
#endif /* */
