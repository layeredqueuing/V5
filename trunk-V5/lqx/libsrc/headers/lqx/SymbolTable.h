/*
 *  SymbolTable.h
 *  ModLang
 *
 *  Created by Martin Mroz on 14/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

#include "ReferenceCountedObject.h"
#include "ReferencedPointer.h"
#include "RuntimeException.h"
#include <string>
#include <map>
#include <vector>

namespace LQX {
  
  /* Foreign class references */
  class LanguageObject;
  class Symbol;
  
  /* A cleaner way of referring to a ReferencedPointer to a Symbol */
  typedef ReferencedPointer<Symbol> SymbolAutoRef;
  
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  
  class Symbol : public ReferenceCountedObject {
    friend class SymbolTable;
    
  public:
    
    /* Convenient methods for creating proper symbols */
    static SymbolAutoRef encodeNull();
    static SymbolAutoRef encodeBoolean( bool value );
    static SymbolAutoRef encodeDouble( double value );
    static SymbolAutoRef encodeString( const char* value, bool release=false );
    static SymbolAutoRef encodeObject( LanguageObject* object, bool derefWhenDone=false );
    static SymbolAutoRef duplicate( SymbolAutoRef& source );
    
  public:
    
    /* Type of the current symbol */
    typedef enum Type {
      SYM_UNINITIALIZED,
      SYM_BOOLEAN,
      SYM_DOUBLE,
      SYM_STRING,
      SYM_OBJECT,
      SYM_FILE_WRITE_POINTER,
      SYM_FILE_READ_POINTER,
      SYM_NEWLINE,
      SYM_NULL
    } Type;
    
  public:
    
    /* Obtain the human-readable name for the type */
    static const std::string& typeToString(Type t);
    
  public:
    
    /* Constructor and Destructor */
    Symbol();
    virtual ~Symbol();
    
    /* Accessing and mutating symbols */
    void copyValue( const Symbol& other );
    void assignBoolean( bool value );
    void assignDouble( double value );
    void assignString( const char* value );
    void assignObject( LanguageObject* value );
    void assignFileWritePointer( FILE* value );
    void assignFileReadPointer( FILE* value );
    void assignNewline() { _symbolType = SYM_NEWLINE; }
    void assignNull();
    bool getBooleanValue() const;
    double getDoubleValue() const;
    const char* getStringValue() const;
    LanguageObject* getObjectValue() const;
    FILE* getFilePointerValue() const;
    Type getType() const;
    
    /* Setting the constant flag */
    void setIsConstant(const bool flag);
    bool isConstant() const;
    
    /* Operators for testing value (not constness) equality */
    bool operator<(const Symbol& other) const;
    bool operator==(const Symbol& other) const;
    
    /* Obtaining the name of a type */
    const std::string& getTypeName() const;
    std::string description() const;
    
  public:
    
    /* Implicit assignment operator/ctor */
    Symbol(const Symbol& other);
    Symbol& operator=(const Symbol& other);
    
  private:
    
    /* Symbol Values */
    Type _symbolType;
    union {
      bool booleanValue;
      double doubleValue;
      const char* stringValue;
      LanguageObject* objectValue;
      FILE* filePointerValue;
    } _storedValue;
    bool _isConstant;
    
  };
  
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  
  class SymbolTable {
  public:
    
    /* Constructor and Destructor */
    SymbolTable();
    virtual ~SymbolTable();
    
  public:
    
    /* Interface to the table */
    bool define(std::string name);
    bool isDefined(std::string name, bool globally=true);
    SymbolAutoRef get(std::string name);
    
    /* Variable Scoping */
    void pushContext();
    void popContext() throw (RuntimeException);
    
    /* Output debug data */
    void dump(std::stringstream& ss);
    
  protected:
    
    /* Output a single level of the syntax table */
    void dumpLevel(std::string name, std::stringstream& ss, std::map<std::string, SymbolAutoRef >& level);
    
  protected:
    
    /* Symbol table is non-copyable */
    SymbolTable(const SymbolTable& other) throw ();
    SymbolTable& operator=(const SymbolTable& other) throw ();
    
  private:
    
    /* The actual object storing the symbols */
    std::vector< std::map<std::string,SymbolAutoRef > > _stack;
    
  };
  
}

#endif /* __SYMBOL_TABLE_H__ */
