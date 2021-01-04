/* runlqx.h	-- Greg Franks
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/runlqx.h $
 * ------------------------------------------------------------------------
 * $Id: runlqx.h 14319 2021-01-02 04:11:00Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _RUNLQX_H
#define _RUNLQX_H

#include <lqx/Program.h>
#include <lqx/MethodTable.h>
#include <lqx/Environment.h>

class Model;
	
namespace SolverInterface {

    /* solve() simply calls up to print_symbol_table() */
    class Solve : public LQX::Method {
    public: 
	typedef bool (Model::*solve_fptr)();
		
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Solve(LQIO::DOM::Document* document, solve_fptr solve, Model* aModel) :
	    _aModel(aModel), _solve(solve), _document(document) {}
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
	Model * _aModel;
	solve_fptr _solve;
	LQIO::DOM::Document* _document;

	static unsigned int invocationCount;
    };
	
}
#endif 
