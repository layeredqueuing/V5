/*
 *  main.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 18/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include <cstring>
#include "lqx/Program.h"
#include "lqx/SymbolTable.h"
#include "lqx/Array.h"
using namespace LQX;

extern void ModLangParserTrace(FILE *TraceFILE, char *zTracePrompt);

int run(int argc, char** argv)
{
//  ModLangParserTrace(stderr, "lqx:");

  /* Output a usage */
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " path" << std::endl;
    return 1;
  }
  
  /* Generate, invoke and clean up a program */
  Program* program = Program::loadFromFile(argv[1]);
  if ( !program ) {
      return 1;
  }
  
  /* Pass in some variables for the first time ever */
  const std::string xargc("$argc");
  const std::string xargv("$argv");
  SymbolAutoRef argCount = program->defineExternalVariable(xargc);
  SymbolAutoRef argArray = program->defineExternalVariable(xargv);
  
  /* Define the argument count */
  argCount->assignDouble(argc);
  
  /* Prepare the argument array */
  ArrayObject* obj = new ArrayObject();
  argArray->assignObject(obj);
  obj->dereference();
  for (int i = 0; i < argc; i++) {
    obj->put(Symbol::encodeDouble(i), Symbol::encodeString(argv[i]));
  }
  
  /* Run the program */  
  program->invoke();
  
  /* Output statistics */
  if (argc < 3 || (argc >= 3 && strcmp(argv[2], "-silent"))) {
    std::cout << "--- Compile Time: " << program->getCompileTime() << "s" << std::endl;
    std::cout << "--- Running Time: " << program->getLastRunTime() << "s" << std::endl;
  }
  
  /* Clean it up... */
  delete(program);
  return 0;
}

int main(int argc, char** argv)
{
  int rv = run(argc, argv);
  return rv;
}
