/*
 *  Intrinsics.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 26/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include <sstream>

#include <config.h>
#include "Intrinsics.h"
#include "SymbolTable.h"
#include "Environment.h"
#include "LanguageObject.h"
#include "RuntimeFlowControl.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace LQX {

    static unsigned short xsubi[3] = { 0x0123, 0x4567, 0x89ab };

    namespace Intrinsics {

	inline double get_infinity()
	{
#if defined(INFINITY)
	    return INFINITY;
#else
	    union {
		unsigned char c[8];
		double f;
	    } x;

#if WORDS_BIGENDIAN
	    x.c[0] = 0x7f;
	    x.c[1] = 0xf0;
	    x.c[2] = 0x00;
	    x.c[3] = 0x00;
	    x.c[4] = 0x00;
	    x.c[5] = 0x00;
	    x.c[6] = 0x00;
	    x.c[7] = 0x00;
#else
	    x.c[7] = 0x7f;
	    x.c[6] = 0xf0;
	    x.c[5] = 0x00;
	    x.c[4] = 0x00;
	    x.c[3] = 0x00;
	    x.c[2] = 0x00;
	    x.c[1] = 0x00;
	    x.c[0] = 0x00;
#endif
	    return x.f;
#endif
	}

	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Copyright::invoke(Environment*, std::vector<SymbolAutoRef >& ) throw (RuntimeException)
	{
	    /* Prints out all the given values if any */
	    std::cout << "Copyright (C) 2009 Carleton University." << std::endl;
	    std::cout << "Written by Martin Mroz for C.M.Woodside and G.Franks" << std::endl;
	    return Symbol::encodeNull();
	}
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef PrintSymbolTable::invoke(Environment* env, std::vector<SymbolAutoRef >& ) throw (RuntimeException)
	{
	    /* Debug print the table */
	    std::stringstream ss;
	    env->getSymbolTable()->dump(ss);
	    std::cout << ss.str() << std::endl;
	    return Symbol::encodeNull();
	}
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef PrintSpecialTable::invoke(Environment* env, std::vector<SymbolAutoRef >& ) throw (RuntimeException)
	{
	    /* Debug print the table */
	    std::stringstream ss;
	    env->getSpecialSymbolTable()->dump(ss);
	    std::cout << ss.str() << std::endl;
	    return Symbol::encodeNull();
	}
    
    }
  
#if !HAVE_DRAND48
    static double erand48( unsigned short xsubi[3] );
#endif
    
    namespace Intrinsics {
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Floor::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* Debug print the table */
	    double arg = decodeDouble(args, 0);
	    return Symbol::encodeDouble(floor(arg));
	}
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Ceil::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* Debug print the table */
	    double arg = decodeDouble(args, 0);
	    return Symbol::encodeDouble(ceil(arg));
	}
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Abs::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* Debug print the table */
	    double arg = decodeDouble(args, 0);
	    return Symbol::encodeDouble(fabs(arg));
	}
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Pow::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* Debug print the table */
	    double arg1 = decodeDouble(args, 0);
	    double arg2 = decodeDouble(args, 1);
	    return Symbol::encodeDouble(pow(arg1, arg2));
	}
    
	SymbolAutoRef Rand::invoke(Environment* , std::vector<SymbolAutoRef >& ) throw (RuntimeException)
	{
	    return Symbol::encodeDouble(erand48(xsubi));		/* We use this because others may use drand48() */
	}

	SymbolAutoRef Exp::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    double arg1 = decodeDouble(args, 0);
	    return Symbol::encodeDouble(exp(arg1));
	}

	SymbolAutoRef Log::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    double arg1 = decodeDouble(args, 0);
	    return Symbol::encodeDouble(log(arg1));
	}

	SymbolAutoRef Round::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    double arg1 = decodeDouble(args, 0);
	    return Symbol::encodeDouble(round(arg1));
	}

	SymbolAutoRef Normal::invoke(Environment* , std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* See Jain, Pg 494 - Convolution method with n = 12 */
	    const double mean = decodeDouble(args, 0);
	    const double stddev = decodeDouble(args, 1);
	    
	    double sum = 0.0;
	    for ( unsigned int i = 0; i < 12; ++i ) {
		sum += erand48(xsubi);
	    }

	    return Symbol::encodeDouble( mean + stddev * (sum - 6) );
	}
    }
  
    namespace Intrinsics {
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Str::invoke(Environment*, std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* Result string goes here */
	    std::stringstream ss;
      
	    /* Prints out all the given values if any */
	    std::vector<SymbolAutoRef>::iterator iter;
	    for (iter = args.begin(); iter != args.end(); ++iter) {
		SymbolAutoRef& current = *iter;
		switch (current->getType()) {
		case Symbol::SYM_BOOLEAN: 
		    ss << (current->getBooleanValue() ? "true" : "false");
		    break;
		case Symbol::SYM_DOUBLE: 
		    ss << current->getDoubleValue();
		    break;
		case Symbol::SYM_STRING:
		    ss << current->getStringValue();
		    break;
		case Symbol::SYM_NULL:
		    ss << "(NULL)";
		    break;
		case Symbol::SYM_OBJECT:
		    ss << current->getObjectValue()->description();
		    break;
		case Symbol::SYM_UNINITIALIZED:
		    ss << "<<uninitialized>>";
		    break;
		default: 
		    throw InternalErrorException("Unsupported type passed to str() function.");
		    break;
		}
	    } 
      
	    /* Pass the arguments up to strcmp */
	    return Symbol::encodeString(ss.str().c_str(), false);
	}
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Double::invoke(Environment*, std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    SymbolAutoRef& current = args[0];
	    double decodedValue = 0.0;
	    bool got = true;
      
	    /* Try to do the type conversion */
	    switch (current->getType()) {
	    case Symbol::SYM_BOOLEAN: {
		decodedValue = (current->getBooleanValue() == true) ? 1.0 : 0.0;
		break;
	    } 
	    case Symbol::SYM_DOUBLE: {
		decodedValue = current->getDoubleValue();
		break;
	    } 
	    case Symbol::SYM_STRING: {
		const char* rawValue = current->getStringValue();
		if (!isdigit(rawValue[0])) { got = false; break; }
		char* endPtr = NULL;
		decodedValue = strtod(rawValue, &endPtr);
		if (endPtr[0] != '\0') { got = false; }
		break;
	    } 
	    case Symbol::SYM_NULL: {
		decodedValue = 0.0;
		break;
	    } 
	    case Symbol::SYM_OBJECT: {
		got = false;
		break;
	    } 
	    default: {
		throw InternalErrorException("Unsupported type passed to str() function.");
		break;
	    }
	    }
      
	    /* We didn't get anything good here */
	    if (got == false) {
		return Symbol::encodeNull();
	    }
      
	    /* Return the decoded value from the provided input */
	    return Symbol::encodeDouble(decodedValue);
	}
    
	/* This method on the other hand actually does all the heavy lifting */
	SymbolAutoRef Boolean::invoke(Environment*, std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    SymbolAutoRef& current = args[0];
	    bool decodedValue = false;
	    bool got = true;
      
	    /* Try to do the type conversion */
	    switch (current->getType()) {
	    case Symbol::SYM_BOOLEAN: {
		decodedValue = current->getBooleanValue();
		break;
	    } 
	    case Symbol::SYM_DOUBLE: {
		decodedValue = current->getDoubleValue() != 0.0;
		break;
	    } 
	    case Symbol::SYM_STRING: {
		const char* rawValue = current->getStringValue();
		if (!strcasecmp(rawValue, "true") || !strcasecmp(rawValue, "yes")) { decodedValue = true; }
		else if (!strcasecmp(rawValue, "false") || !strcasecmp(rawValue, "no")) { decodedValue = false; }
		else { got = false; }
		break;
	    } 
	    case Symbol::SYM_NULL: {
		decodedValue = false;
		break;
	    } 
	    case Symbol::SYM_OBJECT: {
		got = false;
		break;
	    } 
	    default: {
		throw InternalErrorException("Unsupported type passed to str() function.");
		break;
	    }
	    }
      
	    /* We didn't get anything good here */
	    if (got == false) {
		return Symbol::encodeNull();
	    }
      
	    /* Return the decoded value from the provided input */
	    return Symbol::encodeBoolean(decodedValue);      
	}
    
    
    }
  
    namespace Intrinsics {
    
	SymbolAutoRef Abort::invoke(Environment*, std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    const char* reason = decodeString(args, 1);
	    double code = decodeDouble(args, 0);
	    throw AbortException(reason, code);
	}
    
	SymbolAutoRef Return::invoke(Environment* env, std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* Try to find out if we are in a language-defined method */
	    if (env->isExecutingInMainContext()) {
		std::cerr << "WARNING: Attempt to return() out of the main context will fail." << std::endl;
		std::cerr << "WARNING: You may only ever invoke return() out of a user-defined function." << std::endl;
		return Symbol::encodeNull();
	    }
      
	    /* This will be caught within the LanguageImplementedMethod wrapper */
	    throw ReturnValue(Symbol::duplicate(args[0]));
	}
    
	SymbolAutoRef TypeID::invoke(Environment*, std::vector<SymbolAutoRef >& args) throw (RuntimeException)
	{
	    /* Find out what the argument here is */
	    SymbolAutoRef& symbol = args[0];
	    Symbol::Type t = symbol->getType();
      
	    /* If this is an object, return the Object Type ID */
	    if (t == Symbol::SYM_OBJECT) {
		LanguageObject* lo = symbol->getObjectValue();
		return Symbol::encodeDouble(lo->getTypeId());
	    }
      
	    /* All of the language types are negative */
	    return Symbol::encodeDouble(-1 * static_cast<double>(t));
	}
    
    }
  
    void RegisterIntrinsics(MethodTable* table)
    {
	xsubi[0] = 0x0123;
	xsubi[1] = 0x4567;
	xsubi[2] = 0x89ab;

	/* Register all of the intrinsic methods in the table */
	table->registerMethod(new Intrinsics::Copyright());
	table->registerMethod(new Intrinsics::PrintSymbolTable());
	table->registerMethod(new Intrinsics::PrintSpecialTable());
	table->registerMethod(new Intrinsics::Abs());
	table->registerMethod(new Intrinsics::Ceil());
	table->registerMethod(new Intrinsics::Exp());
	table->registerMethod(new Intrinsics::Floor());
	table->registerMethod(new Intrinsics::Log());
	table->registerMethod(new Intrinsics::Pow());
	table->registerMethod(new Intrinsics::Rand());
	table->registerMethod(new Intrinsics::Round());
	table->registerMethod(new Intrinsics::Normal());
	table->registerMethod(new Intrinsics::Str());
	table->registerMethod(new Intrinsics::Double());
	table->registerMethod(new Intrinsics::Boolean());
	table->registerMethod(new Intrinsics::Abort());
	table->registerMethod(new Intrinsics::Return());
	table->registerMethod(new Intrinsics::TypeID());
    }
  
    static inline void registerConstantDouble(SymbolTable* symbolTable, std::string name, double value)
    {
	/* Define and set up the constant */
	symbolTable->define(name);
	SymbolAutoRef sar = symbolTable->get(name);
	sar->assignDouble(value);
	sar->setIsConstant(true);
    }
  
    void RegisterIntrinsicConstants(SymbolTable* symbolTable)
    {
	/* Register the variable type constants for all of the intrinsic types. Classes have to deal with their own... */
	registerConstantDouble(symbolTable, "@type_un",      -1 * static_cast<double>(Symbol::SYM_UNINITIALIZED));
	registerConstantDouble(symbolTable, "@type_boolean", -1 * static_cast<double>(Symbol::SYM_BOOLEAN));
	registerConstantDouble(symbolTable, "@type_double",  -1 * static_cast<double>(Symbol::SYM_DOUBLE));
	registerConstantDouble(symbolTable, "@type_string",  -1 * static_cast<double>(Symbol::SYM_STRING));
	registerConstantDouble(symbolTable, "@type_null",    -1 * static_cast<double>(Symbol::SYM_NULL));
    
	/* Register the Infinity Constant */
	registerConstantDouble(symbolTable, "@infinity", LQX::Intrinsics::get_infinity());
    }

#if !HAVE_DRAND48
    /* Windows doesn't have this... So stolen from Parasol drand48.c */

/*
 *	drand48, etc. pseudo-random number generator
 *	This implementation assumes unsigned short integers of at least
 *	16 bits, long integers of at least 32 bits, and ignores
 *	overflows on adding or multiplying two unsigned integers.
 *	Two's-complement representation is assumed in a few places.
 *	Some extra masking is done if unsigneds are exactly 16 bits
 *	or longs are exactly 32 bits, but so what?
 *	An assembly-language implementation would run significantly faster.
 */

#define N	16
#define MASK	((unsigned)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)	((unsigned)(x) & MASK)
#define HIGH(x)	LOW((x) >> N)
#define MUL(x, y, z)	{ long l = (long)(x) * (long)(y); (z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)	((long)(x) + (long)(y) > MASK)
#define ADDEQU(x, y, z)	(z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0	0x330E
#define X1	0xABCD
#define X2	0x1234
#define A0	0xE66D
#define A1	0xDEEC
#define A2	0x5
#define C	0xB
#define SET3(x, x0, x1, x2)	((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SETLOW(x, y, n) SET3(x, LOW((y)[n]), LOW((y)[(n)+1]), LOW((y)[(n)+2]))
#define SEED(x0, x1, x2) (SET3(x, x0, x1, x2), SET3(a, A0, A1, A2), c = C)
#define NEST(TYPE, f, F)	TYPE f( unsigned short xsubi[3] ) { \
	register TYPE v; unsigned temp[3]; \
	for (unsigned int i = 0; i < 3; i++) { temp[i] = x[i]; x[i] = LOW(xsubi[i]); }  \
	v = F(); for (unsigned int i = 0; i < 3; i++) { xsubi[i] = x[i]; x[i] = temp[i]; } return v; }
#define HI_BIT	(1L << (2 * N - 1))

    static unsigned x[3] = { X0, X1, X2 }, a[3] = { A0, A1, A2 }, c = C;
    static unsigned short lastx[3];
    static void next();

    static double
    drand48()
    {
	static double two16m = 1.0 / (1L << N);
	next();
	return (two16m * (two16m * (two16m * x[0] + x[1]) + x[2]));
    }

    NEST(double, erand48, drand48);

    static void
    next()
    {
	unsigned p[2], q[2], r[2], carry0, carry1;

	MUL(a[0], x[0], p);
	ADDEQU(p[0], c, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(a[0], x[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(a[1], x[0], r);
	x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] + a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
	x[1] = LOW(p[1] + r[0]);
	x[0] = LOW(p[0]);
    }

    static void
    srand48( long seedval )
    {
	SEED(X0, LOW(seedval), HIGH(seedval));
    }

#endif
  
}
