#!/bin/bash

make -j12 -k COMPILER=clang++ tests sandboxes SILENT=@
