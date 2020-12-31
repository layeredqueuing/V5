#                                               -*- Autoconf -*-
# $Id: configure.ac 14307 2020-12-31 15:54:48Z greg $
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT(liblmva, 0.1.0, greg@sce.carleton.ca)
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_MACRO_DIR([../config])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])

# Checks for programs.
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CXX
AC_LANG(C++)

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_STDC
AC_CHECK_HEADERS([fenv.h float.h getopt.h ieeefp.h limits.h unistd.h values.h sys/resource.h sys/utsname.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN

# Checks for library functions.
AC_CHECK_LIB([m], [sqrt])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_SELECT_ARGTYPES 
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_STRUCT_TM
AC_CHECK_FUNCS([ctime floor isascii lgamma mkdir pow sqrt snprintf strcasecmp strdup strerror strrchr strtol uname rmdir])
AC_CHECK_FUNCS([feclearexcept feenableexcept fesetexceptflag fetestexcept finite fpgetsticky fpsetmask fpsetsticky ieee_retrospective signbit])
AC_CHECK_FUNCS([getopt_long getsubopt getrusage])

AC_CONFIG_FILES([Makefile src/Makefile src/headers/mva/Makefile regression/Makefile])
AC_OUTPUT
