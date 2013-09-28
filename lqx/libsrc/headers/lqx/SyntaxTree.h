/*
 *  SyntaxTree.h
 *  ModLang
 *
 *  Created by Martin Mroz on 16/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __SYNTAX_TREE_H__
#define __SYNTAX_TREE_H__

/* Internal headers */
#include "NonCopyable.h"
#include "RuntimeException.h"
#include "Environment.h"
#include "Parser.h"

/* Standard library headers */
#include <sstream>
#include <string>
#include <vector>

namespace LQX {
  
  class SyntaxTreeNode {
  public:
    
    /* This is simply an interface (abstract) */
    SyntaxTreeNode() {}
    virtual ~SyntaxTreeNode() {}
    
  protected:
    
    /* This object is not copyable, and will throw an exception if you try */
    SyntaxTreeNode(const SyntaxTreeNode& other) throw(NonCopyableException) { throw NonCopyableException(); }
    virtual SyntaxTreeNode& operator=(const SyntaxTreeNode& other) throw(NonCopyableException) { throw NonCopyableException(); }
    
  public:
    
    /* Uses of a syntax tree */
    virtual void debugPrintGraphviz(std::stringstream& output) = 0;
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException) = 0;
    
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class CompoundStatementNode : public SyntaxTreeNode {
  public:

    /* Constructors and Destructors */
    CompoundStatementNode(std::vector<SyntaxTreeNode*>* statements);
    CompoundStatementNode(SyntaxTreeNode* first, ...);
    virtual ~CompoundStatementNode();
      
    /* -- Invoking will call pushContext() on Symbol Table -- */
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);

  private:
    
    /* All the statements that need be executed */
    std::vector<SyntaxTreeNode*>* _statements;
    
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

  class ConditionalStatementNode : public SyntaxTreeNode {
  public:

    /* Constructors and Destructors */
    ConditionalStatementNode(SyntaxTreeNode* testNode, SyntaxTreeNode* ifTrue, SyntaxTreeNode* ifFalse);
    virtual ~ConditionalStatementNode();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);

  private:
    
    /* Nodes for performing appropriate actions */
    SyntaxTreeNode* _testNode;
    SyntaxTreeNode* _trueAction;
    SyntaxTreeNode* _falseAction;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class AssignmentStatementNode : public SyntaxTreeNode {
  public:

    /* Constructors and Destructors */
    AssignmentStatementNode(SyntaxTreeNode* target, SyntaxTreeNode* value);
    virtual ~AssignmentStatementNode();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);

  private:
    
    /* Value Storage */
    SyntaxTreeNode* _target;
    SyntaxTreeNode* _value;
    
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class LogicExpression : public SyntaxTreeNode {
  public:
    
    typedef enum LogicOperation {
      AND = 0,
      OR,
      NOT
    } LogicOperation;
    
  public:

    /* Constructors and Destructors */
    LogicExpression(LogicOperation op, SyntaxTreeNode* left, SyntaxTreeNode* right);
    virtual ~LogicExpression();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);

  private:
    
    /* Operation, Arguments and Result */
    LogicOperation _operation;
    SyntaxTreeNode* _left;
    SyntaxTreeNode* _right;
    
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class ComparisonExpression : public SyntaxTreeNode {
  public:
    
    /* How to do the comparison */
    typedef enum CompareMode {
      EQUALS = 0,
      NOT_EQUALS,
      LESS_THAN,
      GREATER_THAN,
      LESS_OR_EQUAL,
      GREATER_OR_EQUAL
    } CompareMode;
    
  public:

    /* Constructors and Destructors */
    ComparisonExpression(CompareMode mode, SyntaxTreeNode* left, SyntaxTreeNode* right);
    virtual ~ComparisonExpression();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);

  private:
    
    /* Operation, Arguments and Result */
    CompareMode _mode;
    SyntaxTreeNode* _left;
    SyntaxTreeNode* _right;
    
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class MathExpression : public SyntaxTreeNode {
  public:
    
    /* The different operations supported */
    typedef enum MathOperation {
      SHIFT_LEFT = 0,
      SHIFT_RIGHT,
      ADD,
      SUBTRACT,
      MULTIPLY,
      DIVIDE,
      MODULUS
    } MathOperation;
    
  public:

    /* Constructors and Destructors */
    MathExpression(MathOperation op, SyntaxTreeNode* left, SyntaxTreeNode* right);
    virtual ~MathExpression();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);

  private:
    
    /* Operation and Arguments */
    MathOperation _operation;
    SyntaxTreeNode* _left;
    SyntaxTreeNode* _right;
    
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class ConstantValueExpression : public SyntaxTreeNode {
  public:

    /* Constructors and Destructors */
    ConstantValueExpression();
    ConstantValueExpression(const char* stringValue);
    ConstantValueExpression(double numericalValue);
    ConstantValueExpression(bool booleanValue);
    virtual ~ConstantValueExpression();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);

  private:
    
    /* The instance of the current symbol */
    SymbolAutoRef _current;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class VariableExpression : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    VariableExpression(const std::string& name, bool external);
    virtual ~VariableExpression();
    
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);
  
  private:
    
    /* The instance of the name */
    std::string _name;
    bool _external;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class MethodInvocationExpression : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    MethodInvocationExpression(const std::string& name);
    MethodInvocationExpression(const std::string& name, std::vector<SyntaxTreeNode*>* arguments);
    MethodInvocationExpression(const std::string& name, SyntaxTreeNode* first, ...);
    virtual ~MethodInvocationExpression();
    
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);
    
  private:
    
    /* The instance of the name */
    std::string _name;
    std::vector<SyntaxTreeNode*>* _arguments;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class LoopStatementNode : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    LoopStatementNode(SyntaxTreeNode* onStart, SyntaxTreeNode* stop, SyntaxTreeNode* onEachRun, SyntaxTreeNode* action);
    virtual ~LoopStatementNode();
    
    /* -- Invoking will call pushContext() on Symbol Table -- */
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);
    
  private:
    
    /* Instance variables for flow control */
    SyntaxTreeNode* _onBegin;
    SyntaxTreeNode* _stopCondition;
    SyntaxTreeNode* _onEachRun;
    SyntaxTreeNode* _action;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class ForeachStatementNode : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    ForeachStatementNode(const std::string& keyName, const std::string& valueName, bool ek, bool ev, 
      SyntaxTreeNode* arrayNode, SyntaxTreeNode* action);
    virtual ~ForeachStatementNode();
    
    /* -- Invoking will call pushContext() on Symbol Table -- */
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);
    
  private:
    
    /* Instance variables for itteration */
    std::string _keyName;
    bool _keyIsExternal;
    std::string _valueName;
    bool _valueIsExternal;
    SyntaxTreeNode* _arrayNode;
    SyntaxTreeNode* _actionNode;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class ObjectPropertyReadNode : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    ObjectPropertyReadNode(SyntaxTreeNode* object, const std::string& property);
    virtual ~ObjectPropertyReadNode();
    
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);
    
  private:
    
    /* Instance variables for reading */
    SyntaxTreeNode* _objectNode;
    std::string _propertyName;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class FileOpenStatementNode : public SyntaxTreeNode {
  public:

    FileOpenStatementNode( const std::string& fileHandle, const std::string& filePath, bool write, bool append=false );
    virtual ~FileOpenStatementNode();

    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke( Environment* env ) throw (RuntimeException);

  private:

    std::string _fileHandle;
    std::string _filePath;
    bool _write;
    bool _append;
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class FileCloseStatementNode : public SyntaxTreeNode {
  public:

    FileCloseStatementNode( const std::string& fileHandle );
    virtual ~FileCloseStatementNode();


    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke( Environment* env ) throw (RuntimeException);

  private:

    std::string _fileHandle;
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class FilePrintStatementNode : public SyntaxTreeNode {
  public:

    FilePrintStatementNode( std::vector<SyntaxTreeNode*>* arguments, bool newline, bool spacing = false );
    virtual ~FilePrintStatementNode();

    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke( Environment* env ) throw (RuntimeException);

  private:

    std::vector<SyntaxTreeNode*>* _arguments;
    bool _newline;
    bool _spacing;
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class ReadDataStatementNode : public SyntaxTreeNode {
  public:

    ReadDataStatementNode( const std::string& fileHandle, std::vector<SyntaxTreeNode*>* arguments );
    virtual ~ReadDataStatementNode();

    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke( Environment* env ) throw (RuntimeException);

  private:

    char * readString( FILE *infile, char *quoted_string, const char *variable = NULL ) throw (RuntimeException);
    bool readBoolean( FILE *infile, const char *variable = NULL ) throw (RuntimeException);

    std::string _fileHandle;
    std::vector<SyntaxTreeNode*>* _arguments;
  };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class FunctionDeclarationNode : public SyntaxTreeNode {
  public:
    
    /* Language-Implemented Method Wrapper */
    class LanguageImplementedMethod : public Method {
    public:
      LanguageImplementedMethod(const std::string& name, const std::string& arguments, std::vector<std::string>* argNames, SyntaxTreeNode* action) :
        _name(name), _arguments(arguments), _argNames(argNames), _action(action) {}
      virtual ~LanguageImplementedMethod() {}
      virtual std::string getName() const { return _name; }
      virtual const char* getParameterInfo() const { return _arguments.c_str(); }
      virtual std::string getHelp() const { return "No help available for Language-Implemented Methods"; }
      virtual SymbolAutoRef invoke(Environment* env, std::vector<SymbolAutoRef >& args) throw (RuntimeException);
    private:
      const std::string _name;
      const std::string _arguments;
      const std::vector<std::string>* _argNames;
      const SyntaxTreeNode* _action;
    };
    
  public:
    
    /* Constructors and Destructors */
    FunctionDeclarationNode(const std::string& name, std::vector<std::string>* proto, SyntaxTreeNode* body);
    virtual ~FunctionDeclarationNode();
    
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::stringstream& output);
    virtual SymbolAutoRef invoke(Environment* env) throw (RuntimeException);
    
  private:
    
    /* Instance variables for declaring functions */
    std::string _name;
    std::vector<std::string>* _prototype;
    SyntaxTreeNode* _body;
    std::string _argTypes;
    
  };
  
}

#endif /* __SYNTAX_TREE_H__ */
