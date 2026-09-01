#!/bin/bash

bear -o ../misc/compile_commands.json -- make -k COMPILER=clang++ SILENT=@ DEBUG_BUILD=1
