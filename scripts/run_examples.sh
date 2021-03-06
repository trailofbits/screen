#!/usr/bin/env bash

echo "RUNNING EXAMPLE SIDE CHANNELS"

eval $(./create_env.sh examples)

cd ..

echo "[+] Compiling example side channel programs..."
${CC} -I./include/ -o examples/01_annotated_function.bc examples/01_annotated_function.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wno-unused-variable -Wunused-parameter 
${CC} -I./include/ -o examples/02_annotated_region_simple.bc examples/02_annotated_region_simple.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wunused-parameter -Wno-unused-variable 
${CC} -I./include/ -o examples/03_interprocedural_basic.bc examples/03_interprocedural_basic.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wunused-parameter -Wno-unused-variable
${CC} -I./include/ -o examples/04_interprocedural_forked.bc examples/04_interprocedural_forked.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wunused-parameter -Wno-unused-variable
${CC} -I./include/ -o examples/05_basic_loop.bc examples/05_basic_loop.c -c -emit-llvm -Wall -Wshadow -Wextra -Wno-unknown-pragmas -Wunused-parameter -Wno-unused-variable

echo "[+] Running screen pass..."
echo
echo "[ EXAMPLE ONE ]"
echo "Description: for loop with modifiable and un-modifiable bounds."
echo "[-] Ouput"
${LLVM_OPT} -mem2reg examples/01_annotated_function.bc -o examples/01_annotated_function.bc
${LLVM_OPT} -load build/lib/screen.${EXT} examples/01_annotated_function.bc -o examples/01_annotated_function_transformed.bc -screen -screen-output examples/OUTPUT -screen-start-symbol main 
echo "[-] Expected Output"
echo "    less than operand, local variable, external variable"
echo "    less than operand, local variable, constant 4"
echo 
cat examples/OUTPUT 
echo
echo "[ EXAMPLE TWO ]"
echo "Description: program reads in magic header and prints file information."
echo "[-] Ouput"
${LLVM_OPT} -mem2reg examples/02_annotated_region_simple.bc -o examples/02_annotated_region_simple.bc
${LLVM_OPT} -load build/lib/screen.${EXT} examples/02_annotated_region_simple.bc -o examples/02_annotated_region_simple_transformed.bc -screen -screen-output examples/OUTPUT -screen-start-symbol main 
echo "[-] Expected Output"
echo "    equals predicate, local variable, NULL"
echo "    not equals predicate, local variable, constant 1" 
echo "    equals predicate, local variable, constant 2135247942 ( or 0x7f454c46 )"
echo 
cat examples/OUTPUT 
echo
echo "[ EXAMPLE THREE ]"
echo "Description: program incrementally steps through functions."
echo "[-] Ouput"
${LLVM_OPT} -mem2reg examples/03_interprocedural_basic.bc -o examples/03_interprocedural_basic.bc
${LLVM_OPT} -load build/lib/screen.${EXT} examples/03_interprocedural_basic.bc -o examples/03_interprocedural_basic_transformed.bc -screen -screen-output examples/OUTPUT -screen-start-symbol main 
echo "[-] Expected Output"
echo "    <no cmp instructions to handle>"
echo 
cat examples/OUTPUT 
echo
echo "[ EXAMPLE FOUR ]"
echo "Description: program incrementally steps through functions. "
echo "There are TWO possible execution paths, the first has one cmp, the second has 2 cmps."
echo "[-] Ouput"
${LLVM_OPT} -mem2reg examples/04_interprocedural_forked.bc -o examples/04_interprocedural_forked.bc
${LLVM_OPT} -load build/lib/screen.${EXT} examples/04_interprocedural_forked.bc -o examples/04_interprocedural_forked_transformed.bc -screen -screen-output examples/OUTPUT -screen-start-symbol main 
echo "[-] Expected Output"
echo "    equals predicate, local variable, constant 0"
echo 
cat examples/OUTPUT 
echo
echo "[ EXAMPLE FIVE ]"
echo "Description: program incrementally steps through functions. "
echo "There are TWO possible execution paths, the first has one cmp, the second has 2 cmps."
echo "[-] Ouput"
${LLVM_OPT} -mem2reg examples/05_basic_loop.bc -o examples/05_basic_loop.bc
${LLVM_OPT} -load build/lib/screen.${EXT} examples/05_basic_loop.bc -o examples/05_basic_loop_transformed.bc -screen -screen-output examples/OUTPUT -screen-start-symbol main 
echo "[-] Expected Output"
echo "    equals predicate, local variable, local variable"
echo 
cat examples/OUTPUT 
echo


