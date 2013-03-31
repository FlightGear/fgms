#!/bin/bash

# Script to compile the docs, requires doxygen 1.8+

## TODO Need to override the OUTPUT_DIRECTORY
##      its currently set to build_docs/


VER=`cat version`


( cat  docs/doxy.conf; \
  echo "PROJECT_NUMBER=$VER" \
  #echo "OUTPUT_DIRECTORY=$OUT" \
  ) | doxygen -
