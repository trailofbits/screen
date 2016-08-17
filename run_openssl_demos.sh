#!/usr/bin/env bash

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

echo "[+] Compiling openssl demos..."
${LLVM_BIN}/clang -I./include/ -o openssl_demos/master.bc openssl_demos/master.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wno-unused-variable -Wunused-parameter 

echo "[+] Running screen pass..."
${LLVM_BIN}/opt -mem2reg -load build/lib/screen.${EXT} openssl_demos/master.bc -o openssl_demos/master_transformed.bc -screen -screen-output openssl_demos/OUTPUT -screen-start-symbol main 

echo
#cat openssl_demos/OUTPUT
echo
${LLVM_BIN}/opt -mem2reg -load build/lib/screen.${EXT} openssl_demos/ssl3_record_0.bc -o openssl_demos/ssl3_record_0_transformed.bc -screen -screen-output openssl_demos/OUTPUT_0 -screen-start-symbol tls1_enc 

echo
cat openssl_demos/OUTPUT_0
echo
echo
${LLVM_BIN}/opt -mem2reg -load build/lib/screen.${EXT} openssl_demos/ssl3_record_1.bc -o openssl_demos/ssl3_record_1_transformed.bc -screen -screen-output openssl_demos/OUTPUT_1 -screen-start-symbol tls1_enc 

echo
cat openssl_demos/OUTPUT_1
echo
#./build/llvm/bin/opt -load ./build/lib/range.dylib tests/test2.bc -o tests/test2.bc -mem2reg -instnamer
#./build/llvm/bin/opt -load ./build/lib/range.dylib -range_analysis  -range-debug tests/test2.bc -o tests/test2.bc
#${LLVM_BIN}/opt -load build/lib/screen.${EXT} tests/libs2n.bc -o tests/libs2n_transformed.bc -screen -screen-output tests/OUTPUT_S2N -screen-start-symbol s2n_client_key_recv #  s2n_client_hello_send 
 #  s2n_client_hello_send 
 # s2n_client_key_recv
#${LLVM_BIN}/opt \
#  -load build/lib/screen.${EXT} -screen \
#  -screen-output tests/OUTPUT_S2N \
#  -screen-debug \
#  -screen-start-symbol s2n_client_key_recv tests/libs2n.bc -o tests/libs2n_transformed.bc 
echo "[!] changes in function return value comparisons are of high risk"
echo
echo
arr=`cat openssl_demos/OUTPUT_0 | grep function | cut -f1 -d"]"`
arr1=`cat openssl_demos/OUTPUT_1 | grep function | cut -f1 -d"]"`
if [ "$arr" != "$arr1" ]; then
	echo "[!] Warning additional cmp on function return value"
	echo "Lucky 13 Bug: function should not branch on -1 but on 0"
	echo
	echo $arr
	echo $arr1
	echo
fi

