# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $id$
# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/parasol/configure.ac $

AC_PREREQ([2.69])
AC_INIT([parsol],[3.2],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_SUBST(DRAND48)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])

# Checks for header files.
AC_CHECK_HEADERS([fenv.h ieeefp.h malloc.h signal.h stddef.h stdlib.h string.h sys/param.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_HEADERS_ONCE([sys/time.h])
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_TYPES([struct sigaltstack])
AC_CHECK_TYPES([stack_t], [], [], [[#include <signal.h>]])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([bzero getpagesize drand48 feenableexcept fpsetmask kill _longjmp memset memmove pow _setjmp select sigaltstack sqrt strcasecmp strchr strncasecmp strrchr strstr])
if test ${ac_cv_func_drand48} = "no"; then
   DRAND48=drand48.o
fi

# The user can disable building of the PDF of the manual, for example
# if the required LaTeX packages are not installed
AC_ARG_ENABLE([latex-doc],
    [AS_HELP_STRING([--disable-latex-doc], [disable building the PDF
    documentation from LaTeX source])],
    [latexdoc=no],
    [latexdoc=yes])

if test "x$latexdoc" = "xyes"; then
   AC_MSG_NOTICE([building of the PDF of the user manual from LaTeX source is enabled])
   # Check for presence of pdfLaTeX
   AC_CHECK_PROGS(XLATEX, latex)
   if test -z "$XLATEX"; then
      AC_MSG_NOTICE([LaTeX not found - Unable to create PDF version of the user manual])
   fi
fi
AM_CONDITIONAL([HAVE_LATEX], test -n "$XLATEX")
AM_CONDITIONAL([BUILD_latexdoc], test "x$latexdoc" = "xyes")
AC_CONFIG_FILES([Makefile include/parasol/Makefile src/Makefile doc/Makefile tests/Makefile])
AC_OUTPUT
