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

echo "[+] Compiling & Running Passes on Openssl Demos..."
${LLVM_BIN}/clang -I./include/ -o openssl_demos/master.bc openssl_demos/master.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wno-unused-variable -Wunused-parameter 

echo "[+] Running Screen Pass..."
${LLVM_BIN}/opt -mem2reg -load build/lib/screen.${EXT} openssl_demos/master.bc -o openssl_demos/master_transformed.bc -screen -screen-output openssl_demos/lucky13/OUTPUT -screen-start-symbol main 

echo
#cat openssl_demos/lucky13/OUTPUT
echo
${LLVM_BIN}/opt -mem2reg -load build/lib/screen.${EXT} openssl_demos/lucky13/ssl3_record_0.bc -o openssl_demos/lucky13/ssl3_record_0_transformed.bc -screen -screen-output openssl_demos/lucky13/OUTPUT_0 -screen-start-symbol tls1_enc 

echo
cat openssl_demos/lucky13/OUTPUT_0
echo
echo
${LLVM_BIN}/opt -mem2reg -load build/lib/screen.${EXT} openssl_demos/lucky13/ssl3_record_1.bc -o openssl_demos/lucky13/ssl3_record_1_transformed.bc -screen -screen-output openssl_demos/lucky13/OUTPUT_1 -screen-start-symbol tls1_enc 

echo
cat openssl_demos/lucky13/OUTPUT_1
echo
echo
echo
${LLVM_BIN}/opt -mem2reg -load build/lib/screen.${EXT} openssl_demos/lucky13/ssl3_record_2.bc -o openssl_demos/lucky13/ssl3_record_2_transformed.bc -screen -screen-output openssl_demos/lucky13/OUTPUT_2 -screen-start-symbol tls1_enc 

echo
cat openssl_demos/lucky13/OUTPUT_2
echo
echo "[!] changes in function return value comparisons are of high risk"
echo
echo
arr=`cat openssl_demos/lucky13/OUTPUT_0 | grep function | cut -f1 -d"]"`
arr1=`cat openssl_demos/lucky13/OUTPUT_1 | grep function | cut -f1 -d"]"`
arr2=`cat openssl_demos/lucky13/OUTPUT_2 | grep function | cut -f1 -d"]"`
if [ "$arr1" != "$arr2" ]; then
	echo "[!] Warning additional cmp on function return value"
	echo "Lucky 13 Bug: function should not branch on -1 but on 0"
	echo
	echo 'pre-vuln: '$arr
	echo 
	echo
	echo 'vuln: '$arr1
	echo 
	echo
	echo 'post-vuln: '$arr2
	echo
	echo
	echo "[+] Vulnerable difference between commits 0 and 1"
	diff <(echo "$arr" ) <(echo "$arr1") | head -n 2
        echo	
	echo "[+] Vulnerability patch between commits 1 and 2"
	diff <(echo "$arr1" ) <(echo "$arr2") | head -n 4 
	echo
	echo
fi

