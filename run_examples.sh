#!/usr/bin/env bash

echo "RUNNING EXAMPLE SIDE CHANNELS"

cd $(dirname $0)
LLVM_BIN=build/llvm/bin
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


if [ ! -d ${LLVM_BIN} ]; then
  echo "[!] LLVM not found. Run ./build.sh"
  exit 1
fi

echo "[+] Compiling example side channel programs..."
${LLVM_BIN}/clang -I./include/ -o examples/01_annotated_function.bc examples/01_annotated_function.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas 
${LLVM_BIN}/clang -I./include/ -o examples/01_annotated_function examples/01_annotated_function.c -Wall -Wshadow -Wextra -Wno-unknown-pragmas

echo "[+] Running screen pass..."
echo
echo "[ EXAMPLE ONE ]"
echo "Description: for loop with modifiable and un-modifiable bounds."
echo "[-] Ouput"
${LLVM_BIN}/opt -mem2reg examples/01_annotated_function.bc -o examples/01_annotated_function.bc
${LLVM_BIN}/opt -load build/lib/screen.${EXT} examples/01_annotated_function.bc -o examples/01_annotated_function_transformed.bc -screen -screen-output examples/OUTPUT -screen-start-symbol main 
echo "[-] Expected Output"
echo "    less than operand"
echo "    variable"
echo "    constant: 4"
echo
echo
echo


