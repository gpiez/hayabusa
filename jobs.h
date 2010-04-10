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

#include "board.h"
#include "console.h"

struct Job {
	virtual void job(Board*) = 0;
};

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
	unsigned int depth_;
public:
	SearchJob(unsigned int depth): depth_(depth) {};
	void job(Board* b) {
		b->search(depth_);
	}
};

class RootSearchJob: public Job {
	unsigned int depth;
public:
	RootSearchJob(unsigned int depth): depth(depth) {};
	void job(Board* b) {
		b->rootSearch(depth);
	}
};


#endif /* JOBS_H_ */
