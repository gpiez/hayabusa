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
	static const unsigned int tBits = CHAR_BIT * sizeof(T);
	static const unsigned int start = pos / tBits;
	static const unsigned int startpos = pos % tBits;
	static const unsigned int end = (pos+width-1)/ tBits;

	T data[end+1];
	operator T () const {
		static_assert(end == start);
		if (width == 1)		// T is a bool type
			return data[start] & (T)1 << startpos;
		else {
			T unmasked = data[start] >> startpos;
			if (width + startpos != tBits)
				unmasked &= ((T)1 << width) - 1;
			return unmasked;
		}
	}

//	void operator = (T value) {
//		data &= (1ULL<<width)-1 << pos;
//		data |= value << pos;
//	}

	void operator |= (T value) {
		static_assert(end == start);
		data[start] |= value << pos;
	}
};

/*
 * Transposition table entry. This assumes little endian order.
 * The static consts are the shift values for components.
 */
union TTEntry {
	Bitfield<0, 12, int> score;
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

struct PerftEntry {
	union {
	Bitfield<0, 6, uint8_t> depth;
	Bitfield<6, 58, Key> upperKey;
	uint64_t data;
	};
	enum { upperShift = 6 };
	uint64_t value;
	
	void zero() { data = 0; };
};

#endif /* TTENTRY_H_ */
