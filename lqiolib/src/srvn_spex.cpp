/*
 *  $Id$
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
#include <cstdlib>
#include <sstream>
#include <iomanip>
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
	    __varnum = 0;
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

	    /* This should map srvn_scan.l */

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
	}


	/* static */ bool 
	Spex::is_global_var( const std::string& name ) 
	{
	    return LQIO::DOM::currentDocument->hasSymbolExternalVariable( name );
	}

	/*
	 * Parameters have all been declared and set.  Now we start the control program.
	 * Create the foreach loops for all of the local variables created due to array lists in the parameters.
	 */

	expr_list * Spex::construct_program( expr_list * main_line, expr_list * result, expr_list * convergence ) 
	{
	    if ( !__have_vars ) return main_line;		/* If we have only set control variables, then nothing to do. */

	    main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( "_0", false ), new LQX::ConstantValueExpression( 0.0 ) ) );			/* Add $0 variable */
	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::iterator obs_p = __observations.begin(); obs_p != __observations.end(); ++obs_p ) {
		const std::string& name = obs_p->first;
		main_line->push_back( new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, false ), new LQX::ConstantValueExpression( 0.0 ) ) );	/* Initialize all observations variables */
	    }
	    if ( !__no_header ) {
		main_line->push_back( print_header() );
	    }
	    main_line->push_back( foreach_loop( __array_variables.begin(), result, convergence ) );
	    return main_line;
	}


	LQX::SyntaxTreeNode * Spex::observation( const int key, const char * name ) 
	{
	    std::string local = name;
	    local[0] = '_';
	    __document_variables.push_back( ObservationInfo( key, 0, local.c_str() ) );
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "document" );
	    LQX::ObjectPropertyReadNode * node = 0;
	    switch ( key ) {
	    case KEY_WAITING:		node = new LQX::ObjectPropertyReadNode( object, "waits" ); break;
	    case KEY_ITERATIONS:	node = new LQX::ObjectPropertyReadNode( object, "iterations" ); break;
	    case KEY_USER_TIME:		node = new LQX::ObjectPropertyReadNode( object, "user_cpu_time" ); break;
	    case KEY_SYSTEM_TIME:	node = new LQX::ObjectPropertyReadNode( object, "system_cpu_time" ); break;
	    default:	abort();
	    }
	    LQX::AssignmentStatementNode * assignment = new LQX::AssignmentStatementNode( new LQX::VariableExpression( local, false ), node );
	    __observations[local] = assignment;
	    return node;
	}

	LQX::SyntaxTreeNode * Spex::observation( const DocumentObject& document_object, const int phase, const int key, const char * name ) 
	{
	    std::string local = name;
	    local[0] = '_';
	    __observation_variables.insert( std::pair<const DocumentObject*,ObservationInfo>(&document_object,ObservationInfo( key, phase, local.c_str() )) );
	    const char * type_name;
	    if ( dynamic_cast<const LQIO::DOM::Processor *>(&document_object) != 0 ) {
		type_name = "processor";
	    } else if ( dynamic_cast<const LQIO::DOM::Task *>(&document_object) != 0 ) {
		type_name = "task";
	    } else if ( dynamic_cast<const LQIO::DOM::Entry *>(&document_object) != 0 ) {
		type_name = "entry";
	    } else if ( dynamic_cast<const LQIO::DOM::Activity *>(&document_object) != 0 ) {
		type_name = "activity";
	    } else {   
		abort();
	    }
	    LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( type_name, new LQX::ConstantValueExpression( document_object.getName() ), 0 );
	    if ( phase > 0 && phase <= LQIO::DOM::Phase::MAX_PHASE ) {
		object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(phase) ), 0 );
	    }
	    LQX::ObjectPropertyReadNode * node = new LQX::ObjectPropertyReadNode( object, __key_lqx_function_map[key] ); 
	    assert( node );
	    LQX::AssignmentStatementNode * assignment = new LQX::AssignmentStatementNode( new LQX::VariableExpression( local, false ), node );
	    __observations[local] = assignment;
	    return node;
	}
    
	LQX::SyntaxTreeNode * Spex::observation( const DocumentObject& src, const int phase, const DocumentObject& dst, const int key, const char * name )
	{
	    std::string local = name;
	    local[0] = '_';
	    LQX::MethodInvocationExpression * object;
	    const LQIO::DOM::Call * call_object;
	    const char * type_name;
	    if ( dynamic_cast<const LQIO::DOM::Entry *>(&src) != 0 && phase != 0 ) {
		object = new LQX::MethodInvocationExpression( "entry", new LQX::ConstantValueExpression( src.getName() ), 0 );
		object = new LQX::MethodInvocationExpression( "phase", object, new LQX::ConstantValueExpression( static_cast<double>(phase) ), 0 );
		call_object = dynamic_cast<const LQIO::DOM::Entry&>(src).getCallToTarget( dynamic_cast<const LQIO::DOM::Entry *>(&dst), phase );
	    } else if ( dynamic_cast<const LQIO::DOM::Activity *>(&src) != 0 && phase == 0 ) {
		object = new LQX::MethodInvocationExpression( "activity", new LQX::ConstantValueExpression( src.getName() ), 0 );
		call_object = dynamic_cast<const LQIO::DOM::Activity&>(src).getCallToTarget( dynamic_cast<const LQIO::DOM::Entry *>(&dst) );
	    } else {
		abort();
	    }
	    __observation_variables.insert( std::pair<const DocumentObject*,ObservationInfo>(call_object,ObservationInfo( key, phase, local.c_str() )) );
	    object = new LQX::MethodInvocationExpression( "call", object, new LQX::ConstantValueExpression( dst.getName() ), 0 );
	    LQX::ObjectPropertyReadNode * node = new LQX::ObjectPropertyReadNode( object, __key_lqx_function_map[key] ); 
	    LQX::AssignmentStatementNode * assignment = new LQX::AssignmentStatementNode( new LQX::VariableExpression( local, false ), node );
	    __observations[local] = assignment;
	    return node;		/* For chaining */
	}

	LQX::SyntaxTreeNode * Spex::confidence_interval( LQX::SyntaxTreeNode * object, const int key, const int interval, const char * name ) 
	{
	    std::string local = name;
	    local[0] = '_';
	    object = new LQX::MethodInvocationExpression( "conf_int", object, new LQX::ConstantValueExpression( static_cast<double>(interval) ), 0 );
	    LQX::ObjectPropertyReadNode * node = new LQX::ObjectPropertyReadNode( object, __key_lqx_function_map[key] ); 
	    assert( node );
	    LQX::AssignmentStatementNode * assignment = new LQX::AssignmentStatementNode( new LQX::VariableExpression( local, false ), node );
	    __observations[local] = assignment;
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
		std::string var = *var_p;	/* Make local copy in case we force to local */
		const bool is_external = is_global_var( var );
		if ( !is_external && var[0] == '$' ) {
		    var[0] = '_';		/* Force to local variable */
		}
		std::map<std::string,Spex::ComprehensionInfo>::const_iterator i = __comprehensions.find( *var_p );
		if ( i == __comprehensions.end() ) {
		    /* if we have $x = [...] */
		    return new LQX::ForeachStatementNode( "", var.c_str(), /* key ext */ false, /* val */ is_external, 
							  new LQX::VariableExpression( &var.c_str()[1], false ), 
							  foreach_loop( ++var_p, result, convergence ) );
		} else {
		/* for ( i = begin; i < end; ++i ) 
		       $x = f(i); */
		    return new LQX::LoopStatementNode( i->second.init(), i->second.test(), i->second.step(), foreach_loop( ++var_p, result, convergence ) );
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
		    std::string local = iv_p->first;
		    if ( !is_external && local[0] == '$' ) {
			local[0] = '_';
		    }
		    LQX::VariableExpression * variable = new LQX::VariableExpression( local, is_external );
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
	    for ( std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator var = __input_variables.begin(); var != __input_variables.end(); ++var ) {
		std::map<std::string,Spex::ComprehensionInfo>::const_iterator comp = __comprehensions.find(var->first);
		if ( comp != __comprehensions.end() ) {
		    output << var->first << " = " << comp->second;
		} else {
		    std::map<std::string,std::string>::const_iterator iter_var = __input_iterator.find(var->first);
		    if ( iter_var != __input_iterator.end() ) {		/* For $iter, $var = <expr> */
			output << iter_var->second << ", ";
		    }
		    output << var->first << " = " << *(var->second);
		}
		output << std::endl;
	    }
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
	    return output;
	}

	/* static  */ std::ostream&
	Spex::printResultVariables( std::ostream& output )
	{
	    /* Force output in the order of input. */
	    for ( std::vector<std::string>::iterator var = __result_variables.begin(); var != __result_variables.end(); ++var ) {
		output << "  " << *var;
		std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator expr = __result_expression.find( *var );
		if ( expr != __result_expression.end() && expr->second ) {
		    output << " = " << *(expr->second);
		}
		output << std::endl;
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

	std::map<std::string,setParameterFunc> Spex::__control_parameters;
	std::map<int,std::string> Spex::__key_code_map;				/* Maps srvn_gram.h KEY_XXX to name */
	std::map<int,std::string> Spex::__key_lqx_function_map;			/* Maps srvn_gram.h KEY_XXX to lqx function name */
	std::map<std::string,LQX::SyntaxTreeNode *> Spex::__result_expression;	/* */

	bool Spex::__have_vars = false;
	bool Spex::__verbose = false;
	bool Spex::__no_header = false;
	unsigned int Spex::__varnum = 0;
    }
}

namespace LQIO {
    namespace DOM {
	Spex::ComprehensionInfo::ComprehensionInfo( const char * name, double init, double test, double step ) 
	    : _init_val(init), _test_val(test), _step_val(step)
	{
	    _init = new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, true ), new LQX::ConstantValueExpression( init ) );
	    _test = new LQX::ComparisonExpression( LQX::ComparisonExpression::LESS_OR_EQUAL, 
						   new LQX::VariableExpression( name, true ), 
						   new LQX::ConstantValueExpression( test ) );
	    _step = new LQX::AssignmentStatementNode( new LQX::VariableExpression( name, true ), 
						      new LQX::MathExpression( LQX::MathExpression::ADD,
									       new LQX::VariableExpression( name, true ), 
									       new LQX::ConstantValueExpression( step ) ) );
	}

	std::ostream&
	Spex::ComprehensionInfo::print( std::ostream& output ) const
	{
	    output << "[" << _init_val << ":" << _test_val << "," << _step_val << "]";
	    return output;
	}
    }
}

namespace LQIO {
    namespace DOM {

	bool
	Spex::ObservationInfo::operator()( const DocumentObject * o1, const DocumentObject * o2 ) const 
	{ 
	    return o1->getSequenceNumber() < o2->getSequenceNumber();
	}

	std::ostream&
	Spex::ObservationInfo::print( std::ostream& output ) const
	{
	    output << "%" << __key_code_map[_key];
	    if ( _phase != 0 ) {
		output << _phase;
	    }
	    output << " " << _variable_name;
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
    LQIO::DOM::spex.construct_program( program, 
			    static_cast<expr_list *>(result_arg), 
			    static_cast<expr_list *>(convergence_arg) );
    LQIO::DOM::currentDocument->setLQXProgram( LQX::Program::loadRawProgram( program ) );
}


/*
 * Create a Globally-scoped variable.  Note that these must all be assigned prior to calling solve().
 * Parameter variables which are not constant are "deferred" and will be assigned while the spex program runs.
 */

void * spex_assignment_statement( const char * name, void * expr, const bool constant_expression )
{
    LQIO::DOM::Spex::initialize_control_parameters();
    LQX::SyntaxTreeNode * statement = 0;

    /* Funky variables for convergence and what have you.  They are explict in the model file, so fake it. */
    std::map<std::string,LQIO::DOM::setParameterFunc>::const_iterator i = LQIO::DOM::Spex::__control_parameters.find( name );
    if ( i != LQIO::DOM::Spex::__control_parameters.end() ) {
	LQIO::DOM::setParameterFunc f = i->second;
	if ( f ) {
	    (LQIO::DOM::currentDocument->*f)(LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL ));
	} else if ( i->first == "$model_comment" ) {
	    LQIO::DOM::currentDocument->setModelComment( name );
	}
	statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(name,true), static_cast<LQX::SyntaxTreeNode *>(expr) );
    } else if ( LQIO::DOM::spex.__input_variables.find( name ) != LQIO::DOM::spex.__input_variables.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "variable", name );
	return 0;
    } else {
	LQIO::DOM::Spex::__have_vars = true;
	LQIO::DOM::Spex::__input_variables[std::string(name)] = static_cast<LQX::SyntaxTreeNode *>(expr);		/* Save variable for printing */
	/* check expr for array_create (fugly!) */
	std::ostringstream ss;
	static_cast<LQX::SyntaxTreeNode *>(expr)->print(ss,0);
	if ( strncmp( "array_create", ss.str().c_str(), 12 ) == 0 ) {
	    LQIO::DOM::spex.__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
	    statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(&name[1],false), static_cast<LQX::SyntaxTreeNode *>(expr) );
	} else {
	    LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL );
	    statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(name,true), static_cast<LQX::SyntaxTreeNode *>(expr) );
	}
    }


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
    LQIO::DOM::Spex::initialize_control_parameters();

    /* Funky variables for convergence and what have you.  They are explict in the model file, so fake it. */
    std::map<std::string,LQIO::DOM::setParameterFunc>::const_iterator i = LQIO::DOM::Spex::__control_parameters.find( name );
    if ( i != LQIO::DOM::Spex::__control_parameters.end() ) {
	LQIO::input_error( "Control variable %s cannot be used in an array assignment.\n" );
	return 0;
    } else if ( !constant_expression ) {
	LQIO::input_error( "Arrays must contain constants only.\n" );
	return 0;
//    } else if ( LQIO::DOM::spex.__array_variables.find( name ) != LQIO::DOM::spex.__array_variables.end() ) {
//	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "array variable", name );
//	return 0;
    } else {
	LQIO::DOM::spex.__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
	LQIO::DOM::Spex::__have_vars = true;
	LQX::SyntaxTreeNode * array = new LQX::MethodInvocationExpression("array_create", static_cast<expr_list *>(list) );
	LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &name[1], false ), array );
	LQIO::DOM::Spex::__input_variables[std::string(name)] =  array;
	return statement;
    }
}

/*
 * As above, but we have to create the array elements from begin to end.
 */

void * spex_array_comprehension( const char * name, double begin, double end, double stride )
{
    LQIO::DOM::Spex::initialize_control_parameters();

    /* Funky variables for convergence and what have you.  They are explict in the model file, so fake it. */
    std::map<std::string,LQIO::DOM::setParameterFunc>::const_iterator i = LQIO::DOM::Spex::__control_parameters.find( name );
    if ( i != LQIO::DOM::Spex::__control_parameters.end() ) {
	LQIO::input_error( "Control variable %s cannot be used in an array assignment.\n" );
	return 0;
    } else {
	LQIO::DOM::spex.__array_variables.push_back( std::string(name) );		/* Save variable name for looping */
	LQIO::DOM::Spex::__have_vars = true;
	LQIO::DOM::Spex::ComprehensionInfo comprehension( name, begin, end, stride );
	LQIO::DOM::Spex::__comprehensions[name] = comprehension;
	LQIO::DOM::Spex::__input_variables[std::string(name)] = comprehension.init();
	return 0;
    }
}


void * spex_ternary( void * arg1, void * arg2, void * arg3 ) 
{
    return new LQX::ConditionalStatementNode(static_cast<LQX::SyntaxTreeNode *>(arg1), static_cast<LQX::SyntaxTreeNode *>(arg2), static_cast<LQX::SyntaxTreeNode *>(arg3));
}

void * spex_forall( const char * iter_name, const char * name, void * expr ) 
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
	LQIO::DOM::Spex::__input_variables[std::string(name)] = static_cast<LQX::SyntaxTreeNode *>(expr);		/* Save variable for printing */
	LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL );
	LQIO::DOM::Spex::__have_vars = true;
	LQIO::DOM::Spex::__input_iterator[std::string(name)] = iter_name;
    }

    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(name,true), static_cast<LQX::SyntaxTreeNode *>(expr) );
    LQIO::DOM::Spex::__deferred_assignment.push_back( statement );

    return 0;
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
    name << "$_";
    do {
	name << std::setw(3) << std::setfill('0') << LQIO::DOM::Spex::__varnum;
	LQIO::DOM::Spex::__varnum += 1;
    } while ( LQIO::DOM::currentDocument->hasSymbolExternalVariable( name.str().c_str() ) );
    LQX::SyntaxTreeNode * statement = new LQX::AssignmentStatementNode( new LQX::VariableExpression(name.str(),true), static_cast<LQX::SyntaxTreeNode *>(expr) );
    LQIO::DOM::Spex::__deferred_assignment.push_back( statement );
    return LQIO::DOM::currentDocument->db_build_parameter_variable(name.str().c_str(),NULL);
}


void * spex_document_observation( const int key, const char * var )
{
    if ( !var ) return 0;
    LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( key, var );
    return object;
}

void * spex_processor_observation( void * obj, const int key, const int conf, const char * var, const char * var2 ) 
{
    LQIO::DOM::Processor * processor = static_cast<LQIO::DOM::Processor *>(obj);

    if ( !obj || !var ) return 0;
    LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( *processor, 0, key, var );
    if ( var2 && object ) {
	object = LQIO::DOM::spex.confidence_interval( object, key, conf, var2 );
    }
    return object;
}


void * spex_task_observation( void * obj, const int key, const int phase, const int conf, const char * var, const char * var2 )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(obj);

    if ( !obj || !var ) return 0;
    LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( *task, phase, key, var );
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
	LQX::SyntaxTreeNode * object =  LQIO::DOM::spex.observation( *entry, phase, key, var );
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
	LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( *activity, 0, key, var );
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
	LQX::SyntaxTreeNode * object = LQIO::DOM::spex.observation( *src_entry, phase, *dst_entry, key, var );
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
	LQX::SyntaxTreeNode * object =  LQIO::DOM::spex.observation( *src_activity, 0, *dst_entry, key, var );
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
    const bool is_external = LQIO::DOM::Spex::is_global_var( name );
    std::string local = name;
    if ( !is_external && name[0] == '$' ) {
	local[0] = '_'; /* Force to local variable */
    }
    LQIO::DOM::Spex::__result_variables.push_back( name );		/* Save variable name for printing */
    LQX::VariableExpression * variable = new LQX::VariableExpression( local, is_external ); 	/* locals are from observations */
    LQIO::DOM::Spex::__result_expression[name] = static_cast<LQX::SyntaxTreeNode *>(expr);
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
    std::vector<LQX::SyntaxTreeNode *> * list = static_cast<expr_list *>(list_arg);
    if ( !list ) {
	list = new std::vector<LQX::SyntaxTreeNode*>();
    }
    if ( node ) {
	list->push_back(static_cast<LQX::SyntaxTreeNode*>(node));
    }
    return list;
}


void * spex_get_symbol( const char * name, const bool spex_initialization )
{
    if ( spex_initialization ) {
	LQIO::DOM::currentDocument->db_build_parameter_variable( name, NULL );
    }
    const bool is_external = LQIO::DOM::Spex::is_global_var( name );
    if ( !is_external && name[0] == '$' ) {
	std::string local = name;
	local[0] = '_'; /* Force to local variable */
	return new LQX::VariableExpression( local, is_external );
    } else {
	return new LQX::VariableExpression( name, is_external );
    }
}

void * spex_get_real( double arg )
{
    return new LQX::ConstantValueExpression( arg );
}


