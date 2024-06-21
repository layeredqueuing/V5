/* -*- C++ -*-
 *  $Id: jmva_document.h 17251 2024-06-17 17:31:44Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO_JMVA_DOCUMENT__
#define __LQIO_JMVA_DOCUMENT__

#include <config.h>
#if HAVE_EXPAT_H
#include <expat.h>
#endif
#include <stack>
#include <set>
#include <cstring>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include "qnio_document.h"

// undef UTILIZATION_BOUNDS

namespace LQIO {
    namespace DOM {
	class Document;
    }
}
namespace LQX {
    class SyntaxTreeNode;
    class VariableExpression;
}

namespace QNIO {
    typedef std::map<const std::string,std::multimap<const std::string,const std::string> > result_t;

    class JMVA_Document : public Document {
	typedef std::string (JMVA_Document::*setIndependentVariable)( const std::string&, const std::string& );

	struct var_name_and_expr {
	    var_name_and_expr( const std::string& name, BCMP::Model::Result::Type type, LQX::SyntaxTreeNode * expr ) :
		_name(name), _type(type), _expr(expr) {}

	    const std::string& name() const { return _name; }
	    BCMP::Model::Result::Type type() const { return _type; }
	    LQX::SyntaxTreeNode * expression() const { return _expr; }
	private:
	    const std::string _name;
	    BCMP::Model::Result::Type _type;
	    LQX::SyntaxTreeNode * _expr;
	};

	/* Used for Population Mix solutions. */
	
	struct Population {
	    Population() : _name(), _population(0), _N() {}
	    const std::pair<double,double>& operator[]( size_t i ) const { return _N.at(i); }
	    bool empty() const { return _N.empty(); }
	    size_t size() const { return _N.size(); }
	    void setName( const std::string& name ) { _name = name; }
	    const std::string& name() const { return _name; }
	    void setPopulation( size_t population ) { _population = population; }
	    size_t population() const { return _population; }
	    void reserve( size_t size ) { _N.reserve( size ); }
	    void emplace_back( const std::pair<double,double>& item ) { _N.emplace_back( item ); }
	private:
	    std::string _name;
	    size_t _population;
	    std::vector<std::pair<double,double>> _N;
	};


	/* Safe union for stack object */
	class Object {
	public:
	    enum class type { VOID, MODEL, CLASS, OBJECT, STATION, DEMAND, PAIR };
	    typedef std::pair<const BCMP::Model::Station *,const BCMP::Model::Station::Class *> MK;
	    Object(const Object&);
	    Object() : _discriminator(type::VOID), u() {}
	    Object(BCMP::Model * _m_) : _discriminator(type::MODEL), u(_m_) {}
	    Object(BCMP::Model::Chain * _k_) : _discriminator(type::CLASS), u(_k_) {}
	    Object(BCMP::Model::Object * _o_ ) : _discriminator(type::OBJECT), u(_o_) {}
	    Object(BCMP::Model::Station *_s_) : _discriminator(type::STATION), u(_s_) {}
	    Object(BCMP::Model::Station::Class * _d_) : _discriminator(type::DEMAND), u(_d_) {}
	    Object(const MK& _mk_) : _discriminator(type::PAIR), u(_mk_) {}
	    type getDiscriminator() const { return _discriminator; }
	    bool isModel() const { return _discriminator == type::MODEL; }
	    bool isClass() const { return _discriminator == type::CLASS; }
	    bool isStation() const { return _discriminator == type::STATION; }
	    bool isDemand() const { return _discriminator == type::DEMAND; }
	    bool isObject() const { return _discriminator == type::OBJECT; }
	    bool isMK() const { return _discriminator == type::PAIR; }
	    BCMP::Model * getModel() const { assert( _discriminator == type::MODEL ); return u.m; }
	    BCMP::Model::Chain * getClass() const { assert( _discriminator == type::CLASS ); return u.k; }
	    BCMP::Model::Station * getStation() const { assert( _discriminator == type::STATION ); return u.s; }
	    BCMP::Model::Station::Class * getDemand() const { assert( _discriminator == type::DEMAND ); return u.d; }
	    BCMP::Model::Object * getObject() const { assert( _discriminator == type::OBJECT ); return u.o; }
	    MK& getMK() { assert( _discriminator == type::PAIR ); return u.mk; }
	    const MK& getMK() const { assert( _discriminator == type::PAIR ); return u.mk; }

	private:
	    const type _discriminator;	/* Once set, that's it. */
	    union u {
		u() : v(nullptr) {}
		u(BCMP::Model * _m_) : m(_m_) {}
		u(BCMP::Model::Chain * _k_) : k(_k_) {}
		u(BCMP::Model::Object * _o_ ) : o(_o_) {}
		u(BCMP::Model::Station *_s_) : s(_s_) {}
		u(BCMP::Model::Station::Class * _d_) : d(_d_) {}
		u(const MK& _mk_) : mk(_mk_) {}
		void * v;
		BCMP::Model * m;
		BCMP::Model::Object * o;
		BCMP::Model::Chain * k;
		BCMP::Model::Station * s;
		BCMP::Model::Station::Class * d;
		MK mk;
	    } u;
	};

	typedef void (JMVA_Document::*start_fptr)( Object&, const XML_Char *, const XML_Char ** );
	typedef void (JMVA_Document::*end_fptr)( Object&, const XML_Char * );

	struct parse_stack_t
	{
	    parse_stack_t(const XML_Char * e, start_fptr sh ) : element(e), start(sh), end(nullptr), object() {}
	    parse_stack_t(const XML_Char * e, start_fptr sh, const Object& o ) : element(e), start(sh), end(nullptr), object(o) {}
	    parse_stack_t(const XML_Char * e, start_fptr sh, end_fptr eh, const Object& o ) : element(e), start(sh), end(eh), object(o) {}
	    bool operator==( const XML_Char * ) const;

	    const std::basic_string<XML_Char> element;
	    start_fptr start;
	    end_fptr end;
	    Object object;
	};

	struct attribute_table_t
	{
	    bool operator()( const XML_Char * s1, const XML_Char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
	};

	struct register_variable {
	    register_variable( LQX::Program * lqx ) : _lqx(lqx) {}
	    void operator()( const std::pair<const std::string,LQX::SyntaxTreeNode*>& symbol ) const;
	private:
	    LQX::Program * _lqx;
	};

	struct what_if_function {
	    Document::Comprehension::Type key;
	    JMVA_Document::setIndependentVariable single_class;
	    JMVA_Document::setIndependentVariable all_class;
	};


    public:
	JMVA_Document( const std::string& input_file_name );
	JMVA_Document( const BCMP::Model& );
	virtual ~JMVA_Document();

	virtual bool load();
	static bool load( LQIO::DOM::Document&, const std::string& );		// Factory.
	virtual InputFormat getInputFormat() const { return InputFormat::JMVA; }

    private:
	bool parse();
	void input_error( const std::string& );
	void input_error( const std::string&, const std::string& );

    public:
	std::string& getLQXProgramText() { return _lqx_program_text; }
	void setLQXProgramLineNumber( const unsigned n ) { _lqx_program_line_number = n; }
	const unsigned getLQXProgramLineNumber() const { return _lqx_program_line_number; }
	virtual std::vector<std::string> getUndefinedExternalVariables() const;
	virtual unsigned getSymbolExternalVariableCount() const;

	virtual void registerExternalSymbolsWithProgram(LQX::Program* program);

	virtual bool disableDefaultOutputWithLQX() const { return true; }

	void defineDefaultResultVariables();
	virtual void saveResults( size_t, const std::string&, size_t, const std::string&, const std::string&, const std::map<BCMP::Model::Result::Type,double>& );

	std::ostream& print( std::ostream& ) const;
	std::ostream& exportModel( std::ostream& ) const;
	void plot( BCMP::Model::Result::Type, const std::string&, bool, LQIO::GnuPlot::Format format=LQIO::GnuPlot::Format::TERMINAL );
	bool plotPopulationMix() const { return !_N1.empty() && !_N2.empty(); }

    private:
	void setStrictJMVA( bool value ) { _strict_jmva = value; }
	bool strictJMVA() const { return _strict_jmva; }
	bool checkAttributes( const XML_Char * element, const XML_Char ** attributes, const std::set<const XML_Char *,JMVA_Document::attribute_table_t>& table ) const;
	const std::deque<Comprehension>& whatif_statements() const { return comprehensions(); }

	static void start( void *data, const XML_Char *el, const XML_Char **attr );
	static void end( void *data, const XML_Char *el );
	static void start_cdata( void *data );
	static void end_cdata( void *data );
	static void handle_text( void * data, const XML_Char * text, int length );
	static void handle_comment( void * data, const XML_Char * text );
	static int handle_encoding( void * data, const XML_Char *name, XML_Encoding *info );

	void startDocument( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startModel( Object&, const XML_Char * element, const XML_Char ** attributes );
	void endModel( Object&, const XML_Char * element );
	void startDescription( Object&, const XML_Char * element, const XML_Char ** attributes );
	void endDescription( Object&, const XML_Char * element );
	void startParameters( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startClasses( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startClass( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startStations( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startStation( Object& station, const XML_Char * element, const XML_Char ** attributes );
	void startServiceTimes( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startServiceTime( Object&, const XML_Char * element, const XML_Char ** attributes );
	void endServiceTime( Object&, const XML_Char * element );
	void startServiceTimeList( Object& station, const XML_Char * element, const XML_Char ** attributes );	/* BUG_411 */
	void endServiceTimeList( Object&, const XML_Char * element );						/* BUG_411 */
	void startVisits( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startVisit( Object&, const XML_Char * element, const XML_Char ** attributes );
	void endVisit( Object&, const XML_Char * element );
	void startCoeffsOfVariation( Object&, const XML_Char * element, const XML_Char ** attributes );		/* BUG_467 */
	void startCoeffOfVariation( Object&, const XML_Char * element, const XML_Char ** attributes );		/* BUG_467 */
	void endCoeffOfVariation( Object&, const XML_Char * element );
	void startReferenceStation( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startAlgParams( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startSolutions( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startAlgorithm( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startStationResults( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startClassResults( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startLQX( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startNOP( Object&, const XML_Char * element, const XML_Char ** attributes );

	LQX::SyntaxTreeNode * getVariableAttribute( const XML_Char **attributes, const XML_Char * attribute, double default_value=-1.0 );
	LQX::SyntaxTreeNode * getVariable( const XML_Char * attribute, const XML_Char * value );
	double getDoubleValue( LQX::SyntaxTreeNode * ) const;

	void createClosedChain( const XML_Char ** attributes );
	void createOpenChain( const XML_Char ** attributes );
	BCMP::Model::Station * createStation( BCMP::Model::Station::Type, const XML_Char ** attributes );
	void createWhatIf( const XML_Char ** attributes );
	void createMeasure( Object& object, const XML_Char ** attributes );

	LQX::SyntaxTreeNode * createObservation( const std::string& name, BCMP::Model::Result::Type type, const BCMP::Model::Station *, const BCMP::Model::Station::Class * );
	LQX::SyntaxTreeNode * createObservation( const std::string& name, BCMP::Model::Result::Type type, const std::string& clasx );
	void setResultIndex( const std::string&, const std::string& );

	std::string setArrivalRate( const std::string&, const std::string& );
	std::string setCustomers( const std::string&, const std::string& );
	std::string setDemand( const std::string&, const std::string& );
	std::string setMultiplicity( const std::string&, const std::string& );
	std::string setPopulationMix( const std::string&, const std::string& );
	std::string setAllCustomers( const std::string&, const std::string& );
	std::string setAllDemands( const std::string&, const std::string& );

	void setPopulationMixN1N2( const std::string& className, const Comprehension& population );
	void setPopulationMixK( bool, const BCMP::Model::Chain::map_t::iterator& k, Population& N );
	void appendResultVariable( const std::string&, BCMP::Model::Result::Type, LQX::SyntaxTreeNode * );

	/* LQX */
	virtual LQX::Program * getLQXProgram();
	LQX::SyntaxTreeNode * foreach_loop( std::deque<Comprehension>::const_iterator, std::deque<Comprehension>::const_iterator ) const;
	std::vector<LQX::SyntaxTreeNode *>* loop_body() const;
	std::vector<LQX::SyntaxTreeNode *>* solve_failure() const;
	std::vector<LQX::SyntaxTreeNode *>* solve_success() const;
	LQX::SyntaxTreeNode * print_csv_header() const;

	class What_If {
	private:
	    class has_customers {
	    public:
		has_customers( const std::string& var ) : _var(var) {}
		bool operator()( const BCMP::Model::Chain::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_arrival_rate {
	    public:
		has_arrival_rate( const std::string& var ) : _var(var) {}
		bool operator()( const BCMP::Model::Chain::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_copies {
	    public:
		has_copies( const std::string& var ) : _var(var) {}
		bool operator()( const BCMP::Model::Station::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_var {
	    public:
		has_var( const std::string& var ) : _var(var) {}
		bool operator()( const BCMP::Model::Station::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_service_time {
	    public:
		has_service_time( const std::string& var ) : _var(var) {}
		bool operator()( const BCMP::Model::Station::pair_t& c2 ) const;
		bool operator()( const BCMP::Model::Station::Class::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_visits {
	    public:
		has_visits( const std::string& var ) : _var(var) {}
		bool operator()( const BCMP::Model::Station::pair_t& c2 ) const;
		bool operator()( const BCMP::Model::Station::Class::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	public:
	    What_If( std::ostream& output, const JMVA_Document& document ) : _output(output), _document(document) {}
	    void operator()( const std::pair<const std::string,LQX::SyntaxTreeNode*>& ) const;	// For input variables. (obsolete)
	    const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	    const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }
	    const std::deque<Comprehension>& whatif_statements() const { return _document.comprehensions(); }
	private:
	    const BCMP::Model& model() const { return _document.model(); }
	    static bool match( const LQX::SyntaxTreeNode * var, const std::string& );
	private:
	    std::ostream& _output;
	    const JMVA_Document& _document;
	};

	class create_result {
	public:
	    create_result( JMVA_Document& self ) : _self(self) {}
	    void operator()( const std::pair<const std::string,const BCMP::Model::Station>& m ) const;
	    void operator()( const std::pair<const std::string,const BCMP::Model::Chain>& k ) const;
	    const BCMP::Model::Station::map_t& stations() const { return _self.model().stations(); }
	    void createObservation( const std::string&, const std::string& name, BCMP::Model::Result::Type type, const BCMP::Model::Station * m, const BCMP::Model::Station::Class * k=nullptr ) const;

	private:
	    JMVA_Document& _self;
	};

	class csv_heading {
	public:
	    csv_heading( std::vector<LQX::SyntaxTreeNode *>* arguments, const BCMP::Model::Chain::map_t& chains ) : _arguments(arguments), _chains(chains) {}
	    void operator()( const std::pair<const std::string,const BCMP::Model::Result::Type>& );
	    const BCMP::Model::Chain::map_t& chains() const { return _chains; }

	private:
	    std::vector<LQX::SyntaxTreeNode *>* _arguments;
	    const BCMP::Model::Chain::map_t& _chains;
	};

	struct notSet {
	    notSet( const JMVA_Document& document ) : _variables() { getVariables(document); }
	    std::vector<std::string> operator()( const std::vector<std::string>& arg1, const std::pair<const std::string,LQX::SyntaxTreeNode*>& arg2 ) const;

	private:
	    void getVariables( const JMVA_Document& document );
	    std::set<std::string> _variables;
	};

	std::ostream& printModel( std::ostream& ) const;
	std::ostream& printSPEX(  std::ostream& ) const;
	std::ostream& printResults( std::ostream& ) const;
	std::ostream& plot_chain( std::ostream& plot, BCMP::Model::Result::Type type );
	std::ostream& plot_class( std::ostream& plot, BCMP::Model::Result::Type type, const std::string& );
	std::ostream& plot_station( std::ostream& plot, BCMP::Model::Result::Type type, const std::string& );
	std::ostream& plot_throughput_vs_population_mix( std::ostream& plot );
	std::ostream& plot_utilization_vs_population_mix( std::ostream& plot );
	std::ostream& plot_bounds( std::ostream& plot, BCMP::Model::Result::Type type, const std::string& );
	std::ostream& plot_one_class_bounds( std::ostream& plot, BCMP::Model::Result::Type type, const std::string& );
	std::ostream& plot_two_class_bounds( std::ostream& plot, BCMP::Model::Result::Type type, const std::string&, const std::string& );
	size_t get_gnuplot_index( const std::string& ) const;
	void compute_itercepts() const;

#if UTILIZATION_BOUNDS
	class Intercepts {
	public:
	    struct point {
		point( double x, double y ) : _x(x), _y(y) {}
		double x() const { return _x; }
		double y() const { return _y; }
		std::ostream& print( std::ostream& ) const;
		bool operator<( const point& right ) const { return x() < right.x() || ( x() == right.x() && y() < right.y() ); }
		point& min( const point& arg ) { _x = std::min( _x, arg.x() ); _y = std::min( _y, arg.y() ); return *this; }
	    private:
		double _x;
		double _y;
	    };
   
	public:
	    Intercepts( const JMVA_Document& self, const std::string& chain_1, const std::string& chain_2 );

	    Intercepts& compute();
	    std::set<point>::const_iterator begin() { return _intercepts.begin(); }
	    std::set<point>::const_iterator end() { return _intercepts.end(); }
	    const point& bound() const { return _bound; }

	private:
	    const BCMP::Model& model() const { return _self.model(); }
	    const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }
	    const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	    double getDoubleValue( LQX::SyntaxTreeNode * value ) const { return _self.getDoubleValue( value ); }
	    point compute( const point&, const point&, const point&, const point& ) const;

	private:
	    const JMVA_Document& _self;
	    const std::string& _chain_1;
	    const std::string& _chain_2;
	    point _bound;
	    std::set<point> _intercepts;
	};

	friend std::ostream& operator<<( std::ostream& output, const JMVA_Document::Intercepts::point& self );
#endif

	/* -------------------------- Output -------------------------- */

    private:
	BCMP::Model::Station::map_t& stations() { return model().stations(); }	/* Not const */
	const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	BCMP::Model::Chain::map_t& chains() { return model().chains(); }
	const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }

	class printCommon {
	public:
	    printCommon( std::ostream& output, const BCMP::Model& model, bool strict_jmva ) : _output(output), _model(model), _strict_jmva(strict_jmva) {}
	protected:
	    const BCMP::Model::Chain::map_t& chains() { return _model.chains(); }
	    bool strictJMVA() const { return _strict_jmva; }
	    double getDoubleValue( LQX::SyntaxTreeNode * value ) const;
	    std::ostream& print_attribute( std::ostream&, const std::string& a, LQX::SyntaxTreeNode* v ) const;
	    std::ostream& print_comment( std::ostream&, LQX::SyntaxTreeNode *v ) const;
	protected:
	    std::ostream& _output;
	    const BCMP::Model& _model;
	    const bool _strict_jmva;
	};

	class printClass : public printCommon {
	public:
	    printClass( std::ostream& output, const BCMP::Model& model, bool strict_jmva=true ) : printCommon( output, model, strict_jmva) {}
	    void operator()( const BCMP::Model::Chain::pair_t& k ) const;
	};

	class printStation  : public printCommon {
	public:
	    printStation( std::ostream& output, const BCMP::Model& model, bool strict_jmva=true ) : printCommon( output, model, strict_jmva) {}
	    void operator()( const BCMP::Model::Station::pair_t& m ) const;
	};

	struct printReference {
	    printReference( std::ostream& output, const BCMP::Model::Station::map_t& stations ) : _output(output), _stations(stations) {}
	    void operator()( const BCMP::Model::Chain::pair_t& ) const;
	private:
	    std::ostream& _output;
	    const BCMP::Model::Station::map_t& _stations;
	};

	class printServiceTime : private printCommon {
	public:
	    printServiceTime( std::ostream& output, const BCMP::Model& model, bool strict_jmva=true  ) : printCommon( output, model, strict_jmva) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& d ) const;
	};

	class printServiceTimeList : private printCommon {	/* BUG_411 */
	public:
	    printServiceTimeList( std::ostream& output, const BCMP::Model& model, LQX::SyntaxTreeNode * customers );
	    void operator()( const BCMP::Model::Station::Class::pair_t& d ) const;
	private:
	    static LQX::SyntaxTreeNode * add_customers( LQX::SyntaxTreeNode * addend, const BCMP::Model::Chain::pair_t& augend ) { return BCMP::Model::add( addend, augend.second.customers() ); }

	    LQX::SyntaxTreeNode * _copies;
	    LQX::SyntaxTreeNode * _customers;
	};

	class printCoeffOfVariation : private printCommon {	/* BUG_467 */
	public:
	    printCoeffOfVariation( std::ostream& output, const BCMP::Model& model, bool strict_jmva=true  ) : printCommon( output, model, strict_jmva) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& d ) const;
	};

	struct printVisits : private printCommon {
	    printVisits( std::ostream& output, const BCMP::Model& model, bool strict_jmva=true  ) : printCommon( output, model, strict_jmva) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& d ) const;
	};

	struct printMeasure {
	    printMeasure( std::ostream& output ) : _output(output) {}
	    void operator()( const BCMP::Model::Result::pair_t& r ) const;
	private:
	    std::ostream& _output;
	};

	static std::string fold( const std::string& s1, const var_name_and_expr& v2 );

    private:
	bool _strict_jmva;							/* True if outputting strict JMVA. */
	XML_Parser _parser;
	std::string _text;
	std::stack<parse_stack_t> _stack;
	std::string _lqx_program_text;
	unsigned int _lqx_program_line_number;
	LQX::Program * _lqx;

	/* LQX */
	std::vector<LQX::SyntaxTreeNode*> _program;
	std::vector<LQX::SyntaxTreeNode*> _whatif_body;
	std::vector<std::string> _independent_variables;			/* x variables */
	std::vector<var_name_and_expr> _result_variables;			/* y variables */
	std::map<const std::string,const size_t> _result_index;			/* For plotting: maps result name to index */
	std::map<const std::string,const std::string> _station_index;		/* For CSV: Result name, station name */

	/* Maps for asssociating var (the string) to an object */
	std::map<const BCMP::Model::Chain *,std::string> _think_time_vars;		/* chain, var 	*/
	std::map<const BCMP::Model::Chain *,std::string> _population_vars;		/* chain, var	*/
	std::map<const BCMP::Model::Chain *,std::string> _arrival_rate_vars;		/* chain, var	*/
	std::map<const BCMP::Model::Station *,std::string> _multiplicity_vars;		/* station, var	*/
	std::map<const BCMP::Model::Station::Class *,std::string> _service_time_vars;	/* class, var	*/
	std::map<const BCMP::Model::Station::Class *,std::string> _visit_vars;		/* class, var	*/

	/* Results */
	std::map<size_t,std::map<const std::string,std::map<const std::string,std::map<BCMP::Model::Result::Type,double>>>> _results;	/* iteration,station,class,results */
	std::map<size_t,std::pair<const std::string,size_t>> _mva_info;

	/* Plotting */
	BCMP::Model::Result::Type _plot_type;
	bool _no_bounds;
	std::vector<LQX::SyntaxTreeNode*> _gnuplot;				/* GNUPlot program		*/
	Population _N1;
	Population _N2;
	size_t _n_labels;
	LQX::SyntaxTreeNode * _x_max;
	LQX::SyntaxTreeNode * _y_max;

	static const std::map<const std::string,JMVA_Document::what_if_function> independent_var_table;

	static const std::set<const XML_Char *,attribute_table_t> algParams_table;
	static const std::set<const XML_Char *,attribute_table_t> compareAlgs_table;
	static const std::set<const XML_Char *,attribute_table_t> demand_table;
	static const std::set<const XML_Char *,attribute_table_t> measure_table;
	static const std::set<const XML_Char *,attribute_table_t> null_table;
	static const std::map<const BCMP::Model::Result::Type,const std::string> __y_label_table;
	static const std::map<const BCMP::Model::Result::Type,const std::string> __lqx_function_table;
	static const std::map<const std::string,const BCMP::Model::Result::Type> __result_name_table;

	static const std::string __Beta;

	static const XML_Char * XArrivalProcess;
	static const XML_Char * XClass;
	static const XML_Char * XReferenceStation;
	static const XML_Char * XalgParams;
	static const XML_Char * XalgType;
	static const XML_Char * Xclass;
	static const XML_Char * Xclasses;
	static const XML_Char * Xclosedclass;
	static const XML_Char * XcoeffsOfVariation;    // Bug 467
	static const XML_Char * XcoeffOfVariation;     // Bug 467
	static const XML_Char * XcompareAlgs;
	static const XML_Char * Xcustomerclass;
	static const XML_Char * Xdelaystation;
	static const XML_Char * Xdescription;
	static const XML_Char * Xjaba;
	static const XML_Char * Xldstation;
	static const XML_Char * Xlistation;
	static const XML_Char * XLQX;
	static const XML_Char * XmaxSamples;
	static const XML_Char * Xmodel;
	static const XML_Char * Xmultiplicity;
	static const XML_Char * Xname;
	static const XML_Char * Xnumber;
	static const XML_Char * Xopenclass;
	static const XML_Char * Xparameters;
	static const XML_Char * Xparam;
	static const XML_Char * Xpopulation;
	static const XML_Char * Xpragma;
	static const XML_Char * Xpriority;
	static const XML_Char * Xrate;
	static const XML_Char * XrefStation;
	static const XML_Char * Xservers;
	static const XML_Char * Xservicetime;
	static const XML_Char * Xservicetimes;
	static const XML_Char * Xstation;
	static const XML_Char * Xstations;
	static const XML_Char * Xthinktime;
	static const XML_Char * Xtolerance;
	static const XML_Char * Xvalue;
	static const XML_Char * Xvisit;
	static const XML_Char * Xvisits;
	static const XML_Char * XwhatIf;
	static const XML_Char * Xxml_debug;

	static const XML_Char * Xalgorithm;
	static const XML_Char * Xclassresults;
	static const XML_Char * XmeanValue;
	static const XML_Char * Xmeasure;
	static const XML_Char * XmeasureType;
	static const XML_Char * Xsolutions;
	static const XML_Char * Xstationresults;
	static const XML_Char * Xsuccessful;

	static const XML_Char * XclassName;
	static const XML_Char * XstationName;
	static const XML_Char * Xtype;
	static const XML_Char * Xvalues;

	static const XML_Char * XalgCount;
	static const XML_Char * Xfalse;
	static const XML_Char * Xiteration;
	static const XML_Char * XiterationValue;
	static const XML_Char * Xiterations;
	static const XML_Char * Xnormconst;
	static const XML_Char * Xok;
	static const XML_Char * XsolutionMethod;
	static const XML_Char * Xtrue;

	static const XML_Char * XArrival_Rates;
	static const XML_Char * XCustomer_Numbers;
	static const XML_Char * XNumberOfCustomers;
	static const XML_Char * XNumber_of_Customers;
	static const XML_Char * XNumber_of_Servers;
	static const XML_Char * XPopulation_Mix;
	static const XML_Char * XResidenceTime;
	static const XML_Char * XResponse_Time;
	static const XML_Char * XResidence_Time;
	static const XML_Char * XResultVariables;
	static const XML_Char * XService_Demands;
	static const XML_Char * XThroughput;
	static const XML_Char * XUtilization;
    };

    inline std::ostream& operator<<( std::ostream& output, const JMVA_Document& doc ) { return doc.print(output); }
#if UTILIZATION_BOUNDS
    inline std::ostream& operator<<( std::ostream& output, const JMVA_Document::Intercepts::point& self ) { return self.print( output ); }
#endif
}
#endif
