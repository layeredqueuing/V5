/*
 *  $Id: srvn_spex.cpp 12594 2016-06-06 16:53:56Z greg $
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
#include <sstream>
#include <iomanip>
#include <lqx/SyntaxTree.h>
#include <lqx/Program.h>
#include "dom_document.h"
#include "dom_phase.h"
#include "srvn_input.h"
#include "srvn_spex.h"
#include "glblerr.h"

extern "C" {
#include "srvn_gram.h"
}

namespace LQIO {
    namespace DOM {

	Spex::Spex() 
	{
	}

	
	/* 
	 * Initialize the control parameters array.  Done at run time because it doesn't like being done in the constructor during program initialization.
	 */

	void Spex::initialize_control_parameters() 
	{
	    if ( LQIO::DOM::Spex::__control_parameters.size() != 0 ) return;
	    clear();

#if 0
	    __control_parameters["$convergence_iters"] 		= attribute_table_t( Document::XSpexIterationLimit );
	    __control_parameters["$convergence_under_relax"]	= attribute_table_t( Document::XSpexUnderrelaxation );
            __control_parameters["$block_time"]        		= attribute_table_t( Document::XSimulationBlockTime );
            __control_parameters[__convergence_limit_str] 	= attribute_table_t( &Document::setModelConvergence );
            __control_parameters["$iteration_limit"]   		= attribute_table_t( &Document::setModelIterationLimit );
            __control_parameters["$model_comment"]     		= attribute_table_t( Document::XComment );
            __control_parameters["$number_of_blocks"]  		= attribute_table_t( Document::XSimulationNumberOfBlocks );
            __control_parameters["$print_interval"]    		= attribute_table_t( &Document::setModelPrintInterval );
            __control_parameters["$result_precision"]  		= attribute_table_t( Document::XSimulationPrecision );
            __control_parameters["$seed_value"]        		= attribute_table_t( Document::XSimulationSeedValue );
            __control_parameters["$underrelaxation"]   		= attribute_table_t( &Document::setModelUnderrelaxationCoefficient );
            __control_parameters["$warm_up_loops"]     		= attribute_table_t( Document::XSimulationWarmUpLoops );
#endif

	    /* This should map srvn_scan.l */

	    __key_code_map[KEY_ELAPSED_TIME]		= "time";
	    __key_code_map[KEY_ITERATIONS]		= "i";
	    __key_code_map[KEY_PROCESSOR_UTILIZATION]	= "p";
	    __key_code_map[KEY_PROCESSOR_WAITING]	= "pw";
	    __key_code_map[KEY_SERVICE_TIME]		= "s";
	    __key_code_map[KEY_SYSTEM_TIME]		= "sys";
	    __key_code_map[KEY_THROUGHPUT]		= "f";
	    __key_code_map[KEY_THROUGHPUT_BOUND]	= "fb";
	    __key_code_map[KEY_USER_TIME]		= "usr";
	    __key_code_map[KEY_UTILIZATION]		= "u";
	    __key_code_map[KEY_VARIANCE]		= "v";
	    __key_code_map[KEY_WAITING]			= "w";
	    __key_code_map[KEY_WAITING_VARIANCE]	= "wv";

	    __key_lqx_function_map[KEY_ELAPSED_TIME]		= "elapsed_time";
	    __key_lqx_function_map[KEY_ITERATIONS]		= "iterations";
	    __key_lqx_function_map[KEY_PROCESSOR_UTILIZATION]	= "proc_utilization";
	    __key_lqx_function_map[KEY_PROCESSOR_WAITING]	= "proc_waiting";
	    __key_lqx_function_map[KEY_SERVICE_TIME]		= "service_time";
	    __key_lqx_function_map[KEY_SYSTEM_TIME]		= "system_cpu_time";
	    __key_lqx_function_map[KEY_THROUGHPUT]		= "throughput";
	    __key_lqx_function_map[KEY_THROUGHPUT_BOUND]	= "throughput_bound";
	    __key_lqx_function_map[KEY_USER_TIME]		= "user_cpu_time";
	    __key_lqx_function_map[KEY_UTILIZATION]		= "utilization";
	    __key_lqx_function_map[KEY_VARIANCE]		= "variance";
	    __key_lqx_function_map[KEY_WAITING]			= "waiting";
	    __key_lqx_function_map[KEY_WAITING_VARIANCE]	= "waiting_variance";
	}


	void Spex::clear()
	{
	    __array_variables.clear();				/* Saves $<array_name> for generating nest for loops */
	    __result_variables.clear();				/* Saves $<name> for printing the header of variable names */
	    __convergence_variables.clear();			/* Saves $<name> for all variables used in convergence section */
	    __observations.clear();
	    __comprehensions.clear();
	    __deferred_assignment.clear();
	    __input_variables.clear();				/* Saves input values per iteration */
	    __observation_variables.clear();			/* */
	    __document_variables.clear();			/* Saves all key-$var for the document */
	    __input_iterator.clear();
	    __result_expression.clear();
	    __inline_expression.clear();			/* Maps temp vars to expressions */
	}


	/* static */ bool 
	Spex::is_global_var( const std::string& name ) 
	{
	    return LQIO::DOM::currentDocument->hasSymbolExternalVariable( name );
	}

	bool Spex::has_input_var( const std::string& name )
	{
	    return LQIO::DOM::Spex::__input_variables.find( name ) != LQIO::DOM::Spex::__input_variables.end();
	}

	/*
	 * Used by lqn2lqx when outputting LQX from SPEX input
	 */
	
	LQX::SyntaxTreeNode * Spex::get_input_var_expr( const std::string& name )
	{
	    std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator iv_p = LQIO::DOM::Spex::__input_variables.find( name );
	    if ( iv_p != LQIO::DOM::Spex::__input_variables.end() ) {
		return iv_p->second;
	    } else {
		return NULL;
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

	bool Spex::construct_program( expr_list * main_line, expr_list * result, expr_list * convergence ) 
	{
	    /* If we have only set control variables, then nothing to do. */
	    if ( !has_vars() ) return false;		

#if 0
	    /* Add magic variable if not present */
	    if ( __convergence_variables.size() > 0 && !LQIO::DOM::currentDocument->hasSymbolExternalVariable( __convergence_limit_str ) ) {
		double convergence_value = LQIO::DOM::currentDocument->getModelConvergenceValue();
		main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( __convergence_limit_str, true ), new LQX::ConstantValueExpression( convergence_value ) ) );			/* Add $0 variable */
		LQIO::DOM::currentDocument->setModelConvergence( LQIO::DOM::currentDocument->db_build_parameter_variable( __convergence_limit_str, NULL ) );
	    }
#endif

	    /* Initialize loop counter variable */
	    main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "_0", false ), new LQX::ConstantValueExpression( 0.0 ) ) );			/* Add $0 variable */

	    /* Add observation variables */
	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::iterator obs_p = __observations.begin(); obs_p != __observations.end(); ++obs_p ) {
		const std::string& name = obs_p->first;
		main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, false ), new LQX::ConstantValueExpression( 0.0 ) ) );	/* Initialize all observations variables */
	    }

	    /* Add code for printing a header in the output file. */
	    if ( !__no_header ) {
		main_line->push_back( print_header() );
	    }

	    /* Add the code for running the SPEX program -- recursive call. */
	    main_line->push_back( foreach_loop( __array_variables.begin(), result, convergence ) );
	    return true;
	}

	/* 
	 * Figure out the destination (lvalue) for an assignment statement.  If it's a control variable, attribute_table_t::operator() will 
	 * determined if the destination is a variable or if it is a "field" for an ObjectPropertyReadNode (somewhat misnamed).  Otherwise, 
	 * the destination is simply a variable.
	 */

	LQX::SyntaxTreeNode * Spex::get_destination( const std::string& name ) const
	{
	    LQX::SyntaxTreeNode * destination;
	    std::map<std::string,LQIO::DOM::Spex::attribute_table_t>::const_iterator i = LQIO::DOM::Spex::__control_parameters.find( name );
	    if ( i != LQIO::DOM::Spex::__control_parameters.end() ) {
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
	
	LQX::SyntaxTreeNode * Spex::observation( const int key, const char * name ) 
	{
	    __document_variables.push_back( ObservationInfo( key, 0, name ) );
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "document" );
	    LQX::ObjectPropertyReadNode * node = 0;
	    switch ( key ) {
	    case KEY_WAITING:		node = new LQX::ObjectPropertyReadNode( object, "waits" ); break;
	    case KEY_ITERATIONS:	node = new LQX::ObjectPropertyReadNode( object, "iterations" ); break;
	    case KEY_ELAPSED_TIME:	node = new LQX::ObjectPropertyReadNode( object, "elapsed_time" ); break;
	    case KEY_USER_TIME:		node = new LQX::ObjectPropertyReadNode( object, "user_cpu_time" ); break;
	    case KEY_SYSTEM_TIME:	node = new LQX::ObjectPropertyReadNode( object, "system_cpu_time" ); break;
	    default:	abort();
	    }
	    __observations[&name[1]] = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), node );
	    return object;		/* For chaining */
	}

	/*
	 * Group, Processor, Task, Entry, Phase variables 
	 */
	
	LQX::SyntaxTreeNode * Spex::observation( const DocumentObject* document_object, const unsigned int phase, const std::string& type_name, ObservationInfo& obs ) 
	{
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( type_name, new LQX::ConstantValueExpression( document_object->getName() ), 0 );
	    if ( dynamic_cast<const Entry *>(document_object) != 0 && 0 < phase && phase <= LQIO::DOM::Phase::MAX_PHASE ) {
		/* A phase of an entry */
		object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(phase) ), 0 );
		document_object = dynamic_cast<const Entry *>(document_object)->getPhase( phase );
	    }
	    return observation( object, document_object, obs );
	}
    
	/* 
	 * Activity variables 
	 */
	
	LQX::SyntaxTreeNode * Spex::observation( const Task* task, const Activity * activity, const std::string& type_name, ObservationInfo& obs ) 
	{
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "task", new LQX::ConstantValueExpression( task->getName() ), 0 );
	    object = new LQX::MethodInvocationExpression( "activity", object, new LQX::ConstantValueExpression( activity->getName() ), 0 );
	    return observation( object, activity, obs );
	}
	
	/*
	 * Call Variables 
	 */
	
	LQX::SyntaxTreeNode * Spex::observation( const DocumentObject* src, const unsigned int phase, const Entry* dst, ObservationInfo& obs )
	{
	    assert( dynamic_cast<const LQIO::DOM::Entry *>(src) != 0 && phase != 0 );

	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "entry", new LQX::ConstantValueExpression( src->getName() ), 0 );
	    object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(phase) ), 0 );
	    const LQIO::DOM::Call * call_object = dynamic_cast<const LQIO::DOM::Entry *>(src)->getCallToTarget( dynamic_cast<const LQIO::DOM::Entry *>(dst), phase );
	    object = new LQX::MethodInvocationExpression( "call", object, new LQX::ConstantValueExpression( dst->getName() ), 0 );
	    return observation( object, call_object, obs );

	}

	/* 
	 * Activity Call variables 
	 */
	
	LQX::SyntaxTreeNode * Spex::observation( const Task* task, const Activity * activity, const Entry * dst, ObservationInfo& obs ) 
	{
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "task", new LQX::ConstantValueExpression( task->getName() ), 0 );
	    object = new LQX::MethodInvocationExpression( "activity", object, new LQX::ConstantValueExpression( activity->getName() ), 0 );
	    const LQIO::DOM::Call * call_object = activity->getCallToTarget( dst );
	    object = new LQX::MethodInvocationExpression( "call", object, new LQX::ConstantValueExpression( dst->getName() ), 0 );
	    return observation( object, call_object, obs );
	}
	
	/*
	 * Common code.
	 */
	
	LQX::SyntaxTreeNode * Spex::observation( LQX::MethodInvocationExpression * lqx_obj, const DocumentObject * doc_obj, ObservationInfo& obs )
	{
	    const char * name = obs.getVariableName().c_str();
	    const int key = obs.getKey();
	    const unsigned int conf_level = obs.getConfLevel();
	    const char * conf_name = obs.getConfVariableName().c_str();
	    __observation_variables.insert( std::pair<const DocumentObject*,ObservationInfo>(doc_obj,obs) );		/* Save variable names */
	    __observations[&name[1]] = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ),
									 new LQX::ObjectPropertyReadNode( lqx_obj, __key_lqx_function_map[key] ) );
	    if ( conf_level != 0 ) {
		lqx_obj = new LQX::MethodInvocationExpression( "conf_int", lqx_obj, new LQX::ConstantValueExpression( static_cast<double>(conf_level) ), 0 );
		__observations[&conf_name[1]] = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &conf_name[1], false ),
										  new LQX::ObjectPropertyReadNode( lqx_obj, __key_lqx_function_map[key] ) );
	    }
	    return lqx_obj;		/* For chaining */
	}
	
	/*
	 * foreach( i,$var in list );
	 * loop_stmt(X) ::= FOREACH OBRACKET IDENTIFIER(A) COMMA IDENTIFIER(B) IN expr(C) CBRACKET stmt(D). 
	 *                 { X = new ForeachStatementNode( A->getStoredIdentifier(), B->getStoredIdentifier(), A->getIsExternal(), B->getIsExternal(), C, D ); }
	 */

	LQX::SyntaxTreeNode* Spex::foreach_loop( std::vector<std::string>::const_iterator var_p, expr_list * result, expr_list * convergence ) const 
	{
	    if ( var_p != __array_variables.end() ) {
		std::string name = *var_p;	/* Make local copy because we force to local */
		name[0] = '_';			/* Array name */
		std::map<std::string,Spex::ComprehensionInfo>::const_iterator i = __comprehensions.find( *var_p );
		if ( i == __comprehensions.end() ) {
		    /* if we have $x = [...] */
		    return new LQX::ForeachStatementNode( "", &name[1], /* key ext */ false, /* val */ false, 
							  new LQX::VariableExpression( name, false ), 
							  foreach_loop( ++var_p, result, convergence ) );
		} else {
		    /* for ( i = begin; i < end; ++i ) 
		           $x = f(i); */
		    return new LQX::LoopStatementNode( i->second.init(&name[1]), i->second.test(&name[1]), i->second.step(&name[1]), foreach_loop( ++var_p, result, convergence ) );
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
														       0 ),
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
							      new LQX::LogicExpression(LQX::LogicExpression::NOT, new LQX::VariableExpression( "__done__", false ), NULL),
							      new LQX::CompoundStatementNode( convergence ),
							      new LQX::CompoundStatementNode( loop_body( result ) ) ) );
#else 
	    /* Stick convergence stuff at end of loop body */
	    expr_list * body = loop_body( result );
	    body->insert( body->end(), convergence->begin(), convergence->end() );
	    loop_code->push_back( new LQX::LoopStatementNode( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "__done__", false ), new LQX::ConstantValueExpression( false ) ),
							      new LQX::LogicExpression(LQX::LogicExpression::NOT, new LQX::VariableExpression( "__done__", false ), NULL),
							      0,
							      new LQX::CompoundStatementNode( body ) ) );
#endif
	    return loop_code;
	}

	expr_list * Spex::loop_body( expr_list * result ) const
	{
	    expr_list * loop_code = new expr_list;

	    /* load in all deferred assignment statements, eg '$x = $loop'... */
	    *loop_code = Spex::__deferred_assignment;

	    /* "Assign" any array variables which are used as parameters */
	    for ( std::vector<std::string>::const_iterator var_p = __array_variables.begin(); var_p != __array_variables.end(); ++var_p ) {
		if ( is_global_var( *var_p ) ) {
		    const std::string& name = *var_p;
		    loop_code->push_back( new LQX::AssignmentStatementNode( get_destination(*var_p), new LQX::VariableExpression( &name[1], false ) ) );
		}
	    }

	    /* Assign input variables which are not array vars. */
	    
	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator iv_p = __input_variables.begin(); iv_p != __input_variables.end(); ++iv_p ) {
		const std::string& name = iv_p->first;
		if ( is_global_var( name ) && find( __array_variables.begin(), __array_variables.end(), name ) == __array_variables.end() ) {
		    loop_code->push_back( new LQX::AssignmentStatementNode( get_destination(name), new LQX::VariableExpression( &name[1], false ) ) );
		}
	    }

	    /* Increment magic variable $0 */
	    loop_code->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "_0", false ), 
								    new LQX::MathExpression(LQX::MathExpression::ADD, new LQX::VariableExpression( "_0", false ), new LQX::ConstantValueExpression( 1.0 ) ) ) );

	    if ( __verbose ) {
		/* Need to go thru all input_variables */

		expr_list * print_args = new expr_list;
		print_args->push_back( new LQX::ConstantValueExpression( const_cast<char *>("Input parameters:") ) );
		for ( std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator iv_p = __input_variables.begin(); iv_p != __input_variables.end(); ++iv_p ) {
		    std::string s = " ";
		    s += iv_p->first;
		    s += "=";
		    print_args->push_back( new LQX::ConstantValueExpression( s ) );

		    const bool is_external = LQIO::DOM::Spex::is_global_var( iv_p->first );
		    const std::string& name = iv_p->first;
//		    if ( !is_external && local[0] == '$' ) {
//			local[0] = '_';
//		    }
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

	expr_list * Spex::solve_success( expr_list * result ) const
	{	
	    expr_list * block = new expr_list;

	    /* Insert all functions to extract results. */

	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::iterator obs_p = __observations.begin(); obs_p != __observations.end(); ++obs_p ) {
		block->push_back( obs_p->second );
	    }

//	    if ( __verbose ) {
//		block->push_back( new LQX::MethodInvocationExpression("print_symbol_table") );
//	    }

	    /* Insert print expression for results */

	    if ( result && result->size() > 0 ) {
		result->insert( result->begin(), new LQX::ConstantValueExpression( ", " ) );	/* Force CSV output with newline */
		block->push_back( new LQX::FilePrintStatementNode( result, true, true ) );
	    }

	    return block;
	}

	expr_list * Spex::solve_failure( expr_list * result ) const
	{
	    expr_list * block = new expr_list;
	    expr_list * args = new expr_list;
	    args->push_back( new LQX::ConstantValueExpression( "solver failed: $0=" ) );
	    args->push_back( new LQX::VariableExpression( "_0", false ) );
	    block->push_back( new LQX::FilePrintStatementNode( args, true, false ) );

	    return block;
	}

	/*
	 * file_output_stmt(X) ::= FILE_PRINTLN_SP OBRACKET expr_list(A) CBRACKET.	     { X = new FilePrintStatementNode( A, true, true ); }
	 */

	LQX::SyntaxTreeNode* Spex::print_header() const 
	{
	    expr_list * list = new std::vector<LQX::SyntaxTreeNode*>;
	    list->push_back( new LQX::ConstantValueExpression( ", " ) );
	    for ( std::vector<std::string>::iterator var = __result_variables.begin(); var != __result_variables.end(); ++var ) {
		list->push_back( new LQX::ConstantValueExpression( *var ) );
	    }
	    return new LQX::FilePrintStatementNode( list, true, true );		/* Println spaced, with first arg being ", " (or: output, ","). */
	}

	/* ------------------------------------------------------------------------ */
	/* lqn 'input' output functions for printing spex stuff out in input file.  */
	/* ------------------------------------------------------------------------ */

	/* static */ std::ostream& 	 
	Spex::printInputVariables( std::ostream& output ) 	 
	{ 	 
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" ); 	 
	    for_each( __input_variables.begin(), __input_variables.end(), PrintInputVariable( output ) ); 	 
	    return output; 	 
	} 	 
	  	 
	/* 	 
	 * Find all observation variables that match object and print them out. 	 
	 */ 	 
	  	 
	/* static  */ std::ostream& 	 
	Spex::printObservationVariables( std::ostream& output ) 	 
	{ 	 
	    for ( std::vector<ObservationInfo>::const_iterator obs = __document_variables.begin(); obs != __document_variables.end(); ++obs ) { 	 
		output << " "; 	 
		obs->print( output ); 	 
	    } 	 
	    return output; 	 
	} 	 
	  	 
	/* static  */ std::ostream& 	 
	Spex::printObservationVariables( std::ostream& output, const DocumentObject& object ) 	 
	{ 	 
	    std::pair<obs_var_tab_t::iterator, obs_var_tab_t::iterator> range = __observation_variables.equal_range( &object ); 	 
	    for ( obs_var_tab_t::iterator obs = range.first; obs != range.second; ++obs ) { 	 
		output << " "; 	 
		obs->second.print( output ); 	 
	    } 	 
	    /* If object is an entry, call again as a phase. */ 	 
	    if ( dynamic_cast<const Entry *>(&object) != 0 ) { 	 
		const std::map<unsigned, Phase*>& phases = dynamic_cast<const Entry *>(&object)->getPhaseList(); 	 
		for ( std::map<unsigned, Phase*>::const_iterator phase = phases.begin(); phase != phases.end(); ++phase ) { 	 
		    printObservationVariables( output, *phase->second ); 	 
		} 	 
	    } 	 
	    return output; 	 
	} 	 
	  	 
	/* static  */ std::ostream& 	 
	Spex::printResultVariables( std::ostream& output ) 	 
	{ 	 
	    /* Force output in the order of input. */ 	 
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" ); 	 
	    for ( std::vector<std::string>::iterator var = __result_variables.begin(); var != __result_variables.end(); ++var ) { 	 
		output << "  " << Spex::print_result_variable( *var ) << std::endl; 	 
	    } 	 
	    return output; 	 
	} 	 

	std::ostream&
	Spex::printInputVariable( std::ostream& output, const std::pair<std::string,LQX::SyntaxTreeNode *>& var )
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
	Spex::printResultVariable( std::ostream& output, const std::string& var )
	{
	    output << var;
	    std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator expr = __result_expression.find( var );
	    if ( expr != __result_expression.end() && expr->second ) {
		output << " = " << *(expr->second);
	    }
	    return output;
	}

	/* ------------------------------------------------------------------------ */

	class Spex spex;

	std::vector<std::string> Spex::__array_variables;
	std::vector<std::string> Spex::__result_variables;
	std::vector<std::string> Spex::__convergence_variables;
	std::map<std::string,LQX::SyntaxTreeNode *> Spex::__observations;
	std::map<std::string,Spex::ComprehensionInfo> Spex::__comprehensions;	/* Saves all comprehensions for $<name> */
	expr_list Spex::__deferred_assignment;

	std::map<std::string,LQX::SyntaxTreeNode *> Spex::__input_variables;	/* Save for printing when __verbose == true */
	Spex::obs_var_tab_t Spex::__observation_variables;			/* Saves all key-$var for each object */
	std::vector<Spex::ObservationInfo> Spex::__document_variables;		/* Saves all key-$var for the document */
	std::map<std::string,std::string> Spex::__input_iterator;		/* Saves iterator for x, y = expr statements */

	std::map<std::string,Spex::attribute_table_t> Spex::__control_parameters;
	std::map<int,std::string> Spex::__key_code_map;				/* Maps srvn_gram.h KEY_XXX to name */
	std::map<int,std::string> Spex::__key_lqx_function_map;			/* Maps srvn_gram.h KEY_XXX to lqx function name */
	std::map<std::string,LQX::SyntaxTreeNode *> Spex::__result_expression;	/* Maps result vars to expressions */
	std::map<const DOM::ExternalVariable *,const LQX::SyntaxTreeNode *> Spex::__inline_expression;	/* Maps temp vars to expressions */

	bool Spex::__verbose = false;
	bool Spex::__no_header = false;

	const char * Spex::__convergence_limit_str = "$convergence_limit";
    }
}

namespace LQIO {
    namespace DOM {
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
}

namespace LQIO {
    namespace DOM {

	LQX::SyntaxTreeNode * Spex::attribute_table_t::operator()( const std::string& name ) const
	{
	    switch ( _t ) {
	    case IS_EXTVAR:
		(LQIO::DOM::currentDocument->*_f.a_extvar)( LQIO::DOM::currentDocument->db_build_parameter_variable( name.c_str(), NULL ) );
		return new LQX::VariableExpression(name.c_str(),true);

	    case IS_PROPERTY:
		return new LQX::ObjectPropertyReadNode( new LQX::MethodInvocationExpression( "document" ), _f.a_string );
		break;

	    default:
		abort();
		break;
	    }
	}
    }
}

namespace LQIO {
    namespace DOM {

	Spex::ObservationInfo::ObservationInfo( int key, unsigned int phase, const char * variable_name, unsigned int conf_level, const char * conf_variable_name ) 
	    : _key(key), _phase(phase), _variable_name(variable_name), _conf_level(conf_level), _conf_variable_name("")
	{
	    if ( conf_level != 0 && conf_variable_name != 0 ) {
		_conf_variable_name = conf_variable_name;
	    }
	}

	bool
	Spex::ObservationInfo::operator()( const DocumentObject * o1, const DocumentObject * o2 ) const 
	{ 
	    return o1 == NULL || (o2 != NULL && o1->getSequenceNumber() < o2->getSequenceNumber());
	}

	std::ostream&
	Spex::ObservationInfo::print( std::ostream& output ) const
	{
	    output << "%" << __key_code_map.at(_key);
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
}

/* ------------------------------------------------------------------------ */
/* Interface between the parser and the spex generator.			    */
/* ------------------------------------------------------------------------ */

/*
 * Create a program.  Param_arg makes up the main line.  mearsure_arg
 * is composed of all the %x type annotations for results.  result_arg
 * is composed of the result section.  measure and result are tacked
 * on after the solve() function to produce output.
 */

void spex_set_program( void * param_arg, void * result_arg, void * convergence_arg )
{
    expr_list * program = static_cast<expr_list *>(param_arg);
    if ( program && LQIO::DOM::spex.construct_program( program, 
						       static_cast<expr_list *>(result_arg), 
						       static_cast<expr_list *>(convergence_arg) ) ) {
	LQIO::DOM::currentDocument->setLQXProgram( LQX::Program::loadRawProgram( program ) );
    }
}

/*
 * Create a Globally-scoped variable.  Note that these must all be assigned prior to calling solve().
 * Parameter variables which are not constant are "deferred" and will be assigned while the spex program runs.
 */

void * spex_assignment_statement( const char * name, void * expr, const bool constant_expression )
{
    if ( LQIO::DOM::Spex::has_input_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return 0;
    }

    LQIO::DOM::Spex::__input_variables[std::string(name)] = static_cast<LQX::SyntaxTreeNode *>(expr);		/* Save variable for printing */

    LQX::VariableExpression * var = 0;
    /* check expr for array_create (fugly!) */
    std::ostringstream ss;
    static_cast<LQX::SyntaxTreeNode *>(expr)->print(ss,0);
    if ( strncmp( "array_create", ss.str().c_str(), 12 ) == 0 ) {
	LQIO::DOM::Spex::__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
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
	LQIO::DOM::Spex::__deferred_assignment.push_back( statement );
	return 0;
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
    if ( LQIO::DOM::Spex::has_input_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return 0;
    } else if ( !constant_expression ) {
	LQIO::input_error( "Arrays must contain constants only.\n" );
	return 0;
    }

    LQIO::DOM::Spex::__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
    LQX::SyntaxTreeNode * array = new LQX::MethodInvocationExpression("array_create", static_cast<expr_list *>(list) );
    std::string local = name;
    local[0] = '_';
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression( local, false ), array );
    LQIO::DOM::Spex::__input_variables[std::string(name)] =  array;
    return statement;
}

/*
 * This creates for loops.
 */

void * spex_array_comprehension( const char * name, double begin, double end, double stride )
{
    if ( LQIO::DOM::Spex::has_input_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return 0;
    }

    LQIO::DOM::Spex::__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
    LQIO::DOM::Spex::__comprehensions[name] = LQIO::DOM::Spex::ComprehensionInfo( begin, end, stride );
    LQIO::DOM::Spex::__input_variables[name] = 0;
    return 0;
}


void * spex_forall( const char * iter_name, const char * name, void * expr ) 
{
    if ( LQIO::DOM::Spex::has_input_var( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return 0;
    }

    LQIO::DOM::Spex::__input_variables[name] = static_cast<LQX::SyntaxTreeNode *>(expr);		/* Save variable for printing */
    LQIO::DOM::Spex::__input_iterator[name] = iter_name;
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), 
									static_cast<LQX::SyntaxTreeNode *>(expr) );
    LQIO::DOM::Spex::__deferred_assignment.push_back( statement );
    return 0;
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
    return new LQX::MethodInvocationExpression("pow", static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2), 0);
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
    return new LQX::LogicExpression(LQX::LogicExpression::NOT, static_cast<LQX::SyntaxTreeNode *>(arg2), NULL );
} 

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
    name << "$_" << std::setw(3) << std::setfill('0') << LQIO::DOM::Spex::__inline_expression.size() << "_";
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(name.str(),true), static_cast<LQX::SyntaxTreeNode *>(expr) );
    LQIO::DOM::Spex::__deferred_assignment.push_back( statement );
    LQIO::DOM::ExternalVariable * var = LQIO::DOM::currentDocument->db_build_parameter_variable(name.str().c_str(),NULL);
    LQIO::DOM::Spex::__inline_expression.insert( std::pair<const LQIO::DOM::ExternalVariable *,const LQX::SyntaxTreeNode*>(var,static_cast<LQX::SyntaxTreeNode *>(expr)) );
    return var;
}


void * spex_document_observation( const int key, const char * var )
{
    if ( !var ) return 0;
    return LQIO::DOM::spex.observation( key, var );
}

void * spex_processor_observation( const void * obj, const int key, const int conf, const char * var, const char * var2 ) 
{
    if ( !obj || !var ) return 0;

    LQIO::DOM::Spex::ObservationInfo obs( key, 0, var, conf, var2 );
    return LQIO::DOM::spex.observation( static_cast<const LQIO::DOM::Processor *>(obj), 0, "processor", obs );
}


void * spex_task_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    if ( !obj || !var ) return 0;

    LQIO::DOM::Spex::ObservationInfo obs( key, phase, var, conf, var2 );
    return LQIO::DOM::spex.observation( static_cast<const LQIO::DOM::Task *>(obj), phase, "task", obs );
}

void * spex_entry_observation( const void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    if ( !obj || !var ) return 0;

    const LQIO::DOM::Entry * entry = static_cast<const LQIO::DOM::Entry *>(obj);

    if ( ((0 < phase && phase <= LQIO::DOM::Phase::MAX_PHASE) && ( key == KEY_SERVICE_TIME || key == KEY_UTILIZATION  || key == KEY_PROCESSOR_UTILIZATION  || key == KEY_PROCESSOR_WAITING  || key == KEY_VARIANCE ))
	 || (phase == 0 && (key == KEY_THROUGHPUT || key == KEY_UTILIZATION || key == KEY_PROCESSOR_UTILIZATION)) ) {
	/* This is by phase... but LQX needs entry and phase name */
	LQIO::DOM::Spex::ObservationInfo obs( key, phase, var, conf, var2 );
	return LQIO::DOM::spex.observation( entry, phase, "entry", obs );
    } else {
	input_error2( LQIO::WRN_INVALID_RESULT_PHASE, phase, LQIO::DOM::Spex::__key_code_map[key].c_str(), entry->getName().c_str() );
	return 0;
    }

}

void * spex_activity_observation( const void * task, const void * activity, const int key, const int conf, const char * var, const char * var2 )
{
    if ( !task || !activity || !var ) return 0;

    LQIO::DOM::Spex::ObservationInfo obs( key, 0, var, conf, var2 );
    return LQIO::DOM::spex.observation( static_cast<const LQIO::DOM::Task *>(task), static_cast<const LQIO::DOM::Activity *>(activity), "activity", obs );
}


void * spex_call_observation( const void * src, const int key, const int phase, const void * dst, const int conf, const char * var, const char * var2 )
{
    if ( !src || !dst || !var ) return 0;

    const LQIO::DOM::Entry * src_entry = static_cast<const LQIO::DOM::Entry *>(src);
    const LQIO::DOM::Entry * dst_entry = static_cast<const LQIO::DOM::Entry *>(dst);

    if ( phase <= 0 || LQIO::DOM::Phase::MAX_PHASE < phase ) {
	input_error2( LQIO::WRN_INVALID_RESULT_PHASE, phase, LQIO::DOM::Spex::__key_code_map[key].c_str(), src_entry->getName().c_str() );
	return 0;
    } else {
	LQIO::DOM::Spex::ObservationInfo obs( key, phase, var, conf, var2 );
	return LQIO::DOM::spex.observation( src_entry, phase, dst_entry, obs );
    }

}

void * spex_activity_call_observation( const void * task, const void * activity, const int key, const void * dst, const int conf, const char * var, const char * var2 )
{
    if ( !task || !activity || !dst || !var ) return 0;

    LQIO::DOM::Spex::ObservationInfo obs( key, 0, var, conf, var2 );
    return LQIO::DOM::spex.observation( static_cast<const LQIO::DOM::Task *>(task), static_cast<const LQIO::DOM::Activity *>(activity),
					static_cast<const LQIO::DOM::Entry *>(dst), obs );
}

/*
 * This is a touch tricky.
 * Some variables are globals and are used as parameters (they should be in the variable database for the doc).
 * Result variables are local even though they have a $, otherwise the solve() function won't run.
 * Local and Global variables have different scope... doh... so we have to figure out what is what.
 */

void * spex_result_assignment_statement( const char * name, void * expr )
{
    LQIO::DOM::Spex::__result_variables.push_back( name );		/* Save variable name for printing */
    LQIO::DOM::Spex::__result_expression[name] = static_cast<LQX::SyntaxTreeNode *>(expr);

    LQX::VariableExpression * variable = static_cast<LQX::VariableExpression * >(spex_get_symbol( name ));
    if ( expr ) {
	return new LQX::AssignmentStatementNode( variable, static_cast<LQX::SyntaxTreeNode *>(expr) );
    } else {
        return variable;
    }
}

/*
 * I need to create a local version of each variable.  Delta them all
 */

void * spex_convergence_assignment_statement( const char * name, void * expr )
{
    LQIO::DOM::Spex::__convergence_variables.push_back( std::string(name) );		/* Save variable name for looping */
    return new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, true ), static_cast<LQX::SyntaxTreeNode *>(expr) );
}

void * spex_list( void * list_arg, void * node )
{
    std::vector<LQX::SyntaxTreeNode *> * list = static_cast<expr_list *>(list_arg);
    if ( !list ) {
	list = new std::vector<LQX::SyntaxTreeNode*>();
    }
    if ( node ) {
	list->push_back(static_cast<LQX::SyntaxTreeNode*>(node));
    }
    return list;
}


void * spex_get_symbol( const char * name )
{
    const bool is_external = LQIO::DOM::Spex::is_global_var( name );
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


