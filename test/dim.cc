/**
 * $Id: dim.cc,v 1.4 2011-01-06 12:13:38 grahn Exp $
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 * These tests assume certain image files exist and are 48 × 21 pixels.
 * If they don't, the tests flag ERROR rather than PASS/FAIL.
 * 
 */
#include <anydim.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <errno.h>

#include <orchis.h>

using orchis::TC;

using std::string;
using std::vector;

namespace {

    /**
     * Read all of non-empty 'file' into 'data', or throw an exception.
     */
    void read(vector<uint8_t>& data, const string& file)
    {
	std::ifstream in(file.c_str());
	char buf[4096];
	while(in) {
	    in.read(buf, sizeof buf);
	    const uint8_t* ubuf = reinterpret_cast<const uint8_t*>(buf);
	    data.insert(data.end(), ubuf, ubuf + in.gcount());
	}
	if(in.bad()) throw errno;
	if(data.empty()) throw "empty image file";
    }

    void test(const string& file,
	      const string& mime,
	      const unsigned width,
	      const unsigned height)
    {
	vector<uint8_t> img;
	read(img, file);

	const uint8_t* a = &img[0];
	const uint8_t* const end = a + img.size();

	anydim::AnyDim dim;
	while(a!=end && dim.undecided()) {
	    dim.feed(a, a+1);
	    ++a;
	}
	orchis::assert_eq(dim.bad(), false);
	orchis::assert_eq(dim.mime(), mime);
	orchis::assert_eq(dim.width, width);
	orchis::assert_eq(dim.height, height);
    }


    void test(const string& file,
	      const string& mime)
    {
	test(file, mime, 48, 21);
    }
}

namespace pnm {
    void ppm(TC) { test("test/anydim.ppm", "image/x-portable-pixmap"); }
    void ppm_raw(TC) { test("test/anydim.raw.ppm", "image/x-portable-pixmap"); }
    void pgm(TC) { test("test/anydim.pgm", "image/x-portable-graymap"); }
    void pbm(TC) { test("test/anydim.pbm", "image/x-portable-bitmap"); }
}

namespace png {
    void test(TC) { ::test("test/anydim.png", "image/png"); }
}

namespace jfif {
    void plain(TC) { test("test/anydim.jpg", "image/jpeg"); }
    void gray(TC) { test("test/anydim.gray.jpg", "image/jpeg"); }
    void progressive(TC) { test("test/anydim.prog.jpg", "image/jpeg"); }
}

namespace garbage {

    /* 10K of garbage should be enough to decide there's no
     * image here.
     */
    void test(TC)
    {
	anydim::AnyDim dim;
	uint8_t a[1];
	for(int i=0; i<10000; ++i) {
	    a[0] = i;
	    dim.feed(a, a+1);
	    orchis::assert_(dim.undecided() || dim.bad());
	}
	orchis::assert_(dim.bad());
    }

    void empty(TC)
    {
	anydim::AnyDim dim;
	dim.eof();
	orchis::assert_(dim.bad());
    }
}
