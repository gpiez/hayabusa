/*
    hayabusa, chess engine
    Copyright (C) 2009-2010 Gunther Piez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef SCORE_H
#define SCORE_H

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
#include "move.h"

typedef int16_t RawScore;

struct MoveRawScore {
	Move m;
	RawScore r;
	operator RawScore () const {
		return r;
	}
	void operator = (RawScore a) {
		r = a;
	}
};

template<Colors C> struct Score
{
	Move m;
	RawScore v;	//Absolute score. Less is good for black, more is good for white.
	
	//Score() {};
	explicit Score (int a) 							{ v = C*a; };
	// Returns a relative score. More is better for the current side.
	int get() const						 			{ return C*v; };
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
	bool operator > (const Score& a) const {
		if ( C==White )
			return v>a.v;
		else
			return v<a.v;
	}
	bool operator < (const Score<(Colors)-C>& a) const {
		if ( C==White )
			return v<a.v;
		else
			return v>a.v;
	}
	bool operator < (RawScore a) const {
		if ( C==White )
			return v<a;
		else
			return v>a;
	}
	bool max(const RawScore b, const Move n) 		{
		if ( C==White ) {
			if (b > v) {
				v = b;
				m = n;
				return true;
			}
		} else {
			if (b < v) {
				v = b;
				m = n;
				return true;
			}
		}
		return false;
	}
//	bool max(const Score<(Colors)-C>& b) 		{
		//return max(b.v);
//	}
	bool max(const Score<(Colors)-C>& b, const Move n) 		{
		if ( C==White ) {
			if (b.v > v) {
				v = b.v;
				m = n;
				return true;
			}
		} else {
			if (b.v < v) {
				v = b.v;
				m = n;
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
	SharedScore<C>* depending;

private:
	SharedScore();
	SharedScore(const SharedScore&);

public:	
//	void operator = (const SharedScore<C>& a)  				{ v = a.v; };
	void operator = (RawScore a)  							{ v = a; };
	SharedScore operator + (const SharedScore<C>& a) const 	{ return v + a.v; };
	SharedScore operator - (const SharedScore<C>& a) const 	{ return v - a.v; };
	SharedScore operator * (int a) const 					{ return v * a; };
	unsigned int shares()									{ return notReady; };
	
	explicit SharedScore (int a):
		notReady(0),
		valueMutex(QMutex::Recursive),
		depending(0)
	{
		v = C*a;
	};

	// construct a shared score depending on the parameter
	// if the parameter score gets a better value, a new
	// maximum is calculated for this score, and if it changes
	// for its depending scores too.
	explicit SharedScore(SharedScore&);

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

	bool operator >= (const SharedScore<(Colors)-C>& a) const {
		QMutexLocker lock(&valueMutex);
		return (Score<C>)*this >= (Score<(Colors)-C>)a; 
	}
	
	bool max(const SharedScore<-C>& a, const Move n) 		{
		QMutexLocker lock(&valueMutex);
		if (((Score<C>)*this).max((Score<(Colors)-C>)a, n)) {
			for ( SharedScore<C>** i = &depending; *i; ++i ) {
				(*i)->max((Score<(Colors)-C>)a);
			}
			return true;
		}
		return false;
	}
};


#endif // SCORE_H
