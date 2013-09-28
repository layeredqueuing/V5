/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* July 2003.								*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $HeadURL$
 *
 * $Id$
 */

#ifndef PARA_STACK_H
#define PARA_STACK_H

#if	defined(__cplusplus)
extern "C" {
#endif

typedef struct para_stack_t
{
    void ** stack;
    unsigned depth;
    unsigned capacity;
} para_stack_t;


void push( para_stack_t * a_stack, void * item );
void * pop( para_stack_t * a_stack );
void * top( para_stack_t * a_stack );
void stack_init( para_stack_t * a_stack );
void stack_delete( para_stack_t * a_stack );
int stack_find( para_stack_t * a_stack, void * item );
#if	defined(__cplusplus)
}
#endif
#endif
