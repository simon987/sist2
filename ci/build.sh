#!/usr/bin/env bash

git clone --recursive https://github.com/simon987/sist2
cd sist2
./scripts/get_static_libs.sh 1

cmake .
make