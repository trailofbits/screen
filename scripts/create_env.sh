#!/bin/bash

PWD=$(dirname $0)
BASE=$(realpath ${PWD}/..)
BUILD=${BASE}/build
BIN=${BUILD}/llvm/bin

if [ ! -d ${BIN} ]; then
  echo "[!] Screen not built; run ./build.sh" > /dev/stderr
  exit 1
fi

if [ "${1}" == "" ]; then
  echo "[!] Usage: ./create_env.sh [output]" > /dev/stderr
  exit 2
fi

case $OSTYPE in
  darwin*) 
    EXT=dylib
    ;;
  linux*)
    EXT=so
    ;;
  *)
    echo "Could not detect OS (${OSTYPE})"
    exit 2
    ;;
esac


echo "export BUILD_DIR=${BUILD}"
echo "export CC=${BIN}/clang"
echo "export CXX=${BIN}/clang++"
echo "export LLVM_LINK=${BIN}/llvm-link"
echo "export LLVM_OPT=${BIN}/opt"
echo "export EXTRA_CFLAGS=-emit-llvm"
echo "export BUILD_BITCODE=1"
echo "export EXT=${EXT}"
echo "export OPT_FLAGS=\"-load ${BUILD}/lib/screen.${EXT} -screen --screen-debug --screen-start-symbol=main --screen-output ${1}\""
