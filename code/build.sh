#!/bin/bash

mkdir -p ../build/
pushd ../ 
xmake project -k compile_commands --lsp=clangd
xmake f -m debug --toolchain=clang --ccache=n && xmake -j$(nproc) 
popd
