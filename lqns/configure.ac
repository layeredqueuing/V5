# Process this file with autoconf to produce a configure script.
# ------------------------------------------------------------------------
# $Id: configure.ac 14563 2021-03-18 02:07:31Z greg $
# ------------------------------------------------------------------------
AC_INIT([lqns],[5.20],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([dim.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_MAKE_SET
AC_SUBST(STATIC)

# ----------------------------------------------------------------------
#
AC_LANG(C++)
AC_PROG_CXX

# Add in libtool.
LT_INIT
LT_INIT

if test "x$enable_shared" = "xno"; then
   echo "Static build."
   STATIC=-all-static
else
   echo "Dynamic build."
   STATIC=
fi

# ----------------------------------------------------------------------
# Checks for programs.
#
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_CHECK_PROGS(XPS2PDF, ps2pdf)
if test -z "$XPS2PDF"; then
  AC_MSG_NOTICE([Ps2pdf not found - Unable to create PDF version of the user manual])
fi
AM_CONDITIONAL([HAVE_PS2PDF], test -n "$XPS2PDF")

# ----------------------------------------------------------------------
# Checks for libraries.
#
AC_CHECK_LIB([m], [sqrt])
quorum=no;
AC_ARG_WITH(quorum,
	AS_HELP_STRING([--with-quorum],[Provided the GSL libraries are present, compile with support for quorum join. [default=no]]),
        [
	    quorum="$withval"
        ])
if test "$quroum" = "yes"; then
     AC_CHECK_HEADERS([gsl/gsl_cdf.h gsl/gsl_math.h])
     AC_CHECK_LIB([gslcblas], [cblas_dsdot])
     AC_CHECK_LIB([gsl], [gsl_cdf_gamma_P])
fi

#AC_CHECK_LIB([pthread], [pthread_create])
AM_CONDITIONAL([QUORUM], [test ${ac_cv_lib_gsl_gsl_cdf_gamma_P} = "yes"] )
# ----------------------------------------------------------------------
# Checks for header files.
#
AC_HEADER_SYS_WAIT
AC_TYPE_SIZE_T
AC_CHECK_HEADERS([fenv.h float.h getopt.h ieeefp.h limits.h unistd.h values.h sys/resource.h sys/utsname.h])
# ----------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_STAT
AC_CHECK_DECLS([sys_siglist],[],[],[#include <signal.h>
/* NetBSD declares sys_siglist in unistd.h.  */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
])

AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_HEADERS_ONCE([sys/time.h])
AC_STRUCT_TM
AC_C_BIGENDIAN
AC_C_INLINE
AC_HEADER_STDBOOL

# ----------------------------------------------------------------------
# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_SELECT_ARGTYPES 
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_STRUCT_TM
AC_CHECK_FUNCS([ctime floor isascii lgamma mkdir pow sqrt snprintf strcasecmp strdup strerror strrchr strtol uname rmdir])
AC_CHECK_FUNCS([feclearexcept feenableexcept fesetexceptflag fetestexcept finite fpgetsticky fpsetmask fpsetsticky ieee_retrospective signbit])
AC_CHECK_FUNCS([getopt_long getsubopt getrusage])
AC_CONFIG_FILES([Makefile regression/Makefile])
AC_OUTPUT
