ifndef ROOT
	ROOT=.
endif

PAGAI_SH = $(ROOT)/scripts/pagai.sh
COMPILE_LLVM = $(ROOT)/scripts/compile_llvm.sh

ifndef VERBOSE
AT = @
else
AT =
endif

%.bc: CFLAGS += -I./
