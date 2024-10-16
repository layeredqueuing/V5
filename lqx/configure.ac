#                                               -*- Autoconf -*-
# $Id: configure.ac 17376 2024-10-16 18:39:20Z greg $
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lqx],[0.3.0],[martinmroz@gmail.com])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_MACRO_DIR([../config])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([drvsrc/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_SUBST(YYLINENO)
# Checks for programs.
AC_LANG(C++)
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LEX([noyywrap])
AC_PROG_YACC

# Checks for libraries.
LT_INIT
AC_CHECK_LIB([m], [fmod])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h sys/time.h])

if test "`flex --version`" = 'flex version 2.5.4'; then
  YYLINENO='-l'
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_STRTOD
AC_CHECK_FUNCS([drand48 gettimeofday strdup strtol floor memset mmap pow strcasecmp strchr strrchr])
AC_CHECK_HEADERS([sys/mman.h])

AC_CONFIG_FILES([Makefile preprocessing/Makefile libsrc/Makefile drvsrc/Makefile libsrc/headers/lqx/Makefile])
AC_OUTPUT
