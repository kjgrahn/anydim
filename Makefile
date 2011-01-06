# $Id: Makefile,v 1.5 2011-01-06 10:18:59 grahn Exp $
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

.PHONY: check checkv
check: tests
	./tests
checkv: tests
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
	$(RM) Makefile.bak core TAGS

love:
	@echo "not war?"

# DO NOT DELETE

anydim.o: anydim.h
main.o: anydim.h
test/dim.o: anydim.h
