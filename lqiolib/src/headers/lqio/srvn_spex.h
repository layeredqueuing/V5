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


namespace LQX {
    class SyntaxTreeNode;
}

extern "C" {
#endif
    void spex_set_program( void * param, void * result, void * convergence );
    void spex_set_default_model_parameters();
    void * spex_list( void * list, void * stmt );
    void * spex_assignment_statement( const char * var, void * expr, const bool );
    void * spex_array_assignment( const char * var, void * list, const bool );
    void * spex_array_assignment2( const char * var, double begin, double stride, double end, const bool );
    void * spex_ternary( void * arg1, void * arg2, void * arg3 );
    void * spex_comma( void * arg1, void * arg2 );
    void * spex_add( void * arg1, void * arg2 );
    void * spex_subtract( void * arg1, void * arg2 );
    void * spex_multiply( void * arg1, void * arg2 );
    void * spex_divide( void * arg1, void * arg2 );
    void * spex_modulus( void * arg1, void * arg2 );
    void * spex_power( void * arg1, void * arg2 );
    void * spex_or( void * arg1, void * arg3 ); 
    void * spex_and( void * arg1, void * arg3 ); 
    void * spex_equals( void * arg1, void * arg3 ); 
    void * spex_not_equals( void * arg1, void * arg3 ); 
    void * spex_less_than( void * arg1, void * arg3 ); 
    void * spex_less_than_or_equals( void * arg1, void * arg3 ); 
    void * spex_greater_than( void * arg1, void * arg3 ); 
    void * spex_greater_than_or_equals( void * arg1, void * arg3 ); 
    void * spex_not( void * arg2 ); 
    void * spex_invoke_function( const char *s, void * arg );
    void * spex_processor_observation( void * obj, const int key, const int, const char * var, const char * var2 );
    void * spex_task_observation( void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );
    void * spex_entry_observation( void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );
    void * spex_activity_observation( void * obj, const int key, const int phase, const int conf, const char * var, const char * var2  );
    void * spex_call_observation( void * src, const int key, const int phase, void * dst, const int conf, const char * var, const char * var2  );
    void * spex_activity_call_observation( void * src, const int key, const int phase, void * dst, const int conf, const char * var, const char * var2  );
    void * spex_result_assignment_statement( const char *var, void * arg );
    void * spex_convergence_assignment_statement( const char *var, void * arg );
    void * spex_get_symbol( const char * s );
    void * spex_get_real( double arg );

#if defined(__cplusplus)
}

typedef std::vector<LQX::SyntaxTreeNode*> expr_list;

namespace LQIO {
    namespace DOM {
	class ExternalVariable;
	class Document;
	typedef LQIO::DOM::Document& (LQIO::DOM::Document::*setParameterFunc)( LQIO::DOM::ExternalVariable* );

	class Spex {
	public:
	    static std::vector<std::string> __array_variables;			/* Saves $<array_name> for generating nest for loops */
	    static std::vector<std::string> __result_variables;			/* Saves $<name> for printing the header of variable names */
	    static std::vector<std::string> __convergence_variables;		/* Saves $<name> for all variables used in convergence section */
	    static std::vector<std::string> __input_variables;			/* Saves input values per iteration */
	    static std::map<std::string,LQX::SyntaxTreeNode *> __observations;	/* Saves all observations (name, and funky assignment) */
	    static expr_list __deferred_assignment;				/* Saves all parameters that depend on a variable for latter assignment */
	    static std::map<std::string,setParameterFunc> __control_parameters;
	    static bool __have_vars;		/* True if any $var (except control args) set */
	    static bool __verbose;		/* Outputs input parameters per iteration. */
	    static bool __no_header;		/* Suppresses the header on output.	*/

	    Spex();

	    /*
	     * Parameters have all been declared and set.  Now we start the control program.
	     * Create the foreach loops for all of the local variables created due to array lists in the parameters.
	     */

	    expr_list * construct_program( expr_list * main_line, expr_list * result, expr_list * convergence );
	    LQX::SyntaxTreeNode * observation( const std::string& type_name, const std::string& object_name, const int phase, const int key, const char * var_name );
	    LQX::SyntaxTreeNode * observation( const std::string& type_name, const std::string& src_name, const int phase, const std::string& dst_name, const int key, const char * var_name );
	    LQX::SyntaxTreeNode * confidence_interval( LQX::SyntaxTreeNode * object, const int key, const int interval, const char * var_name );
	    static void initialize_control_parameters();

	private:
	    Spex(const Spex&);
	    Spex& operator=( const Spex& );

	    LQX::SyntaxTreeNode* foreach_loop( std::vector<std::string>::const_iterator var_p, expr_list * result, expr_list * convergence ) const;
	    expr_list * convergence_loop( expr_list * convergence, expr_list * result ) const;
	    expr_list * loop_body( expr_list * result ) const;
	    expr_list * solve_success( expr_list * result ) const;
	    expr_list * solve_failure( expr_list * result ) const;
	    LQX::SyntaxTreeNode* print_header() const;
	};

	extern class Spex spex;
    }
}
#endif
#endif /* __LQIO_SRVN_SPEX_H__ */
