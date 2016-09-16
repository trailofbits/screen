LLVMDIR=/local/clang/llvm-3.3.src
optbr.so : optbr.cc optbr.h
	clang -I $(LLVMDIR)/include -L $(LLVMDIR)/lib -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -Wall -fPIC -shared optbr.cc -o optbr.so 
