#!/bin/sh
#
# Test the build with a few compilers.
#

for COMPILER in g++-4.9 g++-5 g++-6 clang++-3.8
do
    if [ -z "$(which ${COMPILER})" ]; then
	printf "Compiler not found: %s\n" "${COMPILER}"
    else
	printf "Testing: %s\n" "${COMPILER}"
	make distclean
	if ! make CXX=${COMPILER} ; then
	    printf "\nCompilation failed for %s\n" "${COMPILER}"
	    exit 1
	fi
    fi
    printf "\n"
done

make distclean
exit 0


