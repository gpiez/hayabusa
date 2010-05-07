#include <pch.h>
#include "rootboard.h"
#include "rootboard.tcc"
#include "workthread.h"
#include "console.h"
#include "boardbase.tcc"
#include "jobs.h"
#include "transpositiontable.h"
#include "transpositiontable.tcc"
#ifdef QT_GUI_LIB
#include "statwidget.h"
#endif

RootBoard::RootBoard(Console *c):
	
	timeBudget(300000),
	movesToDo(40),
	console(c)
{
	initWorkThreads();
	tt = new TranspositionTable<TTEntry, transpositionTableAssoc>;
	#ifdef QT_GUI_LIB
	statWidget = new StatWidget(*this);
	statWidget->show();
	#endif
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

template<>
const ColoredBoard<White>& RootBoard::currentBoard<White>() const {
	return boards[iMove].wb;
}

template<>
const ColoredBoard<Black>& RootBoard::currentBoard<Black>() const {
	return boards[iMove].bb;
}

template<>
ColoredBoard<White>& RootBoard::currentBoard<White>() {
	return boards[iMove].wb;
}

template<>
ColoredBoard<Black>& RootBoard::currentBoard<Black>() {
	return boards[iMove].bb;
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
			board.setPiece<Black>( King, square);
			break;
		case 'p':
			board.setPiece<Black>( Pawn, square);
			break;
		case 'b':
			board.setPiece<Black>( Bishop, square);
			break;
		case 'n':
			board.setPiece<Black>( Knight, square);
			break;
		case 'r':
			board.setPiece<Black>( Rook, square);
			break;
		case 'q':
			board.setPiece<Black>( Queen, square);
			break;
		case 'K':
			board.setPiece<White>( King, square);
			break;
		case 'P':
			board.setPiece<White>( Pawn, square);
			break;
		case 'B':
			board.setPiece<White>( Bishop, square);
			break;
		case 'N':
			board.setPiece<White>( Knight, square);
			break;
		case 'R':
			board.setPiece<White>( Rook, square);
			break;
		case 'Q':
			board.setPiece<White>( Queen, square);
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
			board.castling[0].q = true;
			break;
		case 'K':
			board.castling[0].k = true;
			break;
		case 'q':
			board.castling[1].q = true;
			break;
		case 'k':
			board.castling[1].k = true;
			break;
		}

	if ( enPassant.length() == 2 && enPassant[0] >= 'a' && enPassant[0] <= 'h' && ( enPassant[1] == '3' || enPassant[1] == '6' ) )
		board.enPassant = 8 * ((enPassant[1].toLatin1() - '3' )/3 + 3) + enPassant[0].toLatin1() - 'a';
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

void RootBoard::divide(unsigned int depth) {
	if (color == White)
		threads.first()->startJob(new RootDivideJob<White>(this, depth));
	else
		threads.first()->startJob(new RootDivideJob<Black>(this, depth));
}

WorkThread* RootBoard::findFreeThread() {
//	QMutexLocker locker(&threadsLock);
	foreach(WorkThread* th, threads) 
		if (th->isFree())
			return th;
	return 0;
}

void updateAndReady(uint64_t& r, uint64_t v) {
	r += v;
}
void updateAndReady(Result<uint64_t>& r, uint64_t v) {
	r.update(v);
	r.setReady();
}
void updateAndReady(Result<uint64_t>& r, Result<uint64_t>& v) {
	r.update(v);
	r.setReady();
}

template<> void setNotReady<Result<uint64_t> >(Result<uint64_t>& r) {
	r.setNotReady();
}
