/* -*- C++ -*-
 *  $Id: jmva_document.h 16303 2023-01-09 01:52:04Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO_JMVA_DOCUMENT__
#define __LQIO_JMVA_DOCUMENT__

#include <config.h>
#include <expat.h>
#include <stack>
#include <set>
#include <cstring>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include "bcmp_document.h"
#include "qnio_document.h"

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
	typedef std::pair<const std::string,LQX::SyntaxTreeNode *> var_name_and_expr;

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
	    void operator()( const std::string& symbol ) const;
	private:
	    LQX::Program * _lqx;
	};

    public:
	JMVA_Document( const std::string& input_file_name );
	JMVA_Document( const std::string&, const BCMP::Model& );
	virtual ~JMVA_Document();

	virtual bool load();
	static bool load( LQIO::DOM::Document&, const std::string& );		// Factory.

    private:
	bool parse();
	void input_error( const std::string& );
	void input_error( const std::string&, const std::string& );

    public:
	bool hasVariable( const std::string& name ) const { return _input_variables.find(name) != _input_variables.end(); }
	std::string& getLQXProgramText() { return _lqx_program_text; }
	void setLQXProgramLineNumber( const unsigned n ) { _lqx_program_line_number = n; }
	const unsigned getLQXProgramLineNumber() const { return _lqx_program_line_number; }
	virtual std::vector<std::string> getUndefinedExternalVariables() const;
	const std::deque<Comprehension>& whatif_statements() const { return comprehensions(); }

	virtual void registerExternalSymbolsWithProgram(LQX::Program* program);

	virtual bool disableDefaultOutputWithLQX() const { return true; }

	std::ostream& print( std::ostream& ) const;
	std::ostream& exportModel( std::ostream& ) const;
	void plot( BCMP::Model::Result::Type, const std::string& );
	bool plotPopulationMix() const { return _plot_population_mix; }
	void setPlotPopulationMix( bool plot_population_mix ) { _plot_population_mix = plot_population_mix; }
	bool plotCustomers() const { return _plot_customers; }
	void setPlotCustomers( bool plot_customers ) { _plot_customers = plot_customers; }

    private:
	void setStrictJMVA( bool value ) { _strict_jmva = value; }
	bool strictJMVA() const { return  _strict_jmva; }
	bool checkAttributes( const XML_Char * element, const XML_Char ** attributes, const std::set<const XML_Char *,JMVA_Document::attribute_table_t>& table ) const;

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
	void endServiceTimeList( Object&, const XML_Char * element );	/* BUG_411 */
	void startVisits( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startVisit( Object&, const XML_Char * element, const XML_Char ** attributes );
	void endVisit( Object&, const XML_Char * element );
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

	void createClosedChain( const XML_Char ** attributes );
	void createOpenChain( const XML_Char ** attributes );
	BCMP::Model::Station * createStation( BCMP::Model::Station::Type, const XML_Char ** attributes );
	void createWhatIf( const XML_Char ** attributes );
	void createMeasure( Object& object, const XML_Char ** attributes );

	void setResultVariables( const std::string& );
	LQX::SyntaxTreeNode * createObservation( const std::string& name, BCMP::Model::Result::Type type, const BCMP::Model::Station *, const BCMP::Model::Station::Class * );
	LQX::SyntaxTreeNode * createObservation( const std::string& name, BCMP::Model::Result::Type type, const std::string& clasx );
	void setResultIndex( const std::string&, const std::string& );

	std::string setArrivalRate( const std::string&, const std::string& );
	std::string setCustomers( const std::string&, const std::string& );
	std::string setDemand( const std::string&, const std::string& );
	std::string setMultiplicity( const std::string&, const std::string& );
	std::string setPopulationMix( const std::string& stationName, const std::string& className );

	void appendResultVariable( const std::string&, LQX::SyntaxTreeNode * );

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
	    void operator()( const std::string& ) const;
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
	    std::vector<std::string> operator()( const std::vector<std::string>& arg1, const std::string& arg2 ) const;

	private:
	    void getVariables( const JMVA_Document& document );
	    std::set<std::string> _variables;
	};

	bool convertToLQN( LQIO::DOM::Document& ) const;

	std::ostream& printModel( std::ostream& ) const;
	std::ostream& printSPEX(  std::ostream& ) const;
	std::ostream& printResults( std::ostream& ) const;
	std::ostream& plot_chain( std::ostream& plot, BCMP::Model::Result::Type type );
	std::ostream& plot_class( std::ostream& plot, BCMP::Model::Result::Type type, const std::string& );
	std::ostream& plot_station( std::ostream& plot, BCMP::Model::Result::Type type, const std::string& );
	std::ostream& plot_population_mix_vs_throughput( std::ostream& plot );
	std::ostream& plot_population_mix_vs_utilization( std::ostream& plot );
	size_t get_gnuplot_index( const std::string& ) const;

	/* -------------------------- Output -------------------------- */

    private:
	BCMP::Model::Station::map_t& stations() { return model().stations(); }	/* Not const */
	const BCMP::Model::Station::map_t& stations() const { return model().stations(); }
	BCMP::Model::Chain::map_t& chains() { return model().chains(); }
	const BCMP::Model::Chain::map_t& chains() const { return model().chains(); }

	struct printClass {
	    printClass( std::ostream& output ) : _output(output) {}
	    void operator()( const BCMP::Model::Chain::pair_t& k ) const;
	private:
	    std::ostream& _output;
	};

	struct printStation {
	    printStation( std::ostream& output, const BCMP::Model& model, bool strict_jmva=true ) : _output(output), _model(model), _strict_jmva(strict_jmva) {}
	    void operator()( const BCMP::Model::Station::pair_t& m ) const;
	private:
	    const BCMP::Model::Chain::map_t& chains() const { return _model.chains(); }
	    static LQX::SyntaxTreeNode * add_customers( LQX::SyntaxTreeNode * addend, const BCMP::Model::Chain::pair_t& augend );
	    bool strictJMVA() const { return _strict_jmva; }
	private:
	    std::ostream& _output;
	    const BCMP::Model& _model;
	    const bool _strict_jmva;
	};

	struct printReference {
	    printReference( std::ostream& output, const BCMP::Model::Station::map_t& stations ) : _output(output), _stations(stations) {}
	    void operator()( const BCMP::Model::Chain::pair_t& ) const;
	private:
	    std::ostream& _output;
	    const BCMP::Model::Station::map_t& _stations;
	};

	struct printServiceTime {
	    printServiceTime( std::ostream& output ) : _output(output) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& d ) const;
	private:
	    std::ostream& _output;
	};

	struct printServiceTimeList {	/*+ BUG_411 */
	    printServiceTimeList( std::ostream& output, LQX::SyntaxTreeNode * copies, LQX::SyntaxTreeNode * customers ) : _output(output), _copies(copies), _customers(customers) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& d ) const;
	private:
	    std::ostream& _output;
	    LQX::SyntaxTreeNode * _copies;
	    LQX::SyntaxTreeNode * _customers;
	};

	struct printVisits {
	    printVisits( std::ostream& output ) : _output(output) {}
	    void operator()( const BCMP::Model::Station::Class::pair_t& d ) const;
	private:
	    std::ostream& _output;
	};

	struct printMeasure {
	    printMeasure( std::ostream& output ) : _output(output) {}
	    void operator()( const BCMP::Model::Result::pair_t& r ) const;
	private:
	    std::ostream& _output;
	};

	static std::string fold( const std::string& s1, const var_name_and_expr& v2 );

    private:
	bool _strict_jmva;								/* True if outputting strict JMVA. */
	XML_Parser _parser;
	std::string _text;
	std::stack<parse_stack_t> _stack;
	std::string _lqx_program_text;
	unsigned int _lqx_program_line_number;
	LQX::Program * _lqx;

	/* SPEX */
	std::vector<LQX::SyntaxTreeNode*> _program;
	std::set<std::string> _input_variables;						/* Spex vars -- may move to QNAP/QNIO */
	std::vector<LQX::SyntaxTreeNode*> _whatif_body;
	std::vector<std::string> _independent_variables;				/* x variables */
	std::vector<var_name_and_expr> _result_variables;				/* y variables */
	std::map<const std::string,const size_t> _result_index;				/* For plotting: maps result name to index */
	std::map<const std::string,const std::string> _station_index;			/* For CSV: Result name, station name */

	/* Maps for asssociating var (the string) to an object */
	std::map<const BCMP::Model::Chain *,std::string> _think_time_vars;		/* chain, var 	*/
	std::map<const BCMP::Model::Chain *,std::string> _population_vars;		/* chain, var	*/
	std::map<const BCMP::Model::Chain *,std::string> _arrival_rate_vars;		/* chain, var	*/
	std::map<const BCMP::Model::Station *,std::string> _multiplicity_vars;		/* station, var	*/
	std::map<const BCMP::Model::Station::Class *,std::string> _service_time_vars;	/* class, var	*/
	std::map<const BCMP::Model::Station::Class *,std::string> _visit_vars;		/* class, var	*/

	/* Plotting */
	std::vector<LQX::SyntaxTreeNode*> _gnuplot;					/* GNUPlot program		*/
	bool _plot_population_mix;
	bool _plot_customers;

	static const std::map<const std::string,JMVA_Document::setIndependentVariable> independent_var_table;

	static const std::set<const XML_Char *,attribute_table_t> algParams_table;
	static const std::set<const XML_Char *,attribute_table_t> compareAlgs_table;
	static const std::set<const XML_Char *,attribute_table_t> demand_table;
	static const std::set<const XML_Char *,attribute_table_t> measure_table;
	static const std::set<const XML_Char *,attribute_table_t> null_table;
	static const std::map<const BCMP::Model::Result::Type,const std::string> __y_label_table;
	static const std::map<const BCMP::Model::Result::Type,const std::string> __lqx_function_table;
	static const std::map<const std::string,const BCMP::Model::Result::Type> __result_name_table;

	static const XML_Char * XArrivalProcess;
	static const XML_Char * XClass;
	static const XML_Char * XReferenceStation;
	static const XML_Char * XalgParams;
	static const XML_Char * XalgType;
	static const XML_Char * Xclass;
	static const XML_Char * Xclasses;
	static const XML_Char * Xclosedclass;
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
}
#endif
