/* $Id: version.cc,v 1.2 2011-01-08 12:59:33 grahn Exp $
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#include "anydim.h"

#include <cstring>
#include <cctype>
#include <algorithm>

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
 * The parser is a bit crude and leaky because I want to keep this
 * small and not reliant on any regex library. And it's not exactly
 * untrusted user input we're parsing ...
 */
std::string anydim::version()
{
    using std::string;

    do {
	static char dollar_name[] = "$Name:  $";
	char* a = dollar_name;
	char* b = a + std::strlen(dollar_name);

	a = std::find(a, b, ' ');
	if(a==b) break;
	++a;
	if(a==b) break;

	/* first token shaved off, still not empty */
	{
	    char* c = std::find(a, b, ' ');
	    if(c==b) break;
	    if(c==a) break;
	    b = c;
	}

	/* only second token remains, not empty */
	while(!std::isdigit(*a)) {
	    char* c = std::find(a, b, '-');
	    if(c==b) break;
	    c++;
	    if(c==b) break;
	    a = c;
	}

	std::replace(a, b, '-', '.');
	return string(a, b);

    } while(0);

    return "unpublished";
}
