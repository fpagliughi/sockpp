#!/bin/bash
#
# travis_build.sh
#
# Travis CI build/test script for the sockpp library.
#

set -e

# Install Catch2 from sources
pushd /tmp
wget https://github.com/catchorg/Catch2/archive/v2.5.0.tar.gz
tar -xf v2.5.0.tar.gz
cd Catch2-2.5.0/
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build/ --target install
popd

rm -rf build
mkdir build
cd build
echo "travis build dir $TRAVIS_BUILD_DIR pwd $PWD"
cmake -DSOCKPP_BUILD_EXAMPLES=ON -DSOCKPP_BUILD_TESTS=ON .. && \
  make && \
  ./tests/unit/unit_tests --success

#ctest -VV --timeout 600
#cpack --verbose


