/* $Id: anydim.cc,v 1.3 2010-12-19 20:48:10 grahn Exp $
 *
 * Copyright (c) 2010 Jörgen Grahn
 * All rights reserved.
 *
 */

#include <iostream>
#include <fstream>

#include <cstdlib>
#include <cassert>
#include <getopt.h>

namespace {

    const char* version()
    {
	return "unpublished";
    }

    struct Marker {
	explicit Marker(unsigned _n) : n(_n) {}
	unsigned n;
	bool valid() const { return (n>>8)==0xff && n!=0xff00; }
	bool end() const { return n==0xffd9; }
	bool variable() const;
    };

    bool Marker::variable() const
    {
	switch(n) {
	case 0xffd8:
	case 0xffd0:
	case 0xffd1:
	case 0xffd2:
	case 0xffd3:
	case 0xffd4:
	case 0xffd5:
	case 0xffd6:
	case 0xffd7:
	    return false;
	}
	return true;
    }

    std::ostream& operator<< (std::ostream& os,
			      const Marker& val)
    {
	char buf[5];
	std::sprintf(buf, "%04x", val.n);
	return os << buf;
    }

    unsigned eat8(std::istream& is)
    {
	int n = is.get();
	if(n==-1) throw "eof";
	return n;
    }

    unsigned eat16(std::istream& is)
    {
	int hi = is.get();
	int lo = is.get();
	if(lo==-1) throw "eof";
	unsigned n = hi << 8;
	n |= lo;
	return n;
    }

    void ignore(std::istream& is, unsigned n)
    {
	is.ignore(n);
	if(is.eof()) throw "eof";
    }

    Marker seek(std::istream& is)
    {
	while(1) {
	    unsigned ch;
	    while((ch = eat8(is)) != 0xff) {}
	    Marker marker((ch<<8) | eat8(is));
	    if(marker.valid()) return marker;
	}
    }

    void jpegdim(std::ostream& os,
		 const char* const file)
    {
	std::ifstream is(file);
	Marker marker(eat16(is));
	while(!marker.end()) {
	    if(!marker.valid()) {
		os << "...." << '\n';
		marker = seek(is);
	    }
	    else {
		os << marker << '\n';
		if(marker.variable()) {
		    os << "::::" << '\n';
		    unsigned n = eat16(is);
		    ignore(is, n-2);
		}
		marker = Marker(eat16(is));
	    }
	}
    }
}


int main(int argc, char ** argv)
{
    using std::string;

    const string prog = argv[0];
    const string usage = string("usage: ")
	+ prog
	+ " file ...";
    const char optstring[] = "+";
    struct option long_options[] = {
	{"version", 0, 0, 'v'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
    };

    int ch;
    while((ch = getopt_long(argc, argv,
			    optstring, &long_options[0], 0)) != -1) {
	switch(ch) {
	case 'h':
	    std::cout << usage << '\n';
	    return 0;
	case 'v':
	    std::cout << "proffbib " << version() << '\n'
		      << "Copyright (c) 2008--2009 Jörgen Grahn\n";
	    return 0;
	    break;
	case ':':
	case '?':
	    std::cerr << usage << '\n';
	    return 1;
	    break;
	default:
	    break;
	}
    }

    for(int i=optind; i<argc; i++) {
	jpegdim(std::cout, argv[i]);
    }

    return 0;
}
