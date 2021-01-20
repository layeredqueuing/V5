# Process this file with autoconf to produce a configure script.
# $Id: configure.ac 14381 2021-01-19 18:52:02Z greg $
# ------------------------------------------------------------------------
AC_INIT(lqns, 5.19, greg@sce.carleton.ca)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIRS[(config.h)]
AM_INIT_AUTOMAKE
LT_INIT
# ------------------------------------------------------------------------
AC_CHECK_FILE([/usr/src/redhat],
    [RPMDIR=/usr/src/redhat],
    AC_CHECK_FILE([$HOME/usr/rpm],
	[RPMDIR=$HOME/usr/rpm],
	AC_CHECK_FILE([$HOME/rpmbuild],[RPMDIR=$HOME/rpmbuild])))
AC_SUBST(RPMDIR)
# ------------------------------------------------------------------------
AC_CONFIG_SUBDIRS([lqiolib lqx libmva lqns lqsim lqn2ps lqngen qnsolver srvndiff parasol doc])
AC_PROG_MAKE_SET
AC_CONFIG_FILES([Makefile xml/Makefile dist/Makefile dist/examples/Makefile dist/win-nt/Makefile])
AC_OUTPUT
