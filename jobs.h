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

template<Colors C>
class PerftJob: public Job {
	RootBoard* rb;
	BoardBase* b;
	unsigned int depth;
public:
	PerftJob(RootBoard* rb, ColoredBoard<C>* b, unsigned int depth): rb(r), b(b), depth(depth) {};
	void job() {
		uint64_t result=rb->perft(b, depth);
		
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
