#!/usr/bin/env bash

eval $(./create_env.sh examples)

cd ../

echo "[+] Compiling & Running Passes on Openssl Demos..."
echo
./pagai/src/pagai -i ./openssl_demos/aesni_cbc_hmac_sha1_cipher.bc --output-bc-v2 ./openssl_demos/aesni_cbc_hmac_sha1_cipher.bc
echo
${LLVM_OPT} -mem2reg -load build/lib/range.${EXT} openssl_demos/aesni_cbc_hmac_sha1_cipher.bc -o openssl_demos/aesni_cbc_hmac_sha1_cipher_transformed.bc -range_analysis -range-debug -invariant_analysis -invariant-debug 
echo
cat openssl_demos/OUTPUT
echo
exit
echo "[!] changes in function return value comparisons are of high risk"
echo
echo
arr=`cat openssl_demos/OUTPUT_0 | grep function | cut -f1 -d"]"`
arr1=`cat openssl_demos/OUTPUT_1 | grep function | cut -f1 -d"]"`
arr2=`cat openssl_demos/OUTPUT_2 | grep function | cut -f1 -d"]"`
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

