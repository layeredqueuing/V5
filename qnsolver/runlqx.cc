/* runlqx.h	-- Greg Franks
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/runlqx.cc $
 * ------------------------------------------------------------------------
 * $Id: runlqx.cc 16893 2023-12-09 19:29:22Z greg $
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include <iomanip>
#include <sstream>
#include <lqio/dom_document.h>
#include <lqio/qnio_document.h>
#include <lqx/Program.h>
#include <lqx/MethodTable.h>
#include <lqx/Environment.h>
#include <mva/fpgoop.h>
#include <mva/mva.h>
#include "runlqx.h"

extern bool debug_flag;

namespace SolverInterface 
{

    size_t Solve::invocationCount = 0;
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
			
	// /* Tell the world the iteration number */
	// } else if ( flags.verbose ) {
	//     std::cerr << "Solving iteration #" << invocationCount << "..." << std::endl;
	// }
			
	/* Make sure all external variables are accounted for */
	    
	const std::vector<std::string>& undefined = input().getUndefinedExternalVariables();
	if ( !undefined.empty() ) {
	    std::string msg = "The following external variables were not assigned at time of solve: ";
	    for ( std::vector<std::string>::const_iterator var = undefined.begin(); var != undefined.end(); ++var ) {
		if ( var != undefined.begin() ) msg += ", ";
		msg += *var;
	    }
	    throw std::runtime_error( msg );
	} else if ( input().getSymbolExternalVariableCount() > 0 ) {
	}
	
	return LQX::Symbol::encodeBoolean( compute( invocationCount ) );
    }
}


QNIO::Document& SolverInterface::Solve::input() const
{
    return _model._input;
}


bool
SolverInterface::Solve::compute( size_t invocationCount ) const
{
    return _model.compute( invocationCount );
}
