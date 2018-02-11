#!/bin/sh

# install dependencies
# debian like:
#    sudo apt-get install -y build-essential libboost-all-dev

set -x -e

BUILD_DIR=./build-linux
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -DBoost_USE_STATIC_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE="Release" ..
make
