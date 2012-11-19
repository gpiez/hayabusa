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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winline"

#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <set>
#include <map>

#include <climits>
#include <cstdint>
#include <cstring>
#include <cmath>

#ifdef QT_GUI_LIB
#include <QtCore>
#include <QtGui>
#endif

#if defined(__x86_64__)
#include <x86intrin.h>
#elif defined(__arm__)
#define USE_BOOST
#endif

#ifdef __neon__
#include <arm_neon.h>
#endif

#ifdef USE_BOOST

#define BOOST_USE_WINDOWS_H
#define BOOST_THREAD_USE_LIB
#define BOOST_NO_CHAR16_T 1
#define BOOST_NO_CHAR32_T 1
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
//#include <boost/chrono/duration.hpp>
//#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/config.hpp>
typedef boost::thread Thread;
typedef boost::mutex Mutex;
typedef boost::condition_variable Condition;
#define wait_for(a,b,c) timed_wait(a,b,c)
typedef boost::recursive_mutex RecursiveMutex;
typedef boost::timed_mutex TimedMutex;
namespace boost {
	template < typename Rep, typename Period>
	posix_time::time_duration convert(chrono::duration<Rep, Period> const & from)
	{
		typedef chrono::duration<Rep, Period> src_duration_t;
		typedef chrono::nanoseconds duration_t;
		typedef duration_t::rep rep_t;
		rep_t d = chrono::duration_cast<duration_t>(from).count();
		rep_t sec = d/1000000000;
		rep_t nsec = d%1000000000;
		return  posix_time::seconds(static_cast<long long>(sec))+
	#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
				posix_time::nanoseconds(nsec);
	#else
		posix_time::microseconds((nsec+500)/1000);
	#endif
	}

	template < typename Duration>
	posix_time::ptime convert(const chrono::time_point<chrono::system_clock, Duration>& from)
	{
		typedef chrono::time_point<chrono::system_clock, Duration> time_point_t;
		typedef chrono::nanoseconds duration_t;
		typedef duration_t::rep rep_t;
		rep_t d = chrono::duration_cast<duration_t>(from.time_since_epoch()).count();
		rep_t sec = d/1000000000;
		rep_t nsec = d%1000000000;
		return  posix_time::from_time_t(0)+
				posix_time::seconds(static_cast<long>(sec))+
	#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
				posix_time::nanoseconds(nsec);
	#else
		posix_time::microseconds((nsec+500)/1000);
	#endif
	}
}

#define LockGuard boost::lock_guard
#define UniqueLock boost::unique_lock
#define CHRONO boost::chrono

#else

#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
typedef std::mutex Mutex;
typedef std::condition_variable Condition;
typedef std::thread Thread;
typedef std::recursive_mutex RecursiveMutex;
typedef std::timed_mutex TimedMutex;
#define CHRONO std::chrono
#define LockGuard std::lock_guard
#define UniqueLock std::unique_lock

#endif // USE_BOOST

#pragma GCC diagnostic pop

#endif /* PCH_H_ */

