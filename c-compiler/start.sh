#!/bin/sh

cd ./build
if [ "$1" == "c" ] ; then
	rm -rf CMakeFiles
	rm CMakeCache.txt
	cmake -DCMAKE_BUILD_TYPE=Debug ..
	make
else
    set -e
	make
	cd ..
    echo "-------------------------------------------------"
	./build/compiler $1 $2 $3
    clang ./build/ferrum.ll -emit-llvm -S -c -O3 -o out.ll
    clang out.ll
fi
