# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $id$
# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/parasol/configure.ac $

#AC_PREREQ(2.57)
AC_INIT(parsol, 3.2, greg@sce.carleton.ca)
AC_CONFIG_AUX_DIR([config])
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
AC_HEADER_STDC
AC_CHECK_HEADERS([fenv.h ieeefp.h malloc.h signal.h stddef.h stdlib.h string.h sys/param.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_TYPES([struct sigaltstack])
AC_CHECK_TYPES([stack_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([bzero getpagesize drand48 feenableexcept fpsetmask kill _longjmp memset memmove pow _setjmp select sigaltstack sqrt strcasecmp strchr strncasecmp strrchr strstr])
if test ${ac_cv_func_drand48} = "no"; then
   DRAND48=drand48.o
fi

AC_CONFIG_FILES([Makefile include/Makefile src/Makefile doc/Makefile tests/Makefile])
AC_OUTPUT
