/*
 *  Parser.h
 *  ModLang
 *
 *  Created by Martin Mroz on 14/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <vector>

/* Warning - this file is not automatically generated.  The enumeration must match the Parser_pre.ypp and Parser.cpp */

namespace LQX {
  
  /* Forward References */
  class SyntaxTreeNode;
  class ScannerToken;

  
  typedef enum ParserToken {
    PT_EOS = 0,
    
    /* Immediate Values */
    PT_BOOLEAN,
    PT_STRING,
    PT_DOUBLE,
    PT_IDENTIFIER,
    
    /* Control Flow Words */
    PT_FOR,
    PT_FOREACH,
    PT_IN,
    PT_WHILE,
    PT_BREAK,
    PT_IF,
    PT_ELSE,
    PT_RETURN,
    PT_SELECT,

    /* File Output Words */
    PT_FILE_WRITE,
    PT_FILE_READ,
    PT_FILE_APPEND,
    PT_READ_LOOP,
    PT_FILE_OPEN,
    PT_FILE_PRINT,
    PT_FILE_PRINTLN,
    PT_FILE_PRINT_SP,
    PT_FILE_PRINTLN_SP,
    PT_FILE_CLOSE,
    PT_READ_DATA,

    /* Member Access / Structure */
    PT_OPEN_BRACKET,
    PT_CLOSE_BRACKET,
    PT_OPEN_BRACE,
    PT_CLOSE_BRACE,
    PT_OPEN_SQUARE,
    PT_CLOSE_SQUARE,
    PT_DOT,
    
    /* Math Operators */
    PT_EXP,
    PT_STAR,
    PT_SLASH,
    PT_PLUS,
    PT_MINUS,
    PT_MOD,
    PT_LEFTSHIFT,
    PT_RIGHTSHIFT,
    
    /* Boolean Operations */
    PT_LOGIC_OR,
    PT_LOGIC_AND,
    PT_LOGIC_NOT,
    
    /* Relational Operators */
    PT_EQUALS_EQUALS,
    PT_NOT_EQUALS,
    PT_LESS_THAN,
    PT_GREATER_THAN,
    PT_LESS_EQUAL,
    PT_GREATER_EQUAL,
    
    /* Assignment Operators */
    PT_EQUALS,
    PT_EXP_EQUALS,
    PT_STAR_EQUALS,
    PT_SLASH_EQUALS,
    PT_PLUS_EQUALS,
    PT_MINUS_EQUALS,
    PT_MOD_EQUALS,
    PT_LEFTSHIFT_EQUALS,
    PT_RIGHTSHIFT_EQUALS,
    
    /* Mixed, New Variables */
    PT_SEMICOLON,
    PT_COMMA,
    PT_MAPS,
    PT_NULL,
    
    PT_FUNCTION,
    PT_ELLIPSIS,

    /* An invalid token */
    PT_ERROR
    
  } ParserToken;
  
  const char* ParserTokenToName(ParserToken token);
  
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  
  class Parser {
  public:
    
    Parser();
    virtual ~Parser();
    
  public:
    
    /* Public interface to the parser */
    bool processToken(LQX::ScannerToken* value);
    unsigned getSyntaxErrorLineNumber() const;
    const std::string& getLastSyntaxErrorMessage() const;
    bool getSyntaxErrorDidOccur() const;
    void clearSyntaxError();
    
  protected:
    
    /* This object is not copyable, and will throw an exception if you try */
    Parser(const Parser& other);
    Parser& operator=(const Parser& other);
    
  public:
    
    /* Callbacks from the Lemon Parser */
    void parseFailed();
    void syntaxError(LQX::ScannerToken* problemToken);
    void parseAccepted();
    
    /* Setting and Getting the Root Statement List */
    void setRootStatementList(std::vector<SyntaxTreeNode*>* rootList);
    std::vector<SyntaxTreeNode*>* getRootStatementList();
    
  private:
    
    /* Actual parser instance */
    unsigned _syntaxErrorLineno;
    bool _syntaxErrorDidOccur;
    std::string _syntaxErrorMessage;
    std::vector<SyntaxTreeNode*>* _rootStatementList;
    void* _parser;
    
  };
  
}

#endif /* __PARSER_H__ */
