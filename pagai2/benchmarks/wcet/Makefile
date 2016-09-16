BENCHDIR=.

-include config.mk

ROOT=../..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

BCFILES=${wildcard $(BENCHDIR)/*.bc}
RESFILES=$(BCFILES:%.bc=%.res)
COMPAREFILES=$(BCFILES:%.bc=%.res.compare)
NCOMPAREFILES=$(BCFILES:%.bc=%.res.narrow)

all: $(RESFILES)

%.res: %.bc
	$(AT)$(PAGAI_SH) -s -I 2000 -t 800 -i $*.bc > $@

%.res.compare: %.bc
	$(AT)$(PAGAI_SH) -s -t 800 -i $*.bc -a " -c " > $@

%.res.narrow: %.bc
	$(AT)$(PAGAI_SH) -s -I 20000 -t 800 -i $*.bc -a " -t s --domain pk --domain2 pk -M " > $@

clean:
	rm $(BENCHDIR)/*.res*

cclean:
	rm $(BENCHDIR)/*.res.compare

nclean:
	rm $(BENCHDIR)/*.res.narrow

test: $(RESFILES)

compare: $(COMPAREFILES)

ncompare: $(NCOMPAREFILES)

ndiff:
	$(AT)./diff_narrowing.sh $(BENCHDIR)

cdiff:
	$(AT)./diff_compare.sh $(BENCHDIR)

