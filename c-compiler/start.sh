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
	cd ..
    echo "-------------------------------------------------"
	./build/compiler $1 $2 $3
fi
