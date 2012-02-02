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
#ifndef COMPOUNDSCORE_H
#define COMPOUNDSCORE_H

#include "packedscore.h"
#include <x86intrin.h>

/*
 * Stores game phase depended scores, like endgame score and opening score
 * Optimized for speed, it may require more space than the sum of the sizes
 * of the stored elements
 */
struct CompoundScore {
#ifdef __SSE2__
    typedef __v8hi vector_t;
#else
    typedef short vector_t __attribute__((vector_size(4)));
#endif
    vector_t data;
    CompoundScore() = default;
    CompoundScore(int opening, int endgame) {
        ASSERT(opening <= 0x7fff && opening >= -0x8000);
        ASSERT(endgame <= 0x7fff && endgame >= -0x8000);
#ifdef __SSE2__
        data = _mm_insert_epi16(data, opening, 0);
        data = _mm_insert_epi16(data, endgame, 1);
#else
        data = (vector_t) {
            (int16_t)opening, (int16_t)endgame };
#endif
    }
    CompoundScore(vector_t init): data(init) {}

    // making this a non-reference and using an aligned load instead of the
    // unaligned slows down the _whole_ program 20%
    CompoundScore(const PackedScore<>& x) {
#ifdef __SSE2__
        data = _mm_loadu_si128((__m128i*)&x) ;  // DON'T TOUCH
#else
        data = (vector_t) {
            (int16_t)x.opening, (int16_t)x.endgame };;
#endif
    }
    CompoundScore operator + (const CompoundScore& x) const {
        return data + x.data; }
    CompoundScore operator - (const CompoundScore& x) const {
        return data - x.data; }
    CompoundScore operator - () const {
        return CompoundScore(0,0) - *this; }
    int16_t opening() const {
        return _mm_extract_epi16(data, 0); }
    int16_t endgame() const {
        return _mm_extract_epi16(data, 1); }
    PackedScore<> packed() const {
        union {
            int         i;
            PackedScore<> p; } converter;
        converter.i = _mm_cvtsi128_si32(data);
        return converter.p;
//         int i = _mm_cvtsi128_si32(data);
//         return reinterpret_cast<PackedScore<>&>(i);
    }
//    int calc(int material, const Eval& eval) const;
//     CompoundScore operator + (const CompoundScore& x) const {
//         CompoundScore temp;
//         temp.opening = x.opening + opening;
//         temp.endgame = x.endgame + endgame;
//         return temp;
//     }
//     CompoundScore operator - (const CompoundScore& x) const {
//         CompoundScore temp;
//         temp.opening = x.opening - opening;
//         temp.endgame = x.endgame - endgame;
//         return temp;
//     }
//     CompoundScore operator - () const {
//         CompoundScore temp;
//         temp.opening = -opening;
//         temp.endgame = -endgame;
//         return temp;
//     }
//     void operator /= (const int x) {
//         opening /= x;
//         endgame /= x;
//     }
//     int calc(int material, const Eval& eval) const;
//     void operator = (int x) {
//         opening = x;
//         endgame = x;
//     }
};
#endif
