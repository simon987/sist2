#!/usr/bin/env bash

export CC=gcc
export CXX=g++

rm -rf CMakeFiles CMakeCache.txt
cmake -DCMAKE_TOOLCHAIN_FILE=/usr/share/vcpkg/scripts/buildsystems/vcpkg.cmake . || exit
make -j 4
