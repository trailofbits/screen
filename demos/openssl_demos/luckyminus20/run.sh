../../../pagai/src/pagai -i ./luckyMinus20.bc --output-bc-v2 ./luckyMinus20_anno.bc
../../../build/llvm/bin/opt -load ../../build/lib/range.dylib -range_analysis -range-debug -invariant_analysis -invariant-debug ./luckyMinus20_anno.bc -o luckyMinus20_out.bc
