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

std::multimap<unsigned,Job*>    WorkThread::jobs;
Mutex                           WorkThread::runningMutex;
unsigned int                    WorkThread::running = 0;
volatile unsigned int           WorkThread::waiting = 0;
unsigned int                    WorkThread::nThreads = 0;
std::vector<WorkThread*>        WorkThread::threads;
volatile bool                   WorkThread::doStop = false;
unsigned                        WorkThread::logWorkThreads;
unsigned                        WorkThread::nWorkThreads;

__thread Stats stats;
__thread unsigned threadId = 0;
__thread bool isMain = false;
__thread int lastPositionalEval = 0;
__thread RepetitionKeys keys;
__thread WorkThread* workThread;

#ifdef __WIN32__
namespace boost {
    void tss_cleanup_implemented(void) {}
}
#endif

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

void WorkThread::run() {
    lastPositionalEval = 0;
    stats = &::stats;
    threadId = &::threadId;
    workThread = this;

    UniqueLock <Mutex> lock(runningMutex);
#ifdef __linux__
    std::stringstream name;
    name << "WorkThread " << std::find(threads.begin(), threads.end(), this) - threads.begin();
    prctl(PR_SET_NAME, name.str().c_str() );
#endif
    while(keepRunning) {

        if (doStop) stopped.notify_one();    //wake stop() function
        do {
            if (!doStop && jobs.size()>0 && running<nThreads) {
                unsigned parent;
                if (waiting)
                    job = findGoodJob(parent);

                if (!job) {
                    auto last = jobs.end();
                    --last;
                    parent = (*last).first;
                    job = (*last).second;
                    jobs.erase(last);
                }

                getThreadId() = findFreeChild(parent);
                ++running;
//                 ASSERT(running <= nThreads);
//                std::cerr << "deque:" << (void*)job << " parent:" << parent << " id:" << getThreadId() << " count:" << jobs.size() << std::endl;
            }
            if (!job) starting.wait(lock);//starting is signalled by StartJob() or QJob()
            ASSERT(job);
            stats->jobs++;
        } while(!job);
        isStopped = false;
        doStop = false;
//        std::cerr << "start:" << (void*)job << " parent:" << ((getThreadId()-1) >> logWorkThreads) << " id:" << getThreadId() << " count:" << jobs.size() << std::endl;
//        ASSERT(running <= nThreads);
        runningMutex.unlock();
        job->job();             //returning the result must have happened here.
        runningMutex.lock();
//        std::cerr << "end  :" << (void*)job << " parent:" << ((getThreadId()-1) >> logWorkThreads) << " id:" << getThreadId() << " count:" << jobs.size() << std::endl;
        delete job;
        job = NULL;
        --running;
        isStopped = true;
    }
}

// try to find jobs for children, grandchildren of a current waiting job
Job* WorkThread::findGoodJob(unsigned& parent) {
    ASSERT(jobs.size() > 0);
    Job* j;

    for (unsigned generation = 1; generation <= maxThreadId; generation*=nWorkThreads)
        for (auto i=threads.begin(); i != threads.end(); ++i) {
            if ((*i)->isWaiting) {
                parent = (*i)->getThreadId();
                auto ljob = jobs.lower_bound( parent   * generation);
                auto ujob = jobs.upper_bound((parent+1)* generation);

                if (ljob != ujob) {
                    j = (*ljob).second;
                    jobs.erase(ljob);
                    --waiting;
                    return j;
                }
            }
        }
    return NULL;
}

unsigned WorkThread::findFreeChild(unsigned parent) {
    bool used[nWorkThreads];
    for (unsigned i=0; i<nWorkThreads; ++i)
        used[i] = false;

    unsigned firstChild = parent*nWorkThreads+1;
    for (auto ch = threads.begin(); ch !=threads.end(); ++ch)
        if (!(*ch)->isStopped) {
            if ((*ch)->getThreadId() >= firstChild && (*ch)->getThreadId() <  firstChild + nWorkThreads)
                used[(*ch)->getThreadId() - firstChild] = true;
        }

    for (unsigned i=0; i<nWorkThreads; ++i)
        if (!used[i])
            return i + firstChild;

    for (auto ch = threads.begin(); ch !=threads.end(); ++ch) {
        std::cout << (void*)(*ch) << " " << (void*)(*ch)->job;
        if ((*ch)->job) std::cout << " " << (*ch)->getThreadId();
        std::cout << std::endl;
    }
    ASSERT(!"no free child ID");
    return 0;
}

// Queue a job from a parent. This allows the parent later to start only its
// own jobs.
void WorkThread::queueJob(unsigned parent, Job *j) {
    UniqueLock<Mutex> lock(runningMutex);
    if (running < nThreads)
        for (auto th = threads.begin(); th !=threads.end(); ++th) {
            if ((*th)->isStopped) {
                ASSERT((*th)->job == NULL);
                (*th)->getThreadId() = findFreeChild(parent);
                (*th)->job = j;
                (*th)->isStopped = false;
                ++running;
//    std::cerr << "qstrt:" << (void*)j << " parent:" << parent << " id:" << th->getThreadId() << " count:" << jobs.size() << std::endl;
                (*th)->starting.notify_one();
                return;
            }
        }

    jobs.insert(std::pair<unsigned, Job*>(parent, j));
//    std::cerr << "queue:" << (void*)j << " parent:" << parent << " count:" << jobs.size() << std::endl;
}

void WorkThread::idle(int n) {
//  TODO decreasing the number of running threads should only allow to start new
//  jobs in child nodes, otherwise a unrelated node may be started and not
//  finished, while join() continues, which increases the number of running and
//  executed threads beyond the number of cores
    UniqueLock<Mutex> lock(runningMutex);
    running -= n;
    waiting += n;
    workThread->isWaiting = true;
    if (n>0 && running < nThreads && jobs.size() > 0)
        for (auto th = threads.begin(); th !=threads.end(); ++th)
            if ((*th)->isStopped) {
                ASSERT((*th)->job == NULL);
                unsigned parent;
                (*th)->job = (*th)->findGoodJob(parent);
                if ((*th)->job) {
                    (*th)->getThreadId() = findFreeChild(parent);
                    (*th)->isStopped = false;
                    ++running;
//    std::cerr << "istrt:" << (void*)th->job << " parent:" << parent << " id:" << th->getThreadId() << " count:" << jobs.size() << std::endl;
                    (*th)->starting.notify_one();
                    break;
                }
            }
}

void WorkThread::stop() {
    UniqueLock<Mutex> lock(stoppedMutex);
    while (!isStopped) {
        doStop = true;
        stopped.wait(lock);    //waits until the end of the run loop is reached.
    }
}

void WorkThread::stopAll() {
    runningMutex.lock();
    for(auto j=jobs.begin(); j!=jobs.end(); ++j)
        delete (*j).second;
    jobs.clear();
    for (auto th = threads.begin(); th !=threads.end(); ++th)
        (*th)->doStop = true;
    runningMutex.unlock();
    for (auto th = threads.begin(); th !=threads.end(); ++th)
        (*th)->stop();
}

Job* WorkThread::getJob(unsigned parent) {
    UniqueLock<Mutex> lock(runningMutex);
    return findJob(parent);
}

Job* WorkThread::findJob(unsigned parent) {
    auto wjob = jobs.find(parent);
    if (wjob != jobs.end()) {
        Job* j = (*wjob).second;
        jobs.erase(wjob);
        return j;
    }

    return 0;
}

bool WorkThread::canQueued(unsigned parent, int ) {
    UniqueLock<Mutex> lock(runningMutex);
    return jobs.count(parent) < nThreads;
}

void WorkThread::printJobs() {
    for (auto i=jobs.begin(); i!=jobs.end(); ++i) {
        std::cout << (*i).first << ": " << (*i).second << std::endl;
    }
}

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
    for (unsigned int i=0; i<nWorkThreads; ++i) {
        WorkThread* th = new WorkThread();
        new Thread(&WorkThread::run, th);
        threads.push_back(th);
    }
}


WorkThread* WorkThread::findFree() {
#ifdef MYDEBUG
    std::unique_lock<std::mutex> lock(runningMutex);
    for(auto th = threads.begin(); th != threads.end(); ++th)
        if (!(*th)->isStopped)
            ASSERT(!"still running jobs");
#endif
    return threads.front();
}


const std::vector< WorkThread* >& WorkThread::getThreads() {
    return threads;
}

extern "C" void __throw_bad_alloc() { asm("int3"); }
