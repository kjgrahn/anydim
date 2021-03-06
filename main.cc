/* $Id: main.cc,v 1.8 2011-01-08 12:25:43 grahn Exp $
 *
 * Copyright (c) 2011, 2021 J�rgen Grahn
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

    bool dimensions(std::ostream& os,
		    const char* const file,
		    bool do_mime,
		    bool do_filename,
		    bool do_exif,
		    bool do_landscape)
    {
	if(do_filename && file) {
	    os << file << ' ';
	}

	std::ifstream inf;
	if(file) inf.open(file);
	std::istream& in = file? inf: std::cin;

	anydim::AnyDim dim {do_exif};
	char buf[4096];
	while(in && dim.undecided()) {
	    in.read(buf, sizeof buf);
	    const uint8_t* ubuf = reinterpret_cast<const uint8_t*>(buf);
	    dim.feed(ubuf, ubuf + in.gcount());
	}
	if(!in) dim.eof();

	if(in.bad()) {
	    os << "ERROR: " << std::strerror(errno) << '\n';
	    return false;
	}

	if(dim.bad()) {
	    os << "ERROR: not a valid " << dim.mime() << " file\n";
	    return false;
	}

	if(do_mime) {
	    os << dim.mime() << ' ';
	}

	auto a = dim.width;
	auto b = dim.height;
	if(a < b && do_landscape) std::swap(a, b);

	os << a << ' ' << b << '\n';
	return true;
    }
}


int main(int argc, char ** argv)
{
    using std::string;

    const string prog = argv[0];
    const string usage = string("usage: ")
	+ prog
	+ " [-i] [-H|-h] [--no-exif] [--landscape] file ...";
    const char optstring[] = "iHhLX";
    struct option long_options[] = {
	{"landscape", 0, 0, 'L'},
	{"no-exif", 0, 0, 'X'},
	{"version", 0, 0, 'v'},
	{"help", 0, 0, '!'},
	{0, 0, 0, 0}
    };

    int ch;
    bool do_mime = false;
    bool do_landscape = false;
    bool do_exif = true;
    char hflag = 0;
    while((ch = getopt_long(argc, argv,
			    optstring, &long_options[0], 0)) != -1) {
	switch(ch) {
	case 'i':
	    do_mime = true;
	    break;
	case 'L':
	    do_landscape = true;
	    do_exif = false;
	    break;
	case 'X':
	    do_exif = false;
	    break;
	case 'H':
	case 'h':
	    hflag = ch;
	    break;
	case '!':
	    std::cout << usage << '\n';
	    return 0;
	case 'v':
	    std::cout << "anydim 1.5\n"
		      << "Copyright (c) 2011, 2019, 2021 J�rgen Grahn\n";
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

    if(optind==argc) {
	if(!dimensions(std::cout, 0,
		       do_mime, false,
		       do_exif, do_landscape)) {
	    rc = 1;
	}
    }
    else {
	bool do_filenames = (argc-optind > 1);
	switch(hflag) {
	case 'h': do_filenames = false; break;
	case 'H': do_filenames = true; break;
	}

	for(int i=optind; i<argc; i++) {
	    if(!dimensions(std::cout, argv[i],
			   do_mime, do_filenames,
			   do_exif, do_landscape)) {
		rc = 1;
	    }
	}
    }

    return rc;
}
