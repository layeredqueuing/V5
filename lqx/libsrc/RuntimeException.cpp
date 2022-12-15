/*
 *  RuntimeException.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 22/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "RuntimeException.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <sstream>
#include <string>
#include <SyntaxTree.h>

namespace LQX {
  
  RuntimeException::RuntimeException(const std::string format, ...) throw()	// Can't be reference due to ...
  {
    /* Format the string */
    va_list ap;
    va_start(ap, format);
    char result[1024];
    vsnprintf(result, 1024, format.c_str(), ap);
    _reason = std::string(result);
  }
  
  RuntimeException::~RuntimeException() throw()
  {
  }
  
  const char* RuntimeException::what() const throw()
  {
    return _reason.c_str();
  }
  
  std::string& RuntimeException::insert( size_t pos, const std::string& s ) 
  {
    return _reason.insert( pos, s );
  }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  UndefinedVariableException::UndefinedVariableException(const std::string& variableName) throw()
    : RuntimeException("Undefined Variable `%s'", variableName.c_str())
  {
  }
  
  UndefinedVariableException::~UndefinedVariableException() throw()
  {
  }
  
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  IncompatibleTypeException::IncompatibleTypeException(const SyntaxTreeNode* expr, const std::string& from, const std::string& to) throw()
    : RuntimeException("Unable to Convert From: `%s' To: `%s'", from.c_str(), to.c_str() )
  {
    std::stringstream s;
    s << " `" << *expr << "'";	/* If `expr' is a variable node, then this will get the name of the variable */
    insert( 17, s.str() );
  }
  
  IncompatibleTypeException::IncompatibleTypeException(const std::string& from, const std::string& to) throw()
    : RuntimeException("Unable to Convert From: `%s' To: `%s'", from.c_str(), to.c_str())
  {
  }
  
  IncompatibleTypeException::IncompatibleTypeException(const std::string& name) throw()
    : RuntimeException(name)
  {
  }
  
  IncompatibleTypeException::~IncompatibleTypeException() throw()
  {
  }
  
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  InternalErrorException::InternalErrorException(const std::string& message) throw()
    : RuntimeException(message)
  {
  }
  
  InternalErrorException::~InternalErrorException() throw()
  {
  }
  
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  ArgumentMismatchException::ArgumentMismatchException(const std::string& name, int has, int min, int max) throw()
    : RuntimeException("Function %s needs between %d and %d arguments, but has %d", name.c_str(), min, max, has)
  {
  }
  
  ArgumentMismatchException::ArgumentMismatchException(const std::string& name, const std::string& is, const std::string& shouldBe) throw()
    : RuntimeException("Argument to function %s is %s but should be %s", name.c_str(), is.c_str(), shouldBe.c_str())
  {
  }
  
  ArgumentMismatchException::~ArgumentMismatchException() throw()
  {
  }

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {

  AbortException::AbortException(const std::string& whyAbort, double code) throw()
    : RuntimeException("abort() function called, code = %f, reason: %s",
	  code, whyAbort.c_str()) 
  {
  }

  AbortException::~AbortException() throw()
  {
  }
  
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  InvalidPropertyException::InvalidPropertyException(const std::string& typeName, const std::string& propertyName) throw()
    : RuntimeException("Object of type %s does not have property %s",
    typeName.c_str(), propertyName.c_str()) 
  {
  }
  
  InvalidPropertyException::~InvalidPropertyException() throw()
  {
  }
  
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  IndexNotValidException::IndexNotValidException(const std::string& keyDesc, const std::string& arrayDesc) throw()
  : RuntimeException("Index %s is not valid for array %s", keyDesc.c_str(), arrayDesc.c_str())
  {
  }
  
  IndexNotValidException::~IndexNotValidException() throw()
  {
  }
  
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQX {
  
  InvalidArgumentException::InvalidArgumentException(const std::string& name, const std::string& arg) throw()
  : RuntimeException("Invalid argument to function %s: %s", name.c_str(), arg.c_str())
  {
  }
  
  InvalidArgumentException::~InvalidArgumentException() throw()
  {
  }
  
}
