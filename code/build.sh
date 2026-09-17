#!/bin/bash

mkdir -p ../build/
pushd ../ 
xmake f -m debug --toolchain=clang --ccache=n && xmake -j$(nproc) 
popd
