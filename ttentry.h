/*
 * ttentry.h
 *
 *  Created on: Dec 17, 2009
 *      Author: gpiez
 */

#ifndef TTENTRY_H_
#define TTENTRY_H_

#include <stdint.h>

typedef uint64_t Key;

template <unsigned int pos, unsigned int width, typename T>
struct Bitfield {
	static const unsigned int start = pos / ( CHAR_BIT * sizeof(T));
	static const unsigned int startpos = pos % ( CHAR_BIT * sizeof(T));
	static const unsigned int end = (pos+width-1)/( CHAR_BIT * sizeof(T));
	static_assert(end == start);

	T data[end+1];
	operator T () const {
		if (width == 1)		// T is a bool type
			return data[start] & 1ULL<<startpos;
		else
			return data[start] >> startpos & (1ULL<<width)-1;
	}

//	void operator = (T value) {
//		data &= (1ULL<<width)-1 << pos;
//		data |= value << pos;
//	}

	void operator |= (T value) {
		data |= value << pos;
	}
};

/*
 * Transposition table entry. This assumes little endian order.
 * The static consts are the shift values for components.
 */
union TTEntry {
	Bitfield<0, 12, Score> score;
	Bitfield<12, 6, unsigned int> from;
	Bitfield<18, 6, unsigned int> to;
	Bitfield<24, 6, uint8_t> depth;
	Bitfield<30, 1, bool> loBound;
	Bitfield<31, 1, bool> hiBound;
	Bitfield<32, 1, bool> expired;
	Bitfield<33, 31, Key> upperKey;
	uint64_t data;

	void zero() { data = 0; };
};

#endif /* TTENTRY_H_ */
