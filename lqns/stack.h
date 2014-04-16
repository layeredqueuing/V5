/* -*- c++ -*-
 * stack.h	-- Greg Franks
 *
 * $Id$
 */

#ifndef LQNS_STACK_H
#define LQNS_STACK_H

#include <assert.h>

template <class Type>
class Stack {

public:
    Stack() : stack(0), myDepth(0), maxSize(0) {}
    Stack( const unsigned n ) : stack(0), myDepth(0), maxSize(0) { size( n ); }
    virtual ~Stack();
    Stack( const Stack<Type>& );
    Stack<Type>& operator=( const Stack<Type>& );

    int operator==( const Stack<Type>& ) const;
    int contains( const Stack<Type>& ) const;

    unsigned size() const { return myDepth; }

    Stack<Type>& push( const Type );
    Type pop();
    Stack<Type>& swap();
    Type swap( const Type );
    void grow( const unsigned, const Type item = 0 );
    void shrink( const unsigned );

    Type operator()() const { assert( myDepth > 0 ); return stack[myDepth-1]; } 
    Type& operator[](const unsigned ix) { assert( 0 < ix && ix <= myDepth ); return stack[ix-1]; }
    Type operator[](const unsigned ix) const { assert( 0 < ix && ix <= myDepth ); return stack[ix-1]; }
    Type& bottom() { assert( myDepth > 0 ); return stack[0]; }
    Type bottom() const { assert( myDepth > 0 ); return stack[0]; }
    Type& top() { assert( myDepth > 0 ); return stack[myDepth-1]; }
    Type top() const { assert( myDepth > 0 ); return stack[myDepth-1]; }

    unsigned find( const Type& ) const;

private:
    void size( const unsigned );
    void copy( const Type * );
	
private:
    Type * stack;
    unsigned myDepth;
    unsigned maxSize;
};

#endif