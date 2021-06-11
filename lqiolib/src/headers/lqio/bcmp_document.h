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
	    virtual const char * getTypeName() const = 0;

	private:
	    std::string _comment;
	};

	class Result {
	public:
	    enum class Type { QUEUE_LENGTH, RESIDENCE_TIME, RESPONSE_TIME, THROUGHPUT, UTILIZATION };
	    typedef std::map<Type,std::string> map_t;
	    typedef std::pair<Type,std::string> pair_t;

	    Result() {}

	    virtual double throughput() const = 0;
	    virtual double queue_length() const = 0;
	    virtual double residence_time() const = 0;
	    virtual double utilization() const = 0;
	};

	/* ------------------------------------------------------------ */
	/*                            Chain                             */
	/* ------------------------------------------------------------ */

    public:
	class Chain : public Object  {
	    friend class Model;

	public:
	    typedef std::map<const std::string,Chain> map_t;
	    typedef std::pair<const std::string,Chain> pair_t;

	    enum class Type { UNDEFINED, CLOSED, OPEN };

	public:
	    Chain( Type type, const DOM::ExternalVariable* customers, const DOM::ExternalVariable* think_time ) : _type(Type::CLOSED), _customers(customers), _think_time(think_time), _arrival_rate(nullptr) { assert(type==Type::CLOSED); }
	    Chain( Type type, const DOM::ExternalVariable* arrival_rate ) : _type(Type::OPEN), _customers(nullptr), _think_time(nullptr), _arrival_rate(arrival_rate) { assert(type==Type::OPEN); }

	    virtual const char * getTypeName() const { return __typeName; }
	    Type type() const { return _type; }
 	    const DOM::ExternalVariable* customers() const { assert(type()==Type::CLOSED); return _customers; }
	    void setCustomers( DOM::ExternalVariable* customers ) { assert(type()==Type::CLOSED); _customers = customers; }
	    const DOM::ExternalVariable* think_time() const { assert(type()==Type::CLOSED); return _think_time; }
	    void setThinkTime( DOM::ExternalVariable* think_time ) { assert(type()==Type::CLOSED); _think_time = think_time; }
	    const DOM::ExternalVariable* arrival_rate() const { assert(type()==Type::OPEN); return _arrival_rate; }
	    void setArrivalRate( DOM::ExternalVariable* arrival_rate ) { assert(type()==Type::OPEN); _arrival_rate = arrival_rate; }
	    bool isClosed() const { return _type == Type::CLOSED; }
	    bool isOpen() const { return _type == Type::OPEN; }
	    static bool closedChain( const Chain::pair_t& k ) { return k.second.type() == Type::CLOSED; }
	    static bool openChain( const Chain::pair_t& k ) { return k.second.type() == Type::OPEN; }

	    virtual double throughput() const { return 0; }
	    virtual double queue_length() const { return 0; }
	    virtual double residence_time() const { return 0; }
	    virtual double utilization() const { return 0; }

	    struct fold {
		fold( const std::string& suffix="" ) : _suffix(suffix) {}
		std::string operator()( const std::string& s1, const Chain::pair_t& c2 ) const;
	    private:
		const std::string& _suffix;
	    };

	private:
	    struct is_a {
		is_a( Type type ) : _type(type) {}
		bool operator()( const Chain::pair_t& chain ) const { return chain.second.type() == _type; }
	    private:
		const Type _type;
	    };

	public:
	    static const char * const __typeName;

	private:
	    Type _type;
	    const DOM::ExternalVariable * _customers;
	    const DOM::ExternalVariable * _think_time;
	    const DOM::ExternalVariable * _arrival_rate;
	};

	/* -------------------------- Station ------------------------- */

	class Station : public Object, public Result {
	    friend class Model;

	public:
	    typedef std::map<const std::string,Station> map_t;
	    typedef std::pair<const std::string,Station> pair_t;

	    enum class Type { NOT_DEFINED, DELAY, LOAD_INDEPENDENT, MULTISERVER, SOURCE };

	    /* -------------------------------------------------------- */
	    /*                          Class                           */
	    /* -------------------------------------------------------- */

	    class Class : public Result {
		friend class Station;

	    public:
		typedef std::map<const std::string,Class> map_t;
		typedef std::pair<const std::string,Class> pair_t;

		Class( const DOM::ExternalVariable* visits=nullptr, const DOM::ExternalVariable* service_time=nullptr );
		~Class() {}

		void setResults( double throughput, double queue_length, double residence_time, double utilization );
		virtual const char * getTypeName() const { return __typeName; }

		const DOM::ExternalVariable* visits() const { return _visits; }
		const DOM::ExternalVariable* service_time() const { return _service_time; }
		void setVisits( const DOM::ExternalVariable* visits ) { _visits = visits; }
		void setServiceTime( const DOM::ExternalVariable* service_time ) { _service_time = service_time; }
		Result::map_t& resultVariables()  { return _result_vars; }
		const Result::map_t& resultVariables() const { return _result_vars; }

		Class operator+( const Class& augend ) const;
		Class& operator+=( const Class& addend );
		Class& accumulate( double visits, double demand );
		Class& accumulate( const Class& addend );

		double throughput() const { return _results.at(Result::Type::THROUGHPUT); }
		double queue_length() const { return _results.at(Result::Type::QUEUE_LENGTH); }
		double residence_time() const { return _results.at(Result::Type::RESIDENCE_TIME); }
		double utilization() const { return _results.at(Result::Type::UTILIZATION); }
		Class& deriveResidenceTime();
		void insertResultVariable( Result::Type, const std::string& );

	    private:
		static Class::map_t collect( const Class::map_t& augend_t, const Class::pair_t& );

	    public:
		static const char * const __typeName;

	    private:
		const DOM::ExternalVariable* _visits;
		const DOM::ExternalVariable* _service_time;
		std::map<Result::Type,double> _results;
		Result::map_t _result_vars;
	    };

	/* ------------------------------------------------------------ */
	/*                           Station                            */
	/* ------------------------------------------------------------ */

	public:
	    Station( Type type=Type::NOT_DEFINED, scheduling_type scheduling=SCHEDULE_DELAY, const DOM::ExternalVariable* copies=nullptr ) :
		_type(type), _scheduling(scheduling), _copies(copies), _reference(false) {}
	    ~Station();

	    std::pair<Station::Class::map_t::iterator,bool> insertClass( const std::string&, const Class& );
	    std::pair<Station::Class::map_t::iterator,bool> insertClass( const std::string&, const DOM::ExternalVariable* visits=nullptr, const DOM::ExternalVariable* service_time=nullptr );
	    void insertResultVariable( Result::Type, const std::string& );

	    virtual const char * getTypeName() const { return __typeName; }
	    Type type() const { return _type; }
	    void setType(Type type) { _type = type; }
	    scheduling_type scheduling() const { return _scheduling; }
	    const DOM::ExternalVariable* copies() const { return _copies; }
	    void setCopies( const DOM::ExternalVariable* copies ) { _copies = copies; }
	    bool reference() const { return _reference; }
	    void setReference( bool reference ) { _reference = reference; }
	    Class::map_t& classes() { return _classes; }
	    const Class::map_t& classes() const { return _classes; }
	    Class& classAt( const std::string& name ) { return _classes.at(name); }
	    const Class& classAt( const std::string& name ) const { return _classes.at(name); }
	    Result::map_t& resultVariables() { return _result_vars; }
	    const Result::map_t& resultVariables() const { return _result_vars; }

	    bool hasClass( const std::string& name ) const { return _classes.find(name) != _classes.end(); }
	    Class::map_t::const_iterator findClass( const Class * k ) const;

	    static bool isCustomer( const Station::pair_t& m ) { return m.second.reference(); }
	    static bool isServer( const Station::pair_t& m ) { return !m.second.reference() && m.second.type() != Type::NOT_DEFINED; }

	    virtual double throughput() const;
	    virtual double queue_length() const;
	    virtual double residence_time() const;
	    virtual double utilization() const;
	    static Class sumResults( const Class& augend, const Class::pair_t& addend );

	    struct select {
		typedef bool (*predicate)( const Station::pair_t& );
		select( const predicate test ) : _test(test) {}
		Class::map_t operator()( const Class::map_t& augend, const Station::pair_t& m ) const;
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

	    bool any_of( const Chain::map_t& chains, Chain::Type type ) const;
	    size_t count_if( const Chain::map_t& chains, Chain::Type type ) const;
	
	    struct is_a {
		is_a( const Model& model, Chain::Type type ) : _model(model), _type(type) {}
		bool operator()( const Station::pair_t& station ) const { return _type == Chain::Type::UNDEFINED || station.second.any_of( chains(), _type ); }
	    private:
		const Chain::map_t& chains() const { return _model.chains(); }

	    private:
		const Model& _model;
		const Chain::Type _type;
	    };

	private:
	    static double sum_throughput( double addend, const BCMP::Model::Station::Class::pair_t& augend );
	    static double sum_utilization( double addend, const BCMP::Model::Station::Class::pair_t& augend );
	    static double sum_residence_time( double addend, const BCMP::Model::Station::Class::pair_t& augend );
	    static double sum_queue_length( double addend, const BCMP::Model::Station::Class::pair_t& augend );

	public:
	    static const char * const __typeName;

	private:
	    Type _type;
	    scheduling_type _scheduling;
	    const DOM::ExternalVariable* _copies;
	    bool _reference;
	    Class::map_t _classes;
	    Result::map_t _result_vars;
	};

	/* ------------------------------------------------------------ */
	/*                            Bound                             */
	/* ------------------------------------------------------------ */

	class Bound {
	public:
	    Bound( const Chain::pair_t& chain, const Station::map_t& stations );

	    double think_time() const;

	    double D_max() const { return _D_max; }
	    double D_sum() const { return _D_sum; }
	    double Z() const { return _Z; }
	
	private:
	    const std::string& chain() const { return _chain.first; }
	    const Station::map_t& stations() const { return _stations; }

	    void compute();

	    struct max_demand {
		max_demand( const std::string& chain ) : _class(chain) {}
		double operator()( double, const Station::pair_t& );
	    private:
		const std::string& _class;
	    };
	
	    struct sum_demand {
		sum_demand( const std::string& chain ) : _class(chain) {}
		double operator()( double, const Station::pair_t& );
	    private:
		const std::string& _class;
	    };
	
	    struct sum_think_time {
		sum_think_time( const std::string& chain ) : _class(chain) {}
		double operator()( double, const Station::pair_t& );
	    private:
		const std::string& _class;
	    };

	    const Chain::pair_t _chain;
	    const Station::map_t& _stations;
	    double _D_max;
	    double _D_sum;
	    double _Z;
	};

	/* ------------------------------------------------------------ */
	/*                            Model                             */
	/* ------------------------------------------------------------ */

    public:
	Model() : _comment(), _chains(), _stations() {}
	virtual ~Model();

	bool empty() const { return _chains.size() == 0 || _stations.size() == 0; }
	const std::string& comment() const { return _comment; }
	Chain::map_t& chains() { return _chains; }
	const Chain::map_t& chains() const { return _chains; }
	Station::map_t& stations() { return _stations; }
	const Station::map_t& stations() const { return _stations; }
	Station& stationAt( const std::string& name ) { return _stations.at(name); }
	const Station& stationAt( const std::string& name ) const { return _stations.at(name); }
	Chain& chainAt( const std::string& name ) { return _chains.at(name); }
	const Chain& chainAt( const std::string& name ) const { return _chains.at(name); }

	size_t n_chains(Chain::Type) const;
	size_t n_stations(Chain::Type) const;
	
	Station::map_t::const_iterator findStation( const Station* m ) const;

	bool insertComment( const std::string comment ) { _comment = comment; return true; }
	std::pair<Chain::map_t::iterator,bool> insertClosedChain( const std::string&, const DOM::ExternalVariable *, const DOM::ExternalVariable * think_time=nullptr );
	std::pair<Chain::map_t::iterator,bool> insertOpenChain( const std::string&, const DOM::ExternalVariable * );
	std::pair<Station::map_t::iterator,bool> insertStation( const std::string&, const Station& );
	std::pair<Station::map_t::iterator,bool> insertStation( const std::string& name, Station::Type type, scheduling_type scheduling=SCHEDULE_DELAY, const DOM::ExternalVariable* copies=nullptr ) { return insertStation( name, Station( type, scheduling, copies ) ); }

	Station::Class::map_t computeCustomerDemand( const std::string& ) const;
	bool convertToLQN( DOM::Document& ) const;

	double response_time( const std::string& ) const;
	double throughput( const std::string& ) const;

	virtual std::ostream& print( std::ostream& output ) const;	/* NOP (lqn2ps will render) */
	static bool isSet( const LQIO::DOM::ExternalVariable * var, double default_value=0.0 );

	struct pad_demand {
	    pad_demand( const Chain::map_t& chains ) : _chains(chains) {}
	    void operator()( const Station::pair_t& station ) const;
	private:
	    const Chain::map_t& _chains;
	};

	struct sum_visits {
	    sum_visits( const Station::Class::map_t& visits ) : _visits(visits) {}
	    Station::Class::map_t operator()( const Station::Class::map_t& input, const Station::Class::pair_t& visit ) const;
	private:
	    const Station::Class::map_t& _visits;
	};

	struct sum_residence_time {
	    sum_residence_time( const std::string& name ) : _name(name) {}
	    double operator()( double augend, const Station::pair_t& m ) const;
	private:
	    const std::string& _name;
	};

    private:
	struct update_demand {
	    update_demand( Station& station, Station::Class::map_t& classes ) : _station(station), _classes(classes) {}
	    void operator()( const Chain::pair_t& k ) { _station.classAt( k.first ) = _classes.at( k.first ); }
	private:
	    Station& _station;
	    const Station::Class::map_t& _classes;
	};

    private:
	std::string _comment;
	Chain::map_t _chains;
	Station::map_t _stations;
    };
}
#endif /* */
