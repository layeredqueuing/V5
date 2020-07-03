/*
 *  SymbolTable.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 14/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "NonCopyable.h"
#include "SymbolTable.h"
#include "Environment.h"
#include "LanguageObject.h"

#include <cassert>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>

namespace LQX {
  
  SymbolAutoRef Symbol::encodeNull()
  {
    /* Return a referenced pointer to a null symbol */
    Symbol* symbol = new Symbol();
    symbol->assignNull();
    return SymbolAutoRef(symbol, false);
  }
  
  SymbolAutoRef Symbol::encodeBoolean(bool value)
  {
    /* Return a referenced pointer to a boolean symbol */
    Symbol* symbol = new Symbol();
    symbol->assignBoolean(value);
    return SymbolAutoRef(symbol, false);
  }
  
  SymbolAutoRef Symbol::encodeDouble(double value)
  {
    /* Return a referenced pointer to a numeric symbol */
    Symbol* symbol = new Symbol();
    symbol->assignDouble(value);
    return SymbolAutoRef(symbol, false);
  }
  
  SymbolAutoRef Symbol::encodeString(const char* value, bool release)
  {
    /* Return a referenced pointer to a string symbol */
    Symbol* symbol = new Symbol();
    symbol->assignString(value);
    if (release) free((void *)value);
    return SymbolAutoRef(symbol, false);
  }
  
  SymbolAutoRef Symbol::encodeObject(LanguageObject* object, bool derefWhenDone)
  {
    /* Return a referenced pointer to an object symbol */
    Symbol* symbol = new Symbol();
    symbol->assignObject(object);
    if (derefWhenDone) object->dereference();
    return SymbolAutoRef(symbol, false);
  }
  
  SymbolAutoRef Symbol::duplicate(const SymbolAutoRef& source)
  {
    /* Copy the constant value or duplicate the object */
    Symbol* symbol = new Symbol();
    if (source->_symbolType != SYM_OBJECT) {
      symbol->copyValue(*source.getStoredValue());
    } else {
      LanguageObject* lo2 = (source->_storedValue.objectValue)->duplicate();
      symbol->assignObject(lo2);
      lo2->dereference();
    }
    
    /* Return the reference to the object */
    return SymbolAutoRef(symbol, false);
  }
  
  Symbol::Symbol() : _symbolType(SYM_UNINITIALIZED), _isConstant(true)
  {
  }
  
  Symbol::Symbol(const Symbol& other)
  {
    /* Call up to the assignment operator */
    this->operator=(other);
  }
  
  Symbol::~Symbol()
  {
    /* Release memory for a string */
    if (_symbolType == SYM_STRING) {
      free((void *)_storedValue.stringValue);
    } else if (_symbolType == SYM_OBJECT) {
      _storedValue.objectValue->dereference();
    }
  }
  
  Symbol& Symbol::operator=(const Symbol& other)
  {
    /* Copy all the variables provided */
    copyValue(other);
    return *this;
  }
  
  void Symbol::copyValue(const Symbol& other)
  {
    /* Copy the value from the other symbol */
    _symbolType = other._symbolType;
    memcpy((void *)(&_storedValue),(void *)(&other._storedValue), sizeof(_storedValue));
    if (_symbolType == SYM_STRING) { _storedValue.stringValue = strdup(_storedValue.stringValue); }
    if (_symbolType == SYM_OBJECT) { _storedValue.objectValue->reference(); }
  }
  
  void Symbol::assignBoolean(bool value)
  {
    /* Make sure to prevent leaks and such */
    if (_symbolType == SYM_STRING) {
      free((void *)_storedValue.stringValue);
    } else if (_symbolType == SYM_OBJECT) {
      _storedValue.objectValue->dereference();
    }
    
    /* Make the symbol a boolean */
    _symbolType = SYM_BOOLEAN;
    _storedValue.booleanValue = value;
  }
  
  void Symbol::assignDouble(double value)
  {
    /* Make sure to prevent leaks and such */
    if (_symbolType == SYM_STRING) {
      free((void *)_storedValue.stringValue);
    } else if (_symbolType == SYM_OBJECT) {
      _storedValue.objectValue->dereference();
    }
    
    /* Make the symbol a double */
    _symbolType = SYM_DOUBLE;
    _storedValue.doubleValue = value;
  }
  
  void Symbol::assignString( const char* value )
  {
    /* Make sure to prevent leaks and such */
    if (_symbolType == SYM_STRING) {
      free((void *)_storedValue.stringValue);
    } else if (_symbolType == SYM_OBJECT) {
      _storedValue.objectValue->dereference();
    }
    
    /* Make the symbol a string */
    _symbolType = SYM_STRING;
    _storedValue.stringValue = strdup(value);
  }
  
  void Symbol::assignObject( LanguageObject* value )
  {
    /* Make sure to prevent leaks and such */
    if (_symbolType == SYM_STRING) {
      free((void *)_storedValue.stringValue);
    } else if (_symbolType == SYM_OBJECT) {
      _storedValue.objectValue->dereference();
    }
    
    /* Make the symbol a string */
    _symbolType = SYM_OBJECT;
    _storedValue.objectValue = value;
    value->reference();
  }

  void Symbol::assignFileWritePointer( FILE* value )
  {
    /* Make the symbol a file write pointer */
    _symbolType = SYM_FILE_WRITE_POINTER;
    _storedValue.filePointerValue = value;
  }
  
  void Symbol::assignFileReadPointer( FILE* value )
  {
    /* Make the symbol a file read pointer */
    _symbolType = SYM_FILE_READ_POINTER;
    _storedValue.filePointerValue = value;
  }
  
  void Symbol::assignNull()
  {
    /* Make sure to prevent leaks and such */
    if (_symbolType == SYM_STRING) {
      free((void *)_storedValue.stringValue);
    } else if (_symbolType == SYM_OBJECT) {
      _storedValue.objectValue->dereference();
    }
    
    /* Make the symbol a string */
    _symbolType = SYM_NULL;
    _storedValue.objectValue = NULL;
  }
  
  bool Symbol::getBooleanValue() const
  {
    /* Make sure the symbol is a boolean */
    assert(_symbolType == SYM_BOOLEAN);
    return _storedValue.booleanValue;
  }
  
  double Symbol::getDoubleValue() const
  {
    /* Make sure the symbol is a double */
    assert(_symbolType == SYM_DOUBLE);
    return _storedValue.doubleValue;
  }
  
  const char* Symbol::getStringValue() const
  {
    /* Make sure the symbol is a string */
    assert(_symbolType == SYM_STRING);
    return _storedValue.stringValue;
  }
  
  LanguageObject* Symbol::getObjectValue() const
  {
    /* Make sure the symbol is an object */
    assert(_symbolType == SYM_OBJECT);
    return _storedValue.objectValue;
  }

  FILE* Symbol::getFilePointerValue() const
  {
    /* Make sure the symbol is a file pointer */
    assert( _symbolType == SYM_FILE_WRITE_POINTER || _symbolType == SYM_FILE_READ_POINTER );
    return _storedValue.filePointerValue;
  }

  Symbol::Type Symbol::getType() const
  {
    /* Return the type */
    return _symbolType;
  }
  
  void Symbol::setIsConstant(const bool flag)
  {
    /* Store the constness flag */
    _isConstant = flag;
  }
  
  bool Symbol::isConstant() const
  {
    /* Return the constness flag */
    return _isConstant;
  }
  
  const std::string& Symbol::typeToString(Type t)
  {
    /* The names of all the types that are allowed */
    static const std::string symbolTypeNames[] = {
      "<<uninitialized>>",
      "boolean",
      "double",
      "string",
      "object",
      "file_write_pointer",
      "file_read_pointer",
      "newline",
      "null"
    };
    
    /* Return a reference to the symbol type name */
    return symbolTypeNames[static_cast<uint32_t>(t)];
  }
  
  bool Symbol::operator<(const Symbol& other) const
  {
    /* Divvy up by types first, then by values */
    if (_symbolType != other._symbolType) {
      return _symbolType < other._symbolType;
    } else {
      switch(_symbolType) {
        case SYM_UNINITIALIZED:
          return false;
        case SYM_BOOLEAN:
          return _storedValue.booleanValue < other._storedValue.booleanValue;
        case SYM_DOUBLE:
          return _storedValue.doubleValue < other._storedValue.doubleValue;
        case SYM_STRING:
          return strcmp(_storedValue.stringValue, other._storedValue.stringValue) < 0;
        case SYM_OBJECT:
          return _storedValue.objectValue->isLessThan(other._storedValue.objectValue);
        default:
          std::cout << "Warning: operator< called on undefined type..." << std::endl;
          return false;
      }
    }
  }
  
  bool Symbol::operator==(const Symbol& other) const
  {
    /* Divvy up by types first, then by values */
    if (_symbolType != other._symbolType) {
      return false;
    } else {
      switch(_symbolType) {
        case SYM_UNINITIALIZED:
          return true;
        case SYM_BOOLEAN:
          return _storedValue.booleanValue == other._storedValue.booleanValue;
        case SYM_DOUBLE:
          return _storedValue.doubleValue == other._storedValue.doubleValue;
        case SYM_STRING:
          return strcmp(_storedValue.stringValue, other._storedValue.stringValue) == 0;
        case SYM_OBJECT:
          return _storedValue.objectValue->isEqualTo(other._storedValue.objectValue);
        default:
          std::cout << "Warning: operator== called on undefined type..." << std::endl;
          return false;
      }
    }
  }
  
  const std::string& Symbol::getTypeName() const
  {
    /* Get the string representation of the type name */
    return Symbol::typeToString(this->getType());
  }
  
  std::string Symbol::description() const
  {
    std::ostringstream ss;
    if ( getType() == SYM_STRING ) {
      ss << "\"" << _storedValue.stringValue << "\"";		// Put quotes around the string.
    } else {
      print( ss );
    }
    return ss.str();
  }

  std::ostream& Symbol::print( std::ostream& output ) const
  {
    /* Find out what to say */
    switch (_symbolType) {
    case SYM_BOOLEAN:
      output << (_storedValue.booleanValue ? "true" : "false");
      break;
    case SYM_DOUBLE:
      output << _storedValue.doubleValue;
      break;
    case SYM_STRING:
      output << _storedValue.stringValue;
      break;
    case SYM_NULL:
      output << "(NULL)";
      break;
    case SYM_OBJECT:
      output << _storedValue.objectValue->description();
      break;
    case SYM_NEWLINE:
      output << std::endl;
      break;
    case SYM_UNINITIALIZED:
      output << "<<uninitialized>>";
      break;
    default:
      throw InternalErrorException("Unsupported type passed to print function.");
      break;
    }
     
     return output;
  }
  
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
  
  SymbolTable::SymbolTable() : _stack()
  {
    /* Push the root context */
    this->pushContext();
  }
  
  SymbolTable::~SymbolTable()
  {
    /* ReferencedPointer will delete everything automatically. */
    /* Destructor is empty. */
  }
  
  bool SymbolTable::define(const std::string& name)
  {
    /* Make sure this is not defined */
    if (this->isDefined(name, false)) {
      return false;
    }
    
    /* Obtain the symbol and define it */
    Symbol* symbol = new Symbol();
    _stack.back()[name] = SymbolAutoRef(symbol, false);

    return true;
  }
  
  bool SymbolTable::isDefined(const std::string& name, bool globally)
  {
    /* See if this is defined here */
    if (globally == false) {
      if (_stack.back().find(name) == _stack.back().end()) {
        return false;
      } else {
        return true;
      }
    }
    
    /* Check if the given symbol is defined globally */
    std::vector< std::map<std::string,SymbolAutoRef > >::reverse_iterator iter;
    for (iter = _stack.rbegin(); iter != _stack.rend(); ++iter) {
      std::map<std::string,SymbolAutoRef >& context = *iter;
      if (context.find(name) != context.end()) {
        return true;
      }
    }
    
    /* We searched everywhere */
    return false;
  }
  
  SymbolAutoRef SymbolTable::get(const std::string& name)
  {
    /* If this is undefined, return NULL */
    if(!this->isDefined(name)) {
      return SymbolAutoRef(NULL, false);
    }
    
    /* Check if the given symbol is defined globally */
    std::vector< std::map<std::string,SymbolAutoRef > >::reverse_iterator iter;
    for (iter = _stack.rbegin(); iter != _stack.rend(); ++iter) {
      std::map<std::string,SymbolAutoRef >& context = *iter;
      if (context.find(name) != context.end()) {
        SymbolAutoRef& symbol = context[name];
	// printf( "Symbol found - name: \"%s\" type: %s\n", name.c_str(), (symbol->getTypeName()).c_str() );
        return symbol;
      }
    }
    
    /* This is an error condition */
    //printf( "Error: \"%s\" not found in symbol table", name.c_str() );
    return SymbolAutoRef(NULL, false);
  }
  
  void SymbolTable::pushContext()
  {
    /* Push a new operating context onto the stack */
    _stack.push_back( std::map<std::string,SymbolAutoRef >() );
  }
  
  void SymbolTable::popContext()
  {
    /* Throw a variable context away */
    if (_stack.size() == 1) {
      throw InternalErrorException("Unmatched pushContext() and popContext() calls.");
    } else {
      _stack.pop_back();
    }
  }
  
  void SymbolTable::dump(std::stringstream& ss)
  {
    uint32_t level = 0;
    
    /* Display the headers and their value spacers */
    ss << std::setw(03) << std::left << " ";
    ss << std::setw(20) << std::left << " Name:";
    ss << std::setw(18) << std::left << " Type:";
    ss << std::setw(15) << std::left << " Value:" << std::endl;
    ss << "   +-------------------+-----------------+--------------->" << std::endl;
    
    /* Go through the table and output the values */
    for_each( _stack.rbegin(), _stack.rend(), DumpLevel( ss ) );
  }
  
  void SymbolTable::DumpLevel::operator()( const std::map<std::string, SymbolAutoRef >& level )
  {
    _level += 1;

    /* The iterator used to walk the level */
    std::map<std::string, SymbolAutoRef >::const_iterator iter;
    
    /* Output all the variables at the current level */
    for (iter = level.begin(); iter!= level.end(); ++iter) {
      const SymbolAutoRef& symbol = iter->second;
      std::stringstream value;
      try {
	symbol->print( value );
      }
      catch ( const InternalErrorException& e ) {	/* Ignore */
      }
      
      /* Print out the current line for the system */
      _ss << " " << std::setw(03) << std::left << _level;
      _ss << " " << std::setw(19) << std::left << iter->first;
      _ss << " " << std::setw(17) << std::left << symbol->getTypeName();
      _ss << " " << std::setw(15) << std::left << value.str() << std::endl;
    }
  }
  
  SymbolTable::SymbolTable(const SymbolTable&)
  {
    throw NonCopyableException();
  }
  
  SymbolTable& SymbolTable::operator=(const SymbolTable&)
  {
    throw NonCopyableException();
  }
  
};
