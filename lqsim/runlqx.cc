/* runlqx.h	-- Greg Franks
 *
  * ------------------------------------------------------------------------
 * $Id: runlqx.cc 15312 2022-01-01 14:19:39Z greg $
 * ------------------------------------------------------------------------
 */

#include <lqio/dom_bindings.h>
#include <lqio/dom_document.h>
#include <lqx/Program.h>
#include <lqx/MethodTable.h>
#include <lqx/Environment.h>
#include <iomanip>
#include <sstream>
#include "runlqx.h"
#include "model.h"

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

	/* Tell the world the iteration number */
	if ( verbose_flag ) {
	    std::cerr << "Solving iteration #" << invocationCount << std::endl;
	}

	/* Make sure all external variables are accounted for */
	const std::vector<std::string>& undefined = _document->getUndefinedExternalVariables();
	if ( undefined.size() > 0) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": The following external variables were not assigned at time of solve: ";
	    for ( std::vector<std::string>::const_iterator var = undefined.begin(); var != undefined.end(); ++var ) {
		if ( var != undefined.begin() ) std::cerr << ", ";
		std::cerr << *var << std::endl;
	    }
	    std::cerr << std::endl;
	    LQIO::io_vars.error_count += 1;
	    return LQX::Symbol::encodeBoolean(false);
	}

	/* Recalculate dynamic values */
	//recalculateDynamicValues();

	/* Run the solver and return its success as a boolean value */
	bool ok = false;
	try {
	    assert (_aModel );
	    _document->setResultInvocationNumber(invocationCount);
	    ok = (_aModel->*_solve)();
	}
	catch ( const std::runtime_error & error ) {
	    throw LQX::RuntimeException( error.what() );
	}
	catch ( const std::logic_error& error ) {
	    throw LQX::RuntimeException( error.what() );
	}
	return LQX::Symbol::encodeBoolean(ok);
    }
}
