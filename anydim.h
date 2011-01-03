/* -*- c++ -*-
 * $Id: anydim.h,v 1.2 2011-01-03 23:44:21 grahn Exp $
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#ifndef ANYDIM_ANYDIM_H
#define ANYDIM_ANYDIM_H

#include <vector>
#include <stdint.h>

namespace anydim {

    /**
     * Base class for dimension (width × height) decoders for
     * different image formats.
     *
     * The decoding is driven by a series of Dim::feed(a, b) followed by
     * Dim::eof(). You may want to stop decoding as soon as
     * !Dim::undecided() though.
     *
     * Dim::undecided() starts out true (we don't have the dimensions yet,
     * and don't know if we'll ever will), then flops, at which point
     * Dim::bad() is true or (width, height) is valid.
     *
     * There's also the file's MIME type, available as Dim::mime().
     */
    class Dim {
    public:
	Dim() : state_(UNDECIDED) {}
	virtual ~Dim() {}

	virtual const char* mime() const = 0;

	virtual void feed(const uint8_t *a, const uint8_t *b) = 0;
	virtual void eof() = 0;

	bool bad() const { return state_==BAD; }
	bool undecided() const { return state_==UNDECIDED; }

	unsigned width;
	unsigned height;

    protected:
	enum State { UNDECIDED, GOOD, BAD };
	State state_;
    };


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
    class JpegDim : public Dim {
    public:
	JpegDim()
	    : in_entropy_(false),
	      seen_(0)
	{}

	const char* mime() const;

	void feed(const uint8_t *a, const uint8_t *b);
	void eof();

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

	bool in_entropy_;
	unsigned seen_;

	void eat_entropy(const uint8_t *&a, const uint8_t *b);
    };

}
#endif
