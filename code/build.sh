#!/bin/bash

bear -o ../misc/compile_commands.json -- make -j12 -k COMPILER=clang++ SILENT=@ RELEASE=0
