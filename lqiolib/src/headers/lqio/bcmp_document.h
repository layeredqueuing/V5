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

namespace BCMP {
    class Model;
    
    class Model {
    public:
	class Class {
	    friend class Model;

	public:
	    typedef enum { NONE, CLOSED, OPEN, MIXED } Type;
	    
	private:
	    Class() : _type(NONE), _customers(0), _think_time(0.0) {}
	    Class( Type type, unsigned int customers, double think_time ) : _type(type), _customers(customers), _think_time(think_time) {}

	public:
	    Type type() const { return _type; }
	    unsigned int customers() const { return _customers; }
	    bool isInClosedModel() const { return _type == CLOSED || _type == MIXED; }
	    bool isInOpenModel() const { return _type == OPEN || _type == MIXED; }

	private:
	    void print_JMVA_class( std::ostream&, const std::string& ) const;
	    void print_JMVA_reference( std::ostream&, const std::string& ) const;

	private:
	    Type _type;
	    unsigned int _customers;
	    double _think_time;

	private:
	    struct printJMVAClass {
		printJMVAClass( std::ostream& output ) : _output(output) {}
		void operator()( const std::pair<const std::string&,Class>& k ) const { k.second.print_JMVA_class( _output, k.first ); } 
	    private:
		std::ostream& _output;
	    };

	    struct printJMVAReference {
		printJMVAReference( std::ostream& output ) : _output(output) {}
		void operator()( const std::pair<const std::string&,Class>& k ) const { k.second.print_JMVA_reference( _output, k.first ); }
	    private:
		std::ostream& _output;
	    };
	};
	typedef std::map<const std::string,Class> Class_t;

	class Station {
	    friend class Model;

	public:
	    typedef enum { NOT_DEFINED, DELAY, LOAD_INDEPENDENT, MULTISERVER, REFERENCE } Type;

	    class Demand {
	    public:
		Demand() : _visits(0.0), _demand(0.0) {}
		Demand( double visits, double demand ) : _visits(visits), _demand(demand) {}
		
		Demand operator+( const Demand& augend ) const { return Demand( _visits + augend._visits, _demand + augend._demand ); }
		Demand& operator+=( const Demand& addend ) { _visits += addend._visits; _demand += addend._demand; return *this; }
		double visits() const { return _visits; }
		double service_time() const { return _visits > 0 ? _demand / _visits : 0.; }
		Demand& accumulate( double visits, double demand ) { _visits += visits; _demand += demand; return *this; }
		Demand& accumulate( const Demand& addend ) { _visits += addend._visits; _demand += addend._demand; return *this; }
		void setDemand( double demand ) { _demand = demand; }
		
	    private:
		double _visits;
		double _demand;

	    public:
		struct select {
		    select( const std::string& name ) : _name(name) {}
		    Demand operator()( const Demand& augend, const std::pair<const std::string,Demand>& ) const;
		private:
		    const std::string& _name;
		};
		
	    };
	    typedef std::map<const std::string,Demand> Demand_t;
	
	    Station() : _type(NOT_DEFINED), _copies(1), _demands() {}
	    Station( Type type, unsigned int copies ) : _type(type), _copies(copies) {}

	public:
	    bool insertDemand( const std::string&, const Demand& );
	    
	    Type type() const { return _type; }
	    unsigned int copies() const { return _copies; }
	    const Demand_t& demands() const { return _demands; }
	    Demand& demandAt( const std::string& name ) { return _demands.at(name); }
	    const Demand& demandAt( const std::string& name ) const { return _demands.at(name); }

	private:
	    void printJMVA( std::ostream&, const std::string& ) const;
	
	private:
	    Type _type;
	    unsigned int _copies;
	    Demand_t _demands;

	public:
	    struct select {
		select( const std::string& name ) : _name(name) {}
		Demand operator()( const Demand& augend, const std::pair<const std::string,Station>& ) const;
	    private:
		const std::string& _name;
	    };

	private:
	    struct pad_demand {
		pad_demand( const Class_t& classes ) : _classes(classes) {}
		void operator()( const std::pair<const std::string,Station>& station ) const;
	    private:
		const Class_t& _classes;	
	    };

	    struct printJMVAService {
		printJMVAService( std::ostream& output ) : _output(output) {}
		void operator()( const std::pair<const std::string,Demand>& d ) const;
	    private:
		std::ostream& _output;
	    };

	    struct printJMVAVisits {
		printJMVAVisits( std::ostream& output ) : _output(output) {}
		void operator()( const std::pair<const std::string,Demand>& d ) const;
	    private:
		std::ostream& _output;
	    };
	};

	/* ------------------------ Class Model ----------------------- */

    public:
	typedef std::map<const std::string,Station> Station_t;

	Model() : _classes(), _stations() {}

	bool empty() const { return _classes.size() == 0 || _stations.size() == 0; }
	const Class_t& classes() const { return _classes; }
	const Station_t& stations() const { return _stations; }
	unsigned int nClasses() const { return _classes.size(); }
	unsigned int nStations() const { return _stations.size(); }
	Station& stationAt( const std::string& name ) { return _stations.at(name); }
	const Station& stationAt( const std::string& name ) const { return _stations.at(name); }

	bool insertClass( const std::string&, Class::Type, unsigned int, double=0.0 );
	bool insertStation( const std::string&, Station::Type, unsigned int=1 );
	bool insertDemand( const std::string&, const std::string&, const Station::Demand& );
	
	void printJMVA( std::ostream& ) const;
	void printQNAP2( std::ostream& ) const;

    private:
	Class_t _classes;
	Station_t _stations;

    private:
	struct printJMVAStation {
	    printJMVAStation( std::ostream& output ) : _output(output) {}
	    void operator()( const std::pair<const std::string,Station>& m ) const { m.second.printJMVA( _output, m.first ); }
	private:
	    std::ostream& _output;
	};
	
	struct printQNAP2Station {
	    printQNAP2Station( std::ostream& output, const Class_t& classes ) : _output(output), _classes(classes) {}
	    void operator()( const std::pair<const std::string,Station>& m ) const;
	private:
	    std::ostream& _output;
	    const Class_t& _classes;
	};

	struct printQNAP2Variable {
	    printQNAP2Variable( std::ostream& output, const Class_t& classes ) : _output(output), _classes(classes) {}
	    void operator()( const std::pair<const std::string,Station>& m ) const;
	private:
	    std::ostream& _output;
	    const Class_t& _classes;
	};
    };

    /* ---------------------------------------------------------------- */

    namespace XML {
	
	class BooleanManip {
	public:
	    BooleanManip( std::ostream& (*f)(std::ostream&, const std::string&, const bool ), const std::string& a, const bool b ) : _f(f), _a(a), _b(b) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, const bool );
	    const std::string& _a;
	    const bool _b;
	    friend std::ostream& operator<<(std::ostream & os, const BooleanManip& m ) { return m._f(os,m._a,m._b); }
	};

	class StringManip {
	public:
	    StringManip( std::ostream& (*f)(std::ostream&, const std::string&, const std::string& ), const std::string& a, const std::string& v=0 ) : _f(f), _a(a), _v(v) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, const std::string& );
	    const std::string& _a;
	    const std::string& _v;
	    friend std::ostream& operator<<(std::ostream & os, const StringManip& m ) { return m._f(os,m._a,m._v); }
	};

	class IntegerManip {
	public:
	    IntegerManip( std::ostream& (*f)(std::ostream&, const int ), const int i ) : _f(f), _i(i) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const int );
	    const int _i;
	    friend std::ostream& operator<<(std::ostream & os, const IntegerManip& m ) { return m._f(os,m._i); }
	};

	class UnsignedManip {
	public:
	    UnsignedManip( std::ostream& (*f)(std::ostream&, const std::string&, const unsigned int ), const std::string& a, const unsigned int u ) : _f(f), _a(a), _u(u) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, const unsigned int );
	    const std::string& _a;
	    const unsigned int _u;
	    friend std::ostream& operator<<(std::ostream & os, const UnsignedManip& m ) { return m._f(os,m._a,m._u); }
	};

	class DoubleManip {
	public:
	    DoubleManip( std::ostream& (*f)(std::ostream&, const std::string&, const double ), const std::string& a, const double v ) : _f(f), _a(a), _v(v) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, const double );
	    const std::string& _a;
	    const double _v;
	    friend std::ostream& operator<<(std::ostream & os, const DoubleManip& m ) { return m._f(os,m._a,m._v); }
	};

	class InlineElementManip {
	public:
	    InlineElementManip( std::ostream& (*f)(std::ostream&, const std::string&, const std::string&, const std::string&, double ), const std::string& e, const std::string& a, const std::string& v, double d )
		: _f(f), _e(e), _a(a), _v(v), _d(d) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, const std::string&, const std::string&, double );
	    const std::string& _e;
	    const std::string& _a;
	    const std::string& _v;
	    const double _d;
	    friend std::ostream& operator<<(std::ostream & os, const InlineElementManip& m ) { return m._f(os,m._e,m._a,m._v,m._d); }
	};

	int set_indent( int );
	IntegerManip indent( int );
	IntegerManip temp_indent( int );
	
	BooleanManip start_element( const std::string& e, bool b=true );
	BooleanManip end_element( const std::string& e, bool b=true );
	BooleanManip simple_element( const std::string& e );
	InlineElementManip inline_element( const std::string& e, const std::string& a, const std::string& v, double d );
	StringManip attribute( const std::string& a, const std::string& v );
	DoubleManip attribute( const std::string&a, double v );
	UnsignedManip attribute( const std::string&a, unsigned int v );
    }
    
}

#endif /* */
