/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/dim.h $
 *
 * Dimensions common to everything, plus some funky inline functions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: dim.h 14625 2021-05-09 13:02:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(LQNS_DIM_H)
#define LQNS_DIM_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <cstring>

#define	BUG_270		1

#define MAX_CLASSES     200                     /* Max classes (clients)        */
#define MAX_PHASES      3                       /* Number of Phases.            */
#define N_SEMAPHORE_ENTRIES     2               /* Number of semaphore entries  */
#define PAN_REPLICATION	1			/* Use Amy Pan's replication	*/

/*
 * Return square.  C++ doesn't even have an exponentiation operator, let
 * alone a smart one.
 */
const double EPSILON = 0.000001;		/* For testing against 1 or 0 */

template <typename Type> inline Type square( Type a ) { return a * a; }

/* 
 * Common under-relaxation code.  Adapted to include newton-raphson
 * adjustment.  
 */

void under_relax( double& old_value, const double new_value, const double relax );

class exception_handled : public std::runtime_error
{
public:
    explicit exception_handled( const std::string& aStr ) : std::runtime_error(aStr.c_str()) {}
    virtual ~exception_handled() throw() {}
};

static inline void throw_bad_parameter() { throw std::domain_error( "invalid parameter" ); }

struct lt_str
{
    bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; }
};

struct lt_int
{
    bool operator()(const int i1, const int i2) const { return i1 < i2;	}
};

template <class Type> struct Exec
{
    typedef Type& (Type::*funcPtr)();
    Exec<Type>( funcPtr f ) : _f(f) {};
    void operator()( Type * object ) const { (object->*_f)(); }
    void operator()( Type& object ) const { (object.*_f)(); }
private:
    funcPtr _f;
};

template <class Type1, class Type2> struct Exec1
{
    typedef Type1& (Type1::*funcPtr)( Type2 x );
    Exec1<Type1,Type2>( funcPtr f, Type2 x ) : _f(f), _x(x) {}
    void operator()( Type1 * object ) const { (object->*_f)( _x ); }
    void operator()( Type1& object ) const { (object.*_f)( _x ); }
private:
    funcPtr _f;
    Type2 _x;
};

template <class Type1, class Type2, class Type3> struct Exec2
{
    typedef Type1& (Type1::*funcPtr)( Type2 x, Type3 y );
    Exec2<Type1,Type2,Type3>( funcPtr f, Type2 x, Type3 y ) : _f(f), _x(x), _y(y) {}
    void operator()( Type1 * object ) const { (object->*_f)( _x, _y ); }
    void operator()( Type1& object ) const { (object.*_f)( _x, _y ); }
private:
    funcPtr _f;
    Type2 _x;
    Type3 _y;
};

template <class Type> struct ConstExec
{
    typedef const Type& (Type::*funcPtr)() const;
    ConstExec<Type>( const funcPtr f ) : _f(f) {}
    void operator()( const Type * object ) const { (object->*_f)(); }
    void operator()( const Type& object ) const { (object.*_f)(); }
private:
    const funcPtr _f;
};
    
template <class Type1, class Type2 > struct ExecSum
{
    typedef Type2 (Type1::*funcPtr)();
    ExecSum<Type1,Type2>( funcPtr f ) : _f(f), _sum(0.) {}
    void operator()( Type1 * object ) { _sum += (object->*_f)(); }
    void operator()( Type1& object ) { _sum += (object.*_f)(); }
    Type2 sum() const { return _sum; }
private:
    const funcPtr _f;
    Type2 _sum;
};
    
template <class Type1, class Type2 > struct ExecSumSquare
{
    typedef Type2 (Type1::*funcPtr)() const;
    ExecSumSquare<Type1,Type2>( funcPtr f ) : _f(f), _sum(0.) {}
    void operator()( Type1 * object ) { _sum += square( (object->*_f)() ); }
    void operator()( Type1& object ) { _sum += square( (object.*_f)() ); }
    Type2 sum() const { return _sum; }
private:
    const funcPtr _f;
    Type2 _sum;
};
    
template <class Type1, class Type2, class Type3 > struct ExecSum1
{
    typedef Type2 (Type1::*funcPtr)( Type3 );
    ExecSum1<Type1,Type2,Type3>( funcPtr f, Type3 arg ) : _f(f), _arg(arg), _sum(0.) {}
    void operator()( Type1 * object ) { _sum += (object->*_f)( _arg ); }
    void operator()( Type1& object ) { _sum += (object.*_f)( _arg ); }
    Type2 sum() const { return _sum; }
private:
    const funcPtr _f;
    Type3 _arg;
    Type2 _sum;
};
    
template <class Type1, class Type2, class Type3, class Type4 > struct ExecSum2
{
    typedef Type2 (Type1::*funcPtr)( Type3, Type4 );
    ExecSum2<Type1,Type2,Type3,Type4>( funcPtr f, Type3 arg1, Type4 arg2 ) : _f(f), _arg1(arg1), _arg2(arg2), _sum(0.) {}
    void operator()( Type1 * object ) { _sum += (object->*_f)( _arg1, _arg2 ); }
    void operator()( Type1& object ) { _sum += (object.*_f)( _arg1, _arg2 ); }
    Type2 sum() const { return _sum; }
private:
    const funcPtr _f;
    Type3 _arg1;
    Type4 _arg2;
    Type2 _sum;
};
    
template <class Type1, class Type2> struct ConstExec1
{
    typedef const Type1& (Type1::*funcPtr)( Type2 ) const;
    ConstExec1<Type1,Type2>( const funcPtr f, Type2 x ) : _f(f), _x(x) {}
    void operator()( const Type1 * object ) const { (object->*_f)(_x); }
    void operator()( const Type1& object ) const { (object.*_f)(_x); }
private:
    const funcPtr _f;
    Type2 _x;
};
    
template <class Type1, class Type2, class Type3> struct ConstExec2
{
    typedef const Type1& (Type1::*funcPtr)( Type2 x, Type3 y ) const;
    ConstExec2<Type1,Type2,Type3>( const funcPtr f, Type2 x, Type3 y ) : _f(f), _x(x), _y(y) {}
    void operator()( const Type1 * object ) const { (object->*_f)( _x, _y ); }
    void operator()( const Type1& object ) const { (object.*_f)( _x, _y ); }
private:
    const funcPtr _f;
    Type2 _x;
    Type3 _y;
};

template <class Type1> struct ConstPrint
{
    typedef std::ostream& (Type1::*funcPtr)( std::ostream& ) const;
    ConstPrint<Type1>( const funcPtr f, std::ostream& o ) : _f(f), _o(o) {}
    void operator()( const Type1 * object ) const { (object->*_f)( _o ); }
    void operator()( const Type1& object ) const { (object.*_f)( _o ); }
private:
    const funcPtr _f;
    std::ostream& _o;
};


template <class Type1, class Type2> struct ConstPrint1
{
    typedef std::ostream& (Type1::*funcPtr)( std::ostream&, Type2 ) const;
    ConstPrint1<Type1,Type2>( const funcPtr f, std::ostream& o, Type2 x ) : _f(f), _o(o), _x(x) {}
    void operator()( const Type1 * object ) const { (object->*_f)( _o, _x ); }
    void operator()( const Type1& object ) const { (object.*_f)( _o, _x ); }
private:
    const funcPtr _f;
    std::ostream& _o;
    const Type2 _x;
};


template <class Type> struct EQ
{
    EQ<Type>( const Type * const a ) : _a(a) {}
    bool operator()( const Type * const b ) const { return _a == b; }
private:
    const Type * const _a;
};

template <class Type> struct EQStr
{
    EQStr( const std::string & s ) : _s(s) {}
    bool operator()(const Type * e1 ) const { return e1->name() == _s; }
private:
    const std::string & _s;
};

template <class Type> struct Predicate
{
    typedef bool (Type::*predicate)() const;
    Predicate<Type>( const predicate p ) : _p(p) {};
    bool operator()( const Type * object ) const { return (object->*_p)(); }
    bool operator()( const Type& object ) const { return (object.*_p)(); }
private:
    const predicate _p;
};

template <class Type1, class Type2> struct Predicate1
{
    typedef bool (Type1::*predicate)( Type2 ) const;
    Predicate1<Type1,Type2>( const predicate p, Type2 arg ) : _p(p), _arg(arg) {};
    bool operator()( const Type1 * object ) const { return (object->*_p)(_arg); }
    bool operator()( const Type1& object ) const { return (object.*_p)(_arg); }
private:
    const predicate _p;
    Type2 _arg;
};

template <class Type1> struct add_using
{
    typedef double (Type1::*funcPtr)() const;
    add_using<Type1>( funcPtr f ) : _f(f) {}
    double operator()( double l, const Type1 * r ) const { return l + (r->*_f)(); }
    double operator()( double l, const Type1& r ) const { return l + (r.*_f)(); }
private:
    const funcPtr _f;
};

template <class Type1, class Type2> struct add_using_arg
{
    typedef double (Type1::*funcPtr)( Type2 ) const;
    add_using_arg<Type1,Type2>( funcPtr f, Type2 arg ) : _f(f), _arg(arg) {}
    double operator()( double l, const Type1 * r ) const { return l + (r->*_f)(_arg); }
    double operator()( double l, const Type1& r ) const { return l + (r.*_f)(_arg); }
private:
    const funcPtr _f;
    Type2 _arg;
};

template <class Type1, class Type2, class Type3> struct add_two_args
{
    typedef double (Type1::*funcPtr)( Type2, Type3 ) const;
    add_two_args<Type1,Type2,Type3>( funcPtr f, Type2 arg1, Type3 arg2 ) : _f(f), _arg1(arg1), _arg2(arg2) {}
    double operator()( double l, const Type1 * r ) const { return l + (r->*_f)(_arg1,_arg2); }
    double operator()( double l, const Type1& r ) const { return l + (r.*_f)(_arg1,_arg2); }
private:
    const funcPtr _f;
    Type2 _arg1;
    Type3 _arg2;
};

template <class Type1> struct max_using
{
    typedef unsigned int (Type1::*funcPtr)() const;
    max_using<Type1>( funcPtr f ) : _f(f) {}
    unsigned int operator()( unsigned int l, const Type1 * r ) const { return std::max( l, (r->*_f)() ); }
    unsigned int operator()( unsigned int l, const Type1& r ) const { return std::max( l, (r.*_f)() ); }
private:
    const funcPtr _f;
};

template <class Type1, class Type2> struct max_using_arg
{
    typedef unsigned int (Type1::*funcPtr)( Type2 ) const;
    max_using_arg<Type1,Type2>( funcPtr f, Type2 arg ) : _f(f), _arg(arg) {}
    unsigned int operator()( unsigned int l, const Type1 * r ) const { return std::max( l, (r->*_f)(_arg) ); }
    unsigned int operator()( unsigned int l, const Type1& r ) const { return std::max( l, (r.*_f)(_arg) ); }
private:
    const funcPtr _f;
    Type2 _arg;
};

template <class Type1, class Type2, class Type3 > struct max_two_args
{
    typedef unsigned int (Type1::*funcPtr)( Type2, Type3 ) const;
    max_two_args<Type1,Type2,Type3>( funcPtr f, Type2 arg1, Type3 arg2 ) : _f(f), _arg1(arg1), _arg2(arg2) {}
    unsigned int operator()( unsigned int l, const Type1 * r ) const { return std::max( l, (r->*_f)(_arg1,_arg2) ); }
    unsigned int operator()( unsigned int l, const Type1& r ) const { return std::max( l, (r.*_f)(_arg1,_arg2) ); }
private:
    const funcPtr _f;
    Type2 _arg1;
    Type3 _arg2;
};

template <class Type1, class Type2> struct unsigned_add_using_arg
{
    typedef unsigned (Type1::*funcPtr)( Type2 ) const;
    unsigned_add_using_arg<Type1,Type2>( funcPtr f, Type2 arg ) : _f(f), _arg(arg) {}
    unsigned operator()( unsigned l, const Type1 * r ) const { return l + (r->*_f)(_arg); }
    unsigned operator()( unsigned l, const Type1& r ) const { return l + (r.*_f)(_arg); }
private:
    const funcPtr _f;
    Type2 _arg;
};
#endif
