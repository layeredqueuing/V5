# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqngen/configure.ac $
# ------------------------------------------------------------------------
# $Id: configure.ac 13769 2020-08-18 13:42:51Z greg $
# ------------------------------------------------------------------------
AC_PREREQ(2.57)
AC_INIT(lqngen, 5.17, greg@sce.carleton.ca)
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([lqngen.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_SUBST(STATIC)

# ----------------------------------------------------------------------
# Compiler and linking.
#
AC_LANG(C++)
AC_PROG_CXX
AM_PROG_LEX
AC_PROG_YACC
# Add in libtool.
AC_PROG_LIBTOOL
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

# ----------------------------------------------------------------------
# Checks for libraries.
#

# ----------------------------------------------------------------------
# Checks for header files.
#
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h values.h])
AC_CHECK_HEADERS([getopt.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_FUNC_STRTOD
AC_TYPE_SIZE_T

# ----------------------------------------------------------------------
# Checks for library functions.
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([getsubopt])
AC_CHECK_FUNCS([getopt_long])
AC_CHECK_FUNCS([mkdir rmdir])
AC_CHECK_FUNCS([drand48])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([strdup strerror strtol])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
