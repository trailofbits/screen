# List of subdirectories:
BENCHDIR=.

-include config.mk

ROOT=../..
include $(ROOT)/common.mk
ABSROOT=$(PWD)/$(ROOT)

SUBDIRS=$(shell echo $(BENCHDIR)/*/)
TIMESTAMPS=$(SUBDIRS:%=%/timestamp.txt)
CTIMESTAMPS=$(SUBDIRS:%=%/ctimestamp.txt)

all: $(TIMESTAMPS)

compare: $(CTIMESTAMPS)

%/timestamp.txt:
	cd $* && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model test

%/ctimestamp.txt:
	cd $* && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model compare

clean:
	for p in  $(SUBDIRS); do\
		( cd $$p && $(MAKE) ROOT=$(ABSROOT) -f $(PWD)/Makefile.model clean);\
	done ;

debug:
	@echo BENCHDIR=$(BENCHDIR)
	@echo SUBDIRS=$(SUBDIRS)

