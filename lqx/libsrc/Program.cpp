/*
 *  Program.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 18/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <config.h>
#include "NonCopyable.h"
#include "Program.h"
#include "Scanner.h"
#include "Parser.h"
#include "Environment.h"
#include "SyntaxTree.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <errno.h>
#if HAVE_SYS_MMAN_H
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

/* C++ Headers */
#include <iostream>
#include <sstream>
#include <algorithm>

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
#if HAVE_MMAP
    struct stat statbuf;
    int fd = open( path, O_RDONLY );
    if ( fd < 0 || fstat( fd, &statbuf ) < 0 ) {
      throw RuntimeException( "Cannot open %s: %s.", path, strerror( errno ) );
    }
    char * buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, fd, 0 ));
    Program* program = NULL;
    if ( buffer != MAP_FAILED ) {
      program = Program::loadFromText(path, 1, buffer);
      munmap( buffer, statbuf.st_size );
    } else {
      throw RuntimeException( "Cannot open %s: %s.", path, strerror( errno ) );
    }
    close( fd );
    return program;
#else
    /* Attempt to open the file for reading */
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
	throw RuntimeException( "Cannot open %s: %s.", path, strerror( errno ) );
    }
    
    /* Probably not the most efficient way but it should work */
    long length = 0;
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char * buffer = new char[length+1];
    fread(buffer, length, 1, fp);
    fclose(fp);
    buffer[length] = '\0';
    
    /* Load the program from the text, close up and return */
    Program* program = Program::loadFromText(path, 1, buffer);
    delete [] buffer;
    return program;
#endif
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
  
  Program::Program(const Program&)
  {
    throw NonCopyableException();
  }
  
  Program& Program::operator=(const Program&)
  {
    throw NonCopyableException();
  }
  
  std::ostream& Program::getGraphvizRepresentation( std::ostream& ss ) const
  {
    /* Output the full graph */
    ss << "digraph \"G\" {" << std::endl;

    for_each( _program->begin(), _program->end(), printGraphviz( ss ) );
    
    /* Output the directed graph footing */
    ss << "};" << std::endl;
    return ss;
  }
  
  std::ostream& Program::print( std::ostream& output ) const
  {
    for_each( _program->begin(), _program->end(), printStatement( output ) );
    return output;
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
        } catch (const LQX::RuntimeException& re) {
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
  
  SymbolAutoRef Program::defineConstantVariable(const std::string& name)
  {
    /* Now, External variables and Constant variables are the same */
    SymbolAutoRef sar = this->defineExternalVariable(name);
    sar->setIsConstant(true);
    return sar;
  }
  
  SymbolAutoRef Program::defineExternalVariable(const std::string& name)
  {
    /* Define a new external symbol in the symbol table and return a reference */
    LQX::SymbolTable* st = _runEnvironment->getSpecialSymbolTable();
    st->define(name);
    SymbolAutoRef sar = st->get(name);
    sar->setIsConstant(false);
    return sar;
  }
  
  SymbolAutoRef Program::getSpecialVariable(const std::string& name)
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

  void Program::printStatement::operator()( const LQX::SyntaxTreeNode* node ) const 
  {
    node->print( _output );
    if ( node->simpleStatement() ) {
      _output << ";" << std::endl;
    }
  }

  void Program::printGraphviz::operator()( const LQX::SyntaxTreeNode* node ) const 
  {
    node->debugPrintGraphviz( _output );
  }
  
}
