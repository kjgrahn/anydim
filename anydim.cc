/* $Id: anydim.cc,v 1.19 2011-01-03 23:44:21 grahn Exp $
 *
 * Copyright (c) 2010, 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#include "anydim.h"


namespace {

    static unsigned eat16(const uint8_t*& p)
    {
	unsigned n = *p++ << 8;
	n |= *p++;
	return n;
    }

}


namespace anydim {

    const char* JpegDim::mime() const
    {
	return "image/jpeg";
    }

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
