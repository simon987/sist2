#!/usr/bin/env bash

VCPKG_ROOT="/vcpkg"

git submodule update --init --recursive

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_PLATFORM=arm64_linux -DSIST_DEBUG=off -DBUILD_TESTS=off -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j $(nproc)
strip sist2
mv sist2 sist2-arm64-linux

rm -rf CMakeFiles CMakeCache.txt
cmake -DSIST_PLATFORM=arm64_linux -DSIST_DEBUG=on -DBUILD_TESTS=off -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" .
make -j $(nproc)
strip sist2
mv sist2 sist2-arm64-linux-debug