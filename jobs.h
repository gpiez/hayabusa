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

struct Job {
	virtual void job() = 0;
};

template<Colors C>
class RootSearchJob: public Job {
	RootBoard* rb;
public:
	RootSearchJob(RootBoard* rb): rb(rb) {};
	void job() {
		rb->rootSearch<C>();
	}
};

/*
class PerftJob: public QObject, public Job {
Q_OBJECT
	unsigned int depth;
	Console* con;
public:
	PerftJob(unsigned int depth, Console* con): depth(depth), con(con) {};
	void job(Board* b) {
		connect(this, SIGNAL(resultAvailable(QString)), con, SLOT(getResult(QString)));
		QString result;
		QTextStream(&result) << b->perft(depth);
		emit(resultAvailable(result));
	}
signals:
	void resultAvailable(QString);		
};

class DivideJob: public Job {
	unsigned int depth_;
public:
	DivideJob(unsigned int depth): depth_(depth) {};
	void job(Board* b) {
		b->divide(depth_);
	}
};


class SearchJob: public Job {
	unsigned int depth;
	Board* parent;
	BoardBase* current;
	RootBoard* rb;
public:
	SearchJob(unsigned int depth, RootBoard* rb): depth(depth), rb(rb) {};
	void job(Board*) {
		rb->rootSearch(depth);
	}
};
*/
#endif /* JOBS_H_ */
