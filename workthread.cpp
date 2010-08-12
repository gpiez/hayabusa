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

#ifdef __linux__
#include <sys/prctl.h>
#endif
#include "workthread.h"
#include "jobs.h"

QMultiMap<Key,Job*> WorkThread::jobs;
//std::condition_variable WorkThread::starting;
std::mutex WorkThread::runningMutex;
unsigned int WorkThread::running = 0;
volatile unsigned int WorkThread::sleeping = 0;
unsigned int WorkThread::nThreads = 0;
QVector<WorkThread*> WorkThread::threads;
__thread bool WorkThread::isMain = false;
volatile bool WorkThread::doStop = false;
__thread Stats stats;

WorkThread::WorkThread():
    isStopped(true),
    keepRunning(true),
    job(NULL)
{
    board.init();
}

WorkThread::~WorkThread()
{
}

const Stats* WorkThread::getStats() {
    return stats;
}

void WorkThread::run() {
    stats = &::stats;
    std::unique_lock<std::mutex> lock(runningMutex);
#ifdef __linux__    
    std::stringstream name;
    name << "WorkThread " << threads.indexOf(this);
    prctl(PR_SET_NAME, name.str().c_str() );
#endif    
    while(keepRunning) {

        isStopped = true;
        if (doStop) stopped.notify_one();    //wake stop() function
        do {
            if (!doStop && jobs.count()>0 && running<4) {
                if (sleeping) {
                    QMultiMap<Key, Job*>::iterator i = jobs.upperBound(key);
                    if (i != jobs.begin()) {
                        --i;
//                        std::cerr << (key >> 56) << ":";
                        key = i.key();
//                        std::cerr << (key >> 56) << std::endl;
                        job = jobs.take(key);
                        running++;
                    }
                } else {
                    key = jobs.keys().first();
                    job = jobs.take(key);
                    running++;
//                    std::cerr << "deque " << i << " " <<(void*)job << " count " << jobs.count() << std::endl;
                }
            }
            if (!job) starting.wait(lock);//starting is signalled by StartJob() or QJob()
            stats->jobs++;
        } while(!job);
        isStopped = false;
        doStop = false;
        ASSERT(running <= 4);
//        std::cerr << "start " << i << " " << (void*)job << " count " << jobs.count() << std::endl;
        runningMutex.unlock();
        job->job();             //returning the result must have happened here.
        runningMutex.lock();
//        std::cerr << "end   " << i << " " << (void*)job << " count " << jobs.count() << std::endl;
        delete job;
        job = NULL;
        running--;
    }
}

// Queue a job from a parent. This allows the parent later to start only its
// own jobs.
void WorkThread::queueJob(Key parent, Job *j) {
    ASSERT(j);
    std::unique_lock<std::mutex> lock(runningMutex);
/*    for (int i = 0; i<threads.count(); ++i) {
        WorkThread* th = threads[i];*/
    if (running<4)
    foreach (WorkThread* th, threads) {
        if (th->isStopped) {
            ASSERT(th->job == NULL);
            th->isStopped = false;
            th->job = j;
//            std::cerr << "notfy " << i << " " << (void*)j << " count " << jobs.count() << std::endl;
//            runningMutex.unlock();
            running++;
            th->starting.notify_one();
            return;
        }
    }

//    std::cerr << "queue " << (void*)j << " count " << jobs.count() << std::endl;
    jobs.insertMulti(parent, j);
//    starting.notify_all();
}

void WorkThread::idle(int n) {
//  TODO decreasing the number of running threads should only allow to start new
//  jobs in child nodes, otherwise a unrelated node may be started and not
//  finished, while join() continues, which increases the number of running and
//  executed threads beyond the number of cores
//    std::unique_lock<std::mutex> lock(runningMutex);
//    running -= n;
//    sleeping += n;
    if (0 && n>0 && running < nThreads && jobs.count() > 0)
        foreach (WorkThread* th, threads)
            if (th->isStopped) {
                running++;
                th->starting.notify_one();
                break;
            }
    ASSERT(running <= 8);
}

void WorkThread::stop() {
    std::unique_lock<std::mutex> lock(stoppedMutex);
    while (!isStopped) {
        doStop = true;
        stopped.wait(lock);    //waits until the end of the run loop is reached.
    }
}

void WorkThread::stopAll() {
    runningMutex.lock();
    foreach(Job* j, jobs)
        delete j;
    jobs.clear();
    foreach(WorkThread* th, threads)
    	th->doStop = true;
    runningMutex.unlock();
    foreach(WorkThread* th, threads)
        th->stop();
}

Job* WorkThread::getJob(Key parent) {
    std::unique_lock<std::mutex> lock(runningMutex);
    return jobs.take(parent);
}

bool WorkThread::canQueued(Key k, int shared) {
	if (shared >= 2*(signed)WorkThread::nThreads-1) return false;
    std::unique_lock<std::mutex> lock(runningMutex);
    return (unsigned)jobs.count(k) < nThreads;
//    return (unsigned)jobs.size() < nThreads;
}

void WorkThread::printJobs() {
    for (QMultiMap<Key, Job*>::iterator i=jobs.begin(); i!=jobs.end(); ++i) {
        std::cout << i.key() << ": " << i.value() << std::endl;
    }
}

void WorkThread::init() {
    nThreads = sysconf(_SC_NPROCESSORS_ONLN);
    if (nThreads == 0) nThreads = 1;

    for (unsigned int i=0; i<nThreads*2-1; ++i) {
        WorkThread* th = new WorkThread();
        new std::thread(&WorkThread::run, th);
        threads.append(th);
    }
}


WorkThread* WorkThread::findFree() {
#ifdef MYDEBUG
    std::unique_lock<std::mutex> lock(runningMutex);
    foreach(WorkThread* th, WorkThread::threads)
        if (!th->isStopped)
            ASSERT(!"still running jobs");
#endif
    return threads.first();
}


const QVector<WorkThread*>& WorkThread::getThreads() {
    return threads;
}
