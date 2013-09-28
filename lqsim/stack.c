/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* July 2003								*/
/************************************************************************/

/*
 * Input output processing.
 *
 * $HeadURL$
 *
 * $Id$
 */

#include "stack.h"
#include "stdlib.h"
#include <assert.h>

void
push( para_stack_t * a_stack, void * item )
{
    if ( a_stack->depth >= a_stack->capacity ) {
	a_stack->capacity += 10;
	a_stack->stack = realloc( a_stack->stack, sizeof( void * ) * a_stack->capacity );
    }
    a_stack->stack[a_stack->depth] = item;
    a_stack->depth += 1;
}

void *
pop( para_stack_t * a_stack )
{
    assert( a_stack->depth > 0 );
    a_stack->depth -= 1;
    return a_stack->stack[a_stack->depth];
}

void *
top( para_stack_t * a_stack )
{
    return a_stack->stack[a_stack->depth-1];
}


void
stack_init( para_stack_t * a_stack )
{
    a_stack->capacity = 10;
    a_stack->depth = 0;
    a_stack->stack = calloc( a_stack->capacity, sizeof( void *) );
    
}

void
stack_delete( para_stack_t * a_stack )
{
    free( a_stack->stack );
}

int
stack_find( para_stack_t * a_stack, void * item )
{
    int i;
    for ( i = a_stack->depth-1; i >= 0; --i ) {
	if ( a_stack->stack[i] == item ) {
	    return i;
	}
    }
    return -1;
}
