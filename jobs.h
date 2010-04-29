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
#include "rootboard.tcc"

template<typename T> class Result;

struct Job {
	virtual ~Job() {};			// the for calling destructor of QObject in signalJob
	virtual void job() = 0;
};

struct signalJob: public QObject, public Job {
	Q_OBJECT
signals:
    void result(QString arg1);
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

template<Colors C>
class PerftJob: public Job {
	RootBoard* rb;
	Result<uint64_t>* n;
	const ColoredBoard<(Colors)-C>* b;
	Move m;
	unsigned int depth;
public:
	PerftJob(RootBoard* rb, Result<uint64_t>* n, const ColoredBoard<(Colors)-C>* b, Move m, unsigned int depth): rb(rb), n(n), b(b), m(m), depth(depth) {};
	void job() {
		rb->perft<C>(n, b, m, depth);
	}
};

extern uint64_t saved;

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
		qDebug() << saved;
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
		qDebug() << saved;
		delete rb->pt;
	}
};

class SearchJob: public Job {
	unsigned int depth;
	BoardBase* current;
	RootBoard* rb;
public:
	SearchJob(unsigned int depth, RootBoard* rb): depth(depth), rb(rb) {};
	void job() {
//		rb->rootSearch(depth);
	}
};

#endif /* JOBS_H_ */
