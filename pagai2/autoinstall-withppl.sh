#! /bin/bash

# Simple script to install PAGAI's dependencies, and then to compile
# it.

set -e

(cd external/ && make PPL_ENABLED=1)
cmake -DENABLE_PPL=ON .
make
