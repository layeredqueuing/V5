/*
 *  MethodTable.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 26/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "NonCopyable.h"
#include "MethodTable.h"
#include "SymbolTable.h"
#include "Environment.h"
#include "LanguageObject.h"

namespace LQX {
  
  const int kMethodMaximumParameterCount = 10000;
  
  Method::Method() : _dataIsCached(false), _minimumCount(0), _maximumCount(0), _typeList()
  {
  }
  
  Method::~Method()
  {
  }
  
  MethodTable::MethodTable(const MethodTable&)
  {
    throw NonCopyableException();
  }
  
  MethodTable& MethodTable::operator=(const MethodTable&)
  {
    throw NonCopyableException();
  }
  
  std::vector<Symbol::Type>& Method::getRequiredParameterTypes()
  {
    /* This data is auto-cached */
    this->cacheParameterInfo();
    return _typeList;
  }
  
  int Method::getMinimumParameterCount() 
  {
    /* This data is auto-cached */
    this->cacheParameterInfo();
    return _minimumCount;
  }
  
  int Method::getMaximumParameterCount() 
  {
    /* This data is auto-cached */
    this->cacheParameterInfo();
    return _maximumCount;
  }
  
  std::string Method::getHelp() const
  {
    /* Default answer is to provide no help for this */
    return std::string("No help available.");
  }
  
  const char* Method::getParameterInfo() const
  {
    /* Default is 0 args */
    return "";
  }
  
  bool Method::decodeBoolean(std::vector<SymbolAutoRef >& args, int n)
  {
    /* Get the symbol from the list */
    SymbolAutoRef symbol = args[n];
    if (symbol->getType() != Symbol::SYM_BOOLEAN)
      throw IncompatibleTypeException(symbol->getTypeName(), Symbol::typeToString(Symbol::SYM_BOOLEAN));
    return symbol->getBooleanValue();
  }
  
  double Method::decodeDouble(std::vector<SymbolAutoRef >& args, int n)
  {
    /* Get the symbol from the list */
    SymbolAutoRef symbol = args[n];
    if (symbol->getType() != Symbol::SYM_DOUBLE) 
      throw IncompatibleTypeException(symbol->getTypeName(), Symbol::typeToString(Symbol::SYM_DOUBLE));
    return symbol->getDoubleValue();
  }
  
  const char* Method::decodeString(std::vector<SymbolAutoRef >& args, int n)
  {
    /* Get the symbol from the list */
    SymbolAutoRef symbol = args[n];
    if (symbol->getType() != Symbol::SYM_STRING) 
      throw IncompatibleTypeException(symbol->getTypeName(), Symbol::typeToString(Symbol::SYM_STRING));
    return symbol->getStringValue();
  }
  
  LanguageObject* Method::decodeObject(std::vector<SymbolAutoRef >& args, int n)
  {
    /* Get the symbol from the list */
    SymbolAutoRef symbol = args[n];
    if (symbol->getType() != Symbol::SYM_OBJECT) 
      throw IncompatibleTypeException(symbol->getTypeName(), Symbol::typeToString(Symbol::SYM_OBJECT));
    return symbol->getObjectValue();
  }
  
  void Method::cacheParameterInfo()
  {
    /* No sense re-doing work */
    if (_dataIsCached) {
      return;
    } else {
      _dataIsCached = true;
    }
    
    /* Obtain the parameter info we are going to step through */
    const char* letters = this->getParameterInfo();
    bool done = false;
    int i = 0;
    
    /* Walk over the list and build up the data */
    while (letters[i] != '\0' && !done) {
      switch (letters[i++]) {
        case '+': 
          _maximumCount = kMethodMaximumParameterCount;
          done = true;
          break;
        case '1':
          _typeList.push_back(Symbol::SYM_NULL);
          _maximumCount++;
          done = true;
          break;
        case 'b':
          _typeList.push_back(Symbol::SYM_BOOLEAN);
          _minimumCount++; _maximumCount++;
          break;
        case 'd':
          _typeList.push_back(Symbol::SYM_DOUBLE);
          _minimumCount++; _maximumCount++;
          break;
        case 's':
          _typeList.push_back(Symbol::SYM_STRING);
          _minimumCount++; _maximumCount++;
          break;
        case 'o':
          _typeList.push_back(Symbol::SYM_OBJECT);
          _minimumCount++; _maximumCount++;
          break;
        case 'a':
          _typeList.push_back(Symbol::SYM_NULL);
          _minimumCount++; _maximumCount++;
          break;
      }
    }
  }
  
};

namespace LQX {
  
  MethodTable::MethodTable() : _methods()
  {
  }
  
  MethodTable::~MethodTable()
  {
    /* Clean up any dangling values */
    std::map<std::string,Method*>::iterator iter;
    for (iter = _methods.begin(); iter != _methods.end(); ++iter) {
      delete(iter->second);
    }
  }
  
  bool MethodTable::isDefined(const std::string& name)
  {
    return _methods.find(name) != _methods.end();
  }

  void MethodTable::registerMethod(Method* method)
  {
    _methods[method->getName()] = method;
  }
  
  Method* MethodTable::getMethod(const std::string& name)
  {
    std::map<std::string,Method*>::iterator method = _methods.find(name);
    if ( method != _methods.end() ) return method->second;
    return nullptr;
  }
  
};
