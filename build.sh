#!/bin/bash

# Clean previous build (ignore errors if Makefile doesn't exist)
make realclean 2>/dev/null || true

# Remove temporary directories
find . -iname .moc -type d -exec rm -rf {} \; 2>/dev/null || true
find . -iname .obj -type d -exec rm -rf {} \; 2>/dev/null || true

# Configure and build
./configure --prefix=/usr/local/tupitube-server --with-tupitube-dir=/usr/local/tupitube.desk --with-debug
make -j4
