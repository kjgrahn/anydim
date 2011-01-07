/* $Id: pnmdim.cc,v 1.2 2011-01-07 22:34:04 grahn Exp $
 *
 * Copyright (c) 2011 Jörgen Grahn
 * All rights reserved.
 *
 */
#include "anydim.h"

using anydim::PnmDim;

namespace {
    enum PState {
	WANT_P,
	WANT_N,
	WANT_WS1, WANT_WS1B,
	WANT_W,
	WANT_WS2,
	WANT_H
    };

    bool ws(char ch)
    {
	switch(ch) {
	case ' ':
	case '\t':
	case '\r':
	case '\n':
	    return true;
	default:
	    return false;
	}
    }

    bool digit(char ch)
    {
	return ch>='0' && ch<='9';
    }
}


PnmDim::PnmDim()
    : comment_(false),
      pnmstate_(WANT_P),
      mime_("")
{}


const char* PnmDim::mime() const
{
    return mime_;
}


void PnmDim::feed(const uint8_t *a, const uint8_t *b)
{
    while(a!=b) {
	char ch = *a++;
	feed(ch);
    }
}


void PnmDim::eof()
{
    if(state_==UNDECIDED) state_ = BAD;
}


/* Simple state machine which goes like this:
 * 
 * P N WS1A WS1B+ W+ WS2+ H+
 *
 * with detours into comment_ and exits into BAD or GOOD.
 */
void PnmDim::feed(char ch)
{
    if(state_!=UNDECIDED) return;

    if(comment_) {
	if(ch=='\n') {
	    comment_ = false;
	}
	else {
	    return;
	}
    }

    switch(pnmstate_) {
    case WANT_P:
	pnmstate_ = WANT_N;
	if(ch!='P') {
	    state_ = BAD;
	}
	break;
    case WANT_N:
	pnmstate_ = WANT_WS1;
	switch(ch) {
	case '1':
	case '4':
	    mime_ = "image/x-portable-bitmap";
	    break;
	case '2':
	case '5':
	    mime_ = "image/x-portable-graymap";
	    break;
	case '3':
	case '6':
	    mime_ = "image/x-portable-pixmap";
	    break;
	default:
	    state_ = BAD;
	}
	break;
    case WANT_WS1:
	if(ch=='#') {comment_ = true; break;}
	pnmstate_ = WANT_WS1B;
	if(!ws(ch)) state_ = BAD;
	break;
    case WANT_WS1B:
	if(ch=='#') {comment_ = true; break;}
	if(ws(ch)) break;
	if(!digit(ch)) {
	    state_ = BAD;
	}
	else {
	    width = ch - '0';
	    pnmstate_ = WANT_W;
	}
	break;
    case WANT_W:
	if(digit(ch)) {
	    width *= 10;
	    width += ch - '0';
	}
	else if(ws(ch)) {
	    pnmstate_ = WANT_WS2;
	}
	else {
	    state_ = BAD;
	}
	break;
    case WANT_WS2:
	if(ws(ch)) break;
	if(!digit(ch)) {
	    state_ = BAD;
	}
	else {
	    height = ch - '0';
	    pnmstate_ = WANT_H;
	}
	break;
    case WANT_H:
	if(digit(ch)) {
	    height *= 10;
	    height += ch - '0';
	}
	else if(ws(ch)) {
	    state_ = GOOD;
	}
	else {
	    state_ = BAD;
	}
	break;
    }
}
