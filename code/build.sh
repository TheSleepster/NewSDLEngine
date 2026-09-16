#!/bin/bash

mkdir -p ../build/
if [ ! -d ../build/build.ninja ]; then
    premake5 ninja --toolchain=clang
fi

pushd ../build/
ninja Debug
popd
