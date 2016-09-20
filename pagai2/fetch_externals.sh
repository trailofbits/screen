#!/bin/bash

set -e
set -v

mkdir -p external/build
pushd external

make install-yices

mkdir -p build/apron
wget https://s3.amazonaws.com/screen-s3/screen-builds/apron-install.tgz
tar xf ./apron-install.tgz -C build/apron --strip-components=1
ln -sf build/apron apron

mkdir -p build/boost
wget https://s3.amazonaws.com/screen-s3/screen-builds/boost_1_58_0_program_options.tgz
tar xf ./boost_1_58_0_program_options.tgz -C build/boost --strip-components=1
ln -sf build/boost boost

mkdir -p build/llvm
wget https://s3.amazonaws.com/screen-s3/screen-builds/clang%2Bllvm-3.8.1_wRTTI.tgz
tar xf ./clang+llvm-3.8.1_wRTTI.tgz -C build/llvm --strip-components=1
ln -sf build/llvm llvm

mkdir -p build/cudd
wget https://s3.amazonaws.com/screen-s3/screen-builds/cudd-2.5.0-build.tgz
tar xf ./cudd-2.5.0-build.tgz -C build/cudd --strip-components=1
ln -sf build/cudd cudd

mkdir -p build/z3
wget https://s3.amazonaws.com/screen-s3/screen-builds/z3-inst.tgz
tar xf ./z3-inst.tgz -C build/z3 --strip-components=1
ln -sf build/z3 z3

popd
