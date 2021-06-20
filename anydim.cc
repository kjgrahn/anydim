/* $Id: anydim.cc,v 1.25 2011-01-06 19:48:19 grahn Exp $
 *
 * Copyright (c) 2010, 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#include "anydim.h"
#include "jfif.h"
#include "tiff/tiff.h"
#include "orientation.h"

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

    bool is_sof(const jfif::Segment& seg)
    {
	switch(seg.marker) {
	case jfif::marker::SOF0:
	case jfif::marker::SOF1:
	case jfif::marker::SOF2:
	case jfif::marker::SOF9:
	case jfif::marker::SOFa:
	    return true;
	default:
	    return false;
	}
    }

    bool is_app1(const jfif::Segment& seg)
    {
	return seg.marker == jfif::marker::APP1;
    }
}


using anydim::JpegDim;

const char* JpegDim::mime() const
{
    return "image/jpeg";
}

JpegDim::JpegDim(bool use_exif)
    : decoder {new jfif::Decoder},
      use_exif {use_exif}
{}

JpegDim::~JpegDim()
{
    delete decoder;
}

/**
 * Consume another chunk of data, [a, b).
 */
void JpegDim::feed(const uint8_t *a, const uint8_t *b)
{
    if(state_==BAD) return;

    try {
	decoder->feed(a, b);
	auto sof = std::find_if(begin(decoder->v), end(decoder->v), is_sof);
	if(sof!=end(decoder->v)) {
	    const uint8_t* a = sof->v.data();
	    const auto b = a + sof->v.size();
	    if(b-a < 5) {
		state_ = BAD;
	    }
	    else {
		a++;
		height = eat16(a);
		width = eat16(a);
		state_ = GOOD;

		if (!use_exif) return;

		/* To avoid scanning the whole JFIF in case there's no
		 * EXIF information, we assume that APP1 appears
		 * before SOFn.
		 */
		auto app1 = std::find_if(begin(decoder->v), end(decoder->v), is_app1);
		if(app1==end(decoder->v)) return;

		const tiff::File tiff {app1->v};
		Orientation{tiff}.adjust(width, height);
	    }
	}
    }
    catch (const jfif::Decoder::Error&) {
	state_ = BAD;
    }
    catch (const tiff::Error&) {
	// If TIFF/Exif is broken, we can just ignore it
    }
}


void JpegDim::eof()
{
    try {
	decoder->end();
    }
    catch (const jfif::Decoder::Error&) {
	state_ = BAD;
    }
}


using anydim::PngDim;

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


using anydim::AnyDim;

AnyDim::AnyDim(bool use_exif)
    : mime_("image")
{
    dims_.push_back(new JpegDim {use_exif});
    dims_.push_back(new PngDim);
    dims_.push_back(new PnmDim);
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
