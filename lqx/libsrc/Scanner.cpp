/*
 *  Scanner.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 14/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Scanner.h"
#include "Parser.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

struct yy_buffer_state;
extern LQX::ScannerToken* ml_scanner_token_value;
extern int ml_scanner_lineno;
extern int ml_scanner_lex(void);
extern void* ml_scanner__scan_string (const char *yy_str  );
extern void ml_scanner__delete_buffer (yy_buffer_state*  );

/* The shared scanner is the yylval union too */
static LQX::Scanner* sharedScanner = 0;

#pragma mark -

namespace LQX {
  
  ScannerToken::ScannerToken(int lineno, ParserToken code) : 
    _lineNumber(lineno), _tokenCode(code), _storedType(TYPE_NONE), _external(false)
  {
    /* Store nothing */
  }
  
  ScannerToken::ScannerToken(int lineno, ParserToken code, bool b) : 
    _lineNumber(lineno), _tokenCode(code), _storedType(TYPE_BOOLEAN), _external(false)
  {
    /* Store the boolean value */
    _storedValue.boolean = b;
  }
  
  ScannerToken::ScannerToken(int lineno, ParserToken code, double d) : 
    _lineNumber(lineno), _tokenCode(code), _storedType(TYPE_DOUBLE), _external(false)
  {
    /* Store the double value */
    _storedValue.doubleValue = d;
  }
  
  ScannerToken::ScannerToken(int lineno, ParserToken code, Type type, const char* string, unsigned int len, bool external) : 
    _lineNumber(lineno), _tokenCode(code), _storedType(type), _external(external)
  {
    /* Copy in the string for Identifiers and Strings */
    _storedValue.string = static_cast<char *>(malloc(len+1));
    strncpy( _storedValue.string, string, len );
    _storedValue.string[len] = '\0';
  }
  
  ScannerToken::ScannerToken(const ScannerToken& other)
  {
    /* Invoke the assignment operator */
    this->operator=(other);
  }
  
  ScannerToken::~ScannerToken()
  {
    /* Delete the stored string if we copied it over at some point */
    if (_storedType == TYPE_STRING || _storedType == TYPE_IDENTIFIER) {
      free(_storedValue.string);
    }
  }
  
  ScannerToken& ScannerToken::operator=(const ScannerToken& other)
  {
    /* Copy all of the token variables */
    _tokenCode = other._tokenCode;
    _storedType = other._storedType;
    if (_storedType == TYPE_DOUBLE) {
      _storedValue.doubleValue = other._storedValue.doubleValue;
    } else if (_storedType == TYPE_STRING || _storedType == TYPE_IDENTIFIER) {
      _storedValue.string = strdup(other._storedValue.string);
    } else if (_storedType == TYPE_BOOLEAN) {
      _storedValue.boolean = other._storedValue.boolean;
    }
    
    return *this;
  }
  
  unsigned ScannerToken::getLineNumber() const
  {
    return _lineNumber;
  }
  
  ParserToken ScannerToken::getTokenCode() const
  {
    return _tokenCode;
  }
  
  ScannerToken::Type ScannerToken::getStoredType() const
  {
    return _storedType;
  }
  
  double ScannerToken::getStoredDouble() const
  {
    /* Check the scanner token is the right type */
    assert(_storedType == TYPE_DOUBLE);
    return _storedValue.doubleValue;
  }
  
  const char* ScannerToken::getStoredString() const
  {
    /* Check the scanner token is the right type */
    assert(_storedType == TYPE_STRING);
    return _storedValue.string;
  }
  
  const char* ScannerToken::getStoredIdentifier() const
  {
    /* Check the scanner token is the right type */
    assert(_storedType == TYPE_IDENTIFIER);
    return _storedValue.string;
  }
  
  bool ScannerToken::getStoredBoolean() const
  {
    /* Check the scanner token is the right type */
    assert(_storedType == TYPE_BOOLEAN);
    return _storedValue.boolean;
  }
  
  bool ScannerToken::getIsExternal() const
  {
    /* Check the scanner token is the right type */
    assert(_storedType == TYPE_IDENTIFIER);
    return _external;
  }
  
#pragma mark -
  
  Scanner::Scanner() : _current(0, LQX::PT_EOS), _currentBuffer(NULL)
  { 
  }
  
  Scanner::~Scanner()
  {
  }
  
  Scanner* Scanner::getSharedScanner()
  {
    /* Allocates a new shared scanner */
    if (sharedScanner == 0) {
      sharedScanner = new Scanner();
    }
    
    /* Return the instance */
    return sharedScanner;
  }
  
  unsigned Scanner::getCurrentLineNumber()
  {
    /* Returns the current line number */
    return ml_scanner_lineno;
  }
  
  void Scanner::setStringBuffer(const char* inputString, const unsigned int lineno )
  {
    /* Set the current buffer for string scanning */
    _currentBuffer = ml_scanner__scan_string(inputString);
    ml_scanner_lineno = lineno;
  }
  
  void Scanner::deleteStringBuffer()
  {
    /* Clean up now that we've finished */
    ml_scanner__delete_buffer(static_cast<yy_buffer_state*>(_currentBuffer));
  }
  
  ScannerToken* Scanner::getNextToken()
  {
    /* Fetches the next token from stdin... */
    int majorCode = ml_scanner_lex();
    if (majorCode == 0 || ml_scanner_token_value == 0) {
      return new ScannerToken(ml_scanner_lineno, LQX::PT_EOS);
    } else {
      ScannerToken* thisToken = ml_scanner_token_value;
      ml_scanner_token_value = 0;
      return thisToken;
    }
  }
  
};
