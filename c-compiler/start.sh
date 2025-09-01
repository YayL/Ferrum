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
rm coredump.* &> /dev/null
echo "-------------------------------------------------"
set -e

if [ "$1" = "perf" ] ; then  # perf test
    shift 1
    rm -f callgrind.* &> /dev/null # rm callgrind.* > /dev/null 2>&1
    echo "1"
    valgrind --tool=callgrind --collect-systime=usec --dump-instr=yes ./build/compiler "$@"
    echo "2"
    qcachegrind
elif [ "$1" = "mem" ] ; then # memory test
    shift 1
    if [ "$1" = "leak" ]; then
        shift 1
        valgrind --leak-check=full --show-leak-kinds=definite ./build/compiler "$@"
    else
        valgrind --track-origins=yes ./build/compiler "$@"
    fi
else
    ./build/compiler "$@"
fi

