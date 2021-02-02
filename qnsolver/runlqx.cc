/* runlqx.h	-- Greg Franks
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/runlqx.cc $
 * ------------------------------------------------------------------------
 * $Id: runlqx.cc 14440 2021-02-02 12:44:31Z greg $
 * ------------------------------------------------------------------------
 */

#include <iomanip>
#include <sstream>
#include <lqio/jmva_document.h>
#include <lqx/Program.h>
#include <lqx/MethodTable.h>
#include <lqx/Environment.h>
#include <mva/fpgoop.h>
#include <mva/mva.h>
#include "runlqx.h"

extern bool debug_flag;

namespace SolverInterface 
{

    unsigned int Solve::invocationCount = 0;
    std::string Solve::customSuffix;
    bool Solve::solveCallViaLQX = false;	/* Flag when a solve() call was made */
    bool Solve::implicitSolve = false;

    LQX::SymbolAutoRef Solve::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args)
    {
	solveCallViaLQX = true;
	if ( !implicitSolve ) {
	    invocationCount += 1;
	}

	/* See if we were given a suffix */
	if (args.size() > 0) {
	    assert(args.size() == 1);
	    std::stringstream ss;
	    LQX::SymbolAutoRef& suffix = args[0];
	    if (suffix->getType() == LQX::Symbol::SYM_STRING) {
		ss << "-" << suffix->getStringValue();
	    } else {
		ss << "-" << suffix->description();
	    }
	    customSuffix = ss.str();
	} else {
	    std::stringstream ss;
	    ss << "-" << std::setfill( '0' ) << std::setw(3) << invocationCount;
	    customSuffix = ss.str();
	}
			
#if defined(DEBUG_MESSAGES)
	env->cleanInvokeGlobalMethod("print_symbol_table", NULL);
#endif
			
	// /* Tell the world the iteration number */
	// if ( flags.trace_mva ) {
	//     std::cout << "\fSolving iteration #" << invocationCount << std::endl;
	// } else if ( flags.verbose ) {
	//     std::cerr << "Solving iteration #" << invocationCount << "..." << std::endl;
	// }
			
	/* Make sure all external variables are accounted for */
	bool ok = true;
	    
	    // const std::vector<std::string>& undefined = _document->getUndefinedExternalVariables();
	    // if ( undefined.size() > 0) {
	    // 	std::string msg = "The following external variables were not assigned at time of solve: ";
	    // 	for ( std::vector<std::string>::const_iterator var = undefined.begin(); var != undefined.end(); ++var ) {
	    // 	    if ( var != undefined.begin() ) msg += ", ";
	    // 	    msg += *var;
	    // 	}
	    // 	throw std::runtime_error( msg );
	    // }
	try {
	    if ( _open_model && _open_model.instantiate() ) {
		if ( debug_flag ) _open_model.debug(std::cout);
		ok = _open_model.solve( _closed_model );
		if ( debug_flag ) _open_model.print(std::cout);
	    }
	    if ( _closed_model && _closed_model.instantiate() ) {
		if ( debug_flag ) _closed_model.debug(std::cout);
		ok = _closed_model.solve( _solver );
		if ( debug_flag ) _closed_model.print(std::cout);
	    }
	}
	catch ( const std::runtime_error& error ) {
	    throw LQX::RuntimeException( error.what() );
	    ok = false;
	}
	catch ( const std::logic_error& error ) {
	    throw LQX::RuntimeException( error.what() );
	    ok = false;
	}
	catch ( const floating_point_error& error ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": floating point error - " << error.what() << std::endl;
	    LQIO::io_vars.error_count += 1;
	    ok = false;
	}
	return LQX::Symbol::encodeBoolean(ok);
    }
}
