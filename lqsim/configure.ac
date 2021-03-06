#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# ------------------------------------------------------------------------
# $Id: configure.ac 14878 2021-07-06 19:07:54Z greg $
# ------------------------------------------------------------------------
AC_INIT([lqsim],[5.21],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([lqsim.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_PROG_MAKE_SET
AC_SUBST(STATIC)

# ----------------------------------------------------------------------
# Checks for programs.
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
AC_PROG_INSTALL
AC_PROG_LN_S
AC_CHECK_PROGS(XPS2PDF, ps2pdf)
if test -z "$XPS2PDF"; then
  AC_MSG_NOTICE([Ps2pdf not found - Unable to create PDF version of the user manual])
fi
AM_CONDITIONAL([HAVE_PS2PDF], test -n "$XPS2PDF")

# ----------------------------------------------------------------------
# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])

# ----------------------------------------------------------------------
# Checks for header files.

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fenv.h float.h getopt.h limits.h malloc.h stdlib.h string.h sys/time.h sys/times.h unistd.h values.h])
AC_CHECK_HEADERS([regex.h sys/wait.h ieeefp.h sys/utsname.h])
# ----------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# ----------------------------------------------------------------------
# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([floor getopt_long pow regcomp sqrt strchr strdup strerror uname])
AC_CHECK_FUNCS([feclearexcept feenableexcept fesetexceptflag fetestexcept finite fpclass fpgetsticky fpsetmask fpsetsticky])

AC_CONFIG_FILES([Makefile regression/Makefile])
AC_OUTPUT
