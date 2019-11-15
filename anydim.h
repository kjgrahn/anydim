/* -*- c++ -*-
 * $Id: anydim.h,v 1.9 2011-02-05 16:11:05 grahn Exp $
 *
 * anydim - print width and height of images
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#ifndef ANYDIM_ANYDIM_H
#define ANYDIM_ANYDIM_H

#include <vector>
#include <string>
#include <stdint.h>

namespace anydim {

    std::string version();

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
	virtual ~Dim() = default;

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
    class JpegDim final: public Dim {
    public:
	JpegDim()
	    : in_entropy_(false),
	      seen_(0)
	{}

	const char* mime() const override;

	void feed(const uint8_t *a, const uint8_t *b) override;
	void eof() override;

    private:
	std::vector<uint8_t> mem_;

	enum Marker {
	    SOI  = 0xffd8,  DQT  = 0xffdb,
	    SOF0 = 0xffc0,  DRI  = 0xffdd,
	    SOF1 = 0xffc1,  SOS  = 0xffda,
	    SOF2 = 0xffc2,  APP0 = 0xffe0,
	    SOF9 = 0xffc9,  APP1 = 0xffe1,
	    SOFa = 0xffca,  COM  = 0xfffe,
	    DHT  = 0xffc4,  EOI  = 0xffd9
	};

	bool in_entropy_;
	unsigned seen_;

	void eat_entropy(const uint8_t *&a, const uint8_t *b);
    };


    /**
     * PNG dimension decoder.
     *
     * See RFC 2083. The 16 octets before the width and height are
     * happily fixed and constant. We never bother to parse the rest.
     */
    class PngDim final: public Dim {
    public:
	PngDim() {}

	const char* mime() const override;

	void feed(const uint8_t *a, const uint8_t *b) override;
	void eof() override;

    private:
	std::vector<uint8_t> mem_;
    };


    /**
     * PNM (PPM/PGM/PBM) dimension decoder.
     *
     * See ppm(5), pgm(5) and pbm(5) for the file format.
     */
    class PnmDim final: public Dim {
    public:
	PnmDim();

	const char* mime() const override;

	void feed(const uint8_t *a, const uint8_t *b) override;
	void eof() override;

    private:
	void feed(char ch);
	bool comment_;
	unsigned pnmstate_;
	const char* mime_;
    };


    /**
     * The actual 'any' dimension decoder -- decode using all available
     * decoders in parallel until (presumably) zero or one turns
     * !(undecided || bad).
     *
     */
    class AnyDim final: public Dim {
    public:
	AnyDim();
	~AnyDim();

	const char* mime() const override;

	void feed(const uint8_t *a, const uint8_t *b) override;
	void eof() override;

    private:
	std::vector<Dim*> dims_;
	const char* mime_;

	void weed();
    };

}
#endif
