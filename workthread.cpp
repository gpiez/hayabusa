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
#include <unistd.h>
#endif
#include "workthread.h"
#include "jobs.h"

std::multimap<unsigned,Job*>    WorkThread::jobs;
Mutex                           WorkThread::runningMutex;
Condition                       WorkThread::stopped;
unsigned int                    WorkThread::running;
unsigned int                    WorkThread::waiting;
unsigned int                    WorkThread::nThreads;
std::vector<WorkThread*>        WorkThread::threads;
unsigned                        WorkThread::logWorkThreads;
unsigned                        WorkThread::nWorkThreads;

__thread Stats WorkThread::stats;
__thread unsigned WorkThread::threadId;
__thread bool isMain = false;
__thread int lastPositionalEval = 0;
__thread WorkThread* workThread;
__thread unsigned WorkThread::reserved;

#ifdef __WIN32__
namespace boost {
void tss_cleanup_implemented(void) {} }
#endif

// Do not initialize from thread local variables like stats and threadId  here,
// they do not exist at this point
WorkThread::WorkThread():
    parent(0),
    job(NULL) {
}

WorkThread::~WorkThread() {}

void WorkThread::run() {
    lastPositionalEval = 0;
    Game::history.init();
    pThreadId = &threadId;
    pStats = &stats;
    workThread = this;

#ifdef __linux__
    std::stringstream name;
    name << "hayabusa worker " << std::find(threads.begin(), threads.end(), this) - threads.begin();
    prctl(PR_SET_NAME, name.str().c_str() );
#endif
    while(true) {
        UniqueLock <Mutex> lock(runningMutex);
        delete job;
        job = NULL;
        --running;
        if (running == 0) stopped.notify_one();  //wake stop() function
        ASSERT(!job);
        if (jobs.size()>0 && running<nThreads) {
            ASSERT(!job);
            auto last = jobs.end();
            --last;
            parent = (*last).first;
            job = (*last).second;
            jobs.erase(last);
            ++running; }
        starting.wait(lock, [this] () { return job != 0; });   //starting is signalled by StartJob() or QJob()
        ASSERT(job);
        STATS(jobs);
        job->job();             //returning the result must have happened here.
    } }

// Queue a job from a parent. This allows the parent later to start only its
// own jobs.
void WorkThread::queueJob(Job* j) {
    unsigned parent = threadId;
    UniqueLock<Mutex> lock(runningMutex);
    if (reserved > 0) --reserved;
    if (running < nThreads)
        for (auto th = threads.begin(); th !=threads.end(); ++th) {
            if (!(*th)->job) {
                (*th)->job = j;
                ++running;
                (*th)->parent = parent;
                (*th)->starting.notify_one();
                return; } }

    jobs.insert(std::pair<unsigned, Job*>(parent, j)); }

void WorkThread::idle(int n) {
//  TODO decreasing the number of running threads should only allow to start new
//  jobs in child nodes, otherwise a unrelated node may be started and not
//  finished, while join() continues, which increases the number of running and
//  executed threads beyond the number of cores
    UniqueLock<Mutex> lock(runningMutex);
    waiting += n;
    if (n>0 && running < nThreads && jobs.size() > 0)
        for (auto th = threads.begin(); th !=threads.end(); ++th)
            if (!(*th)->job) {
                ASSERT((*th)->job == NULL);
                auto j = jobs.begin();
                (*th)->parent = j->first;
                (*th)->job = j->second;
                jobs.erase(j);
                --waiting;
                (*th)->starting.notify_one();
                return; } }

void WorkThread::stop() {
}
void WorkThread::stopAll() {
    UniqueLock<Mutex> l(runningMutex);
    for (auto th = threads.begin(); th !=threads.end(); ++th)
        if ((*th)->job) (*th)->job->stop();
    stopped.wait(l, [] { return running == 0; });    //waits until the end of the run loop is reached.
}

Job* WorkThread::findJob(unsigned parent, unsigned ply) {
    ASSERT(!runningMutex.try_lock());
    unsigned bestPly = 0;
    std::multimap<unsigned, Job*>::iterator ij = jobs.end();
    for (auto wjob = jobs.find(parent); wjob != jobs.end(); ++wjob) {
        Job* j = (*wjob).second;
        if (j->getPly() >= ply && j->getPly() > bestPly) {
            bestPly = j->getPly();
            ij = wjob; } }

    if (ij != jobs.end() ) {
        jobs.erase(ij);
        return ij->second; }

    return 0; }

void WorkThread::executeOldJobs(unsigned ply) {
    Job* job;
    LockGuard<Mutex> l(runningMutex);
    while((job = findJob(threadId, ply))) {
        job->job(); }
}

bool WorkThread::canQueued( int ) {
    UniqueLock<Mutex> lock(runningMutex);
//     if (parent && getReserved(threads[parent]->parent) + running >= nThreads) return false;
    return running < nThreads; }

void WorkThread::printJobs() {
    for (auto i=jobs.begin(); i!=jobs.end(); ++i) {
        std::cout << (*i).first << ": " << (*i).second << std::endl; } }

void WorkThread::init() {
#ifdef __WIN32__
    nThreads = 4;
#else
    nThreads = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    if (nThreads == 0) nThreads = 1;
    for (logWorkThreads = 0; 1U<<logWorkThreads < nThreads; ++logWorkThreads)
        ;
    if (logWorkThreads) ++logWorkThreads;
    nWorkThreads = 1<<logWorkThreads;
    UniqueLock<Mutex> lock(runningMutex);
    running = nWorkThreads;
    reserved = 0;
    for (unsigned int i=0; i<nWorkThreads; ++i) {
        WorkThread* th = new WorkThread();
        new Thread(&WorkThread::run, th);
        threads.push_back(th); }
    //waits until initialized
    stopped.wait(lock, [] () { return running == 0; });

    unsigned i=0;
    for(auto th = threads.begin(); th != threads.end(); ++th)
        (*th)->getThreadId() = i++;
}


WorkThread* WorkThread::findFree() {
#ifdef MYDEBUG
    UniqueLock<Mutex> lock(runningMutex);
    for(auto th = threads.begin(); th != threads.end(); ++th)
        if ((*th)->job)
            ASSERT(!"still running jobs");
#endif
    return threads.front(); }


const std::vector< WorkThread* >& WorkThread::getThreads() {
    return threads; }

void WorkThread::clearStats() {
    for (unsigned int i=0; i<nWorkThreads; ++i) {
        threads[i]->getStats()->node = 0; } }

unsigned int WorkThread::getReserved(unsigned i) {
    ASSERT(i<nWorkThreads);
    return threads[i]->reserved; }

void WorkThread::reserve(unsigned int n) {
    reserved = std::min(nThreads/2, n); }
