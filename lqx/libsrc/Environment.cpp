/*
 *  Environment.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 26/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cstdarg>
#include "Environment.h"
#include "Intrinsics.h"
#include "LQXStrings.h"
#include "Array.h"

namespace LQX {
  
  Environment::Environment() : _functionEnvStack(), _runtimeVariables(new SymbolTable()),
    _specialVariables(new SymbolTable()), _methodTable(new MethodTable()), _defaultOutput( stdout )
  {
    /* Register the intrinsic methods */
    RegisterIntrinsics(_methodTable);
    RegisterStrings(_methodTable);
    RegisterArrayClass(_methodTable);
    RegisterIntrinsicConstants(_specialVariables);

    /* Register the sole Newline symbol */
    _runtimeVariables->define( "endl" );
    SymbolAutoRef endl_symbol = _runtimeVariables->get( "endl" );
    endl_symbol->assignNewline();
  }
  
  Environment::~Environment()
  {
    /* Free any used space */
    delete _methodTable;
    delete _runtimeVariables;
    delete _specialVariables;
  }
  
  SymbolTable* Environment::getSymbolTable() const
  {
    /* Return the runtime variables */
    return _runtimeVariables;
  }
  
  SymbolTable* Environment::getSpecialSymbolTable() const
  {
    /* This is where External/Constants go */
    return _specialVariables;
  }
  
  MethodTable* Environment::getMethodTable() const
  {
    /* Return the method table */
    return _methodTable;
  }
  
  SymbolAutoRef Environment::invokeGlobalMethod(std::string name, std::vector<SymbolAutoRef >* arguments)
  {
    /* Don't do any type checking _yet_ */
    Method* method = _methodTable->getMethod(name);
    if (!method) throw RuntimeException("Invalid method specified: `%s'", name.c_str());
    std::vector<Symbol::Type> requiredTypes = method->getRequiredParameterTypes();
    
    /* We use an iterator to clean up checking */
    std::vector<SymbolAutoRef >::iterator argIterator = arguments->begin();
    std::vector<Symbol::Type>::iterator typeCheckIterator = requiredTypes.begin();
    int argCount = arguments->size();
    
    /* Make sure that the right number of arguments are going to be passed through */
    if (argCount < method->getMinimumParameterCount() || argCount > method->getMaximumParameterCount()) {
      throw ArgumentMismatchException(method->getName(), argCount, method->getMinimumParameterCount(), 
        method->getMaximumParameterCount());
    }
    
    /* Make sure the required types line up properly for the function call */
    for (typeCheckIterator = requiredTypes.begin(); typeCheckIterator != requiredTypes.end(); ++argIterator, ++typeCheckIterator) {
      if (*typeCheckIterator == Symbol::SYM_NULL) continue;
      if ((*argIterator)->getType() != *typeCheckIterator) {
        throw ArgumentMismatchException(method->getName(), (*argIterator)->getTypeName(), 
          Symbol::typeToString(*typeCheckIterator));
      }
    }
    
    /* Invoke the method confident in the knowledge all is well */
    return method->invoke(this, *arguments);
  }
  
  bool Environment::isExecutingInMainContext()
  {
    /* This means that we are in the right context */
    return (_functionEnvStack.size() == 0);
  }
  
  void Environment::languageMethodPrologue()
  {
    /* When we invoke a language method we do it in sandbox */
    _functionEnvStack.push_back(_runtimeVariables);
    _runtimeVariables = new SymbolTable();
  }
  
  void Environment::languageMethodEpilogue()
  {
    /* Delete the old table and restore */
    delete(_runtimeVariables);
    _runtimeVariables = _functionEnvStack.back();
    _functionEnvStack.pop_back();
  }
  
};
