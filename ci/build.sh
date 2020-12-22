#!/usr/bin/env bash

VCPKG_ROOT="/vcpkg"

rm *.gz

git submodule update --init --recursive

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=off -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j 33
strip sist2
gzip -9 sist2

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=on -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j 33
cp /usr/lib/x86_64-linux-gnu/libasan.so.2.0.0 libasan.so.2
tar -czf sist2_debug.tar.gz sist2_debug libasan.so.2
