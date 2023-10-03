#!/bin/sh

cd ./build
if [ "$1" == "c" ] ; then
	rm -rf CMakeFiles
	rm CMakeCache.txt
	cmake -D CMAKE_BUILD_TYPE=Debug ..
	make
elif [ "$1" == "p" ] ; then
    set -e
	make
    set +e
	cd ..
    echo "-------------------------------------------------"
    set -e
    valgrind --tool=callgrind --collect-systime=usec ./build/compiler $2
    qcachegrind
    set +e
else
    set -e
	make
    set +e
	cd ..
    rm coredump.* &> /dev/null # rm coredump.* > /dev/null 2>&1
    echo "-------------------------------------------------"
    set -e
	./build/compiler $@
    set +e
fi
