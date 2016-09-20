#! /bin/bash

# Simple script to install PAGAI's dependencies, and then to compile
# it.

set -e

./fetch_externals.sh
cmake .
make $(PARALLEL_MAKE_OPTS)
