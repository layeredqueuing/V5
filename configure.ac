# Process this file with autoconf to produce a configure script.
# $Id: configure.ac 15446 2022-03-05 00:01:59Z greg $
# ------------------------------------------------------------------------
AC_INIT([lqns],[5.25],[greg@sce.carleton.ca])
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
AC_CHECK_PROGS(XSVN, svn)
AM_CONDITIONAL([HAVE_SVN], test -n "$XSVN")

AC_CHECK_PROGS(XFIG2DEV, fig2dev)
AM_CONDITIONAL([HAVE_FIG2DEV], test -n "$XFIG2DEV")

AC_ARG_ENABLE([latex-doc],
    [AS_HELP_STRING([--disable-latex-doc], [disable building the PDF
    documentation from LaTeX source])],
    [latexdoc=no],
    [latexdoc=yes])

if test "x$latexdoc" = "xyes"; then
   AC_MSG_NOTICE([building of the PDF of the user manual from LaTeX source is enabled])
   # Check for presence of pdfLaTeX
   AC_CHECK_PROGS(XLATEX, latex)
   if test -z "$XLATEX"; then
      AC_MSG_NOTICE([LaTeX not found - Unable to create PDF version of the user manual])
   fi
fi
AM_CONDITIONAL([HAVE_LATEX], test -n "$XLATEX")
AM_CONDITIONAL([BUILD_latexdoc], test "x$latexdoc" = "xyes")

# ----------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_SYS_WAIT
AC_TYPE_SIZE_T
AC_TYPE_MODE_T
AC_CHECK_HEADERS_ONCE([sys/time.h])
AC_STRUCT_TM

# ----------------------------------------------------------------------
# Checks for header files.
#
AC_CHECK_HEADERS([errno.h fenv.h float.h getopt.h glob.h ieeefp.h libgen.h mcheck.h strings.h time.h unistd.h])
AC_CHECK_HEADERS([sys/errno.h sys/mman.h sys/resource.h sys/times.h sys/utsname.h])
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
# Checks for library functions.
AC_FUNC_MMAP
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([ctime drand48 getopt_long getrusage getsubopt glob mcheck uname])
AC_CHECK_FUNCS([feclearexcept feenableexcept fesetexceptflag fetestexcept fpgetsticky fpsetmask fpsetsticky])
AC_CHECK_FUNCS([lgamma])
if test "x$enable_shared" = "xyes"; then
   AC_CHECK_FUNCS([gdFTUseFontConfig gdImageFilledArc gdImageSetAntiAliased gdImageGifPtr])
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
AC_CONFIG_SUBDIRS([lqiolib lqx libmva parasol])
AC_CONFIG_FILES([Makefile lqns/Makefile lqns/regression/Makefile qnsolver/Makefile])
AC_CONFIG_FILES([lqn2csv/Makefile lqngen/Makefile lqn2ps/Makefile srvndiff/Makefile])
AC_CONFIG_FILES([lqsim/Makefile lqsim/regression/Makefile])
AC_CONFIG_FILES([petrisrvn/Makefile petrisrvn/wspnlib/Makefile petrisrvn/regression/Makefile])
AC_CONFIG_FILES([doc/Makefile doc/tutorial/Makefile doc/userman/Makefile])
AC_CONFIG_FILES([doc/tutorial/activity-templ/Makefile doc/tutorial/calls/Makefile doc/tutorial/model/Makefile doc/tutorial/pseudo-tasks/Makefile doc/tutorial/web-server/Makefile])
AC_CONFIG_FILES([doc/userman/bookstore/Makefile doc/userman/model/Makefile doc/userman/replication/Makefile doc/userman/srvn-schema/Makefile doc/userman/timing-diagrams/Makefile doc/userman/xml-schema/Makefile])
AC_CONFIG_FILES([xml/Makefile dist/Makefile dist/examples/Makefile dist/win-nt/Makefile])
AC_OUTPUT
