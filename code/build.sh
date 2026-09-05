#!/bin/bash

bear -o ../misc/compile_commands.json -- make -j12 -k RELEASE=0 COMPILER=clang++ SILENT=@
