BENCHDIR=.

-include config.mk

ROOT=../..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

CFILES=${wildcard $(BENCHDIR)/*.c}
BCFILES=$(CFILES:%.c=%.bc)
RESFILES=$(CFILES:%.c=%.res)
DISFILES=$(CFILES:%.c=%.dis)
COMPAREFILES=$(CFILES:%.c=%.res.compare)

all: $(BCFILES)

%.bc: %.c
	$(AT)$(COMPILE_LLVM) -i $*.c -o $@

%.res: %.bc
	$(AT)$(PAGAI_SH) -i $*.bc -a " -o $@ " -t 20 2> /dev/null

%.dis: %.bc
	$(AT)$(PAGAI_SH) -i $*.bc -a " -o $@ -t dis " -t 20 2> /dev/null

%.res.compare: %.bc
	$(AT)$(PAGAI_SH) -i $*.bc -a " -c -o $@ " -t 60 2> /dev/null

clean:
	rm $(BENCHDIR)/*.bc
	rm $(BENCHDIR)/*.res*

test: $(RESFILES)

distest: $(DISFILES)

compare: $(COMPAREFILES)

