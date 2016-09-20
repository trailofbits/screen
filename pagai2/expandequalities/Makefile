LLVMDIR=~/git/pagai/external/llvm
FILE=expandequalities
$(FILE).so : ../src/$(FILE).cc ../src/$(FILE).h
	clang -I $(LLVMDIR)/include -L $(LLVMDIR)/lib -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -Wall -fPIC -shared ../src/$(FILE).cc -o $(FILE).so 
