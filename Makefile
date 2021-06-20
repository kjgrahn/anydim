# $Id: Makefile,v 1.9 2011-02-05 16:11:05 grahn Exp $
#
# Makefile
#
# Copyright (c) 2010, 2011 Jörgen Grahn
# All rights reserved.

SHELL=/bin/bash
INSTALLBASE=/usr/local
CXXFLAGS=-Wall -Wextra -pedantic -std=c++14 -g -Os
ARFLAGS=rTP

.PHONY: all
all: anydim
all: tests

.PHONY: install
install: anydim
install: anydim.1
install: libanydim.a
install: anydim.h
	install -m755 anydim $(INSTALLBASE)/bin/
	install -m644 anydim.1 $(INSTALLBASE)/man/man1/
	install -m644 libanydim.a $(INSTALLBASE)/lib
	install -m644 anydim.h $(INSTALLBASE)/include

GENIMAGES=test/anydim.prog.jpg test/anydim.gray.jpg test/anydim.jpg test/anydim.png test/anydim.pbm test/anydim.pgm test/anydim.raw.ppm

.PHONY: check checkv
check: tests
check: test/anydim.ppm
check: $(GENIMAGES)
	./tests

checkv: tests
checkv: test/anydim.ppm
checkv: $(GENIMAGES)
	valgrind -q ./tests -v

anydim: main.o libanydim.a
	$(CXX) $(CXXFLAGS) -o $@ main.o -L. -lanydim

test.cc: libtest.a
	orchis -o$@ $^

tests: test.o libanydim.a libtest.a
	$(CXX) -o $@ test.o -L. -ltest -lanydim

libanydim.a: anydim.o
libanydim.a: pnmdim.o
libanydim.a: jfif.o
libanydim.a: tiff/tiff.o
libanydim.a: tiff/range.o
	$(AR) $(ARFLAGS) $@ $^

libtest.a: test/dim.o
libtest.a: test/jfif.o
libtest.a: test/hexread.o
libtest.a: test/tiff.o
	$(AR) $(ARFLAGS) $@ $^

test/%.o: CPPFLAGS+=-I.

test/anydim.jpg: test/anydim.ppm
	cjpeg -outfile $@ $^
test/anydim.prog.jpg: test/anydim.ppm
	cjpeg -progressive -outfile $@ $^
test/anydim.gray.jpg: test/anydim.ppm
	cjpeg -grayscale -outfile $@ $^
test/anydim.pbm: test/anydim.pgm
	pgmtopbm >$@ $^
test/anydim.pgm: test/anydim.ppm
	ppmtopgm >$@ $^
test/anydim.raw.ppm: test/anydim.ppm
	ppmbrighten -v 100 >$@ $^
test/anydim.png: test/anydim.ppm
	pnmtopng >$@ $^

.PHONY: tags
tags: TAGS
TAGS:
	etags *.cc *.h

.PHONY: clean
clean:
	$(RM) anydim tests
	$(RM) test.cc
	$(RM) *.o {test,tiff}/*.o
	$(RM) *.a
	$(RM) $(GENIMAGES)
	$(RM) Makefile.bak core TAGS
	$(RM) -r dep

love:
	@echo "not war?"

$(shell mkdir -p dep/{test,tiff})
DEPFLAGS=-MT $@ -MMD -MP -MF dep/$*.Td
COMPILE.cc=$(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

%.o: %.cc
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	@mv dep/$*.{Td,d}

dep/%.d: ;
dep/tiff/%.d: ;
dep/test/%.d: ;
-include dep/*.d
-include dep/tiff/*.d
-include dep/test/*.d
