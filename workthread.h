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

#include "coloredboard.h"

class Job;
class RootBoard;

class WorkThread: public QThread {
	QWaitCondition startable, starting;
	QMutex* mutex;
	volatile bool doStop;
	volatile bool keepRunning;
	volatile bool isStopped;
	volatile int result;
	BoardBase board;
	Colors color;
	Job* job;
//	RootBoard* root;
	
public:
	WorkThread();
	virtual ~WorkThread();

	void run();
	void stop();
	void end();
	void startJob(Job* = NULL);
	void setJob(Job*);
	void *operator new (size_t s);
	bool isFree();
};

#endif /* WORKTHREAD_H_ */
