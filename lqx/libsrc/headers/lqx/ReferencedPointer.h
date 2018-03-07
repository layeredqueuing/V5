/*
 *  ReferencedPointer.h
 *  ModLang
 *
 *  Created by Martin Mroz on 12/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQX_REFERENCED_POINTER__
#define __LQX_REFERENCED_POINTER__

#include <iostream>
#include <cassert>

namespace LQX {
  
  template <typename T> class ReferencedPointer {
  public:  
    
    ReferencedPointer<T>() : _value(NULL)
    {
    }
    
    ReferencedPointer<T>(T* value, bool addNewReference=false) : _value(value) 
    {
      /* Increment the reference count */
      if (addNewReference && _value != NULL) {
        _value->reference();
      }
    }
    
    ~ReferencedPointer()
    {
      /* Decrement the reference count */
      if (_value != NULL) {
        _value->dereference();
      }
    }
    
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
    ReferencedPointer<T>(const ReferencedPointer<T>& other)
    {
      /* Add a reference to the value */
      _value = other._value;
      if (_value != NULL) {
        _value->reference();
      }
    }
    
    ReferencedPointer<T>& operator=(const ReferencedPointer<T>& other)
    {
      /* Drop a reference to the other value */
      if (_value != NULL) {
        _value->dereference();
      }
      
      /* Add a reference to the value */
      _value = other._value;
      if (_value != NULL) {
        _value->reference();
      }
      
      return *this;
    }
    
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
    T& operator*() const
    {
      assert (_value != NULL);
      return *_value;
    }
    
    T* operator->() const
    {
      return _value;
    }
    
    T* getStoredValue() const
    {
      return _value;
    }
    
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
    bool operator==(const ReferencedPointer<T>& other) const
    {
      if (_value == NULL && other._value == NULL) { return true; }
      if (_value == NULL || other._value == NULL) { return false; }
      if (_value == other._value) { return true; }
      return (*_value == *other._value);
    }
    
    bool operator<(const ReferencedPointer<T>& other) const
    {
      if (_value == NULL && other._value == NULL) { return false; }
      if (_value == NULL) { return true; }
      if (other._value == NULL) { return false; }
      if (_value == other._value) { return false; }
      return (*_value < *other._value);
    }

    bool operator!=(const ReferencedPointer<T>& other) const
    {
      return !(*this == other);
    }
    
  private:
    
    /* The stored pointer */
    T* _value;
    
  };
  
}

#endif /* __LQX_REFERENCED_POINTER__ */

