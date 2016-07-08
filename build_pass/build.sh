#!/usr/bin/env bash

LLVM_SRC_DIR=$PWD/llvm-3.8.0

echo ""
echo "Copying Screen Pass sources" 1>&2
echo "into LLVM source tree..." 1>&2
cp -r screen_pass $LLVM_SRC_DIR/projects/

echo ""
echo "Installing LLVM..." 1>&2
mkdir $LLVM_SRC_DIR/build
cd $LLVM_SRC_DIR/build
LLVM_BUILD_DIR=$PWD
export CC=gcc
export CXX=g++
REQUIRES_RTTI=1 make -j5

echo "Run with opt..."


