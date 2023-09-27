#!/bin/sh

cd ./build
if [ "$1" == "c" ] ; then
	rm -rf CMakeFiles
	rm CMakeCache.txt
	cmake -D CMAKE_BUILD_TYPE=Debug ..
	make
else
    set -e
	make
    set +e
	cd ..
    rm coredump.* &> /dev/null # rm coredump.* > /dev/null 2>&1
    echo "-------------------------------------------------"
    set -e
	./build/compiler $1 $2 $3
    set +e
fi
