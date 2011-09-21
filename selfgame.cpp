/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Gunther Piez <gpiez@web.de>

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

#include <boost/chrono.hpp>
#include "selfgame.h"
#include "rootboard.tcc"

std::vector<std::string> testPosition = {
    "r2qkb1r/pp1n1ppp/2p2n2/3pp2b/4P3/3P1NPP/PPPN1PB1/R1BQ1RK1 b kq e3 0 8 ",
    "r2q1rk1/ppp1bppp/1nn1b3/4p3/1P6/P1NP1NP1/4PPBP/R1BQ1RK1 b - b3 0 10 ",
    "rnbq1rk1/p3ppbp/1p1p1np1/2p5/3P1B2/2P1PN1P/PP2BPP1/RN1QK2R w KQ - 0 8 ",
    "r1b1qrk1/ppp1p1bp/n2p1np1/3P1p2/2P5/2N2NP1/PP2PPBP/1RBQ1RK1 b - - 2 9 ",
    "rnbq1rk1/1p2ppbp/2pp1np1/p7/P2PP3/2N2N1P/1PP1BPP1/R1BQ1RK1 b - - 0 8 ",
    "2kr3r/ppqn1pp1/2pbp2p/7P/3PQ3/5NP1/PPPB1P2/2KR3R w - - 1 16 ",
    "r3kb1r/pp2pppp/1nnqb3/8/3p4/NBP2N2/PP3PPP/R1BQ1RK1 b kq - 3 10 ",
    "r2qk2r/5pbp/p1npb3/3Np2Q/2B1Pp2/N7/PP3PPP/R4RK1 b kq - 0 15 ",
    "r1bqk2r/4bpp1/p2ppn1p/1p6/3BPP2/2N5/PPPQ2PP/2KR1B1R w kq b6 0 12 ",
    "r1b1k2r/1pq3pp/p1nbpn2/3p4/3P4/2NB1N2/PP3PPP/R1BQ1RK1 w kq - 0 13 ",
    "rn1q1rk1/pp3ppp/3b4/3p4/3P2b1/2PB1N2/P4PPP/1RBQ1RK1 b - - 2 12 ",
    "r2qrbk1/1b1n1p1p/p2p1np1/1p1Pp3/P1p1P3/2P2NNP/1PB2PP1/R1BQR1K1 w - - 0 17 ",
    "r2q1rk1/pp1n1ppp/2p1pnb1/8/PbBPP3/2N2N2/1P2QPPP/R1B2RK1 w - - 1 11 ",
    "r1bq1rk1/1p2bppp/p1n1pn2/8/P1BP4/2N2N2/1P2QPPP/R1BR2K1 b - - 2 11 ",
    "r1b2rk1/pp3ppp/2n1pn2/q1bp4/2P2B2/P1N1PN2/1PQ2PPP/2KR1B1R b - - 2 10 ",
    "rnbq1rk1/2pnppbp/p5p1/1p2P3/3P4/1QN2N2/PP3PPP/R1B1KB1R w KQ - 1 10 ",
    "rnb2rk1/ppp1qppp/3p1n2/3Pp3/2P1P3/5NP1/PP1N1PBP/R2Q1RK1 w - - 1 11 ",
    "r2q1rk1/pbpn1pp1/1p2pn1p/3p4/2PP3B/P1Q1PP2/1P4PP/R3KBNR w KQ - 1 11 ",
    "r3qrk1/1ppb1pbn/n2p2pp/p2Pp3/2P1P2B/P1N5/1P1NBPPP/R2Q1RK1 w - - 1 13 "
    "1r1q1rk1/2pbppbp/p2p2p1/Q3P1B1/1n1P4/2N2N2/PPP2PPP/R4RK1 b - - 6 14 bm ;",
    "r1bqk1nr/1pp4p/p3p1p1/3p1p2/3P1P1N/2PB1Q2/P1P2PPP/R4RK1 w - - 0 14 bm ;",
    "1r3rk1/pbp2ppp/2pbp3/8/2PP3q/PP1Q2N1/4NPPP/R4K1R w - - 1 15 bm ;",
    "1r3rk1/p1p3pb/2p1p2p/2P3qP/3Pp1pb/1P2P3/PB1QBP1R/R3K3 w - - 0 19 bm ;",
    "rn2kb1r/pp2pppp/1qp2n2/2Np4/3P4/5BPP/PPPBPP2/R2QK2R b - - 0 11 bm ;",
    "r2qnrk1/pbpp2pp/4p1n1/2p1P1N1/5P2/3Q4/PPP3PP/R1B1KB1R b - - 1 13 bm ;",
    "1r2nr1k/p1pp2p1/4p3/2qbP1N1/4Bn2/2P5/PPQ3PP/R1B1K2R b - - 2 20 bm ;",
    "1n3rk1/5p1p/p5p1/2p5/4rP2/P7/PBPP1P1P/1R3KR1 w - - 0 19 bm ;",
    "b3k2r/p2r1ppp/p3pb2/N7/2P1nB2/N7/PP3PPP/R4RK1 w - - 3 18 bm ;",
    "1n2k1nr/r1p2p1p/p5p1/1p6/3p1P2/P2P1B1q/P1P1PP1P/1RBQK1R1 w - - 5 14 bm ;",
    "5b1r/p2k1pp1/r1p1p1bp/4P3/3Pp2P/2P1B3/PP2NPP1/R3K2R w - - 0 15 bm ;",
    "1r3rk1/p1p2pp1/3q3p/2pPb3/2P2p2/1PN1p3/P2Q1PPP/3R1K1R w - - 0 18 bm ;",
    "r2q1knr/2p4p/pp2b1p1/3pN2n/3P1p2/2PBB3/P1P2PPP/1R1QR1K1 w - - 0 18 bm ;",
    "r1b1r1k1/1p4pp/2n2p2/3q4/p1pP3Q/P4N2/1P1B1PPP/R4RK1 w - - 4 19 bm ;",
    "r2qr1k1/2p1bppp/b7/p1ppP3/8/2PQ1N2/PP3PPP/R1B1K2R w KQ - 1 14 bm ;",
    "rn2k2r/5ppp/pQ3n2/2p5/3pq3/P6P/PB1PPPP1/R3KB1R b kqKQ - 0 16 bm ;",
    "1k1r3r/1pp3pp/4pp1q/4n3/8/3B1QP1/PPP2P1P/3RR1K1 w - - 2 20 bm ;",
    "1r2r3/p1pq1pkp/2pp4/3pPbb1/3P2p1/1N4B1/PPP2PPP/R2QR2K w - - 12 17 bm ;",
    "r1b2k1r/pp3ppp/2p5/1n1nP3/2N2P2/P2B2PP/P2B4/R3K2R w KQ - 3 19 bm ;",
    "r3k2r/p4ppp/2pBb3/1p5q/4n3/P3PN1P/P4QP1/3RKBR1 w - - 1 20 bm ;",
    "r1bq1r2/ppp1n1pQ/3bpk2/6pB/3P4/P4N2/1PP2PPP/R3K2R w KQ - 5 13 bm ;",
    "1r3rk1/ppp1qppp/2n2n2/3p4/5B2/P2P2P1/P1P2PBP/1R1QR1K1 b - - 3 13 bm ;",
    "r1b2b1r/ppB1pkpp/2n5/3p3n/1q1P4/2N1P3/PPP2PPP/R2QKB1R b - - 0 8 bm ;",
    "5b1r/ppr4p/4pkp1/4N3/1npP4/4PB2/P4PPP/3RK2R w - c3 0 17 bm ;",
    "r1b1kb1r/ppn1pppp/2p2n2/3pN3/3P4/qRN3P1/P1PBPPBP/3Q1RK1 b - - 5 10 bm ;",
    "r1bq2k1/pp1prpp1/2p4p/1N2n3/7P/3BP3/PPP2PP1/R2QK2R w - - 0 14 bm ;",
    "r3k2r/1p3ppp/2p1b3/p1Qp3q/3R4/1PN1P1P1/P1P2P1P/2R3K1 b - - 3 19 bm ;",
    "1r3rk1/p1pqppbp/Q1n5/4Pbp1/3PpNP1/1PP1B2P/P4P2/R3KB1R w - g3 0 15 bm ;",
    "rnq1kbnr/p4ppp/2p1p1b1/3pN2P/B2P2P1/P1N1P3/2P2P2/R1BQK2R b KQkq - 0 15 bm ;",
    "7r/p1pkbppp/r3p3/1P2Pb2/3Pp3/2P5/P4PPP/R1B1K1NR b - - 0 14 bm ;",
    "2k1b2r/ppp2pp1/4p3/4P3/4B1P1/3KP2p/PPP1P2P/5R2 b - - 1 19 bm ;",
    "rn2k2r/pp3pp1/2p2n1p/3pNb2/3Pp2B/bP2P3/2PNBPPP/1R2K2R w kq - 0 15 bm ;",
    "r1b2rk1/pp2q2p/2p1ppp1/3p1P1n/P2P4/3QNNP1/1PP1RP1P/R5K1 b - - 0 17 bm ;",
    "3qkb2/p1pbpp2/p4pr1/2PP3N/4QB2/r7/5PPP/1R3RK1 w - - 0 20 bm ;",
    "r1b1k2r/b1p2pRp/4q3/p7/1p2pP2/4P3/PPPBQ1PP/1R1N2K1 b kq - 0 17 bm ;"
};

SelfGame::SelfGame(Console* c, const Parameters& wp, const Parameters& bp)
{
    wrb = new RootBoard(c, wp, 0x80000, 0x8000);
    brb = new RootBoard(c, bp, 0x80000, 0x8000);

    decisiveScore = 800;
    decisiveScoreMoves = 5;
}

SelfGame::~SelfGame() {
    delete wrb;
    delete brb;
}

void SelfGame::setupRootBoard(RootBoard* rb) {
    std::map<std::string, StringList> p;
    wtime = 100000000;
    std::stringstream t;
    t << wtime/1000000;
//     p["wtime"] = StringList() << t.str().c_str();
    btime = wtime;
    t << btime/1000000;
//     p["btime"] = StringList() << t.str().c_str();
//     p["winc"] = StringList() << "10";
//     p["binc"] = StringList() << "10";
    p["infinity"] = StringList();
    p["nodes"] = StringList() << "80000";
    rb->goReadParam(p);
}

int SelfGame::doGame(RootBoard* rb1, RootBoard* rb2)
{
//     return floor((rand()*3.0)/(RAND_MAX+1.0))-1;
    setupRootBoard(rb1);
    setupRootBoard(rb2);
    nDecisive = 0;
    Move m;
    if (rb1->color == Black) goto black;
    for (;;) {
//         rb1->setTime(wtime, btime);
//         cpuTime();
        rb1->goWait();
//         wtime -= cpuTime();
        m = rb1->bestMove;
        if (!m.data) {
            result = 0;
            break;
        }
//         std::cerr << m.string() << " ";
        rb1->doMove(m);
        rb2->doMove(m);
        if (checkResult<Black>(*rb1)) break;
black:
//         rb2->setTime(wtime, btime);
//         cpuTime();
        rb2->goWait();
//         btime -= cpuTime();
        m = rb2->bestMove;
        if (!m.data) {
            result = 0;
            break;
        }
//         std::cerr << m.string() << " ";
        rb1->doMove(m);
        rb2->doMove(m);
        if (checkResult<White>(*rb2)) break;
    }
//     std::cerr << std::endl;
//     std::cerr << "Game finished in move " << rb1->getRootPly() << " with " << result << std::endl;
//     rb1->currentBoard().print();
//     std::cerr << std::endl;
    return result;
}

uint64_t SelfGame::cpuTime()
{
    boost::chrono::duration<boost::chrono::process_cpu_clock::times, boost::nano> t = boost::chrono::process_cpu_clock::now() - start;
    start = boost::chrono::process_cpu_clock::now();
    return t.count().system + t.count().user;
}

template<Colors C>
bool SelfGame::checkResult(const RootBoard& rb)
{
    if (abs(rb.getScore()) >= decisiveScore) {
        nDecisive++;
        if (nDecisive >= decisiveScoreMoves || abs(rb.getScore()) >= infinity) {
//             if (abs(rb.getScore()) >= infinity) {
//                 std::cerr << "*****************************************" << std::endl;
//                 std::cerr << "*****************************************" << std::endl;
//                 std::cerr << "*****************************************" << std::endl;
//                 wrb->currentBoard().print();
//                 asm("int3\n");
//             }
            if (rb.getScore() >= decisiveScore)
                result = 1;
            else
                result = -1;
            return true;
        }
    } else
        nDecisive = 0;

    int v;
    if (rb.eval.draw<C>(rb.currentBoard<C>(), v) || rb.isDraw(rb.currentBoard<C>())) {
        result = 0;
        return true;
    }

    return false;
}

int SelfGame::tournament()
{
    int sum = 0;
    for (unsigned i=0; i<testPosition.size(); ++i) {
        wrb->setup(testPosition[i]);
        brb->setup(testPosition[i]);
        wrb->clearHash();
        brb->clearHash();
        sum += doGame(wrb, brb);
        wrb->setup(testPosition[i]);
        brb->setup(testPosition[i]);
        wrb->clearHash();
        brb->clearHash();
        sum -= doGame(brb, wrb);
    }
    return sum;
}

