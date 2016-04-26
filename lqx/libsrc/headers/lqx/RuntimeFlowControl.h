/* -*- c++ -*-
 *  RuntimeFlowControl.h
 *  ModLang
 *
 *  Created by Martin Mroz on 02/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __RUNTIME_FLOW_CONTROL_H__
#define __RUNTIME_FLOW_CONTROL_H__

#include "SymbolTable.h"
#include "RuntimeException.h"

/*
 * PREAMBLE: Please remember that using Exceptions as a Flow Control mechanism is NEVER
 * the right thing to do, not even now. Nope, not even now. The problem is that we are backed
 * into a corner somewhat. Since the idea was to carry out the execution of LQX code before
 * even generating an intermediate representation, there is no other reasonable approach.
 * Since there is no requirement for this to be exceptionally fast and we have an existing
 * framework to fit into this probably needs to happen. SPECIAL care will be taken to
 * ensure that leaks will not occur.
 *
 * For the future: Adding some sort of Virtual Machine or IR code is the best thing to do
 * and will totally eliminate the need for this ugly, ugly hack.
 */

namespace LQX {
	
    class ReturnValue : public RuntimeException {
    public:
	ReturnValue(SymbolAutoRef value) throw() : RuntimeException("ReturnValue"), _value(value) {}
	virtual ~ReturnValue() throw() {}
	SymbolAutoRef getValue() { return _value; }
    private:
	SymbolAutoRef _value;
    };
	
    class BreakException : public RuntimeException {
    public:
	BreakException() : RuntimeException("Break outside of loop context.") {}
	virtual ~BreakException() throw() {}
    };
  
}

#endif /* __RUNTIME_FLOW_CONTROL_H__ */
