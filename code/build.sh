#!/bin/bash

mkdir -p ../build/
if [ ! -d ../build/Makefile ]; then
    premake5 gmake --toolchain=clang
fi

pushd ../build/
make config=debug SILENT=@
popd
