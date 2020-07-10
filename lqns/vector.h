/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/vector.h $
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
 * $Id: vector.h 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef	LQNS_VECTOR_H
#define	LQNS_VECTOR_H

#include <assert.h>
#include <iostream>
#include <iterator>

template <typename Type> class Vector;
template <typename Type> class VectorMath;
template <typename Type> class VectorIterator;
template <typename Type> inline std::ostream& operator << ( std::ostream& output, const Vector<Type>& self ) { return self.print( output ); }
template <typename Type> inline std::ostream& operator << ( std::ostream& output, const VectorMath<Type>& self ) { return self.print( output ); }

const int VectorSize = 0;

template <typename Type>
class Vector {
public:
    typedef Type value_type;
    typedef size_t size_type;

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
    explicit Vector( size_type size=0 ) : ia(0), sz(0), mx(0) { resize( size ); }
    Vector( const Type *ar, size_type sz ) { init(ar,sz); }
    Vector( const Vector<Type> &iA ) { init( iA.ia, iA.sz ); }
    virtual ~Vector();
    void clear();

    bool operator==( const Vector<Type>& arg ) const;
    Vector<Type>& operator=( const Vector<Type>& );
    Vector<Type>& operator=( const Type& );

    unsigned find( const Type& elem ) const;	/* temp */

    size_type size() const { return sz; }
    size_type max_size() const { return mx; }
    void resize( size_type size, const value_type = value_type() );
    void insert( const unsigned, const Type& );
    void push_back( const Type& arg ) { insert( size()+1, arg ); }
    iterator erase( const_iterator pos );
    
    std::ostream& print( std::ostream& = std::cout ) const;
	
    Type& first() { assert( sz > 0 ); return ia[1]; }
    Type& first() const { assert( sz > 0 ); return ia[1]; }
    Type& last() { assert( sz > 0 ); return ia[sz]; }
    Type& last() const { assert( sz > 0 ); return ia[sz]; }
    Type& operator[](const unsigned ix) { assert( ix && ix <= sz ); return ia[ix]; }
    Type& operator[](const unsigned ix) const { assert( ix && ix <= sz ); return ia[ix]; }

protected:
    void grow( size_type, const value_type );
    void shrink( size_type );
    
private:
    void init( const Type*, const size_type );

protected:
    Type *ia;

private:
    size_type sz;
    size_type mx;
};



template <typename Type>
class VectorMath : public Vector<Type> {
	
public:
    explicit VectorMath<Type>( unsigned size=0, Type init=0 ) : Vector<Type>() { this->grow( size, init ); }
    VectorMath<Type>( const Type *ar, unsigned sz ) : Vector<Type>( ar, sz ) {}
    VectorMath<Type>( const VectorMath<Type> &iA ) : Vector<Type>( iA ) {}

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
