/* $Id: version.cc,v 1.1 2011-01-08 12:25:43 grahn Exp $
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#include "anydim.h"

/**
 * The version, extracted from the CVS 'Name' keyword.
 *
 * The string is assumed to be
 *    \$Name\s.+?-(\d+(-\d+)*)\s\$
 * and we replace '-' with '.' in $1 to form a version
 * string like:
 *    42
 *    0.1
 *    1.2.3.4
 * If the string doesn't match, the version is "unrelased".
 *
 * It's a bit crude because I want to keep this small
 * and not reliant on any regex library.
 */
std::string anydim::version()
{
    static const char dollar_name[] = "$Name:  $";

    return "unpublished";
}
