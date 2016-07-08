#!/usr/bin/env bash

LLVM_SRC_DIR=$PWD/llvm-3.8.0

echo ""
echo "Downloading LLVM..." 1>&2
wget "http://llvm.org/releases/3.8.0/llvm-3.8.0.src.tar.xz"

echo ""
echo "Downloading Clang..." 1>&2
wget "http://llvm.org/releases/3.8.0/cfe-3.8.0.src.tar.xz"

echo ""
echo "Downloading Compiler-RT..." 1>&2
wget "http://llvm.org/releases/3.8.0/compiler-rt-3.8.0.src.tar.xz"

echo ""
echo "Unpacking LLVM..." 1>&2
tar -xf llvm-3.8.0.src.tar.xz
mv llvm-3.8.0.src $LLVM_SRC_DIR

echo ""
echo "Unpacking Clang into tools..." 1>&2
tar -xf cfe-3.8.0.src.tar.xz
mv cfe-3.8.0.src $LLVM_SRC_DIR/tools/clang

echo ""
echo "Unpacking Compiler-RT into projects..." 1>&2
tar -xf compiler-rt-3.8.0.src.tar.xz
mv compiler-rt-3.8.0.src $LLVM_SRC_DIR/projects/compiler-rt

echo ""
echo "Copying Screen Pass sources" 1>&2
echo "into LLVM source tree..." 1>&2
cp -r Screen Pass $LLVM_SRC_DIR/projects/

echo ""
echo "Installing LLVM..." 1>&2
mkdir $LLVM_SRC_DIR/build
cd $LLVM_SRC_DIR/build
LLVM_BUILD_DIR=$PWD
export CC=gcc
export CXX=g++
../configure --enable-optimized 
REQUIRES_RTTI=1 make -j5

echo "Run with opt..."


