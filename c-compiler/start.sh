#!/bin/sh

cd "$(dirname "$0")"
mkdir -p ./build
cd ./build
if [ "$1" = "c" ] ; then  # recompile everything
        rm -rf CMakeFiles
        rm CMakeCache.txt
        cmake -D CMAKE_BUILD_TYPE=Debug ..
        make
    exit
fi

set -e
make
set +e
cd ..
rm coredump.* > /dev/null 2>&1
echo "-------------------------------------------------"
set -e


if [ "$1" = "perf" ] ; then  # perf test
    rm callgrind.* > /dev/null 2>&1
    shift 1
    valgrind --tool=callgrind --collect-systime=usec ./build/compiler "$@"
    qcachegrind
elif [ "$1" = "mem" ] ; then # memory test
    shift 1
    valgrind ./build/compiler "$@"
else
    ./build/compiler "$@"
fi
