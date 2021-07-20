# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqn2ps/configure.ac $
# ------------------------------------------------------------------------
# $Id: configure.ac 14929 2021-07-20 11:56:23Z greg $
# ------------------------------------------------------------------------
AC_PREREQ([2.69])
AC_INIT([lqn2ps],[5.21],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([../config])
AC_CONFIG_SRCDIR([lqn2ps.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_SUBST(STATIC)

# ----------------------------------------------------------------------
# Compiler and linking.
#
AC_LANG(C++)
AC_PROG_CXX
# Add in libtool.
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
AC_CHECK_PROGS(XPS2PDF, ps2pdf)
if test -z "$XPS2PDF"; then
  AC_MSG_NOTICE([Ps2pdf not found - Unable to create PDF version of the user manual])
fi
AM_CONDITIONAL([HAVE_PS2PDF], test -n "$XPS2PDF")

# ----------------------------------------------------------------------
# Checks for libraries.
#
AC_CHECK_LIB([m], [sqrt])
if test "x$enable_shared" == "xyes"; then
   AC_CHECK_LIB([gd], [gdImageCreate])
   AC_CHECK_LIB([z], [gzopen])
   AC_CHECK_LIB([jpeg], [jpeg_write_raw_data])
   AC_CHECK_LIB([png], [png_write_image])
   AC_CHECK_LIB([fontconfig], [FcConfigSubstitute])
fi

# ----------------------------------------------------------------------
# Checks for header files.
#
AC_CHECK_HEADERS([float.h ieeefp.h])
if test "x$enable_shared" = "xyes"; then
   AC_CHECK_HEADERS([gd.h])
fi
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h values.h])
AC_CHECK_HEADERS([getopt.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_FUNC_STRTOD
AC_TYPE_SIZE_T

# ----------------------------------------------------------------------
# Checks for library functions.
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([basename ctime getsubopt])
AC_CHECK_FUNCS([pow sqrt])
AC_CHECK_FUNCS([getuid])
if test "x$enable_shared" = "xyes"; then
   AC_CHECK_FUNCS([gdImageFilledArc gdImageGifPtr gdImageSetAntiAliased gdFTUseFontConfig])
fi
AC_CHECK_FUNCS([getopt_long])
AC_CHECK_FUNCS([mkdir rmdir])
AC_CHECK_FUNCS([strchr strdup strerror strrchr strtol])
#
# Final pass for jpeg/png/gd. 
#
# echo "ac_cv_lib_png_png_write_image is ${ac_cv_lib_png_png_write_image}"
# echo "ac_cv_lib_gd_gdMalloc is ${ac_cv_lib_gd_gdMalloc}"
AM_CONDITIONAL([COND_PNG], [test "${ac_cv_lib_png_png_write_image}" = "yes" -a "${ac_cv_lib_gd_gdMalloc}" = "yes" ])
AM_CONDITIONAL([COND_GIF], [test "${ac_cv_func_gdImageGifPtr}" = "yes" ])
AM_CONDITIONAL([COND_JPEG], [test "${ac_cv_lib_jpeg_jpeg_write_raw_data}" = "yes" -a "${ac_cv_lib_gd_gdMalloc}" = "yes" ])
if test "${ac_cv_lib_gd_gdMalloc}" = "no"; then
   LIBS=`echo $LIBS | sed -e "s/-lpng//" -e "s/-ljpeg//" -e "s/-lz//"`
fi
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
