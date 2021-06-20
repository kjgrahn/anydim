/* Copyright (c) 2021 Jörgen Grahn
 * All rights reserved.
 *
 */
#ifndef ANYDIM_ORIENTATION_H
#define ANYDIM_ORIENTATION_H

#include "tiff/tiff.h"

/**
 * The Exif Orientation, and a method to take a JPEG (width, height)
 * and decide if it should really be (height, width).
 */
class Orientation {
public:
    explicit Orientation(const tiff::File& file);

    template <class T>
    void adjust(T& width, T& height) const;

private:
    const optional<uint16_t> val;
    bool fallen() const;
};

template <class T>
void Orientation::adjust(T& width, T& height) const
{
    if (fallen()) std::swap(width, height);
}

#endif
