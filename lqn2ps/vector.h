/* -*- c++ -*-
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
 *
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#ifndef	LQN2PS_VECTOR_H
#define	LQN2PS_VECTOR_H

#include "lqn2ps.h"
#include <assert.h>

template <class Type> class Vector;
template <class Type> class VectorIterator;
template <class Type> ostream& operator << ( ostream&, const Vector<Type>& );

const int VectorSize = 0;

template <class Type>
class Vector {
	
public:
    Vector() : sz(0), ia(0) { grow( 0 ); }
    Vector( unsigned size, const Type init=0 ) : sz(0), ia(0) { grow( size, init ); }
    Vector( const Type *ar, unsigned sz ) { init(ar,sz); }
    Vector( const Vector<Type> &iA ) { init( iA.ia, iA.sz ); }
    virtual ~Vector();
    void clear();

    Vector<Type>& operator=( const Vector<Type>& );
    Vector<Type>& operator=( const Type );

    int operator==( const Vector<Type>& item ) const;

    Vector<Type> operator+( const Vector<Type>& ) const;
    Vector<Type> operator-( const Vector<Type>& ) const;
    Vector<Type> operator*( const Type ) const;
    Vector<Type> square() const;

    Vector<Type>& operator+=( const Vector<Type>& );
    Vector<Type>& operator-=( const Vector<Type>& );
    Vector<Type>& operator*=( const Type );
    Vector<Type>& operator/=( const Type );

    Type sum() const;
    Type min() const;
    Type max() const;
    unsigned find( const Type ) const;
	
    unsigned size() const { return sz; }
    void grow( const unsigned, const Type init=0 );
    void insert( const unsigned, const Type );
    Vector<Type> const & swap( const unsigned, const unsigned ) const;

    ostream& print( ostream& = cout ) const;
	
    Type& operator[](const unsigned ix) { assert( ix && ix <= sz ); return ia[ix]; }
    Type operator[](const unsigned ix) const { assert( ix && ix <= sz ); return ia[ix]; }

private:
    unsigned sz;
    Type *ia;

    void init( const Type*, unsigned );
};



template <class Type>
class Vector2 {
	
public:
    Vector2() : sz(0), ia(0) { grow( 0 ); }
    Vector2( unsigned size ) : sz(0), ia(0) { grow( size ); }
    Vector2( const Type *ar, unsigned sz ) { init(ar,sz); }
    Vector2( const Vector2<Type> &iA ) { init( iA.ia, iA.sz ); }
    virtual ~Vector2();
    void clear();
    void resize( unsigned );

    Vector2<Type>& operator=( const Vector2<Type>& );

    unsigned size() const { return sz; }
    void grow( const unsigned );
    virtual ostream& print( ostream& = cout ) const;
	
    Type& operator[](const unsigned ix) { assert( ix && ix <= sz ); return ia[ix]; }
    Type& operator[](const unsigned ix) const { assert( ix && ix <= sz ); return ia[ix]; }

private:
    unsigned sz;
    Type *ia;

    void init( const Type*, unsigned );
};
#endif
