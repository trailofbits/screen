./pagai -i test.bc
./pagai -i test.bc -o test_ir.ll
./pagai -i test.bc --output-bc-v2 test_bc_v2.bc
../../build/llvm/bin/llvm-dis test_bc_v2.bc -o test_bc_v2.ll
