#!/usr/bin/env bash

VCPKG_ROOT="/vcpkg"

rm *.gz

git submodule update --init --recursive

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=off -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j $(nproc)
strip sist2
./sist2 -v > VERSION
cp sist2 Docker/
mv sist2 sist2-x64-linux

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_DEBUG=on -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j  $(nproc)
mv sist2_debug sist2-x64-linux-debug