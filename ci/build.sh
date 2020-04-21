#!/usr/bin/env bash


rm -rf CMakeFiles CmakeCache.txt
cmake -DSIST_DEBUG=off -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake .
make
strip sist2

rm -rf CMakeFiles CmakeCache.txt
cmake -DSIST_DEBUG=on -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake .
make
