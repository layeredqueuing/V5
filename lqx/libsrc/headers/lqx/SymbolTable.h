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
    static SymbolAutoRef duplicate( const SymbolAutoRef& source );
    
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

    /* Output */
    std::ostream& print( std::ostream& ) const;
    
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

    
    /* Output a single level of the syntax table */

    class DumpLevel {
	  
    public:
    DumpLevel(std::stringstream& ss) : _level(0), _ss(ss) {}
      void operator()( const std::map<std::string, SymbolAutoRef >& level );

    private:
      int _level;
      std::stringstream& _ss;
    };

  public:
    
    /* Constructor and Destructor */
    SymbolTable();
    virtual ~SymbolTable();
    
  public:
    
    /* Interface to the table */
    bool define(const std::string& name);
    bool isDefined(const std::string& name, bool globally=true) const;
    SymbolAutoRef get(const std::string& name);
    
    /* Variable Scoping */
    void pushContext();
    void popContext();
    
    /* Output debug data */
    void dump(std::stringstream& ss);
    
  private:
    
    /* Symbol table is non-copyable */
    SymbolTable(const SymbolTable& other) = delete;
    SymbolTable& operator=(const SymbolTable& other) = delete;
    
  private:
    
    /* The actual object storing the symbols */
    std::vector< std::map<std::string,SymbolAutoRef > > _stack;
    
  };

  inline std::ostream& operator<<( std::ostream& output, const SymbolAutoRef& symbol ) { return symbol->print(output); }

}

#endif /* __SYMBOL_TABLE_H__ */
