# Process this file with autoconf to produce a configure script.
# ------------------------------------------------------------------------
# $Id$
# ------------------------------------------------------------------------
AC_INIT(petrisrvn, 5.8, greg@sce.carleton.ca)
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([petrisrvn.h])
AC_CONFIG_HEADERS([config.h])
#AC_CONFIG_SUBDIRS(wspnlib)
AM_INIT_AUTOMAKE
AC_PROG_MAKE_SET
AC_SUBST(STATIC)

# Checks for programs.
AC_PROG_CC
AC_LANG(C++)
AC_PROG_CXX
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
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([errno.h fcntl.h fenv.h ieeefp.h malloc.h stdlib.h string.h unistd.h sys/param.h sys/types.h sys/times.h sys/utsname.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL 
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK 
AC_FUNC_MALLOC
AC_FUNC_REALLOC 
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 feenableexcept feclearexcept floor getcwd getopt_long mkdir putenv regcomp sqrt strchr strdup strrchr uname])

AC_CONFIG_FILES([Makefile wspnlib/Makefile regression/Makefile])
AC_OUTPUT
