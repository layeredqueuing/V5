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
#include <vector>
#include <string>

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
    Program(const Program& other) throw ();
    Program& operator=(const Program& other) throw ();
    
  public:
    
    /* Designated Destructor */
    virtual ~Program();
    
    /* Interacting with the program */
    std::string getGraphvizRepresentation() const;
    double getCompileTime() const;
    double getLastRunTime() const;
    bool invoke();
    
    /* Managing External Variables */
    SymbolAutoRef defineConstantVariable(std::string name);
    SymbolAutoRef defineExternalVariable(std::string name);
    SymbolAutoRef getSpecialVariable(std::string name);
    
    /* For more advanced stuff */
    Environment* getEnvironment() const;
    
  private:
    
    /* Instance Variables for the Program Wrapper */
    std::vector<SyntaxTreeNode*>* _program;
    Environment* _runEnvironment;
    double _compileTime;
    double _lastRunTime;
    
  };
  
}

#endif /* __PROGRAM_H__ */
