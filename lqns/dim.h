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
 * $Id: dim.h 13548 2020-05-21 14:27:18Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(DIMENSIONS_H)
#define DIMENSIONS_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <iostream>
#include <iomanip>
#include <cassert>
#include <stdexcept>
#include <string>
#include <cstring>

using namespace std;

#define MAX_CLASSES     200                     /* Max classes (clients)        */
#define MAX_PHASES      3                       /* Number of Phases.            */
#define N_SEMAPHORE_ENTRIES     2               /* Number of semaphore entries  */

static inline unsigned max( const unsigned a1, const unsigned a2 )
{
    return ( a1 > a2 ) ? a1 : a2;
}


static inline double max( const double a1, const double a2 )
{
    return ( a1 > a2 ) ? a1 : a2;
}


static inline double min( const double a1, const double a2 )
{
    return ( a1 < a2 ) ? a1 : a2;
}

static inline unsigned min( const unsigned a1, const unsigned a2 )
{
    return ( a1 < a2 ) ? a1 : a2;
}

/*
 * Return square.  C++ doesn't even have an exponentiation operator, let
 * alone a smart one.
 */
const double EPSILON = 0.000001;		/* For testing against 1 or 0 */

static inline double square( const double x )
{
    return x * x;
}


/*
 * Compute and return factorial.
 */
double ln_gamma( const double x );
double factorial( unsigned n );
double log_factorial( const unsigned n );
double binomial_coef( const unsigned n, const unsigned k );

/*
 * Compute and return power.  We don't use pow() because we know that b is always an integer.
 */

double power( const double a, const int b );

/*
 * Choose
 */

double choose( unsigned i, unsigned j );


/* 
 * Common under-relaxation code.  Adapted to include newton-raphson
 * adjustment.  
 */

void
under_relax( double& old_value, const double new_value, const double relax );


class class_error : public exception 
{
public:
    class_error( const string& aStr, const char * file, const unsigned line, const char * anError );
    virtual ~class_error() throw() = 0;
    virtual const char* what() const throw();

private:
    string myMsg;
};


class subclass_responsibility : public class_error 
{
public:
    subclass_responsibility( const string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Subclass responsibility." ) {}
    virtual ~subclass_responsibility() throw() {}
};

class not_implemented  : public class_error 
{
public:
    not_implemented( const string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Not implemented." ) {}
    virtual ~not_implemented() throw() {}
};


class should_not_implement  : public class_error 
{
public:
    should_not_implement( const string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Should not implement." ) {}
    virtual ~should_not_implement() throw() {}
};

class path_error : public exception {
public:
    explicit path_error( const unsigned depth=0 ) : myDepth(depth) {}
    virtual ~path_error() throw() = 0;
    virtual const char * what() const throw();
    unsigned depth() const { return myDepth; }

protected:
    string myMsg;
    const unsigned myDepth;
};

class exception_handled : public exception 
{
public:
    explicit exception_handled( const string& aStr ) : exception(), myMsg(aStr) {}
    virtual ~exception_handled() throw() {}
    virtual const char * what() const throw();

private:
    string myMsg;
};

static inline void throw_bad_parameter() { throw std::domain_error( "invalid parameter" ); }

class Activity;
class Entry;
class Call;
class Task;
class Entity;
template <class type> class Sequence;

/*
 * Compare to tasks by their name.  Used by the set class to insert items
 */

struct ltTask
{
    bool operator()(const Task * p1, const Task * p2) const;
};

struct lt_str
{
    bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; }
};

struct lt_int
{
    bool operator()(const int i1, const int i2) const { return i1 < i2;	}
};


typedef void (Activity::*AggregateFunc)(Entry *,const unsigned,const unsigned);
typedef double (Activity::*AggregateFunc2)(const Entry *,const unsigned,const double) const;
typedef void (Call::*callFunc)( const unsigned, const unsigned, const double );
typedef bool (Call::*queryFunc)() const;
#endif
