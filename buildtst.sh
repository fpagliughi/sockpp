#!/bin/bash
#
# Test the build with a few compilers.
#
# Local script for continuous integration (CI). This just runs the build 
# using a number of different compilers, then runs the unit tests for 
# each. If the build or unit tests fail, it stops and reports the error.

# Add or remove any compilers here. If any are not found on the local system,
# it reports the missing compiler and continues to the next one.

GCC="g++-9 g++-12 g++-15"
CLANG="clang++-17 clang++-19 clang++-22"

for COMPILER in ${GCC} ${CLANG} ; do
  if [ -z "$(which ${COMPILER})" ]; then
    printf "Compiler not found: %s\n" "${COMPILER}"
  else
    printf "Testing: %s\n" "${COMPILER}"
    rm -rf buildtst/
    mkdir buildtst
    pushd buildtst

    # Configure the build
    if ! cmake -DSOCKPP_BUILD_EXAMPLES=ON -DSOCKPP_BUILD_TESTS=ON -DCMAKE_CXX_COMPILER=${COMPILER} .. ; then
      printf "\nCMake configuration failed for %s\n" "${COMPILER}"
      exit 1
    fi

    # Build the library, examples, and unit tests
    if ! make -j ; then
      printf "\nCompilation failed for %s\n" "${COMPILER}"
      exit 2
    fi

    # Run the unit tests
    if ! ./tests/unit/unit_tests ; then
      printf "\nUnit tests failed for %s\n" "${COMPILER}"
      exit 3
    fi

    popd
    rm -rf buildtst/
  fi
  printf "\n"
done

printf "\nAll builds passed\n"
exit 0


