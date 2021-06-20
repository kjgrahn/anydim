/*
 * Copyright (c) 2021 Jörgen Grahn
 * All rights reserved.
 *
 */
#include "orientation.h"

Orientation::Orientation(const tiff::File& file)
    : val(tiff::find<tiff::type::Short>(file.ifd0, 0x0112))
{}

/**
 * True iff the Orientation specifies that the height shall become the
 * width. See the example with the letter F in jpegexiforient(1):
 * orientations 5 to 8 show fallen F letters in various
 * configurations.
 */
bool Orientation::fallen() const
{
    if (val) switch(*val) {
	case 5:
	case 6:
	case 7:
	case 8:
	    return true;
    }

    return false;
}
