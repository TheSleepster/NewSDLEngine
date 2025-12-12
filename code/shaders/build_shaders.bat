@echo off

pushd shaders
sokol-shdc --input test.glsl --output test.glsl.h --slang glsl430
popd
