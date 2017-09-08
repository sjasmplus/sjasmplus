#!/bin/sh

set -x -e

BUILD_DIR=./build-macos
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -DBoost_USE_STATIC_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE="Release" ..
make
