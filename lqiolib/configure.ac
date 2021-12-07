#                                               -*- Autoconf -*-
# $Id: configure.ac 15165 2021-12-07 01:23:08Z greg $
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([liblqio],[0.3.0],[martinmroz@gmail.com])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_MACRO_DIR([../config])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
LT_INIT
AC_PROG_LEX([noyywrap])
AC_PROG_YACC
AC_PROG_CXX
AC_PROG_CC
AC_LANG(C++)

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([unistd.h malloc.h strings.h pwd.h values.h time.h])
AC_CHECK_HEADERS([sys/mman.h sys/param.h sys/resource.h sys/time.h sys/times.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([ctime memset strchr strdup strerror strftime])
AC_CHECK_FUNCS([geteuid getpwuid getrusage gettimeofday uname mmap])

AC_CHECK_HEADERS([expat.h])
AC_CHECK_LIB([expat], [XML_ParserCreateNS])
AM_CONDITIONAL([EXPAT], [test "x${ac_cv_lib_expat_XML_ParserCreateNS}" = "xyes"] )

# Produce the desired output files
AC_CONFIG_FILES([Makefile src/Makefile src/headers/lqio/Makefile])
AC_OUTPUT
