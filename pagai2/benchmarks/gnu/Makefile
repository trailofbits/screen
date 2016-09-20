# List of subdirectories:
BENCHDIR=.

-include config.mk

ifndef DOM1
	 DOM1=pk
endif
ifndef DOM2
	 DOM2=box
endif


ROOT=../..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

SUBDIRS=$(shell echo $(BENCHDIR)/*/)
TIMESTAMPS=$(SUBDIRS:%=%/timestamp.txt)
CTIMESTAMPS=$(SUBDIRS:%=%/ctimestamp.txt)
NTIMESTAMPS=$(SUBDIRS:%=%/ntimestamp.txt)
DTIMESTAMPS=$(SUBDIRS:%=%/dtimestamp-$(DOM1)-$(DOM2).txt)

all: $(TIMESTAMPS)

compare: $(CTIMESTAMPS)

ncompare: $(NTIMESTAMPS)

dcompare: $(DTIMESTAMPS)

%/timestamp.txt:
	cd $* && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model test

%/ctimestamp.txt:
	cd $* && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model compare

%/ntimestamp.txt:
	cd $* && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model ncompare

%/dtimestamp-$(DOM1)-$(DOM2).txt:
	cd $* && $(MAKE) ROOT=$(ABSROOT) DOM1=$(DOM1) DOM2=$(DOM2) -f $(PWD)/Makefile.model dcompare

clean:
	for p in  $(SUBDIRS); do\
		( cd $$p && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model clean);\
	done ;

nclean:
	for p in  $(SUBDIRS); do\
		( cd $$p && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model nclean && cd ..);\
	done ;

cclean:
	for p in  $(SUBDIRS); do\
		( cd $$p && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model cclean && cd ..);\
	done ;

dclean:
	for p in  $(SUBDIRS); do\
		( cd $$p && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model dclean && cd ..);\
	done ;

cdiff:
	./diff_analyzer.sh $(BENCHDIR)

ndiff:
	./diff_narrowing.sh $(BENCHDIR)

ddiff:
	./diff_domain.sh $(BENCHDIR)

resdir:
	for p in  $(SUBDIRS); do\
		( cd $$p && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model resdir);\
	done ;

debug:
	@echo BENCHDIR=$(BENCHDIR)
	@echo SUBDIRS=$(SUBDIRS)

