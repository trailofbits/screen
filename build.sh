#!/usr/bin/env bash

if [ $(uname -s) != 'Linux' ]; then
   echo "[!] Screen build script only supported on Linux"
  exit 1
fi

BUILD_DIR=$(dirname $0)/build
LLVM_DIR=llvm

LLVM_VER=3.8.0
DISTRO=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
DIST_VERSION=$(lsb_release -sr)

FILE=clang+llvm-${LLVM_VER}-x86_64-linux-gnu-${DISTRO}-${DIST_VERSION}.tar.xz

echo "[+] Creating '${BUILD_DIR}'"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

if [ ! -f ${FILE} ]; then
  echo "[+] Downloading Clang+LLVM.."
  wget http://llvm.org/releases/${LLVM_VER}/${FILE}
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

echo "[+] Running cmake"
cmake -DLLVM_ROOT=${LLVM_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
make -j4

