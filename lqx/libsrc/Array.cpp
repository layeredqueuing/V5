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
    const ArrayObject* otherArrayObject = dynamic_cast<const ArrayObject *>(other);
    return otherArrayObject && (_map == otherArrayObject->_map);
  }
  
  bool ArrayObject::isLessThan(const LanguageObject* other) const
  {
    /* Check the Type Ids to make sure they're both Arrays */
    const ArrayObject* otherArrayObject = dynamic_cast<const ArrayObject *>(other);
    if ( !otherArrayObject ) return this->LanguageObject::isLessThan(other);

    /* Use the built-in operator< on std::map */
    return _map < otherArrayObject->_map;
  }
  
  std::string ArrayObject::description() const
  {
    /* output as CSV list */
    std::stringstream ss;
    std::map<SymbolAutoRef,SymbolAutoRef>::const_iterator iter;
    for (iter = _map.begin(); iter != _map.end(); ++iter) {
      if ( iter != _map.begin() ) ss << ", ";
      ss << iter->second->description();
    }
    /* Finish up */
    return ss.str();
  }
  
  std::string ArrayObject::getTypeName() const
  {
    /* Return the name of this type */
    return std::string("Array");
  }
  
  LanguageObject* ArrayObject::duplicate()
  {
    /* Do a value-copy on the internal array map */
    ArrayObject* copy = new ArrayObject();
    copy->_map = _map;
    return copy;
  }
  
  SymbolAutoRef ArrayObject::getPropertyNamed(Environment* env, const std::string& name)
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
  
  void ArrayObject::clear()
  {
    _map.clear();
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
  
  bool ArrayObject::has(SymbolAutoRef key) const
  {
    /* Check whether or not we have a value with the given key */
    return _map.find(key) != _map.end();
  }
  
  unsigned ArrayObject::size() const
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
  
  ImplementLanguageMethod(ArrayObject::array_clear)
  {
    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    
    /* Make sure things are what we think they are */
    ArrayObject* array = dynamic_cast<ArrayObject *>(lo);
    if ( !array ) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }

    array->clear();
    return Symbol::encodeNull();
  }

  ImplementLanguageMethod(ArrayObject::array_set)
  {
    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    SymbolAutoRef key = Symbol::duplicate(args[1]);	/* x[i] causes grief since i aliased */
    SymbolAutoRef value = args[2];
    value->setIsConstant(false);
    
    /* Make sure things are what we think they are */
    ArrayObject* array = dynamic_cast<ArrayObject *>(lo);
    if ( !array ) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }

    /* Place the value into the array.  We need a copy (like an assignment statement). */
    SymbolAutoRef symbol = Symbol::encodeNull();
    symbol->copyValue( *value );
    array->put(key, symbol);
    return Symbol::encodeNull();
  }
  
  ImplementLanguageMethod(ArrayObject::array_get)
  {
    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    SymbolAutoRef key = Symbol::duplicate(args[1]);	/* x[i] causes grief since i aliased */
    
    /* Make sure things are what we think they are */
    ArrayObject* array = dynamic_cast<ArrayObject *>(lo);
    if ( !array ) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }

    /* Obtain the hashable string */
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
    ArrayObject* array = dynamic_cast<ArrayObject *>(lo);
    if ( !array ) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }

    return Symbol::encodeBoolean(array->has(key));
  }

  ImplementLanguageMethod(ArrayObject::array_append)
  {
    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    
    /* Make sure things are what we think they are */
    ArrayObject* array = dynamic_cast<ArrayObject *>(lo);
    if ( !array ) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }
    std::vector<SymbolAutoRef>::iterator iter;
    for (iter = args.begin() + 1; iter != args.end(); ++iter) {
      SymbolAutoRef key = Symbol::encodeDouble(array->size());
      SymbolAutoRef symbol = Symbol::encodeNull();	/* Create a copy. */
      symbol->copyValue( **iter );
      array->put( key, symbol );
    }

    return Symbol::encodeObject(array);
  }
  
  ImplementLanguageMethod(ArrayObject::array_keys)
  {
    /* Return a new instance of the ArrayObject type */
    ArrayObject* builtObject = new ArrayObject();

    /* Decode all of the arguments from the list given */
    LanguageObject* lo = decodeObject(args, 0);
    
    /* Make sure things are what we think they are */
    ArrayObject* array = dynamic_cast<ArrayObject *>(lo);
    if ( !array ) {
      throw RuntimeException("Argument is not an instance of `Array'");
    }
    int i = 0;
    for ( std::map<SymbolAutoRef,SymbolAutoRef>::const_iterator iter = array->_map.begin(); iter != array->_map.end(); ++iter ) {
      SymbolAutoRef keySym = Symbol::encodeDouble(i++);
      SymbolAutoRef symbol = Symbol::encodeNull();	/* Create a copy. */
      symbol->copyValue( *iter->first );
      builtObject->put(keySym, symbol);
      
    }

    return Symbol::encodeObject(builtObject);
  }


#pragma mark -
  
  void RegisterArrayClass(MethodTable* table)
  {
    /* Register all of the methods for the Array type */
    table->registerMethod(new ArrayObject::array_create());
    table->registerMethod(new ArrayObject::array_create_map());
    table->registerMethod(new ArrayObject::array_clear());
    table->registerMethod(new ArrayObject::array_set());
    table->registerMethod(new ArrayObject::array_get());
    table->registerMethod(new ArrayObject::array_has());
    table->registerMethod(new ArrayObject::array_append());
    table->registerMethod(new ArrayObject::array_keys());
  }
  
};
