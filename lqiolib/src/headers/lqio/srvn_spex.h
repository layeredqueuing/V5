/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2012.								*/
/************************************************************************/

/*
 * $Id: srvn_spex.h 15360 2022-01-05 12:01:00Z greg $
 */

#ifndef __LQIO_SRVN_SPEX_H__
#define __LQIO_SRVN_SPEX_H__

#include <stdbool.h>
#if defined(__cplusplus)
#include <vector>
#include <set>
#include <map>
#include <string>
#include "common_io.h"

namespace LQX {
    class SyntaxTreeNode;
    class VariableExpression;
    class MethodInvocationExpression;
}
namespace LQIO {
    namespace DOM {
	class ExternalVariable;
	class SymbolExternalVariable;
	class Document;
	class Processor;
	class Group;
	class Task;
	class Activity;
	class DocumentObject;
	class JSON_Document;
	class Expat_Document;
	class BCMP_to_LQN;
    }
}
namespace BCMP {
    class JMVA_Document;
}

extern "C" {
#endif
    void spex_set_program( void * param, void * result, void * convergence );
    void spex_set_parameter_list( void * );
    void spex_set_result_list( void * );
    void spex_set_convergence_list( void * );
    void spex_set_variable( void * );
    void spex_set_default_model_parameters();

    void * spex_activity_call_observation( const void * task, const void * src, const int key, const void * dst, const int conf, const char * var, const char * var2  );
    void * spex_activity_observation( const void * obj, const void * obj2, const int key, const int conf, const char * var, const char * var2  );
    void * spex_array_assignment( const char * var, void * list, const bool );
    void * spex_array_comprehension( const char * var, double begin, double stride, double end );
    void * spex_assignment_statement( const char * var, void * expr, const bool );
    void * spex_call_observation( const void * src, const int key, const int phase, const void * dst, const int conf, const char * var, const char * var2  );
    void * spex_convergence_assignment_statement( const char *var, void * arg );
    void * spex_document_observation( const int key, const char * var );
    void * spex_entry_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );
    void * spex_forall( const char * arg1, const char * arg2, void * arg3 );
    void * spex_fwd_observation( const void * src, const int key, const int phase, const void * dst, const int conf, const char * var, const char * var2  );
    void * spex_group_observation( const void * obj, const int key, const int conf, const char * var, const char * var2 );
    void * spex_inline_expression( void * arg );
    void * spex_processor_observation( const void * obj, const int key, const int, const char * var, const char * var2 );
    void * spex_result_assignment_statement( const char *var, void * arg );
    void * spex_result_function( const char *, void * arg );
    void * spex_task_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );

    void * spex_list( void * list, void * stmt );
    void * spex_add( void * arg1, void * arg2 );
    void * spex_and( void * arg1, void * arg3 ); 
    void * spex_divide( void * arg1, void * arg2 );
    void * spex_equals( void * arg1, void * arg3 ); 
    void * spex_greater_than( void * arg1, void * arg3 ); 
    void * spex_greater_than_or_equals( void * arg1, void * arg3 ); 
    void * spex_array_reference( void * arg1, void *arg3 );
    void * spex_invoke_function( const char *s, void * arg );
    void * spex_less_than( void * arg1, void * arg3 ); 
    void * spex_less_than_or_equals( void * arg1, void * arg3 ); 
    void * spex_modulus( void * arg1, void * arg2 );
    void * spex_multiply( void * arg1, void * arg2 );
    void * spex_not( void * arg2 ); 
    void * spex_not_equals( void * arg1, void * arg3 ); 
    void * spex_or( void * arg1, void * arg3 ); 
    void * spex_power( void * arg1, void * arg2 );
    void * spex_subtract( void * arg1, void * arg2 );
    void * spex_ternary( void * arg1, void * arg2, void * arg3 );
    void * spex_get_symbol( const char * s );
    void * spex_get_real( double arg );
    void * spex_get_string( const char * s );

#if defined(__cplusplus)
}

typedef std::vector<LQX::SyntaxTreeNode*> expr_list;

namespace LQIO {
    typedef LQIO::DOM::Document& (LQIO::DOM::Document::*set_extvar_f)( const LQIO::DOM::ExternalVariable* );
    typedef void (*setSpexFunc)( const LQIO::DOM::ExternalVariable* );

    class Spex {
	friend class BCMP::JMVA_Document;
	friend class DOM::JSON_Document;
	friend class DOM::Expat_Document;
	friend class DOM::BCMP_to_LQN;
	friend void ::spex_set_program( void * param_arg, void * result_arg, void * convergence_arg );
	friend void ::spex_set_parameter_list( void * );
	friend void ::spex_set_result_list( void * );
	friend void ::spex_set_convergence_list( void * );
	friend void ::spex_set_variable( void * variable );
	friend void * ::spex_activity_call_observation( const void * task, const void * src, const int key, const void * dst, const int conf, const char * var, const char * var2  );
	friend void * ::spex_activity_observation( const void * obj1, const void * obj2, const int key, const int conf, const char * var, const char * var2  );
	friend void * ::spex_array_assignment( const char * name, void * list, const bool constant_expression );
	friend void * ::spex_array_comprehension( const char * name, double begin, double end, double stride );
	friend void * ::spex_array_reference( void * arg1, void * arg2 );
	friend void * ::spex_assignment_statement( const char * name, void * expr, const bool constant_expression );
	friend void * ::spex_call_observation( const void * src, const int key, const int phase, const void * dst, const int conf, const char * var, const char * var2  );
	friend void * ::spex_convergence_assignment_statement( const char * name, void * expr );
	friend void * ::spex_document_observation( const int key, const char * var );
	friend void * ::spex_entry_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );
	friend void * ::spex_forall( const char * iter_name, const char * name, void * expr );
	friend void * ::spex_fwd_observation( const void * src, const int key, const int phase, const void * dst, const int conf, const char * var, const char * var2 );
	friend void * ::spex_group_observation( const void * obj, const int key, const int conf, const char * var, const char * var2 );	    
	friend void * ::spex_inline_expression( void * arg );
	friend void * ::spex_processor_observation( const void * obj, const int key, const int, const char * var, const char * var2 );
	friend void * ::spex_result_assignment_statement( const char * name, void * expr );
	friend void * ::spex_result_function( const char * s, void * args );
	friend void * ::spex_task_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );

	
    public:
	typedef std::pair<std::string,LQX::SyntaxTreeNode *> var_name_and_expr;

    public:
	class VariableManip {
	public:
	    VariableManip( std::ostream& (*f)(std::ostream&, const Spex::var_name_and_expr& ), const Spex::var_name_and_expr& var ) : _f(f), _var(var) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const Spex::var_name_and_expr& );
	    const Spex::var_name_and_expr& _var;

	    friend std::ostream& operator<<(std::ostream & os, const VariableManip& m ) { return m._f(os,m._var); }
	};
	    
	class PrintInputVariable {
	public:
	    PrintInputVariable( std::ostream& output ) : _output( output ) {};
	    void operator()( const var_name_and_expr& var ) const { _output << Spex::print_input_variable( var ) << std::endl; }
	private:
	    std::ostream& _output;
	};

	class PrintInputArrayVariable {
	public:
	    PrintInputArrayVariable( std::ostream& output ) : _output( output ) {};
	    void operator()( const var_name_and_expr& var ) const { _output << Spex::print_input_variable( var ) << std::endl; }
	private:
	    std::ostream& _output;
	};
	

	class PrintResultVariable {
	public:
	    PrintResultVariable( std::ostream& output, unsigned int indent=0 ) : _output( output ), _indent(indent) {};
	    void operator()( const var_name_and_expr& var ) const;
	private:
	    std::ostream& _output;
	    const unsigned int _indent;
	};

    public:
	/* Saves info needed to output the observation variables */
	class ObservationInfo {
	public:
	    ObservationInfo() : _key(0), _phase(0), _variable_name(""), _conf_level(0), _conf_variable_name("") {}
	    ObservationInfo( int key, unsigned int phase, const char * variable_name=nullptr, unsigned int conf_level=0, const char * conf_variable_name=nullptr );
	    ObservationInfo& operator=( const ObservationInfo& );
	    bool operator()( const DOM::DocumentObject * o1, const DOM::DocumentObject * o2 ) const;
	    bool operator()( const ObservationInfo&, const ObservationInfo& ) const;
		
	    int getKey() const { return _key; }
	    std::string getKeyCode() const;
	    const std::string& getKeyName() const;
	    unsigned int getPhase() const { return _phase; }
	    unsigned int getConfLevel() const { return _conf_level; }
	    ObservationInfo& setConfLevel( unsigned int conf_level ) { _conf_level = conf_level; return *this; }

	    const std::string& getVariableName() const { return _variable_name; }
	    ObservationInfo& setVariableName( const std::string& variable_name );
	    const std::string& getConfVariableName() const { return _conf_variable_name; }
	    ObservationInfo& setConfVariableName( const std::string& conf_variable_name );
	    std::ostream& print( std::ostream& ) const;

	private:
	    const int _key;
	    const unsigned int _phase;
	    std::string _variable_name;
	    unsigned int _conf_level;
	    std::string _conf_variable_name;
	};

	struct ComprehensionInfo {
	    friend std::ostream& operator<<( std::ostream&, const Spex::ComprehensionInfo& );

	public:
	    ComprehensionInfo() : _init(0), _test(0), _step(0) {}
	    ComprehensionInfo( double, double, double );

	    LQX::SyntaxTreeNode * init(const std::string&) const;
	    LQX::SyntaxTreeNode * test(const std::string&) const;
	    LQX::SyntaxTreeNode * step(const std::string&) const;
	    double getInit() const { return _init; }
	    double getTest() const { return _test; }
	    double getStep() const { return _step; }
	    std::ostream& print( std::ostream& output ) const;

	private:
	    double _init;
	    double _test;
	    double _step;
	};

	class attribute_table_t 
	{
	private:
	    enum class attribute { IS_NULL, IS_EXTVAR, IS_STRING, IS_PROPERTY };

	public:
	    attribute_table_t() : _t(attribute::IS_NULL) { _f.a_null = nullptr; }
	    attribute_table_t( set_extvar_f f ) : _t(attribute::IS_EXTVAR) { _f.a_extvar = f; } 
	    attribute_table_t( const char * s ) : _t(attribute::IS_PROPERTY) { _f.a_string = s; }

	    LQX::SyntaxTreeNode * operator()( const std::string& name ) const;

	private:
	    const attribute _t;
	    union {
		void *	 a_null;
		set_extvar_f a_extvar;
		const char * a_string;
	    } _f;
	};
	    
	typedef std::multimap<const DOM::DocumentObject *,ObservationInfo,ObservationInfo> obs_var_tab_t;

    private:
	friend std::ostream& operator<<( std::ostream&, const Spex::ComprehensionInfo& );

    public:
	static bool __verbose;		/* Outputs input parameters per iteration.  */
	static bool __no_header;	/* Suppresses the header on output.	    */
	static bool __print_comment;	/* Output model comment at top of output    */

	Spex();

	bool construct_program( expr_list * main_line, expr_list * result, expr_list * convergence, expr_list * gnuplot=nullptr );

	static void clear();

	/*
	 * Parameters have all been declared and set.  Now we start the control program.
	 * Create the foreach loops for all of the local variables created due to array lists in the parameters.
	 */

	static bool is_global_var( const std::string& );
	static bool has_input_var( const std::string& );
	static bool has_observation_var( const std::string& );
	static bool has_array_var( const std::string& );
	static LQX::SyntaxTreeNode * get_input_var_expr( const std::string& );
	static void clear_input_variables() { __input_variables.clear(); }
	static unsigned int numberOfInputVariables() { return __input_variables.size(); }
	static unsigned int numberOfResultVariables() { return __result_variables.size(); }

	/* Used by srvn_output and qnap_document... */
	static const std::vector<std::string>& scalar_variables() { return  __scalar_variables; }			/* Saves $<scalar_name> for output */
	static const std::vector<std::string>& array_variables() { return  __array_variables; }				/* Saves $<array_name> for generating nest for loops */
	static const std::map<std::string,ComprehensionInfo>& comprehensions() { return __comprehensions; }		/* comprehension name (and values) */
	static const std::vector<ObservationInfo>& document_variables() { return  __document_variables; }
	static const std::map<const DOM::ExternalVariable *,const LQX::SyntaxTreeNode *>& inline_expressions() { return __inline_expression; }	/* Maps temp vars to expressions */
	static const std::map<std::string,LQX::SyntaxTreeNode *>& input_variables() { return __input_variables; }
	static const obs_var_tab_t& observations() { return __observations; }
	static const std::vector<var_name_and_expr>& result_variables() { return __result_variables; }

	
	static std::ostream& printResultVariables( std::ostream& output );
	static VariableManip print_input_variable( const var_name_and_expr& var ) { return VariableManip( printInputVariable, var ); }
	static VariableManip print_input_array_variable( const var_name_and_expr& var ) { return VariableManip( printInputArrayVariable, var ); }
	static VariableManip print_result_variable( const var_name_and_expr& var ) { return VariableManip( printResultVariable, var ); }


	LQX::VariableExpression * get_observation_variable( const std::string& ) const;
	LQX::SyntaxTreeNode * observation( const ObservationInfo& obs );
	LQX::SyntaxTreeNode * observation( const DOM::DocumentObject* object, const ObservationInfo& obs );
	LQX::SyntaxTreeNode * observation( const DOM::Task* task, const DOM::Activity *activity, const ObservationInfo& obs );
	LQX::SyntaxTreeNode * observation( const DOM::Entry* src, const unsigned int phase, const DOM::Entry* dst, const ObservationInfo& obs );
	LQX::SyntaxTreeNode * observation( const DOM::Entry* src, const DOM::Entry* dst, const ObservationInfo& obs );
	LQX::SyntaxTreeNode * observation( const DOM::Task* task, const DOM::Activity *activity, const DOM::Entry* dst, const ObservationInfo& obs );

    private:
	Spex(const Spex&) = delete;
	Spex& operator=( const Spex& ) = delete;

	bool has_vars() const;							/* True if any $var (except control args) set */
	
	LQX::SyntaxTreeNode * get_destination( const std::string& name ) const;
	LQX::SyntaxTreeNode * observation( LQX::MethodInvocationExpression * lqx_obj, const DOM::DocumentObject * document_obj, const ObservationInfo& obs );

	LQX::SyntaxTreeNode * foreach_loop( std::vector<std::string>::const_iterator var_p, expr_list * result, expr_list * convergence ) const;
	expr_list * convergence_loop( expr_list * convergence, expr_list * result ) const;
	expr_list * loop_body( expr_list * result ) const;
	expr_list * solve_success( expr_list * result ) const;
	expr_list * solve_failure( expr_list * result ) const;

	LQX::SyntaxTreeNode * print_comment( const std::string& = "" ) const;
	LQX::SyntaxTreeNode * print_header() const;
	LQX::SyntaxTreeNode * print_gnuplot_header() const;
	expr_list * plot( expr_list * );
	expr_list * splot( expr_list * );
	std::map<const LQX::SyntaxTreeNode *,std::string> get_plot_args( expr_list *, expr_list * );

	static expr_list * make_list( LQX::SyntaxTreeNode*, ... );
	static LQX::SyntaxTreeNode * print_node( const std::string& );
	
	static ObservationInfo * findObservation( const std::string& );		/* Find the observation matching string */
	static std::ostream& printInputVariable( std::ostream& output, const var_name_and_expr& var );
	static std::ostream& printInputArrayVariable( std::ostream& output, const var_name_and_expr& var );
	static std::ostream& printResultVariable( std::ostream& output, const var_name_and_expr& var );

	static std::vector<var_name_and_expr>::const_iterator find( std::vector<var_name_and_expr>::const_iterator, std::vector<var_name_and_expr>::const_iterator, const std::string& );

    public:
	static std::map<std::string, LQIO::DOM::SymbolExternalVariable*>* __global_variables;	/* Document global variables. (input) */

    private:
	static std::vector<std::string> __scalar_variables;			/* Saves $<scalar_name> for output */
	static std::vector<std::string> __array_variables;			/* Saves $<array_name> for generating nest for loops */
	static std::set<std::string> __array_references;			/* Saves $<array_name> when used as an lvalue */
	static std::vector<var_name_and_expr> __result_variables;		/* Saves $<name> for printing the header of variable names and the expression attached */
	static std::vector<std::string> __convergence_variables;		/* Saves $<name> for all variables used in convergence section */
	static std::map<std::string,LQX::SyntaxTreeNode *> __observation_variables;	/* Saves all observations (name, and funky assignment) */
	static std::map<std::string,ComprehensionInfo> __comprehensions;	/* Saves all comprehensions for $<name> */
	static expr_list __deferred_assignment;					/* Saves all parameters that depend on a variable for latter assignment */

	/* For SRVN input output */

	static obs_var_tab_t __observations;					/* Saves all key-$var for each object */
	static std::vector<ObservationInfo> __document_variables;		/* Saves all key-$var for the document */
	static std::map<std::string,std::string> __input_iterator;		/* Saves iterator for x, y = expr statements */
	static std::map<std::string,LQX::SyntaxTreeNode *> __input_variables;	/* Saves input values per iteration */
	static std::map<const DOM::ExternalVariable *,const LQX::SyntaxTreeNode *> __inline_expression;	/* Maps temp vars to expressions */

	static const std::map<const std::string,const attribute_table_t> __control_parameters;
	static const std::map<const int,const std::pair<const std::string,const std::string> > __key_code_map;	/* Maps srvn_gram.h KEY_XXX to name */
	static const std::map<const int,const std::string> __key_lqx_function_map;	/* Maps srvn_gram.h KEY_XXX to lqx function name */

	static const char * __convergence_limit_str;

	static void * __parameter_list;						/* JSON */
	static void * __result_list;						/* JSON */
	static void * __convergence_list;					/* JSON */
	static void * __temp_variable;						/* JSON */

	expr_list _gnuplot;							/* Gnuplot program */
	std::map<std::string,size_t> _result_pos;				/* Index of variable in list. */

    };

    inline std::ostream& operator<<( std::ostream& output, const Spex::ComprehensionInfo& self) { return self.print( output ); }
    extern class Spex spex;
}
#endif /* __cplusplus */
#endif /* __LQIO_SRVN_SPEX_H__ */
