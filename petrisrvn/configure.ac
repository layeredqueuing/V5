# Process this file with autoconf to produce a configure script.
# ------------------------------------------------------------------------
# $Id: configure.ac 14563 2021-03-18 02:07:31Z greg $
# ------------------------------------------------------------------------
AC_INIT([petrisrvn],[5.20],[greg@sce.carleton.ca])
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
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])

# Checks for header files.
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([errno.h fcntl.h fenv.h ieeefp.h malloc.h stdlib.h string.h unistd.h sys/param.h sys/types.h sys/times.h sys/utsname.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL 
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK 
m4_warn([obsolete],
[your code may safely assume C89 semantics that RETSIGTYPE is void.
Remove this warning and the `AC_CACHE_CHECK' when you adjust the code.])dnl
AC_CACHE_CHECK([return type of signal handlers],[ac_cv_type_signal],[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
],
		 [return *(signal (0, 0)) (0) == 1;])],
		   [ac_cv_type_signal=int],
		   [ac_cv_type_signal=void])])
AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],[Define as the return type of signal handlers
		    (`int' or `void').])

AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 feenableexcept feclearexcept floor getcwd getopt_long mkdir putenv regcomp sqrt strchr strdup strrchr uname])

AC_CONFIG_FILES([Makefile wspnlib/Makefile regression/Makefile])
AC_OUTPUT
