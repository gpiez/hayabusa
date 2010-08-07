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
#include <pch.h>
#include "rootboard.h"
#include "rootboard.tcc"
#include "workthread.h"
#include "console.h"
#include "boardbase.tcc"
#include "jobs.h"
#include "transpositiontable.h"
#include "transpositiontable.tcc"
#include "history.cpp"
#ifdef QT_GUI_LIB
#include "statwidget.h"
#endif

__thread int RootBoard::lastPositionalEval;

/*
__thread unsigned RootBoard::ply;
__thread RootBoard::RepetitionKeys RootBoard::keys;
*/
__thread RepetitionKeys keys;

RootBoard::RootBoard(Console *c):
	timeBudget(300000),
	movesToDo(40),
	iMove(0),
	currentPly(0),
	console(c),
	color(White)
{
	tt = new TranspositionTable<TTEntry, transpositionTableAssoc, Key>;
    for (int p=-nPieces; p<=(int)nPieces; ++p)
        for (unsigned int sq=0; sq<nSquares; ++sq) {
            estimatedError[p+nPieces][sq] = initialError;
            avgE[p+nPieces][sq] = 0.0;
            avgN[p+nPieces][sq] = 0.001;
        }

    boards[0].wb.init();
	#ifdef QT_GUI_LIB
    statWidget = new StatWidget(*this);
	statWidget->show();
	#endif
}

template<>
const ColoredBoard<White>& RootBoard::currentBoard<White>() const {
	return boards[iMove].wb;
}

template<>
const ColoredBoard<Black>& RootBoard::currentBoard<Black>() const {
	return boards[iMove].bb;
}

const BoardBase& RootBoard::currentBoard() const {
    if (color == White)
        return currentBoard<White>();
    else
        return currentBoard<Black>();
}

const BoardBase& RootBoard::setup(QString fen) {

	QString piecePlacement, activeColor, castling, enPassant;
	int fiftyTemp;
	QTextStream str(&fen);
	str >> piecePlacement >> activeColor >> castling >> enPassant >> fiftyTemp >> iMove;
	
	switch ( activeColor[0].toLatin1() ) {
	case 'b':
	case 'B':
		color = Black;
		break;
	default:
		qWarning() << "color to move not understood, assuming white";
	case 'w':
	case 'W':
		color = White;
		break;
	}
	
	BoardBase& board = color == White ? (BoardBase&)currentBoard<White>() : (BoardBase&)currentBoard<Black>();

	board.init();
	board.fiftyMoves = fiftyTemp;
	
	unsigned int p,x,y;
	for ( p=0, x=0, y=7; p<(unsigned int)piecePlacement.length(); p++, x++ ) {
		unsigned int square = x + 8*y;
		switch ( piecePlacement[p].toLatin1() ) {
		case '1'...'8':
			x += piecePlacement[p].toLatin1() - '1';
			break;
		case 'k':
			board.setPiece<Black>( King, square, eval);
			break;
		case 'p':
			board.setPiece<Black>( Pawn, square, eval);
			break;
		case 'b':
			board.setPiece<Black>( Bishop, square, eval);
			break;
		case 'n':
			board.setPiece<Black>( Knight, square, eval);
			break;
		case 'r':
			board.setPiece<Black>( Rook, square, eval);
			break;
		case 'q':
			board.setPiece<Black>( Queen, square, eval);
			break;
		case 'K':
			board.setPiece<White>( King, square, eval);
			break;
		case 'P':
			board.setPiece<White>( Pawn, square, eval);
			break;
		case 'B':
			board.setPiece<White>( Bishop, square, eval);
			break;
		case 'N':
			board.setPiece<White>( Knight, square, eval);
			break;
		case 'R':
			board.setPiece<White>( Rook, square, eval);
			break;
		case 'Q':
			board.setPiece<White>( Queen, square, eval);
			break;
		case '/':
			x = -1;
			y--;
			break;
		default:
			qWarning() << "Unknown char " << piecePlacement[p] << " in FEN import";
		}
	}

	for ( unsigned int p=0; p<(unsigned int)castling.length(); p++ )
		switch ( castling[p].toLatin1() ) {
		case 'Q':
		    if (board.getPieces<White, King>() & 1ULL<<e1 && board.getPieces<White, Rook>() & 1ULL<<a1)
		        board.cep.castling.color[0].q = true;
			break;
		case 'K':
            if (board.getPieces<White, King>() & 1ULL<<e1 && board.getPieces<White, Rook>() & 1ULL<<h1)
                board.cep.castling.color[0].k = true;
			break;
		case 'q':
		    if (board.getPieces<Black, King>() & 1ULL<<e8 && board.getPieces<Black, Rook>() & 1ULL<<a8)
		        board.cep.castling.color[1].q = true;
			break;
		case 'k':
            if (board.getPieces<Black, King>() & 1ULL<<e8 && board.getPieces<Black, Rook>() & 1ULL<<h8)
                board.cep.castling.color[1].k = true;
			break;
		}

	if ( enPassant.length() == 2 && enPassant[0] >= 'a' && enPassant[0] <= 'h' && ( enPassant[1] == '3' || enPassant[1] == '6' ) )
		board.cep.enPassant = 1ULL << (8 * ((enPassant[1].toLatin1() - '3' )/3 + 3) + enPassant[0].toLatin1() - 'a');
	else if ( enPassant != "-" )
		qWarning() << "error importing e. p. move " << enPassant;

	board.buildAttacks();
	history.init();
	return board;
}

void RootBoard::go(QStringList )
{
	Job* job;
	if (color == White)
		job = new RootSearchJob<White>(*this);
	else
		job = new RootSearchJob<Black>(*this);
	WorkThread::findFree()->queueJob(0, job);
}

void RootBoard::perft(unsigned int depth) {
	if (color == White)
		WorkThread::findFree()->queueJob(0, new RootPerftJob<White>(*this, depth));
	else
		WorkThread::findFree()->queueJob(0, new RootPerftJob<Black>(*this, depth));
}

void RootBoard::divide(unsigned int depth) {
	if (color == White)
		WorkThread::findFree()->queueJob(0, new RootDivideJob<White>(*this, depth));
	else
		WorkThread::findFree()->queueJob(0, new RootDivideJob<Black>(*this, depth));
}

void update(uint64_t& r, uint64_t v) {
	r += v;
}

void update(Result<uint64_t>& r, uint64_t v) {
	r.update(v);
}

uint64_t RootBoard::getTime() const {
	QTime temp = startTime.time();
	return temp.msecsTo(QTime::currentTime()) +
	86400000*(startTime.daysTo(QDateTime::currentDateTime()));
}

Stats RootBoard::getStats() const {
	Stats sum = {{0}};
	foreach(WorkThread* th, WorkThread::getThreads()) {
		for (unsigned int i=0; i<sizeof(Stats)/sizeof(uint64_t); ++i)
			sum.data[i] += th->pstats->data[i];
	}
	return sum;
}

void RootBoard::ttClear()
{
	//TODO implement me
}

std::string RootBoard::getLine() const {
    std::stringstream ss;
    ss << std::setw(2) << currentMoveIndex << "/" << std::setw(2) << nMoves << " ";
    std::string temp(ss.str());
    for (unsigned i=1; i<=currentPly; ++i) {
        temp += line[i].string() + " ";
    }
    return temp;
}

// returns true on error
bool RootBoard::doMove(Move m) {
	WorkThread::stopAll();
	Move list[maxMoves];
	Move* good = list+goodMoves;
	Move* bad = good;
	if (color == White)
		currentBoard<White>().generateMoves(good, bad);
	else
		currentBoard<Black>().generateMoves(good, bad);

	for (Move* i=good; i<bad; ++i) {
		if (i->fromto() == m.fromto()) {
			if (m.piece() == 0 || m.piece() == i->piece()) {
				if (color == White) {
					color = Black;
					ColoredBoard<Black>* next = &boards[iMove].bb;
					currentBoard<White>().doMove(next, m);
				} else {
					color = White;
					ColoredBoard<White>* next = &boards[++iMove].wb;
					currentBoard<Black>().doMove(next, m);
				}
				return false;
			}
		}
	}
	return true;
}
