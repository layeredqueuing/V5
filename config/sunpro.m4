# AC_PROG_SUNPRO
# $Header$
# --------------
AC_DEFUN([AC_PROG_CC_SUNPRO],
[AC_CACHE_CHECK(whether CC is SUNPRO_C, ac_cv_prog_cc_sunpro,
[echo 'void f(){ int x = __SUNPRO_C; }' > conftest.c
if test -z "`${CC-cc} -Xt -c conftest.c 2>&1`"; then
  ac_cv_prog_cc_sunpro=yes
else
  ac_cv_prog_cc_sunpro=no
fi
rm -f conftest.*
])])
 	 
