LLVMDIR=~/git/pagai/external/llvm/
instrOverflow.so : ../src/instrOverflow.cc ../src/instrOverflow.h
	clang -I $(LLVMDIR)/include -L $(LLVMDIR)/lib -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -Wall -fPIC -shared $< -o $@
