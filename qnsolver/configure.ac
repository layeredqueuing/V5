# Process this file with autoconf to produce a configure script.
# ------------------------------------------------------------------------
# $Id: configure.ac 15155 2021-12-06 18:54:53Z greg $
# ------------------------------------------------------------------------
AC_INIT([lqns],[5.22],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([closedmodel.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_MAKE_SET
AC_SUBST(STATIC)

# ----------------------------------------------------------------------
#
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
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_CHECK_PROGS(XPS2PDF, ps2pdf)
if test -z "$XPS2PDF"; then
  AC_MSG_NOTICE([Ps2pdf not found - Unable to create PDF version of the user manual])
fi
AM_CONDITIONAL([HAVE_PS2PDF], test -n "$XPS2PDF")
# ----------------------------------------------------------------------
# Checks for libraries.
#
AC_CHECK_LIB([m], [sqrt])
# ----------------------------------------------------------------------
# Checks for header files.
#
AC_HEADER_SYS_WAIT
AC_TYPE_SIZE_T
AC_CHECK_HEADERS([fenv.h float.h getopt.h ieeefp.h unistd.h values.h sys/resource.h sys/utsname.h])
# ----------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_STAT
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_C_BIGENDIAN
AC_C_INLINE
AC_HEADER_STDBOOL

# ----------------------------------------------------------------------
# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([pow sqrt])
AC_CHECK_FUNCS([getopt_long getsubopt])
AC_CONFIG_FILES([Makefile])
AC_CHECK_HEADERS([expat.h])
AC_CHECK_LIB([expat], [XML_ParserCreateNS])
AM_CONDITIONAL([EXPAT], [test "x${ac_cv_lib_expat_XML_ParserCreateNS}" = "xyes"] )

AC_OUTPUT
