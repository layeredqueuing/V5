/*
 *  ReferenceCountedObject.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 12/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "ReferenceCountedObject.h"

namespace LQX {
  
  ReferenceCountedObject::ReferenceCountedObject() : _refCount(1)
  {
  }
  
  ReferenceCountedObject::~ReferenceCountedObject()
  {
    /* Make sure we're down to zero */
    assert(_refCount == 0);
  }
  
  ReferenceCountedObject* ReferenceCountedObject::reference()
  {
    /* Add a reference */
    _refCount++;
    return this;
  }
  
  void ReferenceCountedObject::dereference()
  {
    /* Drop a reference */
    if (--_refCount == 0) {
      delete(this);
    }
  }
  
  uint32_t ReferenceCountedObject::getReferenceCount()
  {
    return _refCount;
  }
  
}
