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
 * $Id: fpgoop.h 16194 2022-12-23 03:22:28Z greg $
 *
 * ------------------------------------------------------------------------
 */

#pragma once
#ifndef	LIBMVA_FPGOOP_H
#define	LIBMVA_FPGOOP_H

#pragma STDC FENV_ACCESS ON

#include <string>
#include <stdexcept>

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

class floating_point_error : public std::runtime_error {
public:
    floating_point_error( const std::string& f, const unsigned l ) : std::runtime_error( construct( f, l ) ) {}

private:
    std::string construct( const std::string& f, const unsigned l );
};

enum class fp_exception_reporting { IGNORE, REPORT, DEFERRED_ABORT, IMMEDIATE_ABORT };
extern fp_exception_reporting matherr_disposition;	/* What to do about math probs.	*/
#endif
