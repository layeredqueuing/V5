/* -*- c++ -*-
 * runlqx.h	-- Greg Franks
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/runlqx.h $
 * ------------------------------------------------------------------------
 * $Id: runlqx.h 14427 2021-01-28 23:13:01Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _RUNLQX_H
#define _RUNLQX_H

#include <lqx/MethodTable.h>
#include <lqx/Environment.h>
#include "closedmodel.h"

class MVA;
	
namespace SolverInterface {

    /* solve() simply calls up to print_symbol_table() */
    class Solve : public LQX::Method {
    public:
	typedef bool (MVA::*solve_fptr)();
		
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Solve(ClosedModel& model, ClosedModel::Using solver ) : _model(model), _solver(solver) {}
	virtual ~Solve() {}
		
	/* All of the glue code to make sure LQX can call solve() */
	virtual std::string getName() const { return "solve"; } 
	virtual const char* getParameterInfo() const { return "1"; } 
	virtual std::string getHelp() const { return "Solves the model."; } 
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
	static bool solveCallViaLQX;
	static bool implicitSolve;
	static std::string customSuffix;
	
    private:
	ClosedModel& _model;
	ClosedModel::Using _solver;

	static unsigned int invocationCount;
    };
	
}
#endif 
