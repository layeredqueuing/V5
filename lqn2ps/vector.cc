/* -*- c++ -*-
 * $Id$
 *
 * Vectors.  Used for scalar types.  See Cltn for other types.  Range
 * checked.  Initialize.  Range from 1..n.
 *
 * Vector2:  Used for non-pointer COMPLEX types.  There is a subtle
 * difference in the [] operator!  Use Cltn/Vector for pointers and
 * scalars respectively.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#include "vector.h"
#include <assert.h>

template <class Type> ostream&
operator<<( ostream& output, const Vector<Type>& self )
{
    return self.print( output );
}



/*
 * Assign iA to receiver.
 */

template <class Type> Vector<Type>&
Vector<Type>::operator=( const Vector<Type> &iA )
{
    if ( this == &iA ) return *this;

    clear();

    init( iA.ia, iA.sz );
    return *this;
}



/*
 * Initialize array to initializer.
 */

template <class Type> Vector<Type>&
Vector<Type>::operator=( const Type initializer )
{
    const unsigned n = size();
    for ( unsigned i = 1; i <= n; ++i ) {
	ia[i] = initializer;
    }

    return *this;
}



/*
 * Check vectors for equality.  Really only needed to keep RS6000
 * compiler happy.  Also note, the for loop uses `==' instead
 * of the more obvious `!=' because use of the latter would
 * mean defining the != operator in all the classes that use cltns.
 */

template <class Type> int
Vector<Type>::operator==( const Vector<Type> &iA ) const
{
    if ( iA.sz != sz ) return 0;	/* Quick check */

    unsigned ix;
    for ( ix = 1; ix <= sz && ia[ix] == iA[ix]; ++ix );

    return ix > sz;
}



/*
 * Multiply all elements by multiplier.
 */

template <class Type> Vector<Type>
Vector<Type>::operator*( const Type multiplier ) const
{
    const unsigned n = size();
    Vector<Type> product( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	product.ia[i] = multiplier * ia[i];
    }
    return product;
}



/*
 * Add vectors.
 */

template <class Type> Vector<Type>
Vector<Type>::operator+( const Vector<Type>& addend ) const
{
    assert( addend.size() == size() );
    const unsigned n = size();
    Vector<Type> sum( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	sum.ia[i] = ia[i] + addend.ia[i];
    }
    return sum;
}



/*
 * Subtract vectors.  
 */
 
template <class Type> Vector<Type>
Vector<Type>::operator-( const Vector<Type>& subtrahend ) const
{
    assert( subtrahend.size() == size() );
    const unsigned n = size();
    Vector<Type> difference( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	difference.ia[i] = ia[i] - subtrahend.ia[i];
    }
    return difference;
}



/*
 * Square elements.
 */

template <class Type> Vector<Type>
Vector<Type>::square() const
{
    const unsigned n = size();
    Vector<Type> square( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	square.ia[i] = ia[i] * ia[i];
    }
    return square;
}



/*
 * Add vector to receiver.
 */

template <class Type> Vector<Type>&
Vector<Type>::operator+=( const Vector<Type>& addend )
{
    assert( size() == addend.size() );
    for ( unsigned ix = 1; ix <= addend.sz; ++ix ) {
	ia[ix] += addend.ia[ix];
    }
    return *this;
}



/*
 * Subtract vector from receiver.
 */

template <class Type> Vector<Type>&
Vector<Type>::operator-=( const Vector<Type>& subtrahend )
{
    assert( size() == subtrahend.size() );
    for ( unsigned ix = 1; ix <= subtrahend.sz; ++ix ) {
	ia[ix] -= subtrahend.ia[ix];
    }
    return *this;
}



/*
 * Scale vector.
 */

template <class Type> Vector<Type>&
Vector<Type>::operator*=( const Type multiplier )
{
    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	ia[ix] *= multiplier;
    }
    return *this;
}



/*
 * Scale vector.
 */

template <class Type> Vector<Type>&
Vector<Type>::operator/=( const Type divisor )
{
    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	ia[ix] /= divisor;
    }
    return *this;
}



/*
 * Release storage.
 */

template <class Type>
Vector<Type>::~Vector()
{
    clear();
}



/*
 * Release storage.
 */

template <class Type> void
Vector<Type>::clear()
{
    if ( ia ) {
	ia += 1;		/* Fix offset before deletion.	*/
	delete [] ia;
    }
    ia = 0;
    sz = 0;
}



/*
 * Set receiver to vector.
 */

template <class Type> void
Vector<Type>::init( const Type *vector, unsigned size )
{
    ia = new Type[sz = size];
    assert( ia != 0 );
    ia -= 1;			/* Offset for 1..n addressing	*/

    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	ia[ix] = vector[ix];
    }
}



/*
 * Grow vector.  New elements are set to init.
 */

template <class Type> void
Vector<Type>::grow( const unsigned amt, const Type init )
{
    if ( amt == 0 ) return;		/* No operation.		*/
	
    Type *oldia = ia;
    unsigned oldSize = sz;
    unsigned newSize = oldSize + amt;

    ia = new Type[sz = newSize];
    assert ( ia != 0 );
    ia -= 1;			/* Offset to allow 1..n index	*/

    unsigned ix;
    for ( ix = 1; ix <= oldSize; ++ix ) {
	ia[ix] = oldia[ix];
    }
    for ( ; ix <= newSize; ++ix ) {	/* Zero remainder.		*/
	ia[ix] = init;
    }

    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
}


/*
 * Return sum of terms.
 */

template <class Type> Type
Vector<Type>::sum() const
{
    Type sum = 0;

    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	sum += ia[ix];
    }
    return sum;
}



/*
 * Return min of terms.
 */

template <class Type> Type
Vector<Type>::min() const
{
    assert( sz > 0 );
    Type aMin = ia[1];

    for ( unsigned ix = 2; ix <= sz; ++ix ) {
	if ( ia[ix] < aMin ) {
	    aMin = ia[ix];
	}
    }
    return aMin;
}



/*
 * Return max of terms.
 */

template <class Type> Type
Vector<Type>::max() const
{
    assert( sz > 0 );
    Type aMax = ia[1];

    for ( unsigned ix = 2; ix <= sz; ++ix ) {
	if ( ia[ix] < aMax ) {
	    aMax = ia[ix];
	}
    }
    return aMax;
}



/*
 * Search for the object `elem'.  Return it's index.  If not found,
 * return 0.
 */

template <class Type> unsigned
Vector<Type>::find( const Type elem ) const
{
    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	if ( elem == ia[ix] ) return ix;
    }
    return 0;
}



/*
 * Insert an element at index.
 */

template <class Type> void
Vector<Type>::insert( const unsigned index, const Type value )
{
    Type *oldia = ia;
    unsigned oldSize = sz;
    unsigned newSize = oldSize + 1;

    ia = new Type[sz = newSize];
    assert ( ia != 0 );
    assert ( 0 < index && index <= newSize );
	
    ia -= 1;			/* Offset to allow 1..n index	*/

    unsigned ix;
    for ( ix = 1; ix < index; ++ix ) {	/* Copy unchanged part.		*/
	ia[ix] = oldia[ix];
    }
    ia[index] = value;				/* Add new element.		*/
    for ( ix = newSize; ix > index; --ix ) {	/* copy remainder.		*/
	ia[ix] = oldia[ix-1];
    }

    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
}


template <class Type> Vector<Type> const&
Vector<Type>::swap( const unsigned i, const unsigned j ) const
{
    assert( i && i <= sz && j && j <= sz );
    if ( i != j ) {
	Type x = ia[j];
	ia[j] = ia[i];
	ia[i] = x;
    }
    return *this;
}



template <class Type>
ostream& Vector<Type>::print( ostream& output ) const
{
    output << '(';
    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	output << ia[ix];
	if ( ix != sz ) output << ',';
    }
    output << ')';
    return output;
}

template <class Type> ostream&
operator<<( ostream& output, const Vector2<Type>& self )
{
    return self.print( output );
}

template <class Type>
ostream& Vector2<Type>::print( ostream& output ) const
{
    output << '(';
    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	output << ia[ix];
	if ( ix != sz ) output << ',';
    }
    output << ')';
    return output;
}


template <class Type> Vector2<Type>&
Vector2<Type>::operator=(const Vector2<Type> &iA )
{
    if ( this == &iA ) return *this;

    clear();
    init( iA.ia, iA.sz );
    return *this;
}



/*
 * Release storage.
 */

template <class Type>
Vector2<Type>::~Vector2()
{
    clear();
}



template <class Type> void
Vector2<Type>::clear()
{
    if ( ia ) {
	ia += 1;		/* Fix offset before deletion.	*/
	delete [] ia;
    }
    ia = 0;
    sz = 0;
}



template <class Type> void
Vector2<Type>::init( const Type *vector, unsigned size )
{
    ia = new Type[sz = size];
    assert( ia != 0 );
    ia -= 1;			/* Offset for 1..n addressing	*/

    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	ia[ix] = vector[ix];
    }
}


template <class Type> void
Vector2<Type>::grow( const unsigned amt )
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

    if ( oldia ) {
	oldia += 1;		/* Fix offset before deletion.	*/
	delete [] oldia;
    }
}


template <class Type> void
Vector2<Type>::resize( const unsigned amt )
{
    if ( amt > sz ) {
	grow( amt - sz );
    } else {
	sz = amt;	/* Don't bother freeing excess */
    }
}
