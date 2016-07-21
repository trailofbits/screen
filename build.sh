#!/usr/bin/env bash

BUILD_DIR=$(dirname $0)/build
LLVM_DIR=llvm
FILE=clang+llvm-3.8.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz

if [ $(uname -s) != 'Linux' ]; then
   echo "[!] Screen build script only supported on Linux"
  exit 1
fi


echo "[+] Creating '${BUILD_DIR}'"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

if [ ! -f ${FILE} ]; then
  echo "[+] Downloading Clang+LLVM.."
  wget http://llvm.org/releases/3.8.1/${FILE}
fi

if [ ! -d ${LLVM_DIR} ]; then
  echo "[+] Extracting.."
  mkdir ${LLVM_DIR}
  tar xf ${FILE} -C ${LLVM_DIR} --strip-components=1 
fi

if [ "$1" == "debug" ]; then
  BUILD_TYPE="Debug"
else
  BUILD_TYPE="Release"
fi

cmake -DLLVM_ROOT=${LLVM_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
make -j4

