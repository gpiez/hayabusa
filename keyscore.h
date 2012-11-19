/*
 * keyscore.h
 *
 *  Created on: Jan 31, 2012
 *      Author: gpiez
 */

#ifndef KEYSCORE_H_
#define KEYSCORE_H_

#include "constants.h"
#include "packedscore.h"
#include "compoundscore.h"
/*
 * update score:        every move      needed early
 * update key:          every move      needed late
 * update pawnkey:      only pawn move  needed very late
 * update material:     only capture    needed early
 * update *material:    some cpatures   needed early
 * update *matscale:    rare            needed early
 */
#ifndef __SSE__
typedef short __attribute__((vector_size(16))) __v8hi;
#endif
#ifdef __SSE4_1__
struct KeyScore {
    __v8hi vector;
    CompoundScore score() const {
        return CompoundScore(vector);
    }
    void opening(int v) {
        vector = _mm_insert_epi16(vector, v, 0);
    }
    void endgame(int v) {
        vector = _mm_insert_epi16(vector, v, 1);        
    }
    void score(CompoundScore v) {
        int v32 = _mm_extract_epi32(v.data, 0);
        vector = _mm_insert_epi32(vector, v32, 0);
    }
    PawnKey pawnKey() const {
        return _mm_extract_epi32(vector, 1);
    }
    void pawnKey(uint32_t pk) {
        vector = _mm_insert_epi32(vector, pk, 1);
    }
    Key key() const {
        return _mm_extract_epi64(vector, 1);
    }
    void key(uint64_t k) {
        vector = _mm_insert_epi64(vector, k, 1);
    }
    int opening() const {
        return _mm_extract_epi16(vector, 0);
    }
    int endgame() const {
        return _mm_extract_epi16(vector, 1);
    }
};
#else
union KeyScore {
    CompoundScore score() const {
    	return CompoundScore(opening(), endgame());
    }
    void opening(int v) {
        m_score.opening = v;
    }
    void endgame(int v) {
        m_score.endgame = v;        
    }
    void score(CompoundScore v) {
        m_score = v.packed();
    }
    PawnKey pawnKey() const {
        return m_pawnKey;
    }
    void pawnKey(uint32_t pk) {
        m_pawnKey = pk;
    }
    Key key() const {
        return m_key;
    }
    void key(uint64_t k) {
        m_key = k;
    }
    int opening() const {
        return m_score.opening;
    }
    int endgame() const {
        return m_score.endgame;
    }
    __v8hi vector;
    struct {
        PackedScore<>  m_score;
        PawnKey        m_pawnKey;
        Key            m_key; 
    };
};
#endif
union KeyMaterialScore {
    __v8hi vector;
    struct {
        PackedScore<>  score;
        unsigned       matIndex;
        Key            key; }; };

#endif /* KEYSCORE_H_ */
