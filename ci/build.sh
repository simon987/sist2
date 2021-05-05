#!/usr/bin/env bash

VCPKG_ROOT="/vcpkg"

rm *.gz

git submodule update --init --recursive

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=off -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j 33
strip sist2
./sist2 -v > VERSION
cp sist2 Docker/
mv sist2 sist2-x64-linux

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=on -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j 33
cp /usr/lib/x86_64-linux-gnu/libasan.so.2.0.0 libasan.so.2
mv sist2_debug sist2-x64-linux-debug
tar -czf sist2-x64-linux-debug.tar.gz sist2-x64-linux-debug libasan.so.2
