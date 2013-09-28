/*
 *  Intrinsics.h
 *  ModLang
 *
 *  Created by Martin Mroz on 26/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __INTRINSICS_H__
#define __INTRINSICS_H__

#include "MethodTable.h"

namespace LQX {
  
  namespace Intrinsics {
    
    DeclareLanguageMethod(Copyright,        "",   "copyright",           "Displays the system copyright.");
    DeclareLanguageMethod(PrintSymbolTable, "",   "print_symbol_table",  "Displays the current symbol table state.");
    DeclareLanguageMethod(PrintSpecialTable,"",   "print_special_table", "Displays the current special table state.");
    
    DeclareLanguageMethod(Floor,            "d",  "floor",               "Returns the floor of the provided double.");    
    DeclareLanguageMethod(Ceil,             "d",  "ceil",                "Returns the ceil of the provided double.");
    DeclareLanguageMethod(Abs,              "d",  "abs",                 "Returns the absolute value of the double.");
    DeclareLanguageMethod(Pow,              "dd", "pow",                 "Raises arg1 to the arg2 power.");
    DeclareLanguageMethod(Rand,		    "",   "rand",		 "Returns a random number between 0 and 1.");
    DeclareLanguageMethod(Exp,		    "d",  "exp",		 "Returns `e' raised to arg1.");
    DeclareLanguageMethod(Log,		    "d",  "log",		 "Returns the natural logarithm of arg1.");
    DeclareLanguageMethod(Round,	    "d",  "round",		 "Returns arg1 rounded to the nearest integer.");
    DeclareLanguageMethod(Normal,	    "dd", "normal",		 "Returns a normally distributed random number with mean of arg1 and a standard deviation of arg2." );
    
    DeclareLanguageMethod(Str,              "+",  "str",                 "Coerce arguments list to flat string");
    DeclareLanguageMethod(Double,           "a",  "double",              "Attempt to convert the argument to a double");
    DeclareLanguageMethod(Boolean,          "a",  "bool",                "Attempt to convert the argument to a boolean");
    
    DeclareLanguageMethod(Abort,            "ds", "abort",               "Immediately halts flow of the program.");
    DeclareLanguageMethod(Return,           "a",  "return",              "Returns a Value out of a USER-DEFINED FUNCTION ONLY.");
    DeclareLanguageMethod(TypeID,           "a",  "type_id",             "Returns the Type ID of any given Symbol.");
    
  }
  
  /* Register the Intrinsics in a Method Table */
  void RegisterIntrinsics(MethodTable* methodTable);
  void RegisterIntrinsicConstants(SymbolTable* symbolTable);
  
}

#endif /* __INTRINSICS_H__ */
