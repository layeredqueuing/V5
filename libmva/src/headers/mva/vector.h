/* -*- c++ -*-
 * $Id: vector.h 17487 2024-11-19 15:02:37Z greg $
 *
 * Vector.  Range checked from 1..n (and not from 0..n-1).
 * VectorMath.  Adds the operators +, -, *, /, square, sum (so only use on numbers).
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * July 2007
 * June 2022
 *
 * ------------------------------------------------------------------------
 */

#ifndef	LQNS_VECTOR_H
#define	LQNS_VECTOR_H

#include <cassert>
#include <iostream>
#include <iterator>

const int VectorSize = 0;

template <typename Type>
class Vector {
public:
    typedef Type * iterator;
    typedef const Type * const_iterator;
    typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;
    typedef std::reverse_iterator<iterator>		 reverse_iterator;
    iterator begin() { return &ia[1]; }			/* ia is offset */
    const_iterator begin() const { return &ia[1]; }
    iterator end() { return &ia[sz+1]; }
    const_iterator end() const { return &ia[sz+1]; }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    
public: 
    explicit Vector( size_t size=0 ) : ia(nullptr), sz(0), mx(0) { resize( size ); }
    Vector( const Type *ar, size_t sz ) { init(ar,sz); }
    Vector( const Vector<Type> &iA ) { init( iA.ia, iA.sz ); }
    virtual ~Vector() { clear(); }
    void clear()
	{
	    if ( ia ) {
		ia += 1;		/* Fix offset before deletion.	*/
		delete [] ia;
	    }
	    ia = nullptr;
	    sz = 0;
	    mx = 0;
	}

    bool operator==( const Vector<Type>& arg ) const
	{
	    const size_t n = size();
	    for ( size_t i = 1; i <= n; ++i ) {
		if ( !(ia[i] == arg.ia[i] ) ) return false;	/* Just require == operator, and not != */
	    }
	    return true;
	}
    Vector<Type>& operator=( const Vector<Type>& iA )
	{
	    if ( this == &iA ) return *this;
	    clear();
	    init( iA.ia, iA.sz );
	    return *this;
	}	    

    Vector<Type>& operator=( const Type& arg )
	{
	    const size_t n = size();
	    for ( size_t i = 1; i <= n; ++i ) {
		ia[i] = arg;
	    }
	    return *this;
	}

    const_iterator find( const Type& elem ) const	/* temp */
	{
	    for ( size_t ix = 1; ix <= sz; ++ix ) {
		if ( elem == ia[ix] ) return &ia[ix];
	    }
	    return &ia[sz+1];
	}

    bool empty() const { return sz == 0; }
    size_t size() const { return sz; }
    size_t max_size() const { return mx; }
    void resize( size_t amt, const Type val = Type() )
	{
	    if ( amt > size() ) {
		grow( amt - size(), val );
	    } else if ( size() > amt ) {
		shrink( size() > amt );
	    }
	}
    void insert( const size_t index, const Type& value )
	{
	    Type *oldia = ia;
	    size_t oldSize = sz;
	    size_t newSize = oldSize + 1;
	    size_t ix;

	    sz = newSize;
	    if ( sz > mx ) {
		mx = sz * 2;
		ia = new Type[mx];
		assert ( ia != nullptr );
	
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
    
	    if ( oldia != nullptr && oldia != ia ) {
		oldia += 1;				/* Fix offset before deletion.	*/
		delete [] oldia;
	    }
	}
    void push_back( const Type& arg ) { insert( size()+1, arg ); }
    iterator erase( const_iterator pos )
	{
	    Vector<Type>::iterator dst = const_cast<Vector<Type>::iterator>(pos);
	    for ( Vector<Type>::iterator src = dst + 1; src != end(); ++src, ++dst ) {
		*dst = *src;
	    }
	    shrink( 1 );
	    return const_cast<Vector<Type>::iterator>(pos) + 1;
	}
    
    std::ostream& print( std::ostream& output = std::cout ) const
	{
	    output << '(';
	    for ( size_t ix = 1; ix <= sz; ++ix ) {
		output << ia[ix];
		if ( ix != sz ) output << ',';
	    }
	    output << ')';
	    return output;
	}
	
    Type& first() { assert( sz > 0 ); return ia[1]; }
    Type& first() const { assert( sz > 0 ); return ia[1]; }
    Type& last() { assert( sz > 0 ); return ia[sz]; }
    Type& last() const { assert( sz > 0 ); return ia[sz]; }
    Type& operator[](const size_t ix) { assert( ix && ix <= sz ); return ia[ix]; }
    Type& operator[](const size_t ix) const { assert( ix && ix <= sz ); return ia[ix]; }

protected:
    void grow( size_t amt, const Type val )
	{
	    if ( amt == 0 ) return;		/* No operation.		*/
	
	    Type *oldia = ia;
	    const size_t oldSize = sz;
	    sz = oldSize + amt;
	    const size_t minSize = std::min( oldSize, sz );

	    if ( sz > mx ) {
		mx = sz * 2;
		ia = new Type[mx];
		assert ( ia != nullptr );
		ia -= 1;			/* Offset to allow 1..n index	*/

		for ( size_t ix = 1; ix <= minSize; ++ix ) {
		    ia[ix] = oldia[ix];		/* Copy to new array.		*/
		}
		for ( size_t ix = minSize + 1; ix <= mx; ++ix ) {
		    ia[ix] = val;		/* Clear everything afterwards */
		}
	    }

	    if ( oldia != nullptr && oldia != ia ) {
		oldia += 1;			/* Fix offset before deletion.	*/
		delete [] oldia;
	    }
	}
    void shrink( size_t amt )
	{
	    if ( amt == 0 ) return;		/* No operation.		*/

	    assert( amt <= sz );

	    Type *oldia = ia;
	    const size_t oldSize = sz;
	    sz = oldSize - amt;

	    if ( sz ) {
		mx = sz;
		ia = new Type[sz];
		assert ( ia != nullptr );
		ia -= 1;			/* Offset to allow 1..n index	*/

		for ( size_t ix = 1; ix <= sz; ++ix ) {
		    ia[ix] = oldia[ix];
		}
	    } else {
		ia = nullptr;
	    }

	    if ( oldia != nullptr ) {
		oldia += 1;		/* Fix offset before deletion.	*/
		delete [] oldia;
	    }
	}
    
private:
    void init( const Type* vector, const size_t size )
	{
	    sz = size;
	    mx = size;
	    ia = new Type[mx];
	    assert( ia != nullptr );
	    ia -= 1;			/* Offset for 1..n addressing	*/
	    for ( size_t ix = 1; ix <= sz; ++ix ) {
		ia[ix] = vector[ix];
	    }
	}

protected:
    Type *ia;

private:
    size_t sz;
    size_t mx;
};



template <typename Type>
class VectorMath : public Vector<Type> {
	
public:
    explicit VectorMath( size_t size=0, const Type init=Type() ) : Vector<Type>() { this->grow( size, init ); }
    VectorMath( const Type *ar, size_t sz ) : Vector<Type>( ar, sz ) {}
    VectorMath( const VectorMath<Type> &iA ) : Vector<Type>( iA ) {}

    VectorMath<Type>& operator=( const Vector<Type>& arg ) { Vector<Type>::operator=( arg ); return *this; }
    VectorMath<Type>& operator=( const Type& arg ) { Vector<Type>::operator=( arg ); return *this; }
    bool operator==( const Vector<Type>& arg ) const { return Vector<Type>::operator==( arg ); }

    VectorMath<Type> operator+( const VectorMath<Type>& addend ) const
	{
	    assert( addend.size() == Vector<Type>::size() );
	    const size_t n = Vector<Type>::size();
	    VectorMath<Type> sum( n );

	    for ( size_t i = 1; i <= n; ++i ) {
		sum.ia[i] = Vector<Type>::ia[i] + addend.ia[i];
	    }
	    return sum;
	}
    VectorMath<Type> operator-( const VectorMath<Type>& subtrahend ) const
	{
	    assert( subtrahend.size() == Vector<Type>::size() );
	    const size_t n = Vector<Type>::size();
	    VectorMath<Type> difference( n );

	    for ( size_t i = 1; i <= n; ++i ) {
		difference.ia[i] = Vector<Type>::ia[i] - subtrahend.ia[i];
	    }
	    return difference;
	}
    VectorMath<Type> operator*( const Type multiplier ) const
	{
	    const size_t n = Vector<Type>::size();
	    VectorMath<Type> product( n );

	    for ( size_t i = 1; i <= n; ++i ) {
		product.ia[i] = multiplier * Vector<Type>::ia[i];
	    }
	    return product;
	}
    VectorMath<Type> square() const
	{
	    const size_t n = Vector<Type>::size();
	    VectorMath<Type> square( n );

	    for ( size_t i = 1; i <= n; ++i ) {
		square.ia[i] = Vector<Type>::ia[i] * Vector<Type>::ia[i];
	    }
	    return square;
	}

    VectorMath<Type>& operator+=( const VectorMath<Type>& addend )
	{
	    assert( Vector<Type>::size() == addend.size() );
	    const size_t n = Vector<Type>::size();
	    for ( size_t ix = 1; ix <= n; ++ix ) {
		Vector<Type>::ia[ix] += addend.ia[ix];
	    }
	    return *this;
	}
    VectorMath<Type>& operator-=( const VectorMath<Type>& subtrahend )
	{
	    assert( Vector<Type>::size() == subtrahend.size() );
	    const size_t n = Vector<Type>::size();
	    for ( size_t ix = 1; ix <= n; ++ix ) {
		Vector<Type>::ia[ix] -= subtrahend.ia[ix];
	    }
	    return *this;
	}
    VectorMath<Type>& operator*=( const Type multiplier )
	{
	    const size_t n = Vector<Type>::size();
	    for ( size_t ix = 1; ix <= n; ++ix ) {
		Vector<Type>::ia[ix] *= multiplier;
	    }
	    return *this;
	}
    VectorMath<Type>& operator/=( const Type divisor )
	{
	    const size_t n = Vector<Type>::size();
	    for ( size_t ix = 1; ix <= n; ++ix ) {
		Vector<Type>::ia[ix] /= divisor;
	    }
	    return *this;
	}

    Type sum() const
	{
	    Type sum = 0;

	    const size_t n = Vector<Type>::size();
	    for ( size_t ix = 1; ix <= n; ++ix ) {
		sum += Vector<Type>::ia[ix];
	    }
	    return sum;
	}
};

template <typename Type> inline std::ostream& operator << ( std::ostream& output, const Vector<Type>& self ) { return self.print( output ); }
template <typename Type> inline std::ostream& operator << ( std::ostream& output, const VectorMath<Type>& self ) { return self.print( output ); }
#endif
