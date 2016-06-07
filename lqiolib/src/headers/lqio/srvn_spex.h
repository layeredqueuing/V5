/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2012.								*/
/************************************************************************/

/*
 * $Id: xerces_common.h 10540 2011-11-17 16:59:31Z greg $
 */

#ifndef __LQIO_SRVN_SPEX_H__
#define __LQIO_SRVN_SPEX_H__

#include <stdbool.h>
#if defined(__cplusplus)
#include <vector>
#include <map>
#include <string>
#include "common_io.h"

namespace LQX {
    class SyntaxTreeNode;
    class MethodInvocationExpression;
}
namespace LQIO {
    namespace DOM {
	class ExternalVariable;
	class Document;
	class Task;
	class Activity;
	class DocumentObject;
	class Json_Document;
    }
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
    void * spex_inline_expression( void * arg );
    void * spex_processor_observation( const void * obj, const int key, const int, const char * var, const char * var2 );
    void * spex_result_assignment_statement( const char *var, void * arg );
    void * spex_task_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );

    void * spex_list( void * list, void * stmt );
    void * spex_add( void * arg1, void * arg2 );
    void * spex_and( void * arg1, void * arg3 ); 
    void * spex_divide( void * arg1, void * arg2 );
    void * spex_equals( void * arg1, void * arg3 ); 
    void * spex_greater_than( void * arg1, void * arg3 ); 
    void * spex_greater_than_or_equals( void * arg1, void * arg3 ); 
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
    namespace DOM {
	typedef LQIO::DOM::Document& (LQIO::DOM::Document::*set_extvar_f)( LQIO::DOM::ExternalVariable* );
	typedef void (*setSpexFunc)( LQIO::DOM::ExternalVariable* );

	class Spex {
	    friend class Json_Document;
	    friend void ::spex_set_program( void * param_arg, void * result_arg, void * convergence_arg );
	    friend void * ::spex_activity_call_observation( const void * task, const void * src, const int key, const void * dst, const int conf, const char * var, const char * var2  );
	    friend void * ::spex_activity_observation( const void * obj1, const void * obj2, const int key, const int conf, const char * var, const char * var2  );
	    friend void * ::spex_array_assignment( const char * name, void * list, const bool constant_expression );
	    friend void * ::spex_array_comprehension( const char * name, double begin, double end, double stride );
	    friend void * ::spex_assignment_statement( const char * name, void * expr, const bool constant_expression );
	    friend void * ::spex_call_observation( const void * src, const int key, const int phase, const void * dst, const int conf, const char * var, const char * var2  );
	    friend void * ::spex_convergence_assignment_statement( const char * name, void * expr );
	    friend void * ::spex_document_observation( const int key, const char * var );
	    friend void * ::spex_entry_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );
	    friend void * ::spex_forall( const char * iter_name, const char * name, void * expr );
	    friend void * ::spex_inline_expression( void * arg );
	    friend void * ::spex_processor_observation( const void * obj, const int key, const int, const char * var, const char * var2 );
	    friend void * ::spex_result_assignment_statement( const char * name, void * expr );
	    friend void * ::spex_task_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );
	    
	public:
	    class InputVariableManip {
	    public:
		InputVariableManip( std::ostream& (*f)(std::ostream&, const std::pair<std::string,LQX::SyntaxTreeNode *>& ), const std::pair<std::string,LQX::SyntaxTreeNode *>& var ) : _f(f), _var(var) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const std::pair<std::string,LQX::SyntaxTreeNode *>& );
		const std::pair<std::string,LQX::SyntaxTreeNode *>& _var;

		friend std::ostream& operator<<(std::ostream & os, const InputVariableManip& m ) { return m._f(os,m._var); }
	    };
	    
	    class PrintInputVariable {
	    public:
		PrintInputVariable( std::ostream& output ) : _output( output ) {};
		void operator()( const std::pair<std::string,LQX::SyntaxTreeNode *>& var ) const { _output << Spex::print_input_variable( var ) << std::endl; }
	    private:
		std::ostream& _output;
	    };

	    class PrintResultVariable {
	    public:
		PrintResultVariable( std::ostream& output ) : _output( output ) {};
		void operator()( const std::string& var ) const { _output << "  " << Spex::print_result_variable( var ) << std::endl; }
	    private:
		std::ostream& _output;
	    };

	public:
	    /* Saves info needed to output the observation variables */
	    class ObservationInfo {
	    public:
		ObservationInfo() : _key(0), _phase(0), _variable_name(""), _conf_level(0), _conf_variable_name("") {}
		ObservationInfo( int key, unsigned int phase, const char * variable_name, unsigned int conf_level=0, const char * conf_variable_name=0 );
		bool operator()( const DocumentObject * o1, const DocumentObject * o2 ) const;
		std::ostream& print( std::ostream& ) const;
		int getKey() const { return _key; }
		unsigned int getPhase() const { return _phase; }
		unsigned int getConfLevel() const { return _conf_level; }

		const std::string& getVariableName() const { return _variable_name; }
		const std::string& getConfVariableName() const { return _conf_variable_name; }

	    private:
		int _key;
		unsigned int _phase;
		std::string _variable_name;
		unsigned int _conf_level;
		std::string _conf_variable_name;
	    };

	    struct ComprehensionInfo {
		friend 	std::ostream& operator<<( std::ostream&, const Spex::ComprehensionInfo& );

	    public:
		ComprehensionInfo() : _init(0), _test(0), _step(0) {}
		ComprehensionInfo( double, double, double );

		LQX::SyntaxTreeNode * init(const std::string&) const;
		LQX::SyntaxTreeNode * test(const std::string&) const;
		LQX::SyntaxTreeNode * step(const std::string&) const;
		std::ostream& print( std::ostream& output ) const;

	    private:
		double _init;
		double _test;
		double _step;
	    };

	    class attribute_table_t 
	    {
	    private:
		typedef enum { IS_NULL, IS_EXTVAR, IS_STRING, IS_PROPERTY } dom_attribute_t;

	    public:
		attribute_table_t() : _t(IS_NULL) { _f.a_null = 0; }
		attribute_table_t( set_extvar_f f ) : _t(IS_EXTVAR) { _f.a_extvar = f; } 
		attribute_table_t( const char * s ) : _t(IS_PROPERTY) { _f.a_string = s; }

		LQX::SyntaxTreeNode * operator()( const std::string& name ) const;

	    private:
		dom_attribute_t _t;
		union {
		    void *	 a_null;
		    set_extvar_f a_extvar;
		    const char * a_string;
		} _f;
	    };
	    
	    typedef std::multimap<const DocumentObject *,ObservationInfo,ObservationInfo> obs_var_tab_t;

	private:
	    friend std::ostream& operator<<( std::ostream&, const Spex::ComprehensionInfo& );

	public:
	    static bool __verbose;		/* Outputs input parameters per iteration. */
	    static bool __no_header;		/* Suppresses the header on output.	*/

	    Spex();
	    static void clear();

	    /*
	     * Parameters have all been declared and set.  Now we start the control program.
	     * Create the foreach loops for all of the local variables created due to array lists in the parameters.
	     */

	    static void initialize_control_parameters();

	    static bool is_global_var( const std::string& );
	    static bool has_input_var( const std::string& );
	    static LQX::SyntaxTreeNode * get_input_var_expr( const std::string& );
	    static const std::vector<ObservationInfo>& get_document_variables() { return  __document_variables; }
	    static const obs_var_tab_t& get_observation_variables() { return __observation_variables; }
	    static const std::map<std::string,LQX::SyntaxTreeNode *>& get_input_variables() { return __input_variables; }
	    static unsigned int numberOfInputVariables() { return __input_variables.size(); }
	    static unsigned int numberOfResultVariables() { return __result_expression.size(); }
	    static const std::vector<std::string>& get_result_variables() { return __result_variables; }
	    static std::ostream& printInputVariables( std::ostream& output );
	    static std::ostream& printObservationVariables( std::ostream& output );
	    static std::ostream& printObservationVariables( std::ostream& output, const DocumentObject& );
	    static std::ostream& printResultVariables( std::ostream& output );
	    static InputVariableManip print_input_variable( const std::pair<std::string,LQX::SyntaxTreeNode *>& var ) { return InputVariableManip( printInputVariable, var ); }
	    static StringManip print_result_variable( const std::string& var ) { return StringManip( printResultVariable, var ); }

	private:
	    Spex(const Spex&);
	    Spex& operator=( const Spex& );

	    bool has_vars() const;							/* True if any $var (except control args) set */

	    bool construct_program( expr_list * main_line, expr_list * result, expr_list * convergence );
	    LQX::SyntaxTreeNode * get_destination( const std::string& name ) const;
	    LQX::SyntaxTreeNode * observation( const int key, const char * var );
	    LQX::SyntaxTreeNode * observation( const DocumentObject* object, const unsigned int phase, const std::string& type_name, ObservationInfo& obs );
	    LQX::SyntaxTreeNode * observation( const Task* task, const Activity *activity, const std::string& type_name, ObservationInfo& obs );
	    LQX::SyntaxTreeNode * observation( const DocumentObject* src, const unsigned int phase, const Entry* dst, ObservationInfo& obs );
	    LQX::SyntaxTreeNode * observation( const Task* task, const Activity *activity, const Entry* dst, ObservationInfo& obs );
	    LQX::SyntaxTreeNode * observation( LQX::MethodInvocationExpression * lqx_obj, const DocumentObject * document_obj, ObservationInfo& obs );

	    LQX::SyntaxTreeNode* foreach_loop( std::vector<std::string>::const_iterator var_p, expr_list * result, expr_list * convergence ) const;
	    expr_list * convergence_loop( expr_list * convergence, expr_list * result ) const;
	    expr_list * loop_body( expr_list * result ) const;
	    expr_list * solve_success( expr_list * result ) const;
	    expr_list * solve_failure( expr_list * result ) const;
	    LQX::SyntaxTreeNode* print_header() const;
	    static std::ostream& printInputVariable( std::ostream&, const std::pair<std::string,LQX::SyntaxTreeNode *>& var );
	    static std::ostream& printResultVariable( std::ostream& output, const std::string& var );

	private:
	    static std::vector<std::string> __array_variables;				/* Saves $<array_name> for generating nest for loops */
	    static std::vector<std::string> __result_variables;				/* Saves $<name> for printing the header of variable names */
	    static std::vector<std::string> __convergence_variables;			/* Saves $<name> for all variables used in convergence section */
	    static std::map<std::string,LQX::SyntaxTreeNode *> __observations;		/* Saves all observations (name, and funky assignment) */
	    static std::map<std::string,ComprehensionInfo> __comprehensions;		/* Saves all comprehensions for $<name> */
	    static expr_list __deferred_assignment;					/* Saves all parameters that depend on a variable for latter assignment */

	    /* For SRVN input output */

	    static std::map<std::string,LQX::SyntaxTreeNode *> __input_variables;	/* Saves input values per iteration */
	    static obs_var_tab_t __observation_variables;				/* Saves all key-$var for each object */
	    static std::vector<ObservationInfo> __document_variables;			/* Saves all key-$var for the document */
	    static std::map<std::string,std::string> __input_iterator;			/* Saves iterator for x, y = expr statements */
	    static std::map<std::string,LQX::SyntaxTreeNode *> __result_expression;	/* Maps result vars to expressions */
	public:
	    static std::map<const DOM::ExternalVariable *,const LQX::SyntaxTreeNode *> __inline_expression;	/* Maps temp vars to expressions */
	private:
	    static std::map<std::string,attribute_table_t> __control_parameters;
	    static std::map<int,std::string> __key_code_map;				/* Maps srvn_gram.h KEY_XXX to name */
	    static std::map<int,std::string> __key_lqx_function_map;			/* Maps srvn_gram.h KEY_XXX to lqx function name */

	    static const char * __convergence_limit_str;
	};

	inline std::ostream& operator<<( std::ostream& output, const Spex::ComprehensionInfo& self) { return self.print( output ); }
	extern class Spex spex;
    }
}
#endif
#endif /* __LQIO_SRVN_SPEX_H__ */
