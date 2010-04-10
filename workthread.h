/*
 * workthread.h
 *
 *  Created on: 22.09.2009
 *      Author: gpiez
 */

#ifndef WORKTHREAD_H_
#define WORKTHREAD_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "board.h"
class Job;

class WorkThread: public QThread {
	QWaitCondition stopped, startable, starting;
	QMutex* mutex;
	volatile bool doStop;
	volatile bool keepRunning;
	volatile bool isStopped;
	Job* job;

public:
	Board b;	//FIXME alignment
	
	WorkThread();
	virtual ~WorkThread();

	void run();
	void stop();
	void end();
	void startJob(Job* = NULL);
	void setJob(Job*);
	void *operator new (size_t s);
};

#endif /* WORKTHREAD_H_ */
