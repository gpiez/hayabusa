#ifndef ROOTBOARD_H_
#define ROOTBOARD_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "eval.h"
#include "coloredboard.h"

class WorkThread;
class SearchJob;
class Console;

class RootBoard: public Eval {
	friend class TestRootBoard;
	
	unsigned int timeBudget;
	unsigned int movesToDo;
	unsigned int numThreads;
	unsigned int iMove;
	Colors color;
	QReadWriteLock threadsLock;
	QLinkedList<SearchJob*> jobs;

	struct {
		ColoredBoard<White> wb;
		ColoredBoard<Black> bb;
	} boards[nMaxGameLength];

	void initWorkThreads();
	void allocateWorkThreads(unsigned int);
	void stop();
	template<Colors C> const ColoredBoard<C>* currentBoard() const;

public:
	QReadWriteLock jobsLock;
	QVector<WorkThread*> threads;
	
	RootBoard(Console*);
	void go(QStringList);
	const BoardBase* setup(QString fen = QString("rnbqkbnr/pppppppp/////PPPPPPPP/RNBQKBNR w KQkq - 0 0"));
	template<Colors C> Score<C> rootSearch();
	template<Colors C> Score<C> search(const ColoredBoard<(Colors)-C>* prev, Move m, unsigned int depth, Score<C> alpha, Score<C> beta, SearchFlag f) const;
	template<Colors C> Score<C> qsearch(const ColoredBoard<(Colors)-C>* prev, Move m, const WorkThread*, Score<C> alpha, Score<C> beta) const;
	uint64_t perft(unsigned int depth) const;
	template<Colors C> uint64_t perft(const ColoredBoard<C>* b, unsigned int depth) const;
	template<Colors C> uint64_t perft(const ColoredBoard<(Colors)-C>* prev, Move m, unsigned int depth) const;
	void divide(unsigned int depth) const;
	template<Colors C> void divide(const ColoredBoard<C>* b, unsigned int depth) const;
	const Console* console;

};
#endif