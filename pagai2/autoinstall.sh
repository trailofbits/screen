#! /bin/bash

# Simple script to install PAGAI's dependencies, and then to compile
# it.

set -e

(cd external/ && make)
cmake .
make $(PARALLEL_MAKE_OPTS)
