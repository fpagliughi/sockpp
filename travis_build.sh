#!/bin/bash

set -e

rm -rf build
mkdir build
cd build
echo "travis build dir $TRAVIS_BUILD_DIR pwd $PWD"
cmake -DSOCKPP_BUILD_DOCUMENTATION=OFF -DSOCKPP_BUILD_EXAMPLES=ON ..
make
#cpack --verbose
kill %1

