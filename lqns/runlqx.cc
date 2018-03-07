/* runlqx.h	-- Greg Franks
 *
 * $URL$
 * ------------------------------------------------------------------------
 * $Id: runlqx.cc 13200 2018-03-05 22:48:55Z greg $
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
#include "lqns.h"
#include "fpgoop.h"

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
	if ( flags.trace_mva ) {
	    cout << "\fSolving iteration #" << invocationCount << endl;
	} else if ( flags.verbose ) {
	    cerr << "Solving iteration #" << invocationCount << "..." << endl;
	}
			
	/* Make sure all external variables are accounted for */
	const std::vector<std::string>& undefined = _document->getUndefinedExternalVariables();
	if ( undefined.size() > 0) {
	    cerr << io_vars.lq_toolname << ": The following external variables were not assigned at time of solve: ";
	    for ( std::vector<std::string>::const_iterator var = undefined.begin(); var != undefined.end(); ++var ) {
		if ( var != undefined.begin() ) cerr << ", ";
		cerr << *var << endl;
	    }
	    cerr << endl;
	    io_vars.anError = true;
	    return LQX::Symbol::encodeBoolean(false);
	}
			
	/* Recalculate dynamic values */
	Model::recalculateDynamicValues( _document );
			
	/* Run the solver and return its success as a boolean value */
	try {
	    assert( _aModel );
	    std::stringstream ss;
	    _document->printExternalVariables( ss );
	    _document->setModelComment( ss.str() );

	    _aModel->InitializeModel();
	    _document->setResultInvocationNumber( invocationCount );
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
	catch ( floating_point_error& error ) {
	    cerr << io_vars.lq_toolname << ": floating point error - " << error.what() << endl;
	    io_vars.anError = true;
	}
	catch ( exception_handled& error ) {
	    io_vars.anError = true;
	}
			
	return LQX::Symbol::encodeBoolean(false);
    }
		
}
