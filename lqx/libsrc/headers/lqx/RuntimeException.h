/*
 *  RuntimeException.h
 *  ModLang
 *
 *  Created by Martin Mroz on 22/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __RUNTIME_EXCEPTION_H__
#define __RUNTIME_EXCEPTION_H__

#include <string>
#include <exception>

namespace LQX {
  
  class SyntaxTreeNode;

  class RuntimeException : public std::exception {
  public:
    
    /* Designated initializer and destructor */
    RuntimeException(std::string format, ...) throw();
    virtual ~RuntimeException() throw();
    
    /* Access to the instance variables */
    virtual const char* what() const throw();

  protected:
    std::string& insert( size_t pos, const std::string& str );
    
  private:
    
    /* Instance Variables */
    std::string _reason;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class UndefinedVariableException : public RuntimeException {
  public:
    UndefinedVariableException(std::string& variableName) throw();
    virtual ~UndefinedVariableException() throw();
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class IncompatibleTypeException : public RuntimeException {
  public:
    IncompatibleTypeException(const SyntaxTreeNode* expr, const std::string& from, const std::string& to) throw();
    IncompatibleTypeException(const std::string& from, const std::string& to) throw();
    IncompatibleTypeException(const std::string& name) throw();
    virtual ~IncompatibleTypeException() throw();
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class InternalErrorException : public RuntimeException {
  public:
    InternalErrorException(std::string message) throw();
    virtual ~InternalErrorException() throw();
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class ArgumentMismatchException : public RuntimeException {
  public:
    ArgumentMismatchException(const std::string& name, int has, int min, int max) throw();
    ArgumentMismatchException(const std::string& name, const std::string& is, const std::string& shouldBe) throw();
    virtual ~ArgumentMismatchException() throw();
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

  class AbortException : public RuntimeException {
  public:
    AbortException(const std::string& whyAbort, double code) throw();
    virtual ~AbortException() throw();
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

  class InvalidPropertyException : public RuntimeException {
  public:
    InvalidPropertyException(const std::string& typeName, const std::string& propertyName) throw();
    virtual ~InvalidPropertyException() throw();
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class IndexNotValidException : public RuntimeException {
  public:
    IndexNotValidException(const std::string& keyDesc, const std::string& arrayDesc) throw();
    virtual ~IndexNotValidException() throw();
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class InvalidArgumentException : public RuntimeException {
  public:
    InvalidArgumentException(const std::string& keyDesc, const std::string& arrayDesc) throw();
    virtual ~InvalidArgumentException() throw();
  };
  
}

#endif /* __RUNTIME_EXCEPTION_H__ */
