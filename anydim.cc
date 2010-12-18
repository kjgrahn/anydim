/* $Id: anydim.cc,v 1.1 2010-12-18 20:44:33 grahn Exp $
 *
 * Copyright (c) 2010 Jörgen Grahn
 * All rights reserved.
 *
 */

#include <iostream>
#include <fstream>

#include <cstdlib>
#include <getopt.h>

namespace {

    const char* version()
    {
	return "unpublished";
    }


    void jpegdim(std::ostream& os,
		 const char* const file)
    {
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
