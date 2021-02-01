/* -*- c++ -*-
 * runlqx.h	-- Greg Franks
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/runlqx.h $
 * ------------------------------------------------------------------------
 * $Id: runlqx.h 14435 2021-02-01 03:05:05Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _RUNLQX_H
#define _RUNLQX_H

#include <lqx/MethodTable.h>
#include <lqx/Environment.h>
#include "closedmodel.h"
#include "openmodel.h"

class MVA;
	
namespace SolverInterface {

    /* solve() simply calls up to print_symbol_table() */
    class Solve : public LQX::Method {
    public:
	typedef bool (MVA::*solve_fptr)();
		
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Solve(ClosedModel& closed_model, ClosedModel::Using solver, OpenModel& open_model ) : _closed_model(closed_model), _solver(solver), _open_model(open_model) {}
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
	ClosedModel& _closed_model;
	const ClosedModel::Using _solver;
	OpenModel& _open_model;

	static unsigned int invocationCount;
    };
	
}
#endif 
