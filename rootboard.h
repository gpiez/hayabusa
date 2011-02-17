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
#ifndef ROOTBOARD_H_
#define ROOTBOARD_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <chrono>
#include "eval.h"
#include "coloredboard.h"
#include "result.h"
#include "stats.h"
#include "history.h"
#ifdef QT_GUI_LIB
#include "statwidget.h"
#include "nodeitem.h"
#endif
#include "stringlist.h"
#include "repetition.h"

using namespace std::chrono;

class WorkThread;
class Console;
template<class T, unsigned int U, class U> class TranspositionTable;
template<class T> class Result;

/* Board representing a position with history, color to move, castling and en
 * passant status. Has an associated Eval object and holds multiple worker
 * threads. Contains information about the time budget.
 */
#ifdef QT_GUI_LIB

class RootBoard: public QObject {
    Q_OBJECT
    
private:    
    StatWidget* statWidget;
signals:
    void createModel();
    void signalInfo(int, uint64_t, uint64_t, QString, QString);
private:

#else

class RootBoard {

#endif

    friend class TestRootBoard;

public:
    struct WBBoard {
        ColoredBoard<White> wb;
        ColoredBoard<Black> bb;
    };
private:
    WBBoard boards[nMaxGameLength];

    static unsigned dMaxCapture;
    static unsigned dMaxThreat;
    unsigned int iMove;                // current half move index of game

    int currentMoveIndex;
    int nMoves;
    Move line[nMaxGameLength];
    unsigned rootPly;
    unsigned currentPly;
    History history;        // FIXME probably needs to be thread local
    std::string info;
    system_clock::time_point start;
    system_clock::time_point lastStatus2;
    int wtime;
    int btime;
    int winc;
    int binc;
    int movestogo;
    int maxSearchDepth;
    int mate;
    int movetime;
    Move bm;
    volatile bool stopSearch;
    TimedMutex infoTimerMutex;
    TimedMutex stopTimerMutex;

    template<Colors C> inline bool find(const ColoredBoard<C>& b, Key k, unsigned ply) const;
    inline void store(Key k, unsigned ply);
    std::string status(system_clock::time_point, int);

public:
    Eval eval;
    unsigned int depth;
    TranspositionTable<TTEntry, transpositionTableAssoc, Key>* tt;
    TranspositionTable<PerftEntry, 1, Key>* pt;
    Console* console;
    Colors color;
    Move bestMove;
    bool infinite;
    uint64_t maxSearchNodes;

    RawScore estimatedError[nPieces*2+1][nSquares];
    double avgE[nPieces*2+1][nSquares];
    double avgE2[nPieces*2+1][nSquares];
    double avgN[nPieces*2+1][nSquares];

    RootBoard(Console*);
    void clearEE();
    template<Colors C> const ColoredBoard<C>& currentBoard() const;
    template<Colors C> ColoredBoard<(Colors)-C>& nextBoard();
    const BoardBase& currentBoard() const;
    void go(const std::map<std::string, StringList>&);
    void stop(bool);
    const BoardBase& setup(const std::string& fen = std::string("rnbqkbnr/pppppppp/////PPPPPPPP/RNBQKBNR w KQkq - 0 0"));
    template<Colors C> Move rootSearch(unsigned int endDepth=maxDepth);
    template<Colors C, Phase P, typename A, typename B, typename T>    bool search(NodeType, const T& prev, Move m, unsigned depth, const A& alpha, B& beta, unsigned ply, bool threatened
#ifdef QT_GUI_LIB
        , NodeItem*
#endif
        );
    void perft(unsigned int depth);
    void divide(unsigned int depth);
    template<Colors C> uint64_t perft(const ColoredBoard<C>* b, unsigned int depth) const;
    template<Colors C> uint64_t rootPerft(unsigned int depth);
    template<Colors C> uint64_t rootDivide(unsigned int depth);
    template<Colors C, Phase P, typename ResultType> void perft(ResultType& result, const ColoredBoard<(Colors)-C>& prev, Move m, unsigned int depth);
    void divide(unsigned int depth) const;
    Stats getStats() const;
    std::string getLine() const;
    bool doMove(Move);
    template<Colors C> bool doMove(Move);
    std::string getInfo() const;
    template<Colors C> inline void clone(const ColoredBoard<C>& b, const RepetitionKeys& other, unsigned ply) const;
    Move findMove(const std::string&) const;
    void stopTimer(milliseconds hardlimit);
    void infoTimer(milliseconds repeat);
    void clearHash();
    void ageHash();
    template<Colors C> int calcReduction(const ColoredBoard<C>& b, int movenr, Move m, int depth
);
};
#endif
