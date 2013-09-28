# Process this file with autoconf to produce a configure script.
# ------------------------------------------------------------------------
# $Id$
# ------------------------------------------------------------------------
AC_INIT(srvndiff, 5.7, greg@sce.carleton.ca)
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([srvndiff.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_SUBST(STATIC)

# ----------------------------------------------------------------------
# Compiler and linking.
#
AC_PROG_LIBTOOL
LT_INIT
AC_PROG_CXX
AC_PROG_CC

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
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_YACC
AM_PROG_LEX

# ----------------------------------------------------------------------
# Checks for libraries.
AC_CHECK_LIB([expat], [XML_ParserCreateNS])
AC_CHECK_LIB([m], [sqrt])
AC_SEARCH_LIBS([regcomp], [regex] )
# ----------------------------------------------------------------------
# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_TYPE_SIZE_T
AC_CHECK_HEADERS([fenv.h]) 
AC_CHECK_HEADERS([float.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([inttypes.h]) 
AC_CHECK_HEADERS([libgen.h]) 
AC_CHECK_HEADERS([limits.h]) 
AC_CHECK_HEADERS([malloc.h]) 
AC_CHECK_HEADERS([regex.h])
AC_CHECK_HEADERS([stddef.h]) 
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([sys/ioctl.h]) 
AC_CHECK_HEADERS([sys/errno.h])
AC_CHECK_HEADERS([sys/param.h]) 
AC_CHECK_HEADERS([sys/time.h]) 
AC_CHECK_HEADERS([sys/uio.h]) 
AC_CHECK_HEADERS([sys/uts_name.h]) 
AC_CHECK_HEADERS([termios.h]) 
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([values.h]) 
AC_CHECK_HEADERS([libintl.h])

# ----------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_HEADER_STDBOOL
AC_HEADER_DIRENT 
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_FUNC_ALLOCA
AC_DECL_SYS_SIGLIST
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_DECLS([sys_siglist])
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPES([ptrdiff_t])

# ----------------------------------------------------------------------
# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID 
AC_FUNC_ERROR_AT_LINE
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP 
AC_FUNC_REALLOC 
AC_FUNC_SELECT_ARGTYPES 
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_STRUCT_TM
AC_CHECK_FUNCS([floor])
AC_CHECK_FUNCS([getopt_long])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([strcasecmp])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
