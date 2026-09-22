@echo off

REM call "C:\Program Files\Microsoft Visual Studio\2022\\VC\Auxiliary\Build\vcvarsall.bat"
IF NOT EXIST "..\build" (
    mkdir "..\build"
)

pushd ..\
xmake -v f -m debug --compiler=clang-cl --ccache=n && xmake -j %NUMBER_OF_PROCESSORS%
popd
