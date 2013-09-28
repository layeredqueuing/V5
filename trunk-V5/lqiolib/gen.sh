# Prepare all of the good stuff
echo ">>> Running: aclocal"
aclocal -I ../config
echo ">>> Running: glibtoolize and/or libtoolize (Ignore Errors)"
glibtoolize --force
libtoolize --force
echo ">>> Running: autoheader"
autoheader
echo ">>> Running: autoconf"
autoconf
echo ">>> Running: automake --add-missing"
automake -a -c
