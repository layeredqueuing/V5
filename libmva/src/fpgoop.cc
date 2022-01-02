/*  -*- c++ -*-
 * $Id: fpgoop.cc 15322 2022-01-02 15:35:27Z greg $
 *
 * Floating point exception handling.  It is all different on all machines.
 * See:
 *   HP:	fpgetround(3).
 *   RS6000:	fp_read_flag(3) (plus CD rom info pages).
 *   SUN:	ieee_flags(3).
 *   linux...	sigaction(2)
 * 
 * if feenableexcet (fenv.h) is present, use it,
 * else if fpsetmask (ieeefp.h) is present, use it,
 *
 * A Common interface is presented to check for divide by zero, overflow,
 * and invalid operations.  Callers must either call set_fp_abort() to
 * cause immediate termination of the application at the fault location, of
 * check_fp_ok() to test afterwards at a convenient location.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <signal.h>
#if HAVE_FENV_H
#include <fenv.h>
#elif HAVE_IEEEFP_H && !defined(MSDOS)
#include <ieeefp.h>
#elif defined(_AIX)
#include <fptrap.h>
#include <fpxcp.h>
#elif defined(MSDOS) || HAVE_FLOAT_H
#undef __STRICT_ANSI__
#include <float.h>
#endif

#pragma STDC FENV_ACCESS ON

#include "fpgoop.h"

#if !defined(__WINNT__) && !defined(MSDOS) && HAVE_IEEEFP_H
typedef	fp_except_t fp_bit_type;
#elif defined(_AIX)
typedef	fpflag_t fp_bit_type;
#else
typedef	int fp_bit_type;
#endif

#if HAVE_IEEEFP_H && defined(__GNUC__) && (__GNUC__ >= 3) && (defined(MSDOS) || defined(__CYGWIN__))
#define	FP_X_DZ		FP_X_DX		/* Redefined by GNU...		*/
#define FP_CLEAR	0		/* If this works like solaris... */
#endif

#define EXCEPTION_EXIT 255		/* See input.h */

/*
 * IEEE exceptions.
 */

static struct {
    fp_bit_type bit;
    const char * str;
} fp_op_str[] = {
#if defined(__hpux) || (!defined(__WINNT__) && !defined(MSDOS) && HAVE_IEEEFP_H)
    { FP_X_INV, "Invalid operation" },
    { FP_X_DZ, "Overflow" },
    { FP_X_OFL, "Underflow" },
    { FP_X_UFL, "Divide by zero" },
    { FP_X_IMP, "Inexact result" },
#elif defined(HAVE_FENV_H)
    { FE_DIVBYZERO, "Divide by zero" },
#if defined(FE_DENORMAL)
    { FE_DENORMAL, "Denormal" },
#endif
    { FE_INEXACT, "Inexact result" },
    { FE_INVALID, "Invalid operation" },
    { FE_OVERFLOW, "Overflow" },
    { FE_UNDERFLOW, "Underflow" },
#elif defined(_AIX)
    { FP_INVALID, "Invalid operation" },
    { FP_OVERFLOW, "Overflow" },
    { FP_UNDERFLOW, "Underflow" },
    { FP_DIV_BY_ZERO, "Divide by zero" },
    { FP_INEXACT, "Inexact result" },
#elif  defined(MSDOS)
    { SW_INVALID, "Invalid operation" },
    { SW_UNDERFLOW, "Underflow" },
    { SW_OVERFLOW, "Overflow" },
    { SW_ZERODIVIDE, "Divide by zero" },
    { SW_INEXACT, "Inexact result" },
#endif
    { 0, 0 }
};

static fp_bit_type fp_bits = 0;
fp_exeception_reporting matherr_disposition;	/* What to do about math probs.	*/

/*
 * Print out gory details of fault.
 */

std::string
floating_point_error::construct( const std::string& file, const unsigned line ) 
{
    const fp_bit_type flags = fp_status_bits();
    std::ostringstream ss;

    ss << "Floating point exception" << ": " << file << " " << line << ": ";

    unsigned count = 0;
    for ( unsigned i = 0; fp_op_str[i].str; ++i ) {
	if ( flags & fp_op_str[i].bit ) {
	    if ( count > 0 ) {
		ss<< ", ";
	    }
	    ss << fp_op_str[i].str;
	    count += 1;
	}
    }
    return ss.str();
}

/*
 *                        ** KLUDGE ALERT **
 * We use func_ptr to cast the definition of my_handler to the proper type
 * defined in signal.h.  However, the signal handler actually accepts more
 * arguments (which are rather useful for error reporting).
 */

extern "C" {
#if defined(HAVE_SIGACTION)
#if defined(SA_SIGINFO)
static void my_handler( int, siginfo_t *, void * );
#else
static void my_handler( int );
#endif
#else
typedef	void (*func_ptr)(int);
#if  defined(__hpux) || defined(_AIX)
static void my_handler (int sig, int code, struct sigcontext *scp );
#else
static void my_handler (int sig);
#endif
#endif
}

/* ---------------- Floating point exception handling.  --------------- */

#if defined(_AIX)
static int trap_mask = 0x000000f0;
#endif

/*
 * Reset matherr signalling as per user's request.
 */

void
set_fp_abort()
{
#if defined(__APPLE__) && defined(__MACH__)
    static fenv_t fenv;
    unsigned int new_excepts = fp_bits & FE_ALL_EXCEPT;
    // previous masks
    unsigned int old_excepts;

    if (fegetenv(&fenv)) {
        return;
    }
    old_excepts = fenv.__control & FE_ALL_EXCEPT;

    // unmask
    fenv.__control &= ~new_excepts;
    fenv.__mxcsr   &= ~(new_excepts << 7);

    fesetenv(&fenv);
    
#elif defined(HAVE_FENV_H) && defined(HAVE_FEENABLEEXCEPT)
    feenableexcept( fp_bits );

#elif defined(HAVE_FENV_H) && defined(HAVE_FESETEXCEPTFLAG)
    fexcept_t fe_flags;
    fegetexceptflag( &fe_flags, fp_bits );
    fesetexceptflag( &fe_flags, fp_bits );

#elif HAVE_IEEEFP_H && defined(HAVE_FPSETMASK)
    fpsetmask( fp_bits );

#elif defined(__hpux)
    fpsetdefaults();

#elif defined(_AIX)
    fp_trap(FP_TRAP_SYNC);
    fp_enable(trap_mask);

#elif defined(MSDOS)
    int ignore = EM_UNDERFLOW|EM_INEXACT|EM_DENORMAL;
    if ( ( fp_bits & SW_OVERFLOW ) == 0 ) {
        ignore |= EM_OVERFLOW;
    }
    (void) _control87( ignore, MCW_EM );

#else
    #warning No FP abort.

#endif
#if defined(HAVE_SIGACTION)
    struct sigaction my_action;

#if defined(SA_SIGINFO) 
    my_action.sa_sigaction = my_handler;
    sigemptyset( &my_action.sa_mask );
    my_action.sa_flags = SA_SIGINFO;	/* Invoke the signal catching function with */
					/*   three arguments instead of one. */
#else
    my_action.sa_handler = my_handler;
    sigemptyset( &my_action.sa_mask );
    my_action.sa_flags = 0;
#endif

    assert( sigaction( SIGFPE, &my_action, 0 ) == 0 );
#else
    signal( SIGFPE, my_handler );
#endif
}


/*
 * signal handler for fp errors.
 * See /usr/include/sys/signal.h. (MacosX)
 */

#if defined(HAVE_SIGACTION) && defined(SA_SIGINFO)
static void
my_handler (int sig, siginfo_t * info, void * )
{
    if ( sig == SIGFPE ) {
	cerr << "Floating point exception: ";

	switch ( info->si_code ) {
#if defined(FPE_INTDIV)
	case FPE_INTDIV: cerr << "integer divide by zero"; break;
#endif
#if defined(FPE_INTOVF)
	case FPE_INTOVF: cerr << "integer overflow"; break;
#endif
	case FPE_FLTDIV: cerr << "floating point divide by zero"; break;
	case FPE_FLTOVF: cerr << "floating point overflow"; break;
	case FPE_FLTUND: cerr << "floating point underflow"; break;
	case FPE_FLTRES: cerr << "floating point inexact result"; break;
	case FPE_FLTINV: cerr << "floating point invalid operation"; break;
#if defined(FPE_FLTSUB)
	case FPE_FLTSUB: cerr << "subscript out of range"; break;
#endif
	default: cerr << "unknown!"; break;
	}

	cerr.fill('0');
	cerr.setf ( ios::hex|ios::showbase, ios::basefield|ios::showbase );  // set hex as the basefield
	cerr << " at " << setw(10) << reinterpret_cast<unsigned long>(info->si_addr);
	cerr << endl;
    } else {
	cerr << "Caught signal " << sig << endl;
    }
    exit( 255 );
}

#elif defined(__hpux)

static void
my_handler (int /* sig */, int code, struct sigcontext * /* scp */ )
{
    char buf[8];
    char * str;

    switch ( code ) {
    case 12: str = "(overflow)"; break;
    case 13: str = "(conditional)"; break;
    case 14: str = "(Assist Exception)"; break;
    case 22: str = "(Assist Emulation)"; break;

    default:
	(void) sprintf( buf, "%x", code );
	str = buf;
	break;
    }


    cerr << "FP exception " << str << " occured." << endl;
    assert(0);
}

#elif defined(_AIX)

/*
 * We have a very compilicated exception handler for AIX.  It patches up
 * the result of the operation so that processing can continue.  See XL
 * C++ User's Guide, page 74 for more information.
 */

typedef struct {
    ulong_t fpe_loc;			// location of fpe instruction.
    char * code;			// its opcode.
    int frt_reg;			// offset of t register...
    int fra_reg;
    int frb_reg;
    int frc_reg;
} opcode_t;

static char * op_table[32] = {
    "fcmp", "?",    "?",    "?",    "?",    "?",    "?",    "?",
    "?",    "?",    "?",    "?",    "frsp", "?",    "?",    "?",
    "?",    "?",    "fd",   "?",    "fs",   "fa",   "?",    "?",
    "?",    "fm",   "?",    "?",    "fms",  "fma",  "fnms", "fnma"
};

static int find_instr( ulong_t *, opcode_t & );

#define FLTTRAPINST (0x7c8f7808)

#define	TST_MASK	(FP_INVALID|FP_OVERFLOW|FP_UNDERFLOW|FP_DIV_BY_ZERO|FP_INEXACT)

static void
my_handler (int sig, int code, struct sigcontext * scp )
{
    fptrap_t fpstat = scp->sc_jmpbuf.jmp_context.fpscr;
    opcode_t opcode;

    if ( *((int *) scp->sc_jmpbuf.jmp_context.iar ) != FLTTRAPINST || !(fpstat &TST_MASK ) ) {
	cerr << "Integer exception (INT Divide by zero/Subscript bounds) at pc "
	     << scp->sc_jmpbuf.jmp_context.iar << endl;
    } else if ( !find_instr( (ulong_t *)scp->sc_jmpbuf.jmp_context.iar, opcode ) ) {
	cerr << "SIGTRAP cannot find exception point." << endl;
    } else {
	fp_bit_type flags = 0;

	if ( fpstat & (TRP_INVALID|FP_INVALID) == (TRP_INVALID|FP_INVALID) ) {
	    flags |= FP_INVALID;
	} else if ( fpstat & (TRP_OVERFLOW|FP_OVERFLOW) == (TRP_OVERFLOW|FP_OVERFLOW) ) {
	    flags |= FP_OVERFLOW;
	    scp->sc_jmpbuf.jmp_context.fpr[opcode.frt_reg] = 0.0;	// Patch up result.
	} else if ( fpstat & (TRP_UNDERFLOW|FP_UNDERFLOW) == (TRP_UNDERFLOW|FP_UNDERFLOW) ) {
	    flags |= FP_UNDERFLOW;
	    scp->sc_jmpbuf.jmp_context.fpr[opcode.frt_reg] = 0.0;	// Patch up result.
	} else if ( fpstat & (TRP_DIV_BY_ZERO|FP_DIV_BY_ZERO) == (TRP_DIV_BY_ZERO|FP_DIV_BY_ZERO) ) {
	    flags |= FP_DIV_BY_ZERO;
	    scp->sc_jmpbuf.jmp_context.fpr[opcode.frt_reg] = 0.0;	// Patch up result.
	} else if ( fpstat & (TRP_INEXACT|FP_INEXACT) == (TRP_INEXACT|FP_INEXACT) ) {
	    flags |= FP_INEXACT;
	}
	cerr << "FP exception (";
	printStatus( cerr, flags );
	cerr << "), opcode: " << opcode.code << " at pc " << opcode.fpe_loc << endl;
    }
    scp->sc_jmpbuf.jmp_context.fpscr &= ~FP_ALL_XCP;	// Reset bits -- they are sticky.
    scp->sc_jmpbuf.jmp_context.iar += 4;			// Continue with next instr.

    exit( 255 );
}


static int
find_instr( ulong_t * trap_loc, opcode_t & op )
{
    for ( int i = 0; i < 10; ++i ) {
	if ( *(--trap_loc) >> 26 == 63 ) goto found;
    }
    return 0;
 found:
    if ( ( (*trap_loc >> 1 ) & 0x3ff) == 72 ) return 0;	// fmr.

    op.fpe_loc = (ulong_t)trap_loc;
    op.code    = op_table[(*trap_loc >> 1) & 0x1f];
    op.frt_reg = (*trap_loc >> 21) & 0x1f;
    op.fra_reg = (*trap_loc >> 16) & 0x1f;
    op.frb_reg = (*trap_loc >> 11) & 0x1f;
    op.frc_reg = (*trap_loc >>  6) & 0x1f;
    return 1;
}

#else
void
my_handler ( int )
{
    throw floating_point_error( __FILE__, __LINE__ );
    exit( 255 );
}

#endif

/*
 * Return 1 if FP is o.k. to date, and 0 otherwise.
 */
bool
check_fp_ok()
{
#if defined(HAVE_FENV_H) && defined(HAVE_FETESTEXCEPT)

    return fetestexcept( fp_bits ) == 0;

#elif HAVE_IEEEFP_H && defined(HAVE_FPGETSTICKY)

    return (fpgetsticky() & fp_bits) == 0;

#elif defined(_AIX)

    return (fp_read_flag() & fp_bits) == 0;

#elif defined(MSDOS) || defined(__WINNT__)

    return (_status87() & fp_bits) == 0;

#else

    return true;

#endif
}


/*
 * Reset the floating point unit to a happy state.
 */

void
set_fp_ok( bool overflow )
{
#if defined(HAVE_FENV_H) && defined(HAVE_FECLEAREXCEPT) 

    fp_bits = FE_DIVBYZERO|FE_INVALID;
    if ( overflow ) {
	fp_bits |= FE_OVERFLOW;
    }
    feclearexcept( fp_bits|FE_INEXACT|FE_OVERFLOW|FE_UNDERFLOW );

#elif defined(__hpux)

    fpsetsticky( FP_X_CLEAR );

#elif HAVE_IEEEFP_H && defined(HAVE_FPSETSTICKY)

    fp_bits = FP_X_INV | FP_X_DZ;
    if ( overflow ) {
	fp_bits |= FP_X_OFL;
    }

    fpsetsticky( FP_CLEAR );

#elif defined(_AIX)

    fp_bits = FP_INVALID | FP_DIV_BY_ZERO;
    if ( overflow ) {
	fp_bits |= FP_OVERFLOW;
    }
    fp_clr_flag( FP_INVALID | FP_OVERFLOW | FP_UNDERFLOW | FP_DIV_BY_ZERO | FP_INEXACT );

#elif defined(MSDOS)
    
    fp_bits = SW_INVALID | SW_ZERODIVIDE;
    if ( overflow ) {
	fp_bits |= SW_OVERFLOW;
    }
    _fpreset();
    (void) _control87( RC_NEAR|PC_64, MCW_RC|MCW_PC );

#endif
}




/*
 * Return the floating point status bits.
 */

int
fp_status_bits()
{
#if  defined(HAVE_FENV_H) && defined(HAVE_FETESTEXCEPT)

    return fetestexcept( FE_DIVBYZERO|FE_INEXACT|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW );

#elif HAVE_IEEEFP_H && defined(HAVE_FPGETSTICKY)

    return fpgetsticky();

#elif defined(_AIX)

    return (int) fp_read_flag();

#elif defined(MSDOS) 

    return _status87() & (SW_INVALID|SW_ZERODIVIDE|SW_OVERFLOW|SW_UNDERFLOW|SW_INEXACT);

#elif defined(__WINNT__)

    return _status87() & ( FE_DIVBYZERO|FE_INEXACT|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW );
#else

    return 0;

#endif
}

/*
 * return factorial.  Cache results.
 */

double
factorial( unsigned n )
{
    static double a[101];
    if ( n == 1 || n == 0 ) return 1.0;
    if ( n <= 100 ) {
	if ( a[n] == 0 ) {
	    a[n] = static_cast<double>(n) * factorial(n-1);
	}
	return a[n];
    } else {
	double product;
	for ( product = 1.0; n > 1; --n ) {
	    product *= n;
	}
	return product;
    }
}


/*
 * return ln of factorial of n.  Cache results.
 */

double
log_factorial( const unsigned n )
{
    static double a[101];	/* Automatically initialized to zero! */
	
    if ( n == 0 ) throw std::domain_error( "log_factorial(0)" );
    if ( n == 1 ) return 0.0;
    if ( n <= 100 ) {
	if ( a[n] == 0 ) {
#if HAVE_LGAMMA	    
	    a[n] = lgamma( n + 1.0 );
#else
	    a[n] = ::log( n ) + log_factorial( n - 1 );
#endif
	}
	return a[n];
    } else {
#if HAVE_LGAMMA	    
	return lgamma( n + 1.0 );
#else
	return ::log( n ) + log_factorial( n - 1 );
#endif
    }
}


/*
 * Returns the binomial coefficient
 */

double
binomial_coef( const unsigned n, const unsigned k )
{
    return floor( 0.5 + exp( log_factorial( n ) - log_factorial( k ) - log_factorial( n - k ) ) );
}


/*
 * Exponentiation.  Handles negative exponents.
 */

double
power( double a, int b )
{
    if ( b > 5 ) {
	return pow( a, (double)b );
    } else if ( b >= 0 ) {
	double product = 1.0;
	for ( ; b > 0; --b ) product *= a;
	return product;
    } else {
	return 1.0 / power( a, -b );
    }
}


/*
 * Choose.
 * i! / ( j! * (i-j)! )
 */

double
choose( unsigned i, unsigned j )
{
    assert ( i >= j );
    if ( i == 0 ) return 0;
    if ( j == 0 || i == j ) return 1;
	
    double product;
    const unsigned int a = std::max( j, i - j );
    const unsigned int b = std::min( j, i - j );
	
    for ( product = 1.0; i > a; --i ) {
	product *= i;
    }
	
    return product / factorial( b );
}
