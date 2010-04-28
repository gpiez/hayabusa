#ifndef ROOTBOARD_H_
#define ROOTBOARD_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "eval.h"
#include "coloredboard.h"
#include "result.h"

class WorkThread;
class SearchJob;
class Console;
template<class T, unsigned int U> class TranspositionTable;
template<class T> class Result;

class RootBoard: public Eval {
	friend class TestRootBoard;
	
	unsigned int timeBudget;
	unsigned int movesToDo;
	unsigned int numThreads;
	unsigned int availableThreads;
	unsigned int iMove;
	Colors color;
	QMutex threadsLock;
	QLinkedList<SearchJob*> jobs;
	TranspositionTable<TTEntry, transpositionTableAssoc>* tt;

	struct {
		ColoredBoard<White> wb;
		ColoredBoard<Black> bb;
	} boards[nMaxGameLength];

	void initWorkThreads();
	void allocateWorkThreads(unsigned int);
	void stop();
	template<Colors C> const ColoredBoard<C>* currentBoard() const;
    WorkThread* findFreeThread();
	unsigned int getAndDecAvailableThreads();
	
public:
	TranspositionTable<PerftEntry, 1>* pt;
	QReadWriteLock jobsLock;
	QVector<WorkThread*> threads;
	
	RootBoard(Console*);
	void go(QStringList);
	const BoardBase* setup(QString fen = QString("rnbqkbnr/pppppppp/////PPPPPPPP/RNBQKBNR w KQkq - 0 0"));
	template<Colors C> Move rootSearch();
	template<Colors C> Score<C> search(const ColoredBoard<(Colors)-C>* prev, Move m, unsigned int depth, Score<C> alpha, Score<C> beta, SearchFlag f) const;
	template<Colors C> Score<C> qsearch(const ColoredBoard<(Colors)-C>* prev, Move m, const WorkThread*, Score<C> alpha, Score<C> beta) const;
	void perft(unsigned int depth);
	template<Colors C> uint64_t perft(const ColoredBoard<C>* b, unsigned int depth) const;
	template<Colors C> uint64_t rootPerft(unsigned int depth);
	template<Colors C> uint64_t perft(const ColoredBoard<(Colors)-C>* prev, Move m, unsigned int depth) const;
	template<Colors C> void perft(Result<uint64_t>* result, const ColoredBoard<(Colors)-C>* prev, const Move m, const unsigned int depth);
	void divide(unsigned int depth) const;
	template<Colors C> void divide(const ColoredBoard<C>* b, unsigned int depth) const;
	Console* console;

};
#endif