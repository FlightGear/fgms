#!/bin/bash

# Script to compile the docs, requires doxygen 1.8+
## see http://stackoverflow.com/questions/11032280/specify-doxygen-parameters-through-command-line

## TODO Need to override the OUTPUT_DIRECTORY
##      its currently set to build_docs/


VER=`cat version`


( cat  docs/doxy.conf; \
  echo "PROJECT_NUMBER=$VER" \
  ) | doxygen -
