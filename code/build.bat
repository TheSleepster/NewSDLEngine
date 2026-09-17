@echo off

REM call "C:\Program Files\Microsoft Visual Studio\2022\\VC\Auxiliary\Build\vcvarsall.bat"
IF NOT EXIST "..\build" (
    mkdir "..\build"
)

pushd ..\
xmake project -k compile_commands --lsp=clangd
xmake f -m debug --toolchain=msvc --ccache=n && xmake -j %NUMBER_OF_PROCESSORS%
popd
