/*  -*- c++ -*-
 * $Id: cltn.cc 11963 2014-04-10 14:36:42Z greg $
 *
 * Collections.  Range checked.  Some set functions.  Start from 1.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#include "cltn.h"


template <class Type> ostream&
operator<<( ostream& os, const Cltn<Type>& self )
{
    return self.print(os);
}

/*----------------------------------------------------------------------*/

template <class Type> Cltn<Type>&
Cltn<Type>::operator=(const Cltn<Type> &iA )
{
    if ( this == &iA ) return *this;

    if ( ia ) {
	ia += 1;		/* Correct for offset.		*/
	delete [] ia;
    }

    init( iA.ia, iA.sz );
    return *this;
}



/*
 * Release storage.
 */

template <class Type>
Cltn<Type>::~Cltn()
{
    if ( ia ) {
	ia += 1;		/* Fix offset before deletion.	*/
	delete [] ia;
    }
    sz = 0;
}



/*
 * Check cltns for equality.  Really only needed to keep RS6000
 * compiler happy.  Also note, the for loop uses `==' instead
 * of the more obvious `!=' because use of the latter would
 * mean defining the != operator in all the classes that use cltns.
 */

template <class Type> int
Cltn<Type>::operator==( const Cltn<Type> &iA ) const
{
    if ( iA.sz != sz ) return 0;	/* Quick check */

    unsigned ix;
    for ( ix = 1; ix <= sz && ia[ix] == iA[ix]; ++ix );

    return ix > sz;
}



/*
 * Concatenate an element to the Cltn.  Return the receiver.
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator<<(const Type &elem )
{
    grow( 1 );

    ia[sz] = elem;
    return *this;
}



/*
 * Concatenate the receiver with `Cltn'.  Return the receiver.
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator<<( const Cltn<Type> &cltn )
{
    unsigned oldSize = sz;

    grow( cltn.sz );
	
    for ( unsigned ix = 1; ix <= cltn.sz; ++ix ) {
	ia[ix+oldSize] = cltn[ix];
    }
    return *this;
}



/*
 * Prepend an element to the vector.  Return the receiver.
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator>>( const Type &elem )
{
    grow( 1 );

    /* Shift right */
	
    for ( unsigned ix = sz; ix > 1; --ix ) {
	ia[ix] = ia[ix-1];
    }
    ia[1] = elem;
    return *this;
}



/*
 * Add all elements in the receiver that are found in Cltn.
 * (Set Union)
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator+=( const Cltn<Type> &addend )
{
    int * tags = new int[addend.size()+1];
    unsigned count = sz;
    unsigned ix;			/* Index into Cltn. */
	
    /* Count up unique elements */

    for ( ix = 1; ix <= addend.sz; ++ix ) {
	tags[ix] = find( addend[ix] );
	if ( !tags[ix] ) {
	    count += 1;
	}
    }
	
    /* reallocate Cltn */
	
    Type * oldia = ia;
    unsigned oldsz = sz;
    ia = new Type[sz = count];
    ia -= 1;

    count = 0;
    for ( ix = 1; ix <= oldsz; ++ix ) {
	count += 1;
	ia[count] = oldia[ix];
    }
    for ( ix = 1; ix <= addend.size(); ++ix ) {
	if ( !tags[ix] ) {
	    count += 1;
	    ia[count] = addend[ix];
	}
    }
	
    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
    delete [] tags;
    return *this;
}



/*
 * Add item to receiver if it is not alread there.
 * Return the receiver.
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator+=( const Type& item )
{
    findOrAdd( item );
    return *this;
}



/*
 * Set Union.  A new Cltn is returned.
 */

template <class Type> Cltn<Type>
Cltn<Type>::operator+( const Cltn<Type> &addend ) const
{
    int * tags = new int[addend.size()+1];
    unsigned count = sz;
    unsigned ix;			/* Index into Cltn. */
	
    /* Count up unique elements */

    for ( ix = 1; ix <= addend.size(); ++ix ) {
	tags[ix] = find( addend[ix] );
	if ( !tags[ix] ) {
	    count += 1;
	}
    }

    /* Allocate space and copy non-tagged items. */
	
    Cltn<Type> aUnion(count);
    count = 0;
    for ( ix = 1; ix <= sz; ++ix ) {
	count += 1;
	aUnion.ia[count] = ia[ix];
    }
    for ( ix = 1; ix <= addend.size(); ++ix ) {
	if ( !tags[ix] ) {
	    count += 1;
	    aUnion.ia[count] = ia[ix];
	}
    }
    delete [] tags;
    return aUnion;
}



/*
 * Remove all elements in the receiver that are found in Cltn.
 * (Set difference)
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator-=( const Cltn<Type> &subtrahend )
{
    int * tags = new int[sz+1];
    unsigned count = 0;
    unsigned ix;			/* Index into Cltn. */
	
    /* Count up unique elements */

    for ( ix = 1; ix <= sz; ++ix ) {
	tags[ix] = subtrahend.find( ia[ix] );
	if ( !tags[ix] ) {
	    count += 1;
	}
    }

	
    /* reallocate Cltn */
	
    Type * oldia = ia;
    unsigned oldsz = sz;
    ia = new Type[sz = count];
    ia -= 1;

    count = 0;
    for ( ix = 1; ix <= oldsz; ++ix ) {
	if ( !tags[ix] ) {
	    count += 1;
	    ia[count] = oldia[ix];
	}
    }

    /* Clear out slop */

    for ( ix = count + 1; ix <= sz; ++ix ) {
	ia[ix] = 0;
    }
	
    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
    delete [] tags;
    return *this;
}


/*
 * Remove item from the receiver.  All occurances are deleted.
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator-=( const Type& item )
{
    int * tags = new int[sz+1];
    unsigned count = 0;
    unsigned ix;			/* Index into Cltn. */
	
    /* Count up unique elements */

    for ( ix = 1; ix <= sz; ++ix ) {
	tags[ix] = (ia[ix] == item);
	if ( !tags[ix] ) {
	    count += 1;
	}
    }

	
    /* reallocate Cltn */
	
    Type * oldia = ia;
    unsigned oldsz = sz;
    ia = new Type[sz = count];
    ia -= 1;

    count = 0;
    for ( ix = 1; ix <= oldsz; ++ix ) {
	if ( !tags[ix] ) {
	    count += 1;
	    ia[count] = oldia[ix];
	}
    }
	
    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
    delete [] tags;
    return *this;
}


/*
 * Set difference.  A new Cltn is returned.
 */

template <class Type> Cltn<Type>
Cltn<Type>::operator-( const Cltn<Type> &subtrahend ) const
{
    int * tags = new int[sz+1];
    unsigned count = 0;
    unsigned ix;			/* Index into Cltn. */
	
    /* Count up unique elements */

    for ( ix = 1; ix <= sz; ++ix ) {
	tags[ix] = subtrahend.find( ia[ix] );
	if ( !tags[ix] ) {
	    count += 1;
	}
    }

    /* Allocate space and copy non-tagged items. */
	
    Cltn<Type> difference(count);
    count = 0;
    for ( ix = 1; ix <= sz; ++ix ) {
	if ( !tags[ix] ) {
	    count += 1;
	    difference.ia[count] = ia[ix];
	}
    }
    delete [] tags;
    return difference;
}



/*
 * Remove all elements in the receiver that are not found in Cltn.
 * (Set intersection)
 */

template <class Type> Cltn<Type>&
Cltn<Type>::operator&=( const Cltn<Type> &set2 )
{
    int * tags = new int[sz+1];
    unsigned count = 0;
    unsigned ix;			/* Index into Cltn. */
	
    /* Count up unique elements */

    for ( ix = 1; ix <= sz; ++ix ) {
	tags[ix] = set2.find( ia[ix] );
	if ( tags[ix] ) {
	    count += 1;
	}
    }

	
    /* reallocate Cltn */
	
    Type * oldia = ia;
    unsigned oldsz = sz;
    ia = new Type[sz = count];
    ia -= 1;

    count = 0;
    for ( ix = 1; ix <= oldsz; ++ix ) {
	if ( tags[ix] ) {
	    count += 1;
	    ia[count] = oldia[ix];
	}
    }
	
    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
    delete [] tags;
    return *this;
}




/*
 * Set intersection.  A new Cltn is returned.
 */

template <class Type> Cltn<Type>
Cltn<Type>::operator&( const Cltn<Type> &subtrahend ) const
{
    int * tags = new int[sz+1];
    unsigned count = 0;
    unsigned ix;			/* Index into Cltn. */
	
    /* Count up unique elements */

    for ( ix = 1; ix <= sz; ++ix ) {
	tags[ix] = subtrahend.find( ia[ix] );
	if ( tags[ix] ) {
	    count += 1;
	}
    }

    /* Allocate space and copy non-tagged items. */
	
    Cltn<Type> difference(count);
    count = 0;
    for ( ix = 1; ix <= sz; ++ix ) {
	if ( tags[ix] ) {
	    count += 1;
	    difference.ia[count] = ia[ix];
	}
    }
    delete [] tags;
    return difference;
}



template <class Type> void
Cltn<Type>::init( const Type *aCltn, unsigned size )
{
    ia = new Type[sz = size];
    assert( ia != 0 );
    ia -= 1;			/* Offset for 1..n addressing	*/

    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	ia[ix] = aCltn[ix];
    }
}


template <class Type> void
Cltn<Type>::grow( const unsigned amt )
{
    if ( amt == 0 ) return;		/* No operation.		*/
	
    Type *oldia = ia;
    unsigned oldSize = sz;
    unsigned newSize = oldSize + amt;

    ia = new Type[sz = newSize];
    assert ( ia != 0 );
    ia -= 1;			/* Offset to allow 1..n index	*/

    for ( unsigned ix = 1; ix <= oldSize; ++ix ) {
	ia[ix] = oldia[ix];
    }
    for ( unsigned ix = oldSize + 1; ix <= newSize; ++ix ) {
	ia[ix] = 0;
    }

    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
}


template <class Type> void
Cltn<Type>::chop( const unsigned amt )
{
    assert( amt <= sz );
    if ( amt == 0 ) return;		/* No operation.		*/

    Type *oldia = ia;
    unsigned oldSize = sz;
    sz = oldSize - amt;

    if ( sz ) {
	ia = new Type[sz];
	assert ( ia != 0 );
	ia -= 1;			/* Offset to allow 1..n index	*/

	for ( unsigned ix = 1; ix <= sz; ++ix ) {
	    ia[ix] = oldia[ix];
	}
    } else {
	ia = 0;
    }

    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
}


/*
 * Delete the contents of the cltn, but don't delete the cltn itself.
 */

template <class Type> void
Cltn<Type>::deleteContents()
{
    deleteContents( size() );
}


/*
 * Delete the contents of the cltn, but don't delete the cltn itself.
 */

template <class Type> void
Cltn<Type>::deleteContents( unsigned int _sz )
{
    assert( _sz <= sz );
    for ( unsigned ix = 1; ix <= _sz; ++ix ) {
	delete ia[ix];
	ia[ix] = 0;
    }
}


/*
 * Reset all items to zero.
 */

template <class Type> void
Cltn<Type>::clearContents()
{
    unsigned ix;
	
    for ( ix = 1; ix <= sz; ++ix ) {
	ia[ix] = 0;
    }
}



/*
 * Search for the object `elem'.  Return it's index.  If not found,
 * return 0.
 */

template <class Type> unsigned
Cltn<Type>::find( const Type& elem ) const
{
    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	if ( elem == ia[ix] ) return ix;
    }
    return 0;
}



/*
 * Add item to receiver if it is not alread there.
 * Return the index of the item.
 */

template <class Type> unsigned
Cltn<Type>::findOrAdd( const Type& item )
{
    unsigned ix = find( item );

    if ( ix ) {
	return ix;
    } else {
	*this << item;
	return size();
    }
}



template <class Type>
ostream& Cltn<Type>::print( ostream& os ) const
{
    const unsigned lineLength = 12;

    os << '<' << sz << ">[";
    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	if ( ix % lineLength == 1 && ix > 1 ) os << "\n\t";
	os << ia[ix];

	if ( ix % lineLength != 0 && ix != sz ) os << ",";
    }
    os << ']';
    return os;
}

/*----------------------------------------------------------------------*/

/*
 * Check for valid bounds and initialize.
 */

template <class Type> void
Sequence<Type>::check()
{
    myIndex = myMin; 
    assert( 0 < myMin && myMax <= vecPtr.sz );
}


/*
 * Return next element in list (starting from myMin).
 * Return 0 if at end of list and reset.
 */

template <class Type> Type
Sequence<Type>::operator()()
{
    if ( myIndex <= myMax ) {
	return vecPtr.ia[myIndex++];
    } else {
	myIndex = myMin;
	return 0;
    }
}


/*
 * Search for the object `elem'.  Return it's myIndex.  If not found,
 * return 0.
 */

template <class Type> unsigned
Sequence<Type>::find( const Type& elem ) const
{
    for ( unsigned ix = myMin; ix <= myMax; ++ix ) {
	if ( elem == vecPtr.ia[ix] ) return ix + 1 - myMin;
    }
    return 0;
}



/*
 * Return last element from the sequence, if it exists.  
 */

template <class Type> Type
Sequence<Type>::lastItem() const
{
    if ( myIndex - 2 >= myMin ) {
	return vecPtr.ia[myIndex-2];
    } else {
	return 0;
    }
}



/*
 * Return the next item to be returned in the sequence.
 */

template <class Type> Type
Sequence<Type>::nextItem() const
{
    if ( myMin < myIndex && myIndex <= myMax ) {
	return vecPtr.ia[myIndex];
    } else {
	return 0;
    }
}


/*
 * Check for valid bounds and initialize.
 * myMin can be larger than max, in which case this class is a nop.
 */

template <class Type> void
BackwardsSequence<Type>::check()
{
    this->myIndex = this->myMax; 
    assert( 0 < this->myMin && this->myMax <= this->vecPtr.sz );
}


/*
 * Return previous element in list, starting from the end.
 * When we hit the beginning, return 0 and reset.
 */

template <class Type> Type
BackwardsSequence<Type>::operator()()
{
    if ( this->myIndex >= this->myMin ) {
	return this->vecPtr.ia[this->myIndex--];
    } else {
	this->myIndex = this->myMax;
	return 0;
    }
}
