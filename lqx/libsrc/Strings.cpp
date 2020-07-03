/*
 *  Strings.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 28/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "LQXStrings.h"

#include "SymbolTable.h"
#include "Environment.h"
#include "LanguageObject.h"

#include <sstream>
#include <cstring>
#include <cstdlib>

namespace LQX {
  namespace Strings {
    
    /* This method on the other hand actually does all the heavy lifting */
    SymbolAutoRef str_cmp::invoke(Environment*, std::vector<SymbolAutoRef >& args)
    {
      /* Pass the arguments up to strcmp */
      const char* s1 = decodeString(args, 0);
      const char* s2 = decodeString(args, 1);
      return Symbol::encodeDouble((double)strcmp(s1,s2));
    }
    
  };
  
  namespace Strings {
    
    /* This method on the other hand actually does all the heavy lifting */
    SymbolAutoRef str_concat::invoke(Environment*, std::vector<SymbolAutoRef >& args)
    {
      /* Pass the arguments up to strcmp */
      const char* s1 = decodeString(args, 0);
      const char* s2 = decodeString(args, 1);
      uint32_t s1Length = strlen(s1);
      uint32_t s2Length = strlen(s2);
      char* result = (char *)malloc(s1Length+s2Length+1);
      memcpy(result, s1, s1Length);
      memcpy(result+s1Length, s2, s2Length);
      result[s1Length+s2Length] = '\0';
      return Symbol::encodeString(result, true);
    }
    
  };
  
  void RegisterStrings(MethodTable* methodTable)
  {
    /* Register all the String package methods */
    methodTable->registerMethod(new Strings::str_cmp());
    methodTable->registerMethod(new Strings::str_concat());
  }
  
};
