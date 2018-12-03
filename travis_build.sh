#!/bin/bash
#
# travis_build.sh
#
# Travis CI build/test script for the sockpp library.
#

set -e

rm -rf build
mkdir build
cd build
echo "travis build dir $TRAVIS_BUILD_DIR pwd $PWD"
cmake -DSOCKPP_BUILD_EXAMPLES=ON -DSOCKPP_BUILD_TESTS .. && \
  make && \
  ./tests/unit/unit_tests --success

#ctest -VV --timeout 600
#cpack --verbose


