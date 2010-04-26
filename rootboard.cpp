#include <pch.h>
#include "rootboard.h"
#include "workthread.h"
#include "console.h"
#include "boardbase.tcc"
//#include "rootboard.tcc"
#include "jobs.h"
#include "transpositiontable.h"
#include "transpositiontable.tcc"

RootBoard::RootBoard(Console *c):
console(c)
{
	initWorkThreads();
	tt = new TranspositionTable<TTEntry, transpositionTableAssoc>;
}

void RootBoard::initWorkThreads() {
	numThreads = sysconf(_SC_NPROCESSORS_ONLN);
	if (numThreads == 0) numThreads = 1;
	allocateWorkThreads(numThreads);
}

void RootBoard::allocateWorkThreads(unsigned int numThreads) {
	unsigned int i=0;
	foreach(WorkThread* th, threads) {
		if (++i > numThreads) {
			th->stop();
			th->wait();
			delete th;
		}
	}

	while (++i <= numThreads) {
		WorkThread* th = new WorkThread();
		th->start();
		threads.append(th);
	}

}

void RootBoard::stop() {
	foreach(WorkThread* th, threads) {
		th->stop();
	}
}

const BoardBase* RootBoard::setup(QString fen) {

	QString piecePlacement, activeColor, castling, enPassant;
	QTextStream str(&fen);
	str >> piecePlacement >> activeColor >> castling >> enPassant;
	
	BoardBase* board;
	switch ( activeColor[0].toLatin1() ) {
	case 'b':
	case 'B':
		color = Black;
		board = & boards[0].bb;
		break;
	default:
		qWarning() << "color to move not understood, assuming white";
	case 'w':
	case 'W':
		color = White;
		board = & boards[0].wb;
		break;
	}

	board->init();
	str >> board->fiftyMoves >> iMove;

	unsigned int p,x,y;
	for ( p=0, x=0, y=7; p<(unsigned int)piecePlacement.length(); p++, x++ ) {
		unsigned int square = x + 8*y;
		switch ( piecePlacement[p].toLatin1() ) {
		case '1'...'8':
			x += piecePlacement[p].toLatin1() - '1';
			break;
		case 'k':
			board->setPiece<Black>( King, square);
			break;
		case 'p':
			board->setPiece<Black>( Pawn, square);
			break;
		case 'b':
			board->setPiece<Black>( Bishop, square);
			break;
		case 'n':
			board->setPiece<Black>( Knight, square);
			break;
		case 'r':
			board->setPiece<Black>( Rook, square);
			break;
		case 'q':
			board->setPiece<Black>( Queen, square);
			break;
		case 'K':
			board->setPiece<White>( King, square);
			break;
		case 'P':
			board->setPiece<White>( Pawn, square);
			break;
		case 'B':
			board->setPiece<White>( Bishop, square);
			break;
		case 'N':
			board->setPiece<White>( Knight, square);
			break;
		case 'R':
			board->setPiece<White>( Rook, square);
			break;
		case 'Q':
			board->setPiece<White>( Queen, square);
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
			board->castling[0].q = true;
			break;
		case 'K':
			board->castling[0].k = true;
			break;
		case 'q':
			board->castling[1].q = true;
			break;
		case 'k':
			board->castling[1].k = true;
			break;
		}

	if ( enPassant.length() == 2 && enPassant[0] >= 'a' && enPassant[0] <= 'h' && ( enPassant[1] == '3' || enPassant[1] == '6' ) )
		board->enPassant = 8 * ((enPassant[1].toLatin1() - '3' )/3 + 3) + enPassant[0].toLatin1() - 'a';
	else if ( enPassant != "-" )
		qWarning() << "error importing e. p. move " << enPassant;

	return board;
}

void RootBoard::go(QStringList )
{
	Job* job;
	if (color == White)
		job = new RootSearchJob<White>(this);
	else
		job = new RootSearchJob<Black>(this);
	threads.first()->startJob(job);
}

void RootBoard::perft(unsigned int depth) {
	if (color == White)
		threads.first()->startJob(new RootPerftJob<White>(this, depth));
	else
		threads.first()->startJob(new RootPerftJob<Black>(this, depth));
}

/*uint64_t RootBoard::perft2(unsigned int depth) const {
	Result<uint64_t> n=0;
	if (color == White)
		perft<White>(&n, &boards[iMove].wb, depth);
	else
		perft<Black>(&n, &boards[iMove].bb, depth);
	return n.get();
}
*/
void RootBoard::divide(unsigned int depth) const {
	if (color == White)
		divide<White>(&boards[iMove].wb, depth);
	else
		divide<Black>(&boards[iMove].bb, depth);
}

template<>
const ColoredBoard<White>* RootBoard::currentBoard<White>() const {
	return &boards[iMove].wb;
}

template<>
const ColoredBoard<Black>* RootBoard::currentBoard<Black>() const {
	return &boards[iMove].bb;
}

WorkThread* RootBoard::findFreeThread() {
	QMutexLocker locker(&threadsLock);
	foreach(WorkThread* th, threads) 
		if (th->isFree())
			return th;
	return 0;
}
