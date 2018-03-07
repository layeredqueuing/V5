/*
 *  Array.h
 *  ModLang
 *
 *  Created by Martin Mroz on 04/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "MethodTable.h"
#include "LanguageObject.h"
#include <map>

namespace LQX {
  
  /* The Unregistered Type ID for Arrays is 1 */
  extern const uint32_t kArrayObjectTypeId;
  
  /* This is the array object, which uses an std::map internally */
  class ArrayObject : public LanguageObject {
    friend void RegisterArrayClass(MethodTable* table);
    
  private:
    
    /* Define all of the methods that you can call for/on/with an array object instance */
    DeclareLanguageMethod(array_create,     "+",   "array_create",     "Returns a new instance of Array.");
    DeclareLanguageMethod(array_create_map, "+",   "array_create_map", "Returns a new instance of Array from map arguments.");
    DeclareLanguageMethod(array_clear,      "o",   "array_clear",      "Clears all elements from the Array." );
    DeclareLanguageMethod(array_set,        "oaa", "array_set",        "Sets a value for a given key on an Array.");
    DeclareLanguageMethod(array_get,        "oa",  "array_get",        "Returns the value for the given key in an Array.");
    DeclareLanguageMethod(array_has,        "oa",  "array_has",        "Returns whether or not the Array contains the object.");
    DeclareLanguageMethod(array_append,     "o+",  "array_append",     "Appends the items to end of the Array." );
    
  public:
    
    /* Class Structors */
    ArrayObject();
    virtual ~ArrayObject();
    
    /* Optional overrides for testing */
    virtual bool isEqualTo(const LanguageObject* other) const;
    virtual bool isLessThan(const LanguageObject* other) const;
    virtual std::string description() const;
    virtual std::string getTypeName() const;
    virtual LanguageObject* duplicate();
    
    /* This method allows objects to expose a set of properties to the language environment */
    virtual SymbolAutoRef getPropertyNamed(Environment* env, const std::string& name);
    
    /* Accessing and Mutating the Underlying Set */
    std::map<SymbolAutoRef,SymbolAutoRef>::iterator begin();
    std::map<SymbolAutoRef,SymbolAutoRef>::iterator end();
    void clear();
    void put(SymbolAutoRef key, SymbolAutoRef value);
    SymbolAutoRef& get(SymbolAutoRef key);
    bool has(SymbolAutoRef key) const;
    unsigned size() const;
    
  private:
    
    /* This is where the data is really stored */
    struct ltSymbolType { bool operator()(SymbolAutoRef s1, SymbolAutoRef s2) const; };
    std::map<SymbolAutoRef,SymbolAutoRef,ltSymbolType> _map;
    
  };
  
  /* Register the "Array" type within the language itself */
  extern void RegisterArrayClass(MethodTable* table);
  
}

#endif /* __ARRAY_H__ */
