# Parameters

# Parallel make, e.g. PARALLEL_MAKE_OPTS=-j4

# GMP and MPFR prefixes, if nonstandard
GMP_PREFIX = /usr
MPFR_PREFIX = /usr

# PPL is not enabled by default
# PPL_ENABLED should be set to 1 for compiling Apron with PPL
PPL_ENABLED=

# OptOctagons from ETHZ are not enabled by default
# OPT_OCT_ENABLED should be set to 1 for compiling Apron with OptOctagons
# Maybe works only on 64-bit arch
OPT_OCT_ENABLED=

# OPT_OCT_OPTIONS
OPT_OCT_OPTIONS="IS_VECTOR=-DVECTOR" # if machine supports AVX (see in /proc/cpuinfo)
OPT_OCT_OPTIONS="IS_VECTOR=-DVECTOR IS_SSE=-DSSE" # if machine supports SSE but not AVX
OPT_OCT_OPTIONS= # otherwise

.SUFFIXES:

DEST=build
SOURCE=$(PWD)
DEST_LINKS=.

CUDD_VERSION=2.5.0
YICES_VERSION=1.0.40
YICES_ARCH=i686-pc-linux-gnu-static-gmp
Z3_COMMIT=29606b5179f76783ffb0c2ca0ed9d614847064b3
LLVM_VERSION=3.8.1
APRON_VERSION=2016-01-05
PPL_VERSION=1.1
CUDD_XCFLAGS=

ARCH=${shell uname -m | sed -e 's/i.86/iX86/'}
ifeq ($(ARCH), x86_64)
YICES_ARCH=x86_64-unknown-linux-gnu-static-gmp
Z3_ARCH=-x64
CUDD_XCFLAGS='XCFLAGS=-mtune=native -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8'
endif
ifeq ($(ARCH), sparc64)
CUDD_XCFLAGS='XCFLAGS=-DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=4 -DSIZEOF_LONG=4'
endif

OS=${shell uname}
#ifeq ($(OS), Darwin)
#TAR=gnutar
#else
TAR=tar
#endif

.PHONY: all install-cudd install-z3 install-llvm install-clang install-apron install-ppl install-yices
all: install-llvm install-clang 
#install-cudd install-z3 install-apron install-yices

ifeq ($(OS), Darwin)
SOEXTENSION=.dylib
else
ifeq ($(OS), Cygwin)
SOEXTENSION=.dll
else
SOEXTENSION=.so
endif
endif

CUDD_COMPILED=$(DEST)/cudd-$(CUDD_VERSION)/cudd/libcudd.a
YICES_COMPILED=$(DEST)/yices-$(YICES_VERSION)/lib/libyices.a
Z3_COMPILED=$(DEST)/z3-$(Z3_COMMIT)/install/lib/libz3$(SOEXTENSION)
LLVM_COMPILED=$(DEST)/llvm-$(LLVM_VERSION)/install/lib/libLLVMBitReader.a
CLANG_COMPILED=$(DEST)/llvm-$(LLVM_VERSION)/install/bin/clang
APRON_COMPILED=$(DEST)/apron_$(APRON_VERSION)/install/lib/libapron$(SOEXTENSION)

PPL_COMPILED=$(DEST)/ppl-$(PPL_VERSION)/installdir/lib/libppl$(SOEXTENSION)

ifeq ($(OPT_OCT_ENABLED), 1)
OPT_OCT_COMPILED=$(DEST)/apron_$(APRON_VERSION)/install/lib/liboptoct$(SOEXTENSION)

$(OPT_OCT_COMPILED): $(DEST)/apron_$(APRON_VERSION)/optoctagons/README $(DEST)/apron_$(APRON_VERSION)/apron/README
	cd $(DEST)/apron_$(APRON_VERSION)/optoctagons/ && make $(OPT_OCT_OPTIONS) $(PARALLEL_MAKE_OPTS) && make install

else
OPT_OCT_COMPILED=
endif

install-cudd: $(CUDD_COMPILED) $(DEST_LINKS)/cudd
install-yices: $(YICES_COMPILED) $(DEST_LINKS)/yices
install-z3: $(Z3_COMPILED) $(DEST_LINKS)/z3
install-llvm: $(LLVM_COMPILED) $(DEST_LINKS)/llvm
install-clang: $(CLANG_COMPILED) $(DEST_LINKS)/llvm
install-apron: $(APRON_COMPILED) $(OPT_OCT_COMPILED) $(DEST_LINKS)/apron
install-ppl: $(PPL_COMPILED) $(DEST_LINKS)/ppl

CUDD_ARCHIVE=cudd-$(CUDD_VERSION).tar.gz
YICES_ARCHIVE=yices-$(YICES_VERSION)-$(YICES_ARCH).tar.gz
Z3_ARCHIVE=z3-$(Z3_COMMIT).tar.gz
LLVM_ARCHIVE=llvm-$(LLVM_VERSION).src.tar.xz
CLANG_ARCHIVE=cfe-$(LLVM_VERSION).src.tar.xz
APRON_ARCHIVE=apron_$(APRON_VERSION).tar.gz
PPL_ARCHIVE=ppl-$(PPL_VERSION).tar.gz

DEST_ABS=${shell mkdir -p "$(DEST)" && cd $(DEST) && pwd}

# Small shell function to download using curl or wget if curl is not
# installed or doesn't work.
DOWNLOAD=download () { \
	url=$$1 ;\
	output=$$2 ;\
	mkdir -p "$$(dirname "$$output")" ;\
	curl -L --fail -o "$$output" "$$url" || \
	wget "$$url" -O "$$output" || { \
		$(RM) "$$output" ;\
		echo "Neither curl nor wget could get the document, sorry." ;\
		exit 1 ;\
	} ;\
}; download


# Symlinks
$(DEST_LINKS)/cudd: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/cudd-$(CUDD_VERSION) $@
$(DEST_LINKS)/yices: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/yices-$(YICES_VERSION) $@
$(DEST_LINKS)/z3: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/z3-$(Z3_COMMIT)/install $@
$(DEST_LINKS)/llvm: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/llvm-$(LLVM_VERSION)/install $@
$(DEST_LINKS)/apron: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/apron_$(APRON_VERSION)/install $@
$(DEST_LINKS)/ppl: $(MAKEFILE_LIST)
	ln -sf $(DEST_ABS)/ppl-$(PPL_VERSION)/installdir $@


#### CUDD Installation
$(DEST)/$(CUDD_ARCHIVE):
	$(DOWNLOAD) "ftp://vlsi.colorado.edu/pub/$(CUDD_ARCHIVE)" "$@"

$(DEST)/cudd-$(CUDD_VERSION)/cudd/cudd.h Â$(DEST)/cudd-$(CUDD_VERSION)/Makefile: $(DEST)/$(CUDD_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(CUDD_ARCHIVE)" && cd cudd-$(CUDD_VERSION) && patch -p1 < $(SOURCE)/cudd-$(CUDD_VERSION).patch
	touch -c $@

$(CUDD_COMPILED): $(DEST)/cudd-$(CUDD_VERSION)/cudd/cudd.h
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && $(MAKE) $(PARALLEL_MAKE_OPTS) $(CUDD_XCFLAGS)
# call "make testobj" from obj, otherwise it tries using -O6 and fails
# to generate symbols for inline functions.
	cd $(DEST)/cudd-$(CUDD_VERSION)/obj && $(MAKE) $(PARALLEL_MAKE_OPTS) $(CUDD_XCFLAGS) testobj
# Just check that the build went OK, its fast
	cd $(DEST)/cudd-$(CUDD_VERSION)/ && ./obj/testobj


#### Yices Installation
$(DEST)/$(YICES_ARCHIVE):
# TODO: find a clean way to have the user accept the licence
	$(DOWNLOAD) "http://yices.csl.sri.com/cgi-bin/yices-newdownload.cgi?file=$(YICES_ARCHIVE)&accept=I+accept" "$@"

$(YICES_COMPILED): $(DEST)/$(YICES_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(YICES_ARCHIVE)"
# override the timestamp set by tar
	touch -c "$@"

#### Z3 Installation
$(DEST)/$(Z3_ARCHIVE):
	$(DOWNLOAD) "https://github.com/Z3Prover/z3/archive/$(Z3_COMMIT).tar.gz" "$@"

$(DEST)/z3-$(Z3_COMMIT)/README: $(DEST)/$(Z3_ARCHIVE)
	tar -x -C $(DEST) -zvf $(DEST)/$(Z3_ARCHIVE)

$(Z3_COMPILED): $(DEST)/z3-$(Z3_COMMIT)/README
	cd $(DEST)/z3-$(Z3_COMMIT)/ && \
		python2 scripts/mk_make.py --prefix $(DEST_ABS)/z3-$(Z3_COMMIT)/install/ && \
		cd build && \
		$(MAKE) $(PARALLEL_MAKE_OPTS) 
	cd $(DEST)/z3-$(Z3_COMMIT)/build && $(MAKE) install

#### LLVM + Clang Installation
$(DEST)/$(LLVM_ARCHIVE):
	$(DOWNLOAD) "http://llvm.org/releases/$(LLVM_VERSION)/$(LLVM_ARCHIVE)" "$@"

$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac: $(DEST)/$(LLVM_ARCHIVE)
	-mkdir $(DEST)/llvm-$(LLVM_VERSION)
	$(TAR) --strip-components=1 -C $(DEST)/llvm-$(LLVM_VERSION) -xJvf $(DEST)/$(LLVM_ARCHIVE)
	touch -c "$@"

$(LLVM_COMPILED) $(CLANG_COMPILED): $(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt
	mkdir -p $(DEST)/llvm-$(LLVM_VERSION).obj && cd $(DEST)/llvm-$(LLVM_VERSION).obj && \
		../llvm-$(LLVM_VERSION)/configure --enable-bindings=none  \
		--prefix=$(DEST_ABS)/llvm-$(LLVM_VERSION)/install && \
		REQUIRES_RTTI=1 $(MAKE) -j8 $(PARALLEL_MAKE_OPTS) && $(MAKE) install

# clang
$(DEST)/$(CLANG_ARCHIVE):
	$(DOWNLOAD) 'http://llvm.org/releases/$(LLVM_VERSION)/$(CLANG_ARCHIVE)' "$@"

$(DEST)/llvm-$(LLVM_VERSION)/tools/clang/README.txt: $(DEST)/$(CLANG_ARCHIVE) \
		$(DEST)/llvm-$(LLVM_VERSION)/autoconf/configure.ac
	-$(RM) -r $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/
	-mkdir $(DEST)/llvm-$(LLVM_VERSION)/tools/clang/
	$(TAR) --strip-components=1 -C $(DEST)/llvm-$(LLVM_VERSION)/tools/clang -xJvf $(DEST)/$(CLANG_ARCHIVE)
	touch -c "$@"

#### Apron
$(DEST)/$(APRON_ARCHIVE):
	$(DOWNLOAD) 'http://forge.imag.fr/frs/download.php/702/$(APRON_ARCHIVE)' "$@"

$(DEST)/apron_$(APRON_VERSION)/apron/README: $(DEST)/$(APRON_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(DEST_ABS)/$(APRON_ARCHIVE)"
	cd $(DEST)/apron_$(APRON_VERSION)
	touch -c "$@"

APRON_HAS_SHARED=1
ifeq ($(OS), Darwin)
APRON_HAS_SHARED=
else
ifeq ($(OS), Cygwin)
APRON_HAS_SHARED=
endif
endif

ifeq ($(PPL_ENABLED), 1)
APRON_PPL_DEP=install-ppl
else
APRON_PPL_DEP=
endif

ifeq ($(OPT_OCT_ENABLED), 1)
APRON_OPT_OCT_DEP=$(DEST)/apron_$(APRON_VERSION)/optoctagons/README
else
APRON_OPT_OCT_DEP=
endif

$(DEST)/apron_$(APRON_VERSION)/optoctagons/README: $(DEST)/apron_$(APRON_VERSION)/apron/README
	cd $(DEST) && rm -rf OptOctagon && git clone https://github.com/eth-srl/OptOctagon
	cp -a $(DEST)/OptOctagon/optoctagons $(DEST)/apron_$(APRON_VERSION)/

# DM Trying to use ocamlfind when unavailable causes numerous problems.
$(APRON_COMPILED): $(APRON_PPL_DEP) $(APRON_OPT_OCT_DEP) $(DEST)/apron_$(APRON_VERSION)/apron/README
	sed -e 's?^\(APRON_PREFIX *= *\).*$$?\1 $(DEST_ABS)/apron_$(APRON_VERSION)/install?' \
	    -e 's?^\(PPL_PREFIX *= *\).*$$?\1 $(DEST_ABS)/ppl-$(PPL_VERSION)/installdir/?' \
	    -e 's?^\(MLGMPIDL_PREFIX *= *\).*$$?\1 $(DEST_ABS)/apron_$(APRON_VERSION)/install?' \
	    -e 's?^ *\(#\|\) *HAS_PPL *=.*$$?HAS_PPL = $(PPL_ENABLED)?' \
		-e 's/^OCAMLFIND *=.*/OCAMLFIND =/' \
		-e 's/^HAS_SHARED *=.*/HAS_SHARED = $(APRON_HAS_SHARED)/' \
		-e 's/^HAS_OCAML *=.*/HAS_OCAML =/' \
		-e 's/^HAS_OCAMLOPT *=.*/HAS_OCAMLOPT =/' \
		-e 's/^# *HAS_PPL *=.*/HAS_PPL = $(PPL_ENABLED)/' \
		-e 's@^GMP_PREFIX *=.*@GMP_PREFIX = $(GMP_PREFIX)@' \
		-e 's@^MPFR_PREFIX *=.*@MPFR_PREFIX = $(MPFR_PREFIX)@' \
		< $(DEST)/apron_$(APRON_VERSION)/Makefile.config.model \
		> $(DEST)/apron_$(APRON_VERSION)/Makefile.config
	cd $(DEST)/apron_$(APRON_VERSION) && $(MAKE) $(PARALLEL_MAKE_OPTS) && $(MAKE) install		

#### PPL Library
$(DEST)/$(PPL_ARCHIVE):
	$(DOWNLOAD) 'ftp://ftp.cs.unipr.it/pub/ppl/releases/$(PPL_VERSION)/$(PPL_ARCHIVE)' "$@"

$(DEST)/ppl-$(PPL_VERSION)/README: $(DEST)/$(PPL_ARCHIVE)
	cd $(DEST) && $(TAR) xzvf "$(DEST_ABS)/$(PPL_ARCHIVE)"
	touch -c "$@"

$(PPL_COMPILED): $(DEST)/ppl-$(PPL_VERSION)/README
	cd $(DEST)/ppl-$(PPL_VERSION)/ && \
		./configure \
			--prefix $(DEST_ABS)/ppl-$(PPL_VERSION)/installdir/ \
			--with-gmp=$(GMP_PREFIX) \
			--enable-interfaces=c,c++ && \
		$(MAKE) $(PARALLEL_MAKE_OPTS) && $(MAKE) install
