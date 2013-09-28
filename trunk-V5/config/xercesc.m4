# -*- autoconf -*-
# $Id$

# --------------------- #
# Checks for Xerces.    #
# --------------------- #


# _AC_PATH_XERCES_DIRECT
# -----------------
# Internal subroutine of _AC_PATH_XERCES.
# Set ac_xerces_includes and/or ac_xerces_libraries.
# Note that the library check will attempt link with first one found, so order is important here.
m4_define([_AC_PATH_XERCES_DIRECT],
[# Standard set of common directories for XERCES headers.  
ac_xerces_header_dirs='
/usr/include
/usr/local/include
/opt/local/include'
if test "$ac_xerces_includes" = no; then
  # Guess where to find include files, by looking for Intrinsic.h.
  # First, try using that file with no special directory specified.
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([@%:@include <xercesc/parsers/XercesDOMParser.hpp>])],
[# We can compile using XERCES headers with no special include directory.
ac_xerces_includes=],
[for ac_dir in $ac_xerces_header_dirs; do
  if test -r "$ac_dir/xercesc/parsers/XercesDOMParser.hpp"; then
    ac_xerces_includes=$ac_dir
    break
  fi
done])
fi # $ac_xerces_includes = no

if test "$ac_xerces_libraries" = no; then
  # Check for the libraries.
  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS=$LIBS
  AC_LANG_PUSH([C++])
  LIBS="-lxerces-c $LIBS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
	[
	@%:@include <xercesc/parsers/XercesDOMParser.hpp>
	@%:@include <iostream>
	XERCES_CPP_NAMESPACE_USE
  	],
	[
	XercesDOMParser* parser = new XercesDOMParser();	
	])],
                 [LIBS=$ac_save_LIBS
# We can link XERCES programs with no special library path.
ac_xerces_libraries=],
                 [LIBS=$ac_save_LIBS
for ac_dir in `echo "$ac_xerces_includes $ac_xerces_header_dirs" | sed s/include/lib/g`
do
  # Don't even attempt the hair of trying to link an XERCES program!
  for ac_extension in a so sl dll; do
    if test -r $ac_dir/libxerces-c.$ac_extension; then
      if test ${enable_static+set} = "set"; then
        $ac_dir/libxerces-c.$ac_extension
      else
        ac_xerces_libraries=$ac_dir
      fi
      break 2
    fi
  done
done])
  AC_LANG_POP
fi # $ac_xerces_libraries = no
])# _AC_PATH_XERCES_DIRECT


# _AC_PATH_XERCES
# ----------
# Compute ac_cv_have_xerces.
AC_DEFUN([_AC_PATH_XERCES],
[AC_CACHE_VAL(ac_cv_have_xerces,
[# One or both of the vars are not set, and there is no cached value.
ac_xerces_includes=no ac_xerces_libraries=no
_AC_PATH_XERCES_DIRECT
if test "$ac_xerces_includes" = no || test "$ac_xerces_libraries" = no; then
  # Didn't find XERCES anywhere.  Cache the known absence of XERCES.
  ac_cv_have_xerces="have_xerces=no"
else
  # Record where we found XERCES for the cache.
  ac_cv_have_xerces="have_xerces=yes \
	        ac_xerces_includes=$ac_xerces_includes ac_xerces_libraries=$ac_xerces_libraries"
fi])dnl
])


# AC_PATH_XERCES
# ---------
# If we find XERCES, set shell vars xerces_includes and xerces_libraries to the
# paths, otherwise set no_xerces=yes.
# Uses ac_ vars as temps to allow command line to override cache and checks.
# --without-xerces overrides everything else, but does not touch the cache.
AN_HEADER([xercesc/parsers/XercesDOMParser.hpp],  [AC_PATH_XERCES])
AC_DEFUN([AC_PATH_XERCES],
[dnl Document the XERCES abnormal options inherited from history.
m4_divert_once([HELP_BEGIN], [
XERCES features:
  --x-includes=DIR    XERCES include files are in DIR
  --x-libraries=DIR   XERCES library files are in DIR])dnl
AC_MSG_CHECKING([for XERCES])
xerces_includes="$x_includes"
xerces_libraries="$x_libraries"

AC_ARG_WITH(xerces, [  --with-xerces                use the XERCES XML Parser])
# $have_xerces is `yes', `no', `disabled', or empty when we do not yet know.
if test "x$with_xerces" = xno; then
  # The user explicitly disabled XERCES.
  have_xerces=disabled
else
  if test "x$xerces_includes" != xNONE && test "x$xerces_libraries" != xNONE; then
    # Both variables are already set.
    have_xerces=yes
  else
    _AC_PATH_XERCES
  fi
  eval "$ac_cv_have_xerces"
fi # $with_xerces != no

if test "$have_xerces" != yes; then
  AC_MSG_RESULT([$have_xerces])
  no_xerces=yes
else
  # If each of the values was on the command line, it overrides each guess.
  test "x$xerces_includes" = xNONE && xerces_includes=$ac_xerces_includes
  test "x$xerces_libraries" = xNONE && xerces_libraries=$ac_xerces_libraries
  # Update the cache value to reflect the command line values.
  ac_cv_have_xerces="have_xerces=yes \
		ac_xerces_includes=$xerces_includes ac_xerces_libraries=$xerces_libraries"
  AC_MSG_RESULT([libraries $xerces_libraries, headers $xerces_includes])
fi

if test "$no_xerces" = yes; then
  # Not all programs may use this symbol, but it does not hurt to define it.
  XERCES_CFLAGS= XERCES_LIBS= XERCES_EXTRA_LIBS=
else
  AC_DEFINE([HAVE_LIBXERCES], 1,
            [Define to 1 if the XERCES XML parser is being used.])
  if test -n "$xerces_includes"; then
    XERCES_CFLAGS="$XERCES_CFLAGS -I$xerces_includes"
  fi

  # It would also be nice to do this for all -L options, not just this one.
  if test -n "$xerces_libraries"; then
    XERCES_LIBS="$XERCES_LIBS -L$xerces_libraries"
  fi
  # always tack this on...
  XERCES_LIBS="$XERCES_LIBS -lxerces-c"

#  XERCES_EXTRA_LIBS="$XERCES_EXTRA_LIBS -lpthread"
fi
AC_SUBST(XERCES_CFLAGS)dnl
AC_SUBST(XERCES_LIBS)dnl
AC_SUBST(XERCES_EXTRA_LIBS)dnl
])# AC_PATH_XERCES
