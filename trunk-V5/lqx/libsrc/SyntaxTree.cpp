/*
 *  SyntaxTree.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 16/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "SyntaxTree.h"
#include "LanguageObject.h"
#include "RuntimeFlowControl.h"
#include "Array.h"

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

/* Quote the item in the context of a stringstream */
#define QUOTE(x) "\"" << x << "\""
#define strequal( x, y ) ( strcmp( x, y ) == 0 )

namespace LQX {
  
  static unsigned convertToInteger(double rightNumber) throw (IncompatibleTypeException) {
    unsigned rightInteger = 0;
    long integralPart = (long)rightNumber;
    double fractionalPart = rightNumber - (double)integralPart;
    if (rightNumber < 0 || fractionalPart != 0) {
      throw IncompatibleTypeException("double", "integer");
    }
    
    /* Return the integral part of the double */
    rightInteger = integralPart;
    return rightInteger;
  }
  
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  CompoundStatementNode::CompoundStatementNode(std::vector<SyntaxTreeNode*>* statements) :
    _statements(statements)
  {
  }
  
  CompoundStatementNode::CompoundStatementNode(SyntaxTreeNode* first, ...) :
    _statements(new std::vector<SyntaxTreeNode*>())
  {
    /* Add the first statement to the list */
    _statements->push_back(first);
    
    /* Add Remaining */
    va_list ap;
    va_start(ap, first);
    SyntaxTreeNode* current = NULL;
    while ((current = va_arg(ap, SyntaxTreeNode*))) { _statements->push_back(current); }
    va_end(ap);
  }

  CompoundStatementNode::~CompoundStatementNode()
  {
    /* Delete all of the items in the list */
    std::vector<SyntaxTreeNode*>::iterator iter;
    for (iter = _statements->begin(); iter != _statements->end(); iter++) {
      delete (*iter);
    }
    
    /* Delete the statements list */
    delete(_statements);
  }

  void CompoundStatementNode::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    output << QUOTE(thisNode) << " [label=\"Invoke\", shape=box];" << std::endl;
    
    /* Output all of the expressions we will be running */
    std::vector<SyntaxTreeNode*>::iterator iter;
    for (iter = _statements->begin(); iter != _statements->end(); iter++) {
      SyntaxTreeNode* current = *iter;
      uintptr_t currentNodeId = reinterpret_cast<uintptr_t>(current);
      current->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(currentNodeId) << ";" << std::endl;
    }
  }

  SymbolAutoRef CompoundStatementNode::invoke(Environment* env) throw (RuntimeException)
  {
#if defined(__VARIABLE_SCOPING__)
    /* Compound statements push the variable context */
    SymbolTable* st = env->getSymbolTable();
    st->pushContext();
#endif
    
    /* Invoke all of the statements given */
    std::vector<SyntaxTreeNode*>::iterator iter;
    for (iter = _statements->begin(); iter != _statements->end(); iter++) {
      (*iter)->invoke(env);
    }
    
#if defined(__VARIABLE_SCOPING__)
    /* Pop the variable context */
    st->popContext();
#endif
    
    /* Returns Nothing */
    return Symbol::encodeNull();
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  ConditionalStatementNode::ConditionalStatementNode(SyntaxTreeNode* testNode, 
    SyntaxTreeNode* ifTrue, SyntaxTreeNode* ifFalse) :
    _testNode(testNode), _trueAction(ifTrue), _falseAction(ifFalse)
  {
  }

  ConditionalStatementNode::~ConditionalStatementNode()
  {
    /* Clean up memory */
    delete(_testNode);
    delete(_trueAction);
    delete(_falseAction);
  }

  void ConditionalStatementNode::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    output << QUOTE(thisNode) << " [label=\"If Then Else\", shape=box];" << std::endl;
    
    /* Output the Test Node */
    if (_testNode) {
      uintptr_t testNodeId = reinterpret_cast<uintptr_t>(_testNode);
      _testNode->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(testNodeId) << ";" << std::endl;
    }
    
    /* Output the True Action Node */
    if (_trueAction) {
      uintptr_t trueNodeId = reinterpret_cast<uintptr_t>(_trueAction);
      _trueAction->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(trueNodeId) << ";" << std::endl;
    }
    
    /* Output the False Action Node */
    if (_falseAction) {
      uintptr_t falseNodeId = reinterpret_cast<uintptr_t>(_falseAction);
      _falseAction->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(falseNodeId) << ";" << std::endl;
    }
  }

  SymbolAutoRef ConditionalStatementNode::invoke(Environment* env) throw (RuntimeException)
  {
    /* Make sure there's something to do */
    if (_testNode == NULL) {
      throw InternalErrorException("");
    }
    
    /* Make sure we got ourselves a boolean */
    SymbolAutoRef testValue = _testNode->invoke(env);
    if (testValue == NULL || testValue->getType() != Symbol::SYM_BOOLEAN) {
      throw IncompatibleTypeException(testValue->getTypeName(), "boolean");
    }
    
    /* Invoke the appropriate node */
    bool result = testValue->getBooleanValue();
    if (result && _trueAction) {
      return _trueAction->invoke(env);
    } else if (!result && _falseAction) {
      return _falseAction->invoke(env);
    }
    
    return Symbol::encodeNull();
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  AssignmentStatementNode::AssignmentStatementNode(SyntaxTreeNode* target, SyntaxTreeNode* value) : 
    _target(target), _value(value)
  {
  }

  AssignmentStatementNode::~AssignmentStatementNode()
  {
    /* Delete the value side */
    delete(_value);
  }

  void AssignmentStatementNode::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this), valueNode = reinterpret_cast<uintptr_t>(_value);
    output << QUOTE(thisNode) << " [label=\"Assign\", shape=box];" << std::endl;
    uintptr_t leftNodeId = reinterpret_cast<uintptr_t>(_target);
    _target->debugPrintGraphviz(output);
    output << QUOTE(thisNode) << " -> " QUOTE(leftNodeId) << ";" << std::endl;
    _value->debugPrintGraphviz(output);
    output << QUOTE(thisNode) << " -> " << QUOTE(valueNode) << ";" << std::endl;
    output << std::endl;
  }

  SymbolAutoRef AssignmentStatementNode::invoke(Environment* env) throw (RuntimeException)
  {
    /* Obtain the value we are assigning here */
    SymbolAutoRef value = _value->invoke(env);
    if (value == NULL) {
      throw InternalErrorException("Unable to obtain rvalue for assignment.");
    }
    
    /* Figure out what we are setting on */
    SymbolAutoRef target = _target->invoke(env);
    if (target->isConstant()) {
      std::cout << "warning: Attempting to assign to a constant variable. No effect." << std::endl;
    } else {
      target->copyValue(*value);
    }
    
    return target;
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  LogicExpression::LogicExpression(LogicOperation op, SyntaxTreeNode* left, SyntaxTreeNode* right) : 
    _operation(op), _left(left), _right(right)
  {
  }

  LogicExpression::~LogicExpression()
  {
    /* Clean up the tree */
    delete(_left);
    delete(_right);
  }

  void LogicExpression::debugPrintGraphviz(std::stringstream& output)
  {
    /* The names of the operations */
    static const char* logicNames[] = { "||", "&&", "!" };
    
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    const char* thisName = logicNames[static_cast<uint32_t>(_operation)];
    output << QUOTE(thisNode) << " [label=\"" << thisName << "\", shape=box];" << std::endl;
    
    /* Output the Left Node */
    if (_left) {
      uintptr_t leftNodeId = reinterpret_cast<uintptr_t>(_left);
      _left->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(leftNodeId) << ";" << std::endl;
    }
    
    /* Output the Right Node */
    if (_right) {
      uintptr_t rightNodeId = reinterpret_cast<uintptr_t>(_right);
      _right->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(rightNodeId) << ";" << std::endl;
    }
  }

  SymbolAutoRef LogicExpression::invoke(Environment* env) throw (RuntimeException)
  {
    /* Find out what we are operaing on */
    SymbolAutoRef left = _left->invoke(env);
    SymbolAutoRef right(NULL);
    bool hasRight = false;
    
    /* Obtain the right side of the expression where applicable */
    if (_operation == AND || _operation == OR) {
      right = _right->invoke(env);
      hasRight = true;
    }
    
    /* Check if everything actually worked out right */
    if (left == NULL || (hasRight == true && right == NULL)) {
      throw InternalErrorException("Left or Right Side Didn't Evaluate to a Symbol.");
    } else if (left->getType() != Symbol::SYM_BOOLEAN) {
      throw IncompatibleTypeException(left->getTypeName(), "boolean");
    } else if ((hasRight && (right->getType() != Symbol::SYM_BOOLEAN))) {
      throw IncompatibleTypeException(right->getTypeName(), "boolean");
    }
    
    /* Get the value to perform the operation */
    bool result = false;
    
    /* Do the actual work */
    switch (_operation) {
      case AND: 
        result = left->getBooleanValue() && right->getBooleanValue();
        break;
      case OR:
        result = left->getBooleanValue() || right->getBooleanValue();
        break;
      case NOT:
	  result = !left->getBooleanValue();
        break;
    }
    
    /* Return an anonymous symbol */
    return Symbol::encodeBoolean(result);
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
#include <iostream>

namespace LQX {

  ComparisonExpression::ComparisonExpression(CompareMode mode, SyntaxTreeNode* left, SyntaxTreeNode* right) :
    _mode(mode), _left(left), _right(right)
  {
  }

  ComparisonExpression::~ComparisonExpression()
  {
    /* Clean up the tree */
    delete(_left);
    delete(_right);
  }

  void ComparisonExpression::debugPrintGraphviz(std::stringstream& output)
  {
    /* The names of the operations */
    static const char* compareNames[] = { "==", "!=", "<", ">", "<=", ">=" };
    
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    const char* thisName = compareNames[static_cast<uint32_t>(_mode)];
    output << QUOTE(thisNode) << " [label=\"" << thisName << "\", shape=box];" << std::endl;
    
    /* Output the Left Node */
    if (_left) {
      uintptr_t leftNodeId = reinterpret_cast<uintptr_t>(_left);
      _left->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(leftNodeId) << ";" << std::endl;
    }
    
    /* Output the Right Node */
    if (_right) {
      uintptr_t rightNodeId = reinterpret_cast<uintptr_t>(_right);
      _right->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(rightNodeId) << ";" << std::endl;
    }
  }

  SymbolAutoRef ComparisonExpression::invoke(Environment* env) throw (RuntimeException)
  {
    /* Find out what we are operaing on */
    SymbolAutoRef left = _left->invoke(env);
    SymbolAutoRef right = _right->invoke(env);
    Symbol::Type leftType, rightType;
    bool result = false;
        
    /* Check if everything actually worked out right */
    if (left == NULL || right == NULL) {
      throw InternalErrorException("Unable to find either the lvalue or the rvalue.");
    }
    
    /* Get the type of the nodes */
    leftType = left->getType();
    rightType = right->getType();
    
    /* We can compare objects against eachother and nulls using built-in operations */
    if (leftType == Symbol::SYM_NULL && rightType == Symbol::SYM_NULL) {
      switch (_mode) {
        case EQUALS:     result = true;  break;
        case NOT_EQUALS: result = false; break;
        default:         throw RuntimeException("Objects/nulls can only be compared with == and !=");
      }
    } else if ((leftType == Symbol::SYM_NULL && rightType == Symbol::SYM_OBJECT) ||
      (leftType == Symbol::SYM_OBJECT && rightType == Symbol::SYM_NULL)) {
      switch (_mode) {
        case EQUALS:     result = false; break;
        case NOT_EQUALS: result = true;  break;
        default:         throw RuntimeException("Objects/nulls can only be compared with == and !=");
      }
    } else if ((leftType == Symbol::SYM_NULL && rightType == Symbol::SYM_OBJECT)) {
      LanguageObject* leftObject = left->getObjectValue();
      LanguageObject* rightObject = right->getObjectValue();
      switch (_mode) {
        case EQUALS:     result =  leftObject->isEqualTo(rightObject); break;
        case NOT_EQUALS: result = !leftObject->isEqualTo(rightObject); break;
        default:         throw RuntimeException("Objects/nulls can only be compared with == and !=");
      }
    } else {
      
      /* Check to see if we can work this as a double or a boolean comparison */
      if (left->getType() == Symbol::SYM_DOUBLE && right->getType() != Symbol::SYM_DOUBLE) {
        throw IncompatibleTypeException(right->getTypeName(), Symbol::typeToString(Symbol::SYM_DOUBLE));
      } else if (left->getType() == Symbol::SYM_BOOLEAN && right->getType() != Symbol::SYM_BOOLEAN) {
        throw IncompatibleTypeException(right->getTypeName(), Symbol::typeToString(Symbol::SYM_BOOLEAN));      
      } else if ((left->getType() != Symbol::SYM_BOOLEAN && left->getType() != Symbol::SYM_DOUBLE) && 
        (right->getType() != Symbol::SYM_BOOLEAN && right->getType() != Symbol::SYM_DOUBLE)) {
        throw InternalErrorException("Unknown type in comparison.");
      }
      
      /* If we are dealing with doubles, do it that way. */
      if (left->getType() == Symbol::SYM_DOUBLE) {
        double d1 = left->getDoubleValue();
        double d2 = right->getDoubleValue();
        switch (_mode) {
          case EQUALS:           result = (d1 == d2); break;
          case NOT_EQUALS:       result = (d1 != d2); break;
          case LESS_THAN:        result = (d1 <  d2); break;
          case GREATER_THAN:     result = (d1 >  d2); break;
          case LESS_OR_EQUAL:    result = (d1 <= d2); break;
          case GREATER_OR_EQUAL: result = (d1 >= d2); break;
          default:               throw InternalErrorException("Invalid operation specified.");
        }
      } else if (left->getType() == Symbol::SYM_BOOLEAN) {
        bool b1 = left->getBooleanValue();
        bool b2 = right->getBooleanValue();
        switch (_mode) {
          case EQUALS:           result = (b1 == b2); break;
          case NOT_EQUALS:       result = (b1 != b2); break;
          default:               throw RuntimeException("Booleans can only be compared with == and !=");
        }
      }
      
    }
    
    /* Generate the boolean symbol for the result */
    return Symbol::encodeBoolean(result);
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  MathExpression::MathExpression(MathOperation op, SyntaxTreeNode* left, SyntaxTreeNode* right) : 
    _operation(op), _left(left), _right(right)
  {
  }

  MathExpression::~MathExpression()
  {
    /* Clean up the tree */
    delete(_left);
    delete(_right);
  }

  void MathExpression::debugPrintGraphviz(std::stringstream& output)
  {
    /* The names of the operations */
    static const char* compareNames[] = { "<<", ">>", "+", "-", "*", "/", "%" };
    
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    const char* thisName = compareNames[static_cast<uint32_t>(_operation)];
    output << QUOTE(thisNode) << " [label=\"" << thisName << "\", shape=box];" << std::endl;
    
    /* Output the Left Node */
    if (_left) {
      uintptr_t leftNodeId = reinterpret_cast<uintptr_t>(_left);
      _left->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(leftNodeId) << ";" << std::endl;
    }
    
    /* Output the Right Node */
    if (_right) {
      uintptr_t rightNodeId = reinterpret_cast<uintptr_t>(_right);
      _right->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(rightNodeId) << ";" << std::endl;
    }
  }

  SymbolAutoRef MathExpression::invoke(Environment* env) throw (RuntimeException)
  {
    /* Find out what we are operaing on */
    SymbolAutoRef left = _left->invoke(env);
    SymbolAutoRef right = _right->invoke(env);
    
    /* Check if everything actually worked out right */
    if (left == NULL || right == NULL) {
      throw InternalErrorException("Left or Right Side Didn't Evaluate to a Symbol.");
    } else if (left->getType() != Symbol::SYM_DOUBLE) {
      throw IncompatibleTypeException(left->getTypeName(), "double");
    } else if (right->getType() != Symbol::SYM_DOUBLE) {
      throw IncompatibleTypeException(right->getTypeName(), "double");      
    }
    
    /* Pull out the values */
    double leftNumber = left->getDoubleValue();
    double rightNumber = right->getDoubleValue();
    double result = 0.0f;
    unsigned rightInteger = 0;
    unsigned leftInteger = 0; 
    
    /* If this operation requires an integer argument, then we need to grab it */
    if (_operation == SHIFT_LEFT || _operation == SHIFT_RIGHT || _operation == MODULUS) {
      rightInteger = convertToInteger(rightNumber);
      leftInteger = convertToInteger(leftNumber);
    }
    
    /* Perform the operation itself */
    switch (_operation) {
      case SHIFT_LEFT:
        result = leftInteger << rightInteger;
        break;
      case SHIFT_RIGHT:
        result = leftInteger >> rightInteger;
        break;
      case MODULUS:
        result = leftInteger % rightInteger;
        break;
      case ADD:
        result = leftNumber + rightNumber;
        break;
      case SUBTRACT:
        result = leftNumber - rightNumber;
        break;
      case MULTIPLY:
        result = leftNumber * rightNumber;
        break;
      case DIVIDE:
        result = leftNumber / rightNumber;
        break;
      default:
        throw InternalErrorException("Unsupported Math Operation");
        break;
    };
    
    /* Generate an anonymous symbol and assign it a double */
    return Symbol::encodeDouble(result);
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  ConstantValueExpression::ConstantValueExpression()
  {
    /* Assign a NULL value */
    _current = Symbol::encodeNull();
  }
  
  ConstantValueExpression::ConstantValueExpression(const char* stringValue)
  {
    /* Assign a string value */
    _current = Symbol::encodeString(stringValue);
  }

  ConstantValueExpression::ConstantValueExpression(double numericalValue)
  {
    /* Assign a double value */
    _current = Symbol::encodeDouble(numericalValue);
  }
  
  ConstantValueExpression::ConstantValueExpression(bool booleanValue)
  {
    /* Assign a boolean value */
    _current = Symbol::encodeBoolean(booleanValue);
  }

  ConstantValueExpression::~ConstantValueExpression()
  {
  }

  void ConstantValueExpression::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output the value string */
    std::string valueStr;
    
    /* Figure out what to say */
    switch (_current->getType()) {
      case Symbol::SYM_BOOLEAN: {
        valueStr = _current->getBooleanValue() ? "B(true)" : "B(false)";
        break;
      } case Symbol::SYM_DOUBLE: {
        std::stringstream ss;
        ss << _current->getDoubleValue();
        valueStr = ss.str();
        break;
      } case Symbol::SYM_STRING: {
        std::stringstream ss;
        ss << "S(" << _current->getStringValue() << ")";
        valueStr = ss.str();
        break;
      } default: {
        valueStr = "<<invalid>>";
        break;
      }
    }
    
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    output << QUOTE(thisNode) << " [label=\"" << valueStr << "\"];" << std::endl;
  }

  SymbolAutoRef ConstantValueExpression::invoke(Environment* env) throw (RuntimeException)
  {
    /* Simple enough... */
    return _current;
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  VariableExpression::VariableExpression(const std::string& name, bool external) : _name(name), _external(external)
  {
  }
  
  VariableExpression::~VariableExpression()
  {
  }
  
  void VariableExpression::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    output << QUOTE(thisNode) << " [label=\"" << _name << "\"];" << std::endl;
  }
  
  SymbolAutoRef VariableExpression::invoke(Environment* env) throw (RuntimeException)
  {
    /* Get the Symbol Table from the Environment */
    SymbolTable* table = env->getSymbolTable();
    SymbolTable* specialTable = env->getSpecialSymbolTable();
    
    /* Check if the value is defined */
    if (!_external && !table->isDefined(_name)) {
      table->define(_name);
      SymbolAutoRef symbol = table->get(_name);
      symbol->setIsConstant(false);
      return symbol;
    } else if (_external && !specialTable->isDefined(_name)) {
      throw UndefinedVariableException(_name);
    }
  
    /* If it is, return it for us */
    return _external ? specialTable->get(_name) : table->get(_name);
  }
  
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  MethodInvocationExpression::MethodInvocationExpression(const std::string& name) : 
    _name(name), _arguments(new std::vector<SyntaxTreeNode*>())
  {
  }
  
  MethodInvocationExpression::MethodInvocationExpression(const std::string& name, std::vector<SyntaxTreeNode*>* arguments) :
    _name(name), _arguments(arguments)
  {
  }
  
  MethodInvocationExpression::MethodInvocationExpression(const std::string& name, SyntaxTreeNode* first, ...) :
    _name(name), _arguments(new std::vector<SyntaxTreeNode*>())
  {
    /* Throw in the arguments */
    va_list ap;
    va_start(ap, first);
    SyntaxTreeNode* node = NULL;
    _arguments->push_back(first);
    while ((node = va_arg(ap, SyntaxTreeNode*)) != NULL) { _arguments->push_back(node); }
    va_end(ap);
  }
  
  MethodInvocationExpression::~MethodInvocationExpression()
  {
    /* Delete the arguments list */
    delete _arguments;
  }
  
  void MethodInvocationExpression::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    output << QUOTE(thisNode) << " [label=\"Call(" << _name << ")\"];" << std::endl;
    
    /* Output all of the expressions we will be running */
    std::vector<SyntaxTreeNode*>::iterator iter;
    for (iter = _arguments->begin(); iter != _arguments->end(); iter++) {
      SyntaxTreeNode* current = *iter;
      uintptr_t currentNodeId = reinterpret_cast<uintptr_t>(current);
      current->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(currentNodeId) << ";" << std::endl;
    }
  }
  
  SymbolAutoRef MethodInvocationExpression::invoke(Environment* env) throw (RuntimeException)
  {
    /* Get the Symbol Table from the Environment */
    std::vector<SymbolAutoRef > args;
    
    /* Obtain all the arguments we need */
    std::vector<SyntaxTreeNode*>::iterator iter;
    for (iter = _arguments->begin(); iter != _arguments->end(); iter++) {
      SymbolAutoRef arg = (*iter)->invoke(env);
      if (arg == NULL) throw InternalErrorException("One of the arguments produces no value.");
      args.push_back(arg);
    }
    
    /* Run the method with the given arguments */
    return env->invokeGlobalMethod(_name, &args);
  }
  
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  LoopStatementNode::LoopStatementNode(SyntaxTreeNode* onStart, SyntaxTreeNode* stop, SyntaxTreeNode* onEachRun, SyntaxTreeNode* action) :
    _onBegin(onStart), _stopCondition(stop), _onEachRun(onEachRun), _action(action)
  {
  }
  
  LoopStatementNode::~LoopStatementNode()
  {
    /* Clean up the nodes */
    delete(_onBegin);
    delete(_stopCondition);
    delete(_onEachRun);
    delete(_action);
  }
  
  void LoopStatementNode::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    output << QUOTE(thisNode) << " [label=\"Loop\", shape=box];" << std::endl;
    
    /* Output the Begin Node */
    if (_onBegin) {
      uintptr_t onBeginId = reinterpret_cast<uintptr_t>(_onBegin);
      _onBegin->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(onBeginId) << ";" << std::endl;
    }
    
    /* Output the Stop Condition Node */
    if (_stopCondition) {
      uintptr_t stopNodeId = reinterpret_cast<uintptr_t>(_stopCondition);
      _stopCondition->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(stopNodeId) << ";" << std::endl;
    }
    
    /* Output the On Each Run Node */
    if (_onEachRun) {
      uintptr_t onRunId = reinterpret_cast<uintptr_t>(_onEachRun);
      _onEachRun->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(onRunId) << ";" << std::endl;
    }
    
    /* Output the Action Node */
    if (_action) {
      uintptr_t actionId = reinterpret_cast<uintptr_t>(_action);
      _action->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " QUOTE(actionId) << ";" << std::endl;
    }
  }
  
  SymbolAutoRef LoopStatementNode::invoke(Environment* env) throw (RuntimeException)
  {
#if defined(__VARIABLE_SCOPING__)
    /* Compound statements push the variable context */
    SymbolTable* st = env->getSymbolTable();
    st->pushContext();
#endif
    
    /* Make the initial thing happen for us here */
    if (_onBegin) { _onBegin->invoke(env); }
    
    /* Run the loop */
    for (;;) {
      
      /* Find out if we should stop */
      if (_stopCondition) {
        SymbolAutoRef shouldStop = _stopCondition->invoke(env);
        if (shouldStop->getType() != Symbol::SYM_BOOLEAN) {
          throw IncompatibleTypeException(shouldStop->getTypeName(), Symbol::typeToString(Symbol::SYM_BOOLEAN));
        } else if (shouldStop->getBooleanValue() == false) {
          break;
        }
      }
      
      /* Invoke the loop action */
      if (_action) { _action->invoke(env); }
      if (_onEachRun) { _onEachRun->invoke(env); }
    }
    
#if defined(__VARIABLE_SCOPING__)
    /* Pops the Context */
    st->popContext();
#endif
    return Symbol::encodeNull();
  }
  
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  ForeachStatementNode::ForeachStatementNode(const std::string& keyName, const std::string& valueName, bool ek, bool ev, SyntaxTreeNode* arrayNode, SyntaxTreeNode* action) :
    _keyName(keyName), _keyIsExternal(ek), _valueName(valueName), _valueIsExternal(ev), _arrayNode(arrayNode), _actionNode(action)
  {
  }
  
  ForeachStatementNode::~ForeachStatementNode()
  {
    /* Clean up allocated memory */
    delete(_arrayNode);
    delete(_actionNode);
  }
  
  void ForeachStatementNode::debugPrintGraphviz(std::stringstream& output)
  {
    /* Output this nodes declaration */
    uintptr_t thisNode = reinterpret_cast<uintptr_t>(this);
    output << QUOTE(thisNode) << " [label=\"Loop\", shape=box];" << std::endl;
    
    /* Output the variables */
    output << QUOTE(thisNode << "x") << " [label=\"" << _keyName << ", " << _valueName << "\"];" << std::endl;
    output << QUOTE(thisNode) << " -> " << QUOTE(thisNode << "x") << ";" << std::endl;
    
    /* Output the array node */
    if (_arrayNode) {
      uintptr_t arrayNodeId = reinterpret_cast<uintptr_t>(_arrayNode);
      _arrayNode->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " << QUOTE(arrayNodeId) << ";" << std::endl;
    }
    
    /* Output the array node */
    if (_actionNode) {
      uintptr_t actionNodeId = reinterpret_cast<uintptr_t>(_actionNode);
      _actionNode->debugPrintGraphviz(output);
      output << QUOTE(thisNode) << " -> " << QUOTE(actionNodeId) << ";" << std::endl;
    }
  }
  
  SymbolAutoRef ForeachStatementNode::invoke(Environment* env) throw (RuntimeException)
  {
#if defined(__VARIABLE_SCOPING__)
    /* Compound statements push the variable context */
    SymbolTable* st = env->getSymbolTable();
    st->pushContext();
#endif
    
    SymbolAutoRef arraySymbol = _arrayNode->invoke(env);
    ArrayObject* arrayObject = NULL;
    
    /* Check to see the type of the symbol matches up right */
    if (arraySymbol->getType() == Symbol::SYM_NULL) {
      return Symbol::encodeNull();
    } else if (arraySymbol->getType() != Symbol::SYM_OBJECT) {
      throw IncompatibleTypeException(Symbol::typeToString(arraySymbol->getType()), "Array");
    } else if (arraySymbol->getObjectValue()->getTypeId() != kArrayObjectTypeId) {
      throw RuntimeException("The object provided to the `foreach' statement was not an Array.");
    }
    
    /* The provided object is an Array */
    arrayObject = (ArrayObject *)arraySymbol->getObjectValue();
    std::map<SymbolAutoRef,SymbolAutoRef>::iterator iter;
    
    /* Get the Symbol Table from the Environment */
    SymbolTable* table = env->getSymbolTable();
    SymbolTable* specialTable = env->getSpecialSymbolTable();
    SymbolAutoRef keySymbol, valueSymbol;
    
    /* Grab the key from the table */
    if (_keyName != "") {
      if (_keyIsExternal) {
        if (!specialTable->isDefined(_keyName)) 
          throw UndefinedVariableException(_keyName);
        keySymbol = specialTable->get(_keyName);
      } else {
        if (!table->isDefined(_keyName)) 
          table->define(_keyName);
        keySymbol = table->get(_keyName);
      }
    }
    
    /* Grab the value from the table */
    if (_valueIsExternal) {
      if (!specialTable->isDefined(_valueName)) 
        throw UndefinedVariableException(_valueName);
      valueSymbol = specialTable->get(_valueName);
    } else {
      if (!table->isDefined(_valueName)) 
        table->define(_valueName);
      valueSymbol = table->get(_valueName);
    }
    
    /* Itterate over the key-value pairs in the array */
    for (iter = arrayObject->begin(); iter != arrayObject->end(); ++iter) {
      if (_keyName != "") { keySymbol->copyValue(*(iter->first)); }
      valueSymbol->copyValue(*(iter->second));
      if (_actionNode != NULL) { _actionNode->invoke(env); }
    }
    
#if defined(__VARIABLE_SCOPING__)
    /* Pops the Context */
    st->popContext();
#endif
    return Symbol::encodeNull();
  }
  
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  FileOpenStatementNode::FileOpenStatementNode( const std::string& fileHandle, const std::string& filePath, bool write, bool append ) :
    _fileHandle(fileHandle), _filePath(filePath), _write(write), _append(append)
  {
  }

  FileOpenStatementNode::~FileOpenStatementNode()
  {
  }

  void FileOpenStatementNode::debugPrintGraphviz( std::stringstream& output )
  {
    output << "NOT SUPPORTED";
  }

  SymbolAutoRef FileOpenStatementNode::invoke( Environment* env ) throw ( RuntimeException )
  {
    /* open file for writing using _filePath and place file pointer and handle in Symbol table */
    FILE *output_file;
    SymbolAutoRef symbol;
    const char *mode;
    std::stringstream err;

    SymbolTable* table = env->getSymbolTable();

    if( table->isDefined( _fileHandle ) ) {
      symbol = table->get( _fileHandle );
      output_file = symbol->getFilePointerValue();
      if( output_file != NULL ) {
	err <<  "The file handle \"" << _fileHandle.c_str() << "\" is already being used.";
	throw RuntimeException( err.str().c_str() );
      }
    } else {
      table->define( _fileHandle );
      symbol = table->get( _fileHandle );
    }

    if( _write )
      mode = _append ? "a" : "w";
    else /* read */
      mode = "r";

    if( (output_file = fopen( _filePath.c_str(), mode )) == 0 ) {
      err << "The file \"" << _filePath.c_str() << "\" could not be opened for ";
      if( _write ){
	if( _append )
	  err << "appending.";
	else
	  err << "writing.";
      } else {
	err << "reading.";
      }
      throw RuntimeException( err.str().c_str() );
    }

    if( _write )
      symbol->assignFileWritePointer( output_file );
    else
      symbol->assignFileReadPointer( output_file );

    return Symbol::encodeNull();
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  FileCloseStatementNode::FileCloseStatementNode( const std::string& fileHandle ) :
    _fileHandle(fileHandle)
  {
  }

  FileCloseStatementNode::~FileCloseStatementNode()
  {
  }

  void FileCloseStatementNode::debugPrintGraphviz( std::stringstream& output )
  {
    output << "NOT SUPPORTED";
  }

  SymbolAutoRef FileCloseStatementNode::invoke( Environment* env ) throw ( RuntimeException )
  {
    /*  search for file handle in symbol table, if found close file and remove handle from table */
    std::stringstream err;
    SymbolTable* table = env->getSymbolTable();
   
    if( !table->isDefined( _fileHandle ) ) {
      err << "The file handle \"" << _fileHandle.c_str() << "\" could not be found.";
      throw RuntimeException( err.str().c_str() );
    }

    SymbolAutoRef symbol = table->get( _fileHandle );

    Symbol::Type type = symbol->getType();
    if( !((type == Symbol::SYM_FILE_WRITE_POINTER) || (type == Symbol::SYM_FILE_READ_POINTER)) ) {
      err << "The given variable \"" << _fileHandle.c_str() << "\" is not a file handle.";
      throw RuntimeException( err.str().c_str() );
    }

    FILE* outfile = symbol->getFilePointerValue();

    if( outfile == NULL ) {
      err << "The file with handle \"" << _fileHandle.c_str() << "\" has already been closed.";
      throw RuntimeException( err.str().c_str() );
    }

    fclose( outfile );
    symbol->assignFileWritePointer( NULL );

    // need to remove handle from Symbol table, method doesn't exist

    return Symbol::encodeNull();
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
namespace LQX {

  FilePrintStatementNode::FilePrintStatementNode( std::vector<SyntaxTreeNode*>* arguments, bool newline, bool spacing ) :
    _arguments(arguments), _newline(newline), _spacing(spacing)
  {
  }
  
  FilePrintStatementNode::~FilePrintStatementNode()
  {
  }

  void FilePrintStatementNode::debugPrintGraphviz( std::stringstream& output )
  {
    output << "NOT SUPPORTED";
  }

  SymbolAutoRef FilePrintStatementNode::invoke( Environment* env ) throw ( RuntimeException )
  {
    std::stringstream ss, err;
    FILE* outfile;
    std::vector<SymbolAutoRef > args;
    std::vector<SyntaxTreeNode*>::iterator iter;
    SymbolAutoRef arg;
    SymbolAutoRef symbol;
    Symbol::Type type;
    bool file_pointer_found = false;
   
    /* check if first argument is a valid open file write pointer, if not set output to standard output */
    iter = _arguments->begin();
    arg = (*iter)->invoke( env );
    if (arg == NULL) throw InternalErrorException( "The first argument produces no value." );
   
    type = arg->getType();

    if( (type == Symbol::SYM_FILE_WRITE_POINTER) || (type == Symbol::SYM_FILE_READ_POINTER) ) {
      if( type != Symbol::SYM_FILE_WRITE_POINTER ) {
	err <<  "The given file handle \"" << arg->getStringValue() << "\" has been opened for reading instead of writing.";
	throw RuntimeException( err.str().c_str() );
      }
	  
      outfile = arg->getFilePointerValue();

      if( outfile == NULL ) {
	err << "The file with handle \"" << arg->getStringValue() << "\" has already been closed.";
	throw RuntimeException( err.str().c_str() );
      }
      file_pointer_found = true;
    }

    if( !file_pointer_found ) {
      outfile = env->getDefaultOutput();
      args.push_back( arg );
    }

    iter++;

    for ( ; iter != _arguments->end(); iter++ ) {
      arg = (*iter)->invoke( env );
      if (arg == NULL) throw InternalErrorException( "One of the arguments produces no value." );
      args.push_back( arg );
    }

    bool first_arg = true;
    bool second_arg = true;
    const char *separator_string = NULL;
    bool tab_spacing = false;
    int column_width = 10; /* default */

    std::vector<SymbolAutoRef >::iterator iter2;
    for (iter2 = args.begin(); iter2 != args.end(); ++iter2) {
      SymbolAutoRef& current = *iter2;

      /* if spacing flag was set interpret first argument as spacing string with which to separate all future arguments, if not a string error condition */
      if( _spacing ) {
	if( first_arg ){
	  if( current->getType() == Symbol::SYM_STRING ) {
	    separator_string = current->getStringValue();
	  } else if( current->getType() == Symbol::SYM_DOUBLE ) {
	    tab_spacing = true;
	    column_width = (int) current->getDoubleValue();
	  } else {  
	    throw InternalErrorException( "Second argument to print_spaced or println_spaced is not either a separator string or an integer specifying column width." );
	  }

	  first_arg = false;
	  continue;
	}
	if( second_arg )
	  second_arg = false;
	else { /* output separator string after second and succeeding arguments */
	  if( !tab_spacing ) {
	    if( separator_string ) ss << separator_string;
	  }
	}
      }

      if( tab_spacing ) {
	ss.width( column_width );
	ss.setf( std::ios::left );
      }

     switch (current->getType()) {
      case Symbol::SYM_BOOLEAN: 
	ss << (current->getBooleanValue() ? "true" : "false");
	break;
      case Symbol::SYM_DOUBLE: 
	ss << current->getDoubleValue();
	break;
      case Symbol::SYM_STRING:
	ss << current->getStringValue();
	break;
      case Symbol::SYM_NULL:
	ss << "(NULL)";
	break;
      case Symbol::SYM_OBJECT:
	ss << current->getObjectValue()->description();
	break;
     case Symbol::SYM_NEWLINE:
	fprintf( outfile, "%s\n", ss.str().c_str() );
	ss.str(""); // clear the string  
	break;
      case Symbol::SYM_UNINITIALIZED:
	ss << "<<uninitialized>>";
	break;
      default: 
	printf( "description: %s\n", current->description().c_str() );
	throw InternalErrorException( "Unsupported type passed to print function." );
	break;
      }

    } 

    fprintf( outfile, "%s%s", ss.str().c_str(), _newline ? "\n" : "" );
    fflush( outfile );

    return Symbol::encodeNull();
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
namespace LQX {

  ReadDataStatementNode::ReadDataStatementNode( const std::string& fileHandle, std::vector<SyntaxTreeNode*>* arguments ) :
    _fileHandle(fileHandle), _arguments(arguments)
  {
  }
  
  ReadDataStatementNode::~ReadDataStatementNode()
  {
  }

  void ReadDataStatementNode::debugPrintGraphviz( std::stringstream& output )
  {
    output << "NOT SUPPORTED";
  }

  SymbolAutoRef ReadDataStatementNode::invoke( Environment* env ) throw ( RuntimeException )
  {
    /* find open file pointer corresponding to fileHandle in Symbol table, error if not found */
    SymbolAutoRef symbol, key_symbol;
    double input_value;
    char input_string[128];
    const char *array_name, *key;
    FILE* infile;
    LanguageObject* lo;
    ArrayObject* array = 0;
    std::stringstream err;
    bool map_input = false;
    Symbol::Type value_type = Symbol::SYM_UNINITIALIZED;
    SymbolTable *table, *standardTable = env->getSymbolTable();
    SymbolTable *specialTable = env->getSpecialSymbolTable();

    if( strequal( _fileHandle.c_str(), "stdin" ) || strequal(  _fileHandle.c_str(), "-" ) )
      infile = stdin;
    else {

      if( !standardTable->isDefined( _fileHandle ) ) {
	err << "The file handle \"" << _fileHandle.c_str() << "\" could not be found.";
	throw RuntimeException( err.str().c_str() );
      }

      symbol = standardTable->get( _fileHandle );

      Symbol::Type type = symbol->getType();
      if( !((type == Symbol::SYM_FILE_WRITE_POINTER) || (type == Symbol::SYM_FILE_READ_POINTER)) ) {
	err << "The given variable \"" << _fileHandle.c_str() << "\" is not a file handle.";
	throw RuntimeException( err.str().c_str() );
      } else if( type != Symbol::SYM_FILE_READ_POINTER ) {
	err << "The given file handle  \"" << _fileHandle.c_str() << "\" has been opened for writing instead of reading.";
	throw RuntimeException( err.str().c_str() );
      }

      infile = symbol->getFilePointerValue();

      if( infile == NULL ) {
	err << "The file with handle \"" << _fileHandle.c_str() << "\" has already been closed.";
	throw RuntimeException( err.str().c_str() );
      }
    }

    if( _arguments != NULL ){

      std::vector<SymbolAutoRef > args;
    
      std::vector<SyntaxTreeNode*>::iterator iter;
      for (iter = _arguments->begin(); iter != _arguments->end(); iter++) {
	SymbolAutoRef arg = (*iter)->invoke( env );
	if (arg == NULL) throw InternalErrorException( "One of the arguments to read_data produces no value." );
	args.push_back( arg );
      }

      std::vector<SymbolAutoRef >::iterator iter2;
      for (iter2 = args.begin(); iter2 != args.end(); ++iter2) {
	SymbolAutoRef& current = *iter2;

	Symbol::Type type = current->getType();
	if( type == Symbol::SYM_DOUBLE ){
	  if( fscanf( infile, "%lf", &input_value ) != 1 ){
	    err << "Unable to read expected double value from the input file with handle \"" << _fileHandle.c_str() << "\".";
	    throw RuntimeException( err.str().c_str() );
	  }
	  current->assignDouble( input_value );
	} else if( type == Symbol::SYM_BOOLEAN ){
	  current->assignBoolean( readBoolean( infile ) );
	} else if( type == Symbol::SYM_STRING ) {
	  char unquoted_string[100] = "";
	  current->assignString( readString( infile, unquoted_string, NULL ) );
	} else
	  throw InternalErrorException( "One of the arguments to read_data is not of type double, boolean or string." );
	}

    } else {
      /*  read string/double pairs in loop from input file/pipe until termination string is encountered */
      char current_variable[50];

      while( true ) {

	map_input = false;
	if( fscanf( infile, "%127s", &input_string[0] ) != 1 ){
	  err << "Unable to read expected string value from the input file with handle \"" << _fileHandle.c_str() << "\".";
	  throw RuntimeException( err.str().c_str() );
	}

	if( strequal( input_string, "STOP_READ" ) )
	  return Symbol::encodeNull();

	table = ( input_string[0] == '$' ) ? specialTable : standardTable;

	if( !table->isDefined( input_string ) ) {
	  
	  int count = 0; // search for scalar array element in the form array_name["key"]

	  array_name = strtok( input_string, "[\"" );
	  if( array_name != NULL ) count++;
	  key = strtok( NULL, "\"]" );
	  if( key != NULL ) count++;

	  if( count == 2 ){

	    if( !table->isDefined( array_name ) ) {
	      err << "The map \"" << array_name << "\" could not be found.";
	      throw RuntimeException( err.str().c_str() );
	    }

	    symbol = table->get( array_name );
	    if (symbol->getType() != Symbol::SYM_OBJECT) {
	      err << "The variable \"" << array_name << "\" is not a map.";
	      throw RuntimeException( err.str().c_str() );
	    }
	    lo = symbol->getObjectValue();
	    if (lo->getTypeId() != kArrayObjectTypeId) {
	      err << "The variable \"" << array_name << "\" is not a map.";
	      throw RuntimeException( err.str().c_str() );
	    }

	    array = (ArrayObject *)lo;
	    key_symbol = Symbol::encodeString( key );
	    if( array->has( key_symbol ) ){
	      map_input = true;
	      value_type = array->get( key_symbol )->getType(); // determine type of value
	    } else {
	      err << "The key \"" << key << "\" could not be found in map \"" << array_name << "\".";
	      throw RuntimeException( err.str().c_str() );
	    }

	  } else {
	    err << "The variable \"" << input_string << "\" could not be found.";
	    throw RuntimeException( err.str().c_str() );
	  }
	}

	if( !map_input )
	  symbol = table->get( input_string );

	Symbol::Type type = symbol->getType();
	if( type == Symbol::SYM_DOUBLE ) {
	  if( fscanf( infile, "%lf", &input_value ) != 1 ){
	    err << "Unable to read expected double value from the input file with handle \"" << _fileHandle.c_str() << "\".";
	    throw RuntimeException( err.str().c_str() );
	  }
	  symbol->assignDouble( input_value );

	} else if( type == Symbol::SYM_BOOLEAN ) {
	  strcpy( current_variable, input_string );
	  symbol->assignBoolean( readBoolean( infile, current_variable ) );
	} else if( type == Symbol::SYM_STRING ) {
	  char unquoted_string[100] = "";
	  symbol->assignString( readString( infile, unquoted_string, input_string ) );

	} else if( type == Symbol::SYM_OBJECT ) {
	  if( value_type == Symbol::SYM_DOUBLE ) {
	    
	    if( fscanf( infile, "%lf", &input_value ) != 1 ){
	      err << "Unable to read expected double value from the input file with handle \"" << _fileHandle.c_str() << "\".";
	      throw RuntimeException( err.str().c_str() );
	    }
	    array->put( key_symbol, Symbol::encodeDouble( input_value ) );
	  } else if ( value_type == Symbol::SYM_BOOLEAN ) {
	    array->put( key_symbol, Symbol::encodeBoolean( readBoolean( infile ) ) );
	  } else if( value_type == Symbol::SYM_STRING ) {
	    char unquoted_string[100] = "";
	    array->put( key_symbol, Symbol::encodeString( readString( infile, unquoted_string, input_string ) ) );
	  }
	} else {
	  err << "The variable \"" << input_string << "\" is not of type double, boolean, string or a map.";
	  throw InternalErrorException( err.str().c_str() );
	}
      }

    }
    return Symbol::encodeNull();
  }

  char * ReadDataStatementNode::readString( FILE *infile,  char *unquoted_string, const char *variable )  throw ( RuntimeException )
  {

    char input_string[128], current_variable[50] = "", quoted_string[100] = "";
    std::stringstream err;

    if( variable )
      strcpy( current_variable, variable );
    bool first_word = true;
    while( true ) {
      if( fscanf( infile, "%127s", &input_string[0] ) != 1 ){
	err << "Unable to read expected quoted string from the input file with handle \"" << _fileHandle.c_str() << "\".";
	throw RuntimeException( err.str().c_str() );
      }
      if( first_word ) {
	first_word = false;
	if( input_string[0] != '"' ) {
	  if( variable )
	    err << "Expected string for variable \"" << current_variable << "\" is not quoted.";
	  else
	    err << "Expected string is not quoted.";
	  throw RuntimeException( err.str().c_str() );
	}
      } else
	strcat( quoted_string, " " );

      if( strlen( quoted_string ) + strlen( input_string ) > 100 ) {
	if( variable )
	  err << "Expected string for variable \"" << current_variable << "\" is not quoted.";
	else
	  err << "Expected string is not quoted.";
	throw RuntimeException( err.str().c_str() );
      } else
	strcat( quoted_string, input_string );

      if( input_string[strlen(input_string)-1] == '"' ) { break; }
    }

    strncpy( unquoted_string, quoted_string+1, strlen(quoted_string)-2 );
    return( unquoted_string );
  }

  bool ReadDataStatementNode::readBoolean( FILE *infile, const char *variable ) throw (RuntimeException)
  {
    char input_string[128];
    std::stringstream err;

    if( fscanf( infile, "%127s", &input_string[0] ) != 1 ){
      if( variable )
	err << "Unable to read expected boolean value for variable \"" << variable << "\" the input file with handle \"" << _fileHandle.c_str() << "\".";
      else
	err << "Unable to read expected boolean value from the input file with handle \"" << _fileHandle.c_str() << "\".";
      throw RuntimeException( err.str().c_str() );
    }
    if( strequal( input_string, "true" ) )
      return true;
    else if( strequal( input_string, "false" ) )
      return false;
    else {
      throw RuntimeException( "Value of boolean variable is not either true or false." );
      return false; // not reached
    }
  }

};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  ObjectPropertyReadNode::ObjectPropertyReadNode(SyntaxTreeNode* object, const std::string& property) :
    _objectNode(object), _propertyName(property)
  {
  }
  
  ObjectPropertyReadNode::~ObjectPropertyReadNode()
  {
    /* Clean up allocated memory */
    delete(_objectNode);
  }
  
  void ObjectPropertyReadNode::debugPrintGraphviz(std::stringstream& output)
  {
    output << "NOT SUPPORTED";
  }
  
  SymbolAutoRef ObjectPropertyReadNode::invoke(Environment* env) throw (RuntimeException)
  {
    /* First step is to grab the object */
    SymbolAutoRef symbol = _objectNode->invoke(env);
    
    /* Attempt to access the property of the object */
    if (symbol->getType() != Symbol::SYM_OBJECT) {
      throw RuntimeException("Property accesses must be performed on objects.");
    }
    
    /* Access the property from the language object */
    LanguageObject* lo = symbol->getObjectValue();
    return lo->getPropertyNamed(env, _propertyName);
  }
  
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  SymbolAutoRef FunctionDeclarationNode::LanguageImplementedMethod::invoke(Environment* env, std::vector<SymbolAutoRef>& args) throw (RuntimeException)
  {
    /* [1] Push a new invocation context */
    SymbolAutoRef result = Symbol::encodeNull();
    SymbolTable* mst = NULL;
    env->languageMethodPrologue();
    mst = env->getSymbolTable();
    
    /* [2] Copy any named arguments over */
    unsigned argsIndex = 0;
    std::vector<std::string>::const_iterator iter;
    for (iter = _argNames->begin(); iter != _argNames->end(); ++iter) {
      if (*iter == "...") {
        unsigned j = 0;
        ArrayObject* argsList = new ArrayObject();
        for (j = 0; j < (args.size() - argsIndex); ++j) { argsList->put(Symbol::encodeDouble(j), Symbol::duplicate(args[argsIndex+j])); }
        mst->define("_va_list");
        SymbolAutoRef sar = mst->get("_va_list");
        sar->assignObject(argsList);
        break;
      } else {
        if(!mst->define(*iter)) { throw RuntimeException("Multiple arguments have the same name to %s", _name.c_str()); }
        SymbolAutoRef sar = mst->get(*iter);
        sar->copyValue(*args[argsIndex++]);
      }
    }
    
    /* [3] Run the method while catching ReturnValue */
    try {
      const_cast<SyntaxTreeNode*>(_action)->invoke(env);
    } catch (const ReturnValue& rv) {
      result = const_cast<ReturnValue&>(rv).getValue();
    }
    
    /* [4] Tear apart the work we did */
    env->languageMethodEpilogue();
    return result;
  }
  
  FunctionDeclarationNode::FunctionDeclarationNode(const std::string& name, std::vector<std::string>* proto, SyntaxTreeNode* body) :
    _name(name), _prototype(proto), _body(body), _argTypes("")
  {
    /* Build the argument types string */
    if (_prototype->size() != 0) {
      std::stringstream ss;
      std::vector<std::string>::iterator iter;
      
      /* The idea is we add an `any' for all regular arguments and a `+' for an ellipsis */
      for (iter = _prototype->begin(); iter != _prototype->end(); ++iter) {
        if (*iter == "...") {
          ss << "+";
          break;
        } else {
          ss << "a";
        }
      }
      
      /* The argument type list is flattened */
      _argTypes = ss.str();
    }
  }
  
  FunctionDeclarationNode::~FunctionDeclarationNode()
  {
    /* Clean out the prototype */
    delete(_prototype);
  }
  
  void FunctionDeclarationNode::debugPrintGraphviz(std::stringstream& output)
  {
    /* There is no implementation right now */
    output << "NOT SUPPORTED";
  }
  
  SymbolAutoRef FunctionDeclarationNode::invoke(Environment* env) throw (RuntimeException)
  {
    /* Check if this method was registered yet */
    MethodTable* mt = env->getMethodTable();
    if (mt->getMethod(_name)) {
      std::cout << "THROW EXCEPTION HERE, INTERNAL ERROR.";
      return Symbol::encodeNull();
    }
    
    /* Since it was not, generate a nice little LanguageImplementedMethod for it */
    LanguageImplementedMethod* lim = new LanguageImplementedMethod(_name, _argTypes, _prototype, _body);
    mt->registerMethod(lim);
    return Symbol::encodeNull();
  }
  
};
