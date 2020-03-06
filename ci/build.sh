#!/usr/bin/env bash

./scripts/get_static_libs.sh

rm -rf CMakeFiles CmakeCache.txt
cmake -DSIST_DEBUG=off .
make
strip sist2

rm -rf CMakeFiles CmakeCache.txt
cmake -DSIST_DEBUG=on .
make
