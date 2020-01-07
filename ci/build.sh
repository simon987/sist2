#!/usr/bin/env bash

./scripts/get_static_libs.sh

cmake .
make
strip sist2
strip sist2_scan
