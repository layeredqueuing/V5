/* -*- c++ -*-
 * $Id: vector.cc 11963 2014-04-10 14:36:42Z greg $
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
 * March 2008
 * ------------------------------------------------------------------------
 */

#include "vector.h"

template <class Type> ostream&
operator<<( ostream& output, const Vector<Type>& self )
{
    return self.print( output );
}


template <class Type> ostream&
operator<<( ostream& output, const VectorMath<Type>& self )
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
Vector<Type>::operator=( const Type& initializer )
{
    const unsigned n = size();
    for ( unsigned i = 1; i <= n; ++i ) {
	ia[i] = initializer;
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


template <class Type> bool 
Vector<Type>::operator==( const Vector<Type>& arg ) const
{
    const unsigned n = size();
    for ( unsigned i = 1; i <= n; ++i ) {
	if ( !(ia[i] == arg.ia[i] ) ) return false;	/* Just require == operator, and not != */
    }
    return true;
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
    mx = 0;
}



/*
 * Set receiver to vector.
 */

template <class Type> void
Vector<Type>::init( const Type *vector, unsigned size )
{
    sz = size;
    mx = size;
    ia = new Type[mx];
    assert( ia != 0 );
    ia -= 1;			/* Offset for 1..n addressing	*/

    for ( unsigned ix = 1; ix <= sz; ++ix ) {
	ia[ix] = vector[ix];
    }
}



/*
 * Grow vector.  New elements are set to init.
 */

template <class Type> Vector<Type>&
Vector<Type>::size( const unsigned amt )
{
    const int diff = amt - size();
    if ( diff != 0 ) {
	grow( diff );
    }
    return *this;
}



/*
 * Grow vector.  New elements are set to init.
 */

template <class Type> void
Vector<Type>::grow( const int amt )
{
    if ( amt == 0 ) return;		/* No operation.		*/
	
    Type *oldia = ia;
    const unsigned oldSize = sz;
    const unsigned newSize = (int)(oldSize + amt) >= 0 ? oldSize + amt : 0;
    const unsigned minSize = min( oldSize, newSize );

    sz = newSize;
    if ( sz > mx ) {
	mx = sz * 2;
	ia = new Type[mx];
	assert ( ia != 0 );
	ia -= 1;			/* Offset to allow 1..n index	*/

	for ( unsigned ix = 1; ix <= minSize; ++ix ) {
	    ia[ix] = oldia[ix];		/* Copy to new array.		*/
	}
    }

    if ( oldia && oldia != ia ) {
	oldia += 1;			/* Fix offset before deletion.	*/
	delete [] oldia;
    }
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
Vector<Type>::insert( const unsigned index, const Type& value )
{
    Type *oldia = ia;
    unsigned oldSize = sz;
    unsigned newSize = oldSize + 1;
    unsigned ix;

    sz = newSize;
    if ( sz > mx ) {
	mx = sz * 2;
	ia = new Type[mx];
	assert ( ia != 0 );
	
	ia -= 1;				/* Offset to allow 1..n index	*/

	for ( ix = 1; ix < index; ++ix ) {	/* Copy unchanged part.		*/
	    ia[ix] = oldia[ix];
	}
    }

    for ( ix = newSize; ix > index; --ix ) {	/* copy or shift remainder.	*/
	ia[ix] = oldia[ix-1];
    }

    assert ( 0 < index && index <= newSize );
    ia[index] = value;				/* Add new element.		*/
    
    if ( oldia && oldia != ia ) {
	oldia += 1;				/* Fix offset before deletion.	*/
	delete [] oldia;
    }
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

template <class Type> void
VectorMath<Type>::grow( const int amt, const Type init )
{
    const unsigned oldSize = Vector<Type>::size();
    const unsigned newSize = (int)(oldSize + amt) >= 0 ? oldSize + amt : 0;
    const unsigned minSize = min( oldSize, newSize );

    Vector<Type>::grow( amt );

    for ( unsigned ix = minSize + 1; ix <= newSize; ++ix ) {
	Vector<Type>::ia[ix] = init;		/* initialize remainder.	*/
    }
}


/*
 * Multiply all elements by multiplier.
 */

template <class Type> VectorMath<Type>
VectorMath<Type>::operator*( const Type multiplier ) const
{
    const unsigned n = Vector<Type>::size();
    VectorMath<Type> product( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	product.ia[i] = multiplier * Vector<Type>::ia[i];
    }
    return product;
}



/*
 * Add vectors.
 */

template <class Type> VectorMath<Type>
VectorMath<Type>::operator+( const VectorMath<Type>& addend ) const
{
    assert( addend.size() == Vector<Type>::size() );
    const unsigned n = Vector<Type>::size();
    VectorMath<Type> sum( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	sum.ia[i] = Vector<Type>::ia[i] + addend.ia[i];
    }
    return sum;
}



/*
 * Subtract vectors.  
 */
 
template <class Type> VectorMath<Type>
VectorMath<Type>::operator-( const VectorMath<Type>& subtrahend ) const
{
    assert( subtrahend.size() == Vector<Type>::size() );
    const unsigned n = Vector<Type>::size();
    VectorMath<Type> difference( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	difference.ia[i] = Vector<Type>::ia[i] - subtrahend.ia[i];
    }
    return difference;
}



/*
 * Square elements.
 */

template <class Type> VectorMath<Type>
VectorMath<Type>::square() const
{
    const unsigned n = Vector<Type>::size();
    VectorMath<Type> square( n );

    for ( unsigned i = 1; i <= n; ++i ) {
	square.ia[i] = Vector<Type>::ia[i] * Vector<Type>::ia[i];
    }
    return square;
}



/*
 * Add vector to receiver.
 */

template <class Type> VectorMath<Type>&
VectorMath<Type>::operator+=( const VectorMath<Type>& addend )
{
    assert( Vector<Type>::size() == addend.size() );
    const unsigned n = Vector<Type>::size();
    for ( unsigned ix = 1; ix <= n; ++ix ) {
	Vector<Type>::ia[ix] += addend.ia[ix];
    }
    return *this;
}



/*
 * Subtract vector from receiver.
 */

template <class Type> VectorMath<Type>&
VectorMath<Type>::operator-=( const VectorMath<Type>& subtrahend )
{
    assert( Vector<Type>::size() == subtrahend.size() );
    const unsigned n = Vector<Type>::size();
    for ( unsigned ix = 1; ix <= n; ++ix ) {
	Vector<Type>::ia[ix] -= subtrahend.ia[ix];
    }
    return *this;
}



/*
 * Scale vector.
 */

template <class Type> VectorMath<Type>&
VectorMath<Type>::operator*=( const Type multiplier )
{
    const unsigned n = Vector<Type>::size();
    for ( unsigned ix = 1; ix <= n; ++ix ) {
	Vector<Type>::ia[ix] *= multiplier;
    }
    return *this;
}



/*
 * Scale vector.
 */

template <class Type> VectorMath<Type>&
VectorMath<Type>::operator/=( const Type divisor )
{
    const unsigned n = Vector<Type>::size();
    for ( unsigned ix = 1; ix <= n; ++ix ) {
	Vector<Type>::ia[ix] /= divisor;
    }
    return *this;
}


/*
 * Return sum of terms.
 */

template <class Type> Type
VectorMath<Type>::sum() const
{
    Type sum = 0;

    const unsigned n = Vector<Type>::size();
    for ( unsigned ix = 1; ix <= n; ++ix ) {
	sum += Vector<Type>::ia[ix];
    }
    return sum;
}



