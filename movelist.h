/*
    Boost Software License - Version 1.0 - August 17th, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/


#ifndef MOVELIST_H
#define MOVELIST_H

#include "move.h"
#include "coloredboard.h"
#include <cstring>

class OldMoveList {
protected:
    Move list[maxMoves];
    unsigned first;
    unsigned last;
    unsigned badCaptures;
    unsigned captures, threats, nonCaptures;

public:
    unsigned current;
    template<Colors C>
    OldMoveList(const ColoredBoard<C>& b) {
        Move* pfirst = list+goodMoves;
        Move* plast = pfirst;
//        b.template generateMateMoves<false>(&first, &last);
        if (b.template inCheck<C>())
            b.generateCheckEvasions(pfirst, plast);
        else {
            b.generateNonCap(pfirst, plast);
            b.template generateCaptureMoves<AllMoves>(pfirst, plast); }
        first = pfirst - list;
        last = plast - list; }

    const Move& operator * () const {
        return list[current]; }

    void operator ++ () {
        ++current; }

    bool isValid() const {
        return current < last; }

    unsigned count() const {
        return last-first; }

    void begin() {
        current = first; }

    void currentToFront() {
        Move temp = list[current];
        memmove(list+first+1, list+first, sizeof(Move) * (current-first));
        list[first] = temp; } };

template<Colors C, Phase P>
class MoveList;

template<Colors C, Phase P>
struct MoveFunc {
    virtual void next(MoveList<C,P> &) = 0;
};


template<Colors C, Phase P>
class MoveList {
    
    struct TT: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
//        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };

    struct Mates: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };

    struct Captures: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };
    
    struct Checks: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };

    struct Threats: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };
    
    struct Skewers: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };
    
    struct NonCaptures: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };
    
    struct BadCaptures: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };
    
    struct Evasions: public MoveFunc<C,P> {
#ifdef MYDEBUG        
        int max;
#endif        
        void init(MoveList<C,P>&);
        void next(MoveList<C,P>&); };
    /*
     * 1. Generate  TT
     * 2. Use       TT
     * 3. Generate  Mates / Checks                  l1 used / l3 used
     * 4. Use       Mates                           l1 free
     * 5. Generate  Captures / BadCaptures          l1 used / l2 used
     * 6. Use       Captures                        l1 free
     * 7. Use       Checks                                    l3 free
     * 8. Generate  Threats                         l1 used
     * 9. Use       Threats                         l1 free
     * 10.Generate  Skewers                         l1 used
     * 11.Use       Skewers                         l1 free
     * 12a.Generate  NonCaptures/append BadNonCaptures to BadCaptures  l1/l2 used
     * 12b.Generate  nothing
     * 13a.Use       NonCaptures+BadNonCaptures+BadCaptures    l1/l2 free
     * 13b.Use       BadCaptures                               l2 free
     *
     * The sublists do not contain any real data. The moves are stored in this
     * MoveList object, in 3 lists, which are partially shared during the move
     * generation process. As the sublist contain no data, but only a vtable,
     * the are made static. This has the advantage that the vtables do not
     * need to be constructed during runtime each time a MoveList is created on
     * the stack, but rather are initialized during startup. The sublist
     * objects are basically used only as jumptables
     */
    static TT tt;
    static Mates mates;
    static Captures captures;
    static Checks checks;
    static Threats threats;
    static Skewers skewers;
    static NonCaptures nonCaptures;
    static BadCaptures badCaptures;
    static Evasions evasions;
    
    Move l[192]; // shared between Mates, Captures, Threats, Skewers, NonCaptures, Evasions
    Move lend[0];
    Move l2[64]; // badCaptures + appended BadNonCaptures
    Move l2end[0];
    Move l3[64]; // checks
    Move l3end[0];
    Move* p;
    Move* p2;
    Move* p3;
    Move* pend;
    const ColoredBoard<C>& b;
    
    MoveFunc<C,P>* iterator;
    Move currentMove;
    unsigned imove;
    Move ttMove;
    
public:
    MoveList(const ColoredBoard<C>& b, Move);
    
    const Move& operator * () const {
        return currentMove; }

    bool operator ++ ();
    
    Move* operator -> () {
        return &currentMove;
    }

    bool isEmpty() const {
        return !iterator; }

    unsigned size() const {
        ASSERT((b.template inCheck<C>()));
        if (P==vein && !b.threatened)
            return -1;
        if (P==leaf && !b.threatened)
            return -1;
        return pend-p; }
    unsigned index() const {
        return imove;
    }
    bool isCapture() { return iterator == &captures; }
    bool isNonCapture() { return iterator == &nonCaptures; }
    bool isFirst() {
        return false; } //FIXME
    bool isMate() { return iterator == &mates; }
    
};
#endif // MOVELIST_H
