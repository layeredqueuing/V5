# Process this file with autoconf to produce a configure script.
#
# ------------------------------------------------------------------------
# $Id: configure.ac 14878 2021-07-06 19:07:54Z greg $
# ------------------------------------------------------------------------
AC_INIT([lqns],[5.21],[greg@sce.carleton.ca])
AM_INIT_AUTOMAKE
# The user can disable building of the PDF of the manual, for example
# if the required LaTeX packages are not installed
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
AC_CONFIG_FILES([Makefile tutorial/Makefile tutorial/model/Makefile tutorial/pseudo-tasks/Makefile tutorial/calls/Makefile tutorial/activity-templ/Makefile tutorial/web-server/Makefile userman/Makefile userman/model/Makefile userman/bookstore/Makefile userman/srvn-schema/Makefile userman/timing-diagrams/Makefile userman/replication/Makefile userman/xml-schema/Makefile])
AC_OUTPUT

