#!/usr/bin/env bash

VCPKG_ROOT="/vcpkg"

rm *.gz

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=off -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j 4
strip sist2
mv sist2 sist2_arm64
gzip -9 sist2_arm64