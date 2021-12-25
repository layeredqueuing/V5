# Process this file with autoconf to produce a configure script.
# $Id: configure.ac 15256 2021-12-25 01:47:40Z greg $
# ------------------------------------------------------------------------
AC_INIT([lqns],[5.23],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIRS([config])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_MAKE_SET
AC_SUBST(STATIC)

# ------------------------------------------------------------------------
AC_CHECK_FILE([/usr/src/redhat],
    [RPMDIR=/usr/src/redhat],
    AC_CHECK_FILE([$HOME/usr/rpm],
	[RPMDIR=$HOME/usr/rpm],
	AC_CHECK_FILE([$HOME/rpmbuild],[RPMDIR=$HOME/rpmbuild])))
AC_SUBST(RPMDIR)
# ----------------------------------------------------------------------
#
AC_LANG(C++)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LEX([noyywrap])
AC_PROG_YACC

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
# Checks for header files.
#
AC_HEADER_SYS_WAIT
AC_TYPE_SIZE_T
AC_CHECK_HEADERS([glob.h errno.h getopt.h unistd.h libgen.h])
AC_CHECK_HEADERS([sys/resource.h sys/utsname.h sys/times.h])
AC_CHECK_HEADERS([fenv.h float.h ieeefp.h])
AC_CHECK_HEADERS([expat.h])
AC_CHECK_HEADERS([gd.h])

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
quorum=no;
AC_ARG_WITH(quorum,
	AS_HELP_STRING([--with-quorum],[Provided the GSL libraries are present, compile with support for quorum join. [default=no]]),
        [
	    quorum="$withval"
        ])
if test "$quroum" = "yes"; then
     AC_CHECK_HEADERS([gsl/gsl_cdf.h gsl/gsl_math.h])
     AC_CHECK_LIB([gslcblas], [cblas_dsdot])
     AC_CHECK_LIB([gsl], [gsl_cdf_gamma_P])
fi

#AC_CHECK_LIB([pthread], [pthread_create])
AM_CONDITIONAL([QUORUM], [test ${ac_cv_lib_gsl_gsl_cdf_gamma_P} = "yes"] )

AC_CHECK_LIB([expat], [XML_ParserCreateNS])
AM_CONDITIONAL([EXPAT], [test "x${ac_cv_lib_expat_XML_ParserCreateNS}" = "xyes"] )

# ----------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
#

AC_TYPE_MODE_T
AC_CHECK_HEADERS_ONCE([sys/time.h])
AC_STRUCT_TM

# ----------------------------------------------------------------------
# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_SELECT_ARGTYPES 
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_STRUCT_TM
AC_CHECK_FUNCS([ctime getopt_long getsubopt getrusage glob])
AC_CHECK_FUNCS([lgamma])
AC_CHECK_FUNCS([drand48])
if test "x$enable_shared" = "xyes"; then
   AC_CHECK_FUNCS([gdImageFilledArc gdImageGifPtr gdImageSetAntiAliased gdFTUseFontConfig])
fi
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

# ------------------------------------------------------------------------
AC_CONFIG_SUBDIRS([lqiolib lqx libmva parasol lqsim doc])
AC_PROG_MAKE_SET
AC_CONFIG_FILES([Makefile lqns/Makefile lqns/regression/Makefile qnsolver/Makefile])
AC_CONFIG_FILES([lqn2csv/Makefile lqngen/Makefile lqn2ps/Makefile srvndiff/Makefile])
AC_CONFIG_FILES([petrisrvn/Makefile petrisrvn/wspnlib/Makefile petrisrvn/regression/Makefile])
AC_CONFIG_FILES([xml/Makefile dist/Makefile dist/examples/Makefile dist/win-nt/Makefile])
AC_OUTPUT
