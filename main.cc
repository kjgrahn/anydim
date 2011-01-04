/* $Id: main.cc,v 1.3 2011-01-04 00:34:19 grahn Exp $
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#include <iostream>
#include <fstream>

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <getopt.h>

#include "anydim.h"


namespace {

    const char* version()
    {
	return "unpublished";
    }
}

namespace {

    bool jpegdim(std::ostream& os,
		 const char* const file,
		 bool emit_filename)
    {
	if(emit_filename) {
	    os << file << ' ';
	}

	std::ifstream in(file);

	anydim::JpegDim dim;
	char buf[4096];
	while(in && dim.undecided()) {
	    in.read(buf, sizeof buf);
	    const uint8_t* ubuf = reinterpret_cast<const uint8_t*>(buf);
	    dim.feed(ubuf, ubuf + in.gcount());
	}
	if(in) dim.eof();

	if(in.bad()) {
	    os << "ERROR: " << std::strerror(errno) << '\n';
	    return false;
	}

	if(dim.bad()) {
	    os << "ERROR: not a valid " << dim.mime() << " file\n";
	    return false;
	}

	os << dim.width << ' ' << dim.height << '\n';
	return true;
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
	    std::cout << "anydim " << version() << '\n'
		      << "Copyright (c) 2011 Jörgen Grahn\n";
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

    int rc = 0;

    for(int i=optind; i<argc; i++) {
	if(!jpegdim(std::cout, argv[i], (argc-optind) > 1)) {
	    rc = 1;
	}
    }

    return rc;
}
