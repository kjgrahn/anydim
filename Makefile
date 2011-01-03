# $Id: Makefile,v 1.4 2011-01-03 23:44:21 grahn Exp $
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

libanydim.a: anydim.o
	$(AR) -r $@ $^

.PHONY: tags
tags: TAGS
TAGS:
	etags *.cc *.h

depend:
	makedepend -- $(CXXFLAGS) $(CPPFLAGS) -- -Y -I. *.cc

.PHONY: clean
clean:
	$(RM) anydim
	$(RM) *.o
	$(RM) Makefile.bak core TAGS

love:
	@echo "not war?"

# DO NOT DELETE

anydim.o: anydim.h
main.o: anydim.h
