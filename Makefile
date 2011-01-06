# $Id: Makefile,v 1.6 2011-01-06 12:03:25 grahn Exp $
#
# Makefile
#
# Copyright (c) 2010, 2011 Jörgen Grahn
# All rights reserved.

SHELL=/bin/sh
INSTALLBASE=/usr/local
CXXFLAGS=-Wall -Wextra -pedantic -std=c++98 -g -O3

.PHONY: all
all: anydim

.PHONY: install
install: anydim
install: anydim.1
	install -m755 anydim $(INSTALLBASE)/bin/
	install -m644 anydim.1 $(INSTALLBASE)/man/man1/

GENIMAGES=test/anydim.prog.jpg test/anydim.gray.jpg test/anydim.jpg test/anydim.png test/anydim.pbm test/anydim.pgm test/anydim.raw.ppm

.PHONY: check checkv
check: tests
check: test/anydim.ppm
check: $(GENIMAGES)
	./tests

checkv: tests
checkv: test/anydim.ppm
checkv: $(GENIMAGES)
	valgrind -q ./tests

anydim: main.o libanydim.a
	$(CXX) $(CXXFLAGS) -o $@ main.o -L. -lanydim

test.cc: libtest.a
	testicle -o$@ $^

tests: test.o libanydim.a libtest.a
	$(CXX) -o $@ test.o -L. -ltest -lanydim

libanydim.a: anydim.o
	$(AR) -r $@ $^

libtest.a: test/dim.o
	$(AR) -r $@ $^

test/%.o : test/%.cc
	$(CXX) -I. $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

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

depend:
	makedepend -- $(CXXFLAGS) $(CPPFLAGS) -- -Y -I. *.cc test/*.cc

.PHONY: clean
clean:
	$(RM) anydim tests
	$(RM) test.cc
	$(RM) *.o test/*.o
	$(RM) *.a
	$(RM) $(GENIMAGES)
	$(RM) Makefile.bak core TAGS

love:
	@echo "not war?"

# DO NOT DELETE

anydim.o: anydim.h
main.o: anydim.h
test/dim.o: anydim.h
