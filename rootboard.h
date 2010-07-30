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

#include "eval.h"
#include "coloredboard.h"
#include "result.h"
#include "stats.h"
#include "history.h"
#ifdef QT_GUI_LIB
#include "statwidget.h"
#include "nodeitem.h"
#endif

class WorkThread;
class Console;
template<class T, unsigned int U, class U> class TranspositionTable;
template<class T> class Result;
typedef Key RepetitionKeys[100];
extern __thread RepetitionKeys keys;

/* Board representing a position with history, color to move, castling and en
 * passant status. Has an associated Eval object and holds multiple worker
 * threads. Contains information about the time budget.
 */
#ifdef QT_GUI_LIB
class RootBoard: QObject {
	Q_OBJECT
    StatWidget* statWidget;
signals:
	void createModel();
private:
#else
class RootBoard {
#endif
	friend class TestRootBoard;
	struct Killer {
		int move[3];
	};
	struct {
		ColoredBoard<White> wb;
		ColoredBoard<Black> bb;
	} boards[nMaxGameLength];

	unsigned int timeBudget;
	unsigned int movesToDo;
	unsigned int iMove;				// current half move index of game
	QDateTime startTime;

//	template<Colors C> ColoredBoard<C>& currentBoard();
//	unsigned int getAndDecAvailableThreads();

    int currentMoveIndex;
    int nMoves;
    static __thread int lastPositionalEval;
    Move line[nMaxGameLength];
    unsigned currentPly;
    static __thread Killer killer[maxMoves];
    unsigned fiftyMovesRoot;
    History history;

    template<Colors C> inline bool find(const ColoredBoard<C>& b, Key k, unsigned ply);
    inline void store(Key k, unsigned ply);
    template<Colors C> inline void clone(const ColoredBoard<C>& b, RepetitionKeys& other, unsigned ply);

public:
    Eval eval;
	unsigned int depth;
	TranspositionTable<TTEntry, transpositionTableAssoc, Key>* tt;
	TranspositionTable<PerftEntry, 1, Key>* pt;
	Console* console;
	Colors color;
	Move bestMove;
    RawScore estimatedError[nPieces*2+1][nSquares];
	double avgE[nPieces*2+1][nSquares];
	double avgE2[nPieces*2+1][nSquares];
	double avgN[nPieces*2+1][nSquares];
	RootBoard(Console*);
	template<Colors C> const ColoredBoard<C>& currentBoard() const;
    const BoardBase& currentBoard() const;
	void go(QStringList);
	const BoardBase& setup(QString fen = QString("rnbqkbnr/pppppppp/////PPPPPPPP/RNBQKBNR w KQkq - 0 0"));
	template<Colors C> Move rootSearch(unsigned int endDepth=maxDepth, uint64_t endNode = ~0);
	template<Colors C, Phase P, typename A, typename B, typename T>	bool search(const T& prev, Move m, unsigned depth, const A& alpha, B& beta, unsigned ply, NodeItem*);
	void perft(unsigned int depth);
	void divide(unsigned int depth);
	template<Colors C> uint64_t perft(const ColoredBoard<C>* b, unsigned int depth) const;
	template<Colors C> uint64_t rootPerft(unsigned int depth);
	template<Colors C> uint64_t rootDivide(unsigned int depth);
	template<Colors C, Phase P, typename ResultType> void perft(ResultType& result, const ColoredBoard<(Colors)-C>& prev, Move m, unsigned int depth);
	void divide(unsigned int depth) const;
	uint64_t getTime() const;
	Stats getStats() const;
    std::string getLine() const;
	void ttClear();
	bool doMove(Move);
};
#endif
