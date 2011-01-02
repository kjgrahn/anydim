/* $Id: anydim.cc,v 1.9 2011-01-02 21:44:04 grahn Exp $
 *
 * Copyright (c) 2010 Jörgen Grahn
 * All rights reserved.
 *
 */

#include <iostream>
#include <fstream>
#include <vector>

#include <cstdlib>
#include <cassert>
#include <getopt.h>

namespace {

    const char* version()
    {
	return "unpublished";
    }
}

namespace {

    /* The JPEG standard is not available for free, so here's a short summary
     * of the JIF format (including JFIF and whatever you call JIF+EXIF).
     *
     * A JIF file is a series of /segments/ introduced by /markers/.
     *
     * ff xx - marker for a fixed-size segment. All I've seen have xx in
     *         the range d0--d9 and have a fixed size zero
     *         (SOI, EOI and RSTn).
     *
     * ff xx - marker
     * nn nn - 2 + octet length of segment data
     * ...   - segment data
     * ...   - entropy-encoded data, containing no ff octets
     *         except followed by an 00 octet
     *
     * The file starts with a SOI followed by an APPn, and ends with
     * EOI.  The width and height of the image is in a SOF0 or (for
     * progressive JPEG) SOF2 segment, which both appear to be
     *
     * 1 octet  something
     * 2 octets height
     * 2 octets width
     * ...
     *
     * Refs: <http://en.wikipedia.org/wiki/JPEG#Syntax_and_structure>
     * and some googling.
     */

    struct Marker {
	explicit Marker(unsigned _n) : n(_n) {}
	unsigned n;

	bool operator== (const Marker& other) const {
	    return n==other.n;
	}

	bool valid() const { return (n>>8)==0xff && n!=0xff00; }
	bool end() const { return *this==EOI; }
	bool variable() const;

	static const Marker SOI;
	static const Marker SOF0;
	static const Marker SOF2;
	static const Marker DHT;
	static const Marker DQT;
	static const Marker DRI;
	static const Marker SOS;
	static const Marker APP0;
	static const Marker APP1;
	static const Marker COM;
	static const Marker EOI;
    };

    const Marker Marker::SOI (0xffd8);
    const Marker Marker::SOF0(0xffc0);
    const Marker Marker::SOF2(0xffc2);
    const Marker Marker::DHT (0xffc4);
    const Marker Marker::DQT (0xffdb);
    const Marker Marker::DRI (0xffdd);
    const Marker Marker::SOS (0xffda);
    const Marker Marker::APP0(0xffe0);
    const Marker Marker::APP1(0xffe1);
    const Marker Marker::COM (0xfffe);
    const Marker Marker::EOI (0xffd9);

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

    unsigned eat16(const uint8_t*& p)
    {
	unsigned n = *p++ << 8;
	n <<= 8;
	n |= *p++;
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

    class JpegDim {
    public:
	JpegDim();
	void feed(const uint8_t *a, const uint8_t *b);
	void eof();

	bool bad() const;
	bool measured() const;

	unsigned width;
	unsigned height;

    private:
	std::vector<uint8_t> mem_;

	enum Marker {
	    SOI  = 0xffd8,
	    SOF0 = 0xffc0,
	    SOF2 = 0xffc2,
	    DHT  = 0xffc4,
	    DQT  = 0xffdb,
	    DRI  = 0xffdd,
	    SOS  = 0xffda,
	    APP0 = 0xffe0,
	    APP1 = 0xffe1,
	    COM  = 0xfffe,
	    EOI  = 0xffd9
	};

	enum State { UNDECIDED, GOOD, BAD };
	State state_;
	bool in_entropy_;
	unsigned seen_;

	void eat_entropy(const uint8_t *&a, const uint8_t *b);
    };


    JpegDim::JpegDim()
	: state_(UNDECIDED),
	  in_entropy_(false),
	  seen_(0)
    {}

    void JpegDim::feed(const uint8_t *a, const uint8_t *b)
    {
	if(state_==BAD) return;

	std::vector<uint8_t> ws;

	if(!mem_.empty()) {
	    std::swap(ws, mem_);
	    ws.insert(ws.end(), a, b);
	    a = &ws[0];
	    b = a + ws.size();
	}

	if(in_entropy_)	eat_entropy(a, b);

	while(!in_entropy_) {

	    if(b-a<2) break;
	    const unsigned m = eat16(a);
	    if(seen_==0 && m!=SOI) {
		state_ = BAD;
		break;
	    }
	    if(m>>8 != 0xff) {
		state_ = BAD;
		break;
	    }

	    if(0xffd0 <= m && m <= 0xffd9) {
		/* just a marker */
		continue;
	    }

	    /* marker, length, data */
	    if(b-a<2) {
		a-=2;
		break;
	    }
	    unsigned n = eat16(a);
	    if(n<2) break;
	    n-=2;
	    if(b-a<n) {
		a-=2+2;
		break;
	    }

	    if(m==SOF0 || m==SOF2) {
		if(n<5) {
		    state_ = BAD;
		    break;
		}
		state_ = GOOD;
		a++;
		height = eat16(a);
		width = eat16(a);
		a += n<1+2+2;
	    }
	    else {
		a += n;
	    }

	    eat_entropy(a, b);
	}

	if(a!=b) {
	    mem_.insert(mem_.end(), a, b);
	}
    }


    /**
     * Consume entropy-encoded data from [a, b) until all is consumed
     * or a segment is found. Sets 'in_entropy_' and updates 'a' and 'seen_'.
     */
    void JpegDim::eat_entropy(const uint8_t *&a, const uint8_t *b)
    {
	const uint8_t* const c = a;

	in_entropy_ = true;
	while(a!=b) {
	    if(*a++==0xff) {
		if(a==b) {
		    a--;
		    break;
		}
		else if(*a) {
		    a--;
		    in_entropy_ = false;
		    break;
		}
	    }
	}

	seen_ += a-c;
    }


    void jpegdim(std::istream& is,
		 unsigned& width,
		 unsigned& height)
    {
	Marker marker(eat16(is));
	while(!marker.end()) {
	    if(!marker.valid()) {
		marker = seek(is);
	    }
	    else {
		if(marker==Marker::SOF0 ||
		   marker==Marker::SOF2) {
		    unsigned n = eat16(is);
		    ignore(is, 1);
		    height = eat16(is);
		    width = eat16(is);
		    return;
		    ignore(is, n-2-1-4);
		}
		else if(marker.variable()) {
		    unsigned n = eat16(is);
		    ignore(is, n-2);
		}
		marker = Marker(eat16(is));
	    }
	}
    }

    bool jpegdim(std::ostream& os,
		 const char* const file,
		 bool emit_filename)
    {
	if(emit_filename) {
	    os << file << ' ';
	}

	std::ifstream is(file);
	unsigned width = 0;
	unsigned height = 0;

	try {
	    jpegdim(is, width, height);
	    os << width << ' ' << height;
	}
	catch(const char*) {
	}
	os << '\n';

	is.close();
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

    int rc = 0;

    for(int i=optind; i<argc; i++) {
	if(!jpegdim(std::cout, argv[i], (argc-optind) > 1)) {
	    rc = 1;
	}
    }

    return rc;
}
