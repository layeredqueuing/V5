# Process this file with autoconf to produce a configure script.
# $Id: configure.ac 14563 2021-03-18 02:07:31Z greg $
# ------------------------------------------------------------------------
AC_INIT([lqns],[5.20],[greg@sce.carleton.ca])
AC_CONFIG_AUX_DIR([config])
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
