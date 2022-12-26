#                                               -*- Autoconf -*-
# $Id: configure.ac 16200 2022-12-26 21:57:14Z greg $
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([liblmva],[0.1.0],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_MACRO_DIR([../config])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])

# Checks for programs.
LT_INIT
AC_PROG_INSTALL
AC_PROG_CXX
AC_LANG(C++)

# Checks for header files.
AC_CHECK_HEADERS([fenv.h ieeefp.h signal.h])
AC_CHECK_HEADERS([float.h])
AC_CHECK_HEADERS([xmmintrin.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB([m], [sqrt])
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([feclearexcept feenableexcept fesetexceptflag fetestexcept finite fpgetsticky fpsetmask fpsetsticky])
AC_CHECK_FUNCS([sigaction])
AC_CHECK_FUNCS([_clearfp _controlfp_s _statusfp])		# windows
AC_CONFIG_FILES([Makefile src/Makefile src/headers/mva/Makefile regression/Makefile])
AC_OUTPUT
