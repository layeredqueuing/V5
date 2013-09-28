/*
 *  Program.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 18/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "NonCopyable.h"
#include "Program.h"
#include "Scanner.h"
#include "Parser.h"
#include "Environment.h"
#include "SyntaxTree.h"

#include <cstdlib>
#include <cstdio>
#include <sys/time.h>

/* C++ Headers */
#include <iostream>

namespace LQX {

Program* Program::loadFromText(const char* filename, const unsigned line_number, const char* text)
  {
    /* Obtain the file processing time */
    struct timeval start_tv;
    struct timeval end_tv;
    double difference;
    bool errorOccurred = false;
    
    /* Obtain the current time */
    gettimeofday(&start_tv, NULL);
    
    /* Run the Parser and the Scanner */
    std::vector<LQX::ScannerToken*> trashHeap;
    LQX::Scanner* scanner = LQX::Scanner::getSharedScanner();
    LQX::Parser* parser = new LQX::Parser();
    scanner->setStringBuffer(text, line_number);
    
    /* Scan the tokens until end of line */
    while (LQX::ScannerToken* st = scanner->getNextToken()) {
      trashHeap.push_back(st);
      
      /* We ran into some serious trouble scanning text */
      if (st->getTokenCode() == LQX::PT_ERROR) {
        std::cerr << filename << ":" << scanner->getCurrentLineNumber() << ": ";
        std::cerr << st->getStoredString() << std::endl;
        errorOccurred = true;
        break;
      }
      
      /* All is well, process the token */
      if (!parser->processToken(st)) {
        std::cerr << filename << ":" << parser->getSyntaxErrorLineNumber() << ": ";
        std::cerr << parser->getLastSyntaxErrorMessage() << std::endl;
        errorOccurred = true;
        break;
      } else if (st->getTokenCode() == LQX::PT_EOS) {
        break;
      }
    }
    
    /* Clean up the buffer now that we're done */
    scanner->deleteStringBuffer();
    
    /* Now that we're done, clean it up */
    std::vector<LQX::ScannerToken*>::iterator iter;
    for (iter = trashHeap.begin(); iter != trashHeap.end(); ++iter) {
      LQX::ScannerToken* st = *iter;
      if (st) { delete (st); }
    }
    
    /* Compute time it took to compile */
    gettimeofday(&end_tv, NULL);
    difference = (double)(end_tv.tv_sec - start_tv.tv_sec) + (double)((end_tv.tv_usec - start_tv.tv_usec) * 10e-6f);
    
    /* There was a problem... */
    if (errorOccurred) {
      return NULL;
    }
    
    /* Obtain the fruits of our labor and release the parser */
    std::vector<LQX::SyntaxTreeNode*>* stmtList = parser->getRootStatementList();
    delete(parser);
    
    /* Obviously not so well :) */
    if (stmtList == NULL) {
      return NULL;
    }
    
    /* Load up a Program with the result and time */
    return new Program(stmtList, difference);
  }
  
  Program* Program::loadFromFile(char* path)
  {
    /* Attempt to open the file for reading */
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
      return NULL;
    }
    
    /* Probably not the most efficient way but it should work */
    char* buffer = NULL;
    long length = 0;
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buffer = (char *)malloc(length+1);
    fread(buffer, length, 1, fp);
    fclose(fp);
    buffer[length] = '\0';
    
    /* Load the program from the text, close up and return */
    Program* program = Program::loadFromText(path, 1, buffer);
    free(buffer);
    return program;
  }
  
  Program* Program::loadRawProgram(std::vector<SyntaxTreeNode*>* rawProgram)
  {
    return new Program(rawProgram, 0.);
  }

  Program::Program(std::vector<SyntaxTreeNode*>* rawProgram, double compileTime) :
    _program(rawProgram), _runEnvironment(new Environment()), 
    _compileTime(compileTime), _lastRunTime(0.0)
  {
  }
  
  Program::~Program()
  {
    /* Delete all of the syntax tree nodes */
    std::vector<SyntaxTreeNode*>::iterator iter;
    for (iter = _program->begin(); iter != _program->end(); ++iter) {
      delete(*iter);
    }
    
    /* Clean up any used memory */
    delete(_runEnvironment);
    delete(_program);
  }
  
  Program::Program(const Program& other) throw ()
  {
    throw NonCopyableException();
  }
  
  Program& Program::operator=(const Program& other) throw ()
  {
    throw NonCopyableException();
  }
  
  std::string Program::getGraphvizRepresentation() const
  {
    /* Output the full graph */
    std::stringstream ss;
    ss << "digraph \"G\" {" << std::endl;
    
    /* Walk through the list and spit stuff out */
    std::vector<LQX::SyntaxTreeNode*>::iterator iter;
    for (iter = _program->begin(); iter != _program->end(); ++iter) {
      (*iter)->debugPrintGraphviz(ss);
    }
    
    /* Output the directed graph footing */
    ss << "};" << std::endl;
    return ss.str();
  }
  
  double Program::getCompileTime() const
  {
    return _compileTime;
  }
  
  double Program::getLastRunTime() const
  {
    return _lastRunTime;
  }
  
  bool Program::invoke()
  {
    /* Obtain the file processing time */
    struct timeval start_tv;
    struct timeval end_tv;
    bool success = true;
    
    /* Get the start time */
    gettimeofday(&start_tv, NULL);
      
    /* Make sure we got one */
    if(_program) {
      std::vector<LQX::SyntaxTreeNode*>::iterator iter;
      for (iter = _program->begin(); iter != _program->end(); ++iter) {
        try {
          (*iter)->invoke(_runEnvironment);
        } catch (LQX::RuntimeException re) {
          std::cout << "--> Runtime Exception Occured: " << re.what() << std::endl;
	  success = false;
          break;
        }
      }
    }
    
    /* Display time it took to run */
    gettimeofday(&end_tv, NULL);
    _lastRunTime = (double)(end_tv.tv_sec - start_tv.tv_sec) + (double)((end_tv.tv_usec - start_tv.tv_usec) * 10e-6f);
    return success;
  }
  
  SymbolAutoRef Program::defineConstantVariable(std::string name)
  {
    /* Now, External variables and Constant variables are the same */
    SymbolAutoRef sar = this->defineExternalVariable(name);
    sar->setIsConstant(true);
    return sar;
  }
  
  SymbolAutoRef Program::defineExternalVariable(std::string name)
  {
    /* Define a new external symbol in the symbol table and return a reference */
    LQX::SymbolTable* st = _runEnvironment->getSpecialSymbolTable();
    st->define(name);
    SymbolAutoRef sar = st->get(name);
    sar->setIsConstant(false);
    return sar;
  }
  
  SymbolAutoRef Program::getSpecialVariable(std::string name)
  {
    /* Return a reference to an external symbol */
    LQX::SymbolTable* st = _runEnvironment->getSpecialSymbolTable();
    return st->get(name);
  }
  
  Environment* Program::getEnvironment() const
  {
    /* Allow more messing around */
    return _runEnvironment;
  }
  
}
