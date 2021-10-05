# Process this file with autoconf to produce a configure script.
# ------------------------------------------------------------------------
# $Id: configure.ac 15042 2021-10-05 14:11:14Z greg $
# ------------------------------------------------------------------------
AC_INIT([srvndiff],[5.22],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([lqn2csv.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_SUBST(STATIC)

# ----------------------------------------------------------------------
# Compiler and linking.
#
LT_INIT
AC_PROG_CXX
AC_LANG(C++)
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
AC_CHECK_PROGS(XPS2PDF, ps2pdf)
if test -z "$XPS2PDF"; then
  AC_MSG_NOTICE([Ps2pdf not found - Unable to create PDF version of the user manual])
fi
AM_CONDITIONAL([HAVE_PS2PDF], test -n "$XPS2PDF")

# ----------------------------------------------------------------------
# Checks for libraries.
# ----------------------------------------------------------------------
# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_TYPE_SIZE_T
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([glob.h])
AC_CHECK_HEADERS([libgen.h]) 
AC_CHECK_HEADERS([unistd.h])

# ----------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.

# ----------------------------------------------------------------------
# Checks for library functions.
AC_CHECK_FUNCS([getopt_long])
AC_CHECK_FUNCS([basename])
AC_CHECK_FUNCS([glob])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
