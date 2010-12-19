/* $Id: anydim.cc,v 1.2 2010-12-19 14:59:17 grahn Exp $
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

#if 0

ff d8 ff e0 00 10 4a 46 49 46 00 01 01 00 00 01
00 01 00 00 ff fe 00 55 43 52 45 41 54 4f 52 3a
20 58 56 20 56 65 72 73 69 6f 6e 20 33 2e 31 30
61 20 20 52 65 76 3a 20 31 32 2f 32 39 2f 39 34
20 28 77 2f 20 70 61 74 63 68 65 73 29 20 20 51
75 61 6c 69 74 79 20 3d 20 37 31 2c 20 53 6d 6f
6f 74 68 69 6e 67 20 3d 20 30 0a ff db 00 43 00
09 06 07 08 07 06 09 08 08 08 0a 0a 09 0b 0e 17
0f 0e 0d 0d 0e 1c 14 15 11 17 22 1e 23 23 21 1e
20 20 25 2a 35 2d 25 27 32 28 20 20 2e 3f 2f 32
37 39 3c 3c 3c 24 2d 42 46 41 3a 46 35 3b 3c 39
ff db 00 43 01 0a 0a 0a 0e 0c 0e 1b 0f 0f 1b 39
26 20 26 39 39 39 39 39 39 39 39 39 39 39 39 39
39 39 39 39 39 39 39 39 39 39 39 39 39 39 39 39

ff d8 ff e1 38 45 45 78 69 66 00 00 49 49 2a 00
08 00 00 00 0c 00 0e 01 02 00 20 00 00 00 9e 00
00 00 0f 01 02 00 14 00 00 00 be 00 00 00 10 01
02 00 07 00 00 00 d6 00 00 00 12 01 03 00 01 00
00 00 01 00 00 00 1a 01 05 00 01 00 00 00 ee 00
00 00 1b 01 05 00 01 00 00 00 f6 00 00 00 28 01
03 00 01 00 00 00 02 00 00 00 31 01 02 00 08 00
00 00 fe 00 00 00 32 01 02 00 14 00 00 00 1e 01
00 00 13 02 03 00 01 00 00 00 02 00 00 00 69 87
04 00 01 00 00 00 26 02 00 00 a5 c4 07 00 04 01
00 00 32 01 00 00 96 04 00 00 4f 4c 59 4d 50 55
53 20 44 49 47 49 54 41 4c 20 43 41 4d 45 52 41
20 20 20 20 20 20 20 20 20 00 4f 4c 59 4d 50 55
53 20 43 4f 52 50 4f 52 41 54 49 4f 4e 00 00 00
00 00 43 37 37 30 55 5a 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 48 00 00 00 01 00
00 00 48 00 00 00 01 00 00 00 76 37 37 32 2d 38
32 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 32 30 30 39 3a 30
31 3a 30 36 20 32 32 3a 31 34 3a 32 37 00 50 72

#endif

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
