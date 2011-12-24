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
#include "rootsearch.tcc"
#include "jobs.tcc"
#include "score.tcc"
#include "transpositiontable.tcc"
#include "book.h"

void RootBoard::stopTimer(milliseconds hardlimit) {
    UniqueLock<TimedMutex> lock(stopTimerMutex, hardlimit);
/*#ifdef __WIN32__    
    Sleep(hardlimit.count()*1000);
#else
    timespec ts = { hardlimit.count()/1000, (hardlimit.count()%1000)*1000000 };
    nanosleep(&ts, NULL);
#endif    */
    stopSearch = Stopping;
}

std::string RootBoard::commonStatus() const {
    std::stringstream g;
    g.imbue(std::locale("de_DE"));
    g << "D" << std::setfill('0') << std::setw(2) << depth-(eval.dMaxExt)
        << " " << std::setfill(' ') << std::fixed << std::setprecision(1) << std::showpoint << std::setw(9) << duration_cast<milliseconds>(system_clock::now()-start).count()/1000.0
        << "s " << std::setw(13) << getStats().node
        << " ";
    return g.str();
}

void RootBoard::infoTimer(milliseconds repeat) {
    static int lastCurrentMoveIndex = 0;
    while(!infoTimerMutex.try_lock()) {
        UniqueLock<TimedMutex> lock(infoTimerMutex, repeat);
        Stats tempStats = getStats();
        uint64_t ntemp = tempStats.node;
        uint64_t t = duration_cast<milliseconds>(system_clock::now()-start).count();
        std::stringstream g;
        if (!Options::humanreadable && !Options::quiet && (currentMoveIndex != lastCurrentMoveIndex)) {
            g << "info currmove " << currentMove.algebraic() << " currmovenumber " << currentMoveIndex << std::endl;
            lastCurrentMoveIndex = currentMoveIndex;
        }
        if (Options::currline) {
            g << "info currline " << getLine() << std::endl;
        }
        if (Options::humanreadable) {
            g << commonStatus() 
            << std::fixed << std::setprecision(2) << std::setw(5) << getStats().node/(t*1000+1.0) << "/s "
            << getLine();
            std::string str = g.str();
            str.resize(80,' ');
            std::cout << str << "\015" << std::flush;
        } else {
            g   << "info"
                << " nodes " << ntemp
                << " nps " << (1000*ntemp)/(t+1)
                << " hashfull " << (2000*tempStats.ttuse+1)/(2*tt->getEntries())
                ;
            console->send(g.str());
#ifdef MYDEBUG
#if 0        
        g << std::endl;
        g << "info string ";
        g << " bmob1 " << eval.bmob1 / (eval.bmobn+0.1);
        g << " bmob2 " << eval.bmob2 / (eval.bmobn+0.1);
        g << " bmob3 " << eval.bmob3 / (eval.bmobn+0.1);
#endif        
#endif            
        }
    }
    infoTimerMutex.unlock();
}

RootBoard::RootBoard(Console *c, const Parameters& p, uint64_t hashSize, uint64_t phashSize):
    iMove(0),
    currentPly(0),
    wtime(300000),
    btime(300000),
    winc(5000),
    binc(5000),
    movestogo(0),
    stopSearch(Stopped),
    eval(phashSize, p),
    console(c),
    color(White)
{
//     eval.setParameters(p);
    int null = 2;
    int verify = 2;
    unsigned nullIncr = eval.dNullIncr;
    unsigned verifyIncr = eval.dVerifyIncr;
    for (unsigned i=eval.dMaxExt+2; i<=maxDepth; ++i) {
        while (nullIncr & 1) {
            null++;
            nullIncr >>= 1;
        }
        nullReduction[i] = i-null;
        nullIncr >>= 1;
        
        while (verifyIncr & 1) {
            verify++;
            verifyIncr >>= 1;
        }
        verifyReduction[i] = i-verify;
        verifyIncr >>= 1;
#ifdef MYDEBUG
        if (i>eval.dMaxExt) std::cout << std::setw(2) << i-eval.dMaxExt << ": " << nullReduction[i]-eval.dMaxExt << std::endl;
#endif
    }
    tt = new TranspositionTable<TTEntry, transpositionTableAssoc, Key>(hashSize);
    clearEE();
    boards[0].wb.init();
#ifdef QT_GUI_LIB
    statWidget = new StatWidget(*this);
    statWidget->show();
#endif
}

RootBoard::~RootBoard() {
    delete tt;
}

std::string RootBoard::status(system_clock::time_point now, int score)
{
    unsigned d = depth - eval.dMaxExt;
    std::stringstream g;
    milliseconds t = duration_cast<milliseconds>(now-start);
    if (Options::humanreadable) {
        g << commonStatus() 
            << Score<White>::str(score) << " " << tt->bestLine(*this);
    } else {
        g   << "info depth " << d << " time " << t.count()
            << " nodes " << getStats().node 
            << " pv " << tt->bestLine(*this)
            << " score cp " << score*color
            << " nps " << (1000*getStats().node)/(t.count()+1)
            ;
    }
#ifdef QT_GUI_LIB
    emit(signalInfo(d, (uint64_t)(now-start).count(), getStats().node, QString::fromUtf8(Score<White>::str(score).c_str()), QString::fromStdString(tt->bestLine(*this))));
#endif
    return g.str();
}

void RootBoard::clearEE() {
    for (int p=-(int)nPieces; p<=(int)nPieces; ++p)
        for (unsigned int sq1=0; sq1<nSquares; ++sq1)
        for (unsigned int sq=0; sq<nSquares; ++sq) {
            PositionalError& pe = this->pe[p+nPieces][sq1][sq];
            pe.v = 0;
            pe.error = eval.standardError;
#ifdef CALCULATE_MEAN_POSITIONAL_ERROR
            pe.n = 0.0;
            pe.e = 0.0;
            pe.e2 = 0.0;
#endif            
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
ColoredBoard<Black>& RootBoard::nextBoard<White>() {
    return boards[iMove].bb;
}

template<>
ColoredBoard<White>& RootBoard::nextBoard<Black>() {
    return boards[iMove+1].wb;
}

const BoardBase& RootBoard::currentBoard() const {
    if (color == White)
        return currentBoard<White>();
    else
        return currentBoard<Black>();
}

const BoardBase& RootBoard::setup(const std::string& str) {
    static const StringList tokens = StringList() << "fen" << "bm" << "am" << "id" << "pm";
    auto tl = split("fen " + str, "; ").parse(tokens);
    if (tl.find("fen") == tl.end()) return currentBoard();
    info = tl["id"].join(" ");
    std::string piecePlacement, activeColor, castling, enPassant;
    int fiftyTemp = 0;
    iMove = 0;
//    std::cout << tl["fen"].join(" ") << std::endl;
    std::istringstream ss(tl["fen"].join(" "));
    ss >> piecePlacement >> activeColor >> castling >> enPassant >> fiftyTemp >> iMove;

    switch ( activeColor[0] ) {
    case 'b':
    case 'B':
        color = Black;
        break;
    default:
        std::cerr << "color to move not understood, assuming white" << std::endl;
    case 'w':
    case 'W':
        color = White;
        break;
    }

    BoardBase& board = color == White ? (BoardBase&)currentBoard<White>() : (BoardBase&)currentBoard<Black>();

    board.init();
    board.fiftyMoves = fiftyTemp;
    //clearEE();

    unsigned int p,x,y;
    int wp = 0, wn = 0, wb = 0, wr = 0, wq = 0, wk = 0;
    int bp = 0, bn = 0, bb = 0, br = 0, bq = 0, bk = 0;
    for ( p=0, x=0, y=7; p<(unsigned int)piecePlacement.length(); p++, x++ ) {
        unsigned int square = x + 8*y;
        switch ( piecePlacement[p] ) {
        case '1'...'8':
            x += piecePlacement[p] - '1';
            break;
        case 'k':
            board.setPiece<Black>( King, square, eval);
            bk++;
            break;
        case 'p':
            if (y==0 || y==7) {
                std::cerr << "Illegal pawn position" << std::endl;
            }
            board.setPiece<Black>( Pawn, square, eval);
            bp++;
            break;
        case 'b':
            board.setPiece<Black>( Bishop, square, eval);
            bb++;
            break;
        case 'n':
            board.setPiece<Black>( Knight, square, eval);
            bn++;
            break;
        case 'r':
            board.setPiece<Black>( Rook, square, eval);
            br++;
            break;
        case 'q':
            board.setPiece<Black>( Queen, square, eval);
            bq++;
            break;
        case 'K':
            board.setPiece<White>( King, square, eval);
            wk++;
            break;
        case 'P':
            if (y==0 || y==7) {
                std::cerr << "Illegal pawn position" << std::endl;
            }
            board.setPiece<White>( Pawn, square, eval);
            wp++;
            break;
        case 'B':
            board.setPiece<White>( Bishop, square, eval);
            wb++;
            break;
        case 'N':
            board.setPiece<White>( Knight, square, eval);
            wn++;
            break;
        case 'R':
            board.setPiece<White>( Rook, square, eval);
            wr++;
            break;
        case 'Q':
            board.setPiece<White>( Queen, square, eval);
            wq++;
            break;
        case '/':
            x = -1;
            y--;
            break;
        default:
            std::cerr << "Unknown char " << piecePlacement[p] << " in FEN import";
        }
    }

    for ( unsigned int p=0; p<(unsigned int)castling.length(); p++ )
        switch ( castling[p] ) {
        case 'Q':
            if (board.getPieces<White, King>() & 1ULL<<e1 && board.getPieces<White, Rook>() & 1ULL<<a1)
                board.cep.castling.color[0].q = true;
            else
                std::cerr << "Illegal white castling" << std::endl;
            break;
        case 'K':
            if (board.getPieces<White, King>() & 1ULL<<e1 && board.getPieces<White, Rook>() & 1ULL<<h1)
                board.cep.castling.color[0].k = true;
            else
                std::cerr << "Illegal white castling" << std::endl;
            break;
        case 'q':
            if (board.getPieces<Black, King>() & 1ULL<<e8 && board.getPieces<Black, Rook>() & 1ULL<<a8)
                board.cep.castling.color[1].q = true;
            else
                std::cerr << "Illegal black castling" << std::endl;
            break;
        case 'k':
            if (board.getPieces<Black, King>() & 1ULL<<e8 && board.getPieces<Black, Rook>() & 1ULL<<h8)
                board.cep.castling.color[1].k = true;
            else
                std::cerr << "Illegal black castling" << std::endl;
            break;
        }

    if ( enPassant.length() == 2 && enPassant[0] >= 'a' && enPassant[0] <= 'h' && ( enPassant[1] == '3' || enPassant[1] == '6' ) )
        board.cep.enPassant = 1ULL << (8 * ((enPassant[1] - '3' )/3 + 3) + enPassant[0] - 'a');
    else if ( enPassant != "-" )
        std::cerr << "error importing e. p. move " << enPassant << std::endl;

    board.buildAttacks();
    if (board.inCheck<White>() && color == Black)
        std::cerr << "White king in check and black to move" << std::endl;
    if (board.inCheck<Black>() && color == White)
        std::cerr << "White king in check and black to move" << std::endl;

    if (wp > 8 || bp > 8)
        std::cerr << "To many pawns" << std::endl;

    if (!wk || !bk)
        std::cerr << "No King" << std::endl;

    if (std::max(wq,1)-1 + std::max(wr,2)-2 + std::max(wb,2)-2 + std::max(wn,2)-2 + wp > 8)
        std::cerr << "To many promoted white pieces" << std::endl;
    
    if (std::max(bq,1)-1 + std::max(br,2)-2 + std::max(bb,2)-2 + std::max(bn,2)-2 + bp > 8)
        std::cerr << "To many promoted black pieces" << std::endl;
        
    if (tl.find("bm") != tl.end()) {
        Move m = findMove(tl["bm"].front());
        if (m.data) {
            std::cerr << "Move " << tl["bm"].front() << " not recognized" << std::endl;
        }
        bm = m;
    }
    if (getStats().ttuse) tt->agex();
    
    rootPly = 0;
    board.positionalScore = eval(board, color);
    board.isExact = true;
    return board;
}

Move RootBoard::findMove(const std::string& ) const {
    return Move(0,0,0);
}


void RootBoard::go(const std::map<std::string, StringList>& param )
{
    goReadParam(param);
    goExecute();
}

void RootBoard::goReadParam(const std::map<std::string, StringList>& param )
{
    infinite = param.count("infinite");

    if (param.find("wtime") != param.end())
        wtime = convert((*param.find("wtime")).second.front());

    if (param.find("btime") != param.end())
        btime = convert((*param.find("btime")).second.front());

    if (param.find("winc") != param.end())
        winc = convert((*param.find("winc")).second.front());

    if (param.find("binc") != param.end())
        binc = convert((*param.find("binc")).second.front());

    if (param.find("movestogo") != param.end())
        movestogo = convert((*param.find("movestogo")).second.front());
    else
        movestogo = 0;

    if (param.find("depth") != param.end())
        maxSearchDepth = convert((*param.find("depth")).second.front());
    else
        maxSearchDepth = maxDepth;

    if (param.find("nodes") != param.end())
        maxSearchNodes = convert<uint64_t>((*param.find("nodes")).second.front());
    else
        maxSearchNodes = ~0ULL;

    if (param.find("mate") != param.end())
        mate = convert((*param.find("mate")).second.front());
    else
        mate = 0;

    if (param.find("movetime") != param.end())
        movetime = convert((*param.find("movetime")).second.front());
    else
        movetime = 0;
}

void RootBoard::goExecute()
{
    Job* job;
    if (color == White)
        job = new RootSearchJob<White>(*this, keys, maxSearchDepth);
    else
        job = new RootSearchJob<Black>(*this, keys, maxSearchDepth);
    WorkThread::findFree()->queueJob(0U, job);
}

void RootBoard::stop() {
    UniqueLock<Mutex> l(stopSearchMutex);
    if (stopSearch == Running) stopSearch = Stopping;
    while (stopSearch != Stopped)
        stoppedCond.wait(l);
}

void RootBoard::perft(unsigned int depth) {
    if (color == White)
        WorkThread::findFree()->queueJob(0U, new RootPerftJob<White>(*this, depth));
    else
        WorkThread::findFree()->queueJob(0U, new RootPerftJob<Black>(*this, depth));
}

void RootBoard::divide(unsigned int depth) {
    if (color == White)
        WorkThread::findFree()->queueJob(0U, new RootDivideJob<White>(*this, depth));
    else
        WorkThread::findFree()->queueJob(0U, new RootDivideJob<Black>(*this, depth));
}

void update(uint64_t& r, uint64_t v) {
    r += v;
}

void update(Result<uint64_t>& r, uint64_t v) {
    r.update(v);
}

Stats RootBoard::getStats() const {
    auto& threads = WorkThread::getThreads();
    Stats sum = * (* threads.begin() )->getStats();
    for (auto th = threads.begin() + 1; th !=threads.end(); ++th) {
        for (unsigned int i=0; i<sizeof(Stats)/sizeof(uint64_t); ++i)
            sum.data[i] += (*th)->getStats()->data[i];
    }
    return sum;
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

bool RootBoard::doMove(Move m) {
    if (color == White)
        return doMove<White>(m);
    else
        return doMove<Black>(m);
}
// returns true on error
template<Colors C>
bool RootBoard::doMove(Move m) {
    WorkThread::stopAll();
    MoveList ml(currentBoard<C>());

    for (ml.begin(); ml.isValid(); ++ml) {
        if ((*ml).fromto() == m.fromto()) {
            if (m.piece() == 0 || m.piece() == (*ml).piece()) {
                const ColoredBoard<C>& cb = currentBoard<C>();
                store(cb.getZobrist(), 0);
                ColoredBoard<(Colors)-C>* next = &nextBoard<C>();
                cb.copyPieces(*next);
                cb.doMove(next, *ml);
                next->keyScore.vector = eval.estimate<C>(*ml, cb.keyScore);
                next->buildAttacks();

                if (C == Black) iMove++;
                color = (Colors)-C;
                rootPly++;
                return false;
            }
        }
    }
    return true;
}

std::string RootBoard::getInfo() const {
    return info;
}

void RootBoard::clearHash()
{
    tt->clear();        //FIXME needs locking
    eval.ptClear();
    history.init();
    clearEE();
}

void RootBoard::openBook(std::string bstr)
{
    book.read(bstr);
}

void RootBoard::resetBook(std::string bstr)
{
    book.read(bstr);
    book.resetWeights();
    book.write(bstr);
}

int score2tt(int s) {
    return inline_score2tt(s);
}

int tt2Score(int s) {
    return inline_tt2Score(s);
}

void RootBoard::setTime(uint64_t w, uint64_t b)
{
    wtime = w / 1000000;
    btime = b / 1000000;
}

void RootBoard::goWait()
{
    Job* job;
    if (color == White)
        job = new RootSearchJob<White>(*this, keys, maxSearchDepth);
    else
        job = new RootSearchJob<Black>(*this, keys, maxSearchDepth);
    job->job();
    delete job;
}
