#!/usr/bin/env bash

BUILD_DIR=build
LLVM_DIR=llvm
FILE=clang+llvm-3.8.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz

echo "Making ${BUILD_DIR}"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

if [ ! -f ${FILE} ]; then
  echo "Downloading Clang+LLVM..."
  wget -q --show-progress http://llvm.org/releases/3.8.1/${FILE}
fi

if [ ! -d ${LLVM_DIR} ]; then
  echo "Unpacking.."
  mkdir ${LLVM_DIR}
  tar xf ${FILE} -C ${LLVM_DIR} --strip-components=1 
fi

cmake -DLLVM_ROOT=${LLVM_DIR} ..
make -j4

