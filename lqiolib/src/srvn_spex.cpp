/*
 *  $Id: srvn_spex.cpp 14879 2021-07-07 01:51:16Z greg $
 *
 *  Created by Greg Franks on 2012/05/03.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 * Note: Be careful with static casts because of lists and nodes.
 */

#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <lqx/SyntaxTree.h>
#include <lqx/Program.h>
#include "dom_document.h"
#include "dom_bindings.h"
#include "dom_phase.h"
#include "filename.h"
#include "srvn_input.h"
#include "srvn_spex.h"
#include "glblerr.h"

extern "C" {
#include "srvn_gram.h"
}

namespace LQIO {

    Spex::Spex() : _gnuplot(), _result_pos()
    {
    }

    void Spex::clear()
    {
	__array_variables.clear();			/* Saves $<array_name> for generating nest for loops */
	__array_references.clear();			/* Saves $<array_name> when used as an lvalue */
	__result_variables.clear();			/* Saves $<name> for printing the header of variable names */
	__convergence_variables.clear();		/* Saves $<name> for all variables used in convergence section */
	__observations.clear();
	__comprehensions.clear();
	__deferred_assignment.clear();
	__input_variables.clear();			/* Saves input values per iteration */
	__observation_variables.clear();		/* */
	__document_variables.clear();			/* Saves all key-$var for the document */
	__input_iterator.clear();
	__inline_expression.clear();			/* Maps temp vars to expressions */
	__parameter_list = nullptr;
	__result_list = nullptr;
	__convergence_list = nullptr;
	spex._gnuplot.clear();
	spex._result_pos.clear();
    }


    bool Spex::is_global_var( const std::string& name )
    {
	return __global_variables != nullptr && __global_variables->find( name ) != __global_variables->end();
    }

    bool Spex::has_input_var( const std::string& name )
    {
	return __input_variables.find( name ) != __input_variables.end();
    }

    bool Spex::has_array_var( const std::string& name )
    {
	return std::find( __array_variables.begin(), __array_variables.end(), name ) != __array_variables.end();
    }

    bool Spex::has_observation_var( const std::string& name )
    {
	return __observation_variables.find( name ) != __observation_variables.end();
    }

    /*
     * Used by lqn2lqx when outputting LQX from SPEX input
     */
	
    LQX::SyntaxTreeNode * Spex::get_input_var_expr( const std::string& name )
    {
	std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator iv_p = LQIO::Spex::__input_variables.find( name );
	if ( iv_p != LQIO::Spex::__input_variables.end() ) {
	    return iv_p->second;
	} else {
	    return nullptr;
	}
    }

	
    /*
     * Return true if there are any $vars (either as parameters, observations for objects (%f...), or observations for the document (%usr...)
     */

    bool Spex::has_vars() const
    {
	return __input_variables.size() > 0 || __observation_variables.size() > 0 || __document_variables.size() > 0;
    }

    /* ------------------------------------------------------------------------ */

    /*
     * Parameters have all been declared and set.  Now we start the control program.
     * Create the foreach loops for all of the local variables created due to array lists in the parameters.
     */

    bool Spex::construct_program( expr_list * main_line, expr_list * result, expr_list * convergence, expr_list * gnuplot )
    {
	/* If we have only set control variables, then nothing to do. */
	if ( !has_vars() ) return false;		

	/* If we don't have any results, print them all. */
	if ( result == nullptr ) {
	    /* copy name from array variables to __result_variable */
	    for ( std::vector<std::string>::iterator arr_p = __array_variables.begin(); arr_p !=__array_variables.end(); ++arr_p ) {
		result = static_cast<expr_list *>(spex_list( result, spex_result_assignment_statement( (*arr_p).c_str(), nullptr ) ));
	    }
	
	    /* copy name from observation variable __observation_variables to __result_variable. */
	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::iterator obs_p = __observation_variables.begin(); obs_p != __observation_variables.end(); ++obs_p ) {
		result = static_cast<expr_list *>(spex_list( result, spex_result_assignment_statement( obs_p->first.c_str(), nullptr ) ));
	    }
	}
	
	/* Add magic variable if not present */
	if ( __convergence_variables.size() > 0 && !DOM::__document->hasSymbolExternalVariable( __convergence_limit_str ) ) {
	    double convergence_value = DOM::__document->getModelConvergenceValue();
	    main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( __convergence_limit_str, true ), new LQX::ConstantValueExpression( convergence_value ) ) );			/* Add $0 variable */
	    DOM::__document->setModelConvergence( DOM::__document->db_build_parameter_variable( __convergence_limit_str, nullptr ) );
	}

	/* Initialize loop counter variable */
	main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "_0", false ), new LQX::ConstantValueExpression( 0.0 ) ) );			/* Add $0 variable */

	/* Add observation variables */
	for ( std::map<std::string,LQX::SyntaxTreeNode *>::iterator obs_p = __observation_variables.begin(); obs_p != __observation_variables.end(); ++obs_p ) {
	    const std::string& name = obs_p->first;		    /* Strip $ for LQX var name	 vvvvvvvv */
	    main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), new LQX::ConstantValueExpression( 0.0 ) ) );	/* Initialize all observations variables */
	}

	/* Remove any array references from the completions. */
	for ( std::set<std::string>::const_iterator i = __array_references.begin(); i != __array_references.end(); ++i ) {
	    std::vector<std::string>::iterator j = std::find( __array_variables.begin(), __array_variables.end(), *i );
	    if ( j != __array_variables.end() ) {
		__array_variables.erase(j);
	    }
	}

	/*+ GNUPlot or other header stuff. */
	if ( gnuplot != nullptr && !gnuplot->empty() ) {
	    main_line->push_back( print_node( "#!/opt/local/bin/gnuplot" ) );
	    main_line->push_back( print_gnuplot_header() );
	    main_line->push_back( print_node( "$DATA << EOF" ) );				/* Append newline.  Don't space */
	} else if ( !__no_header ) {
	    main_line->push_back( print_header() );
	}

	/* Add the code for running the SPEX program -- recursive call. */
	main_line->push_back( foreach_loop( __array_variables.begin(), result, convergence ) );

	/*+ gnuplot -> append the gnuplot program. */
	if ( gnuplot != nullptr && !gnuplot->empty() ) {
	    main_line->push_back( print_node( "EOF" ) );
	    main_line->insert( main_line->end(), gnuplot->begin(), gnuplot->end() );
	}
	return true;
    }

    /*
     * Figure out the destination (lvalue) for an assignment statement.	 If it's a control variable, attribute_table_t::operator() will
     * determine if the destination is a variable or if it is a "field" for an ObjectPropertyReadNode (somewhat misnamed).  Otherwise,
     * the destination is simply a variable.
     */

    LQX::SyntaxTreeNode * Spex::get_destination( const std::string& name ) const
    {
	LQX::SyntaxTreeNode * destination;
	std::map<const std::string,const Spex::attribute_table_t>::const_iterator i = Spex::__control_parameters.find( name );
	if ( i != Spex::__control_parameters.end() ) {
	    destination = i->second( i->first );
	} else {
	    destination = new LQX::VariableExpression(name,true);
	}
	return destination;
    }


    /* ------------------------------------------------------------------------ */

    /*
     * Document variables
     */
	
    LQX::SyntaxTreeNode * Spex::observation( const ObservationInfo& obs )
    {
	__document_variables.push_back( obs );
	LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "document" );
	LQX::ObjectPropertyReadNode * node = 0;
	switch ( obs.getKey() ) {
	case KEY_WAITING:	node = new LQX::ObjectPropertyReadNode( object, "waits" ); break;	/* Waits */
	case KEY_SERVICE_TIME:	node = new LQX::ObjectPropertyReadNode( object, "steps" ); break;	/* Steps */
	case KEY_ITERATIONS:	node = new LQX::ObjectPropertyReadNode( object, "iterations" ); break;
	case KEY_ELAPSED_TIME:	node = new LQX::ObjectPropertyReadNode( object,	 __lqx_elapsed_time ); break;
	case KEY_USER_TIME:	node = new LQX::ObjectPropertyReadNode( object, "user_cpu_time" ); break;
	case KEY_SYSTEM_TIME:	node = new LQX::ObjectPropertyReadNode( object, "system_cpu_time" ); break;
	default:	abort();
	}
	const std::string& name = obs.getVariableName();	      /* Strip $ for LQX name vvvvvvvv */
	__observation_variables[name] = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), node );
	return object;		/* For chaining */
    }

    /*
     * Processor, Task, Entry, Phase variables
     */
	
    LQX::SyntaxTreeNode * Spex::observation( const DOM::DocumentObject* document_object, const ObservationInfo& obs )
    {
	const unsigned p = obs.getPhase();
	LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( document_object->getTypeName(), new LQX::ConstantValueExpression( document_object->getName() ), NULL );
	if ( dynamic_cast<const DOM::Entry *>(document_object) != nullptr && 0 < p && p <= DOM::Phase::MAX_PHASE ) {
	    /* A phase of an entry */
	    object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(p) ), NULL );
	    document_object = dynamic_cast<const DOM::Entry *>(document_object)->getPhase( p );
	}
	return observation( object, document_object, obs );
    }

    /*
     * Activity variables
     */
	
    LQX::SyntaxTreeNode * Spex::observation( const DOM::Task* task, const DOM::Activity * activity, const ObservationInfo& obs )
    {
	LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( task->getTypeName(), new LQX::ConstantValueExpression( task->getName() ), NULL );
	object = new LQX::MethodInvocationExpression( activity->getTypeName(), object, new LQX::ConstantValueExpression( activity->getName() ), NULL );
	return observation( object, activity, obs );
    }
	
    /*
     * Call Variables
     */
	
    LQX::SyntaxTreeNode * Spex::observation( const DOM::Entry* src, const unsigned int phase, const DOM::Entry* dst, const ObservationInfo& obs )
    {
	assert( dynamic_cast<const DOM::Entry *>(src) != 0 && phase != 0 );

	LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( src->getTypeName(), new LQX::ConstantValueExpression( src->getName() ), NULL );
	object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(phase) ), NULL );
	const DOM::Call * call = dynamic_cast<const DOM::Entry *>(src)->getCallToTarget( dynamic_cast<const DOM::Entry *>(dst), phase );
	object = new LQX::MethodInvocationExpression( call->getTypeName(), object, new LQX::ConstantValueExpression( dst->getName() ), NULL );
	return observation( object, call, obs );

    }

    /*
     * Activity Call variables
     */
	
    LQX::SyntaxTreeNode * Spex::observation( const DOM::Task* task, const DOM::Activity * activity, const DOM::Entry * dst, const ObservationInfo& obs )
    {
	LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( task->getTypeName(), new LQX::ConstantValueExpression( task->getName() ), NULL );
	object = new LQX::MethodInvocationExpression( activity->getTypeName(), object, new LQX::ConstantValueExpression( activity->getName() ), NULL );
	const DOM::Call * call = activity->getCallToTarget( dst );
	object = new LQX::MethodInvocationExpression( call->getTypeName(), object, new LQX::ConstantValueExpression( dst->getName() ), NULL );
	return observation( object, call, obs );
    }
	
    /*
     * Common code.
     */
	
    LQX::SyntaxTreeNode * Spex::observation( LQX::MethodInvocationExpression * lqx_obj, const DOM::DocumentObject * doc_obj, const ObservationInfo& obs )
    {
	const std::string& name = obs.getVariableName();
	const int key = obs.getKey();
	if ( has_input_var( name ) ) {
	    LQIO::input_error2( LQIO::ERR_SPEX_PARAMETER_OBSERVATION, name.c_str(), obs.getKeyCode().c_str(), doc_obj->getName().c_str() );
	}
	__observations.insert( std::pair<const DOM::DocumentObject*,ObservationInfo>(doc_obj,obs) );			/* Save variable names per DOM object */
	__observation_variables[name] = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ),	/* Strip $ for LQX variable name */
									  new LQX::ObjectPropertyReadNode( lqx_obj, __key_lqx_function_map.at(key) ) );
	const unsigned int conf_level = obs.getConfLevel();
	if ( conf_level != 0 ) {
	    const std::string& conf_name = obs.getConfVariableName().c_str();
	    lqx_obj = new LQX::MethodInvocationExpression( "conf_int", lqx_obj, new LQX::ConstantValueExpression( static_cast<double>(conf_level) ), NULL );
	    __observation_variables[conf_name] = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &conf_name[1], false ),
										   new LQX::ObjectPropertyReadNode( lqx_obj, __key_lqx_function_map.at(key) ) );
	}
	return lqx_obj;		/* For chaining */
    }
	
    /*
     * foreach( i,$var in list );
     * loop_stmt(X) ::= FOREACH OBRACKET IDENTIFIER(A) COMMA IDENTIFIER(B) IN expr(C) CBRACKET stmt(D).
     *		       { X = new ForeachStatementNode( A->getStoredIdentifier(), B->getStoredIdentifier(), A->getIsExternal(), B->getIsExternal(), C, D ); }
     */

    LQX::SyntaxTreeNode* Spex::foreach_loop( std::vector<std::string>::const_iterator var_p, expr_list * result, expr_list * convergence ) const
    {
	if ( var_p != __array_variables.end() ) {
	    std::string name = *var_p;	/* Make local copy because we force to local */
	    const std::map<std::string,Spex::ComprehensionInfo>::const_iterator i = __comprehensions.find( *var_p );
	    LQX::SyntaxTreeNode * expr = foreach_loop( std::next( var_p ), result, convergence );
	    if ( !_gnuplot.empty() && std::next( var_p ) != __array_variables.end() ) {
		expr_list * loop_body = new expr_list;
		loop_body->push_back( expr );
		loop_body->push_back( new LQX::FilePrintStatementNode( new expr_list, true, false ) );	/* Insert a blank line */
		expr = new LQX::CompoundStatementNode( loop_body );
	    }
	    if ( i == __comprehensions.end() ) {
		/* if we have $x = [...] */
		name[0] = '_';			/* Array name */
		return new LQX::ForeachStatementNode( "", &name[1], /* key ext */ false, /* val */ false,
						      new LQX::VariableExpression( name, false ), expr );
	    } else {
		/* for ( i = begin; i < end; ++i )
		   $x = f(i); */
		return new LQX::LoopStatementNode( i->second.init(&name[1]), i->second.test(&name[1]), i->second.step(&name[1]), expr );
	    }
	} else if ( convergence ) {
	    return new LQX::CompoundStatementNode( convergence_loop( convergence, result ) );
	} else {
	    return new LQX::CompoundStatementNode( loop_body( result ) );
	}
    }

    expr_list * Spex::convergence_loop( expr_list * convergence, expr_list * result ) const
    {
	expr_list * loop_code = new expr_list;

	/* Make local 'old' variables for testing. */
	for ( std::vector<std::string>::const_iterator var_p = __convergence_variables.begin(); var_p != __convergence_variables.end(); ++var_p ) {
	    const std::string& name = *var_p;
	    loop_code->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), new LQX::ConstantValueExpression( 1.e9 ) ) );
	}

	/* Convergence test over all assignment statements/variables */
	LQX::SyntaxTreeNode * left_expr = 0;
	for ( std::vector<std::string>::const_iterator var_p = __convergence_variables.begin(); var_p != __convergence_variables.end(); ++var_p ) {
	    const std::string& name = *var_p;
	    LQX::SyntaxTreeNode * right_expr = new LQX::ComparisonExpression( LQX::ComparisonExpression::LESS_THAN,
									      new LQX::MethodInvocationExpression( "abs",
														   new LQX::MathExpression(LQX::MathExpression::SUBTRACT,
																	   new LQX::VariableExpression( &name[1], false ),
																	   new LQX::VariableExpression( name, true ) ),
														   NULL ),
									      new LQX::VariableExpression( __convergence_limit_str, true ) );
	    if ( var_p != __convergence_variables.begin() ) {
		left_expr = new LQX::LogicExpression( LQX::LogicExpression::AND, left_expr, right_expr );
	    }  else {
		left_expr = right_expr;
	    }
	}
	convergence->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "__done__", false ), left_expr ) );

	/* Copy new to old */
	for ( std::vector<std::string>::const_iterator var_p = __convergence_variables.begin(); var_p != __convergence_variables.end(); ++var_p ) {
	    const std::string& name = *var_p;
	    convergence->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), new LQX::VariableExpression( name, true ) ) );
	}
	
	/* Convergence loop */
#if sneaky
	loop_code->push_back( new LQX::LoopStatementNode( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "__done__", false ), new LQX::ConstantValueExpression( false ) ),
							  new LQX::LogicExpression(LQX::LogicExpression::NOT, new LQX::VariableExpression( "__done__", false ), nullptr),
							  new LQX::CompoundStatementNode( convergence ),
							  new LQX::CompoundStatementNode( loop_body( result ) ) ) );
#else
	/* Stick convergence stuff at end of loop body */
	expr_list * body = loop_body( result );
	body->insert( body->end(), convergence->begin(), convergence->end() );
	loop_code->push_back( new LQX::LoopStatementNode( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "__done__", false ), new LQX::ConstantValueExpression( false ) ),
							  new LQX::LogicExpression(LQX::LogicExpression::NOT, new LQX::VariableExpression( "__done__", false ), nullptr),
							  0,
							  new LQX::CompoundStatementNode( body ) ) );
#endif
	return loop_code;
    }

    /*
     * Code which is run in the innermost iteration of the experiments, i.e., for () { for () { loop_body() } } ;
     * The guts of the execution.
     */
	
    expr_list * Spex::loop_body( expr_list * result ) const
    {
	expr_list * loop_code = new expr_list;

	/* "Assign" any array variables which are used as parameters.  The array variable cannot be directly referenced. */
	for ( std::vector<std::string>::const_iterator var_p = __array_variables.begin(); var_p != __array_variables.end(); ++var_p ) {
	    const std::string& name = *var_p;
	    if ( (*is_global_var)( name ) ) {
		loop_code->push_back( new LQX::AssignmentStatementNode( get_destination(*var_p), new LQX::VariableExpression( &name[1], false ) ) );
	    }
	}

	/* Assign input variables which are not array vars. */
	
	for ( std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator iv_p = __input_variables.begin(); iv_p != __input_variables.end(); ++iv_p ) {
	    const std::string& name = iv_p->first;
	    if ( (*is_global_var)( name ) && std::find( __array_variables.begin(), __array_variables.end(), name ) == __array_variables.end() ) {
		loop_code->push_back( new LQX::AssignmentStatementNode( get_destination(name), new LQX::VariableExpression( &name[1], false ) ) );
	    }
	}

	/* load in all deferred assignment statements, eg '$x = $loop'... */
	loop_code->insert( loop_code->end(), Spex::__deferred_assignment.begin(), Spex::__deferred_assignment.end() );

	/* Increment magic variable $0 */
	loop_code->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "_0", false ),
								new LQX::MathExpression(LQX::MathExpression::ADD, new LQX::VariableExpression( "_0", false ), new LQX::ConstantValueExpression( 1.0 ) ) ) );

	if ( __verbose ) {
	    /* Need to go thru all input_variables */

	    expr_list * print_args = make_list( new LQX::ConstantValueExpression( const_cast<char *>("Input parameters:") ), nullptr );
	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator iv_p = __input_variables.begin(); iv_p != __input_variables.end(); ++iv_p ) {
		std::string s = " ";
		s += iv_p->first;
		s += "=";
		print_args->push_back( new LQX::ConstantValueExpression( s ) );

		const bool is_external = (*is_global_var)( iv_p->first );
		const std::string& name = iv_p->first;
		LQX::VariableExpression * variable = new LQX::VariableExpression( name, is_external );
		print_args->push_back( variable );
	    }
	    loop_code->push_back( new LQX::FilePrintStatementNode( print_args, true, false ) );
	}

	loop_code->push_back( new LQX::ConditionalStatementNode( new LQX::MethodInvocationExpression("solve"),
								 new LQX::CompoundStatementNode( solve_success( result ) ),
								 new LQX::CompoundStatementNode( solve_failure( result ) ) ) );

	return loop_code;
    }

    /*
     * Code which is run provided that the solver ran successfully.  This simply prints out the results.
     * !!! I need to check for unassigned variables. !!!
     */
	
    expr_list * Spex::solve_success( expr_list * result ) const
    {	
	expr_list * block = new expr_list;

	/* Insert all functions to extract results. */

	for ( std::map<std::string,LQX::SyntaxTreeNode *>::iterator obs_p = __observation_variables.begin(); obs_p != __observation_variables.end(); ++obs_p ) {
	    block->push_back( obs_p->second );
	}

//	    if ( __verbose ) {
//		block->push_back( new LQX::MethodInvocationExpression("print_symbol_table") );
//	    }

	/* Check result for observations.  Remove unused and flag -- they won't be in __observation_variables. */

	for ( std::vector<var_name_and_expr>::const_iterator var = __result_variables.begin(); var != __result_variables.end(); ++var ) {
	    if ( var->first == "$0" ) continue;		// Ignore $0.
	    const std::string& name = var->first;
	    if ( var->second == nullptr && !has_observation_var( name ) && !has_input_var( name ) ) {
		LQIO::input_error2( LQIO::ADV_SPEX_UNUSED_RESULT_VARIABLE, name.c_str() );
	    }
	}

	/* Insert print expression for results */

	if ( result && result->size() > 0 ) {
	    LQX::SyntaxTreeNode * separator = new LQX::ConstantValueExpression( ", " );	/* CSV. */
	    result->insert( result->begin(), separator );
	    block->push_back( new LQX::FilePrintStatementNode( result, true, true ) );	/* Force spaced output with newline */
	}

	return block;
    }

    /*
     * Sad panda.  The solver did not run successfully.
     */
	
    expr_list * Spex::solve_failure( expr_list * result ) const
    {
	return make_list( new LQX::FilePrintStatementNode( make_list( new LQX::ConstantValueExpression( "solver failed: $0=" ), new LQX::VariableExpression( "_0", false ), nullptr ), true, false ), nullptr );
    }

    /*
     * file_output_stmt(X) ::= FILE_PRINTLN_SP OBRACKET expr_list(A) CBRACKET.	     { X = new FilePrintStatementNode( A, true, true ); }
     */

    LQX::SyntaxTreeNode* Spex::print_header() const
    {
	expr_list * list = make_list( new LQX::ConstantValueExpression( ", " ), nullptr );
	for ( std::vector<Spex::var_name_and_expr>::iterator var = __result_variables.begin(); var != __result_variables.end(); ++var ) {
	    list->push_back( new LQX::ConstantValueExpression( var->first ) );	/* Variable name */
	}
	return new LQX::FilePrintStatementNode( list, true, true );		/* Println spaced, with first arg being ", " (or: output, ","). */
    }

    LQX::SyntaxTreeNode * Spex::print_gnuplot_header() const
    {
	expr_list * list = make_list( new LQX::ConstantValueExpression( " " ), new LQX::ConstantValueExpression( "# " ), nullptr );
	for ( std::vector<Spex::var_name_and_expr>::iterator var = __result_variables.begin(); var != __result_variables.end(); ++var ) {
	    list->push_back( new LQX::ConstantValueExpression( var->first ) );	/* Variable name */
	}
	return new LQX::FilePrintStatementNode( list, true, true );		/* Print out a comment with the values that will follow */
    }


    /*
     * Generate code for GNUPLOT output.  This will be appended to the main line code to output Gnuplot.  This could be changed to
     * allow for more than one plot call in the input file as the result variables are set here (so use the size of
     * __result_variables as the index for x and y).  That depends though on whether I can plot different things in one gnuplot run.
     * I suppose if I output to a file for each plot that might work.
     */

    expr_list *
    Spex::plot( expr_list * args )
    {
	expr_list * list = new expr_list;
	size_t y1_vars = 0;			// dependent variable count (y1 axis)
	size_t y2_vars = 0;			// dependent variable count (y2 axis)
	int y1_obs_key = 0;			// dependent (y1 axis) type.
	int y2_obs_key = 0;			// dependent (y2 axis) type.
	const std::map<const LQX::SyntaxTreeNode *,std::string> index = get_plot_args( list, args );
	std::ostringstream plot;		// Plot command collected here.

	_gnuplot.push_back( print_node( "set datafile separator \",\"" ) );		/* Use CSV. */
	_gnuplot.push_back( print_node( "set title \"" + DOM::__document->getModelCommentString() + "\"" ) );
	_gnuplot.push_back( print_node( "#set output \"" + LQIO::Filename( DOM::__document->__input_file_name, "svg", "", "" )() + "\"" ) );
	_gnuplot.push_back( print_node( "#set terminal svg" ) );

	/* Go through the args, (x, y11, y12..., y21, y22...). */
	
	size_t x = 0;		/* Index of first x (and only) variable. */
	for ( std::vector<LQX::SyntaxTreeNode*>::const_iterator arg = args->begin(); arg != args->end(); ++arg ) {
	    const std::string& name = index.at(*arg);
	    if ( arg == args->begin() ) {
		/* X independent variable */

		x = _result_pos.at(name);
		_gnuplot.push_back( print_node( "set xlabel \"" + name + "\"" ) );
		const std::map<std::string,Spex::ComprehensionInfo>::const_iterator i = __comprehensions.find( name );
		/* Set range */
		if ( i != __comprehensions.end() ) {
		    _gnuplot.push_back( print_node( "set xtics " + std::to_string(i->second.getInit()) + "," + std::to_string(i->second.getStep()) ) );
		    _gnuplot.push_back( print_node( "set xrange [" + std::to_string(i->second.getInit()) + ":" + std::to_string(i->second.getTest()) + "]" ) );
		}

	    } else if ( has_array_var( name ) ) {
		/* X2 handling here */

		// Too many independent variables
		LQIO::input_error2( ADV_TOO_MANY_GNUPLOT_VARIABLES, name.c_str() );
		continue;		/* Ignore */

	    } else {

		/* Y dependent variable */
		const ObservationInfo * obs = findObservation( name );
		int key = 0;
		std::string key_name;
		size_t delta = 0;	/* Index of delta (for interval) */
		
		/* If name is an observation, use obs type as Y label.  Otherwise punt. */
		if ( obs != nullptr ) {

		    /*
		     * If there is a confidence interval, and it was on the argument list, then add it as a result
		     * variable (goto done), but don't run the plot code.  When the primary variable is processed
		     * and the observation has a confidence interval, plot both at the same time by setting 'delta'.
		     */
		
		    const std::string& conf_name = obs->getConfVariableName();
		    if ( !conf_name.empty() ) {
			if ( conf_name == name ) continue;	/* Handled by main result 	*/
			const std::map<std::string,size_t>::const_iterator i = _result_pos.find(conf_name);
			if ( i != _result_pos.end() ) {
			    delta = i->second;			/* set position of conf int.	*/
			}
		    }
		    key = obs->getKey();
		    key_name = obs->getKeyName();
		} else {
		    key_name = name;
		}

		if ( y1_vars == 0 ) {
		    y1_obs_key = key;
		    _gnuplot.push_back( print_node( "set ylabel \"" + key_name + "\"" ) );
		    y1_vars += 1;
		    plot << "plot ";
		} else if ( y1_vars > 0 && y1_obs_key == key ) {
		    plot << ", ";
		    y1_vars += 1;
		} else if ( y2_vars == 0 ) {
		    y2_obs_key = key;
		    _gnuplot.push_back( print_node( "set y2label \"" + key_name + "\"" ) );
		    _gnuplot.push_back( print_node( "set y2tics" ) );
		    _gnuplot.push_back( print_node( "set key top left" ) );
		    _gnuplot.push_back( print_node( "set key box" ) );
		    plot << ", ";
		    y2_vars += 1;
		} else if ( y2_vars > 0 && y2_obs_key == key ) {
		    plot << ", ";
		    y2_vars += 1;
 		} else {
		    // Too many dependent variable types.
		    LQIO::input_error2( ADV_TOO_MANY_GNUPLOT_VARIABLES, name.c_str() );
		    continue;
		}
		const size_t y = _result_pos.at(name);		/* Index of this y variable 	*/

		/* Append plot command to plot */
		plot << "\"$DATA\" using " << x << ":" << y;
		if ( delta == 0 ) {
		    plot << " with linespoints";
		} else {
		    plot << ":" << delta << " with yerrorlines";	/* Confidence interval */
		}

		if ( y2_vars > 0 ) {
		    plot << " axes x1y2";
		}
		plot << " title \"" << name << "\"";
	    }
	}

	/* Append the plot command to the program (plot has to be near the end) */
	_gnuplot.push_back( print_node( plot.str() ) );
	return list;
    }


    expr_list *
    Spex::splot( expr_list * args )
    {
	expr_list * list = new expr_list;
	size_t z_vars = 0;			// Dependent variable count (y2 axis)
	int z_obs_key = 0;			// Dependent (y2 axis) type.
	const std::map<const LQX::SyntaxTreeNode *,std::string> index = get_plot_args( list, args );
	std::ostringstream splot;		// Plot command collected here.

	const std::string comment = "set title \"" + DOM::__document->getModelCommentString() + "\"";
	_gnuplot.push_back( print_node( comment ) );

	/* Go through the args, (x, y, z1, z2, z3..). */
	
	size_t count = 1;
	size_t x = 0;		/* Index of first x variable. */
	size_t y = 0;		/* Index of first y variable. */
	for ( std::vector<LQX::SyntaxTreeNode*>::const_iterator arg = args->begin(); arg != args->end(); ++arg, ++count ) {
	    const std::string& name = index.at(*arg);

	    if ( arg == args->begin() ) {
		/* X independent variable */

		x = _result_pos.at( name );
		_gnuplot.push_back( print_node( "set xlabel \"" + name + "\"" ) );
		const std::map<std::string,Spex::ComprehensionInfo>::const_iterator i = __comprehensions.find( name );
		/* Set range */
		if ( i != __comprehensions.end() ) {
		    _gnuplot.push_back( print_node( "set xtics " + std::to_string(i->second.getInit()) + "," + std::to_string(i->second.getStep()) ) );
		    _gnuplot.push_back( print_node( "set xrange [" + std::to_string(i->second.getInit()) + ":" + std::to_string(i->second.getTest()) + "]" ) );
		}

	    } else if ( has_array_var( name ) ) {
		/* Y independent variable */

		if ( y == 0 ) {
		    y = _result_pos.at( name );
		    _gnuplot.push_back( print_node( "set ylabel \"" + name + "\"" ) );
		    const std::map<std::string,Spex::ComprehensionInfo>::const_iterator i = __comprehensions.find( name );
		    /* Set range */
		    if ( i != __comprehensions.end() ) {
			_gnuplot.push_back( print_node( "set ytics " + std::to_string(i->second.getInit()) + "," + std::to_string(i->second.getStep()) ) );
			_gnuplot.push_back( print_node( "set yrange [" + std::to_string(i->second.getInit()) + ":" + std::to_string(i->second.getTest()) + "]" ) );
		    }

		} else {
		// Too many independent variables
		    LQIO::input_error2( ADV_TOO_MANY_GNUPLOT_VARIABLES, name.c_str() );
		    continue;		/* Ignore */
		}

	    } else {

		/* Z dependent variable */
		const ObservationInfo * obs = findObservation( name );
		int key = 0;
		std::string key_name;

		/* If name is an observation, keep it, otherwise make a fake name */
		if ( obs != nullptr ) {
		    key = obs->getKey();
		    key_name = obs->getKeyName();
		} else {
		    key_name = name;
		}

		if ( z_vars == 0 ) {
		    z_obs_key = key;
		    _gnuplot.push_back( print_node( "set zlabel \"" + key_name + "\"" ) );
		    z_vars += 1;
		    splot << "splot ";
		} else if ( z_vars > 0 && (z_obs_key == key) ) {
		    splot << ", ";
		    z_vars += 1;
 		} else {
		    // Too many dependent variable types.
		    LQIO::input_error2( ADV_TOO_MANY_GNUPLOT_VARIABLES, name.c_str() );
		    continue;
		}

		/* Append plot command to plot */
		const size_t z = _result_pos.at(name);
		splot << "\"$DATA\" using " << x << ":" << y << ":" << z << " with linespoints";
		splot << " title \"" << name << "\"";
	    }
	}

	/* Append the plot command to the program (plot has to be near the end) */
	_gnuplot.push_back( print_node( "set datafile separator \",\"" ) );		/* Use CSV. */
	_gnuplot.push_back( print_node( splot.str() ) );
	return list;
    }

    /* 
     * Collect the position for all args.  Generate a name for each arg 
     */
    
    std::map<const LQX::SyntaxTreeNode *,std::string> Spex::get_plot_args( expr_list * list, expr_list * args )
    {
	std::map<const LQX::SyntaxTreeNode *,std::string>  index;
	for ( std::vector<LQX::SyntaxTreeNode*>::const_iterator arg = args->begin(); arg != args->end(); ++arg ) {
	    std::ostringstream expr;
	    (*arg)->print(expr);			/* convert the expression to a string */
	    std::string name = expr.str();		/* See if it can be converted to a variable */
	    if ( dynamic_cast<const LQX::VariableExpression *>(*arg) != nullptr && name[0] != '$' ) {
		name.insert( 0, "$" );			/* Convert to external variable */
	    }
	    index.emplace( *arg, name );
	    _result_pos.emplace( name, _result_pos.size() + 1 );	/* GNUPlot starts at 1. */

	    /* Save variable as a result variable */
	    LQIO::Spex::__result_variables.push_back( LQIO::Spex::var_name_and_expr(name,*arg) );		/* Save variable name for printing */
	    list->push_back( *arg );	    /* Convert to locals */
	}
	
	return index;
    }

    /*
     * Convert the null terminated list of arguments into an expression list.
     */

    expr_list * Spex::make_list( LQX::SyntaxTreeNode* arg1, ... )
    {
	expr_list * list = new expr_list;
	va_list arguments;		       // A place to store the list of arguments

	va_start ( arguments, arg1 );		// Initializing arguments to store all values after num
	for ( LQX::SyntaxTreeNode* arg = arg1; arg != nullptr; arg = va_arg( arguments, LQX::SyntaxTreeNode* ) ) {
	    list->push_back( arg );
	}
	va_end( arguments );
	return list;
    }


    LQX::SyntaxTreeNode * Spex::print_node( const std::string& s )
    {
	return new LQX::FilePrintStatementNode( make_list( new LQX::ConstantValueExpression( s ), nullptr ), true, false );
    }

    /*
     * Find the observation matching name.
     */

    Spex::ObservationInfo * Spex::findObservation( const std::string& name )
    {
	for ( obs_var_tab_t::iterator i = __observations.begin(); i != __observations.end(); ++i ) {
	    ObservationInfo& obs = i->second;
	    if ( obs.getVariableName() == name || obs.getConfVariableName() == name ) {
		return &obs;
	    }
	}
	return nullptr;
    }

    std::vector<Spex::var_name_and_expr>::const_iterator Spex::find( std::vector<var_name_and_expr>::const_iterator begin, std::vector<var_name_and_expr>::const_iterator end,
							  const std::string& name )
    {
	for ( ; begin != end; ++begin ) {
	    if ( begin->first == name ) break;
	}
	return begin;
    }

    /* ------------------------------------------------------------------------ */
    /* lqn 'input' output functions for printing spex stuff out in input file.	*/
    /* ------------------------------------------------------------------------ */

    std::ostream&
    Spex::printInputVariable( std::ostream& output, const Spex::var_name_and_expr& var )
    {
	std::map<std::string,Spex::ComprehensionInfo>::const_iterator comp = __comprehensions.find(var.first);
	if ( comp != __comprehensions.end() ) {
	    output << var.first << " = " << comp->second;
	} else {
	    std::map<std::string,std::string>::const_iterator iter_var = __input_iterator.find(var.first);
	    if ( iter_var != __input_iterator.end() ) {		/* For $iter, $var = <expr> */
		output << iter_var->second << ", ";
	    }
	    output << var.first << " = " << *(var.second);
	}
	return output;
    }

    std::ostream&
    Spex::printInputArrayVariable( std::ostream& output, const Spex::var_name_and_expr& var )
    {
	std::map<std::string,Spex::ComprehensionInfo>::const_iterator comp = __comprehensions.find(var.first);
	if ( comp != __comprehensions.end() && comp->second.getStep() > 0. ) {
	    output << var.first;
	}
	return output;
    }

    std::ostream&
    Spex::printResultVariable( std::ostream& output, const Spex::var_name_and_expr& var )
    {
	output << var.first;
	if ( var.second != nullptr ) {
	    output << " = " << *(var.second);
	}
	return output;
    }

    /* ------------------------------------------------------------------------ */

    class Spex spex;

    std::vector<std::string> Spex::__array_variables;			    /* Saves $<array_name> for generating nest for loops */
    std::set<std::string> Spex::__array_references;			    /* Saves $<array_name> when used as an lvalue */
    std::vector<Spex::var_name_and_expr> Spex::__result_variables;	    /* Saves $<name> for printing the header of variable names */
    std::vector<std::string> Spex::__convergence_variables;		    /* Saves $<name> for all variables used in convergence section */
    std::map<std::string,LQX::SyntaxTreeNode *> Spex::__observation_variables;	/* Saves all observations (name, and funky assignment) */
    std::map<std::string,Spex::ComprehensionInfo> Spex::__comprehensions;   /* Saves all comprehensions for $<name> */
    expr_list Spex::__deferred_assignment;

    std::map<std::string, DOM::SymbolExternalVariable*>* Spex::__global_variables = nullptr;	/* Document global variables. (input) */
    std::map<std::string,LQX::SyntaxTreeNode *> Spex::__input_variables;    /* Save for printing when __verbose == true */
    Spex::obs_var_tab_t Spex::__observations;				    /* Saves all key-$var for each object */
    std::vector<Spex::ObservationInfo> Spex::__document_variables;	    /* Saves all key-$var for the document */
    std::map<std::string,std::string> Spex::__input_iterator;		    /* Saves iterator for x, y = expr statements */

    std::map<const DOM::ExternalVariable *,const LQX::SyntaxTreeNode *> Spex::__inline_expression;  /* Maps temp vars to expressions */

    bool Spex::__verbose = false;
    bool Spex::__no_header = false;

    const char * Spex::__convergence_limit_str = "$convergence_limit";

    /*+ JSON */
    void * Spex::__parameter_list = nullptr;
    void * Spex::__result_list = nullptr;
    void * Spex::__convergence_list = nullptr;
    void * Spex::__temp_variable = nullptr;
    /*- JSON */

    const std::map<const std::string,const Spex::attribute_table_t> Spex::__control_parameters =
    {
	{ "$convergence_iters",		    Spex::attribute_table_t( DOM::Document::XSpexIterationLimit ) },
	{ "$convergence_under_relax",	    Spex::attribute_table_t( DOM::Document::XSpexUnderrelaxation ) },
	{ Spex::__convergence_limit_str,    Spex::attribute_table_t( &DOM::Document::setModelConvergence ) },
	{ "$iteration_limit",		    Spex::attribute_table_t( &DOM::Document::setModelIterationLimit ) },
	{ "$model_comment",		    Spex::attribute_table_t( DOM::Document::XComment ) },
	{ "$print_interval",		    Spex::attribute_table_t( &DOM::Document::setModelPrintInterval ) },
	{ "$underrelaxation",		    Spex::attribute_table_t( &DOM::Document::setModelUnderrelaxationCoefficient ) }
    };


    /* Maps srvn_gram.h KEY_XXX to name.  First is the SPEX "key", second is the name. */
    const std::map<const int,const std::pair<const std::string,const std::string> > Spex::__key_code_map =
    {
	{ KEY_ELAPSED_TIME,		{ "time","Elapsed Time" } },
	{ KEY_EXCEEDED_TIME,		{ "x","Pr" } },
	{ KEY_ITERATIONS,		{ "i","Iterations" } },
	{ KEY_PROCESSOR_UTILIZATION,	{ "pu","Utilization" } },
	{ KEY_PROCESSOR_WAITING,	{ "pw","Waiting Time" } },
	{ KEY_SERVICE_TIME,		{ "s","Service Time" } },
	{ KEY_SYSTEM_TIME,		{ "sys","System Time" } },
	{ KEY_THROUGHPUT,		{ "f","Throughput" } },
	{ KEY_THROUGHPUT_BOUND,		{ "fb","Throughput Bound" } },
	{ KEY_USER_TIME,		{ "usr","User Time"  } },
	{ KEY_UTILIZATION,		{ "u","Utilization" } },
	{ KEY_VARIANCE,			{ "v","Variance" } },
	{ KEY_WAITING,			{ "w","Waiting Time" } },
	{ KEY_WAITING_VARIANCE,		{ "wv","Waiting Time Variance" } }
    };


    /* Maps srvn_gram.h KEY_XXX to lqx function name */
    const std::map<const int,const std::string> Spex::__key_lqx_function_map =
    {
	{ KEY_ELAPSED_TIME,		__lqx_elapsed_time },
	{ KEY_EXCEEDED_TIME,		__lqx_pr_exceeded },
	{ KEY_ITERATIONS,		__lqx_iterations },
	{ KEY_PROCESSOR_UTILIZATION,	__lqx_processor_utilization },
	{ KEY_PROCESSOR_WAITING,	__lqx_processor_waiting },
	{ KEY_SERVICE_TIME,		__lqx_service_time },
	{ KEY_SYSTEM_TIME,		__lqx_system_time },
	{ KEY_THROUGHPUT,		__lqx_throughput },
	{ KEY_THROUGHPUT_BOUND,		__lqx_throughput_bound },
	{ KEY_USER_TIME,		__lqx_user_time },
	{ KEY_UTILIZATION,		__lqx_utilization },
	{ KEY_VARIANCE,			__lqx_variance },
	{ KEY_WAITING,			__lqx_waiting },
	{ KEY_WAITING_VARIANCE,		__lqx_waiting_variance },
    };
}

namespace LQIO {
    Spex::ComprehensionInfo::ComprehensionInfo( double init, double test, double step )
	: _init(init), _test(test), _step(step)
    {
    }


    LQX::SyntaxTreeNode *
    Spex::ComprehensionInfo::init( const std::string& name ) const
    {
	const bool is_external = (name.size() > 1 && name[0] == '$');
	return new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, is_external ), new LQX::ConstantValueExpression( _init ) );
    }

    LQX::SyntaxTreeNode *
    Spex::ComprehensionInfo::test( const std::string& name ) const
    {
	const bool is_external = (name.size() > 1 && name[0] == '$');
	return new LQX::ComparisonExpression( LQX::ComparisonExpression::LESS_OR_EQUAL,
					      new LQX::VariableExpression( name, is_external ),
					      new LQX::ConstantValueExpression( _test ) );
    }

    LQX::SyntaxTreeNode *
    Spex::ComprehensionInfo::step( const std::string& name ) const
    {	
	const bool is_external = (name.size() > 1 && name[0] == '$');
	return new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, is_external ),
						 new LQX::MathExpression( LQX::MathExpression::ADD,
									  new LQX::VariableExpression( name, is_external ),
									  new LQX::ConstantValueExpression( _step ) ) );
    }

    std::ostream&
    Spex::ComprehensionInfo::print( std::ostream& output ) const
    {
	output << "[" << _init << ":" << _test << "," << _step << "]";
	return output;
    }
}

namespace LQIO {

    LQX::SyntaxTreeNode * Spex::attribute_table_t::operator()( const std::string& name ) const
    {
	switch ( _t ) {
	case attribute::IS_EXTVAR:
	    (DOM::__document->*_f.a_extvar)( DOM::__document->db_build_parameter_variable( name.c_str(), nullptr ) );
	    return new LQX::VariableExpression(name.c_str(),true);

	case attribute::IS_PROPERTY:
	    return new LQX::ObjectPropertyReadNode( new LQX::MethodInvocationExpression( "document" ), _f.a_string );
	    break;

	default:
	    abort();
	    break;
	}
    }
}

namespace LQIO {

    void Spex::PrintResultVariable::operator()( const var_name_and_expr& var ) const
    {
	if ( _indent ) {
	    _output << std::setw( _indent ) << " ";
	}
	_output << Spex::print_result_variable( var ) << std::endl;
    }

    Spex::ObservationInfo::ObservationInfo( int key, unsigned int phase, const char * variable_name, unsigned int conf_level, const char * conf_variable_name )
	: _key(key), _phase(phase), _variable_name(), _conf_level(conf_level), _conf_variable_name()
    {
	if ( variable_name != nullptr ) _variable_name = variable_name;
	if ( conf_level != 0 && conf_variable_name != nullptr ) _conf_variable_name = conf_variable_name;
    }

    Spex::ObservationInfo& Spex::ObservationInfo::operator=( const Spex::ObservationInfo& src )
    {
	*const_cast<int *>(&_key) = src._key;
	*const_cast<unsigned int *>(&_phase) = src._phase;
	_variable_name = src._variable_name;
	_conf_level = src._conf_level;
	_conf_variable_name = src._conf_variable_name;
	return *this;
    }

    bool
    Spex::ObservationInfo::operator()( const DOM::DocumentObject * o1, const DOM::DocumentObject * o2 ) const
    {
	return o1 == nullptr || (o2 != nullptr && o1->getSequenceNumber() < o2->getSequenceNumber());
    }

    bool
    Spex::ObservationInfo::operator()( const ObservationInfo& o1, const ObservationInfo& o2 ) const
    {
	return o1.getKey() < o2.getKey() || o1.getPhase() < o2.getPhase();
    }

    std::string Spex::ObservationInfo::getKeyCode() const
    {
	const std::map<const int,const std::pair<const std::string,const std::string> >::const_iterator ix = __key_code_map.find(getKey());
	std::string key_str = "%";
	if ( ix == __key_code_map.end() ) abort();

	key_str += ix->second.first;
	switch ( getPhase() ) {
	case 1: key_str += "1"; break;
	case 2: key_str += "2"; break;
	case 3: key_str += "3"; break;
	}
	return key_str;
    }

    const std::string& Spex::ObservationInfo::getKeyName() const
    {
	const std::map<const int,const std::pair<const std::string,const std::string> >::const_iterator ix = __key_code_map.find(getKey());
	if ( ix == __key_code_map.end() ) abort();

	return ix->second.second;
    }

    Spex::ObservationInfo&
    Spex::ObservationInfo::setVariableName( const std::string& variable_name )
    {
	if ( variable_name == "" || variable_name[0] != '$' ) {
	    throw std::invalid_argument( variable_name );
	}
	_variable_name = variable_name;
	return *this;
    }
	
    Spex::ObservationInfo&
    Spex::ObservationInfo::setConfVariableName( const std::string& variable_name )
    {
	if ( variable_name == "" || variable_name[0] != '$' ) {
	    throw std::invalid_argument( variable_name );
	}
	_conf_variable_name = variable_name;
	return *this;
    }

    std::ostream&
    Spex::ObservationInfo::print( std::ostream& output ) const
    {
	output << "%" << __key_code_map.at(_key).first;
	if ( _phase != 0 ) {
	    output << _phase;
	}
	if ( _conf_level > 0 ) {
	    output << " " << _conf_level;
	}
	output << " " << _variable_name;
	if ( _conf_level > 0 ) {
	    output << " " << _conf_variable_name;
	}
	return output;
    }
}

/* ------------------------------------------------------------------------ */
/* Interface between the parser and the spex generator.			    */
/* ------------------------------------------------------------------------ */

/*
 * Create a program.  Param_arg makes up the main line.	 mearsure_arg
 * is composed of all the %x type annotations for results.  result_arg
 * is composed of the result section.  measure and result are tacked
 * on after the solve() function to produce output.
 */

void spex_set_program( void * param_arg, void * result_arg, void * convergence_arg )
{
    if ( !LQIO::spex.has_vars() ) return;	/* Nothing to do */
    expr_list * program;
    if ( param_arg == nullptr ) {
	program = new expr_list;
    } else {
	program = static_cast<expr_list *>(param_arg);
    }
	
    if ( LQIO::spex.__observations.empty() ) {
	LQIO::solution_error( LQIO::WRN_NO_SPEX_OBSERVATIONS );
    }

    /* Handle this here -- can't change after program is compiled */
    if ( !LQIO::Spex::__no_header ) {
	std::string header = LQIO::DOM::__document->getPragma( "spex-header" );
	LQIO::Spex::__no_header = !LQIO::DOM::Pragma::isTrue(header);
    }

    if ( LQIO::spex.construct_program( program,
				       static_cast<expr_list *>(result_arg),
				       static_cast<expr_list *>(convergence_arg),
				       &LQIO::spex._gnuplot ) ) {
	LQIO::DOM::__document->setLQXProgram( LQX::Program::loadRawProgram( program ) );
    }
}

void spex_set_parameter_list( void * param_arg )
{
    LQIO::Spex::__parameter_list = param_arg;						/* JSON */
}

void spex_set_result_list( void * result_arg )
{
    LQIO::Spex::__result_list = result_arg;
}

void spex_set_convergence_list( void * convergence_arg )
{
    LQIO::Spex::__convergence_list = convergence_arg;
}

void spex_set_variable( void * variable )
{
    LQIO::Spex::__temp_variable = variable;
}


/*
 * Create a Globally-scoped variable.  Note that these must all be assigned prior to calling solve().
 * Parameter variables which are not constant are "deferred" and will be assigned while the spex program runs.
 */

void * spex_assignment_statement( const char * name, void * expr, const bool constant_expression )
{
    if ( LQIO::Spex::has_input_var( name ) || LQIO::Spex::has_observation_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return nullptr;
    }

    LQIO::Spex::__input_variables[std::string(name)] = static_cast<LQX::SyntaxTreeNode *>(expr);		/* Save variable for printing */

    LQX::VariableExpression * var = 0;
    /* check expr for array_create (fugly!) */
    std::ostringstream ss;
    static_cast<LQX::SyntaxTreeNode *>(expr)->print(ss,0);
    if ( strncmp( "array_create", ss.str().c_str(), 12 ) == 0 ) {
	LQIO::Spex::__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
	std::string local = name;
	local[0] = '_';
	var = new LQX::VariableExpression(local,false);
    } else {
	var = new LQX::VariableExpression(&name[1],false);
    }
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( var, static_cast<LQX::SyntaxTreeNode *>(expr) );

    if ( constant_expression ) {
	return statement;
    } else {
	LQIO::Spex::__deferred_assignment.push_back( statement );
	return nullptr;
    }
}


/*
 * Create and array.  Funky stuff:
 *  1) Create a local variable with the array definition.  Simply strip off the $.
 *  2) Stash the parameter variable ($var) so that we can iterate over it when we "create" the program.
 *  Note: the 'name' parameter of "$var" is converted to "_var" internally and treated as a local variable.
 */

void * spex_array_assignment( const char * name, void * list, const bool constant_expression )
{
    if ( LQIO::Spex::has_input_var( name ) || LQIO::Spex::has_observation_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return nullptr;
    } else if ( !constant_expression ) {
	LQIO::input_error( "Arrays must contain constants only.\n" );
	return nullptr;
    }

    LQIO::Spex::__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
    LQX::SyntaxTreeNode * array = new LQX::MethodInvocationExpression("array_create", static_cast<expr_list *>(list) );
    std::string local = name;
    local[0] = '_';
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression( local, false ), array );
    LQIO::Spex::__input_variables[std::string(name)] =	array;
    return statement;
}

/*
 * This creates for loops.
 */

void * spex_array_comprehension( const char * name, double begin, double end, double stride )
{
    if ( LQIO::Spex::has_input_var( name ) || LQIO::Spex::has_observation_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return nullptr;
    }

    LQIO::Spex::__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
    LQIO::Spex::__comprehensions[name] = LQIO::Spex::ComprehensionInfo( begin, end, stride );
    LQIO::Spex::__input_variables[name] = nullptr;
    return nullptr;
}


void * spex_forall( const char * iter_name, const char * name, void * expr )
{
    if ( LQIO::Spex::has_input_var( name ) || LQIO::Spex::has_observation_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return nullptr;
    }

    LQIO::Spex::__input_variables[name] = static_cast<LQX::SyntaxTreeNode *>(expr);		/* Save variable for printing */
    LQIO::Spex::__input_iterator[name] = iter_name;
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ),
									static_cast<LQX::SyntaxTreeNode *>(expr) );
    LQIO::Spex::__deferred_assignment.push_back( statement );
    return nullptr;
}

/* ------------------------------------------------------------------------ */

void * spex_ternary( void * arg1, void * arg2, void * arg3 )
{
    return new LQX::ConditionalStatementNode(static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2), static_cast<LQX::SyntaxTreeNode *>(arg3));
}

void * spex_add( void * arg1, void * arg2 )
{
    return new LQX::MathExpression(LQX::MathExpression::ADD, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2));
}

void * spex_subtract( void * arg1, void * arg2 )
{
    return new LQX::MathExpression(LQX::MathExpression::SUBTRACT, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2));
}

void * spex_multiply( void * arg1, void * arg2 )
{
    return new LQX::MathExpression(LQX::MathExpression::MULTIPLY, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2));
}

void * spex_divide( void * arg1, void * arg2 )
{
    return new LQX::MathExpression(LQX::MathExpression::DIVIDE, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2));
}

void * spex_modulus( void * arg1, void * arg2 )
{
    return new LQX::MathExpression(LQX::MathExpression::MODULUS, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2));
}

void * spex_power( void * arg1, void * arg2 )
{
    return new LQX::MethodInvocationExpression("pow", static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2), NULL);
}

void * spex_or( void * arg1, void * arg3 )
{
    return new LQX::LogicExpression(LQX::LogicExpression::OR, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_and( void * arg1, void * arg3 )
{
    return new LQX::LogicExpression(LQX::LogicExpression::AND, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_equals( void * arg1, void * arg3 )
{
    return new LQX::ComparisonExpression(LQX::ComparisonExpression::EQUALS, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_not_equals( void * arg1, void * arg3 )
{
    return new LQX::ComparisonExpression(LQX::ComparisonExpression::NOT_EQUALS, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_less_than( void * arg1, void * arg3 )
{
    return new LQX::ComparisonExpression(LQX::ComparisonExpression::LESS_THAN, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_less_than_or_equals( void * arg1, void * arg3 )
{
    return new LQX::ComparisonExpression(LQX::ComparisonExpression::LESS_OR_EQUAL, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_greater_than( void * arg1, void * arg3 )
{
    return new LQX::ComparisonExpression(LQX::ComparisonExpression::GREATER_THAN, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_greater_than_or_equals( void * arg1, void * arg3 )
{
    return new LQX::ComparisonExpression(LQX::ComparisonExpression::GREATER_OR_EQUAL, static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg3) );
}

void * spex_not( void * arg2 )
{
    return new LQX::LogicExpression(LQX::LogicExpression::NOT, static_cast<LQX::SyntaxTreeNode *>(arg2), nullptr );
}

/*
 * Array references.  Arg1 is the name (as a variable), arg3 is the index.
 */

void * spex_array_reference( void * arg1, void * arg3 )
{
    LQX::SyntaxTreeNode * var = static_cast<LQX::SyntaxTreeNode *>(arg1);

    /* Is arg1 a variable name (otherwise it's an array...) */

    if ( dynamic_cast<LQX::VariableExpression *>(var) != nullptr ) {
	std::ostringstream ss;		/* get <name> */
	ss << "$";			/* Regenerate <$name> */
	var->print(ss,0);
	std::string name = ss.str();
	LQIO::Spex::__array_references.insert( name );

	name[0] = '_';			/* Convert to local <_name> */
	arg1 = new LQX::VariableExpression( name, false );
    }

    std::vector<LQX::SyntaxTreeNode *> * list = new std::vector<LQX::SyntaxTreeNode*>();
    list->push_back( static_cast<LQX::SyntaxTreeNode*>(arg1) );
    list->push_back( static_cast<LQX::SyntaxTreeNode*>(arg3) );
    return new LQX::MethodInvocationExpression( "array_get", list );
}

/*
 * Intercept plot and splot here.
 */

void * spex_invoke_function( const char * s, void * arg )
{
    if ( arg ) {
	return new LQX::MethodInvocationExpression( s, static_cast<expr_list *>(arg) );
    } else {
	return new LQX::MethodInvocationExpression( s );
    }
}


/*
 * We need to create a temporary variable, assign "expr" to this variable, then ensure that "expr" is run "later" in the deferred assignment phase.
 */

void * spex_inline_expression( void * expr )
{
    std::ostringstream name;
    name << "$_" << std::setw(3) << std::setfill('0') << LQIO::Spex::__inline_expression.size() << "_" << std::setfill(' ');
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(name.str(),true), static_cast<LQX::SyntaxTreeNode *>(expr) );
    LQIO::Spex::__deferred_assignment.push_back( statement );
    LQIO::DOM::ExternalVariable * var = LQIO::DOM::__document->db_build_parameter_variable(name.str().c_str(),nullptr);
    LQIO::Spex::__inline_expression.insert( std::pair<const LQIO::DOM::ExternalVariable *,const LQX::SyntaxTreeNode*>(var,static_cast<LQX::SyntaxTreeNode *>(expr)) );
    return var;
}


void * spex_document_observation( const int key, const char * var )
{
    if ( !var ) return nullptr;
    return LQIO::spex.observation( LQIO::Spex::ObservationInfo( key, 0, var ) );
}

void * spex_processor_observation( const void * obj, const int key, const int conf, const char * var, const char * var2 )
{
    if ( !obj || !var ) return nullptr;

    return LQIO::spex.observation( static_cast<const LQIO::DOM::Processor *>(obj), LQIO::Spex::ObservationInfo( key, 0, var, conf, var2 ) );
}


void * spex_group_observation( const void * obj, const int key, const int conf, const char * var, const char * var2 )
{
    if ( !obj || !var ) return nullptr;

    return LQIO::spex.observation( static_cast<const LQIO::DOM::Group *>(obj), LQIO::Spex::ObservationInfo( key, 0, var, conf, var2 ) );
}


void * spex_task_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    if ( !obj || !var ) return nullptr;

    return LQIO::spex.observation( static_cast<const LQIO::DOM::Task *>(obj), LQIO::Spex::ObservationInfo( key, phase, var, conf, var2 ) );
}

void * spex_entry_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    if ( !obj || !var ) return nullptr;

    const LQIO::DOM::Entry * entry = static_cast<const LQIO::DOM::Entry *>(obj);

    if ( ((0 < phase && phase <= (int)LQIO::DOM::Phase::MAX_PHASE) && ( key == KEY_SERVICE_TIME || key == KEY_UTILIZATION  || key == KEY_PROCESSOR_UTILIZATION	|| key == KEY_PROCESSOR_WAITING	 || key == KEY_VARIANCE || KEY_EXCEEDED_TIME ))
	 || (phase == 0 && (key == KEY_THROUGHPUT || key == KEY_THROUGHPUT_BOUND || key == KEY_UTILIZATION || key == KEY_PROCESSOR_UTILIZATION || key == KEY_WAITING)) ) {
	/* This is by phase... but LQX needs entry and phase name */
	return LQIO::spex.observation( entry, LQIO::Spex::ObservationInfo( key, phase, var, conf, var2 ) );
    } else {
	input_error2( LQIO::WRN_INVALID_SPEX_RESULT_PHASE, phase, LQIO::Spex::__key_code_map.at(key).first.c_str(), entry->getName().c_str() );
	return nullptr;
    }

}

void * spex_activity_observation( const void * task, const void * activity, const int key, const int conf, const char * var, const char * var2 )
{
    if ( !task || !activity || !var ) return nullptr;

    return LQIO::spex.observation( static_cast<const LQIO::DOM::Task *>(task), static_cast<const LQIO::DOM::Activity *>(activity), LQIO::Spex::ObservationInfo( key, 0, var, conf, var2 ) );
}


void * spex_call_observation( const void * src, const int key, const int phase, const void * dst, const int conf, const char * var, const char * var2 )
{
    if ( !src || !dst || !var ) return nullptr;

    const LQIO::DOM::Entry * src_entry = static_cast<const LQIO::DOM::Entry *>(src);
    const LQIO::DOM::Entry * dst_entry = static_cast<const LQIO::DOM::Entry *>(dst);

    if ( phase <= 0 || (int)LQIO::DOM::Phase::MAX_PHASE < phase ) {
	input_error2( LQIO::WRN_INVALID_SPEX_RESULT_PHASE, phase, LQIO::Spex::__key_code_map.at(key).first.c_str(), src_entry->getName().c_str() );
	return nullptr;
    } else {
	return LQIO::spex.observation( src_entry, phase, dst_entry, LQIO::Spex::ObservationInfo( key, phase, var, conf, var2 ) );
    }

}

void * spex_activity_call_observation( const void * task, const void * activity, const int key, const void * dst, const int conf, const char * var, const char * var2 )
{
    if ( !task || !activity || !dst || !var ) return nullptr;

    return LQIO::spex.observation( static_cast<const LQIO::DOM::Task *>(task), static_cast<const LQIO::DOM::Activity *>(activity),
					static_cast<const LQIO::DOM::Entry *>(dst), LQIO::Spex::ObservationInfo( key, 0, var, conf, var2 ) );
}

/*
 * This is a touch tricky.
 * Some variables are globals and are used as parameters (they should be in the variable database for the doc).
 * Result variables are local even though they have a $, otherwise the solve() function won't run.
 * Local and Global variables have different scope... doh... so we have to figure out what is what.
 */

void * spex_result_assignment_statement( const char * name, void * expr )
{
    std::string var_name;
    if ( name != nullptr ) {
	var_name = name;
	LQIO::Spex::__result_variables.push_back( LQIO::Spex::var_name_and_expr(var_name,static_cast<LQX::SyntaxTreeNode *>(expr)) );		/* Save variable name for printing */
	LQX::VariableExpression * variable = static_cast<LQX::VariableExpression * >(spex_get_symbol( var_name.c_str() ));
	if ( expr ) {
	    return new LQX::AssignmentStatementNode( variable, static_cast<LQX::SyntaxTreeNode *>(expr) );
	} else { 
	    return variable;
	}
    } else if ( dynamic_cast<const LQX::VariableExpression *>(static_cast<LQX::SyntaxTreeNode *>(expr) ) ) {
	std::ostringstream ss;
	const LQX::SyntaxTreeNode * node = static_cast<LQX::SyntaxTreeNode *>(expr);
	node->print( ss );
	var_name = ss.str();
	if ( dynamic_cast<const LQX::VariableExpression *>(node) != nullptr && var_name[0] != '$' ) {
	    var_name.insert( 0, "$" );			/* Convert to external variable */
	}
	LQIO::Spex::__result_variables.push_back( LQIO::Spex::var_name_and_expr(var_name,nullptr) );		/* Save variable name for printing */
    }
    return expr;
}

/*
 * The args are a list of variables for printing.  Return an array with the args.  I also have to set up the gnuplot stuff.
 * Args is a list of variables.
 */

void * spex_result_function( const char * s, void * arg )
{
    if ( strcmp( s, "plot" ) == 0 ) {
	return LQIO::spex.plot( static_cast<expr_list *>( arg ) );
    } else if ( strcmp( s, "splot" ) == 0 ) {
	return LQIO::spex.splot( static_cast<expr_list *>( arg ) );
    } else {
	return nullptr;
    }
}


/*
 * I need to create a local version of each variable.  Delta them all
 */

void * spex_convergence_assignment_statement( const char * name, void * expr )
{
    LQIO::Spex::__convergence_variables.push_back( std::string(name) );		/* Save variable name for looping */
    return new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, true ), static_cast<LQX::SyntaxTreeNode *>(expr) );
}


/*
 * Append node to the list.  If the list does not exist, create it.  Ignore null nodes.
 */

void * spex_list( void * list_arg, void * node )
{
    std::vector<LQX::SyntaxTreeNode *> * list = static_cast<expr_list *>(list_arg);
    if ( list == nullptr ) {
	list = new std::vector<LQX::SyntaxTreeNode*>();
    }
    if ( node != nullptr ) {
	list->push_back(static_cast<LQX::SyntaxTreeNode*>(node));
    }
    return list;
}


void * spex_get_symbol( const char * name )
{
    const bool is_external = LQIO::Spex::is_global_var( name );
    if ( is_external || name[0] != '$' ) {
	return new LQX::VariableExpression( name, is_external );
    } else if ( !isdigit( name[1] ) ) {
	return new LQX::VariableExpression( &name[1], is_external );
    } else {
	std::string local = name;
	local[0] = '_';
	return new LQX::VariableExpression( local, is_external );
    }
}

void * spex_get_real( double arg )
{
    return new LQX::ConstantValueExpression( arg );
}


void * spex_get_string( const char * arg )
{
    return new LQX::ConstantValueExpression( arg );
}
