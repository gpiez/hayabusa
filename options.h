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
#ifndef OPTIONS_H_
#define OPTIONS_H_
/*
 * Definitions are in console.cpp
 */
namespace Options {
extern unsigned int splitDepth;
extern int humanreadable;
extern uint64_t hash;
extern uint64_t pHash;
extern bool quiet;
extern bool preCutIfNotThreatened;
extern unsigned veinDepth;
extern unsigned leafDepth;
extern bool reduction;
extern bool pruning;
extern unsigned debug;
extern bool currline;
extern bool cpuTime;
#ifdef QT_NETWORK_LIB
extern bool server;
#endif
}

enum DebugFlags { debugSearch = 1, debugEval = 2, debugMobility = 4 };
#endif