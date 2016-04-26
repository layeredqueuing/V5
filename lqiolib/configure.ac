#                                               -*- Autoconf -*-
# $Id: configure.ac 12554 2016-04-08 20:28:43Z greg $
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT(liblqio, 0.2.0, martinmroz@gmail.com)
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_MACRO_DIR([../config])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

#
# -- Xerces --
#
AC_DEFUN([rlc_XERCESC],
[
  dnl Prerequisites of AC_LIB_LINKFLAGS_BODY.
  dnl AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  dnl AC_REQUIRE([AC_LIB_RPATH])
  AC_LANG_PUSH(C++)
  dnl rlc_XERCESC_BODY()

  ac_save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS -I/opt/local/include"

  AC_CACHE_CHECK([for libxerces-c], [ac_cv_libxerces_c], [
   ac_save_LDFLAGS="$LDFLAGS"
   LDFLAGS="$LDFLAGS -L/opt/local/lib -lxerces-c"
   AC_TRY_LINK([
#include <xercesc/util/PlatformUtils.hpp>
#ifdef XERCES_CPP_NAMESPACE_USE
XERCES_CPP_NAMESPACE_USE
#endif
	 ], [
try
{
   XMLPlatformUtils::Initialize();
}
catch (...)
{
   // ...
}
XMLPlatformUtils::Terminate();
	 ], [ac_cv_libxerces_c=yes], [ac_cv_libxerces_c=no])
    LIBS="$ac_save_LIBS"
  ])
  if test "$ac_cv_libxerces_c" = yes; then
    HAVE_LIBXERCES_C=yes
    AC_DEFINE(HAVE_LIBXERCES_C, 1, [Define if you have the xerces-c library.])
    AC_MSG_CHECKING([how to link with libxerces-c])
    AC_MSG_RESULT([$LIBXERCES_C])
  else
    HAVE_LIBXERCES_C=no
    CPPFLAGS="$ac_save_CPPFLAGS"
    LIBXERCES_C=
    LTLIBXERCES_C=
  fi
  
  AC_LANG_POP(C++)
  
  AC_SUBST(HAVE_LIBXERCES_C)
  AC_SUBST(LIBXERCES_C)
  AC_SUBST(LTLIBXERCES_C)
])


# Checks for programs.
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_CXX
AC_PROG_CC
AC_LANG(C++)

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_STDC
AC_CHECK_HEADERS([float.h inttypes.h libintl.h pwd.h malloc.h stddef.h stdlib.h string.h strings.h unistd.h values.h time.h])
AC_CHECK_HEADERS([sys/mman.h sys/param.h sys/resource.h sys/time.h sys/times.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([ctime getopt_long getsubopt memset pow rint strcasecmp strchr strdup strerror strrchr strftime])
AC_CHECK_FUNCS([geteuid getpwuid getrusage gettimeofday uname mmap])

AC_ARG_WITH([xerces],
  [AS_HELP_STRING([--with-xerces], [Use the Xerces XML parser instead of expat])],
  [],
  [with_xerces=no])

dnl LIBXERCES=
if test "x$with_xerces" = xyes; then
  rlc_XERCESC
fi

# Check for libexpat
if test "x$ac_cv_libxerces_c" != xyes; then
  AC_CHECK_HEADERS([expat.h])
  AC_CHECK_LIB([expat], [XML_ParserCreateNS])
fi

AM_CONDITIONAL([EXPAT], [test "x${ac_cv_lib_expat_XML_ParserCreateNS}" = "xyes"] )

# Produce the desired output files
AC_CONFIG_FILES([Makefile src/Makefile src/headers/lqio/Makefile])
AC_OUTPUT
