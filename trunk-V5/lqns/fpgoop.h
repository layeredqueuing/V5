/* -*- c++ -*-
 * $HeadURL$
 *
 * FP error handling.
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

#ifndef	FPGOOP_H
#define	FPGOOP_H

#include "dim.h"
#if HAVE_IEEEFP_H && !defined(MSDOS)
#include <ieeefp.h>
#endif

/* Borland C++ 4.02 does not have the IEEE math support functions like
 * isisfinite(), isnan(), etc.  However, precise exceptions are supported and used
 * here which means that these kinds of tests are unnecessary. */

#if !HAVE_FINITE
#if defined(isisfinite)		/* See math.h */
#define isfinite(d) isfinite(d)
#else
#define isfinite(d) (1)
#endif
#endif


void set_fp_abort();
bool check_fp_ok();
void set_fp_ok( bool );
int fp_status_bits();
double get_infinity();

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class floating_point_error : public exception {
public:
    floating_point_error( const char * f, const unsigned l );

    virtual ~floating_point_error() throw() {} 
    virtual const char * what() const throw();

private:
    string myMsg;
};

typedef enum { FP_IGNORE, FP_REPORT, FP_DEFERRED_ABORT, FP_IMMEDIATE_ABORT } fp_exeception_reporting;
extern fp_exeception_reporting matherr_disposition;	/* What to do about math probs.	*/
#endif
