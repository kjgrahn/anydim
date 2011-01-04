/* $Id: anydim.cc,v 1.23 2011-01-04 23:14:18 grahn Exp $
 *
 * Copyright (c) 2010, 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#include "anydim.h"

#include <algorithm>


namespace {

    static unsigned eat16(const uint8_t*& p)
    {
	unsigned n = *p++ << 8;
	n |= *p++;
	return n;
    }

    static unsigned eat32(const uint8_t*& p)
    {
	unsigned n = 0;
	n = (n<<8) | *p++;
	n = (n<<8) | *p++;
	n = (n<<8) | *p++;
	n = (n<<8) | *p++;
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
	if(state_==UNDECIDED || !mem_.empty()) {
	    state_ = BAD;
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
}


namespace {

    /* The PNG file signature and the initial
     * parts of the IHDR. Immediately after these
     * the image width and height come.
     */
    static const uint8_t pngintro[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
	0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52
    };
}


namespace anydim {

    const char* PngDim::mime() const
    {
	return "image/png";
    }

    void PngDim::feed(const uint8_t *a, const uint8_t *b)
    {
	if(state_==BAD) return;

	std::vector<uint8_t> ws;

	if(!mem_.empty()) {
	    std::swap(ws, mem_);
	    ws.insert(ws.end(), a, b);
	    a = &ws[0];
	    b = a + ws.size();
	}

	if(b-a < int(sizeof pngintro + 4 + 4)) {
	    mem_.insert(mem_.end(), a, b);
	    return;
	}

	if(!std::equal(pngintro, pngintro + sizeof pngintro, a)) {
	    state_ = BAD;
	    return;
	}

	state_ = GOOD;
	a += sizeof pngintro;
	width = eat32(a);
	height = eat32(a);
    }

    void PngDim::eof()
    {
	if(state_==UNDECIDED) state_ = BAD;
    }
}


using anydim::AnyDim;


AnyDim::AnyDim()
    : mime_("image")
{
    dims_.push_back(new JpegDim);
    dims_.push_back(new PngDim);
}


namespace {

    void del(anydim::Dim* d) { delete d; }

    struct feed {
	feed(const uint8_t *a, const uint8_t *b) : a(a), b(b) {}
	void operator() (anydim::Dim* dim) const { dim->feed(a, b); }
	const uint8_t* const a;
	const uint8_t* const b;
    };

    void eof(anydim::Dim* d) { d->eof(); }
}


AnyDim::~AnyDim()
{
    std::for_each(dims_.begin(), dims_.end(), del);
}


const char* AnyDim::mime() const
{
    return mime_;
}


void AnyDim::feed(const uint8_t *a, const uint8_t *b)
{
    std::for_each(dims_.begin(), dims_.end(),
		  ::feed(a, b));
    weed();
}

void AnyDim::eof()
{
    std::for_each(dims_.begin(), dims_.end(),
		  ::eof);
    weed();
    if(state_==UNDECIDED) state_ = BAD;
}

void AnyDim::weed()
{
    Dim* last_good = 0;
    unsigned notbad = 0;
    unsigned good = 0;
    for(std::vector<Dim*>::iterator i = dims_.begin();
	i!=dims_.end();
	++i) {

	Dim& dim = **i;
	if(!dim.bad()) {
	    ++notbad;
	    if(!dim.undecided()) {
		++good;
		last_good = &dim;
	    }
	}
    }

    if(!notbad) {
	state_ = BAD;
    }
    else if(good==1) {
	state_ = GOOD;
	width = last_good->width;
	height = last_good->height;
	mime_ = last_good->mime();
    }
}
