#!/bin/sh

OSTYPE=`uname -s`
MACHINE=`uname -m`
AUTO_MAKE_VERSION=`automake --version | head -1 | awk '{print $4}' | sed -e 's/\.\([0-9]*\).*/\1/'`

echo "Host info: $OSTYPE $MACHINE"
echo -n " automake: `automake --version | head -1 | awk '{print $4}'`"
echo " ($AUTO_MAKE_VERSION)"
echo ""

echo "Running aclocal"
aclocal
if [ $? != 0 ]; then
        echo 
        echo "#"
        echo "failed to call aclocal"
        echo "You need autoconf and automake to build the sources"
        echo "#"
        echo 
        exit
fi

echo "Running libtoolize"
libtoolize --force --copy
if [ $? != 0 ]; then
        echo 
        echo "#"
        echo "failed to call libtoolize"
        echo "You need libtool to build the sources"
        echo "#"
        echo 
        exit
fi

echo "Running autoheader"
autoheader
if [ ! -e config.h.in ]; then
        echo "ERROR: autoheader didn't create simgear/simgear_config.h.in!"
        exit 1
fi

echo "Running automake --add-missing"
automake --add-missing

echo "Running autoconf"
autoconf
if [ ! -e configure ]; then
        echo "ERROR: configure was not created!"
        exit 1
fi

echo ""
echo "======================================"
if [ -f config.cache ]; then
        echo "config.cache exists.  Removing the config.cache file will force"
        echo "the ./configure script to rerun all it's tests rather than using"
        echo "the previously cached values."
        echo ""
fi

echo "Now you are ready to run './configure'"
echo "======================================"


