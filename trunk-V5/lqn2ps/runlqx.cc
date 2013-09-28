/* -*- C++ -*-
 * $HeadURL$
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

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
    bool Solve::implicitSolve = false;

    LQX::SymbolAutoRef Solve::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) throw (LQX::RuntimeException) 
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
			
	/* Make sure all external variables are accounted for */
	if (!_document->areAllExternalVariablesAssigned()) {
	    cerr << io_vars.lq_toolname << ": Not all external variables are assigned at time of solve." << endl;
	    cerr << io_vars.lq_toolname << ": Solve was not invoked." << endl;
	    return LQX::Symbol::encodeBoolean(false);
	}
			
	/* Recalculate dynamic values */
	//recalculateDynamicValues();
			
	/* Run the solver and return its success as a boolean value */
	try {
	    assert (_aModel );
	    _document->setResultInvocationNumber(invocationCount);
	    const bool ok = (_aModel->*_solve)();
	    return LQX::Symbol::encodeBoolean(ok);
	}
	catch ( runtime_error & error ) {
	    cerr << io_vars.lq_toolname << ": runtime error - " << error.what() << endl;
	    io_vars.anError = true;
	}
	catch ( logic_error& error ) {
	    cerr << io_vars.lq_toolname << ": logic error - " << error.what() << endl;
	    io_vars.anError = true;
	}
	return LQX::Symbol::encodeBoolean(false);
    }
}
