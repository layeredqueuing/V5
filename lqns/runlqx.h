/* runlqx.h	-- Greg Franks
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/runlqx.h $
 * ------------------------------------------------------------------------
 * $Id: runlqx.h 14785 2021-06-09 14:03:54Z greg $
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
	Solve(LQIO::DOM::Document* document, solve_fptr solve, Model* model) :

	_document(document), _solve(solve), _model(model) {}
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
	static std::string fold( const std::string& s1, const std::string& s2 );
	
    private:
	LQIO::DOM::Document* _document;
	solve_fptr _solve;
	Model * _model;

	static unsigned int invocationCount;
    };
}
#endif 
