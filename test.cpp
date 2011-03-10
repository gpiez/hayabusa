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

static inline uint64_t readtsc() {
    uint64_t a, d;
    asm volatile("cpuid\n rdtsc" : "=a" (a), "=d" (d) :: "%rbx", "%rcx");
    return a + (d << 32);
}

void TestRootBoard::initTestCase() {
    qRegisterMetaType<std::string>("std::string");
    BoardBase::initTables();
    StringList args;
    static char* argv[] = { "UnitTest", NULL };
    static int argc = 1;
    c = new Console(argc, argv);
    b = c->board;
}

void TestRootBoard::cmpMates(std::string bstr, std::string mstr) {
    Move ml[256];
    Move* good = ml+192;
    Move* bad=good;
    b->setup(bstr);
    b->boards[0].wb.generateMateMoves<false>(&good, &bad);
    StringList mstrl = split(mstr);
    while(good < ml+192) {
        std::string res = good++->algebraic();
        StringList::iterator i = find(mstrl.begin(), mstrl.end(), res);
        if (i!=mstrl.end())
            mstrl.erase(i);
        else
            std::cerr << bstr << ": " << res << " missing mate" << std::endl;
    }
    for (StringList::iterator i = mstrl.begin(); i != mstrl.end(); ++i) {
        std::cerr << bstr << ": " << *i << " wrong mate" << std::endl;
    }
}

void TestRootBoard::generateMateMoves() {
    cmpMates("6k/R/1R/////K w - -", "b6b8");
    cmpMates("K/////1R/R/6k w - -", "b3b1");
    cmpMates("5nkn/RR//////K w - -","b7g7");
    cmpMates("5nkn/QR//////K w - -", "a7g1 b7g7");
    cmpMates("5nkn/RQ//////K w - -", "b7g7 b7g2");
    cmpMates("R/5nkn/RQ/////K w - -", "b6g6 b6g1");
    cmpMates("R/5nkn/QR/////K w - -", "b6g6");
    cmpMates("Q/5nkn/RR/////K w - -", "b6g6");
    cmpMates("6k/R/1Q/////K w - -", "b6d8 b6b8");
    cmpMates("6k/R/4Q/////K w - -", "e6e8 e6c8");
    cmpMates("6k/6pp/4Q/////K w - -", "e6e8");
    cmpMates("6k/5ppp/4Q/////K w - -", "e6e8 e6c8");
    cmpMates("6k/5pbp/4Q/////K w - -", "");
    cmpMates("7k/4r1p1/5p1p/1pp2Q2/1b2B1P1/1P2P2P/5PK1/q7 w - -", "f5c8 f5h7"); 
}

void TestRootBoard::pieceList() {
}

void TestRootBoard::generateCaptures() {
    cpu_set_t mask;
    CPU_ZERO( &mask );
    CPU_SET( 1, &mask );
    if ( sched_setaffinity( 0, sizeof(mask), &mask ) == -1 )
        qDebug() << "Could not set CPU Affinity" << endl;
    static const unsigned testCases = 200;
    static const int iter = 1000;
    typedef QVector<uint64_t> Sample;
    QVector<Sample> times(testCases, Sample(iter));
    QVector<Sample> movetimes(testCases, Sample(iter));
    QVector<Sample> captimes(testCases, Sample(iter));
    Move moveList[256];
    uint64_t sum=0;
    uint64_t movesum=0;
    uint64_t nmoves=0;
    uint64_t ncap =0;
    uint64_t a, d, tsc;
    Key blah;
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
//                      b->setup(testPositions[i]);
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

            tsc = readtsc();
            Move* good = moveList+192;
            Move* bad = good;
            if (color[i] == White)
                b->boards[i].wb.generateCaptureMoves<false>(good, bad);
            else
                b->boards[i].bb.generateCaptureMoves<false>(good, bad);
            ncap += bad - good;
            captimes[i][j] = readtsc() - tsc - overhead;

            tsc = readtsc();
            if (color[i] == White)
                b->boards[i].wb.generateNonCap(good, bad);
            else
                b->boards[i].bb.generateNonCap(good, bad);
            nmoves += bad - good;
            times[i][j] = readtsc() - tsc - overhead;
            for (Move* k=good; k<bad; ++k) {
//                              std::cout << k->string() << std::endl;
                tsc = readtsc();
                if (color[i] == White) {
                    __v8hi est = b->boards[i].wb.estimatedEval(*k, b->eval);
                    ColoredBoard<Black> bb(b->boards[i].wb, *k, est);
                    blah += bb.getZobrist();
                } else {
                    __v8hi est = b->boards[i].bb.estimatedEval(*k, b->eval);
                    ColoredBoard<White> bb(b->boards[i].bb, *k, est);
                    blah += bb.getZobrist();
                }
                movetimes[i][j] += readtsc() - tsc - overhead;
            }
//                      std::string empty;
//                      std::cin >> empty;
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
//      Zobrist::test();
    b->setup("5n1n/4kPPP/////pppK4/N1N5 w - - 0 1");
    WorkThread::findFree()->queueJob(0U, new RootPerftJob<White>(*b, 6));
    QCOMPARE( c->getAnswer(), std::string("71179139"));

    b->setup("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
    WorkThread::findFree()->queueJob(0U, new RootPerftJob<Black>(*b, 6));
    QCOMPARE( c->getAnswer(), std::string("71179139"));

    b->setup("r3k2r/B1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 1");
    WorkThread::findFree()->queueJob(0U, new RootPerftJob<Black>(*b, 5));
    QCOMPARE( c->getAnswer(), std::string("176577789"));

    b->setup("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    WorkThread::findFree()->queueJob(0U, new RootPerftJob<White>(*b, 7));
    QCOMPARE( c->getAnswer(), std::string("178633661"));

    b->setup("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    WorkThread::findFree()->queueJob(0U, new RootPerftJob<White>(*b, 5));
    QCOMPARE( c->getAnswer(), std::string("193690690"));

    b->setup("rnbqk2r/pppp1Np1/8/2b4p/4P3/8/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
    WorkThread::findFree()->queueJob(0U, new RootPerftJob<White>(*b, 6));
    QCOMPARE( c->getAnswer(), std::string("1273001810"));

    b->setup("4k2r/8/8/8/8/6nB/8/4K2R w Kk - 0 1");
    WorkThread::findFree()->queueJob(0U, new RootPerftJob<White>(*b, 8));
    QCOMPARE( c->getAnswer(), std::string("9941334384"));
}

QTEST_APPLESS_MAIN(TestRootBoard);
