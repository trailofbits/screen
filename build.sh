#!/usr/bin/env bash


BUILD_DIR=$(dirname $0)/build
LLVM_DIR=llvm

ARCH=$(uname -m)
LLVM_VER=3.8.0

case $(uname -s) in
  Darwin)
    OS=apple-darwin
    FILE=clang+llvm-${LLVM_VER}-${ARCH}-${OS}.tar.xz
    ;;
  Linux)
    OS=linux-gnu
    DISTRO=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
    DIST_VERSION=$(lsb_release -sr)
    if [ $DIST_VERSION == "15.04" ]; then
	    DIST_VERSION="14.04"
    fi
    FILE=clang+llvm-${LLVM_VER}-${ARCH}-${OS}-${DISTRO}-${DIST_VERSION}.tar.xz
    ;;
  *)
    echo '[!] Unsupported OS'
    exit 1
    ;;
esac


echo "[+] Creating '${BUILD_DIR}'"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

if [ ! -f ${FILE} ]; then
  echo "[+] Downloading Clang+LLVM.."
  wget http://llvm.org/releases/${LLVM_VER}/${FILE}

  if [ "$?" != "0" ]; then
    echo "[!] Unsupported operating system."
    echo "[!]  Check http://llvm.org/releases/${LLVM_VER}/ to see what LLVM"
    echo "[!]  packages are available."
    exit 2
  fi

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
make -j4 VERBOSE=1 screen

