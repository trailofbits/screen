#!/bin/bash

PWD=$(dirname $0)
PWD=$(realpath ${PWD})
BUILD=${PWD}/build
BIN=${BUILD}/llvm/bin

if [ ! -d ${BIN} ]; then
  echo "[!] Screen not built; run ./build.sh" > /dev/stderr
  exit 1
fi

if [ "${1}" == "" ]; then
  echo "[!] Usage: ./build.sh [output]" > /dev/stderr
  exit 2
fi

echo "export CC=${BIN}/clang"
echo "export CXX=${BIN}/clang++"
echo "export LLVM_LINK=${BIN}/llvm-link"
echo "export LLVM_OPT=${BIN}/opt"
echo "export EXTRA_CFLAGS=-emit-llvm"
echo "export BUILD_BITCODE=1"
echo "export OPT_FLAGS=\"-load ${BUILD}/lib/screen.so -screen --screen-output ${1}\""
