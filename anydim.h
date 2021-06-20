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

namespace jfif {
    class Decoder;
}

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
     * JPEG (JFIF) dimension decoder. See jfif::Decoder for more
     * on the overall format.
     *
     * The width and height of the image is in a SOF0 or (for
     * progressive JPEG) SOF2 segment, which both appear to be
     *
     * 1 octet  something
     * 2 octets height
     * 2 octets width
     * ...
     *
     * The same goes for at least SOF1, SOF9 and SOF10.
     *
     * 	   Refs: <http://en.wikipedia.org/wiki/JPEG#Syntax_and_structure>,
     * 	   and some googling, and the libjpeg sources.
     *
     * This width x height may be modified by Exif (TIFF?) Orientation.
     */
    class JpegDim final: public Dim {
    public:
	explicit JpegDim(bool use_exif);
	~JpegDim();

	const char* mime() const override;

	void feed(const uint8_t *a, const uint8_t *b) override;
	void eof() override;

    private:
	jfif::Decoder* const decoder;
	const bool use_exif;
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
	explicit AnyDim(bool use_exif);
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
