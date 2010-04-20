/*
 * workthread.cpp
 *
 *  Created on: 22.09.2009
 *      Author: gpiez
 */
#include <pch.h>

#include "workthread.h"
#include "board.h"
#include "jobs.h"

WorkThread::WorkThread():
	doStop(true),
	keepRunning(true),
	isStopped(true)
{
	mutex = new QMutex;
	board.init();
}

WorkThread::~WorkThread()
{
	delete mutex;
}

void WorkThread::run() {
	// TODO implement WorkThread::run

	while(keepRunning) {

		mutex->lock();
		doStop = false;
		startable.wakeOne();
		starting.wait(mutex);
		isStopped = false;
		mutex->unlock();

		job->job();
			
		mutex->lock();
		isStopped = true;
		stopped.wakeOne();
		mutex->unlock();
	}
}

void WorkThread::stop() {
	mutex->lock();
	if (!isStopped) {
		doStop = true;
		stopped.wait(mutex);	//waits until the end of the run loop is reached.
	}
	mutex->unlock();
}

void WorkThread::startJob(Job *j) {
	ASSERT(j);
	stop();
	delete job;
	job = j;
	mutex->lock();
	if (doStop)
		startable.wait(mutex);	//we are somewhere at the run loop. Wait until in a defined state.
	starting.wakeOne();
	mutex->unlock();
}

void WorkThread::end() {
	mutex->lock();
	keepRunning = false;
	doStop = true;
	stopped.wait(mutex);
	mutex->unlock();
}

void WorkThread::setJob(Job* j) {
	ASSERT(j);
	delete job;
	job = j;
}

void* WorkThread::operator new (size_t s) {
	void *p;
	posix_memalign(&p, CACHE_LINE_SIZE, s);
	return p;
}
