BCFILES= $(wildcard *.bc)
BLOCKS_MATCH_FILES= $(wildcard *.blocks.match)
EDGES_MATCH_FILES= $(wildcard *.edges.match)
RES=$(BCFILES:.bc=.res)
NOCUTSRES=$(BCFILES:.bc=.nocuts.res)
BLOCKRES=$(BLOCKS_MATCH_FILES:.match=.res)
EDGERES=$(EDGES_MATCH_FILES:.match=.res)
EDGERECURSIVECUTSRES=$(EDGES_MATCH_FILES:.match=.recursivecuts.res)
EDGENOCUTSRES=$(EDGES_MATCH_FILES:.match=.nocuts.res)
SCRIPTDIR=..
ULIMIT=120

all: $(EDGERES) $(EDGENOCUTSRES) $(RES) $(NOCUTSRES) $(EDGERECURSIVECUTSRES)

%.blocks.res: %.bc %.blocks.match
	#$(SCRIPTDIR)/go.sh -c $< -b -l -n -M $*.blocks.match > $@ 2> $@
	$(SCRIPTDIR)/go.sh -c $< -b -n -T $(ULIMIT) -M $*.blocks.match 

%.edges.res: %.bc %.edges.match
	$(SCRIPTDIR)/go.sh -c $< -b -l -n -T $(ULIMIT) -M $*.edges.match -S $*.edges.syntactic

%.edges.recursivecuts.res: %.bc %.edges.match
	$(SCRIPTDIR)/go.sh -c $< -b -l -n -R -T $(ULIMIT) -M $*.edges.match -S $*.edges.syntactic

%.edges.nocuts.res: %.bc %.edges.match
	$(SCRIPTDIR)/go.sh -c $< -b -l -n -T $(ULIMIT) -s -M $*.edges.match -S $*.edges.syntactic

%.res: %.bc
	#$(SCRIPTDIR)/go.sh -c $< -b -l -n > $@ 2> $@
	$(SCRIPTDIR)/go.sh -c $< -b -l -n -T $(ULIMIT) 

%.nocuts.res: %.bc
	#$(SCRIPTDIR)/go.sh -c $< -b -l -n > $@ 2> $@
	$(SCRIPTDIR)/go.sh -c $< -b -l -n -T $(ULIMIT) -s

clean:
	rm -f *.gen *.smtmatch *.smt2 *.syntactic *.smtopt *.ll smtopt.smt *.cuts

realclean: clean
	rm -f *.res benchmarks.tex benchmarksedges.tex benchmarkslist.tex
