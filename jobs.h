/*
 * jobs.h
 *
 *  Created on: Feb 3, 2010
 *      Author: gpiez
 */

#ifndef JOBS_H_
#define JOBS_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "rootboard.h"
#include "console.h"
#include "boardbase.h"
//#include "rootboard.tcc"

struct Job {
	virtual ~Job() {};			// the for calling destructor of QObject in signalJob
	virtual void job() = 0;
};

struct signalJob: public QObject, public Job {
	Q_OBJECT
signals:
    void result(QString arg1);
};

template<Colors C, typename A, typename B>
class SearchJob: public Job {
	RootBoard* rb;
	const ColoredBoard<(Colors)-C>& b;
	Move m;
	unsigned int depth;
	A& alpha;
	const B& beta;
public:
	SearchJob(RootBoard* rb, const ColoredBoard<(Colors)-C>& b, Move m, unsigned int depth, A& alpha, const B& beta): rb(rb), b(b), m(m), depth(depth), alpha(alpha), beta(beta) {};
	void job() {
		//QTextStream xout(stderr);
		//xout << depth;
		rb->search<C, root>(b, m, depth, alpha, beta);
	}
};

template<Colors C>
class RootSearchJob: public signalJob {
	RootBoard* rb;
	
public:
	RootSearchJob(RootBoard* rb): rb(rb) {};
	void job() {
		connect(this, SIGNAL(result(QString)), rb->console, SLOT(getResult(QString)));
		Move m = rb->rootSearch<C>();
		emit(result(m.string()));
	}
};

template<Colors C, typename T>
class PerftJob: public Job {
	RootBoard* rb;
	T& n;
	const ColoredBoard<(Colors)-C>& b;
	Move m;
	unsigned int depth;
public:
	PerftJob(RootBoard* rb, T& n, const ColoredBoard<(Colors)-C>& b, Move m, unsigned int depth): rb(rb), n(n), b(b), m(m), depth(depth) {};
	void job() {
		QTextStream xout(stderr);
		//xout << depth;
		rb->perft<C, root>(n, b, m, depth);
	}
};

template<Colors C>
class RootPerftJob: public signalJob {
	RootBoard* rb;
	unsigned int depth;
	
public:
	RootPerftJob(RootBoard* rb, unsigned int depth): rb(rb), depth(depth) {};
	void job() {
		rb->pt = new TranspositionTable<PerftEntry, 1>;
		connect(this, SIGNAL(result(QString)), rb->console, SLOT(getResult(QString)));
		uint64_t n=rb->rootPerft<C>(depth);
		emit(result(QString("%1").arg(n)));
		delete rb->pt;
	}
};

template<Colors C>
class RootDivideJob: public signalJob {
	RootBoard* rb;
	unsigned int depth;

public:
	RootDivideJob(RootBoard* rb, unsigned int depth): rb(rb), depth(depth) {};
	void job() {
		rb->pt = new TranspositionTable<PerftEntry, 1>;
		connect(this, SIGNAL(result(QString)), rb->console, SLOT(getResult(QString)));
		uint64_t n=rb->rootDivide<C>(depth);
		emit(result(QString("%1").arg(n)));
		delete rb->pt;
	}
};

#endif /* JOBS_H_ */
