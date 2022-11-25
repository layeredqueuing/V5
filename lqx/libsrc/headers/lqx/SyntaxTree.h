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
#include <iostream>
#include <string>
#include <vector>

namespace LQX {
  
  class SyntaxTreeNode {
  public:
    
    /* This is simply an interface (abstract) */
    SyntaxTreeNode() {}
    virtual ~SyntaxTreeNode() {}

    static void setVariablePrefix( const std::string& s ) { variablePrefix = s; }
    
  protected:
    
    /* This object is not copyable, and will throw an exception if you try */
    SyntaxTreeNode(const SyntaxTreeNode&) { throw NonCopyableException(); }
    virtual SyntaxTreeNode& operator=(const SyntaxTreeNode&) { throw NonCopyableException(); }
    
  public:
    
    /* Uses of a syntax tree */
    virtual void debugPrintGraphviz(std::ostream& output) const = 0;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const = 0;
    virtual bool simpleStatement() const { return true; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env) = 0;
    
  protected:
    /* The names of the operations */
    static const char* logicNames[];
    static const char* compareNames[];
    static const char* arithmeticNames[];

    static std::string variablePrefix;					// for printing.
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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);

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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual bool simpleStatement() const { return false; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env);

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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);

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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);

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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);

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
      MODULUS,
      POWER,
      NEGATE,
    } MathOperation;
    
  public:

    /* Constructors and Destructors */
    MathExpression(MathOperation op, SyntaxTreeNode* left, SyntaxTreeNode* right);
    virtual ~MathExpression();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);

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
    ConstantValueExpression(const std::string& );
    ConstantValueExpression(double numericalValue);
    ConstantValueExpression(bool booleanValue);
    virtual ~ConstantValueExpression();
      
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);

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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);
    const std::string& getName() const { return _name; }
    
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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);
    const std::string& getName() const { return _name; }
    
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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual bool simpleStatement() const { return false; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env);
    
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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual bool simpleStatement() const { return false; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env);
    
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
  
  class BreakStatementNode : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    BreakStatementNode();
    virtual ~BreakStatementNode();
    
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual bool simpleStatement() const { return true; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env);
    
  private:
    
    /* Instance variables for flow control */
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class ReturnStatementNode : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    ReturnStatementNode( SyntaxTreeNode* expr  );
    virtual ~ReturnStatementNode();
    
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual bool simpleStatement() const { return true; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env);
    
  private:
    
    /* Instance variables for return */
    SyntaxTreeNode * _expr;
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  class SelectExpression : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    SelectExpression(const std::string& valueName, bool ev, SyntaxTreeNode* arrayNode, SyntaxTreeNode* condition);
    virtual ~SelectExpression();
    
    /* -- Invoking will call pushContext() on Symbol Table -- */
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual bool simpleStatement() const { return true; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env);
    
  private:
    
    /* Instance variables for itteration */
    std::string _valueName;
    bool _valueIsExternal;
    SyntaxTreeNode* _arrayNode;
    SyntaxTreeNode* _selectCondition;
    
  };
  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  class ComprehensionExpression : public SyntaxTreeNode {
  public:
    
    /* Constructors and Destructors */
    ComprehensionExpression(const std::string& valueName, bool ev, SyntaxTreeNode* initialValue, SyntaxTreeNode* stopCondition, SyntaxTreeNode* onEachRun);
    virtual ~ComprehensionExpression();
    
    /* -- Invoking will call pushContext() on Symbol Table -- */
    /* Actual implementation of tree methods */
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual bool simpleStatement() const { return true; }		// Statements that end with `;' 
    virtual SymbolAutoRef invoke(Environment* env);
    
  private:
    
    /* Instance variables for itteration */
    std::string _valueName;
    bool _valueIsExternal;
    SyntaxTreeNode* _initialValue;
    SyntaxTreeNode* _stopCondition;
    SyntaxTreeNode* _onEachRun;
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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);
    
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

    FileOpenStatementNode( const std::string& fileHandle, SyntaxTreeNode* object, bool write, bool append=false );
    virtual ~FileOpenStatementNode();

    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke( Environment* env );

  private:

    const std::string _fileHandle;
    SyntaxTreeNode * _filePath;
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

    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke( Environment* env );

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

    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke( Environment* env );

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

    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke( Environment* env );

  private:

    char * readString( FILE *infile, char *quoted_string, const char *variable = NULL );
    bool readBoolean( FILE *infile, const char *variable = NULL );

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
      virtual SymbolAutoRef invoke(Environment* env, std::vector<SymbolAutoRef >& args);
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
    virtual void debugPrintGraphviz(std::ostream& output) const;
    virtual std::ostream& print(std::ostream& output, unsigned int indent=0) const;
    virtual SymbolAutoRef invoke(Environment* env);
    
  private:
    
    /* Instance variables for declaring functions */
    std::string _name;
    std::vector<std::string>* _prototype;
    SyntaxTreeNode* _body;
    std::string _argTypes;
    
  };
  
  inline std::ostream& operator<<( std::ostream& output, const SyntaxTreeNode& self ) { return self.print( output ); }
}

#endif /* __SYNTAX_TREE_H__ */
