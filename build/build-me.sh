#!/bin/sh
#< build-me.sh for fgms-0-x
BN=`basename $0`

BSERV2=0

CMOPTS=""
MKOPTS=""

CMOPTS="$CMOPTS -DCMAKE_VERBOSE_MAKEFILE=TRUE"
if [ "$BSERV2" = "1" ]; then
    CMOPTS="$CMOPTS -DBUILD_SERVER2:BOOL=TRUE"
fi

echo "Doing: 'cmake .. $CMOPTS'"
cmake .. $CMOPTS

if [ ! "$?" = "0" ]; then
    echo "$BN: Appears a cmake configuration or generation problem!"
    exit 1
else
    echo "$BN: cmake configuration and generation appears ok."
fi

echo "$BN: Doing 'make $MKOPTS'"
make $MKOPTS

if [ ! "$?" = "0" ]; then
    echo "$BN: Appears a make problem!"
    exit 1
else
    echo "$BN: Appears to be a successful make."
fi

# eof
