/*
    Copyright (c) <year> <copyright holders>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef SCORE_H
#define SCORE_H

#include "constants.h"

typedef uint16_t RawScore;

template<Colors C> struct Score
{
	RawScore v;	//Absolute score. Less is good for black, more is good for white.

	Score() {};
	explicit Score (int a) 						{ v = C*a; };
	// Returns a relative score. More is better for the current side.
	//operator int () 			{ return C*v; };
	Score (const Score<White>& a) 					{ v = a.v; };
	Score (const Score<Black>& a) 					{ v = a.v; };
	void operator = (Score<C> a)  	{ v = a.v; };
	Score operator + (Score<C> a) const 	{ return v + a.v; };
	Score operator - (Score<C> a) const 	{ return v - a.v; };
	Score operator * (int a) const 		{ return v * a; };
	bool operator > (Score<C> a) const {
		if (C > 0)
			return v > a.v;
		else 
			return v < a.v;
	};
	bool operator < (Score<C> a) const {
		if (C > 0)
			return v < a.v;
		else
			return v > a.v;
	};
	bool operator == (Score<C> a) const 	{ return v == a.v; };
};

#endif // SCORE_H
