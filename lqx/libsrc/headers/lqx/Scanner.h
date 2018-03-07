/*
 *  Scanner.h
 *  ModLang
 *
 *  Created by Martin Mroz on 14/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __SCANNER_H__
#define __SCANNER_H__

#include "Parser.h"

namespace LQX {
  
  class ScannerToken {
  public:
    
    /* Type of the stored value */
    typedef enum Type {
      TYPE_NONE = 0,
      TYPE_DOUBLE = 1,
      TYPE_STRING = 2,
      TYPE_IDENTIFIER = 3,
      TYPE_BOOLEAN = 4
    } Type;
    
  public:
    
    /* Constructors and Destructor */
    ScannerToken(int lineno, ParserToken code);
    ScannerToken(int lineno, ParserToken code, bool b);
    ScannerToken(int lineno, ParserToken code, double d);
    ScannerToken(int lineno, ParserToken code, Type type, const char* string, unsigned int len, bool external);
    ScannerToken(const ScannerToken& other);
    virtual ~ScannerToken();
    
    /* Provide the assignment operator also */
    ScannerToken& operator=(const ScannerToken& other);
    
    /* Obtaining the Value */
    unsigned getLineNumber() const;
    ParserToken getTokenCode() const;
    Type getStoredType() const;
    double getStoredDouble() const;
    const char* getStoredString() const;
    const char* getStoredIdentifier() const;
    bool getStoredBoolean() const;
    
    /* For Identifiers Only */
    bool getIsExternal() const;
    
  private:
    
    /* Code, Type and Values */
    unsigned _lineNumber;
    ParserToken _tokenCode;
    Type _storedType;
    bool _external;
    union {
      double doubleValue;
      char* string;
      bool boolean;
    } _storedValue;
    
  };
  
  /* 
   * WARNING: LQX::Scanner is the very definition of not thread-safe. It uses global
   *   variables and a whole ton of stuff it probably shouldn't, but thats what you get
   *   directly from Flex. Thus, LQX::Scanner's methods are all static and no
   *   two tasks should involve it at any one time.
   */
  class Scanner {
  public:
    
    /* Virtual Destructor */
    virtual ~Scanner();
    
    /* Scanner itself is a singleton class */
    static Scanner* getSharedScanner();
    
  public:
    
    /* Obtaining a reference to the current token */
    unsigned getCurrentLineNumber();
    void setStringBuffer(const char* inputString, const unsigned int lineno=1);
    void deleteStringBuffer();
    ScannerToken* getNextToken();
    
  protected:
    
    /* Class is a Singleton */
    Scanner();
    
    /* This object is not copyable, and will throw an exception if you try */
    Scanner(const Scanner& other);
    Scanner& operator=(const Scanner& other);
    
  private:
    
    /* This is the Token Lvalue */
    ScannerToken _current;
    void* _currentBuffer;
    
  };
  
}

extern LQX::ScannerToken* ml_scanner_token_value;
extern int ml_scanner_lineno;
extern int ml_scanner_lex(void);

#endif /* __SCANNER_H__ */
