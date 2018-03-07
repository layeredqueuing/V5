/*
 *  Environment.h
 *  ModLang
o *
 *  Created by Martin Mroz on 26/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include "SymbolTable.h"
#include "MethodTable.h"
#include <vector>
#include <cstdio>

namespace LQX {
  
  class Environment {
  public:
    
    /* Default Constructor/Destructor */
    Environment();
    virtual ~Environment();
    
  protected:
    
    /* This object is not copyable, and will throw an exception if you try */
    Environment(const Environment& other);
    Environment& operator=(const Environment& other);
    
  public:
    void setDefaultOutput( FILE * defaultOutput ) { _defaultOutput = defaultOutput; }
    FILE * getDefaultOutput() const { return _defaultOutput; }
    
    /* Obtaining the symbol table */
    SymbolTable* getSymbolTable() const;
    SymbolTable* getSpecialSymbolTable() const;
    MethodTable* getMethodTable() const;
    SymbolAutoRef invokeGlobalMethod(std::string name, std::vector<SymbolAutoRef >* arguments);
    
    /* Support for methods defined in-language */
    bool isExecutingInMainContext();
    void languageMethodPrologue();
    void languageMethodEpilogue();
    
  private:
    
    /* Runtime Environment Variables */
    std::vector<SymbolTable*> _functionEnvStack;
    SymbolTable* _runtimeVariables;
    SymbolTable* _specialVariables;
    MethodTable* _methodTable;
    FILE * _defaultOutput;		/* If output is not to a file, then... */
  };
  
}

#endif /* __ENVIRONMENT_H__ */
