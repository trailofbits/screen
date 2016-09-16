
ROOT=../..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

CFILES=${wildcard *.c}
BCFILES=$(CFILES:%.c=bin/%.bc)
RESFILES=$(CFILES:%.c=res/%.res)
ANNOTATEDFILES=$(CFILES:%.c=annotated/%.c)

all: $(BCFILES)

bin/%.bc: %.c
	$(COMPILE_LLVM) -g -i $*.c -o $@

res/%.res: bin/%.bc
	$(PAGAI_SH) -i bin/$*.bc -a " -o $@ "

annotated/%.c: bin/%.bc %.c
	$(PAGAI_SH) -i bin/$*.bc -a " -o /tmp/output.txt --source $*.c --annotated $@ "

realclean:
	rm bin/*.bc
	rm res/*.res

clean:
	rm res/*.res

test: $(RESFILES)
	if [ ! -d res ] ; then mkdir res; fi

annotated: $(ANNOTATEDFILES)
	if [ ! -d annotated ] ; then mkdir annotated; fi

