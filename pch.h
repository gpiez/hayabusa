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

// to make kdevelop happy, which doesn't parse gcc command line arguments right

#ifndef __GXX_EXPERIMENTAL_CXX0X__
#define __GXX_EXPERIMENTAL_CXX0X__
#endif
#ifndef __SSE3__
#define __SSE3__
#endif
#ifndef __SSSE3__
#define __SSSE3__
#endif

extern "C" void __throw_bad_alloc();
#include <x86intrin.h>
#include <mutex>
#include <condition_variable>
#include <thread>
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

#ifdef QT_GUI_LIB
#include <QtCore>
#include <QtGui>
#endif

#endif /* PCH_H_ */
