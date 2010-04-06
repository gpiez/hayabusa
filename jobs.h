/*
 * jobs.h
 *
 *  Created on: Feb 3, 2010
 *      Author: gpiez
 */

#ifndef JOBS_H_
#define JOBS_H_

#include "board.h"

struct Job {
	virtual void job(Board*) = 0;
};

class PerftJob: public Job {
	unsigned int depth_;
public:
	PerftJob(unsigned int depth): depth_(depth) {};
	void job(Board* b) {
		b->perft(depth_);
	}
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
	unsigned int depth_;
public:
	RootSearchJob(unsigned int depth): depth_(depth) {};
	void job(Board* b) {
		b->rootSearch(depth_);
	}
};


#endif /* JOBS_H_ */
