/*
 *  LanguageObject.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 29/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Environment.h"
#include "SymbolTable.h"
#include "LanguageObject.h"
#include <sstream>

namespace LQX {
  
  LanguageObject::LanguageObject(uint32_t typeId) : _typeId(typeId)
  {
  }
  
  LanguageObject::~LanguageObject()
  {
  }
  
  bool LanguageObject::isEqualTo(const LanguageObject* other) const
  {
    /* Default operation is to compare pointers */
    return this == other;
  }
  
  bool LanguageObject::isLessThan(const LanguageObject* other) const
  {
    /* Default is to compare type Ids */
    return (_typeId < other->_typeId);
  }
  
  std::string LanguageObject::description() const
  {
    /* Return the description */
    return "Object";
  }
  
  LanguageObject* LanguageObject::duplicate()
  {
    /* Unless this is overridden by the subclass this cannot ever happen */
    throw RuntimeException("Object of type %s cannot be duplicated.", this->getTypeName().c_str());
  }
  
  uint32_t LanguageObject::getTypeId() const
  {
    /* Return the type id */
    return _typeId;
  }
  
  std::string LanguageObject::getTypeName() const
  {
    /* Return the type name */
    return "Object";
  }
  
  SymbolAutoRef LanguageObject::getPropertyNamed(Environment*, const std::string& name)
  {
    /* All we support is type id's */
    if (name == "type_id") {
      return Symbol::encodeDouble(this->getTypeId());
    } else if (name == "type_name") {
      return Symbol::encodeString(this->getTypeName().c_str());
    }
    
    /* This particular object does not support this particular property */
    throw InvalidPropertyException(this->getTypeName(), name);
    return NULL;
  }
  
}
