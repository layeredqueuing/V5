/* stack.cc	-- Greg Franks Wed Sep 23 1998
 * $Id: stack.cc 11963 2014-04-10 14:36:42Z greg $
 */

#include "stack.h"

/*
 * Copy stack.
 */

template <class Type> Stack<Type>&
Stack<Type>::operator=(const Stack<Type> &aStack )
{
    if ( this == &aStack ) return *this;

    const unsigned newSize = aStack.size();
    stack   = new Type [newSize+1];
    maxSize = newSize;
    myDepth = aStack.myDepth;

    copy( aStack.stack );
    return *this;
}


/*
 * Copy stack.
 */

template <class Type> 
Stack<Type>::Stack( const Stack<Type> &aStack )
{
    const unsigned newSize = aStack.size();
    stack   = new Type [newSize+1];
    maxSize = newSize;
    myDepth = aStack.myDepth;

    copy( aStack.stack );
}


/*
 * Delete stack.
 */

template <class Type>
Stack<Type>::~Stack()
{
    maxSize = 0;
    myDepth = 0;
    if ( stack ) {
	delete [] stack;
    }
}


/*
 * Copy array into receiver.  Array must be big enough.
 */

template <class Type> void
Stack<Type>::copy( const Type * array  )
{
    if ( !array ) return;
	
    /* Copy contents */
	
    for ( unsigned i = 0; i < myDepth; ++i ) {
	stack[i] = array[i];
    }
}
		


/*
 * Compare stack.
 */

template <class Type> int
Stack<Type>::operator==(const Stack<Type> &aStack ) const
{
    if ( this == &aStack ) return 1;
    if ( size() != aStack.size() ) return 0;

    for ( unsigned i = 0; i < size(); ++i ) {
	if ( stack[i] != aStack.stack[i] ) return 0;
    }
    return 1;
}


/*
 * Compare stack.
 */

template <class Type> int
Stack<Type>::contains(const Stack<Type> &aStack ) const
{
    if ( this == &aStack ) return 1;
    const unsigned n = aStack.size();
    if ( size() < n ) return 0;

    for ( unsigned i = 0; i < n; ++i ) {
	if ( stack[i] != aStack.stack[i] ) return 0;
    }
    return 1;
}


/*
 * Set the stack size to newSize.  If it is shrinking, delete
 * old elements.
 */

template <class Type> void
Stack<Type>::size( const unsigned newSize )
{
    assert( myDepth <= maxSize );
	
    Type * oldStack = stack;

    stack   = new Type [newSize+1];
    maxSize = newSize;
    if ( newSize < myDepth ) {
	myDepth = newSize;
    }

    copy( oldStack );

    if ( oldStack ) {
	delete [] oldStack;
    }
}


/*
 * Add an element.
 */

template <class Type> Stack<Type>&
Stack<Type>::push( const Type anItem )
{
    if ( myDepth == maxSize ) {
	size( maxSize + 10 );
    }

    stack[myDepth] = anItem;
    myDepth += 1;
    return *this;
}


/*
 * Remove an item.
 */

template <class Type> Type
Stack<Type>::pop()
{
    assert(myDepth > 0);
    myDepth -= 1;

    return stack[myDepth];
}


/*
 * Swap top of stack with anItem.
 */

template <class Type> Type
Stack<Type>::swap( const Type anItem )
{
    assert( myDepth > 0 );
    Type temp = stack[myDepth-1];
    stack[myDepth-1] = anItem;
    return temp;
}


/*
 * Swap top of stack with top of stack -1;
 */

template <class Type> Stack<Type>&
Stack<Type>::swap()
{
    assert( myDepth > 1 );
    Type temp = stack[myDepth-2];
    stack[myDepth-2] = stack[myDepth-1];
    stack[myDepth-1] = temp;
    return *this;
}


/*
 * Locate an item on the stack -- return it's index +1 if found,
 * and 0 otherwise.
 */

template <class Type> unsigned
Stack<Type>::find( const Type& item ) const
{
    for ( unsigned j = myDepth; j > 0; --j ) {
	if ( stack[j-1] == item ) return j;
    }
    return 0;
}


/*
 * Grow the stack.  Items are cloned.
 */

template <class Type> void
Stack<Type>::grow( const unsigned n, const Type item )
{
    if ( myDepth + n >= maxSize ) {
	size( myDepth + n + 10 );
    }

    for ( unsigned i = 0; i < n; ++i ) {
	stack[myDepth+i] = item;
    }
    myDepth += n;
}



/*
 * Grow the stack.  Items are cloned.
 */

template <class Type> void
Stack<Type>::shrink( const unsigned n )
{
    assert ( n <= myDepth );

    myDepth -= n;
}
