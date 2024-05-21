/* -*- C++ -*-
 *  $Id: bcmp_document.h 17220 2024-05-16 16:00:29Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO_BCMP_DOCUMENT__
#define __LQIO_BCMP_DOCUMENT__

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include "input.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}
namespace LQX {
    class SyntaxTreeNode;
    class Environment;
}

namespace BCMP {
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
	    enum class Type { NONE, QUEUE_LENGTH, RESIDENCE_TIME, RESPONSE_TIME, MEAN_SERVICE, THROUGHPUT, UTILIZATION };
	    typedef std::map<Type,std::string> map_t;
	    typedef std::pair<Type,std::string> pair_t;

	    Result() {}

	    virtual double mean_service() const = 0;
	    virtual double queue_length() const = 0;
	    virtual double residence_time() const = 0;		// waiting time multiplied by visits
	    virtual double response_time() const = 0;		// waiting time per visit.
	    virtual double throughput() const = 0;
	    virtual double utilization() const = 0;

	public:
	    static const map_t suffix;
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
	    Chain() : _type(Type::UNDEFINED), _arrival_rate(nullptr), _customers(nullptr), _think_time(nullptr), _priority(nullptr), _result_vars() {}
	    Chain( Type type, LQX::SyntaxTreeNode * customers, LQX::SyntaxTreeNode * think_time, LQX::SyntaxTreeNode * priority=nullptr ) : _type(Type::CLOSED), _arrival_rate(nullptr), _customers(customers), _think_time(think_time), _priority(priority), _result_vars() { assert(type==Type::CLOSED); }
	    Chain( Type type, LQX::SyntaxTreeNode * arrival_rate ) : _type(Type::OPEN), _arrival_rate(arrival_rate), _customers(nullptr), _think_time(nullptr), _priority(nullptr), _result_vars() { assert(type==Type::OPEN); }

	    virtual const char * getTypeName() const { return __typeName; }
	    Type type() const { return _type; }
	    void setType( Type type ) { _type = type; }
 	    LQX::SyntaxTreeNode * customers() const { assert(type()==Type::CLOSED); return _customers; }
	    void setCustomers( LQX::SyntaxTreeNode* customers ) { assert(type()==Type::CLOSED); _customers = customers; }
 	    LQX::SyntaxTreeNode * priority() const { assert(type()==Type::CLOSED); return _priority; }
	    void setPriority( LQX::SyntaxTreeNode* priority ) { assert(type()==Type::CLOSED); _priority = priority; }
	    LQX::SyntaxTreeNode * think_time() const { assert(type()==Type::CLOSED); return _think_time; }
	    void setThinkTime( LQX::SyntaxTreeNode* think_time ) { assert(type()==Type::CLOSED); _think_time = think_time; }
	    LQX::SyntaxTreeNode * arrival_rate() const { assert(type()==Type::OPEN); return _arrival_rate; }
	    void setArrivalRate( LQX::SyntaxTreeNode* arrival_rate ) { assert(type()==Type::OPEN); _arrival_rate = arrival_rate; }
	    Result::map_t& resultVariables()  { return _result_vars; }
	    const Result::map_t& resultVariables() const { return _result_vars; }

	    bool isClosed() const { return _type == Type::CLOSED; }
	    bool isOpen() const { return _type == Type::OPEN; }
	    static bool closedChain( const Chain::pair_t& k ) { return k.second.type() == Type::CLOSED; }
	    static bool openChain( const Chain::pair_t& k ) { return k.second.type() == Type::OPEN; }

	    virtual double mean_service() const { return 0; }
	    virtual double queue_length() const { return 0; }
	    virtual double residence_time() const { return 0; }
	    virtual double responce_time() const { return 0; }
	    virtual double throughput() const { return 0; }
	    virtual double utilization() const { return 0; }
	    void insertResultVariable( Result::Type, const std::string& );

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
	    LQX::SyntaxTreeNode * _arrival_rate;
	    LQX::SyntaxTreeNode * _customers;
	    LQX::SyntaxTreeNode * _think_time;
	    LQX::SyntaxTreeNode * _priority;
	    Result::map_t _result_vars;
	};

	/* -------------------------- Station ------------------------- */

	class Station : public Object, public Result {
	    friend class Model;

	public:
	    typedef std::map<const std::string,Station> map_t;
	    typedef std::pair<const std::string,Station> pair_t;

	    enum class Type { NOT_DEFINED, DELAY, LOAD_INDEPENDENT, MULTISERVER, SOURCE };
	    enum class Distribution { EXPONENTIAL, HYPER_EXPONENTIAL };

	    /* -------------------------------------------------------- */
	    /*                          Class                           */
	    /* -------------------------------------------------------- */

	    class Class : public Result {
		friend class Station;

	    public:
		typedef std::map<const std::string,Class> map_t;
		typedef std::pair<const std::string,Class> pair_t;

		Class( LQX::SyntaxTreeNode * visits=nullptr, LQX::SyntaxTreeNode * service_time=nullptr, LQX::SyntaxTreeNode * service_shape=nullptr );
		~Class() {}

		Class& operator=( const Class& );
		void clear();

		void setResults( double throughput, double queue_length, double residence_time, double utilization );
		virtual const char * getTypeName() const { return __typeName; }

		LQX::SyntaxTreeNode * visits() const { return _visits; }
		LQX::SyntaxTreeNode * service_time() const { return _service_time; }
		LQX::SyntaxTreeNode * service_shape() const { return _service_shape; }
		void setVisits( LQX::SyntaxTreeNode * visits ) { _visits = visits; }
		void setServiceTime( LQX::SyntaxTreeNode * service_time ) { _service_time = service_time; }
		void setServiceShape( LQX::SyntaxTreeNode * service_shape ) { _service_shape = service_shape; }
		const std::map<Result::Type,double>& results() const { return _results; }
		Result::map_t& resultVariables()  { return _result_vars; }
		const Result::map_t& resultVariables() const { return _result_vars; }

		Class operator+( const Class& augend ) const;
		Class& operator+=( const Class& addend );
		Class& accumulate( double visits, double demand );
		Class& accumulate( const Class& addend );
		Class& accumulateResults( const Class& addend );

		virtual double mean_service() const { return throughput() > 0. ? utilization() / throughput() : 0.; }
		virtual double queue_length() const { return _results.at(Result::Type::QUEUE_LENGTH); }
		virtual double response_time() const { return throughput() > 0. ? queue_length() / throughput() : 0.; }	// Per visit.
		virtual double residence_time() const { return _results.at(Result::Type::RESIDENCE_TIME); }	// Over all visits.
		virtual double throughput() const { return _results.at(Result::Type::THROUGHPUT); }
		virtual double utilization() const { return _results.at(Result::Type::UTILIZATION); }
		void insertResultVariable( Result::Type, const std::string& );

		struct print {
		    print( std::ostream& output ) : _output(output) {}
		    void operator()( const Class::pair_t& ) const;
		private:
		    std::ostream& _output;
		};

	    private:
		static Class::map_t collect( const Class::map_t& augend_t, const Class::pair_t& );
		static void clear_all_result_variables( Class::pair_t& c ) { c.second.resultVariables().clear(); }

	    public:
		static const char * const __typeName;

	    private:
		LQX::SyntaxTreeNode* _visits;
		LQX::SyntaxTreeNode* _service_time;
		LQX::SyntaxTreeNode* _service_shape;
		std::map<Result::Type,double> _results;
		Result::map_t _result_vars;
	    };

	/* ------------------------------------------------------------ */
	/*                           Station                            */
	/* ------------------------------------------------------------ */

	public:
	    Station( Type type=Type::NOT_DEFINED, scheduling_type scheduling=SCHEDULE_PS, Distribution distribution=Distribution::EXPONENTIAL, LQX::SyntaxTreeNode * copies=nullptr ) :
		_type(type), _scheduling(scheduling), _distribution(distribution), _copies(copies), _reference(false) {}
	    ~Station();

	    Station& operator=( const Station& );
	    void clear();

	    std::pair<Station::Class::map_t::iterator,bool> insertClass( const std::string&, const Class& );
	    std::pair<Station::Class::map_t::iterator,bool> insertClass( const std::string&, LQX::SyntaxTreeNode * visits=nullptr, LQX::SyntaxTreeNode * service_time=nullptr );
	    void insertResultVariable( Result::Type, const std::string& );

	    virtual const char * getTypeName() const { return __typeName; }
	    Type type() const { return _type; }
	    void setType(Type type) { _type = type; }
	    scheduling_type scheduling() const { return _scheduling; }
	    void setScheduling( scheduling_type type ) { _scheduling = type; }
	    LQX::SyntaxTreeNode * copies() const { return _copies; }
	    void setCopies( LQX::SyntaxTreeNode * copies ) { _copies = copies; }
	    bool reference() const { return _reference; }
	    void setReference( bool reference ) { _reference = reference; }
	    LQX::SyntaxTreeNode * demand( const Class& ) const;
	    
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

	    virtual double mean_service() const { const double x = throughput(); return x > 0 ? utilization() / x : 0.; }
	    virtual double queue_length() const;
	    virtual double residence_time() const;
	    virtual double response_time() const;
	    virtual double throughput() const;
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
	    static double sum_response_time( double addend, const BCMP::Model::Station::Class::pair_t& augend );
	    static double sum_residence_time( double addend, const BCMP::Model::Station::Class::pair_t& augend );
	    static double sum_queue_length( double addend, const BCMP::Model::Station::Class::pair_t& augend );
	    static void clear_all_result_variables( BCMP::Model::Station::pair_t& m );

	    struct print {
		print( std::ostream& output ) : _output(output) {}
		void operator()( const Station::pair_t& m ) const;
	    private:
		std::ostream& _output;
	    };
	    

	public:
	    static const char * const __typeName;

	private:
	    Type _type;
	    scheduling_type _scheduling;
	    Distribution _distribution;
	    LQX::SyntaxTreeNode * _copies;
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

	    static LQX::SyntaxTreeNode * D( const Station& m, const Chain::pair_t& chain );

	    LQX::SyntaxTreeNode * D_max() const { return _D_max; }		/* Max Demand (adjusted for multiservers) */
	    LQX::SyntaxTreeNode * D_sum() const { return _D_sum; }		/* Sum of Demands */
	    LQX::SyntaxTreeNode * Z_sum() const { return _Z_sum; }
	    LQX::SyntaxTreeNode * N() const;
	    LQX::SyntaxTreeNode * N_star() const;
	    bool is_D_max( const Station& ) const;		/* Station with highest demand		*/
	
	private:
	    const std::string& chain() const { return _chain.first; }
	    const Station::map_t& stations() const { return _stations; }

	    void compute();

	    LQX::SyntaxTreeNode * Z() const;
	    static LQX::SyntaxTreeNode * demand( const Station& m, const std::string& chain );
	    
	    struct max_demand {
		max_demand( const std::string& chain ) : _class(chain) {}
		LQX::SyntaxTreeNode * operator()( LQX::SyntaxTreeNode *, const Station::pair_t& );
	    private:
		const std::string& _class;
	    };
	
	    struct sum_demand {
		sum_demand( const std::string& chain ) : _class(chain) {}
		LQX::SyntaxTreeNode * operator()( LQX::SyntaxTreeNode *, const Station::pair_t& );
	    private:
		const std::string& _class;
	    };
	
	    struct sum_think_time {
		sum_think_time( const std::string& chain ) : _class(chain) {}
		LQX::SyntaxTreeNode * operator()( LQX::SyntaxTreeNode *, const Station::pair_t& );
	    private:
		const std::string& _class;
	    };

	    const Chain::pair_t _chain;
	    const Station::map_t& _stations;
	    LQX::SyntaxTreeNode * _D_max;
	    LQX::SyntaxTreeNode * _D_sum;
	    LQX::SyntaxTreeNode * _Z_sum;
	};

	/* ------------------------------------------------------------ */
	/*                            Model                             */
	/* ------------------------------------------------------------ */

    public:
	Model() : _comment(), _chains(), _stations(), _environment(nullptr) {}
	virtual ~Model();

	bool empty() const { return _chains.size() == 0 || _stations.size() == 0; }
	bool insertComment( const std::string comment ) { _comment = comment; return true; }
	const std::string& comment() const { return _comment; }
	void setResultDescription( const std::string& result_description ) { _result_description = result_description; }
	const std::string& getResultDescription() const { return _result_description; }
	Chain::map_t& chains() { return _chains; }
	const Chain::map_t& chains() const { return _chains; }
	Station::map_t& stations() { return _stations; }
	const Station::map_t& stations() const { return _stations; }
	Station& stationAt( const std::string& name ) { return _stations.at(name); }
	const Station& stationAt( const std::string& name ) const { return _stations.at(name); }
	Chain& chainAt( const std::string& name ) { return _chains.at(name); }
	const Chain& chainAt( const std::string& name ) const { return _chains.at(name); }
	void setEnvironment( LQX::Environment * environment ) { _environment = environment; }
	LQX::Environment * environment() const { return _environment; }

	size_t n_chains(Chain::Type) const;
	size_t n_stations(Chain::Type) const;
	
	Station::map_t::const_iterator findStation( const Station* m ) const;

	std::pair<Chain::map_t::iterator,bool> insertClosedChain( const std::string&, LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * think_time=nullptr, LQX::SyntaxTreeNode * priority=nullptr );
	std::pair<Chain::map_t::iterator,bool> insertOpenChain( const std::string&, LQX::SyntaxTreeNode * );
	std::pair<Station::map_t::iterator,bool> insertStation( const std::string&, const Station& );
	std::pair<Station::map_t::iterator,bool> insertStation( const std::string& name, Station::Type type, scheduling_type scheduling=SCHEDULE_DELAY, Station::Distribution distribution=Station::Distribution::EXPONENTIAL, LQX::SyntaxTreeNode * copies=nullptr ) { return insertStation( name, Station( type, scheduling, distribution, copies ) ); }

	Station::Class::map_t computeCustomerDemand( const std::string& ) const;

	double response_time( const std::string& ) const;
	double throughput( const std::string& ) const;

	virtual std::ostream& print( std::ostream& output ) const;

	static bool isDefault( LQX::SyntaxTreeNode * var, double default_value=0.0 );

	static double getDoubleValue( LQX::SyntaxTreeNode * );
	static LQX::SyntaxTreeNode * constant( double );
	static LQX::SyntaxTreeNode * add( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );
	static LQX::SyntaxTreeNode * subtract( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );
	static LQX::SyntaxTreeNode * max( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );
	static LQX::SyntaxTreeNode * divide( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );
	static LQX::SyntaxTreeNode * multiply( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );
	static LQX::SyntaxTreeNode * reciprocal( LQX::SyntaxTreeNode * );

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
	std::string _comment;			/* Model Comment (input generally)	*/
	std::string _result_description;	/* Solver version etc.			*/
	Chain::map_t _chains;
	Station::map_t _stations;
	LQX::Environment * _environment;
    };
}
#endif /* */
