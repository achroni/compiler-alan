#!/bin/sh

if [ "$1" != "" ]; then
    echo "Compiling $1"
    ./alanc < $1 > a.ll || exit 1
    echo "Done IR"
    llc-6.0 a.ll -o a.s
    echo "Linking..."
    clang-6.0 a.s lib.a -o alan.out
fi
