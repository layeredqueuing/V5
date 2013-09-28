/*
 *  Strings.h
 *  ModLang
 *
 *  Created by Martin Mroz on 28/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "MethodTable.h"

namespace LQX {
  
  namespace Strings {
    DeclareLanguageMethod(str_cmp,    "ss", "str_cmp",    "Compare the strings");   /* (string s1, string s2) */
    DeclareLanguageMethod(str_concat, "ss", "str_concat", "Join the strings");      /* (string s1, string s2) */
  };
  
  /* Register the Intrinsics in a Method Table */
  void RegisterStrings(MethodTable* methodTable);
  
};
