#!/usr/bin/env bash

eval $(./create_env.sh examples)

cd ../

echo "[+] Compiling tests..."
${CC} -o tests/test1.bc tests/test1.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wno-unused-variable -Wunused-parameter 
${CC} -o tests/test1 tests/test1.c -Wall -Wshadow -Wextra

${CC} -I./include/ -o tests/test2.bc tests/test2.c -c -emit-llvm -Wall -Wshadow -Wextra 
${CC} -I./include/ -o tests/test2 tests/test2.c -Wall -Wshadow -Wextra

${CC}  -I./include/ -o tests/test.bc tests/test.c -c -emit-llvm -Wall -Wshadow -Wextra 
${CC} -I./include/ -o tests/test tests/test.c -Wall -Wshadow -Wextra

echo "[+] Running screen pass..."
${LLVM_OPT} -load build/lib/screen.${EXT} tests/test1.bc -o tests/test1_transformed.bc -screen -screen-output tests/OUTPUT -screen-start-symbol main 

echo
echo
echo "[+] Verifying paths:"
echo "B1: main-> printf-> fun1-> anno-> printf-> anno-> printf-> printf"
echo "B2: main-> printf-> foo-> printf-> printf"
echo
echo
echo "[+] Running range analysis pass..."
${LLVM_OPT} -load ./build/lib/range.${EXT} tests/test2.bc -o tests/test2.bc -mem2reg -instnamer
${LLVM_OPT} -load ./build/lib/range.${EXT} -range_analysis  -range-debug tests/test2.bc -o tests/test2.bc
#${LLVM_OPT} -load build/lib/screen.${EXT} tests/libs2n.bc -o tests/libs2n_transformed.bc -screen -screen-output tests/OUTPUT_S2N -screen-start-symbol s2n_client_key_recv #  s2n_client_hello_send 
 #  s2n_client_hello_send 
 # s2n_client_key_recv
#${LLVM_OPT} \
#  -load build/lib/screen.${EXT} -screen \
#  -screen-output tests/OUTPUT_S2N \
#  -screen-debug \
#  -screen-start-symbol s2n_client_key_recv tests/libs2n.bc -o tests/libs2n_transformed.bc 
echo
echo


