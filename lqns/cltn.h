/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/cltn.h $
 *
 * Cltns.  Range checked.  Some set functions.  Start from 1.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: cltn.h 11963 2014-04-10 14:36:42Z greg $
 *
 * ------------------------------------------------------------------------
 */


#ifndef	CLTN_H
#define	CLTN_H

#include "dim.h"
#include <assert.h>


template <class Type> class Cltn;
template <class Type> class Sequence;
template <class Type> class BackwardsSequence;
template <class Type> ostream& operator << ( ostream&, const Cltn<Type>& );

const int CltnSize = 0;

template <class Type>
class Cltn {
    friend class Sequence<Type>;
    friend class BackwardsSequence<Type>;
    friend class CallInfo;		/* See entry.C	*/
    friend class TaskCallList;		/* See task.C	*/
    friend class EntryCallList;		/* See entry.C	*/
	
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
    Cltn<Type>  operator+( const Cltn<Type>& ) const;	/* Union	*/
    Cltn<Type>& operator+=( const Cltn<Type>& );		/* Union	*/
    Cltn<Type>& operator+=( const Type& );			/* Union	*/
    Cltn<Type>  operator-( const Cltn<Type>& ) const;	/* Difference	*/
    Cltn<Type>& operator-=( const Cltn<Type>& );		/* Difference	*/
    Cltn<Type>& operator-=( const Type& );			/* Removal	*/
    Cltn<Type>  operator&( const Cltn<Type>& ) const;	/* Intersection	*/
    Cltn<Type>& operator&=( const Cltn<Type>& );		/* Intersection	*/
	
    unsigned size() const { return sz; }
    void grow( const unsigned );
    void chop( const unsigned );
    void clearContents();
    void deleteContents();
    void deleteContents( unsigned int );
	
    ostream& print( ostream& = cout ) const;
	
    unsigned find( const Type & ) const;
    unsigned findOrAdd( const Type& );

    Type& operator[](const unsigned ix) { assert( ix && ix <= sz ); return ia[ix]; }
    Type operator[](const unsigned ix) const { assert( ix && ix <= sz ); return ia[ix]; }

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
