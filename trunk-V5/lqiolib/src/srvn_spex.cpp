/*
 *  $Id: srvn_input.cpp 10813 2012-05-04 01:53:23Z greg $
 *
 *  Created by Greg Franks on 2012/05/03.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 * Note: Be careful with static casts because of lists and nodes.
 */

#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <lqx/SyntaxTree.h>
#include <lqx/Program.h>
#include "dom_document.h"
#include "dom_phase.h"
#include "srvn_input.h"
#include "srvn_spex.h"
#include "srvn_gram.h"
#include "glblerr.h"

namespace LQIO {
    namespace DOM {

	Spex::Spex() 
	{
	    __have_vars = false;
	}

	
	/* 
	 * Initialize the control parameters array.  Done at run time because it doesn't like being done in the constructor during program initialization.
	 */

	void Spex::initialize_control_parameters() 
	{
	    if ( LQIO::DOM::Spex::__control_parameters.size() != 0 ) return;

            __control_parameters["$convergence_limit"] = &Document::setModelConvergenceValue;
            __control_parameters["$iteration_limit"]   = &Document::setModelIterationLimit;
            __control_parameters["$print_interval"]    = &Document::setModelPrintInterval;
            __control_parameters["$seed_value"]        = &Document::setSimulationSeedValue;
            __control_parameters["$underrelaxation"]   = &Document::setModelUnderrelaxationCoefficient;
            __control_parameters["$block_time"]        = &Document::setSimulationBlockTime;
            __control_parameters["$model_comment"]     = 0;
            __control_parameters["$number_of_blocks"]  = &Document::setSimulationNumberOfBlocks;
            __control_parameters["$result_precision"]  = &Document::setSimulationResultPrecision;
            __control_parameters["$warm_up_loops"]     = &Document::setSimulationWarmUpLoops;    
	}


	/*
	 * Parameters have all been declared and set.  Now we start the control program.
	 * Create the foreach loops for all of the local variables created due to array lists in the parameters.
	 */

	expr_list * Spex::construct_program( expr_list * main_line, expr_list * result, expr_list * convergence ) 
	{
	    if ( !__have_vars ) return main_line;		/* If we have only set control variables, then nothing to do. */

	    main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "$0", false ), new LQX::ConstantValueExpression( 0.0 ) ) );			/* Add $0 variable */
	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::iterator obs_p = __observations.begin(); obs_p != __observations.end(); ++obs_p ) {
		const std::string& name = obs_p->first;
		main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( name.c_str(), true ), new LQX::ConstantValueExpression( 0.0 ) ) );		/* Initialize all observations variables */
	    }
	    if ( !__no_header ) {
		main_line->push_back( print_header() );
	    }
	    main_line->push_back( foreach_loop( __array_variables.begin(), result, convergence ) );
	    return main_line;
	}


	LQX::SyntaxTreeNode * Spex::observation( const std::string& type_name, const std::string& object_name, const int phase, const int key, const char * var_name ) 
	{
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( type_name, new LQX::ConstantValueExpression( const_cast<char *>(object_name.c_str())), 0  );
	    if ( phase > 0 && phase <= LQIO::DOM::Phase::MAX_PHASE ) {
		object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(phase) ), 0 );
	    }
	    LQX::ObjectPropertyReadNode * node;
	    switch ( key ) {
	    case KEY_PROCESSOR_UTILIZATION: node = new LQX::ObjectPropertyReadNode( object, "proc_utilization" ); break;
	    case KEY_PROCESSOR_WAITING:	    node = new LQX::ObjectPropertyReadNode( object, "proc_waiting" ); break;
	    case KEY_SERVICE_TIME:	    node = new LQX::ObjectPropertyReadNode( object, "service_time" ); break;
	    case KEY_THROUGHPUT:            node = new LQX::ObjectPropertyReadNode( object, "throughput" ); break;
	    case KEY_THROUGHPUT_BOUND:	    node = new LQX::ObjectPropertyReadNode( object, "throughput_bound" ); break;
	    case KEY_UTILIZATION:           node = new LQX::ObjectPropertyReadNode( object, "utilization" ); break;
	    case KEY_VARIANCE:		    node = new LQX::ObjectPropertyReadNode( object, "variance" ); break;
	    }
	    LQX::AssignmentStatementNode * assignment = new LQX::AssignmentStatementNode( new LQX::VariableExpression( var_name, true ), node );
	    LQIO::DOM::currentDocument->db_build_parameter_variable(var_name,NULL);
	    __observations[std::string(var_name)] = assignment;
	    return node;
	}
    
	LQX::SyntaxTreeNode * Spex::observation( const std::string& type_name, const std::string& src_name, const int phase, const std::string& dst_name, const int key, const char * var_name )
	{
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "entry", new LQX::ConstantValueExpression( const_cast<char *>(src_name.c_str())), 0 );
	    if ( phase > 0 && phase <= LQIO::DOM::Phase::MAX_PHASE ) {
		object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(phase) ), 0 );
	    }
	    object = new LQX::MethodInvocationExpression( type_name, object, new LQX::ConstantValueExpression( const_cast<char *>(dst_name.c_str())), 0 );
	    LQX::ObjectPropertyReadNode * node;
	    switch ( key ) {
	    case KEY_WAITING: node = new LQX::ObjectPropertyReadNode( object, "waiting" ); break;
	    }
	    LQX::AssignmentStatementNode * assignment = new LQX::AssignmentStatementNode( new LQX::VariableExpression( var_name, true ), node );
	    LQIO::DOM::currentDocument->db_build_parameter_variable(var_name,NULL);
	    __observations[std::string(var_name)] = assignment;
	    return node;		/* For chaining */
	}

	LQX::SyntaxTreeNode * Spex::confidence_interval( LQX::SyntaxTreeNode * object, const int key, const int interval, const char * var_name ) 
	{
	    object = new LQX::MethodInvocationExpression( "conf_int", object, new LQX::ConstantValueExpression( static_cast<double>(interval) ), 0 );
	    LQX::ObjectPropertyReadNode * node = 0;
	    switch ( key ) {
	    case KEY_PROCESSOR_UTILIZATION: node = new LQX::ObjectPropertyReadNode( object, "proc_utilization" ); break;
	    case KEY_PROCESSOR_WAITING:	    node = new LQX::ObjectPropertyReadNode( object, "proc_waiting" ); break;
	    case KEY_SERVICE_TIME:	    node = new LQX::ObjectPropertyReadNode( object, "service_time" ); break;
	    case KEY_THROUGHPUT:            node = new LQX::ObjectPropertyReadNode( object, "throughput" ); break;
	    case KEY_THROUGHPUT_BOUND:	    node = new LQX::ObjectPropertyReadNode( object, "throughput_bound" ); break;
	    case KEY_UTILIZATION:           node = new LQX::ObjectPropertyReadNode( object, "utilization" ); break;
	    case KEY_VARIANCE:		    node = new LQX::ObjectPropertyReadNode( object, "variance" ); break;
	    case KEY_WAITING: 		    node = new LQX::ObjectPropertyReadNode( object, "waiting" ); break;
	    }
	    assert( node );
	    LQX::AssignmentStatementNode * assignment = new LQX::AssignmentStatementNode( new LQX::VariableExpression( var_name, true ), node );
	    LQIO::DOM::currentDocument->db_build_parameter_variable(var_name,NULL);
	    __observations[std::string(var_name)] = assignment;
	    return node;		/* For chaining */
	}


	/*
	 * foreach( i,$var in list );
	 * loop_stmt(X) ::= FOREACH OBRACKET IDENTIFIER(A) COMMA IDENTIFIER(B) IN expr(C) CBRACKET stmt(D). 
	 *                 { X = new ForeachStatementNode( A->getStoredIdentifier(), B->getStoredIdentifier(), A->getIsExternal(), B->getIsExternal(), C, D ); }
	 */

	LQX::SyntaxTreeNode* Spex::foreach_loop( std::vector<std::string>::const_iterator var_p, expr_list * result, expr_list * convergence ) const 
	{
	    if ( var_p != __array_variables.end() ) {
		const std::string& var = *var_p;
		return new LQX::ForeachStatementNode( "", var.c_str(), false, true, 
						      new LQX::VariableExpression( &var.c_str()[1], false ), 
						      foreach_loop( ++var_p, result, convergence ) );
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
	    LQX::SyntaxTreeNode * test = 0;
	    for ( std::vector<std::string>::const_iterator var_p = __convergence_variables.begin(); var_p != __convergence_variables.end(); ++var_p ) {
		const std::string& name = *var_p;
		convergence->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "temp", false ), 
									  new LQX::ComparisonExpression( LQX::ComparisonExpression::LESS_THAN, 
													 new LQX::MethodInvocationExpression( "abs", 
																	      new LQX::MathExpression(LQX::MathExpression::SUBTRACT,
																				      new LQX::VariableExpression( &name[1], false ), 
																				      new LQX::VariableExpression( name, true ) ), 
																	      0 ),
													 new LQX::VariableExpression( "$convergence_limit", true ) ) ) );

		/* Prepare for next variable */
		if ( var_p !=  __convergence_variables.begin() ) {
		    convergence->push_back(  new LQX::AssignmentStatementNode( new LQX::VariableExpression( "done", false ), 
									       new LQX::LogicExpression( LQX::LogicExpression::AND, 
													 new LQX::VariableExpression( "done", false ), 
													 new LQX::VariableExpression( "temp", false ) ) ) );
		} else {
		    convergence->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "done", false ), new LQX::VariableExpression( "temp", false ) ) );

		}
		/* Copy new to old */
		convergence->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), new LQX::VariableExpression( name, true ) ) );

#if 0
		expr_list * debug3 = new expr_list;
		debug3->push_back( new LQX::ConstantValueExpression( "test: " ) );
		debug3->push_back( test ); 
		convergence->push_back( new LQX::FilePrintStatementNode( debug3, true, false ) );
#endif		
	    }
	    
#if 0
	    expr_list * debug2 = new expr_list;
	    debug2->push_back( new LQX::ConstantValueExpression( "done: " ) );
	    debug2->push_back( new LQX::VariableExpression( "done", false ) ); 
	    convergence->push_back( new LQX::FilePrintStatementNode( debug2, true, false ) );
#endif
	    /* Convergence loop */
	    loop_code->push_back( new LQX::LoopStatementNode( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "done", false ), new LQX::ConstantValueExpression( false ) ),
							      new LQX::LogicExpression(LQX::LogicExpression::NOT, new LQX::VariableExpression( "done", false ), NULL),
							      new LQX::CompoundStatementNode( convergence ),
							      new LQX::CompoundStatementNode( loop_body( result ) ) ) );
	    return loop_code;
	}

	expr_list * Spex::loop_body( expr_list * result ) const
	{
	    expr_list * loop_code = new expr_list;

	    *loop_code = Spex::__deferred_assignment;	/* load in all deferred assignement statements, eg '$x = $loop'... */
	    loop_code->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "$0", false ), 
								    new LQX::MathExpression(LQX::MathExpression::ADD, new LQX::VariableExpression( "$0", false ), new LQX::ConstantValueExpression( 1.0 ) ) ) );

	    if ( __verbose ) {
		/* Need to go thru all input_variables */

		expr_list * print_args = new expr_list;
		print_args->push_back( new LQX::ConstantValueExpression( const_cast<char *>("Input parameters:") ) );
		for ( std::vector<std::string>::iterator iv_p = __input_variables.begin(); iv_p != __input_variables.end(); ++iv_p ) {
		    std::string s = " ";
		    s += *iv_p;
		    s += "=";
		    print_args->push_back( new LQX::ConstantValueExpression( const_cast<char *>(s.c_str()) ) );
		    LQX::VariableExpression * variable = new LQX::VariableExpression( const_cast<char *>(iv_p->c_str()), true ); /* locals are from observations */
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
	    if ( result && result->size() > 0 ) {
		expr_list * args = new expr_list;
		args->push_back( new LQX::ConstantValueExpression( "solver failed: $0=" ) );
		args->push_back( new LQX::VariableExpression( "$0", false ) );
		block->push_back( new LQX::FilePrintStatementNode( args, true, false ) );
	    }

	    return block;
	}

	/*
	 * file_output_stmt(X) ::= FILE_PRINTLN_SP OBRACKET expr_list(A) CBRACKET.	     { X = new FilePrintStatementNode( A, true, true ); }
	 */

	LQX::SyntaxTreeNode* Spex::print_header() const 
	{
	    expr_list * list = new std::vector<LQX::SyntaxTreeNode*>;
	    list->push_back( new LQX::ConstantValueExpression( ", " ) );
	    for ( std::vector<std::string>::iterator rv_p = __result_variables.begin(); rv_p != __result_variables.end(); ++rv_p ) {
		std::string& s = *rv_p;
		list->push_back( new LQX::ConstantValueExpression( const_cast<char *>(s.c_str()) ) );	/* Cast is necessary, or it is treated as a boolean */
	    }
	    return new LQX::FilePrintStatementNode( list, true, true );		/* Println spaced, with first arg being ", " (or: output, ","). */
	}

	/* ------------------------------------------------------------------------ */

	class Spex spex;

	std::vector<std::string> Spex::__array_variables;
	std::vector<std::string> Spex::__result_variables;
	std::vector<std::string> Spex::__convergence_variables;
	std::vector<std::string> Spex::__input_variables;
	std::map<std::string,LQX::SyntaxTreeNode *> Spex::__observations;
	expr_list Spex::__deferred_assignment;
	std::map<std::string,setParameterFunc> Spex::__control_parameters;
	bool Spex::__have_vars = false;
	bool Spex::__verbose = false;
	bool Spex::__no_header = false;
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
    LQIO::DOM::spex.construct_program( program, 
			    static_cast<expr_list *>(result_arg), 
			    static_cast<expr_list *>(convergence_arg) );
    LQIO::DOM::currentDocument->setLQXProgram( LQX::Program::loadRawProgram( program ) );
    // create an initial program, registers initiial variables, then run the program.
}


/*
 * Create a Globally-scoped variable.  Note that these must all be assigned prior to calling solve().
 */

void * spex_assignment_statement( const char * name, void * expr, const bool constant_expression )
{
    LQIO::DOM::Spex::initialize_control_parameters();

    /* Funky variables for convergence and what have you.  They are explict in the model file, so fake it. */
    std::map<std::string,LQIO::DOM::setParameterFunc>::const_iterator i = LQIO::DOM::Spex::__control_parameters.find( name );
    if ( i != LQIO::DOM::Spex::__control_parameters.end() ) {
	LQIO::DOM::setParameterFunc f = i->second;
	if ( f ) {
	    (LQIO::DOM::currentDocument->*f)(LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL ));
	} else if ( i->first == "$model_comment" ) {
	    LQIO::DOM::currentDocument->setModelComment( name );
	}
    } else {
	LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL );
	LQIO::DOM::Spex::__have_vars = true;
	if ( LQIO::DOM::Spex::__verbose ) {
	    LQIO::DOM::Spex::__input_variables.push_back( std::string(name) );		/* Save variable for printing */
	}
    }

    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(name,true), static_cast<LQX::SyntaxTreeNode *>(expr) );

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
 */

void * spex_array_assignment( const char * name, void * list, const bool constant_expression )
{
    LQIO::DOM::Spex::initialize_control_parameters();

    /* Funky variables for convergence and what have you.  They are explict in the model file, so fake it. */
    std::map<std::string,LQIO::DOM::setParameterFunc>::const_iterator i = LQIO::DOM::Spex::__control_parameters.find( name );
    if ( i != LQIO::DOM::Spex::__control_parameters.end() ) {
	LQIO::input_error( "Control variable %s cannot be used in an array assignment.\n" );
	return 0;
    } else {
	LQIO::DOM::spex.__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
	LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL );
	LQIO::DOM::Spex::__have_vars = true;
	if ( LQIO::DOM::Spex::__verbose ) {
	    LQIO::DOM::Spex::__input_variables.push_back( std::string(name) );		/* Save variable for printing */
	}
	LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), 
									    new LQX::MethodInvocationExpression("array_create", static_cast<expr_list *>(list) ) );
	return statement;
    }
}

/*
 * As above, but we have to create the array elements from begin to end.
 */

void * spex_array_assignment2( const char * name, double begin, double end, double stride, const bool constant_expression )
{
    LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL );
    expr_list * list = new expr_list;
    for ( double i = begin; i <= end; i += stride ) {
	list->push_back( new LQX::ConstantValueExpression( i ) );
    }
    return spex_array_assignment( name, list, constant_expression );
}


void * spex_ternary( void * arg1, void * arg2, void * arg3 ) 
{
    return new LQX::ConditionalStatementNode(static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2), static_cast<LQX::SyntaxTreeNode *>(arg3));
}

void * spex_comma( void * arg1, void * arg2 ) 
{
    return new LQX::CompoundStatementNode(static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2), NULL);
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

void * spex_processor_observation( void * obj, const int key, const int conf, const char * var, const char * var2 ) 
{
    LQIO::DOM::Processor * processor = static_cast<LQIO::DOM::Processor *>(obj);

    if ( !obj || !var ) return 0;
    if ( key != KEY_UTILIZATION ) return &LQIO::DOM::Spex::__observations;
    LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( "processor", processor->getName(), 0, key, var );
    if ( var2 && object ) {
	object = LQIO::DOM::spex.confidence_interval( object, key, conf, var2 );
    }
    return object;
}


void * spex_task_observation( void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(obj);

    if ( !obj || !var ) return 0;
    LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( "task", task->getName(), phase, key, var );
    if ( var2 && object ) {
	object = LQIO::DOM::spex.confidence_interval( object, key, conf, var2 );
    }
    return object;
}

void * spex_entry_observation( void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    if ( !obj || !var ) return 0;
    LQIO::DOM::Entry * entry = static_cast<LQIO::DOM::Entry *>(obj);

    if ( entry->hasPhase(phase) || ((key == KEY_THROUGHPUT || key == KEY_UTILIZATION || key == KEY_PROCESSOR_UTILIZATION) && phase == 0) ) {
	LQX::SyntaxTreeNode * object =  LQIO::DOM::spex.observation( "entry", entry->getName(), phase, key, var );
	if ( var2 && object ) {
	    object = LQIO::DOM::spex.confidence_interval( object, key, conf, var2 );
	}
	return object;
    } else {
	input_error2( LQIO::WRN_INVALID_RESULT_PHASE, phase, "%x", entry->getName().c_str() );
	return 0;
    }

}


void * spex_activity_observation( void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    LQIO::DOM::Activity * activity = static_cast<LQIO::DOM::Activity *>(obj);

    if ( !obj || !var ) return 0;
    if ( phase != 0 ) {
	input_error2( LQIO::WRN_INVALID_RESULT_PHASE, phase, "%x", activity->getName().c_str() );
	return 0;
    } else {
	LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( "activity", activity->getName(), 0, key, var );
	if ( var2 && object ) {
	    object = LQIO::DOM::spex.confidence_interval( object, key, conf, var2 );
	}
	return object;
    }

}


void * spex_call_observation( void * src, const int key, const int phase, void * dst, const int conf, const char * var, const char * var2 )
{
    if ( !src || !var ) return 0;
    LQIO::DOM::Entry * src_entry = static_cast<LQIO::DOM::Entry *>(src);
    LQIO::DOM::Entry * dst_entry = static_cast<LQIO::DOM::Entry *>(dst);

    if ( phase == 0 || !src_entry->hasPhase(phase) ) {
	input_error2( LQIO::WRN_INVALID_RESULT_PHASE, phase, "%w", src_entry->getName().c_str() );
	return 0;
    } else {
	LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( "call", src_entry->getName(), phase, dst_entry->getName(), key, var );
	if ( var2 && object ) {
	    object = LQIO::DOM::spex.confidence_interval( object, key, conf, var2 );
	}
	return object;
    }

}

void * spex_activity_call_observation( void * src, const int key, const int phase, void * dst, const int conf, const char * var, const char * var2 )
{
    if ( !src || !var ) return 0;
    LQIO::DOM::Activity * src_activity = static_cast<LQIO::DOM::Activity *>(src);
    LQIO::DOM::Entry * dst_entry = static_cast<LQIO::DOM::Entry *>(dst);

    if ( phase != 0 ) {
	input_error2( LQIO::WRN_INVALID_RESULT_PHASE, phase, "%w", src_activity->getName().c_str() );
	return 0;
    } else {
	LQX::SyntaxTreeNode * object =  LQIO::DOM::spex.observation( "call", src_activity->getName(), 0, dst_entry->getName(), key, var );
	if ( var2 && object ) {
	    object = LQIO::DOM::spex.confidence_interval( object, key, conf, var2 );
	}	
	return object;
    }
}

/*
 * This is a touch tricky.
 * Some variables are globals and are used as parameters (they should be in the variable database for the doc).
 * Result variables are local even though they have a $, otherwise the solve() function won't run.
 * Local and Global variables have different scope... doh... so we have to figure out what is what.
 */

void * spex_result_assignment_statement( const char * name, void * expr )
{
    LQIO::DOM::Spex::__result_variables.push_back( std::string(name) );		/* Save variable name for printing */
    const bool is_global_var = LQIO::DOM::currentDocument->hasSymbolExternalVariable( name );
    LQX::VariableExpression * variable = new LQX::VariableExpression( name, is_global_var ); /* locals are from observations */
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
    LQIO::DOM::spex.__convergence_variables.push_back( std::string(name) );		/* Save variable name for looping */
    return new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, true ), static_cast<LQX::SyntaxTreeNode *>(expr) );
}

void * spex_list( void * list_arg, void * node )
{
    if ( !node ) return list_arg;
    std::vector<LQX::SyntaxTreeNode *> * list = static_cast<expr_list *>(list_arg);
    if ( !list ) list = new std::vector<LQX::SyntaxTreeNode*>();
    list->push_back(static_cast<LQX::SyntaxTreeNode*>(node));
    return list;
}


void * spex_get_symbol( const char * name )
{
    const bool is_global_var = LQIO::DOM::currentDocument->hasSymbolExternalVariable( name );
    return new LQX::VariableExpression( name, is_global_var );
}

void * spex_get_real( double arg )
{
    return new LQX::ConstantValueExpression( arg );
}


