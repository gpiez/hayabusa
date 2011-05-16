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
#ifndef PCH_H_
#define PCH_H_

#define BITBOARD

extern "C" void __throw_bad_alloc();
#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <chrono>

#include <climits>
#include <cstdint>
#include <cstring>
#include <cmath>

#ifdef QT_GUI_LIB
#include <QtCore>
#include <QtGui>
#endif
#ifdef QT_NETWORK_LIB
#include <QtCore>
#include <QtNetwork>
#endif

#ifdef __WIN32__
#define BOOST_USE_WINDOWS_H
#define BOOST_THREAD_USE_LIB
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
typedef boost::thread Thread;
typedef boost::mutex Mutex;
typedef boost::condition_variable Condition;
typedef boost::recursive_mutex RecursiveMutex;
typedef boost::timed_mutex TimedMutex;

#define LockGuard boost::lock_guard
#define UniqueLock boost::unique_lock
#else
#include <x86intrin.h>
/*#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
typedef boost::thread Thread;
typedef boost::mutex Mutex;
typedef boost::condition_variable Condition;
typedef boost::recursive_mutex RecursiveMutex;
typedef boost::timed_mutex TimedMutex;
#define LockGuard boost::lock_guard
#define UniqueLock boost::unique_lock
/*/
#include <mutex>
#include <condition_variable>
#include <thread>
typedef std::mutex Mutex;
typedef std::condition_variable Condition;
typedef std::thread Thread;
typedef std::recursive_mutex RecursiveMutex;
typedef std::timed_mutex TimedMutex;
#define LockGuard std::lock_guard
#define UniqueLock std::unique_lock//*/
#endif

#endif /* PCH_H_ */
