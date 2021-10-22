/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/libmva/src/headers/mva/fpgoop.h $
 *
 * FP error handling.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: fpgoop.h 15089 2021-10-22 16:14:46Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef	FPGOOP_H
#define	FPGOOP_H

#include <config.h>
#if HAVE_IEEEFP_H && !defined(__WINNT__) &&!defined(MSDOS)
#include <ieeefp.h>
#endif
#include <string>
#include <exception>

void set_fp_abort();
bool check_fp_ok();
void set_fp_ok( bool );
int fp_status_bits();

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

double power( double a, int b );

/*
 * Choose
 */

double choose( unsigned i, unsigned j );
inline double square( double x ) { return x * x; }


/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class floating_point_error : public std::exception {
public:
    floating_point_error( const char * f, const unsigned l );

    virtual ~floating_point_error() throw() {} 
    virtual const char * what() const throw();

private:
    std::string _msg;
};

typedef enum { FP_IGNORE, FP_REPORT, FP_DEFERRED_ABORT, FP_IMMEDIATE_ABORT } fp_exeception_reporting;
extern fp_exeception_reporting matherr_disposition;	/* What to do about math probs.	*/

class class_error : public std::exception 
{
public:
    class_error( const std::string& aStr, const char * file, const unsigned line, const char * anError );
    virtual ~class_error() throw() = 0;
    virtual const char* what() const throw();

private:
    std::string _msg;
};


class subclass_responsibility : public class_error 
{
public:
    subclass_responsibility( const std::string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Subclass responsibility." ) {}
    virtual ~subclass_responsibility() throw() {}
};

class not_implemented  : public class_error 
{
public:
    not_implemented( const std::string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Not implemented." ) {}
    virtual ~not_implemented() throw() {}
};

class should_not_implement  : public class_error 
{
public:
    should_not_implement( const std::string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Should not implement." ) {}
    virtual ~should_not_implement() throw() {}
};

#endif

