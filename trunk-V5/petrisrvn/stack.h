/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* July 2003.								*/
/************************************************************************/

/*
 * Generic Stack operations.
 *
 * $Id$
 */

#ifndef MY_STACK_H
#define MY_STACK_H

#include <cstdlib>

template <class Type> class my_stack_t
{
public:
    my_stack_t()
	: _capacity(10), _depth(0)
	{
	    _stack = static_cast<Type *>(calloc( _capacity, sizeof( Type ) ));
    
	}
    ~my_stack_t()
	{
	    free( _stack );
	}

    bool empty() const 
	{	
	    return _depth == 0;
	}

    unsigned int depth() const
	{
	    return _depth;
	}

    void push( Type item )
	{
	    if ( _depth >= _capacity ) {
		_capacity += 10;
		_stack = static_cast<Type *>(realloc( static_cast<void *>(_stack), sizeof( Type ) * _capacity ));
	    }
	    _stack[_depth] = item;
	    _depth += 1;
	}

    Type pop()
	{
	    _depth -= 1;
	    return _stack[_depth];
	}

    Type top()
	{
	    return _stack[_depth-1];
	}

    Type& operator[]( const unsigned int ix ) 
	{
	    return _stack[ix];
	}

    int find( const Type& item )
	{
	    for ( int i = _depth-1; i >= 0; --i ) {
		if ( _stack[i] == item ) {
		    return i;
		}
	    }
	    return -1;
	}


private:
    my_stack_t( const my_stack_t& );
    my_stack_t& operator=( const my_stack_t& );

    Type * _stack;
    unsigned _capacity;
    unsigned _depth;
};


#endif
