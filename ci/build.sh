#!/usr/bin/env bash

VCPKG_ROOT="/usr/share/vcpkg"

rm *.gz

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=off -DVCPKG_BUILD_TYPE=release -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j 12
strip sist2
gzip -9 sist2

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=on -DVCPKG_BUILD_TYPE=debug -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j 12
cp /usr/lib/x86_64-linux-gnu/libasan.so.2.0.0 libasan.so.2
tar -czf sist2_debug.tar.gz sist2_debug libasan.so.2
