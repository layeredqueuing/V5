/*
 *  MethodTable.h
 *  ModLang
 *
 *  Created by Martin Mroz on 26/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __METHODTABLE_H__
#define __METHODTABLE_H__

#include <string>
#include <vector>
#include <map>

/* For throwing exceptions here */
#include "RuntimeException.h"
#include "SymbolTable.h"

namespace LQX {
  
  /* Forward References */
  class LanguageObject;
  class Environment;
  
  /* The absolute maximum number of arguments */
  extern const int kMethodMaximumParameterCount;
  
  /*!
   * @class       Method
   * @abstract    The interface for all classes supplying methods to ModLang.
   * @discussion  Any global method stored in the method table is actually a subclass of
   *              the Method class, providing a name, an optional help string, an invocation
   *              function and a list of parameter info characters. These are as follows:
   *              b = boolean, d = double, s = string, a = any
   *              + = 0 or more of any
   */
  
  class Method {
  public:
    
    /* Designated Initializers */
    Method();
    virtual ~Method();
    
  protected:
    
    /* This object is not copyable, and will throw an exception if you try */
    Method(const Method& other);
    Method& operator=(const Method& other);
    
  public:
    
    /* Methods NOT overridden by subclasses */
    std::vector<Symbol::Type>& getRequiredParameterTypes();
    int getMinimumParameterCount();
    int getMaximumParameterCount();
    
    /* Methods overriden by subclasses */
    virtual std::string getName() const = 0;
    virtual std::string getHelp() const;
    virtual SymbolAutoRef invoke(Environment* env, std::vector<SymbolAutoRef >& args) = 0;
    
  protected:
    
    /* Obtain the parameter information */
    virtual const char* getParameterInfo() const;
    
  protected:
    
    /* Helper methods for cleanliness */
    bool decodeBoolean(std::vector<SymbolAutoRef >& args, int n);
    double decodeDouble(std::vector<SymbolAutoRef >& args, int n);
    const char* decodeString(std::vector<SymbolAutoRef >& args, int n);
    LanguageObject* decodeObject(std::vector<SymbolAutoRef >& args, int n);
    
  private:
    
    /* Used internally to build the cache */
    void cacheParameterInfo();
    
  private:
    
    /* Used by non-virtual members */
    bool _dataIsCached;
    int _minimumCount, _maximumCount;
    std::vector<Symbol::Type> _typeList;
    
  };
  
  /*!
   * @class       MethodTable
   * @abstract    A simple wrapper around the std::map for String to Method matching.
   * @discussion  This class is simply a wrapper around the internal storage relationship
   *              for the mapping of method names to Method instances used by the language
   *              in order to perform invocations.
   */
  
  class MethodTable {
  public:
    
    /* Designated Initializers */
    MethodTable();
    virtual ~MethodTable();
    
  protected:
    
    /* This object is not copyable, and will throw an exception if you try */
    MethodTable(const MethodTable& other);
    MethodTable& operator=(const MethodTable& other);
    
  public:
    
    /* Adding and Obtaining Methods */
    void registerMethod(Method* method);
    Method* getMethod(std::string& name);
    
  private:
    
    /* Instance Variable for Storing the Methods */
    std::map<std::string,Method*> _methods;
    
  };
  
}

/* Quick and Dirty Macro for Declaring Methods */
#define DeclareLanguageMethod(dec_name, params, name, help) \
class dec_name : public Method { \
public: \
  virtual std::string getName() const { return name; } \
  virtual const char* getParameterInfo() const { return params; } \
  virtual std::string getHelp() const { return help; } \
  virtual SymbolAutoRef invoke(Environment* env, std::vector<SymbolAutoRef >& args); \
};

/* Quick and Dirty Macro for Implementing Methods */
#define ImplementLanguageMethod(dec_name) \
SymbolAutoRef dec_name::invoke(Environment* env, std::vector<SymbolAutoRef >& args) \

#endif /* __METHODTABLE_H__ */
