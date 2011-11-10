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

#ifdef __x86_64__
#include <boost/chrono.hpp>
#endif
#include "selfgame.h"
#include "rootboard.tcc"

std::vector<std::string> testPosition = {
    "1r1q1rk1/ppp2ppp/2nbpn2/5b2/2QP4/2N1PN2/PP2BPPP/R1B2RK1 w - - pm Nb5; id Neutral.001;",
    "1rbq1rk1/5pp1/2nb1n1p/pp2p3/1PPp4/P2P1NP1/2N2PBP/1RBQ1RK1 w - - pm c5; id Neutral.002;",
    "1rbq1rk1/5ppp/2np4/p2Np1b1/2B1P3/2P5/RPN2PPP/3QK2R b K - pm Ne7; id Neutral.003;",
    "1rbq1rk1/5ppp/p1np1b2/1p1Np3/4P3/2P5/PPN2PPP/R2QKB1R w KQ - pm Bd3; id Neutral.004;",
    "1rq1k2r/3pbppp/p1p1p3/3nP3/8/1P2B3/P1P1BPPP/R2Q1RK1 b k - pm Nxe3; id Neutral.005;",
    "2kr3r/ppqn1pp1/2pbp2p/7P/3PQ3/5NP1/PPPB1P2/2KR3R w - - 1 16 ",
    "2r1k2r/1bqnbppp/pp1ppn2/8/2PNPP2/2N1B1P1/PP4BP/R2QR1K1 w k - pm Rc1; id Neutral.006;",
    "2r2rk1/1pqnbppp/p2pbn2/P3p3/4P3/1NN1B3/1PPQBPPP/R2R2K1 b - - pm Rfd8; id Neutral.007;",
    "2r2rk1/1pqnbppp/p2pbn2/P3p3/4P3/1NN1BB2/1PPQ1PPP/R4RK1 b - - pm Rfd8; id Neutral.008;",
    "2rq1rk1/1p1nbppp/p2pbn2/4p3/P3P3/1NN1B3/1PPQBPPP/R4RK1 w - - pm a5; id Neutral.009;",
    "2rq1rk1/p2nb1pp/bpp1p3/3p1p2/2PP4/1PB1P1P1/P2N1PBP/R2QR1K1 b - - pm Nf6; id Neutral.010;",
    "2rq1rk1/p2nb1pp/bpp1p3/3p1p2/2PP4/1PB3P1/P2NPPBP/R2QR1K1 w - - pm a4; id Neutral.011;",
    "2rq1rk1/p2nbppp/bpp1p3/3p4/2PPP3/1PB3P1/P2N1PBP/R2Q1RK1 b - e3 0 13 ",
    "2rq1rk1/p2nbppp/bpp1pn2/3p4/2PP1B2/1PN2NP1/P1Q1PPBP/R2R2K1 b - - pm Nh5; id Neutral.012;",
    "2rq1rk1/pb1n1pp1/1p1ppn1p/2p5/2PP3B/P1QBPP2/1P2N1PP/R4RK1 b - - pm cxd4; id Neutral.013;",
    "2rqr1k1/p2nbppp/b1p1p3/8/P1pPP3/2B3P1/2QN1PBP/3RR1K1 b - - pm Bf8; id Neutral.014;",
    "r1b1k2r/1pq3pp/p1nbpn2/3p4/3P4/2NB1N2/PP3PPP/R1BQ1RK1 w kq - 0 13 ",
    "r1b1k2r/1pqp1ppp/p1n1pn2/2b5/3NP3/2N3P1/PPP2PBP/R1BQ1RK1 w kq - pm Nxc6; id Neutral.015;",
    "r1b1k2r/1pqpbppp/p1n1pn2/8/3NP3/2N3P1/PPP2PBP/R1BQR1K1 b kq - pm O-O; id Neutral.016;",
    "r1b1k2r/2qp1ppp/p3pn2/1pb3B1/4P3/2NQ4/PPP1BPPP/R4R1K b kq - pm Bb7; id Neutral.018;",
    "r1b1k2r/4b1pp/p1pppn2/6B1/4P3/q1N5/P1PQB1PP/1R3RK1 b kq - pm O-O; id Neutral.019;",
    "r1b1k2r/pp1n1ppp/5n2/q2p1BB1/1bpP4/2N1PN2/PPQ2PPP/R3K2R b KQkq - pm O-O; id Neutral.020;",
    "r1b1k2r/pp2bppp/1qnppn2/6B1/2B1P3/1NN5/PPP2PPP/R2Q1RK1 b kq - pm O-O; id Neutral.021;",
    "r1b1k2r/pp3ppp/1qnppn2/8/1bP1PP2/1NN3P1/PP2Q2P/R1B1KB1R w KQkq - pm Be3; id Neutral.022;",
    "r1b1k2r/ppp1nppp/3p2q1/2b1n3/3NP3/2P1B3/PP2BPPP/RN1Q1RK1 w kq - pm f3; id Neutral.023;",
    "r1b1k2r/ppp1nppp/5q2/2bpn3/3NP3/2P1B3/PP2BPPP/RN1Q1RK1 b kq - pm O-O; id Neutral.024;",
    "r1b1kb1r/1p3ppp/p1nppn2/5PB1/3NP3/q1N5/P1PQ2PP/1R2KB1R w Kkq - pm fxe6; id Neutral.025;",
    "r1b1kb1r/1pp2ppp/p1p2n2/8/3NP3/5P2/PPP3PP/RNB1K2R b KQkq - pm Bd7; id Neutral.026;",
    "r1b1kb1r/1pp2ppp/p1p2n2/8/3NP3/8/PPP2PPP/RNB1K2R w KQkq - pm f3; id Neutral.027;",
    "r1b1kb1r/1pqp1ppp/p1n1pn2/8/3NP3/2N3P1/PPP2PBP/R1BQ1RK1 b kq - pm Be7; id Neutral.028;",
    "r1b1kb1r/1pqp1ppp/p1n1pn2/8/3NP3/2NBB3/PPP2PPP/R2QK2R w KQkq - pm O-O; id Neutral.029;",
    "r1b1kb1r/1pqp1ppp/p3pn2/4n3/3NP3/2N1B3/PPP1BPPP/R2Q1RK1 b kq - pm Bb4; id Neutral.030;",
    "r1b1kb1r/6pp/p1pppn2/6B1/4P3/q1N5/P1PQ2PP/1R2KB1R w Kkq - pm Be2; id Neutral.031;",
    "r1b1kb1r/pp1p1ppp/1qn1pn2/8/2P5/1NN5/PP2PPPP/R1BQKB1R w KQkq - pm a3; id Neutral.032;",
    "r1b1kb1r/pp1p1ppp/1qn1pn2/8/2PN4/2N3P1/PP2PP1P/R1BQKB1R w KQkq - pm Nb3; id Neutral.033;",
    "r1b1kb1r/pp3ppp/1qn1p3/3pPn2/3P4/2N2N2/PP2BPPP/R1BQK2R w KQkq - pm Na4; id Neutral.034;",
    "r1b1kb1r/pp3ppp/2n1p3/q2pPn2/3P4/2N2N2/PP2BPPP/R1BQK2R b KQkq - pm Bb4; id Neutral.035;",
    "r1b1kb1r/pp3ppp/2n1p3/q2pPn2/N2P4/5N2/PP2BPPP/R1BQK2R w KQkq - pm Nc3; id Neutral.036;",
    "r1b1kbnr/pp1ppppp/1qn5/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1b1kbnr/pp3ppp/1qn1p3/2ppP3/3P4/2P2N2/PP2BPPP/RNBQK2R b KQkq - pm cxd4; id Neutral.037;",
    "r1b1kbnr/pp3ppp/1qn1p3/3pP3/3P4/5N2/PP2BPPP/RNBQK2R b KQkq - pm Nge7; id Neutral.038;",
    "r1b1qrk1/ppp1p1bp/2np1np1/5p2/2NP4/1P3NP1/PBP1PPBP/R2Q1RK1 b - - pm e6; id Neutral.039;",
    "r1b1qrk1/ppp1p1bp/n2p1np1/3P1p2/2P5/2N2NP1/PP2PPBP/1RBQ1RK1 b - - 2 9 ",
    "r1b1r1k1/1pq1bppp/p1nppn2/8/P2NPP2/2N1BB2/1PP3PP/R2Q1R1K b - - pm Rb8; id Neutral.040;",
    "r1b1r1k1/1pq1bppp/p2ppn2/n7/P2NPP2/2N1BB2/1PP3PP/R2Q1R1K w - - pm Bf2; id Neutral.041;",
    "r1b1r1k1/pp3pbp/2pp1np1/q3n3/2PNP3/2N3PP/PP3P2/R1BQRBK1 w - - pm Be3; id Neutral.042;",
    "r1b1r1k1/ppq2ppp/2n2n2/3p4/P1pP4/B1P1PN2/2B2PPP/R2Q1RK1 b - - pm Ne4; id Neutral.043;",
    "r1b2rk1/1pq1bppp/p1nppn2/8/3NP3/1BN1B3/PPP1QPPP/2KR3R w - - 6 11",
    "r1b2rk1/1pq1bppp/p1nppn2/8/P3PP2/1NN1B3/1PP1B1PP/R2Q1RK1 b - - pm b6; id Neutral.044;",
    "r1b2rk1/1pq2ppp/p1p1pn2/8/1b2P3/2NQB3/PPP1BPPP/R4RK1 w - - pm Rad1; id Neutral.045;",
    "r1b2rk1/1pqpbppp/p1n1pn2/8/3NP3/2N1B1P1/PPP2PBP/R2Q1RK1 w - - pm f4; id Neutral.046;",
    "r1b2rk1/2q1bppp/p2p1n2/np2p3/3PP3/5N1P/PPBN1PP1/R1BQR1K1 b - - pm Bb7; id Neutral.047;",
    "r1b2rk1/2q1bppp/p2p1n2/npp1p3/3PP3/2P2N1P/PPBN1PP1/R1BQR1K1 b - - 2 12 ",
    "r1b2rk1/pp1pppbp/2n2np1/1NqB4/2P5/2N3P1/PP2PP1P/R1BQ1RK1 b - - pm Qb6; id Neutral.048;",
    "r1b2rk1/pp2bppp/2n1pn2/q2p4/2P2B2/P1N1PN2/1PQ2PPP/1K1R1B1R b - - pm a6; id Neutral.049;",
    "r1b2rk1/pp2qppp/5n2/2pp4/2P4Q/P3P3/1P3PPP/R1B1KB1R w KQ - pm cxd5; id Neutral.050;",
    "r1b2rk1/pp3ppp/2n1pn2/q1bp4/2P2B2/P1N1PN2/1PQ2PPP/2KR1B1R b - - 2 10 ",
    "r1b2rk1/pp4pp/1qnbpn2/3p4/3P4/2NB1N2/PP3PPP/R1BQ1RK1 w - - pm Nb5; id Neutral.051;",
    "r1b2rk1/ppp1qppp/2np1n2/3Pp3/2P1P3/5NP1/PP1N1PBP/R2Q1RK1 b - - pm Nb8; id Neutral.052;",
    "r1b2rk1/ppp1qppp/2nppn2/8/2PP4/5NP1/PP1NPPBP/R2Q1RK1 w - - pm e4; id Neutral.053;",
    "r1b2rk1/ppp1qppp/5n2/3p4/2PQ4/P3P3/1P3PPP/R1B1KB1R w KQ - pm Bd2; id Neutral.054;",
    "r1b2rk1/ppq1bppp/2n1p3/4P3/2B5/2P2N2/P3QPPP/R1B2RK1 w - - pm Bd3; id Neutral.055;",
    "r1b2rk1/ppq2ppp/2n2n2/2p1p3/3P4/P1PBPN2/2Q2PPP/R1B2RK1 b - - pm Re8; id Neutral.056;",
    "r1bq1rk1/1p2bppp/p1n1pn2/8/P1BP4/2N2N2/1P2QPPP/R1BR2K1 b - - 2 11 ",
    "r1bq1rk1/1p2bppp/p1nppn2/8/2P1P3/N1N5/PP2BPPP/R1BQ1RK1 b - - pm b6; id Neutral.057;",
    "r1bq1rk1/1p2bppp/p2p1n2/2n5/P3PB2/2NB1N2/1PP3PP/R2Q1R1K b - - pm Bg4; id Neutral.058;",
    "r1bq1rk1/1p2ppbp/p2p1np1/n1pP2B1/2P5/2NQ1NP1/PP2PPBP/R4RK1 b - - pm Bd7; id Neutral.059;",
    "r1bq1rk1/1pp1ppbp/p1np1np1/3P4/2P5/2N2NP1/PP2PPBP/R1BQ1RK1 b - - pm Na5; id Neutral.060;",
    "r1bq1rk1/1pp2pbp/2n2np1/p2pp3/4P3/2PP1NP1/PP1N1PBP/R1BQ1RK1 w - - pm exd5; id Neutral.061;",
    "r1bq1rk1/1pp2pbp/2n3p1/p2np3/8/2PP1NP1/PP1N1PBP/R1BQ1RK1 w - - pm Re1; id Neutral.062;",
    "r1bq1rk1/1pp2pbp/2n3p1/p2np3/8/2PP1NP1/PP1N1PBP/R1BQR1K1 b - - pm Re8; id Neutral.063;",
    "r1bq1rk1/2p1bppp/p1np1n2/1p2p3/3PP3/1BP2N2/PP3PPP/RNBQR1K1 b - - pm Bg4; id Neutral.064;",
    "r1bq1rk1/2p1bppp/p2p1n2/np2p3/4P3/1BP2N1P/PP1P1PP1/RNBQR1K1 w - - pm Bc2; id Neutral.065;",
    "r1bq1rk1/2p1bppp/p2p1n2/np2p3/4P3/2PP1N2/PPB2PPP/RNBQR1K1 b - - pm c5; id Neutral.066;",
    "r1bq1rk1/4bpp1/p1p1pn1p/3p4/4PB2/2N1Q3/PPP1BPPP/2KR3R w - - pm Qg3; id Neutral.067;",
    "r1bq1rk1/4bppp/p1pp1n2/np1Pp3/4P3/2P2N1P/PPB2PP1/RNBQR1K1 w - - pm dxc6; id Neutral.068;",
    "r1bq1rk1/4bppp/p2p1n2/npp1p3/3PP3/2P2N1P/PPB2PP1/RNBQR1K1 b - - pm Qc7; id Neutral.069;",
    "r1bq1rk1/4bppp/p2p1n2/npp1p3/4P3/2PP1N2/PPB2PPP/RNBQR1K1 w - - pm Nbd2; id Neutral.070;",
    "r1bq1rk1/5ppp/2np4/p2Np1b1/R3P3/2P5/1PN2PPP/3QKB1R w K - pm Bc4; id Neutral.071;",
    "r1bq1rk1/5ppp/p1np1b2/1p1Np3/4P3/N1P5/PP3PPP/R2QKB1R w KQ - pm Nc2; id Neutral.072;",
    "r1bq1rk1/5ppp/p1np4/1p1Np1b1/P3P3/2P5/1PN2PPP/R2QKB1R b KQ - pm bxa4; id Neutral.073;",
    "r1bq1rk1/bpp2pp1/p1np1n1p/4p3/1PP5/P1NP1NP1/1B2PPBP/R2Q1RK1 b - - pm Be6; id Neutral.074;",
    "r1bq1rk1/bpp2pp1/p1np1n1p/4p3/1PP5/P1NP1NP1/4PPBP/R1BQ1RK1 w - - pm Bb2; id Neutral.075;",
    "r1bq1rk1/bpp2pp1/p1np1n1p/4p3/4P3/1BPP1N1P/PP1N1PP1/R1BQR1K1 b - - pm Nh5; id Neutral.076;",
    "r1bq1rk1/bpp2ppp/p1np1n2/4p3/4P3/1BPP1N1P/PP1N1PP1/R1BQ1RK1 b - - pm h6; id Neutral.077;",
    "r1bq1rk1/bpp2ppp/p1np1n2/4p3/4P3/1BPP1N2/PP1N1PPP/R1BQ1RK1 w - - pm Nc4; id Neutral.078;",
    "r1bq1rk1/p1p2ppp/5n2/3p2B1/1b6/2NB4/PPP2PPP/R2Q1RK1 b - - pm c6; id Neutral.079;",
    "r1bq1rk1/p4ppp/2p2n2/3p2B1/1b6/2NB4/PPP2PPP/R2Q1RK1 w - - pm Qf3; id Neutral.080;",
    "r1bq1rk1/pp1pbppp/2n1pn2/8/2PNP3/P1N5/1P3PPP/R1BQKB1R w KQ - pm Nf3; id Neutral.081;",
    "r1bq1rk1/pp1pppbp/2n2np1/8/2BNP3/2N1B3/PPP2PPP/R2QK2R w KQ - pm Bb3; id Neutral.082;",
    "r1bq1rk1/pp1pppbp/2n2np1/8/3NP3/1BN1B3/PPP2PPP/R2QK2R b KQ - pm d6; id Neutral.083;",
    "r1bq1rk1/pp2bpp1/2np1n1p/4p3/4P3/2N2N2/PPP1BPPP/R1BQR1K1 w - - pm h3; id Neutral.084;",
    "r1bq1rk1/pp2bppp/2n2n2/4p1B1/2Pp4/PNN1P3/1PQ2PPP/3RKB1R w K - pm Be2; id Neutral.085;",
    "r1bq1rk1/pp2bppp/2np1n2/2p1p1N1/2B1PP2/2NP4/PPP3PP/R1BQ1RK1 b - - pm Bg4; id Neutral.086;",
    "r1bq1rk1/pp2bppp/2nppn2/8/3NP3/1BN1B3/PPP2PPP/R2Q1RK1 b - - pm a6; id Neutral.087;",
    "r1bq1rk1/pp2npbp/2n1p1p1/3P4/3N4/2N1P1P1/PP3PBP/R1BQ1RK1 b - - pm Nxd5; id Neutral.088;",
    "r1bq1rk1/pp2npbp/3pp1p1/2p5/3nPP2/2NPBNP1/PPP3BP/R2Q1RK1 w - - pm e5; id Neutral.089;",
    "r1bq1rk1/pp2nppp/2n1p3/2ppP2Q/3P4/P1PB4/2P2PPP/R1B1K1NR b KQ - pm Ng6; id Neutral.090;",
    "r1bq1rk1/pp2ppbp/2n2np1/2p5/3P4/N1P1PNP1/P3QPBP/R1B2RK1 b - - pm Bf5; id Neutral.091;",
    "r1bq1rk1/pp2ppbp/2n3p1/1B1pP3/3P4/2P2N2/P4PPP/R1BQR1K1 b - - pm Na5; id Neutral.092;",
    "r1bq1rk1/pp2ppbp/2n3p1/2p5/3PP3/2P1BN2/P2Q1PPP/1R2KB1R b K - pm cxd4; id Neutral.093;",
    "r1bq1rk1/pp2ppbp/2n3p1/2p5/3PP3/2P1BN2/P4PPP/1R1QKB1R w K - pm Qd2; id Neutral.094;",
    "r1bq1rk1/pp2ppbp/2np1np1/2p5/2P5/P1NP1NP1/1P2PPBP/R1BQ1RK1 b - - pm a6; id Neutral.095;",
    "r1bq1rk1/ppp1bppp/1nn5/4p3/8/2NP1NP1/PP2PPBP/R1BQ1RK1 w - - pm Be3; id Neutral.096;",
    "r1bq1rk1/ppp1bppp/1nn5/4p3/8/2NPBNP1/PP2PPBP/R2Q1RK1 b - - pm Be6; id Neutral.097;",
    "r1bq1rk1/ppp1bppp/2np1n2/4p3/4P3/1BPP1N2/PP3PPP/RNBQ1RK1 b - - pm Na5; id Neutral.098;",
    "r1bq1rk1/ppp1npbp/2np2p1/4p3/2P5/2NPP1P1/PP2NPBP/R1BQ1RK1 b - - 0 8 ",
    "r1bq1rk1/ppp1ppbp/1nn3p1/3P4/8/2N2NP1/PP2PPBP/R1BQ1RK1 b - - pm Na5; id Neutral.099;",
    "r1bq1rk1/ppp1ppbp/2np1np1/8/2P5/2NP1NP1/PP2PPBP/R1BQ1RK1 b - - pm e5; id Neutral.100;",
    "r1bq1rk1/ppp2pp1/2np1n1p/2b1p3/2P5/2NP1NP1/PP2PPBP/R1BQ1RK1 w - - pm a3; id Neutral.101;",
    "r1bq1rk1/ppp2ppp/2np1n2/2b1p3/2P5/2NP1NP1/PP2PPBP/R1BQ1RK1 b - - pm h6; id Neutral.102;",
    "r1bq1rk1/ppp2ppp/2np1n2/4p3/1bP5/2N2NP1/PP1PPPBP/R1BQ1RK1 w - - pm d3; id Neutral.103;",
    "r1bq1rk1/pppn1ppp/4pb2/8/3PN3/5N2/PPPQ1PPP/R3KB1R w KQ - pm O-O-O; id Neutral.104;",
    "r1bq1rk1/pppp1pbp/2n2np1/4p3/2P5/P1N2NP1/1P1PPPBP/R1BQ1RK1 b - - pm d6; id Neutral.105;",
    "r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2P5/2NP1NP1/PP2PPBP/R1BQK2R b KQ - pm d6; id Neutral.106;",
    "r1bq2k1/pp2nrpp/2n1p3/2pp2B1/3P2Q1/P1PB1N2/2P2PPP/R3K2R w KQ - pm Bxe7; id Neutral.107;",
    "r1bqk1nr/1pp2pbp/2np2p1/p3p3/2P5/P1NP2P1/1P2PPBP/1RBQK1NR b Kkq - pm Nf6; id Neutral.108;",
    "r1bqk1nr/1pppbppp/p1n5/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 2;",
    "r1bqk1nr/bp3ppp/p1npp3/8/4P3/1NNBB3/PPP1QPPP/R3K2R b KQkq - pm Nf6; id Neutral.109;",
    "r1bqk1nr/pp1ppp1p/2n3pb/2p5/2P1P3/2N4P/PP1P1PP1/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqk1nr/pp1pppbp/2n3p1/1Bp5/4P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - fmvn 5; hmvc 2;",
    "r1bqk1nr/pp1pppbp/2n3p1/2p5/2P1P3/2N3P1/PP1P1P1P/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqk1nr/pp1pppbp/2n3p1/2p5/2P1P3/2N4P/PP1P1PP1/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqk1nr/pp1pppbp/2n3p1/2p5/4P3/2NP2P1/PPP2PBP/R1BQK1NR b KQkq - pm d6; id Neutral.110;",
    "r1bqk1nr/pp1pppbp/2n3p1/2p5/4P3/3P2P1/PPP2PBP/RNBQK1NR w KQkq - fmvn 5; hmvc 2;",
    "r1bqk1nr/pp1pppbp/2n3p1/2p5/4P3/3P2P1/PPP2PBP/RNBQK1NR w KQkq - pm Nc3; id Neutral.111;",
    "r1bqk1nr/pp1pppbp/2n3p1/2p5/4P3/5NP1/PPPP1PBP/RNBQK2R w KQkq - pm O-O; id Neutral.112;",
    "r1bqk1nr/pp1pppbp/2n3p1/2p5/4PP2/2N2N2/PPPP2PP/R1BQKB1R w KQkq - pm Bb5; id Neutral.113;",
    "r1bqk1nr/pp2bppp/2np4/1N2p3/2P1P3/8/PP3PPP/RNBQKB1R w KQkq - pm N1c3; id Neutral.114;",
    "r1bqk1nr/pp3pbp/2n1p1p1/2pp4/4P3/3P1NP1/PPPN1PBP/R1BQ1RK1 b kq - pm Nge7; id Neutral.115;",
    "r1bqk1nr/ppp2ppp/2n1p3/3p4/1b1PP3/2N1B3/PPP2PPP/R2QKBNR w KQkq - fmvn 5; hmvc 4;",
    "r1bqk1nr/pppp1pbp/2n3p1/4p3/2P1P3/2N3P1/PP1P1PBP/R1BQK1NR b KQkq - pm d6; id Neutral.116;",
    "r1bqk1nr/pppp1pbp/2n3p1/4p3/2P5/2N3P1/PP1PPPBP/R1BQK1NR w KQkq - fmvn 5; hmvc 2;",
    "r1bqk1nr/pppp1pbp/2n3p1/4p3/2P5/2NP2P1/PP2PPBP/R1BQK1NR b KQkq - pm d6; id Neutral.117;",
    "r1bqk1nr/pppp1ppp/2n5/2b5/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqk2r/1p2bppp/p1nppn2/8/2BNP3/2N1B3/PPP1QPPP/2KR3R b kq - pm Qc7; id Neutral.118;",
    "r1bqk2r/1p2bppp/p1nppn2/8/3NP3/1BN1B3/PPP2PPP/R2Q1RK1 b kq - pm O-O; id Neutral.119;",
    "r1bqk2r/1p2bppp/p1nppn2/8/P2NP3/2N1B3/1PP1BPPP/R2QK2R w KQkq - pm O-O; id Neutral.120;",
    "r1bqk2r/2p1bppp/p1np1n2/3Np3/Pp2P3/1B3N2/1PPP1PPP/R1BQ1RK1 b kq - pm Bg4; id Neutral.121;",
    "r1bqk2r/4bpp1/p2ppn1p/1p6/3BPP2/2N5/PPPQ2PP/2KR1B1R w kq b6 0 12 ",
    "r1bqk2r/5pp1/p1p1pn1p/3p4/1b2PB2/P1N1Q3/1PP2PPP/2KR1B1R b kq - pm Ba5; id Neutral.122;",
    "r1bqk2r/5ppp/p1n1p3/1pnpP3/5P2/2N2N2/PPPQ2PP/R3KB1R w KQkq - pm Qf2; id Neutral.123;",
    "r1bqk2r/5ppp/p1np1b2/1p1Np3/2P1P3/N7/PP3PPP/R2QKB1R b KQkq - pm b4; id Neutral.124;",
    "r1bqk2r/p1p2ppp/2p2n2/3P4/1b6/2NB4/PPP2PPP/R1BQK2R b KQkq - pm cxd5; id Neutral.125;",
    "r1bqk2r/p2p1ppp/2p1pn2/8/1bP1P3/2N5/PP3PPP/R1BQKB1R w KQkq - pm Bd3; id Neutral.126;",
    "r1bqk2r/pp1n1pp1/2pbpn1p/3p4/2PP2P1/2N1PN2/PPQ2P1P/R1B1KB1R w KQkq - pm Bd2; id Neutral.127;",
    "r1bqk2r/pp1nbppp/2n1p3/2ppP3/3P4/2PB1N2/PP1N1PPP/R1BQ1RK1 b kq - pm a5; id Neutral.128;",
    "r1bqk2r/pp1p1ppp/2N1pn2/8/1bP1P3/2N5/PP3PPP/R1BQKB1R b KQkq - pm bxc6; id Neutral.129;",
    "r1bqk2r/pp1pbppp/2n1pn2/8/2PN4/P1N5/1P2PPPP/R1BQKB1R w KQkq - pm e4; id Neutral.130;",
    "r1bqk2r/pp2bpp1/2np1n1p/4p3/2B1P3/2N2N2/PPP2PPP/R1BQ1RK1 b kq - pm O-O; id Neutral.131;",
    "r1bqk2r/pp2bppp/2n1pn2/3p4/2PP4/P1N2N2/1P3PPP/R1BQKB1R w KQkq - pm Bd3; id Neutral.132;",
    "r1bqk2r/pp2bppp/2nppn2/8/2BNP3/2N1B3/PPP1QPPP/R3K2R b KQkq - pm a6; id Neutral.133;",
    "r1bqk2r/pp2bppp/2nppn2/8/3NP3/2N5/PPP1BPPP/R1BQ1R1K b kq - pm O-O; id Neutral.134;",
    "r1bqk2r/pp2nppp/2n1p3/2ppP3/3P4/P1P2N2/2P2PPP/R1BQKB1R w KQkq - pm Be2; id Neutral.135;",
    "r1bqk2r/pp2ppbp/2np1np1/8/2P1P3/2N5/PPN1BPPP/R1BQK2R b KQkq - pm Nd7; id Neutral.136;",
    "r1bqk2r/pp3ppp/2nbpn2/2pp4/3P4/2P1PNB1/PP1N1PPP/R2QKB1R b KQkq - pm O-O; id Neutral.137;",
    "r1bqk2r/pp4pp/2nbpn2/3p4/3P4/3B1N2/PP2NPPP/R1BQ1RK1 b kq - pm O-O; id Neutral.138;",
    "r1bqk2r/pp4pp/2nbpn2/3p4/3P4/3B1N2/PP2NPPP/R1BQK2R w KQkq - pm O-O; id Neutral.139;",
    "r1bqk2r/ppp1npbp/2np2p1/4p3/2P1P3/2NP2P1/PP2NPBP/R1BQK2R b KQkq - pm O-O; id Neutral.140;",
    "r1bqk2r/ppp2pbp/2np1np1/4p3/2P5/2NP1NP1/PP2PPBP/R1BQK2R w KQkq - pm O-O; id Neutral.141;",
    "r1bqk2r/ppp2ppp/2n1p3/8/QnpP4/5NP1/PP2PPBP/RN3RK1 b kq - pm Rb8; id Neutral.142;",
    "r1bqk2r/pppp1ppp/2n2n2/1Bb1p3/4P3/P4N2/1PPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 3;",
    "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/2P2N2/PP1P1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 1;",
    "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2P5/2N2NP1/PP1PPP1P/R1BQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqk2r/pppp1ppp/2n2n2/4p3/1b2P3/2N2N1P/PPPP1PP1/R1BQKB1R w KQkq - fmvn 5; hmvc 3;",
    "r1bqk2r/pppp1ppp/2n2n2/4p3/1bB1P3/2NP4/PPP2PPP/R1BQK1NR w KQkq - fmvn 5; hmvc 1;",
    "r1bqk2r/pppp1ppp/2n2n2/4p3/1bP5/2N1PN2/PP1P1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/1p1n1ppp/p2ppn2/6B1/3NPP2/2N5/PPP3PP/R2QKB1R w KQkq - pm Qf3; id Neutral.143;",
    "r1bqkb1r/1p1p1ppp/p1n1p3/8/3NP3/2N5/PPP2PPP/R1BQ1RK1 b kq - pm Qc7; id Neutral.144;",
    "r1bqkb1r/1pp2ppp/p1p2n2/4p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - pm d3; id Neutral.145;",
    "r1bqkb1r/1ppp1ppp/p1n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 2;",
    "r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 2;",
    "r1bqkb1r/3n1p1p/4pp2/1p6/3N4/3B4/PP3PPP/R1BQK2R b KQkq - pm Qb6; id Neutral.146;",
    "r1bqkb1r/3n1pp1/p2ppn1p/1p6/3NP1P1/2N1BP2/PPPQ3P/2KR1B1R b kq - pm Bb7; id Neutral.147;",
    "r1bqkb1r/3n1pp1/p2ppn1p/1p6/3NP1P1/2N1BP2/PPPQ3P/R3KB1R w KQkq - pm O-O-O; id Neutral.148;",
    "r1bqkb1r/3n1ppp/p1n1p3/1pppP3/3P1P2/P1N1BN2/1PPQ2PP/R3KB1R b KQkq - pm Bb7; id Neutral.149;",
    "r1bqkb1r/3n1ppp/p2ppn2/1p6/3NP1P1/2N1BP2/PPPQ3P/R3KB1R b KQkq - pm h6; id Neutral.150;",
    "r1bqkb1r/3n1ppp/p2ppn2/1p6/3NP3/2N1BP2/PPPQ2PP/R3KB1R w KQkq - pm g4; id Neutral.151;",
    "r1bqkb1r/3n1ppp/p3pn2/2p5/Pp1P4/2NBPN2/1P3PPP/R1BQ1RK1 w kq - pm Ne4; id Neutral.152;",
    "r1bqkb1r/3n1ppp/p3pn2/2p5/Pp1PN3/3BPN2/1P3PPP/R1BQK2R w KQkq - pm Nxf6+; id Neutral.153;",
    "r1bqkb1r/5p1p/p1np4/1p1NpP2/8/N7/PPP2PPP/R2QKB1R b KQkq - pm Bxf5; id Neutral.154;",
    "r1bqkb1r/5pp1/p1p1pn1p/3p4/4PB2/2N1Q3/PPP2PPP/2KR1B1R b kq - pm Bb4; id Neutral.155;",
    "r1bqkb1r/5pp1/p1p1pn1p/3p4/4PB2/2N5/PPPQ1PPP/1K1R1B1R b kq - pm Bb4; id Neutral.156;",
    "r1bqkb1r/p2p1ppp/1p2pn2/8/2PQ4/P1N5/1P2PPPP/R1B1KB1R w KQkq - pm Qf4; id Neutral.157;",
    "r1bqkb1r/p2ppppp/n4n2/1ppP4/8/3Q1N2/PPP1PPPP/RNB1KB1R w KQkq - fmvn 5; hmvc 2;",
    "r1bqkb1r/pp1npppp/2p2n2/3p2B1/3P4/2N2N2/PPP1PPPP/R2QKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/2P1P3/2N2N2/PP1P1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 3;",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/2P5/2N2NP1/PP1PPP1P/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/2P5/5NP1/PP1PPPBP/RNBQK2R w KQkq - pm O-O; id Neutral.158;",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/3P1P2/4PN2/PPP3PP/RNBQKB1R w KQkq - fmvn 5; hmvc 3;",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/3P4/5NP1/PPP1PPBP/RNBQ1RK1 b kq - pm cxd4; id Neutral.159;",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/4P3/2N2N2/PPPPBPPP/R1BQK2R w KQkq - fmvn 5; hmvc 4;",
    "r1bqkb1r/pp1p1ppp/2n1pn2/8/2PN4/6P1/PP2PPBP/RNBQK2R b KQkq - pm Qb6; id Neutral.160;",
    "r1bqkb1r/pp1pnppp/2n1p3/2p5/4P3/3P1N2/PPP1QPPP/RNB1KB1R w KQkq - fmvn 5; hmvc 4;",
    "r1bqkb1r/pp1pnppp/2n1p3/2p5/4P3/3P1NP1/PPP2P1P/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pp1ppp1p/2n2np1/2p5/2P5/2N2NP1/PP1PPP1P/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pp1ppp1p/2n2np1/2p5/2P5/5NP1/PP1PPPBP/RNBQK2R w KQkq - pm Nc3; id Neutral.161;",
    "r1bqkb1r/pp1ppp1p/2n2np1/2p5/4P3/3B1N2/PPPP1PPP/RNBQ1RK1 w kq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pp1ppppp/2n2n2/8/2PN4/8/PP2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pp1ppppp/2n2n2/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pp2pppp/2n2n2/2pp4/2P5/2N2NP1/PP1PPP1P/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pp2pppp/2n2n2/2pp4/3P1B1P/4P3/PPP2PP1/RN1QKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pp2pppp/2n5/3n4/3P4/2N2N2/PP3PPP/R1BQKB1R b KQkq - pm Bg4; id Neutral.162;",
    "r1bqkb1r/pp2pppp/2np1n2/1Bp5/4P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pp2pppp/2np1n2/2p5/2B1P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - fmvn 5; hmvc 4;",
    "r1bqkb1r/pp2pppp/2np1n2/2p5/2P1P3/2N2N2/PP1P1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 3;",
    "r1bqkb1r/pp2pppp/2np1n2/2p5/2P1P3/2N4P/PP1P1PP1/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pp2pppp/2np1n2/2p5/4P3/2NP2P1/PPP2P1P/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pp2pppp/2np1n2/2p5/4P3/2P2N2/PP1PBPPP/RNBQK2R w KQkq - fmvn 5; hmvc 3;",
    "r1bqkb1r/pp3pp1/1np1pn1p/8/3P4/3B1N2/PPP1QPPP/R1B1K1NR b KQkq - pm c5; id Neutral.163;",
    "r1bqkb1r/pp3ppp/2n1pn2/2pp4/3P1B2/2P1PN2/PP3PPP/RN1QKB1R w KQkq - pm Nbd2; id Neutral.164;",
    "r1bqkb1r/pp3ppp/2n2n2/2pp4/3P4/2N1PN2/PP3PPP/R1BQKB1R w KQkq - pm Be2; id Neutral.165;",
    "r1bqkb1r/pp3ppp/2np1n2/4p3/4P3/2N2N2/PPP1BPPP/R1BQK2R b KQkq - pm h6; id Neutral.166;",
    "r1bqkb1r/pp3ppp/2nppn2/8/3NP3/1BN5/PPP2PPP/R1BQK2R b KQkq - pm Be7; id Neutral.167;",
    "r1bqkb1r/ppp1pp1p/2n2np1/3p4/3P4/1P2PN2/P1P2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/ppp1pppp/2n2n2/3p4/3P4/5NP1/PPP1PPBP/RNBQK2R b KQkq - pm e6; id Neutral.168;",
    "r1bqkb1r/ppp1pppp/2n2n2/3p4/8/5NP1/PPPPPPBP/RNBQK2R w KQkq - pm d4; id Neutral.169;",
    "r1bqkb1r/ppp1pppp/2n5/3pP3/3Pn3/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/ppp2ppp/2n1pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "r1bqkb1r/ppp2ppp/2n1pn2/3p4/4P3/3P1N2/PPPN1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 4;",
    "r1bqkb1r/ppp2ppp/2n1pn2/8/2pP4/5NP1/PP2PPBP/RNBQ1RK1 b kq - pm Rb8; id Neutral.170;",
    "r1bqkb1r/ppp2ppp/2n2n2/3pp3/2P5/2N2NP1/PP1PPP1P/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/ppp2ppp/2n2n2/3pp3/3P4/2P1B2P/PP2PPP1/RN1QKBNR w KQkq - fmvn 5; hmvc 3;",
    "r1bqkb1r/ppp2ppp/2n2n2/3pp3/8/2PP2P1/PP2PPBP/RNBQK1NR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/ppp2ppp/2n5/3np3/8/2N2NP1/PP1PPPBP/R1BQK2R b KQkq - pm Nb6; id Neutral.171;",
    "r1bqkb1r/ppp2ppp/2n5/3np3/8/2NP1NP1/PP2PP1P/R1BQKB1R b KQkq - pm Be7; id Neutral.172;",
    "r1bqkb1r/ppp2ppp/2np1n2/1B2p3/4P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/ppp2ppp/2np1n2/1B2p3/8/1P2P3/PBPP1PPP/RN1QK1NR w KQkq - pm Ne2; id Neutral.173;",
    "r1bqkb1r/pppn1ppp/4pn2/3p2B1/3P4/2N2N2/PPP1PPPP/R2QKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pppn1ppp/4pn2/3p2B1/3PP3/2N5/PPP2PPP/R2QKBNR w KQkq - fmvn 5; hmvc 4;",
    "r1bqkb1r/pppn1ppp/4pn2/8/Q1pP4/5NP1/PP2PP1P/RNB1KB1R w KQkq - pm Bg2; id Neutral.174;",
    "r1bqkb1r/pppp1pp1/2n2n1p/4p3/2P5/P1NP4/1P2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/2P2N2/PP1P1PPP/RNBQKB1R w KQkq - pm d4; id Neutral.175;",
    "r1bqkb1r/pppp1ppp/2n2n2/8/2Pp4/2N1P3/PP3PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pppp1ppp/2n2n2/8/2Pp4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkb1r/pppp1ppp/2n2n2/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqkb1r/pppp1ppp/2n2n2/8/4P3/4Q3/PPP2PPP/RNB1KBNR w KQkq - fmvn 5; hmvc 3;",
    "r1bqkb1r/pppp1ppp/2n5/1B2p3/4n3/5N2/PPPP1PPP/RNBQ1RK1 w kq - fmvn 5; hmvc 0;",
    "r1bqkbnr/1p1p1ppp/p1n1p3/2p5/4P3/2N2N2/PPPPBPPP/R1BQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/1pp2ppp/p1np4/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/1pp2ppp/p1p5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/1ppp1pp1/p1n4p/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/2pn1ppp/p3p3/1p6/2Q5/5NP1/PP1PPPBP/RNB1K2R w KQkq - pm Qc2; id Neutral.176;",
    "r1bqkbnr/2pp1ppp/p1n5/1p2p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp1npppp/2p5/8/3PN3/8/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkbnr/pp1p1ppp/2n1p3/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqkbnr/pp1p1ppp/2n1p3/8/3pP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp1p1ppp/2n5/2p1p3/2P5/2N3P1/PP1PPPBP/R1BQK1NR b KQkq - pm Nf6; id Neutral.177;",
    "r1bqkbnr/pp1p1ppp/n3p3/2p5/3PP3/2P2N2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp1ppp1p/2n3p1/2p5/4PP2/2N5/PPPP2PP/R1BQKBNR w KQkq - pm Nf3; id Neutral.178;",
    "r1bqkbnr/pp1ppp1p/2n3p1/8/2PpP3/5N2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp1ppp1p/2n3p1/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp1ppp1p/2n3p1/8/3pP3/2N5/PPP1NPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp1ppppp/2n5/8/3pP3/5N2/PPP2PPP/RNBQKB1R w KQkq - pm Nxd4; id Neutral.179;",
    "r1bqkbnr/pp2pp1p/2np2p1/2p5/2P1P3/2N4P/PP1P1PP1/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pp1p/2np2p1/2p5/4P3/2N3P1/PPPP1PBP/R1BQK1NR w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pp1p/2np2p1/2p5/4P3/2N3PP/PPPP1P2/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pp1p/2np2p1/2p5/4P3/5NP1/PPPP1PBP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pp1p/2np2p1/2p5/4PP2/2NP4/PPP3PP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pp1p/2p3p1/2p5/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pppp/2n5/3p4/3P4/3B4/PPP2PPP/RNBQK1NR w KQkq - fmvn 5; hmvc 2;",
    "r1bqkbnr/pp2pppp/2np4/8/2PpP3/5N2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pppp/2np4/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pppp/2np4/8/3pP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp2pppp/2np4/8/3QP3/5N2/PPP2PPP/RNB1KB1R w KQkq - fmvn 5; hmvc 1;",
    "r1bqkbnr/pp3ppp/2n1p3/2pp4/4P3/2PB1N2/PP1P1PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pp3ppp/2n1p3/2ppP3/3P4/2P5/PP3PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqkbnr/pp3ppp/2n5/2pp4/3P4/4PN2/PP3PPP/RNBQKB1R w KQkq - pm Be2; id Neutral.180;",
    "r1bqkbnr/pp3ppp/2np4/2p1p3/2B1P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/ppp2ppp/8/3pn3/8/2P5/PP2PPPP/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "r1bqkbnr/pppn1ppp/4p3/8/3PN3/8/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "r1bqnrk1/ppp1npbp/3p2p1/3Pp3/2P1P3/2N1B3/PP2BPPP/R2QNRK1 b - - 4 10 ",
    "r1bqr1k1/ppp1bppp/1nn5/4p3/8/P1NP1NP1/1P2PPBP/R1BQ1RK1 w - - pm b4; id Neutral.181;",
    "r1bqr1k1/ppp2pp1/2np1n1p/3N4/2P1p3/2P3P1/PP2PPBP/R1BQ1RK1 b - - pm Bf5; id Neutral.182;",
    "r1bqr1k1/ppp2pp1/2np1n1p/8/2P1N3/2PP2P1/P4PBP/R1BQ1RK1 w - - pm Bf4; id Neutral.183;",
    "r1bqr1k1/ppp3pp/2n2n2/5p2/2P1N3/3PpPP1/P3P1BP/R1BQ1RK1 w - - pm Nc3; id Neutral.184;",
    "r1bqr1k1/ppp3pp/2n5/3n1p2/2P1N3/3PpPP1/P3P1BP/R1BQ1RK1 b - - pm Nf6; id Neutral.185;",
    "r1bqr1k1/pppn1pbp/3p1np1/4p3/2PPP3/2N2NPP/PP3PB1/R1BQ1RK1 b - - pm exd4; id Neutral.186;",
    "r1bqr1k1/pppp1ppp/8/3P4/1b1p4/6P1/PP1PPPBP/R1BQ1RK1 w - - pm e4; id Neutral.187;",
    "r1bqrnk1/pp2bppp/2p2n2/3p2B1/3P4/2NBPN2/PPQ2PPP/R4RK1 w - - 5 11 ",
    "r1r3k1/1b4pp/p3Nn2/1pb5/2p5/2N5/PPB2PPP/3RR1K1 w - - pm Nxc5; id Neutral.188;",
    "r2q1rk1/1bp1bppp/p1np1n2/1p2p3/3PP3/1BP2N1P/PP3PP1/RNBQR1K1 b - - pm Re8; id Neutral.189;",
    "r2q1rk1/1bp1bppp/p1np1n2/1p2p3/4P3/1BP2N1P/PP1P1PP1/RNBQR1K1 w - - pm d4; id Neutral.190;",
    "r2q1rk1/1p1nbppp/p2pbn2/4p3/4P3/1NN1B3/PPPQBPPP/R4RK1 w - - pm a4; id Neutral.191;",
    "r2q1rk1/1p2bppp/1nn1b3/4p3/p7/P1NP2P1/3NPPBP/1RBQ1RK1 w - - pm Nf3; id Neutral.192;",
    "r2q1rk1/2p1bppp/p1n1b3/1p1pP3/P7/1BP2N2/2P2PPP/R1BQ1RK1 w - - pm axb5; id Neutral.193;",
    "r2q1rk1/3nb1pp/p2p1p2/1p1Pp1Pn/8/1N2BP2/PPPQ3P/2KR1B1R w - - pm gxf6; id Neutral.194;",
    "r2q1rk1/4bppp/p2p1n2/npp5/3PP1b1/4BN2/PPB2PPP/RN1QR1K1 w - - pm h3; id Neutral.195;",
    "r2q1rk1/bpp2ppp/p1npbn2/4p3/2N1P3/1BPP1N2/PP3PPP/R1BQ1RK1 w - - pm Bg5; id Neutral.196;",
    "r2q1rk1/p3bppp/b1p1pn2/1p6/2PPp3/1PB3P1/P2N1PBP/R2QR1K1 w - - pm Qc2; id Neutral.197;",
    "r2q1rk1/pb1n1ppp/2pb1n2/1p2p3/3PP3/2NB1N2/PPQ2PPP/R1B2RK1 w - - pm Rd1; id Neutral.198;",
    "r2q1rk1/pb1n1ppp/2pbpn2/1p6/3P4/2NBPN2/PPQB1PPP/R4RK1 b - - pm b4; id Neutral.199;",
    "r2q1rk1/pb1nbppp/1pp1pn2/3p4/2PP1B2/5NP1/PPQ1PPBP/RN1R2K1 w - - pm Nc3; id Neutral.200;",
    "r2q1rk1/pb2ppbp/1pnp1np1/2p5/4P3/2PP1NP1/PP1N1PBP/R1BQR1K1 w - - pm a4; id Neutral.201;",
    "r2q1rk1/pbpn1pp1/1p2pn1p/3p4/2PP3B/P1Q1PP2/1P4PP/R3KBNR w KQ - 1 11 ",
    "r2q1rk1/pbpn1ppp/1p1ppn2/6B1/2PP4/P1Q1PP2/1P4PP/R3KBNR w KQ - pm Bd3; id Neutral.202;",
    "r2q1rk1/pp1n1ppp/2p1p3/3nP2b/Pb1P4/2NB1N2/1P1BQPPP/R4RK1 b - - pm c5; id Neutral.203;",
    "r2q1rk1/pp1n1ppp/2p1pnb1/8/Pb1PP3/2NB1N2/1P2QPPP/R1B2RK1 b - - pm Bh5; id Neutral.204;",
    "r2q1rk1/pp1n1ppp/2p1pnb1/8/PbBPP3/2N2N2/1P2QPPP/R1B2RK1 w - - 1 11 ",
    "r2q1rk1/pp2bpp1/2npbn1p/4p3/4P3/2N2N1P/PPP1BPP1/R1BQR1K1 w - - pm Bf1; id Neutral.205;",
    "r2q1rk1/pp2bppp/2np1n2/2p1p1N1/2B1PPb1/2NP4/PPP3PP/R1BQ1RK1 w - - pm Qe1; id Neutral.206;",
    "r2q1rk1/pp2ppbp/2npbnp1/8/4PP2/1NN1B3/PPP1B1PP/R2Q1RK1 b - - pm Qc8; id Neutral.207;",
    "r2q1rk1/pp2ppbp/3pbnp1/8/2P4Q/2N3P1/PP2PPBP/R1B2RK1 w - - pm Bxb7; id Neutral.208;",
    "r2q1rk1/ppp1b1pp/1nn1b3/4pp2/8/P1NPBNP1/1P2PPBP/R2Q1RK1 w - - pm Rc1; id Neutral.209;",
    "r2q1rk1/ppp1bppp/1n2b3/4p3/1P1n4/P1NP1NP1/4PPBP/1RBQ1RK1 b - - pm Nxf3+; id Neutral.210;",
    "r2q1rk1/ppp1bppp/1nn1b3/4p3/1P6/P1NP1NP1/4PPBP/R1BQ1RK1 b - b3 0 10 ",
    "r2q1rk1/ppp1bppp/1nn5/4p3/6b1/2NPBNP1/PP2PPBP/R2Q1RK1 w - - pm Rc1; id Neutral.211;",
    "r2q1rk1/ppp2ppp/2np1n2/2bNp3/2P3b1/4PNPP/PP1P1PB1/R1BQ1RK1 b - - pm Bh5; id Neutral.212;",
    "r2qk2r/1p1n1ppp/2p1pn2/p4b2/PbBP4/1Q2PN2/NP3PPP/R1B2RK1 b kq - pm Bd6; id Neutral.213;",
    "r2qk2r/1p3pp1/p1b1pn1p/3p4/1b2PB2/P1N2P2/1PP3PP/2KRQB1R b kq - pm Bxc3; id Neutral.214;",
    "r2qk2r/2p1bppp/p1n1b3/1p1pP3/4n3/1BP2N2/PP3PPP/RNBQ1RK1 w kq - pm Be3; id Neutral.215;",
    "r2qk2r/2pb1ppp/p2b1n2/1p2n3/2p1P3/2N2NP1/PP2QPBP/R1B2RK1 w kq - pm Nxe5; id Neutral.216;",
    "r2qk2r/3bbpp1/p1nppn1p/1p6/3NPP2/2NBB3/PPPQ2PP/1K1R3R b kq - pm O-O; id Neutral.217;",
    "r2qk2r/3bbppp/p1nppn2/1p4B1/4PP2/2N2N2/PPPQ2PP/2KR1B1R w kq - pm Bxf6; id Neutral.218;",
    "r2qk2r/3nbpp1/pn1pb2p/1p2p3/4P1P1/1NN1BP2/PPP2Q1P/1K1R1B1R w kq - pm h4; id Neutral.219;",
    "r2qk2r/3nbppp/p2pbn2/1p2p3/4P3/1NN1BP2/PPPQ2PP/2KR1B1R w kq - pm g4; id Neutral.220;",
    "r2qk2r/5pbp/p1np4/1p1Npb2/8/N1P5/PP3PPP/R2QKB1R w KQkq - 0 14",
    "r2qk2r/5pbp/p1npb3/3Np2Q/2B1Pp2/N7/PP3PPP/R4RK1 b kq - 0 15 ",
    "r2qk2r/pp1bppbp/2np1np1/1B6/3NP3/2N1B3/PPP2PPP/R2Q1RK1 w kq - pm f3; id Neutral.221;",
    "r2qk2r/pp1n1ppp/2p1pn2/8/PbBP2bN/1QN1P3/1P3PPP/R1B2RK1 b kq - pm a5; id Neutral.222;",
    "r2qk2r/pp2bppp/2bppn2/6B1/3QP3/2N2N2/PPP2PPP/2KR3R w kq - pm Rhe1; id Neutral.223;",
    "r2qk2r/pp2bppp/2n1pn2/1N5b/3P4/4BN1P/PP2BPP1/R2Q1RK1 w kq - pm Ne5; id Neutral.224;",
    "r2qk2r/pp2bppp/5n2/2np4/8/1N3N2/PPP2PPP/R1BQ1RK1 b kq - pm Nce4; id Neutral.225;",
    "r2qkb1r/1b1n1pp1/p3p2p/3n4/1p1N2P1/4BP2/PPPQN2P/1K1R1B1R w kq - pm Nf4; id Neutral.226;",
    "r2qkb1r/1b1n1ppp/p1p1pn2/1p6/P2P4/2NBPN2/1P1B1PPP/R2QK2R b KQkq - pm b4; id Neutral.227;",
    "r2qkb1r/1b1n1ppp/p1p1pn2/1p6/P2P4/2NBPN2/1P3PPP/R1BQ1RK1 b kq - pm b4; id Neutral.228;",
    "r2qkb1r/1b1n1ppp/p3pn2/2p5/Pp1PN3/3BPN2/1P3PPP/R1BQ1RK1 w kq - pm Nxf6+; id Neutral.229;",
    "r2qkb1r/1b3ppp/p3pn2/2p5/Pp1P4/3BPN2/1P3PPP/R1BQ1RK1 w kq - pm Qe2; id Neutral.230;",
    "r2qkb1r/2p2ppp/p1n1b3/1p1pP3/8/1BP2N2/P1P2PPP/R1BQ1RK1 b kq - pm Be7; id Neutral.231;",
    "r2qkb1r/3b1p1p/p1nppp2/1p6/3NPP2/2N5/PPPQ2PP/2KR1B1R w kq - pm Kb1; id Neutral.232;",
    "r2qkb1r/3n1pp1/p3p2p/1p1b4/3N2P1/P3BP2/1PPQ2BP/2KR3R b kq - pm Rc8; id Neutral.233;",
    "r2qkb1r/5pp1/p1bppn1p/1p6/4P3/4BP2/PPPQN1PP/2KR1B1R b kq - pm Qc7; id Neutral.234;",
    "r2qkb1r/5ppp/pn1p1n2/3Pp3/Pp6/1N2BP2/1PPQ2PP/R3KB1R w KQkq - pm Bxb6; id Neutral.235;",
    "r2qkb1r/p1pp1ppp/bpn1pn2/8/2PP4/1Q3NP1/PP2PP1P/RNB1KB1R w KQkq - pm Nbd2; id Neutral.236;",
    "r2qkb1r/pb1n1ppp/2p1pn2/1p6/3P4/2NBPN2/PP1B1PPP/R2QK2R b KQkq - pm a6; id Neutral.237;",
    "r2qkb1r/pp1n1ppp/2p2n2/3pp2b/4P3/3P1NPP/PPPN1PB1/R1BQ1RK1 b kq e3 0 8 ",
    "r2qkb1r/pp1n1ppp/2p2n2/4p2b/4P3/5NPP/PPPN1PB1/R1BQ1RK1 b kq - pm Bc5; id Neutral.238;",
    "r2qkb1r/pp1npppp/2p2n2/3p4/2P3b1/3P1NP1/PP2PPBP/RNBQ1RK1 b kq - pm e6; id Neutral.239;",
    "r2qkb1r/pp1npppp/2p2n2/3p4/6b1/1P3NP1/PBPPPPBP/RN1Q1RK1 b kq - pm e6; id Neutral.240;",
    "r2qkbnr/pp1n1ppp/2p1p3/3pPb2/3P4/4B3/PPPN1PPP/R2QKBNR w KQkq - pm Ngf3; id Neutral.241;",
    "r2qkbnr/pp1npppp/2p5/3p4/6b1/3P1NP1/PPP1PPBP/RNBQ1RK1 b kq - pm Ngf6; id Neutral.242;",
    "r2qkbnr/ppp1pp1p/2np2p1/8/2PP2b1/1Q3N2/PP2PPPP/RNB1KB1R w KQkq - fmvn 5; hmvc 2;",
    "r2qnrk1/3nbppp/3pb3/p3pPP1/1p2P3/1N2B3/PPPQN2P/2KR1B1R b - - pm a4; id Neutral.243;",
    "r2qnrk1/3nbppp/p2pb3/4p1P1/1p2P3/1N2BP2/PPPQN2P/2KR1B1R w - - pm f4; id Neutral.244;",
    "r2qr1k1/1bpnbppp/p2p1n2/1p2p3/P2PP3/2P2N1P/1PBN1PP1/R1BQR1K1 b - - pm c5; id Neutral.245;",
    "r2qrbk1/1b1n1p1p/p2p1np1/1p1Pp3/P1p1P3/2P2NNP/1PB2PP1/R1BQR1K1 w - - 0 17 ",
    "r2qrbk1/1bp2ppp/p1np1n2/1p2p3/P2PP3/1BP2N1P/1P1N1PP1/R1BQR1K1 b - - pm h6; id Neutral.246;",
    "r2qrbk1/1bpn1ppp/p2p1n2/1p1Pp3/4P3/1BP2NNP/PP3PP1/R1BQR1K1 b - - pm Nc5; id Neutral.247;",
    "r3k2r/1bq3pp/p2bpn2/1pn3N1/2p1P3/2N4Q/PPB2PPP/R1B2RK1 b kq - pm O-O-O; id Neutral.248;",
    "r3k2r/1bqnbppp/pp1ppn2/8/2PNP3/2N1B1P1/PP3PBP/R2QR1K1 w kq - pm Rc1; id Neutral.249;",
    "r3k2r/p1qbnppp/1pn1p3/2ppP3/P2P4/2PB1N2/2P2PPP/R1BQ1RK1 b kq - 5 11 ",
    "r3k2r/pp1qppbp/2np1np1/8/2PNP3/2N5/PP3PPP/R1BQ1RK1 w kq - pm Nde2; id Neutral.250;",
    "r3k2r/pp3ppp/n1p1p3/3q4/P1N5/8/1PK1Q1PP/R4B1R b kq - pm O-O; id Neutral.251;",
    "r3kb1r/1b1n1ppp/p3pq2/2p5/Pp1P4/3BPN2/1P3PPP/R1BQ1RK1 w kq - pm Qe2; id Neutral.252;",
    "r3kb1r/1bqn1ppp/p3pn2/1p1P4/2p1P3/2N2N2/PPB2PPP/R1BQ1RK1 w kq - pm dxe6; id Neutral.253;",
    "r3kb1r/1bqn1ppp/pp1ppn2/8/P2NPP2/2N1BB2/1PP3PP/R2QK2R w KQkq - pm Qe2; id Neutral.254;",
    "r3kb1r/1ppb1ppp/p1p2n2/8/3NP3/5P2/PPP3PP/RNB1K2R w KQkq - pm Nc3; id Neutral.255;",
    "r3kb1r/5ppp/pq1p1n2/3Pp3/Pp6/1N3P2/1PPQ2PP/R3KB1R w KQkq - pm a5; id Neutral.256;",
    "r3kb1r/pp1bpp1p/2np1p2/q7/3NP3/2N5/PPP1BPPP/R2QK2R w KQkq - pm O-O; id Neutral.257;",
    "r3kb1r/pp2pppp/1nnqb3/8/3p4/NBP2N2/PP3PPP/R1BQ1RK1 b kq - 3 10 ",
    "r3kb1r/pp3ppp/2n1pn2/3q3b/3P4/4BN1P/PP2BPP1/RN1Q1RK1 b kq - pm Be7; id Neutral.258;",
    "r3qrk1/1ppb1pbn/n2p2pp/p2Pp3/2P1P2B/P1N5/1P1NBPPP/R2Q1RK1 w - - 1 13 ",
    "r3r1k1/1bq2ppp/ppnb1n2/2pB4/P3P3/2N1BN1P/1P2QPP1/R2R2K1 b - - pm h6; id Neutral.259;",
    "r3r1k1/1pqbbppp/p2ppn2/P7/1n1NPP2/2NBB3/1PP3PP/R2Q1R1K w - - pm Nf3; id Neutral.260;",
    "r3r1k1/pbqn1ppp/2pb1n2/1p2p3/3PP3/2N2NP1/PPQ1BP1P/R1BR2K1 w - - pm a3; id Neutral.261;",
    "r4rk1/1bq2ppp/ppnb1n2/2pB4/P7/2N1PN1P/1P2QPP1/R1BR2K1 w - - pm e4; id Neutral.262;",
    "r4rk1/1p2b1p1/2n4p/1q2pp2/p7/Pb1PBNP1/1Q2PPBP/1RR3K1 w - - pm Rxc6; id Neutral.263;",
    "r4rk1/1p2bppp/2n5/1q2p3/p7/Pb1P1NP1/1Q2PPBP/1RB2RK1 w - - pm Bd2; id Neutral.264;",
    "r4rk1/1pqnbppp/p2pbn2/8/P3PR2/1NN1B3/1PP1B1PP/R2Q3K b - - pm Ne5; id Neutral.265;",
    "r4rk1/p1qnbppp/bpp1pn2/3p4/2PP4/1PBN2P1/P2NPPBP/R2Q1RK1 b - - pm c5; id Neutral.266;",
    "r4rk1/p4ppp/4p3/2Qn3q/8/B3PP2/PP3P1P/R4R1K w - - pm Kg2; id Neutral.267;",
    "r4rk1/pp2bp1p/2b1pp2/q3P3/2BQ4/2N5/PPP3PP/2KR3R w - - pm Bd5; id Neutral.268;",
    "r4rk1/pp2bppp/2nqpn2/7b/3P4/1QN1BN1P/PP2BPP1/R4RK1 w - - pm Rfd1; id Neutral.269;",
    "r4rk1/pp3ppp/n1p1p3/3q4/P1N5/8/1PK1Q1PP/R4B1R w - - pm Qe5; id Neutral.270;",
    "rn1q1rk1/1b2bppp/pp1p1n2/4p3/4P3/1NN1B3/PPP1BPPP/R2Q1R1K w - - pm f3; id Neutral.271;",
    "rn1q1rk1/1bp1bppp/p3pn2/1p6/3P4/5NP1/PPQBPPBP/RN3RK1 b - - pm Be4; id Neutral.272;",
    "rn1q1rk1/1p2bppp/p2pbn2/4p3/4PP2/1NN1B3/PPP1B1PP/R2Q1RK1 b - - pm exf4; id Neutral.273;",
    "rn1q1rk1/1pp1bppp/p1b1pn2/8/P1QP4/5NP1/1P2PPBP/RNB2RK1 w - - pm Bg5; id Neutral.274;",
    "rn1q1rk1/p1p1bppp/1p6/3pP3/3Pn3/1PN2NP1/P2B1PKP/R2Q3R b - - pm Qd7; id Neutral.275;",
    "rn1q1rk1/p1p1bppp/bp2pn2/3p4/2PPP3/1PN2NP1/P2B1P1P/R2QKB1R w KQ - pm cxd5; id Neutral.276;",
    "rn1q1rk1/p3bppp/bpp1pn2/3p4/2PP4/1PB2NP1/P2NPPBP/R2QK2R w KQ - pm O-O; id Neutral.277;",
    "rn1q1rk1/pb1p1ppp/1p3b2/2pP4/4n3/2N2NP1/PP1BPPBP/2RQ1RK1 b - - pm Nxd2; id Neutral.278;",
    "rn1q1rk1/pb1pbppp/1p2pn2/2p5/2P5/1P3NP1/PB1PPPBP/RN1Q1RK1 w - - pm Nc3; id Neutral.279;",
    "rn1q1rk1/pb1pbppp/4pn2/2p5/2P2B2/P1N1PN2/1PQ2PPP/3RKB1R b K - pm Nh5; id Neutral.280;",
    "rn1q1rk1/pb1pppbp/1p3np1/2p5/2P5/1P3NP1/PB1PPPBP/RN1Q1RK1 w - - pm Nc3; id Neutral.281;",
    "rn1q1rk1/pb2bppp/1p1ppn2/8/2PQ4/1PN2NP1/P3PPBP/R1BR2K1 b - - pm Nbd7; id Neutral.282;",
    "rn1q1rk1/pb2ppbp/1p1p1np1/2p5/4P3/2PP1NP1/PP1N1PBP/R1BQ1RK1 w - - pm Re1; id Neutral.283;",
    "rn1q1rk1/pb2ppbp/1p1p1np1/2p5/4P3/2PP1NP1/PP1N1PBP/R1BQR1K1 b - - pm Nc6; id Neutral.284;",
    "rn1q1rk1/pb3pp1/1pp2b1p/3p4/1P1P4/2N1PN2/P3BPPP/R2QK2R w KQ - pm O-O; id Neutral.285;",
    "rn1q1rk1/pb3ppp/1p1ppb2/2p5/2PP4/1PQ2NP1/PB2PPBP/R2R2K1 b - - pm Qe7; id Neutral.286;",
    "rn1q1rk1/pbp2ppp/1p1ppn2/6B1/2PP4/P1Q1PN2/1P3PPP/R3KB1R b KQ - pm Nbd7; id Neutral.287;",
    "rn1q1rk1/pbp2ppp/1p1ppn2/6B1/2PP4/P1Q1PP2/1P4PP/R3KBNR b KQ - pm Nbd7; id Neutral.288;",
    "rn1q1rk1/pbpp1ppp/1p2pn2/6B1/2PP4/P1Q1P3/1P3PPP/R3KBNR b KQ - pm d6; id Neutral.289;",
    "rn1q1rk1/pbpp1ppp/1p2pn2/6B1/2PP4/P1Q2P2/1P2P1PP/R3KBNR b KQ - pm h6; id Neutral.290;",
    "rn1q1rk1/pbpp1ppp/1p2pn2/8/2PP4/P1Q2N2/1P2PPPP/R1B1KB1R w KQ - pm e3; id Neutral.291;",
    "rn1q1rk1/pp3pp1/2p1pn1p/2bp1b2/8/1P1P1NP1/PBPNPPBP/R2Q1RK1 w - - pm Ne5; id Neutral.292;",
    "rn1q1rk1/pp3ppp/3b4/3p4/3P2b1/2PB1N2/P4PPP/1RBQ1RK1 b - - 2 12 ",
    "rn1qk1nr/pbpp2pp/1p2p3/8/1bPPP3/2NB4/PP4PP/R1BQK1NR b KQkq - pm Nf6; id Neutral.293;",
    "rn1qk1nr/pbppppbp/1p4p1/8/3PP3/3B1N2/PPP2PPP/RNBQK2R w KQkq - fmvn 5; hmvc 2;",
    "rn1qk2r/1bp2pp1/pp1bpn1p/3p4/3P1B2/2NBPN2/PPP2PPP/R2QR1K1 b kq - pm O-O; id Neutral.294;",
    "rn1qk2r/p1p1bppp/bp3n2/3p4/3P4/1PN2NP1/P2BPP1P/R2QKB1R w KQkq - pm Bg2; id Neutral.295;",
    "rn1qk2r/p1ppbppp/bp2pn2/8/2PP4/1P3NP1/P2BPP1P/RN1QKB1R w KQkq - pm Bg2; id Neutral.296;",
    "rn1qk2r/p1ppbppp/bp2pn2/8/2PP4/1P3NP1/P2BPPBP/RN1QK2R b KQkq - pm c6; id Neutral.297;",
    "rn1qk2r/pb1pbppp/1p2pn2/2p5/2P5/1P3NP1/PB1PPPBP/RN1Q1RK1 b kq - pm O-O; id Neutral.298;",
    "rn1qk2r/pb2bppp/1p2p3/2pn4/8/2N2NP1/PP1PPPBP/R1BQR1K1 w kq - pm e4; id Neutral.299;",
    "rn1qk2r/pp3ppp/2p1pn2/4N3/PbpPb3/2N2P2/1P4PP/R1BQKB1R w KQkq - pm fxe4; id Neutral.300;",
    "rn1qk2r/pp3ppp/2p1pn2/4Nb2/PbpP4/2N2P2/1P2P1PP/R1BQKB1R w KQkq - pm e4; id Neutral.301;",
    "rn1qkb1r/1b3pp1/p2ppn1p/1p6/3NP1P1/P1N1BP2/1PP4P/R2QKB1R w KQkq - pm Qd2; id Neutral.302;",
    "rn1qkb1r/p1pp1ppp/bp2pn2/8/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkb1r/p1pp1ppp/bp2pn2/8/2PP4/P4N2/1P2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkb1r/pb3p2/2p1pn1p/1p2N3/2pPP1pP/2N3B1/PP2BPP1/R2QK2R b KQkq - pm h5; id Neutral.303;",
    "rn1qkb1r/pbpp1ppp/1p2pn2/8/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rn1qkb1r/pbpp1ppp/1p2pn2/8/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkb1r/pbpp1ppp/1p2pn2/8/2PP4/P4N2/1P2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkb1r/pp2pppp/2p2n2/3p1b2/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 4;",
    "rn1qkb1r/pp2pppp/2p2n2/3p1b2/2PP4/4PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkb1r/pp2pppp/2p2n2/3p1b2/3P1B2/2N2N2/PPP1PPPP/R2QKB1R w KQkq - fmvn 5; hmvc 2;",
    "rn1qkb1r/pp2pppp/2p2n2/3p1b2/8/1P3NP1/P1PPPPBP/RNBQK2R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkb1r/pp2pppp/2p2n2/3p1b2/8/5NP1/PPPPPPBP/RNBQ1RK1 w kq - fmvn 5; hmvc 2;",
    "rn1qkb1r/pp2pppp/2p2n2/3p1b2/8/5NP1/PPPPPPBP/RNBQ1RK1 w kq - pm d3; id Neutral.304;",
    "rn1qkb1r/pp2pppp/2p2n2/3p4/2P3b1/1P3NP1/P2PPP1P/RNBQKB1R w KQkq - pm Bg2; id Neutral.305;",
    "rn1qkb1r/pp2pppp/2p2n2/3p4/6b1/3P1NP1/PPP1PPBP/RNBQK2R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkb1r/pp2pppp/2p2n2/3p4/6b1/3P1NP1/PPP1PPBP/RNBQK2R w KQkq - pm O-O; id Neutral.306;",
    "rn1qkb1r/pp2pppp/2p2n2/3p4/6b1/5NP1/PPPPPPBP/RNBQ1RK1 w kq - fmvn 5; hmvc 2;",
    "rn1qkb1r/pp2pppp/2p2n2/4Nb2/P1pP4/2N5/1P2PPPP/R1BQKB1R b KQkq - pm e6; id Neutral.307;",
    "rn1qkb1r/pp2pppp/3p1n2/2pP1b2/2P5/2N5/PP2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 2;",
    "rn1qkb1r/ppp2ppp/4pnb1/4N3/3P2P1/2N4P/PPP5/R1BQKB1R w KQkq - pm Bg2; id Neutral.308;",
    "rn1qkb1r/pppbpppp/5n2/3p4/1P1P3N/8/P1P1PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 3;",
    "rn1qkbnr/pbpp2pp/1p2p3/5p2/2P5/1P2P3/PB1P1PPP/RN1QKBNR w KQkq - fmvn 5; hmvc 0;",
    "rn1qkbnr/pp1bpppp/3p4/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkbnr/pp2pppp/2p1b3/8/2pP4/4PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rn1qkbnr/pp2pppp/2p5/5b2/3PN3/8/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rn1qkbnr/pp3ppp/2p1p3/3pPb2/3P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rn1qkbnr/pp3ppp/2p1p3/3pPb2/3P4/2P5/PP3PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rn1qkbnr/pp3ppp/2p1p3/3pPb2/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rn1qkbnr/pp3ppp/2p1p3/3pPb2/3P4/P7/1PP2PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rn1qkbnr/pp3ppp/2p1p3/3pPb2/8/5N1P/PPPP1PP1/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rn1qkbnr/ppp1pppp/8/3p4/6b1/1P3N2/P1PPPPPP/RNBQKB1R w KQkq - pm e3; id Neutral.309;",
    "rn1qkbnr/pppb1ppp/4p3/3pP3/3P4/8/PPP2PPP/RNBQKBNR w KQkq - pm Nf3; id Neutral.310;",
    "rn1qr1k1/pbp2pp1/1p3b1p/3p4/3P4/1QN1PN2/PP2BPPP/R4RK1 b - - pm c6; id Neutral.311;",
    "rn1qr1k1/ppp2ppp/5n2/3p1b2/1b1P4/2N1PN2/PP1B1PPP/2RQKB1R w K - pm Qb3; id Neutral.312;",
    "rn3rk1/pbq1bppp/1p2p3/2p5/3PP3/P1PB1N2/4QPPP/R1B2RK1 b - - pm Nc6; id Neutral.313;",
    "rnb1k1nr/pp1pppbp/1q4p1/2p5/2PPP3/5N2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnb1k1nr/pppq1ppp/4p3/3pP3/1b1P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnb1k2r/1p2bppp/pq1ppn2/6B1/4PP2/1NN2Q2/PPP3PP/R3KB1R b KQkq - pm Nbd7; id Neutral.314;",
    "rnb1k2r/ppppqppp/4pn2/8/1bPP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 4;",
    "rnb1kb1r/1p3ppp/p2ppn2/5PB1/3NP3/q1N5/P1PQ2PP/1R2KB1R b Kkq - pm Nc6; id Neutral.315;",
    "rnb1kb1r/1p3ppp/p2ppn2/6B1/3NPP2/q1N5/P1PQ2PP/1R2KB1R w Kkq - 2 10 ",
    "rnb1kb1r/1p3ppp/p2ppn2/6B1/3NPP2/q1N5/P1PQ2PP/1R2KB1R w Kkq - pm f5; id Neutral.316;",
    "rnb1kb1r/1p3ppp/pq1ppn2/6B1/4PP2/1NN5/PPP3PP/R2QKB1R b KQkq - pm Be7; id Neutral.317;",
    "rnb1kb1r/1pq2ppp/p2ppn2/6B1/3NPP2/2N2Q2/PPP3PP/R3KB1R b KQkq - pm Be7; id Neutral.318;",
    "rnb1kb1r/pp2pppp/2pp1n2/q7/3PPP2/2N5/PPP3PP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnb1kb1r/ppp2pp1/3ppq1p/8/3PP3/5N2/PPP2PPP/RN1QKB1R w KQkq - pm Nc3; id Neutral.319;",
    "rnb1kbnr/1pqp1ppp/p3p3/2p5/4P3/2N2N2/PPPPBPPP/R1BQK2R w KQkq - fmvn 5; hmvc 2;",
    "rnb1kbnr/pp2pppp/8/q2p4/3P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnb1kbnr/pp3ppp/1q2p3/2ppP3/3P4/2P5/PP3PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnb1kbnr/ppp1pp1p/6p1/q7/5P2/2N5/PPPP2PP/R1BQKBNR w KQkq - fmvn 5; hmvc 2;",
    "rnb1kbnr/ppp3pp/3p1q2/4Np2/3PP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnb1kbnr/ppqp1ppp/4p3/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnb2rk1/1pp2p1p/p7/3Pq2n/2BN1ppP/8/PPP3P1/R1BQ1RK1 w - - pm c3; id Neutral.320;",
    "rnb2rk1/1pq1bppp/p2ppn2/8/P2NPP2/2N1B3/1PP1B1PP/R2Q1RK1 b - - pm Nc6; id Neutral.321;",
    "rnb2rk1/2q1bppp/p2ppn2/1p6/3NP3/1BN3Q1/PPP2PPP/R1B1R1K1 b - - pm Re8; id Neutral.322;",
    "rnb2rk1/ppp1qppp/3p1n2/3Pp3/2P1P3/5NP1/PP1N1PBP/R2Q1RK1 w - - 1 11 ",
    "rnbq1k1r/ppp2p1p/5bp1/3p2N1/3P1Q1P/2N5/PP2PPP1/R3KB1R b KQ - pm h6; id Neutral.323;",
    "rnbq1rk1/1p2ppbp/2pp1np1/p7/P2PP3/2N2N1P/1PP1BPP1/R1BQ1RK1 b - - 0 8 ",
    "rnbq1rk1/2pnppbp/p5p1/1p2P3/3P4/1QN2N2/PP3PPP/R1B1KB1R w KQ - 1 10 ",
    "rnbq1rk1/p3bppp/1pp1pn2/3p4/2PP1B2/5NP1/PPQ1PPBP/RN3RK1 b - - pm Bb7; id Neutral.324;",
    "rnbq1rk1/p3ppbp/1p1p1np1/2p5/3P1B2/2P1PN1P/PP2BPP1/RN1QK2R w KQ - 0 8 ",
    "rnbq1rk1/pp1pppbp/5np1/2p5/2PP4/2N1PN2/PP2BPPP/R1BQK2R b KQ - pm cxd4; id Neutral.325;",
    "rnbq1rk1/pp2ppbp/5np1/2p3B1/2B1P3/2P2N2/PP1N1PPP/R2QK2R b KQ - pm Nc6; id Neutral.326;",
    "rnbq1rk1/pp2ppbp/5np1/3p4/2PP4/2N2N2/PP2BPPP/R1BQ1RK1 b - - pm Nc6; id Neutral.327;",
    "rnbq1rk1/pp2ppbp/6p1/8/3PP3/5N2/P3BPPP/1RBQK2R b K - 0 10 ",
    "rnbq1rk1/pp3pbp/2p3p1/3p4/1P1P4/2N1PN2/P3BPPP/R2QK2R b KQ - pm Be6; id Neutral.328;",
    "rnbq1rk1/pp3ppp/4p3/3n4/1b1N4/2N3P1/PP2PPBP/R1BQK2R w KQ - pm Qb3; id Neutral.329;",
    "rnbq1rk1/pp3ppp/5n2/2bp4/8/3B1N2/PPP2PPP/RNBQ1RK1 w - - pm Nc3; id Neutral.330;",
    "rnbq1rk1/ppp1bppp/4pn2/3p4/2P5/1P2PN2/PB1P1PPP/RN1QKB1R w KQ - pm Be2; id Neutral.331;",
    "rnbq1rk1/ppp2pbp/3p1np1/4p3/2P5/2N2NP1/PP1PPPBP/R1BQ1RK1 w - - pm d3; id Neutral.332;",
    "rnbq1rk1/pppp1ppp/4pn2/8/1bPP4/2N1P3/PP3PPP/R1BQKBNR w KQ - fmvn 5; hmvc 1;",
    "rnbq1rk1/pppp1ppp/4pn2/8/1bPP4/2N5/PPQ1PPPP/R1B1KBNR w KQ - fmvn 5; hmvc 4;",
    "rnbq1rk1/pppp1ppp/4pn2/8/2PP4/P1Q5/1P2PPPP/R1B1KBNR b KQ - pm b6; id Neutral.333;",
    "rnbq1rk1/pppp1ppp/5n2/2b1p3/2B1P3/2NP4/PPP2PPP/R1BQK1NR w KQ - fmvn 5; hmvc 1;",
    "rnbq1rk1/pppp1ppp/5n2/2b1p3/2P5/2N1P1P1/PP1P1PBP/R1BQK1NR b KQ - pm Nc6; id Neutral.334;",
    "rnbq1rk1/ppppppbp/5np1/6B1/3P4/5N2/PPPNPPPP/R2QKB1R w KQ - pm c3; id Neutral.335;",
    "rnbq1rk1/ppppppbp/5np1/8/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQ - fmvn 5; hmvc 4;",
    "rnbq1rk1/ppppppbp/5np1/8/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQ - fmvn 5; hmvc 1;",
    "rnbq1rk1/ppppppbp/5np1/8/2PP4/6P1/PP2PPBP/RNBQK1NR w KQ - fmvn 5; hmvc 3;",
    "rnbqk1nr/1ppp1ppp/8/p1bNp3/2P5/P7/1P1PPPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqk1nr/p1p2ppp/1p2p3/3pP3/1b1P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqk1nr/pp1p1ppp/4p3/2b5/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqk1nr/pp1p1ppp/8/2b1p3/8/4P3/P1PP1PPP/RNBQKBNR w KQkq - pm d4; id Neutral.336;",
    "rnbqk1nr/pp1pppbp/6p1/2p5/2P5/2N2NP1/PP1PPP1P/R1BQKB1R b KQkq - pm Nc6; id Neutral.337;",
    "rnbqk1nr/pp1pppbp/6p1/8/3pP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk1nr/pp1pppbp/6p1/8/3pP3/2P2N2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk1nr/pp2bppp/3p4/2p1p3/2P1P3/5N2/PP1PBPPP/RNBQK2R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk1nr/pp2ppbp/2p3p1/3N4/3PP3/8/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqk1nr/pp2ppbp/2p3p1/3p4/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk1nr/pp2ppbp/3p2p1/2pP4/4P3/5N2/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk1nr/pp3ppp/2pb4/3pp3/8/3P1NP1/PPP1PPBP/RNBQK2R w KQkq - pm O-O; id Neutral.338;",
    "rnbqk1nr/pp3ppp/4p3/2ppP3/1b1P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqk1nr/ppp2ppp/4p3/3pP3/1b1P4/2N5/PPP2PPP/R1BQKBNR b KQkq - pm c5; id Neutral.339;",
    "rnbqk1nr/ppp2ppp/8/3p4/1b1P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqk1nr/pppp1ppp/8/4p3/1bP5/2N5/PP1PPPPP/R1BQKBNR w KQkq - pm Nd5; id Neutral.340;",
    "rnbqk2r/1p2bppp/p2ppn2/6B1/3NPP2/2N2Q2/PPP3PP/R3KB1R b KQkq - pm Qc7; id Neutral.341;",
    "rnbqk2r/1p2ppb1/p2p3p/6p1/3NP1n1/2N3B1/PPP1BPPP/R2QK2R b KQkq - pm h5; id Neutral.342;",
    "rnbqk2r/1p2ppb1/p2p4/6pp/3NP1n1/2N3B1/PPP1BPPP/R2QK2R w KQkq - pm Bxg4; id Neutral.343;",
    "rnbqk2r/p1pp1ppp/1p2pn2/8/1bPP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/p1ppbppp/1p2pn2/8/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/p1ppppbp/1p3np1/8/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/p2p1ppp/1p2pn2/b1p5/2PP4/PQN2N2/1P2PPPP/R1B1KB1R w KQkq - pm Bg5; id Neutral.344;",
    "rnbqk2r/pp1p1ppp/4pn2/2p5/1bPP4/2N1P3/PP2NPPP/R1BQKB1R b KQkq - pm cxd4; id Neutral.345;",
    "rnbqk2r/pp1p1ppp/4pn2/2p5/1bPP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/pp1p1ppp/4pn2/2p5/1bPP4/2N5/PPQ1PPPP/R1B1KBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/pp1p1ppp/4pn2/8/1bPN4/6P1/PP1BPP1P/RN1QKB1R b KQkq - pm Qb6; id Neutral.346;",
    "rnbqk2r/pp1pbppp/2p1pn2/8/2PP1B2/5N2/PP2PPPP/RN1QKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/pp1pppbp/5np1/2pP4/2P5/2N5/PP2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/ppp1bppp/4pn2/3p2B1/2PP4/2N5/PP2PPPP/R2QKBNR w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/ppp1bppp/4pn2/3p2B1/2PP4/5N2/PP2PPPP/RN1QKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/ppp1bppp/4pn2/3p2B1/3PP3/2N5/PPP2PPP/R2QKBNR w KQkq - fmvn 5; hmvc 4;",
    "rnbqk2r/ppp1bppp/4pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/ppp1bppp/4pn2/3p4/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/ppp1bppp/4pn2/3p4/3P4/5NP1/PPP1PPBP/RNBQK2R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/ppp1nppp/4p3/3pP3/1b1P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/ppp1nppp/4p3/3pP3/3P4/P1P5/2P2PPP/R1BQKBNR b KQkq - pm c5; id Neutral.347;",
    "rnbqk2r/ppp1ppbp/3p1np1/8/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/ppp1ppbp/3p1np1/8/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/ppp1ppbp/3p1np1/8/2PPP3/2N5/PP3PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/ppp1ppbp/3p1np1/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/ppp1ppbp/3p1np1/8/3PP3/2P2N2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/ppp1ppbp/3p1np1/8/3PPP2/2N5/PPP3PP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/ppp1ppbp/5np1/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/ppp1ppbp/5np1/3P4/3P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/ppp1ppbp/5np1/3p4/3P4/P1P2N2/1P2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 3;",
    "rnbqk2r/ppp1ppbp/5np1/3p4/5P2/4PN2/PPPPB1PP/RNBQK2R w KQkq - fmvn 5; hmvc 3;",
    "rnbqk2r/ppp1ppbp/5np1/3p4/8/1P2PN2/PBPP1PPP/RN1QKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqk2r/ppp2pbp/6p1/3p4/3P4/2N1PN2/PP3PPP/R2QKB1R b KQkq - pm O-O; id Neutral.348;",
    "rnbqk2r/ppp2ppp/3p1n2/2b1p3/2P1P3/2NP4/PP3PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/ppp2ppp/4pn2/3p2B1/1b1PP3/2N5/PPP2PPP/R2QKBNR w KQkq - fmvn 5; hmvc 3;",
    "rnbqk2r/ppp2ppp/4pn2/3p4/1bPP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqk2r/ppp2ppp/5n2/3p2B1/1b1P4/2N5/PPQ1PPPP/R3KBNR b KQkq - pm h6; id Neutral.349;",
    "rnbqk2r/pppp1ppp/4pn2/8/1bPP4/1QN5/PP2PPPP/R1B1KBNR b KQkq - pm c5; id Neutral.350;",
    "rnbqk2r/pppp1ppp/4pn2/8/2PP4/2b2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/pppp1ppp/5n2/2b5/3pP3/2N5/PPP1NPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqk2r/pppp2pp/4pn2/5p2/1bP5/2NP1N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 3;",
    "rnbqk2r/ppppbppp/4pn2/8/2PP4/5N2/PP1BPPPP/RN1QKB1R w KQkq - fmvn 5; hmvc 4;",
    "rnbqk2r/ppppbppp/4pn2/8/2PP4/6P1/PP1BPP1P/RN1QKBNR w KQkq - fmvn 5; hmvc 3;",
    "rnbqkb1r/1p2pppp/p1p2n2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/1p3pp1/p2ppn1p/8/3NPPP1/2N1B3/PPP4P/R2QKB1R b KQkq - pm e5; id Neutral.351;",
    "rnbqkb1r/1p3ppp/p2ppn2/8/3NP3/2N3P1/PPP2PBP/R1BQK2R b KQkq - pm Be7; id Neutral.352;",
    "rnbqkb1r/1p3ppp/p2ppn2/8/3NPP2/2N1B3/PPP3PP/R2QKB1R b KQkq - pm b5; id Neutral.353;",
    "rnbqkb1r/1pp1pppp/p4n2/8/2pP4/4PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/1pp2ppp/p3pn2/3p4/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/3ppppp/p4n2/1ppP4/2P5/2N5/PP2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/3ppppp/p4n2/1PpP4/8/8/PP2PPPP/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/5pp1/p2ppn1p/1p6/3NP1P1/P1N1BP2/1PP4P/R2QKB1R b KQkq - pm Bb7; id Neutral.354;",
    "rnbqkb1r/5ppp/p2ppn2/1p6/3NP1P1/2N1BP2/PPP4P/R2QKB1R b KQkq - pm h6; id Neutral.355;",
    "rnbqkb1r/p1p2ppp/1p2pn2/3p4/2PP4/4PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/p1p2ppp/1p2pn2/3p4/4P3/3P1N2/PPPN1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/p2p1ppp/4pn2/1ppP4/2P5/5N2/PP2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/p2pp1pp/5n2/1ppP1p2/2P5/6P1/PP2PP1P/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1p1ppp/2p2n2/4p3/2B1P3/3P4/PPP2PPP/RNBQK1NR w KQkq - pm Nf3; id Neutral.356;",
    "rnbqkb1r/pp1p1ppp/2p2n2/8/2P1p3/2N2NP1/PP1PPP1P/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1p1ppp/4p3/2pnP3/8/2P2N2/PP1P1PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1p1ppp/4pn2/2p3B1/3P4/5N2/PPP1PPPP/RN1QKB1R w KQkq - pm e3; id Neutral.357;",
    "rnbqkb1r/pp1p1ppp/4pn2/2p5/2P5/1P3N2/PB1PPPPP/RN1QKB1R b KQkq - pm Nc6; id Neutral.358;",
    "rnbqkb1r/pp1p1ppp/4pn2/2p5/2PP4/2N2N2/PP2PPPP/R1BQKB1R b KQkq - pm cxd4; id Neutral.359;",
    "rnbqkb1r/pp1p1ppp/4pn2/2p5/3P4/5NP1/PPP1PP1P/RNBQKB1R w KQkq - pm Bg2; id Neutral.360;",
    "rnbqkb1r/pp1p1ppp/4pn2/8/2PN4/8/PP2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1p1ppp/4pn2/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pp1p1ppp/5n2/2pp4/2P5/6P1/PP2PP1P/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1p1ppp/5n2/4p3/2PN4/8/PP2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1p2pp/4pn2/2p2p2/3P1P2/4PN2/PPP3PP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1pp2p/5np1/2pP1p2/2P5/5N2/PP2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp1ppppp/5n2/2p5/2P5/1P3N2/P2PPPPP/RNBQKB1R b KQkq - pm e6; id Neutral.361;",
    "rnbqkb1r/pp2pp1p/2p2np1/3p4/2P5/5NP1/PP1PPPBP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pp1p/2p2np1/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pp1p/2pp1np1/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pp1p/2pp1np1/8/3PP3/5N2/PPP1QPPP/RNB1KB1R w KQkq -",
    "rnbqkb1r/pp2pp1p/3p1np1/2pP4/2P5/2N5/PP2PPPP/R1BQKBNR w KQkq -",
    "rnbqkb1r/pp2pp1p/5np1/2pp4/8/1P2PN2/PBPP1PPP/RN1QKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pppp/2p2n2/4P3/3p4/2NP4/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pppp/2p2n2/8/2pP4/1Q3N2/PP2PPPP/RNB1KB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pppp/2p2n2/8/2pP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pppp/2p2n2/8/3PN3/8/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pp2pppp/2p2n2/8/4N3/5N2/PPPP1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pp2pppp/3p1n2/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pp2pppp/3p1n2/8/3pP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pppp/5n2/2pp4/3P4/5NP1/PPP1PP1P/RNBQKB1R w KQkq - pm Bg2; id Neutral.362;",
    "rnbqkb1r/pp2pppp/5n2/2pp4/3P4/5NP1/PPP1PPBP/RNBQK2R b KQkq - pm e6; id Neutral.363;",
    "rnbqkb1r/pp2pppp/5n2/3p4/2PP4/8/PP3PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pp2pppp/5n2/3p4/3P4/2N5/PP2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/pp2pppp/5n2/3p4/3P4/3B4/PPP2PPP/RNBQK1NR w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/pp2pppp/5n2/3p4/3P4/5N2/PP2PPPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp2pppp/5n2/3p4/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/pp2pppp/5n2/3P4/8/8/PP1P1PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pp3pp1/2p1pn1p/3p4/2PP3B/2N2N2/PP2PPPP/R2QKB1R b KQkq - pm dxc4; id Neutral.364;",
    "rnbqkb1r/pp3pp1/3ppn1p/8/3NP1PP/2N5/PPP2P2/R1BQKB1R b KQkq - pm Nc6; id Neutral.365;",
    "rnbqkb1r/pp3ppp/2p1pn2/3p2B1/2PP4/2N5/PP2PPPP/R2QKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/2p1pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/2p1pn2/3p4/2PP4/4PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/2p1pn2/3p4/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/2p2n2/3pp3/4P3/2NP2P1/PPP2P1P/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pp3ppp/2p2n2/3pp3/Q3P3/2P2N2/PP1P1PPP/RNB1KB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/pp3ppp/3ppn2/2p5/2B1P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/3ppn2/2p5/4P3/1PN2N2/P1PP1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/2P5/1P3N2/PB1PPPPP/RN1QKB1R w KQkq - pm e3; id Neutral.366;",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/2PP4/2N1P3/PP3PPP/R1BQKBNR w KQkq - pm Nf3; id Neutral.367;",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/3P4/2P1PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/3P4/4PN2/PPPN1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/3PP3/5N2/PPPN1PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/3PP3/5N2/PPPN1PPP/R1BQKB1R w KQkq - pm exd5; id Neutral.368;",
    "rnbqkb1r/pp3ppp/5n2/1Bpp4/3P4/4PN2/PP3PPP/RNBQK2R b KQkq - pm Nc6; id Neutral.369;",
    "rnbqkb1r/pp3ppp/5n2/2pp4/3P4/5N2/PPPNBPPP/R1BQK2R b KQkq - pm Nc6; id Neutral.370;",
    "rnbqkb1r/ppp1pp1p/3p2p1/3nP3/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp1pp1p/5np1/8/2pP4/4PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp1ppp1/5n1p/8/3PN3/8/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/ppp1pppp/1n1p4/4P3/2PP4/8/PP3PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp1pppp/5n2/3p2B1/3P4/4P3/PPP2PPP/RN1QKBNR b KQkq - pm Ne4; id Neutral.371;",
    "rnbqkb1r/ppp1pppp/5n2/3p4/8/6P1/PPPPPPBP/RNBQK1NR w KQkq - pm Nf3; id Neutral.372;",
    "rnbqkb1r/ppp1pppp/8/3p4/3PnB2/4P3/PPP2PPP/RN1QKBNR b KQkq - pm c5; id Neutral.373;",
    "rnbqkb1r/ppp2pp1/4pn1p/3p4/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/ppp2pp1/4pn1p/3p4/3PP3/2NB4/PPP2PPP/R1BQK1NR w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/ppp2ppp/3p1n2/8/2PPp3/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/3p4/8/4n3/5N2/PPPP1PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4p3/3p4/2PPn3/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/ppp2ppp/4pn2/3p4/2P5/1P2PN2/P2P1PPP/RNBQKB1R b KQkq - pm Be7; id Neutral.374;",
    "rnbqkb1r/ppp2ppp/4pn2/3p4/3P4/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - pm Bg5; id Neutral.375;",
    "rnbqkb1r/ppp2ppp/4pn2/6B1/2pP4/2N2N2/PP2PPPP/R2QKB1R b KQkq - pm c6; id Neutral.376;",
    "rnbqkb1r/ppp2ppp/4pn2/6B1/2pP4/5N2/PP2PPPP/RN1QKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4pn2/6B1/3Pp3/2N5/PPP2PPP/R2QKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4pn2/8/2p5/5NP1/PP1PPPBP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4pn2/8/2pP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4pn2/8/2pP4/4PN2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4pn2/8/2pP4/5NP1/PP2PP1P/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4pn2/8/2pP4/5NP1/PP2PPBP/RNBQK2R b KQkq - pm Nc6; id Neutral.377;",
    "rnbqkb1r/ppp2ppp/4pn2/8/2pP4/6P1/PP2PPBP/RNBQK1NR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/4pn2/8/Q1pP4/6P1/PP2PPBP/RNB1K1NR b KQkq - pm Nbd7; id Neutral.378;",
    "rnbqkb1r/ppp2ppp/5n2/3p4/3P4/2N5/PP2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/5n2/3P4/4p3/5N2/PPPPQPPP/RNB1KB1R w KQkq - fmvn 5; hmvc 2;",
    "rnbqkb1r/ppp2ppp/8/3np3/8/2N3P1/PP1PPP1P/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/ppp2ppp/8/3pp3/3Pn3/3B1N2/PPP2PPP/RNBQK2R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkb1r/pppn1ppp/4p3/3pP3/3P4/2N5/PPP2PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pppn1ppp/4p3/3pP3/3P4/8/PPPN1PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pppp1ppp/4pn2/6B1/3P4/8/PPP1PPPP/RN1QKBNR w KQkq - pm e4; id Neutral.379;",
    "rnbqkb1r/pppp1ppp/5n2/4p3/2P5/6P1/PP1PPP1P/RNBQKBNR w KQkq - pm Bg2; id Neutral.380;",
    "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/3P1N2/PPP2PPP/RNBQKB1R b KQkq - pm Nc6; id Neutral.381;",
    "rnbqkb1r/pppp1ppp/8/4P3/3pn3/5N2/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 1;",
    "rnbqkb1r/pppppp1p/5np1/8/3P4/4PN2/PPP2PPP/RNBQKB1R b KQkq - pm Bg7; id Neutral.382;",
    "rnbqkb1r/pppppppp/n7/4P3/3P4/8/PPP2PPP/RNBQKBNR w KQkq -",
    "rnbqkbnr/1p1p1ppp/p3p3/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/1p2pppp/p2p4/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pp1p1ppp/2p5/4p3/2P5/2N3P1/PP1PPP1P/R1BQKBNR b KQkq - pm Nf6; id Neutral.383;",
    "rnbqkbnr/pp1p1ppp/4p3/2p5/2P1P3/8/PP1P1PPP/RNBQKBNR w KQkq - pm Nc3; id Neutral.384;",
    "rnbqkbnr/pp1p1ppp/4p3/2p5/4P3/3P4/PPP2PPP/RNBQKBNR w KQkq - pm Nf3; id Neutral.385;",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/3P4/PPP2PPP/RNBQKBNR b KQkq - pm Nc6; id Neutral.386;",
    "rnbqkbnr/pp2p1pp/2p5/3pp3/3P4/P7/1PP2PPP/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pp2pp1p/3p2p1/8/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pp2pppp/3p4/2p5/4P3/2P2N2/PP1P1PPP/RNBQKB1R b KQkq - pm Nf6; id Neutral.387;",
    "rnbqkbnr/pp2pppp/8/2p5/2Pp4/5NP1/PP1PPP1P/RNBQKB1R w KQkq - pm Bg2; id Neutral.388;",
    "rnbqkbnr/pp3ppp/2p1p3/8/2PPp3/2N5/PP3PPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pp3ppp/2p5/3p4/3P4/2N5/PP2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pp3ppp/2p5/4P3/2Pp4/2N5/PP2PPPP/R1BQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pp3ppp/3p4/2pP4/8/8/PP2PPPP/RNBQKBNR w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pp3ppp/3pp3/8/3pP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/ppp1pppp/8/3p4/3P4/2N5/PPP1PPPP/R1BQKBNR b KQkq - pm Nf6; id Neutral.389;",
    "rnbqkbnr/ppp1pppp/8/3p4/5P2/5N2/PPPPP1PP/RNBQKB1R b KQkq - pm Nf6; id Neutral.390;",
    "rnbqkbnr/ppp1pppp/8/3p4/5P2/8/PPPPP1PP/RNBQKBNR w KQkq - pm Nf3; id Neutral.391;",
    "rnbqkbnr/ppp1pppp/8/3p4/8/5NP1/PPPPPP1P/RNBQKB1R b KQkq - pm Nf6; id Neutral.392;",
    "rnbqkbnr/ppp1pppp/8/8/2pPP3/8/PP3PPP/RNBQKBNR b KQkq - pm e5; id Neutral.393;",
    "rnbqkbnr/ppp2ppp/3p4/4p3/2PP4/2N5/PP2PPPP/R1BQKBNR b KQkq - pm exd4; id Neutral.394;",
    "rnbqkbnr/ppp2ppp/4p3/3p4/2P5/1P3N2/P2PPPPP/RNBQKB1R b KQkq - pm Nf6; id Neutral.395;",
    "rnbqkbnr/ppp2ppp/4p3/3p4/3P1B2/5N2/PPP1PPPP/RN1QKB1R b KQkq - pm Nf6; id Neutral.396;",
    "rnbqkbnr/ppp2ppp/8/8/2ppP3/5N2/PP3PPP/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pppp1p1p/8/8/4PppP/5N2/PPPP2P1/RNBQKB1R w KQkq - fmvn 5; hmvc 0;",
    "rnbqkbnr/pppp1ppp/8/4p3/2P5/2N5/PP1PPPPP/R1BQKBNR b KQkq - pm Nf6; id Neutral.397;",
    "rnbqkbnr/pppp2pp/8/4pp2/2PP4/6P1/PP2PP1P/RNBQKBNR b KQkq - pm exd4; id Neutral.398;",
    "rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - pm d5; id Neutral.399;",
    "rq2kb1r/1b1n1ppp/p3pn2/1ppP4/8/1BN1PN2/PP2QPPP/R1BR2K1 b kq - pm c4; id Neutral.400;"    
};

SelfGame::SelfGame(Console* c, const Parameters& wp, const Parameters& bp)
{
    wrb = new RootBoard(c, wp, 0x200000, 0x10000);
    brb = new RootBoard(c, bp, 0x200000, 0x10000);

    decisiveScore = 700;
    decisiveScoreMoves = 4;
    drawScoreMoves = 6;
}

SelfGame::~SelfGame() {
    delete wrb;
    delete brb;
}

int SelfGame::nTests() {
    return 2*testPosition.size();
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
    p["infinite"] = StringList();
    p["nodes"] = StringList() << "16000";
    rb->goReadParam(p);
}

int SelfGame::doGame(RootBoard* rb1, RootBoard* rb2)
{
//     return floor((rand()*3.0)/(RAND_MAX+1.0))-1;
    setupRootBoard(rb1);
    setupRootBoard(rb2);
    nDecisive = 0;
    nDraw = 0;
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
#ifdef __x86_64__
    std::chrono::nanoseconds t = std::chrono::system_clock::now() - start;
    start = std::chrono::system_clock::now();
    return t.count();
#endif    
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

    if (abs(rb.getScore()) == 0) {
        nDraw++;
        if (nDraw >= drawScoreMoves) {
            result = 0;
            return true;
        }
    } else
        nDraw = 0;
    
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
//     Options::quiet = false;
    for (unsigned i=0; i<testPosition.size(); ++i) {
//         std::cerr << "Game " << i << std::endl;
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

