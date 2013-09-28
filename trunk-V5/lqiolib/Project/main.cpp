/*
 *  main.cpp
 *  libsrvnio2
 *
 *  Created by Martin Mroz on 23/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdio.h>
#include <lqio/dom_document.h>

#include <lqx/Program.h>
#include <lqx/MethodTable.h>
#include <lqx/Environment.h>

static LQIO::DOM::Document* result = NULL;

namespace Testing {
  
  /* solve() simply calls up to print_symbol_table() */
  class Solve : public LQX::Method {
  public: 
    virtual std::string getName() const { return "solve"; } 
    virtual const char* getParameterInfo() const { return ""; } 
    virtual std::string getHelp() const { return "Solves the model."; } 
    virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, 
      std::vector<LQX::SymbolAutoRef >& args) throw (LQX::RuntimeException) {
      std::vector<LQX::SymbolAutoRef > emptyArguments;
      std::string name = "print_symbol_table";
      env->invokeGlobalMethod(name, &emptyArguments);
      return LQX::Symbol::encodeBoolean(true);
    }
  };
  
};

int main(int argc, char** argv)
{
  /* All of the inputs and outputs to the LoadDocument call */
  char* filename = "/Users/martin/Projects/Work/trunk/lqns2/regression/71-fair.lqnx";
  FILE* fp = fopen(filename, "rb");
  unsigned errorCode = 0;
  
  /* Interface glue code */
  lqio_params_stats io_vars = { 
    /* tot_. */ 0,  0,  0,  0, 
    /* tooln */ "lqio-test",
    /* ver   */ "1",
    /* cmdln */ "",
    /* sevac */ (void (*)(unsigned int))exit,
    /* error */ 0,
    /* errcnt*/ 0,
    /* sevel */ LQIO::ADVISORY_ONLY,
    /* anerr */ 0,
    /* yylnn */ 0
  };
	
  /* Load the DOM::Document for the given test input file */
  LQIO::SetIOVariables(&io_vars);
  if(LQIO::LoadDocument(&result, fp, filename, &errorCode) == false) {
    printf("FAILED TO LOAD!\n");
    return 1;
  }
	  
  /* Let everyone know we are moving onto stage 2 */
  printf("\n\n--- Preparing LQX Program for Execution --- \n\n");
  
  /* Compile the LQX Program from the Document */
  LQX::Program* program = LQX::Program::loadFromText(filename, result->getLQXProgram());
  if (program == NULL) { return 0; }
  program->getEnvironment()->getMethodTable()->registerMethod(new Testing::Solve());
  result->registerExternalSymbolsWithProgram(program);
  program->invoke();
  
  /* Display the run statistics */
  printf("\n-- Compile  time: %f seconds.\n", program->getCompileTime());
  printf("-- Last run time: %f seconds.\n", program->getLastRunTime());

  /* Release any used memory */
  delete(program); program = NULL;
  delete(result); result = NULL; 
  return 0;
}
