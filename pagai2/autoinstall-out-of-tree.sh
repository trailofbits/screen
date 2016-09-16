#! /bin/bash

# Simple script to install PAGAI's dependencies, and then to compile
# it.

set -e

srcdir=$(dirname "$0")
srcdir=$(cd "$srcdir" && pwd)
builddir=$(pwd)

(cd "$srcdir"/external/ && make DEST="$builddir"/external/build DEST_LINKS="$builddir"/external)
cmake "$srcdir"
make
