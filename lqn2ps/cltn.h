/* -*- c++ -*-
 *
 * Cltns.  Range checked.  Some set functions.  Start from 1.
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


#ifndef	CLTN_H
#define	CLTN_H

#include "lqn2ps.h"
#include <assert.h>


template <class Type> class Cltn;
template <class Type> class Sequence;
template <class Type> class BackwardsSequence;
template <class Type> ostream& operator << ( ostream&, const Cltn<Type>& );

const int CltnSize = 0;

typedef int (* compare_func_ptr)( const void *, const void * );

template <class Type>
class Cltn {
    friend class Sequence<Type>;
    friend class BackwardsSequence<Type>;
	
public:
    Cltn() : sz(0), ia(0) { grow( 0 ); }
    Cltn( unsigned size ) : sz(0), ia(0) { grow( size ); }
    Cltn( const Type *ar, unsigned sz ) { init(ar,sz); }
    Cltn( const Cltn<Type> &iA ) { init( iA.ia, iA.sz ); }
    virtual ~Cltn();

    int operator==( const Cltn<Type>& ) const;
    int operator!=( const Cltn<Type>& aCltn ) const { return !( *this == aCltn ); }
	
    Cltn<Type>& operator=( const Cltn<Type>& );
    Cltn<Type>& operator<<( const Type& );			/* Concatenate	*/
    Cltn<Type>& operator<<( const Cltn<Type>& );		/* Concatenate	*/
    Cltn<Type>& operator>>( const Type& );			/* Prepend.	*/
    Cltn<Type>  operator+( const Cltn<Type>& ) const;		/* Union	*/
    Cltn<Type>& operator+=( const Cltn<Type>& );		/* Union	*/
    Cltn<Type>& operator+=( const Type& );			/* Union	*/
    Cltn<Type>  operator-( const Cltn<Type>& ) const;		/* Difference	*/
    Cltn<Type>& operator-=( const Cltn<Type>& );		/* Difference	*/
    Cltn<Type>& operator-=( const Type& );			/* Removal	*/
    Cltn<Type>  operator&( const Cltn<Type>& ) const;		/* Intersection	*/
    Cltn<Type>& operator&=( const Cltn<Type>& );		/* Intersection	*/
	
    unsigned size() const { return sz; }
    Cltn<Type>& grow( const unsigned );
    Cltn<Type>& chop( const unsigned );
    Cltn<Type>& clearContents();
    Cltn<Type>& deleteContents();
	
    ostream& print( ostream& = cout ) const;
	
    unsigned find( const Type & ) const;
    unsigned findOrAdd( const Type& );


    Type& operator[](const unsigned ix) { assert( ix && ix <= sz ); return ia[ix]; }
    Type operator[](const unsigned ix) const { assert( ix && ix <= sz ); return ia[ix]; }
    Type first() const { assert( sz > 0 ); return ia[1]; }
    Type last() const { assert( sz > 0 ); return ia[sz]; }
    Type& first() { assert( sz > 0 ); return ia[1]; }
    Type& last() { assert( sz > 0 ); return ia[sz]; }

    Cltn<Type> const & sort( compare_func_ptr, const unsigned size = ~0 ) const;
    Cltn<Type> const & swap( const unsigned, const unsigned ) const;

private:
    void init( const Type*, unsigned );

protected:
    unsigned sz;
    Type *ia;
};



template <class Type>
class Sequence {

public:
    Sequence( const Cltn<Type>& aVec ) : vecPtr(aVec), myMax(vecPtr.sz), myMin(1) { check(); }
    Sequence( const Cltn<Type>& aVec, const unsigned hi ) : vecPtr(aVec), myMax(hi), myMin(1) { check(); }
    Sequence( const Cltn<Type>& aVec, const unsigned lo, const unsigned hi ) : vecPtr(aVec), myMax(hi), myMin(lo) { check(); }
    virtual ~Sequence() {}

    virtual Type operator()();
    Type operator[]( const unsigned ix ) const { return vecPtr[ix+myMin-1]; }

    unsigned size() const { return myMax + 1 - myMin; }
    unsigned find( const Type & ) const;
    Type lastItem() const;
    Type nextItem() const;
    unsigned first() const { return myMin; }
    unsigned last() const { return myMax; }

private:
    void check();

protected:
    const Cltn<Type>& vecPtr;
    unsigned myIndex;
    const unsigned myMax;
    const unsigned myMin;
};

template <class Type>
class BackwardsSequence : public Sequence<Type> {
public:
    BackwardsSequence( const Cltn<Type>& aVec ) : Sequence<Type>( aVec ) { check(); }
    BackwardsSequence( const Cltn<Type>& aVec, const unsigned hi ) : Sequence<Type>( aVec, hi ) { check(); }
    BackwardsSequence( const Cltn<Type>& aVec, const unsigned lo, const unsigned hi ) : Sequence<Type>( aVec, lo, hi )  { check(); }

    virtual Type operator()();

private:
    void check();
};

#endif
