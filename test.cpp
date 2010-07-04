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
#include "test.h"
#include "rootboard.h"
#include "testpositions.h"
#include "console.h"
#include "rootboard.tcc"
#include "generateMoves.tcc"
#include "transpositiontable.tcc"
#include <sched.h>

void TestRootBoard::initTestCase() {
    qRegisterMetaType<std::string>("std::string");
	BoardBase::initTables();
	c = new Console(QCoreApplication::instance());
	b = c->board;
}

void TestRootBoard::setPiece() {
}

void TestRootBoard::pieceList() {
}

void TestRootBoard::generateCaptures() {
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 1, &mask );
	if( sched_setaffinity( 0, sizeof(mask), &mask ) == -1 )
		qDebug() << "Could not set CPU Affinity" << endl;
	static const unsigned testCases = 200;
	static const int iter = 1000;
	typedef QVector<uint64_t> Sample;
	QVector<Sample> times(testCases, Sample(iter));
	QVector<Sample> movetimes(testCases, Sample(iter));
	QVector<Sample> captimes(testCases, Sample(iter));
	Move list[256];
	uint64_t sum=0;
	uint64_t movesum=0;
	uint64_t nmoves=0;
	uint64_t ncap =0;
	uint64_t a, d, tsc;
	Key blah;
	Move* end;
	Colors color[testCases];
	for (unsigned int i = testCases; i;) {
		--i;
		b->setup(testPositions[i]);
		color[i] = b->color;
		if (i) {
			b->boards[i] = b->boards[0];
		}
		movetimes[i].reserve(iter*2);
		times[i].reserve(iter*2);
		captimes[i].reserve(iter*2);
	}
	for (int j = 0; j < iter; ++j) {
		nmoves = 0;
		ncap=0;
		for (unsigned int i = 0; i < testCases; ++i) {
//			b->setup(testPositions[i]);
			uint64_t  overhead;
			/*
			 asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			 tsc = (a + (d << 32));
			 asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			 overhead = (a + (d << 32)) - tsc;
			 */
			overhead = 272;
			if (color[i] == White)
				b->boards[i].wb.buildAttacks();
			else
				b->boards[i].bb.buildAttacks();

			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			tsc = (a + (d << 32));
			if (color[i] == White)
				end = b->boards[i].wb.generateCaptureMoves<false>(list);
			else
				end = b->boards[i].bb.generateCaptureMoves<false>(list);
			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			ncap += end - list;
			captimes[i][j] = (a + (d << 32)) - tsc - overhead;

			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			tsc = (a + (d << 32));
			if (color[i] == White)
				end = b->boards[i].wb.generateMoves(list);
			else
				end = b->boards[i].bb.generateMoves(list);
			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			nmoves += end - list;
			times[i][j] = (a + (d << 32)) - tsc - overhead;
			for (Move* k=list; k<end; ++k) {
//				std::cout << k->string() << std::endl;
				asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
				tsc = (a + (d << 32));
				if (color[i] == White) {
					__v8hi est = b->boards[i].wb.estimatedEval(*k, b->eval);
					ColoredBoard<Black> bb(b->boards[i].wb, *k, est);
					blah += bb.getZobrist();
				} else {
					__v8hi est = b->boards[i].bb.estimatedEval(*k, b->eval);
					ColoredBoard<White> bb(b->boards[i].bb, *k, est);
					blah += bb.getZobrist();
				}
				asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
				movetimes[i][j] += (a + (d << 32)) - tsc - overhead;
			}
//			std::string empty;
//			std::cin >> empty;
		}
	}
	for (QVector<Sample>::Iterator i = times.begin(); i != times.end(); ++i) {
		qSort(*i);
		sum += (*i)[iter / 2];
	}
	uint64_t capsum=0;
	for (QVector<Sample>::Iterator i = captimes.begin(); i != captimes.end(); ++i) {
		qSort(*i);
		capsum += (*i)[iter / 2];
	}
	for (QVector<Sample>::Iterator i = movetimes.begin(); i != movetimes.end(); ++i) {
		qSort(*i);
		movesum += (*i)[iter / 2];
	}

	QTextStream xout(stderr);
    xout << endl << nmoves << " Moves, " << sum/nmoves << " Clocks, " << 3750.0*nmoves/sum << " generated Mmoves/s, " << 3750.0*nmoves/movesum << " executed Mmoves/s" << endl;
    xout << ncap << " Captures, " << capsum/ncap << " Clocks, " << 3750.0*ncap/capsum << " generated Mmoves/s, " /*<< 3750.0*ncap/movesum << " executed Mmoves/s" */<< endl;
	xout << blah << endl;
	xout << dec;

}

void TestRootBoard::perft() {
//	Zobrist::test();

	b->setup("4k2r/8/8/8/8/6nB/8/4K2R w Kk - 0 1");
  	WorkThread::findFree()->startJob(new RootPerftJob<White>(*b, 8));
 	QCOMPARE( c->getAnswer(), std::string("9941334384"));

 	b->setup("rnbqk2r/pppp1Np1/8/2b4p/4P3/8/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
 	WorkThread::findFree()->startJob(new RootPerftJob<White>(*b, 6));
 	QCOMPARE( c->getAnswer(), std::string("1273001810"));

	b->setup("r3k2r/B1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 1");
	WorkThread::findFree()->startJob(new RootPerftJob<Black>(*b, 5));
	QCOMPARE( c->getAnswer(), std::string("176577789"));

	b->setup("5n1n/4kPPP/////pppK4/N1N5 w - - 0 1");
	WorkThread::findFree()->startJob(new RootPerftJob<White>(*b, 6));
	QCOMPARE( c->getAnswer(), std::string("71179139"));

	b->setup("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
	WorkThread::findFree()->startJob(new RootPerftJob<Black>(*b, 6));
	QCOMPARE( c->getAnswer(), std::string("71179139"));

	b->setup();
	WorkThread::findFree()->startJob(new RootPerftJob<White>(*b, 6));
	QCOMPARE( c->getAnswer(), std::string("119060324"));

	b->setup("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	WorkThread::findFree()->startJob(new RootPerftJob<White>(*b, 5));
	QCOMPARE( c->getAnswer(), std::string("193690690"));

	b->setup("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
	WorkThread::findFree()->startJob(new RootPerftJob<White>(*b, 7));
	QCOMPARE( c->getAnswer(), std::string("178633661"));

}

QTEST_MAIN(TestRootBoard);
