/*
 *  Program.h
 *  ModLang
 *
 *  Created by Martin Mroz on 18/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "ReferencedPointer.h"
#include "SymbolTable.h"
#include "RuntimeException.h"
#include <vector>
#include <string>
#include <iostream>

namespace LQX {
  
  /* Bits and pieces for programs */
  class SyntaxTreeNode;
  class Environment;
  class Symbol;
  
  class Program {
  public:
    
    /* Loading programs from various locations */
    static Program* loadFromFile(char* path);
    static Program* loadFromText(const char* filename, const unsigned line_number, const char* text);
    static Program* loadRawProgram(std::vector<SyntaxTreeNode*>* rawProgram);
    
  protected:
    
    /* Always use the provided Static Initializers for loading */
    Program(std::vector<SyntaxTreeNode*>* rawProgram, double compileTime);
    Program(const Program& other);
    Program& operator=(const Program& other);
    
  public:
    
    /* Designated Destructor */
    virtual ~Program();
    
    /* Interacting with the program */
    std::ostream& getGraphvizRepresentation(std::ostream&) const;
    std::ostream& print(std::ostream&) const;
    double getCompileTime() const;
    double getLastRunTime() const;
    bool invoke();
    
    /* Managing External Variables */
    SymbolAutoRef defineConstantVariable(const std::string& name);
    SymbolAutoRef defineExternalVariable(const std::string& name);
    SymbolAutoRef getSpecialVariable(const std::string& name);
    
    /* For more advanced stuff */
    Environment* getEnvironment() const;
    
  private:
    
    /* Instance Variables for the Program Wrapper */
    std::vector<SyntaxTreeNode*>* _program;
    Environment* _runEnvironment;
    double _compileTime;
    double _lastRunTime;
    
    struct printStatement {
      printStatement( std::ostream& output ) : _output( output ) {};
      void operator()( const LQX::SyntaxTreeNode* node ) const;
    private:
      std::ostream& _output;
    };

    struct printGraphviz {
      printGraphviz( std::ostream& output ) : _output( output ) {};
      void operator()( const LQX::SyntaxTreeNode* node ) const;
    private:
      std::ostream& _output;
    };

  };
  
}

#endif /* __PROGRAM_H__ */
