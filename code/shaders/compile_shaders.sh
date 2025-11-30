#!/bin/bash

echo "Compiling vertex shaders"
slangc test.slang -target spirv -entry vertex_main -stage vertex -o test_vert.spv

echo "Done..."
echo "Compiling fragment shaders"

slangc test.slang -target spirv -entry fragment_main -stage fragment -o test_frag.spv
echo "Done..."
