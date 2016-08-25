#clang -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include -lssl -lcrypto -O0 -g aesni_cbc_hmac_sha1_cipher.c && ./a.out

../../build/llvm/bin/clang -c -emit-llvm -O0 -g -I../../include/ aesni_cbc_hmac_sha1_cipher.c

