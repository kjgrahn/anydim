/* -*- c++ -*-
 * $Id: anydim.h,v 1.1 2011-01-03 23:27:45 grahn Exp $
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#ifndef ANYDIM_ANYDIM_H
#define ANYDIM_ANYDIM_H

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

    private:
	enum State { UNDECIDED, GOOD, BAD };
	State state_;
    };

}
#endif
