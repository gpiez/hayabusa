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

// struct MoveRawScore {
// 	Move m;
// 	RawScore r;
// 	operator RawScore () const {
// 		return r;
// 	}
// 	void operator = (RawScore a) {
// 		r = a;
// 	}
// };

template<Colors C> struct Score
{
	Move m;
	RawScore v;	//Absolute score. Less is good for black, more is good for white.
	typedef Score<C> Base;
	
	//Score() {};
	explicit Score (int a) 							{ v = C*a; m.data = 0; };
	// Returns a relative score. More is better for the current side.
	void join() const {};
	operator RawScore() const				{ return v;	}
//	void operator = (const Score<C>& a)  			{ v = a.v; };
//	void operator = (RawScore a)  					{ v = a; };
//	Score operator + (const Score<C>& a) const 		{ return v + a.v; };
//	Score operator - (const Score<C>& a) const 		{ return v - a.v; };
//	Score operator * (int a) const 					{ return v * a; };
	void setReady()									{};
	void setNotReady()								{};
//	unsigned int shares()							{ return 0; };

	bool operator >= (RawScore a) const {
		if ( C==White )
			return v>=a;
		else
			return v<=a;
	}
	bool operator > (RawScore a) const {
		if ( C==White )
			return v>a;
		else
			return v<a;
	}
	bool operator < (RawScore a) const {
		if ( C==White )
			return v<a;
		else
			return v>a;
	}
	bool max(const RawScore b) 		{
		if ( C==White ) {
			if (b > v) {
				v = b;
				return true;
			}
		} else {
			if (b < v) {
				v = b;
				return true;
			}
		}
		return false;
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
};

template<Colors C> struct SharedScore: public Score<C>
{
	using Score<C>::v;
	using Score<C>::m;
	typedef Score<C> Base;
	
	volatile unsigned int notReady;
	QMutex valueMutex;
	QWaitCondition readyCond;
	QMutex readyMutex;
	SharedScore<C>* depending;

private:
	SharedScore();
	SharedScore(SharedScore&);

public:
	~SharedScore() {
		ASSERT(!notReady);
	}

	explicit SharedScore (int a):
		Score<C>(a),
		notReady(0),
		valueMutex(QMutex::Recursive),
		depending(0)
	{
	};
	// construct a shared score depending on the parameter
	// if the parameter score gets a better value, a new
	// maximum is calculated for this score, and if it changes
	// for its depending scores too.
	explicit SharedScore(const SharedScore& a):
		Score<C>(0),
		notReady(0),
		valueMutex(QMutex::Recursive),
		depending(0)
	{
		v = a.v;
		m.data = 0;
	}

	bool operator >= (RawScore a) {
		if ( C==White )
			return v>=a;
		else
			return v<=a;
	}
	bool operator > (RawScore a) {
		if ( C==White )
			return v>a;
		else
			return v<a;
	}
	bool operator < (RawScore a) {
		if ( C==White )
			return v<a;
		else
			return v>a;
	}

	bool max(const RawScore b) 		{
		QMutexLocker lock(&valueMutex);
		if ( C==White ) {
			if (b > v) {
				v = b;
				return true;
			}
		} else {
			if (b < v) {
				v = b;
				return true;
			}
		}
		return false;
	}
	bool max(const RawScore b, const Move n) 		{
		QMutexLocker lock(&valueMutex);
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

	void join() {
		QMutexLocker lock(&readyMutex);
		while (notReady)
			readyCond.wait(&readyMutex);
	}
	
	void setReady() {
		QMutexLocker lock(&readyMutex);
		--notReady;
		ASSERT(notReady <= 4);
		readyCond.wakeOne();
	}

	void setNotReady() {
		QMutexLocker lock(&readyMutex);
		++notReady;
		ASSERT(notReady <= 4);
	}
};


#endif // SCORE_H
