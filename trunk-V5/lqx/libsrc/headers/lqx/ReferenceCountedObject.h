/*
 *  ReferenceCountedObject.h
 *  ModLang
 *
 *  Created by Martin Mroz on 12/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQX_REFERENCECOUNTEDOBJECT_H__
#define __LQX_REFERENCECOUNTEDOBJECT_H__

#include <stdint.h>
#include <iostream>
#include <cassert>

namespace LQX {
  
  class ReferenceCountedObject {
  public:
    
    /* Constructor and Destructor (initial refCount = 1) */
    ReferenceCountedObject();
    virtual ~ReferenceCountedObject();
    
    /* Public interface for interacting with such objects */
    ReferenceCountedObject* reference();
    void dereference();
    uint32_t getReferenceCount();
    
  private:
    
    /* Instance Variables */
    uint32_t _refCount;
    
  };
  
}

#endif /* __LQX_REFERENCECOUNTEDOBJECT_H__ */
