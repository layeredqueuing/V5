/* -*- c++ -*-
 * $HeadURL$
 *
 * Vectors.  Used for scalar types.  See Cltn for other types.  Range checked.
 * Initialize.  Range from 1..n.
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
 * July 2007
 *
 * $Id$
 * ------------------------------------------------------------------------
 */

#ifndef	LQNS_VECTOR_H
#define	LQNS_VECTOR_H

#include "dim.h"
#include <assert.h>

template <class Type> class Vector;
template <class Type> class VectorMath;
template <class Type> class VectorIterator;
template <class Type> ostream& operator << ( ostream&, const Vector<Type>& );
template <class Type> ostream& operator << ( ostream&, const VectorMath<Type>& );

const int VectorSize = 0;

template <class Type>
class Vector {
	
public:
    explicit Vector( unsigned size=0 ) : ia(0), sz(0), mx(0) { grow( size ); }
    Vector( const Type *ar, unsigned sz ) { init(ar,sz); }
    Vector( const Vector<Type> &iA ) { init( iA.ia, iA.sz ); }
    virtual ~Vector();
    void clear();

    bool operator==( const Vector<Type>& arg ) const;
    Vector<Type>& operator=( const Vector<Type>& );
    Vector<Type>& operator=( const Type& );

    unsigned find( const Type ) const;
	
    unsigned size() const { return sz; }
    Vector<Type>& size( const unsigned );
    void grow( const int );
    void insert( const unsigned, const Type& );
    void append( const Type& arg ) { insert( size()+1, arg ); }

    ostream& print( ostream& = cout ) const;
	
    Type& operator[](const unsigned ix) { assert( ix && ix <= sz ); return ia[ix]; }
    Type& operator[](const unsigned ix) const { assert( ix && ix <= sz ); return ia[ix]; }

protected:
    Type *ia;

private:
    unsigned sz;
    unsigned mx;
    void init( const Type*, unsigned );
};



template <class Type>
class VectorMath : public Vector<Type> {
	
public:
    explicit VectorMath<Type>( unsigned size=0, Type init=0 ) : Vector<Type>() { grow( size, init ); }
    VectorMath<Type>( const Type *ar, unsigned sz ) : Vector<Type>( ar, sz ) {}
    VectorMath<Type>( const VectorMath<Type> &iA ) : Vector<Type>( iA ) {}

    void grow( const int, const Type init = 0 );

    VectorMath<Type>& operator=( const Vector<Type>& arg ) { Vector<Type>::operator=( arg ); return *this; }
    VectorMath<Type>& operator=( const Type& arg ) { Vector<Type>::operator=( arg ); return *this; }
    bool operator==( const Vector<Type>& arg ) const { return Vector<Type>::operator==( arg ); }
    VectorMath<Type> operator+( const VectorMath<Type>& ) const;
    VectorMath<Type> operator-( const VectorMath<Type>& ) const;
    VectorMath<Type> operator*( const Type ) const;
    VectorMath<Type> square() const;

    VectorMath<Type>& operator+=( const VectorMath<Type>& );
    VectorMath<Type>& operator-=( const VectorMath<Type>& );
    VectorMath<Type>& operator*=( const Type );
    VectorMath<Type>& operator/=( const Type );

    Type sum() const;
};


#endif
