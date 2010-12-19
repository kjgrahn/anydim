# $Id: Makefile,v 1.1 2010-12-19 20:46:42 grahn Exp $
#
# Makefile
#
# Copyright (c) 2010 J�rgen Grahn
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

anydim: anydim.o
	$(CXX) -o $@ anydim.o

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
