/*
 *  NonCopyable.h
 *  ModLang
 *
 *  Created by Martin Mroz on 12/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQX_NONCOPYABLE_H__
#define __LQX_NONCOPYABLE_H__

#include <exception>

namespace LQX {
  
  class NonCopyableException : public std::exception {
  public:
    NonCopyableException() throw();
    virtual ~NonCopyableException() throw();
    virtual const char* what() const throw();
  };
  
}

#endif /* __LQX_NONCOPYABLE_H__ */
