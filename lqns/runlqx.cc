/* runlqx.h	-- Greg Franks
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/runlqx.cc $
 * ------------------------------------------------------------------------
 * $Id: runlqx.cc 17281 2024-09-12 15:21:59Z greg $
 * ------------------------------------------------------------------------
 */

#include "lqns.h"
#include <iomanip>
#include <sstream>
#include <numeric>
#include <lqio/dom_document.h>
#include <lqx/Program.h>
#include <lqx/MethodTable.h>
#include <lqx/Environment.h>
#include <mva/fpgoop.h>
#include "runlqx.h"
#include "model.h"
#include "option.h"

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
	env->cleanInvokeGlobalMethod("print_symbol_table", nullptr);
#endif

	/* Tell the world the iteration number */
	if ( Options::Trace::mva() ) {
	    std::cout << "\fSolving iteration #" << invocationCount << std::endl;
	} else if ( Options::Trace::verbose() ) {
	    std::cerr << "Solving iteration #" << invocationCount << "..." << std::endl;
	}

	/* Make sure all external variables are accounted for */
	bool ok = false;
	try {
	    const std::vector<std::string>& undefined = _document->getUndefinedExternalVariables();
	    if ( undefined.size() > 0) {
		const std::string msg = "The following external variables were not assigned at time of solve: "
		    + std::accumulate( std::next(undefined.begin()), undefined.end(), undefined.front(), &fold );
		throw std::runtime_error( msg );
	    }

	    /* Recalculate dynamic values */
	    _model->recalculateDynamicValues();

	    /* Run the solver and return its success as a boolean value */
	    assert( _model );
	    if ( _model->check() && _model->initialize() ) {
		_document->setResultInvocationNumber( invocationCount );
		ok = (_model->*_solve)();
	    } else {
		ok = false;
	    }
	}
	catch ( const std::range_error& error ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": range error - " << error.what() << std::endl;
	    LQIO::io_vars.error_count += 1;
	    ok = false;
	}
	catch ( const floating_point_error& error ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": floating point error - " << error.what() << std::endl;
	    LQIO::io_vars.error_count += 1;
	    ok = false;
	}
	catch ( const std::runtime_error& error ) {
	    throw LQX::RuntimeException( error.what() );
	}
	catch ( const std::logic_error& error ) {
	    throw LQX::RuntimeException( error.what() );
	}
	return LQX::Symbol::encodeBoolean(ok);
    }

    std::string SolverInterface::Solve::fold( const std::string& s1, const std::string& s2 ) { return s1 + "," + s2; }
}
