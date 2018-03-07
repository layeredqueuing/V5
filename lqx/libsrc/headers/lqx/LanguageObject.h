/*
 *  LanguageObject.h
 *  ModLang
 *
 *  Created by Martin Mroz on 29/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LANGUAGEOBJECT_H__
#define __LANGUAGEOBJECT_H__

#include "ReferenceCountedObject.h"
#include "RuntimeException.h"
#include <string>

namespace LQX {
  
  class Environmnent;
  class Symbol;
  
  class LanguageObject : public ReferenceCountedObject {
  public:
    
    /* Designated Initializers */
    LanguageObject(uint32_t typeId);
    virtual ~LanguageObject();
    
    /* Comparison and Operators */
    virtual bool isEqualTo(const LanguageObject* other) const;
    virtual bool isLessThan(const LanguageObject* other) const;
    virtual std::string description() const;
    virtual LanguageObject* duplicate();
    
    /* Properties of Language Objects */
    virtual uint32_t getTypeId() const;
    virtual std::string getTypeName() const;
    
    /* Support for Attributes -- Allow Objects to Bypass Method Addition for Simple Getters */
    virtual SymbolAutoRef getPropertyNamed(Environment* env, const std::string& name);
    
  private:
    
    /* Reference count info */
    uint32_t _typeId;
    
  };
  
}

#endif /* __LANGUAGEOBJECT_H__ */
