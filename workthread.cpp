/*
    hayabusa, chess engine
    Copyright (C) 2009-2010 Gunther Piez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <pch.h>

#include "workthread.h"
#include "jobs.h"

WorkThread::WorkThread():
	doStop(true),
	keepRunning(true),
	isStopped(true),
	job(NULL)
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
		isStopped = true;
		startable.wakeAll();	//wake stop() function
		while(isStopped)		//isStopped is resetted by startJob() and free()
			starting.wait(mutex);//starting is signalled by StartJob()
		mutex->unlock();
//        qDebug() << "execute" << (const void*)job;
        job->job();             //returning the result must have happened here.
	}
}

void WorkThread::stop() {
	mutex->lock();
	while (!isStopped) {
		doStop = true;
		startable.wait(mutex);	//waits until the end of the run loop is reached.
	}
	mutex->unlock();
}

void WorkThread::startJob(Job *j) {
	ASSERT(j);
	delete job;
	job = j;
	mutex->lock();
	isStopped = false;
	starting.wakeOne();
	mutex->unlock();
}

void WorkThread::end() {
	mutex->lock();
	keepRunning = false;
	doStop = true;
	delete job;
//	job = new VoidJob();
	starting.wakeOne();
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

bool WorkThread::isFree() { // TODO merge into startJob
	mutex->lock();
	bool ret = isStopped;
	isStopped = false;		// return true only once
	mutex->unlock();
	return ret;
}
