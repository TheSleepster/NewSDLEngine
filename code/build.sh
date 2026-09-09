#!/bin/bash

make -j12 -k COMPILER=llvm-mingw tests sandboxes SILENT=@
