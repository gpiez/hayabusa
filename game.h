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

#include "eval.h"
#include "coloredboard.h"
#include "stats.h"
#include "history.h"
#ifdef QT_GUI_LIB
#include "statwidget.h"
#include "nodeitem.h"
#endif
#include "stringlist.h"

class Parameters;
class WorkThread;
class Console;
union TTEntry;
class PerftEntry;
template<class T, unsigned int U, class U> class TranspositionTable;
template<class T> class Result;

typedef Key RepetitionKeys[100+nMaxGameLength];

extern __thread RepetitionKeys keys;

enum Extension {ExtNot = 0, ExtCheck = 1, ExtSingleReply = 2, ExtDualReply = 4,
                ExtMateThreat = 8, ExtForkThreat = 16, ExtPawnThreat = 32,
                ExtFork = 64, ExtTestMate = 128, ExtDiscoveredCheck = 256, ExtFirstMove = 512 };

enum Status { Running, Stopping, Stopped };

/* Board representing a position with history, color to move, castling and en
 * passant status. Has an associated Eval object and holds multiple worker
 * threads. Contains information about the time budget.
 */
#ifdef QT_GUI_LIB

#define NODE ,node
#define NODEDEF ,NodeItem* node

class Game: public QObject {
    Q_OBJECT

private:
    StatWidget* statWidget;
signals:
    void createModel();
    void signalInfo(int, uint64_t, uint64_t, QString, QString);
private:

#else

#define NODE
#define NODEDEF

class Game {

#endif

    friend class TestRootBoard;

public:
    struct WBBoard {
        ColoredBoard<White> wb;
        ColoredBoard<Black> bb; };
    struct PositionalError {
        int16_t v;
        int16_t error;
#ifdef CALCULATE_MEAN_POSITIONAL_ERROR
        float n;
        float e;
        float e2;
#endif
        void init(int x); };
private:

private:
    WBBoard boards[nMaxGameLength];
    unsigned nullReduction[maxDepth+1];
    unsigned verifyReduction[maxDepth+1];
    TranspositionTable<TTEntry, transpositionTableAssoc, Key>* tt;
    TranspositionTable<PerftEntry, 1, Key>* pt;

    unsigned int iMove;                // current half move index of game

    int currentMoveIndex;
    Move currentMove;
    int nMoves;
    Move line[nMaxGameLength];
    unsigned rootPly;
    unsigned currentPly;
    std::string info;
    std::chrono::system_clock::time_point start;
    uint64_t stopTime;
    int wtime;
    int btime;
    int winc;
    int binc;
    int movestogo;
    int maxSearchDepth;
    int mate;
    int movetime;
    Move bm;
    volatile Status stopSearch;
    Mutex stopSearchMutex;
    Condition stoppedCond;

    TimedMutex infoTimerMutex;
    TimedMutex stopTimerMutex;
    int bestScore;
    int whiteLimit;
    int blackLimit;
    static Mutex allocMutex;

    template<Colors C> inline bool find(const ColoredBoard<C>& b, Key k, unsigned ply) const;
    inline void store(Key k, unsigned ply);
    std::string status(std::chrono::system_clock::time_point, int);
    template<int C> int limit() const {
        if (C==White) return whiteLimit;
        else return blackLimit; }
    template<int C> int& limit() {
        if (C==White) return whiteLimit;
        else return blackLimit; }

public:
    Eval eval ALIGN_XMM;
    static __thread History history;
    unsigned int depth;
    Console* console;
    Colors color;
    Move bestMove;
    bool infinite;
    uint64_t maxSearchNodes;

    PositionalError pe[nPieces*2+1][nSquares][nSquares];

    Game(Console* c, const Parameters& p, uint64_t, uint64_t);
#ifdef QT_GUI_LIB
    virtual
#endif
    ~Game();
    void* operator new(size_t size);
    void operator delete(void*);
    void clearEE();
    template<Colors C> const ColoredBoard<C>& currentBoard() const;
    template<Colors C> ColoredBoard<(Colors)-C>& nextBoard();
    const Board& currentBoard() const;
    void go(const std::map<std::string, StringList>&);
    void goReadParam(const std::map<std::string, StringList>&);
    void goExecute();
    void stop();
    const Board& setup(const std::string& fen = std::string("rnbqkbnr/pppppppp/////PPPPPPPP/RNBQKBNR w KQkq - 0 0"));
    template<Colors C> Move rootSearch(unsigned int endDepth=maxDepth);
    template<Colors C, Phase P, class A, class B, Colors PREVC>
    int search3(const ColoredBoard<PREVC>& prev, Move m, unsigned depth,
                const A& alpha, const B& beta,
                unsigned ply, Extension ext, bool& nextMaxDepth, NodeType nt NODEDEF );
    template<Colors C, Phase P, class A, class B, Colors PREVC>
    int search4(const ColoredBoard<PREVC>& prev, Move m, unsigned depth,
                const A& alpha, const B& beta,
                unsigned ply, Extension ext, NodeType nt NODEDEF ) __attribute__((always_inline));
    template<Colors C, Phase P, class A, class B>
    int search9(const bool doNull, const unsigned reduction, const ColoredBoard<(Colors)-C>& prev, Move m, unsigned depth,
                const A& alpha, const B& beta,
                unsigned ply, Extension ext, NodeType nt NODEDEF );

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
    void stopTimer(std::chrono::milliseconds hardlimit);
    void infoTimer(std::chrono::milliseconds repeat);
    void clearHash();
    void ageHash();
    void setHashSize(size_t s);
    template<Colors C> int calcReduction(const ColoredBoard<C>& b, int movenr, Move m, int depth);
    std::string commonStatus() const;
    void setTime(uint64_t wnanoseconds, uint64_t bnanoseconds);
    int getScore() const {
        return bestScore; }
    void goWait();
    template<Colors C> bool isDraw(const ColoredBoard<C>& b) const;
    unsigned getRootPly() const {
        return rootPly; } };
#endif
