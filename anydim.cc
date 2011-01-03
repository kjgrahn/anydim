/* $Id: anydim.cc,v 1.18 2011-01-03 23:27:45 grahn Exp $
 *
 * Copyright (c) 2010, 2011 J�rgen Grahn
 * All rights reserved.
 *
 */
#include <vector>

namespace {

    static unsigned eat16(const uint8_t*& p)
    {
	unsigned n = *p++ << 8;
	n |= *p++;
	return n;
    }

    /**
     * JPEG dimension decoder.
     *
     * The JPEG standard is not available for free, so here's a short summary
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
     *
     * Either kind of segment may be followed by entropy-encoded data,
     * containing no ff octets except followed by an 00 octet.
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
     * The same goes for at least SOF1, SOF9 and SOF10.
     *
     * Refs: <http://en.wikipedia.org/wiki/JPEG#Syntax_and_structure>,
     * and some googling, and the libjpeg sources.
     */
    class JpegDim {
    public:
	JpegDim()
	    : state_(UNDECIDED),
	      in_entropy_(false),
	      seen_(0)
	{}

	const char* mime() const;

	void feed(const uint8_t *a, const uint8_t *b);
	void eof();

	bool bad() const { return state_==BAD; }
	bool undecided() const { return state_==UNDECIDED; }

	unsigned width;
	unsigned height;

    private:
	std::vector<uint8_t> mem_;

	enum Marker {
	    SOI  = 0xffd8,
	    SOF0 = 0xffc0,
	    SOF1 = 0xffc1,
	    SOF2 = 0xffc2,
	    SOF9 = 0xffc9,
	    SOFa = 0xffca,
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


    /**
     * Consume another chunk of data, [a, b).
     */
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
		seen_ += 2;
	    }
	    else {
		/* marker, length, data, [entropy] */
		if(b-a<2) {
		    a-=2;
		    break;
		}
		const int n = eat16(a) - 2;
		if(n<0) {
		    state_ = BAD;
		    break;
		}
		if(b-a<n) {
		    a-=2+2;
		    break;
		}

		if(m==SOF0 ||
		   m==SOF1 ||
		   m==SOF2 ||
		   m==SOF9 ||
		   m==SOFa) {
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

		seen_ += 2+2+n;
	    }

	    eat_entropy(a, b);
	}

	if(a!=b) {
	    mem_.insert(mem_.end(), a, b);
	}
    }


    void JpegDim::eof()
    {
	if(state_==UNDECIDED) state_ = BAD;
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


}
