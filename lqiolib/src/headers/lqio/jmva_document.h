/* -*- C++ -*-
 *  $Id: expat_document.h 13717 2020-08-03 00:04:28Z greg $
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
#include "dom_pragma.h"
#include "bcmp_document.h"
#include "srvn_spex.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}

namespace BCMP {
    typedef std::map<const std::string,std::multimap<const std::string,const std::string> > result_t;

    class JMVA_Document {
	typedef std::string (JMVA_Document::*setIndependentVariable)( const std::string&, const std::string& );
	
	/* Safe union for stack object */
	class Object {
	public:
	    enum class type { VOID, MODEL, CLASS, OBJECT, STATION, DEMAND, PAIR };
	    typedef std::pair<const Model::Station *,const Model::Station::Class *> MK;
	    Object(const Object&);
	    Object() : _discriminator(type::VOID), u() {}
	    Object(Model * _m_) : _discriminator(type::MODEL), u(_m_) {}
	    Object(Model::Chain * _k_) : _discriminator(type::CLASS), u(_k_) {}
	    Object(Model::Object * _o_ ) : _discriminator(type::OBJECT), u(_o_) {}
	    Object(Model::Station *_s_) : _discriminator(type::STATION), u(_s_) {}
	    Object(Model::Station::Class * _d_) : _discriminator(type::DEMAND), u(_d_) {}
	    Object(const MK& _mk_) : _discriminator(type::PAIR), u(_mk_) {}
	    type getDiscriminator() const { return _discriminator; }
	    bool isModel() const { return _discriminator == type::MODEL; }
	    bool isClass() const { return _discriminator == type::CLASS; }
	    bool isStation() const { return _discriminator == type::STATION; }
	    bool isDemand() const { return _discriminator == type::DEMAND; }
	    bool isObject() const { return _discriminator == type::OBJECT; }
	    bool isMK() const { return _discriminator == type::PAIR; }
	    Model * getModel() const { assert( _discriminator == type::MODEL ); return u.m; }
	    Model::Chain * getClass() const { assert( _discriminator == type::CLASS ); return u.k; }
	    Model::Station * getStation() const { assert( _discriminator == type::STATION ); return u.s; }
	    Model::Station::Class * getDemand() const { assert( _discriminator == type::DEMAND ); return u.d; }
	    Model::Object * getObject() const { assert( _discriminator == type::OBJECT ); return u.o; }
	    MK& getMK() { assert( _discriminator == type::PAIR ); return u.mk; }
	    const MK& getMK() const { assert( _discriminator == type::PAIR ); return u.mk; }

	private:
	    const type _discriminator;	/* Once set, that's it. */
	    union u {
		u() : v(nullptr) {}
		u(Model * _m_) : m(_m_) {}
		u(Model::Chain * _k_) : k(_k_) {}
		u(Model::Object * _o_ ) : o(_o_) {}
		u(Model::Station *_s_) : s(_s_) {}
		u(Model::Station::Class * _d_) : d(_d_) {}
		u(const MK& _mk_) : mk(_mk_) {}
		void * v;
		Model * m;
		Model::Object * o;
		Model::Chain * k;
		Model::Station * s;
		Model::Station::Class * d;
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

	struct Generator
	{
	public:
	    Generator( const std::string& s ) : _begin(-1.), _end(-1.), _stride(0.) { convert(s); }
	    double begin() const { return _begin; }
	    double end() const { return _end; }
	    double stride() const { return _stride; }
	private:
	    void convert( const std::string& );

	    double _begin;
	    double _end;
	    double _stride;
	};

	struct register_variable {
	    register_variable( LQX::Program * lqx ) : _lqx(lqx) {}
	    void operator()( const std::pair<std::string, LQIO::DOM::SymbolExternalVariable*>& symbol ) const;
	private:
	    LQX::Program * _lqx;
	};

	struct plot_axis {
	    plot_axis() : var(), label(), max(0.0) {}
	    void set( const std::string& v, const std::string& l, double m ) { var = v; label = l; max = std::max( max, m ); }
	    bool empty() const { return var.empty(); }
	    std::string var;
	    std::string label;
	    double max;
	};

    public:
	JMVA_Document( const std::string& input_file_name );
	JMVA_Document( const std::string&, const BCMP::Model& );
	virtual ~JMVA_Document();
	static JMVA_Document * create( const std::string& input_file_name );
	static bool load( LQIO::DOM::Document&, const std::string& );		// Factory.
	bool parse();
	const BCMP::Model& model() const { return _model; }
	const std::string& getInputFileName() const { return _input_file_name; }
	const std::map<std::string,std::string>& getPragmaList() const { return _pragmas.getList(); }

	bool hasSPEX() const { return !_variables.empty() || _lqx_program != nullptr; }
	bool hasPragmas() const { return !_pragmas.empty(); }
	bool hasVariable( const std::string& name ) { return _variables.find(name) != _variables.end(); }
	expr_list * getLQXProgram() const { return _lqx_program; }
	expr_list * getGNUPlotProgram() { return &_gnuplot; }
	std::vector<std::string> getUndefinedExternalVariables() const;

	void registerExternalSymbolsWithProgram(LQX::Program* program);
	void mergePragmas(const std::map<std::string,std::string>&);

	std::ostream& print( std::ostream& ) const;
	void plot( Model::Result::Type, const std::string& );

    private:
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
	void startVisits( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startVisit( Object&, const XML_Char * element, const XML_Char ** attributes );
	void endVisit( Object&, const XML_Char * element );
	void startReferenceStation( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startAlgParams( Object&, const XML_Char * element, const XML_Char ** attributes );
	void startSolutions( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startAlgorithm( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startStationResults( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startClassResults( Object& object, const XML_Char * element, const XML_Char ** attributes );
	void startNOP( Object&, const XML_Char * element, const XML_Char ** attributes );

	const LQIO::DOM::ExternalVariable * getVariableAttribute( const XML_Char **attributes, const XML_Char * attribute, double default_value=-1.0 );
	const LQIO::DOM::ExternalVariable * getVariable( const XML_Char * attribute, const XML_Char * value );

	void createClosedChain( const XML_Char ** attributes );
	void createOpenChain( const XML_Char ** attributes );
	Model::Station * createStation( Model::Station::Type, const XML_Char ** attributes );
	void createWhatIf( const XML_Char ** attributes );
	void createResults( Object& object, const XML_Char ** attributes );
	void createResult( const Model::Station::map_t::const_iterator& m, std::map<const std::string,const Model::Result::Type>::const_iterator& r );
	void setResultVariables( const std::string& );
	LQX::SyntaxTreeNode * createObservation( const std::string& name, Model::Result::Type type, const Model::Station *, const Model::Station::Class * );
	LQX::SyntaxTreeNode * createObservation( const std::string& name, Model::Result::Type type, const std::string& clasx );

	std::string setArrivalRate( const std::string&, const std::string& );
	std::string setCustomers( const std::string&, const std::string& );
	std::string setDemand( const std::string&, const std::string& );
	std::string setMultiplicity( const std::string&, const std::string& );
	std::string setPopulationMix( const std::string& stationName, const std::string& className );
	
	void appendResultVariable( const std::string& );
	
	class What_If {
	private:
	    class has_customers {
	    public:
		has_customers( const std::string& var ) : _var(var) {}
		bool operator()( const Model::Chain::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_arrival_rate {
	    public:
		has_arrival_rate( const std::string& var ) : _var(var) {}
		bool operator()( const Model::Chain::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_copies {
	    public:
		has_copies( const std::string& var ) : _var(var) {}
		bool operator()( const Model::Station::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_var {
	    public:
		has_var( const std::string& var ) : _var(var) {}
		bool operator()( const Model::Station::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_service_time {
	    public:
		has_service_time( const std::string& var ) : _var(var) {}
		bool operator()( const Model::Station::pair_t& c2 ) const;
		bool operator()( const Model::Station::Class::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	    class has_visits {
	    public:
		has_visits( const std::string& var ) : _var(var) {}
		bool operator()( const Model::Station::pair_t& c2 ) const;
		bool operator()( const Model::Station::Class::pair_t& c2 ) const;
	    private:
		const std::string& _var;
	    };

	public:
	    What_If( std::ostream& output, const BCMP::Model& model ) : _output(output), _model(model) {}
	    void operator()( const std::pair<std::string,LQX::SyntaxTreeNode *>& ) const;
	    void operator()( const std::string& ) const;
	    const Model::Model::Station::map_t& stations() const { return _model.stations(); }
	    const Model::Chain::map_t& chains() const { return _model.chains(); }
	private:
	    std::ostream& _output;
	    const BCMP::Model _model;
	};

	bool convertToLQN( LQIO::DOM::Document& ) const;

	struct notSet {
	    notSet(std::vector<std::string>& list) : _list(list) {}
	    void operator()( const std::pair<std::string,LQIO::DOM::SymbolExternalVariable*>& var ) { if (!var.second->wasSet()) _list.push_back(var.first); }
	private:
	    std::vector<std::string>& _list;
	};

	std::ostream& plot_chain( std::ostream& plot, Model::Result::Type type );
	std::ostream& plot_class( std::ostream& plot, Model::Result::Type type, const std::string& );
	std::ostream& plot_station( std::ostream& plot, Model::Result::Type type, const std::string& );
	std::ostream& plot_population_mix( std::ostream& plot );

	/* -------------------------- Output -------------------------- */

    private:
	Model::Station::map_t& stations() { return _model.stations(); }	/* Not const */
	const Model::Station::map_t& stations() const { return _model.stations(); }
	Model::Chain::map_t& chains() { return _model.chains(); }
	const Model::Chain::map_t& chains() const { return _model.chains(); }

	struct printClass {
	    printClass( std::ostream& output ) : _output(output) {}
	    void operator()( const Model::Chain::pair_t& k ) const;
	private:
	    std::ostream& _output;
	};

	struct printStation {
	    printStation( std::ostream& output ) : _output(output) {}
	    void operator()( const Model::Station::pair_t& m ) const;
	private:
	    std::ostream& _output;
	};

	struct printReference {
	    printReference( std::ostream& output, const Model::Station::map_t& stations ) : _output(output), _stations(stations) {}
	    void operator()( const Model::Chain::pair_t& ) const;
	private:
	    std::ostream& _output;
	    const Model::Station::map_t& _stations;
	};

	struct printService {
	    printService( std::ostream& output ) : _output(output) {}
	    void operator()( const Model::Station::Class::pair_t& d ) const;
	private:
	    std::ostream& _output;
	};

	struct printVisits {
	    printVisits( std::ostream& output ) : _output(output) {}
	    void operator()( const Model::Station::Class::pair_t& d ) const;
	private:
	    std::ostream& _output;
	};

	struct printResultVariable {
	    printResultVariable( std::ostream& output ) : _output(output) {}
	    void operator()( const Model::Result::pair_t& r ) const;
	private:
	    std::ostream& _output;
	};

	static std::string fold( const std::string& s1, const Spex::var_name_and_expr& v2 );

    private:
	BCMP::Model _model;
	const std::string _input_file_name;
	XML_Parser _parser;
	std::string _text;
	std::stack<parse_stack_t> _stack;
	LQIO::DOM::Pragma _pragmas;

	/* SPEX */
	expr_list* _lqx_program;
	std::map<std::string,LQIO::DOM::SymbolExternalVariable*> _variables;	/* Spex vars */

	/* Maps for asssociating var (the string) to an object */
	std::map<const Model::Chain *,std::string> _think_time_vars;		/* chain, var 	*/
	std::map<const Model::Chain *,std::string> _population_vars;		/* chain, var	*/
	std::map<const Model::Chain *,std::string> _arrival_rate_vars;		/* chain, var	*/
	std::map<const Model::Station *,std::string> _multiplicity_vars;	/* station, var	*/
	std::map<const Model::Station::Class *,std::string> _service_time_vars;	/* class, var	*/
	std::map<const Model::Station::Class *,std::string> _visit_vars;	/* class, var	*/

	/* Plotting */
	expr_list _gnuplot;			/* GNUPlot program		*/
	bool _plot_population_mix;
	plot_axis _x1;
	plot_axis _x2;

	static const std::map<const std::string,JMVA_Document::setIndependentVariable> independent_var_table;
	
	static const std::set<const XML_Char *,attribute_table_t> algParams_table;
	static const std::set<const XML_Char *,attribute_table_t> class_results_table;
	static const std::set<const XML_Char *,attribute_table_t> compareAlgs_table;
	static const std::set<const XML_Char *,attribute_table_t> demand_table;
	static const std::set<const XML_Char *,attribute_table_t> measure_table;
	static const std::set<const XML_Char *,attribute_table_t> null_table;

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
	static const XML_Char * Xldstation;
	static const XML_Char * Xlistation;
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
