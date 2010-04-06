/*
 * test.cpp
 *
 *  Created on: 29.09.2009
 *      Author: gpiez
 */

#include "test.h"
#include "board.h"
#include "board.tcc"
#include "testpositions.h"

void TestBoard::initTestCase() {
	BoardBase::initTables();
}

void TestBoard::setPiece() {
	Board b;
	b.setup("/ w - -");
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

void TestBoard::add() {
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

void TestBoard::generateCaptures() {
	static const unsigned testCases = 200;
	static const int iter = 200;
	typedef QVector<uint64_t> Sample;
	QVector<Sample> times(testCases, Sample(iter));
	Board b;
	Move list[256];
	uint64_t sum=0;
	uint64_t nmoves=0;
	Move* end;
	for (int j = 0; j < iter; ++j) {
		nmoves = 0;
		for (unsigned int i = 0; i < testCases; ++i) {
			b.setup(testPositions[i]);
			uint64_t a, d, tsc, overhead;
			/*
			 asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			 tsc = (a + (d << 32));
			 asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			 overhead = (a + (d << 32)) - tsc;
			 */
			overhead = 272;
			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			tsc = (a + (d << 32));
			end = b.generateMoves(list);
			asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
			nmoves += end - list;
			times[i][j] = (a + (d << 32)) - tsc - overhead;
		}
	}
	for (QVector<Sample>::Iterator i = times.begin(); i != times.end(); ++i) {
		qSort(*i);
		sum += (*i)[iter / 4];
	}

    xout << endl << nmoves << " Moves, " << sum/nmoves << " Clocks, " << 3500*nmoves/sum << " Mmoves/s" << endl;
	b.setup();
//	((ColoredBoard<White>*)&b)->doMove( (Move) { h2, h4 } );
//	((ColoredBoard<Black>*) b.boards ) [0] .doMove( (Move) { e7, e5 } );
//	((ColoredBoard<White>*) b.boards ) [1] .doMove( (Move) { e2, e4 } );
//	((ColoredBoard<Black>*) b.boards ) [2] .doMove( (Move) { d7, d5 } );
//	((ColoredBoard<White>*) b.boards ) [3] .doMove( (Move) { e4, d5, -Pawn } );
//	((ColoredBoard<Black>*) b.boards ) [4] .doMove( (Move) { b8, c6 } );
//	((ColoredBoard<White>*) b.boards ) [5] .doMove( (Move) { d5, c6, -Knight } );
//	((ColoredBoard<Black>*) b.boards ) [6] .doMove( (Move) { d8, h4, Pawn } );
//	((ColoredBoard<White>*) b.boards ) [7] .doMove( (Move) { d1, e2 } );
//	((ColoredBoard<Black>*) b.boards ) [8] .doMove( (Move) { h4, h1, Rook, disableOpponentShortCastling } );
//	((ColoredBoard<White>*) b.boards ) [9] .doMove( (Move) { f2, f4, 0, enableEP } );
//	b.setup("rnb1k1nr/pppp1ppp/4p/6q//b3KP/PPPPP1PP/RNBQ1BNR w KQkq - 0 2");
/*
	Board b2;
	b2.setup();
	((ColoredBoard<White>*)&b2)->doMove( (Move) { g2, g3 } );
	for ( unsigned int i=0; i<0x3C0; ++i) {
		if (((char*)&b)[i] != ((char*)&b2.boards[0])[i]) {
			cout << "At " << hex << i << " " << ((uint8_t*)&b)[i] << " is " << ((uint8_t*)&b2.boards[0])[i] << endl;
		}
	}
*/
	b.boards[0].print();
	xout << dec;
	((ColoredBoard<Black>*) b.boards )[0].divide(6);
	//cout << sum << endl;
}

QTEST_MAIN(TestBoard);
