/*
 *  Array.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 04/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Array.h"
#include "Environment.h"
#include <sstream>
#include <iostream>

namespace LQX {
  
  const uint32_t kArrayObjectTypeId = 1;
  
  bool ArrayObject::ltSymbolType::operator()(SymbolAutoRef s1, SymbolAutoRef s2) const
  {
    /* Utilize the built-in comparison function for symbols */
    return *(s1.getStoredValue()) < *(s2.getStoredValue());
  }
  
  ArrayObject::ArrayObject() : LanguageObject(kArrayObjectTypeId), _map() 
  {
  }
  
  ArrayObject::~ArrayObject() 
  {
  }
  
  bool ArrayObject::isEqualTo(const LanguageObject* other) const
  {
    /* Check the Type Ids to make sure they're both Arrays */
    if (other->getTypeId() != kArrayObjectTypeId) return false;
    ArrayObject* otherArrayObject = (ArrayObject *)other;
    return ((this->_map) == otherArrayObject->_map);
  }
  
  bool ArrayObject::isLessThan(const LanguageObject* other) const
  {
    /* Check the Type Ids to make sure they're both Arrays */
    if (other->getTypeId() != kArrayObjectTypeId) {
      return this->LanguageObject::isLessThan(other);
    }
    
    /* Use the built-in operator< on std::map */
    ArrayObject* otherArrayObject = (ArrayObject *)other;
    bool lt = ((this->_map) < otherArrayObject->_map);
    return lt;
  }
  
  std::string ArrayObject::description()
  {
    /* Return a very simple description */
    std::stringstream ss;
    ss << "[";
    std::map<SymbolAutoRef,SymbolAutoRef>::iterator iter;
    for (iter = _map.begin(); iter != _map.end(); ++iter) {
      ss << iter->first->description() << "=>" << iter->second->description();
      if (iter != --(_map.end())) {
        ss << ", ";
      }
    }
    
    /* Finish up */
    ss << "]";
    return ss.str();
  }
  
  std::string ArrayObject::getTypeName()
  {
    /* Return the name of this type */
    return std::string("Array");
  }
  
  LanguageObject* ArrayObject::duplicate() throw (RuntimeException)
  {
    /* Do a value-copy on the internal array map */
    ArrayObject* copy = new ArrayObject();
    copy->_map = _map;
    return copy;
  }
  
  SymbolAutoRef ArrayObject::getPropertyNamed(Environment* env, const std::string& name) throw (RuntimeException)
  {
    /* Return the array size */
    if (name == "size") {
      return Symbol::encodeDouble(size());
    }
    
    /* Make sure that users can still access Object type variables */
    return this->LanguageObject::getPropertyNamed(env, name);
  }
  
  std::map<SymbolAutoRef,SymbolAutoRef>::iterator ArrayObject::begin()
  {
    return _map.begin();
  }
  
  std::map<SymbolAutoRef,SymbolAutoRef>::iterator ArrayObject::end()
  {
    return _map.end();
  }
  
  void ArrayObject::put(SymbolAutoRef key, SymbolAutoRef value)
  {
    /* Comparison is handled automatically */
    _map[key] = value;
  }
  
  SymbolAutoRef& ArrayObject::get(SymbolAutoRef key)
  {
    /* Return the array object pair for the given key */
    return _map[key];
  }
  
  bool ArrayObject::has(SymbolAutoRef key)
  {
    /* Check whether or not we have a value with the given key */
    return _map.find(key) != _map.end();
  }
  
  unsigned ArrayObject::size()
  {
    /* Return the size of the map */
    return _map.size();
  }

#pragma mark -
  
  ImplementLanguageMethod(ArrayObject::array_create)
  {
    /* Return a new instance of the ArrayObject type */
    ArrayObject* builtObject = new ArrayObject();
    
    /* Make the array */
    int i = 0;
    std::vector<SymbolAutoRef>::iterator iter;
    for (iter = args.begin(); iter != args.end(); ++iter) {
      SymbolAutoRef keySym = Symbol::encodeDouble(i++);
      SymbolAutoRef valueSym = *iter;
      valueSym->setIsConstant(false);
      builtObject->put(keySym, valueSym);
    }
    
    /* Return the encoded array with or without items */
    return Symbol::encodeObject(builtObject);
  }
  
  ImplementLanguageMethod(ArrayObject::array_create_map)
  {
    /* Return a new instance of the ArrayObject type */
    ArrayObject* builtObject = new ArrayObject();
    int max = args.size() / 2;
    int i = 0;
    
    /* Add all of the paired values */
    for (i = 0; i < max; ++i) {
      SymbolAutoRef keySym = args[(i * 2)];
      SymbolAutoRef valSym = args[(i * 2) + 1];
      valSym->setIsConstant(false);
      builtObject->put(Symbol::duplicate(keySym), valSym);
    }
    
    /* Add the tail value mapped to NULL */
    if ((args.size() % 2) == 1) {
      SymbolAutoRef keySym = args.back();
      SymbolAutoRef valSym = Symbol::encodeNull();
      valSym->setIsConstant(false);
      builtObject->put(Symbol::duplicate(keySym), valSym);
    }
    
    /* Return the encoded array with or without items */
    return Symbol::encodeObject(builtObject);
  }
  
  ImplementLanguageMethod(ArrayObject::array_set)
  {
    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    SymbolAutoRef key = args[1];
    SymbolAutoRef value = args[2];
    value->setIsConstant(false);
    
    /* Make sure things are what we think they are */
    if (lo->getTypeId() != kArrayObjectTypeId) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }
        
    /* Place the value into the array */
    (*((ArrayObject *)lo)).put(key, value);
    return Symbol::encodeNull();
  }
  
  ImplementLanguageMethod(ArrayObject::array_get)
  {
    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    SymbolAutoRef key = args[1];
    
    /* Make sure things are what we think they are */
    if (lo->getTypeId() != kArrayObjectTypeId) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }
    
    /* Obtain the hashable string */
    ArrayObject* array = (ArrayObject *)lo;
    if (array->has(key) == false) {
      SymbolAutoRef symbol = Symbol::encodeNull();
      symbol->setIsConstant(false);
      array->put(key, symbol);
      return symbol;
    }
    
    return array->get(key);
  }
  
  ImplementLanguageMethod(ArrayObject::array_has)
  {
    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    SymbolAutoRef key = args[1];
    
    /* Make sure things are what we think they are */
    if (lo->getTypeId() != kArrayObjectTypeId) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }
    
    /* Obtain the hashable string */
    ArrayObject* array = (ArrayObject *)lo;
    return Symbol::encodeBoolean(array->has(key));
  }

#pragma mark -
  
  void RegisterArrayClass(MethodTable* table)
  {
    /* Register all of the methods for the Array type */
    table->registerMethod(new ArrayObject::array_create());
    table->registerMethod(new ArrayObject::array_create_map());
    table->registerMethod(new ArrayObject::array_set());
    table->registerMethod(new ArrayObject::array_get());
    table->registerMethod(new ArrayObject::array_has());
  }
  
};
