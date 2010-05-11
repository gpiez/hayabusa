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
	BoardBase::initTables();
	c = new Console(QCoreApplication::instance());
	b = c->board;
}

void TestRootBoard::setPiece() {
	
	b->setup("/ w - -");
#if 0	
	QCOMPARE(b.getLen(0, d4), 4U);
	QCOMPARE(b.getLen(4, f4), 5U);
	b.setPiece(Queen, e4);
	QCOMPARE(b.getLen(0, d4), 1U);
	QCOMPARE(b.getLen(4, f4), 1U);
	QVERIFY(b.attackedBy<Queen>(f4));
	QVERIFY(b.attackedBy<Queen>(d4));
	QVERIFY(b.attackedBy<Queen>(e3));
	QVERIFY(b.attackedBy<Queen>(e5));

	b.setPiece(Rook, b4);

	QVERIFY(b.attackedBy<Queen>(f4));
	QVERIFY(b.attackedBy<Queen>(d4));
	QVERIFY(b.attackedBy<Queen>(e3));
	QVERIFY(b.attackedBy<Queen>(e5));

	b.clrPiece(&b, Queen, e4);

	QVERIFY(!b.attackedBy<Queen>(f4));
	QVERIFY(!b.attackedBy<Queen>(d4));
	QVERIFY(!b.attackedBy<Queen>(e3));
	QVERIFY(!b.attackedBy<Queen>(e5));

	QVERIFY(b.attackedBy<Rook>(d4));
	QVERIFY(b.attackedBy<Rook>(a4));
	QVERIFY(b.attackedBy<Rook>(b8));
	QVERIFY(b.attackedBy<Rook>(b1));

	b.setPiece(Knight, c7);

	QVERIFY(b.attackedBy<Knight>(e8));
	QVERIFY(b.attackedBy<Knight>(e6));
	QVERIFY(b.attackedBy<Knight>(d5));

	b.setPiece(Pawn, d4);

	QVERIFY(b.attackedBy<Pawn>(e5));
	QVERIFY(b.attackedBy<Pawn>(c5));

	b.setPiece(King, h1);

	QVERIFY(b.attackedBy<King>(g1));
	QVERIFY(b.attackedBy<King>(g2));
	QVERIFY(b.attackedBy<King>(h2));

	b.setPiece(-Pawn, d5);

	QVERIFY(b.attackedBy<-Pawn>(e4));
	QVERIFY(b.attackedBy<-Pawn>(c4));

	QBENCHMARK {
	b.setPiece(Rook, g4);
	b.clrPiece(&b, Rook, g4);
	b.setPiece(Rook, g8);
	b.clrPiece(&b, Rook, g8);
	}
#endif
}

void TestRootBoard::pieceList() {
	PieceList p;
	p.init();
	p.add(Queen, d1);
	QCOMPARE((unsigned int )p[Queen], 1U);
	p.add(Rook, a1);
	QCOMPARE((unsigned int )p[Rook], 1U);
	p.add(Bishop, c1);
	QCOMPARE((unsigned int )p[Bishop], 1U);
	p.add(King, e1);
	QCOMPARE((unsigned int )p[King], 1U);
	p.add(Knight, b1);
	QCOMPARE((unsigned int )p[Knight], 1U);
	p.add(Pawn, a2);
	QCOMPARE((unsigned int )p[Pawn], 1U);
	p.add(Pawn, b2);
	QCOMPARE((unsigned int )p[Pawn], 2U);

	QCOMPARE((unsigned int )p.getKing(), (unsigned int )e1);
	QCOMPARE((unsigned int )p.getPawn(0), (unsigned int )b2);
	QCOMPARE((unsigned int )p.getPawn(1), (unsigned int )a2);
	QCOMPARE((unsigned int )p.get(Rook, 0), (unsigned int )a1);

	p.move(a2, h8);
	QCOMPARE((unsigned int )p.getPawn(1), (unsigned int )h8);

	p.sub(Pawn, h8);
	QCOMPARE((unsigned int )p.getKing(), (unsigned int )e1);
	QCOMPARE((unsigned int )p.getPawn(0), (unsigned int )b2);
	QCOMPARE((unsigned int )p.get(Rook,0), (unsigned int )a1);

}

void TestRootBoard::generateCaptures() {
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 1, &mask );
	if( sched_setaffinity( 0, sizeof(mask), &mask ) == -1 )
		qDebug() << "Could not set CPU Affinity" << endl;
	static const unsigned testCases = 200;
	static const int iter = 256;
	typedef QVector<uint64_t> Sample;
	QVector<Sample> times(testCases, Sample(iter));
	QVector<Sample> movetimes(testCases, Sample(iter));
	Move list[256];
	uint64_t sum=0;
	uint64_t movesum=0;
	uint64_t nmoves=0;
	uint64_t a, d, tsc;
	Key blah;
	Move* end;
	sleep(2);
	for (int j = 0; j < iter; ++j) {
		nmoves = 0;
		for (unsigned int i = 0; i < testCases; ++i) {
			b->setup(testPositions[i]);
			uint64_t  overhead;
			/*
			 asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			 tsc = (a + (d << 32));
			 asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			 overhead = (a + (d << 32)) - tsc;
			 */
			overhead = 272;
			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			tsc = (a + (d << 32));
			end = b->boards[0].wb.generateMoves(list);
			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			nmoves += end - list;
			times[i][j] = (a + (d << 32)) - tsc - overhead;
			for (Move* k=list; k<end; ++k) {
				asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
				tsc = (a + (d << 32));
				ColoredBoard<Black> bb(b->boards[0].wb, *k, *b);
				blah += bb.getZobrist();
				asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
				movetimes[i][j] += (a + (d << 32)) - tsc - overhead;
			}
		}
	}
	for (QVector<Sample>::Iterator i = times.begin(); i != times.end(); ++i) {
		qSort(*i);
		sum += (*i)[iter / 16];
	}
	for (QVector<Sample>::Iterator i = movetimes.begin(); i != movetimes.end(); ++i) {
		qSort(*i);
		movesum += (*i)[iter / 16];
	}

	QTextStream xout(stderr);
    xout << endl << nmoves << " Moves, " << sum/nmoves << " Clocks, " << 3750.0*nmoves/sum << " generated Mmoves/s, " << 3750.0*nmoves/movesum << " executed Mmoves/s" << endl;
	xout << blah;
	xout << dec;

}

void TestRootBoard::perft() {
  	b->setup("4k2r/8/8/8/8/6nB/8/4K2R w Kk - 0 1");
  	b->threads.first()->startJob(new RootPerftJob<White>(b, 8));
 	QCOMPARE( c->getAnswer(), QString("9941334384"));

 	b->setup("rnbqk2r/pppp1Np1/8/2b4p/4P3/8/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
 	b->threads.first()->startJob(new RootPerftJob<White>(b, 6));
 	QCOMPARE( c->getAnswer(), QString("1273001810"));

	b->setup("r3k2r/B1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 1");
	b->threads.first()->startJob(new RootPerftJob<Black>(b, 5));
	QCOMPARE( c->getAnswer(), QString("176577789"));

	b->setup("5n1n/4kPPP/////pppK4/N1N5 w - - 0 1");
	b->threads.first()->startJob(new RootPerftJob<White>(b, 6));
	QCOMPARE( c->getAnswer(), QString("71179139"));

	b->setup("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
	b->threads.first()->startJob(new RootPerftJob<Black>(b, 6));
	QCOMPARE( c->getAnswer(), QString("71179139"));

	b->setup();
	b->threads.first()->startJob(new RootPerftJob<White>(b, 6));
	QCOMPARE( c->getAnswer(), QString("119060324"));

	b->setup("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	b->threads.first()->startJob(new RootPerftJob<White>(b, 5));
	QCOMPARE( c->getAnswer(), QString("193690690"));

	b->setup("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
	b->threads.first()->startJob(new RootPerftJob<White>(b, 7));
	QCOMPARE( c->getAnswer(), QString("178633661"));
}

QTEST_MAIN(TestRootBoard);
