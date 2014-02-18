#!/bin/sh
rm -f configure
libtoolize
aclocal
autoconf
touch AUTHORS NEWS README ChangeLog
automake --add-missing

