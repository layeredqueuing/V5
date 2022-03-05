#!/bin/sh
dirs="lqx lqiolib libmva parasol"
echo "Out with the old..."
find . \( -name 'libtool' -o -name '*.cache' -o -name '.deps' -o -name config.in -o -name aclocal.m4 \) -print | xargs rm -rf
echo "In with the new..."
mkdir -p config
aclocal -I config
echo "Libtoolize..."
if test -x /opt/local/bin/glibtoolize; then glibtoolize --force
elif test -x /usr/bin/libtoolize; then libtoolize --force
else echo "Can't find libtoolize!"; exit
fi
echo "aclocal subdirs..."
for i in $dirs; do (cd $i; aclocal -I ../config); done
echo "automake subdirs..."
for i in $dirs; do (cd $i; if [ $i != "doc" ]; then autoheader; fi; automake -a -c; autoconf); done
echo "top level..."
autoheader
automake -a -c
autoconf
echo "done!"
echo "run configure; make; make install"
