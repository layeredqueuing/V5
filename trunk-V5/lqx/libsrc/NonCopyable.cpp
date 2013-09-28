/*
 *  NonCopyable.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 12/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "NonCopyable.h"

namespace LQX {
  
  NonCopyableException::NonCopyableException() throw()
  {
  }
  
  NonCopyableException::~NonCopyableException() throw()
  {
  }
  
  const char* NonCopyableException::what() const throw()
  {
    return "This object cannot be copied.";
  }

}

