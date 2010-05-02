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

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"

typedef int16_t RawScore;

template<Colors C> struct Score
{
	RawScore v;	//Absolute score. Less is good for black, more is good for white.

	Score() {};
	explicit Score (int a) 							{ v = C*a; };
	// Returns a relative score. More is better for the current side.
	int get()							 			{ return C*v; };
//	Score (const Score<White>& a) 					{ v = a.v; };
//	Score (const Score<Black>& a) 					{ v = a.v; };
	void operator = (const Score<C>& a)  			{ v = a.v; };
	void operator = (RawScore a)  					{ v = a; };
	Score operator + (const Score<C>& a) const 		{ return v + a.v; };
	Score operator - (const Score<C>& a) const 		{ return v - a.v; };
	Score operator * (int a) const 					{ return v * a; };
	void setReady()									{};
	void setNotReady()								{};
	unsigned int shares()							{ return 0; };

	bool operator >= (const Score<(Colors)-C>& a) const {
		if ( C==White )
			return v>=a.v;
		else
			return v<=a.v;
	}
	bool max(const Score<(Colors)-C>& b) 		{
		if ( C==White ) {
			if (b.v > v) {
				v = b.v;
				return true;
			}
		} else {
			if (b.v < v) {
				v = b.v;
				return true;
			}
		}
		return false;
	}
	
};

template<Colors C> struct SharedScore: private Score<C>
{
	using Score<C>::v;
	volatile unsigned int notReady;
	QMutex valueMutex;
	QWaitCondition readyCond;
	QMutex readyMutex;
	SharedScore* depending;

private:
	SharedScore();
	SharedScore(const SharedScore&);

public:	
	void operator = (const SharedScore<C>& a)  				{ v = a.v; };
	void operator = (RawScore a)  							{ v = a; };
	SharedScore operator + (const SharedScore<C>& a) const 	{ return v + a.v; };
	SharedScore operator - (const SharedScore<C>& a) const 	{ return v - a.v; };
	SharedScore operator * (int a) const 					{ return v * a; };
	unsigned int shares()									{ return notReady; };
	
	explicit SharedScore (int a):
		notReady(0)
	{
		v = C*a;
	};
	// Returns a relative score. More is better for the current side.
	int get() {
		QMutexLocker lock(&readyMutex);
		while (notReady)
			readyCond.wait(&readyMutex);
		return C*v;
	};
	
	
	void setReady() {
		readyMutex.lock();
		--notReady;
		readyMutex.unlock();
		readyCond.wakeOne();

	}

	void setNotReady() {
		readyMutex.lock();
		++notReady;
		readyMutex.unlock();
	}

	bool operator < (const SharedScore<C>& a) const {
		if ( C==White )
			return v<a.v;
		else
			return v>a.v;
	}
	bool operator <= (const SharedScore<C>& a) const {
		if ( C==White )
			return v<=a.v;
		else
			return v>=a.v;
	}
	bool max(const SharedScore<C>& b) 		{
		if (*this < b) {
			v = b.v;
			return true;
		}
		return false;
	}

};


#endif // SCORE_H
