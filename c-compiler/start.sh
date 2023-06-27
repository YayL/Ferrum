#!/bin/sh

set -e
cd /home/zimon/Projects/ferrum/c-compiler/build
cp ../submodule/String-Formatter/include/fmt.h ../include
if [ "$1" == "c" ] ; then
	rm -rf CMakeFiles
	rm CMakeCache.txt
	cmake -DCMAKE_BUILD_TYPE=Debug ..
	make
else
	make
	cd ..
    echo "-------------------------------------------------"
	./build/compiler $1 $2 $3
fi
