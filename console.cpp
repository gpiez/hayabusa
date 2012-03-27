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

#include "console.h"
#include "board.h"
#include "eval.h"
#include "workthread.h"
#include "parameters.h"
#include "game.h"
#include "evolution.h"
#include "testpositions.h"
#include "options.h"

#include <unistd.h>
#include <future>
#include <sstream>

namespace Options {
unsigned int        splitDepth = 777;
int                 humanreadable = 0;
uint64_t            hash = 0x1000000;
uint64_t            pHash = 0x1000000;
bool                quiet = false;
bool                preCutIfNotThreatened = false;
bool                reduction = true;
bool                pruning = true;
unsigned            debug = 0;
bool                currline = false;
#ifdef QT_NETWORK_LIB
bool                server = false;
#endif
}

Console console;

void Console::init(int& argc, char** argv)
{
#if defined(QT_GUI_LIB)
    app = new QApplication(argc, argv);
#endif
    new Thread(&Console::outputThread, this);
    args.resize(argc);
    std::copy(argv+1, argv+argc, args.begin());
    if (args[0] == "debug") {
        StringList debugParm;
        for (int i=1; i<argc; ++i)
            debugParm << args[i].c_str();
        debug(debugParm); }
//     Options::debug = debugEval;
    Board::initTables();
    Eval::initTables();
    WorkThread::init();
    Parameters::init();
#ifdef USE_GENETIC
    evolution = new Evolution(this);
    evolution->init();
#endif
    game = new Game(this, defaultParameters, Options::hash, Options::pHash);
    game->setup();

    dispatcher["uci"] = &Console::uci;
    dispatcher["debug"] = &Console::debug;
    dispatcher["isready"] = &Console::isready;
    dispatcher["setoption"] = &Console::setoption;
    dispatcher["register"] = &Console::reg;
    dispatcher["ucinewgame"] = &Console::ucinewgame;
    dispatcher["position"] = &Console::position;
    dispatcher["go"] = &Console::go;
    dispatcher["stop"] = &Console::stop;
    dispatcher["ponderhit"] = &Console::ponderhit;
    dispatcher["quit"] = &Console::quit;

    dispatcher["perft"] = &Console::perft;
    dispatcher["divide"] = &Console::divide;
    dispatcher["ordering"] = &Console::ordering;
    dispatcher["eval"] = &Console::eval;
#ifdef USE_GENETIC
    dispatcher["selfgame"] = &Console::selfgame;
    dispatcher["parmtest"] = &Console::parmtest;
    dispatcher["egtest"] = &Console::egtest;
#endif
}

Console::~Console() {}

int Console::exec() {
    std::string argStr = args.join(" ");
    StringList cmdsList = split(argStr, ":");
    for(auto cmdStr = cmdsList.begin(); cmdStr != cmdsList.end(); ++cmdStr) {
        parse(simplified(*cmdStr)); }
#if defined(QT_GUI_LIB)
    new Thread(&Console::inputThread, this);
    return QApplication::exec(); }

void Console::inputThread() {
#endif
    while(true) {
        std::string str;
        std::getline(std::cin, str);
//         f << str;
        if (str[str.length()-1] == 13 || str[str.length()-1] == 10) {
            str.erase(str.length()-1); }
        parse(str); }
}

//std::string Console::getAnswer() {
//    answer = "";
//    while(answer == "") {
//        processEvents();
//        sleep(1); }
//    return answer; }
//
void Console::send(std::string str) {
    if (Options::quiet) return;
    LockGuard<Mutex> lock(outputMutex);
    outputData = str + "\n";
    answer = str;
    outputCondition.notify_one();
}

void Console::parse(std::string str) {
    if (!str.empty()) {
        StringList cmds = split(str, " ");
        if (dispatcher.find(cmds[0]) != dispatcher.end()) {
            (this->*(dispatcher[cmds[0]]))(cmds); }
        else {
            tryMove(cmds[0]); } } }

void Console::tryMove(std::string cmds) {
    std::string mstr = toLower(cmds);
    if (mstr.length() < 4 || mstr.length() > 5) {
        send("command '" + mstr + "' not understood");
        return; }
    int piece;
    bool special;
    if (mstr.length() == 5) {
        special = true;
        switch (mstr[4]) {
        case 'q':
            piece = Queen;
            break;
        case 'r':
            piece = Rook;
            break;
        case 'b':
            piece = Bishop;
            break;
        case 'n':
        case 'k':
            piece = Knight;
            break;
        default:
            send("move '" + mstr + "' not understood");
            return; } }
    else {
        piece = 0;
        special = false; }
    Move m(mstr[0] -'a' + (mstr[1]-'1')*8, mstr[2]-'a' + (mstr[3]-'1')*8, piece, 0, special);
    if (game->doMove(m)) {
        send("move '" + mstr + "' illegal"); } }

void Console::perft(StringList cmds) {
    game->perft(convert(cmds[1])); }

void Console::divide(StringList cmds) {
    game->divide(convert(cmds[1])); }

void Console::uci(StringList /*cmds*/) {
    send("id name hayabusa 0.11.7");
    send("id author Gunther Piez");
    std::stringstream str;
    str << " 32768"; //TranspositionTable::maxSize;
    str << " default ";
    str << hashDefaultSize;
    send("option name Hash type spin min 1 max " + str.str());
    send("option name Reduction type check default true");
    send("option name Pruning type check default true");
    send("option name Clear Hash type button");
    send("option name UCI_ShowCurrLine type check default false");
#if 0
    for (auto i=Parameters::index.begin(); i!=Parameters::index.end(); ++i) { //FIXME this is overwhelming some UIs
        std::stringstream ss;
        ss << "option name " << i->first << " type spin default " << Parameters::base.at(i->second);
        send(ss.str()); }
#endif
    send("uciok"); }

void Console::debug(StringList cmds) {
    auto pos = cmds.parse(StringList() << "eval" << "mobility" << "search");
    if (pos.count("eval")) Options::debug |= debugEval;
    if (pos.count("mobility")) Options::debug |= debugMobility;
    if (pos.count("search")) Options::debug |= debugSearch; }

// engine is always ready as soon as the command dispatcher is working.
void Console::isready(StringList /*cmds*/) {
    send("readyok"); }

void Console::setoption(StringList cmds) {
    std::map<std::string, StringList> o = cmds.parse(StringList() << "name" << "value");
    std::string name =  toLower(o["name"].join(" "));
    if (!name.empty()) {
        std::string data = toLower(o["value"].join(" "));
        if (name == "splitdepth") {
            Options::splitDepth = convert(data); }
        else if (name == "uci_showcurrline") {
            Options::currline = convert<bool>(data); }
        else if (name == "humanreadable") {
            Options::humanreadable = convert<bool>(data); }
        else if (name == "book") {
            ; }
        else if (name == "bookreset") {
            ; }
        else if (name == "hash") {
            Options::hash = convert(data);
            if (Options::hash) game->setHashSize(Options::hash*0x100000ULL); }
        else if (name == "quiet") {
            Options::quiet = convert<bool>(data); }
        else if (name == "reduction") {
            Options::reduction = convert< bool >(data); }
        else if (name == "pruning") {
            Options::pruning = convert< bool >(data); }
        else if (name == "clear hash") {
            game->clearHash();
        }
        else if (Parameters::exists(name)) {
            defaultParameters[name] = convert<float>(data);
            game->eval.init(defaultParameters);
            game->clearEE(); }
        else {
            std::cerr << "option " << name << " not understood"; } } }

void Console::reg(StringList /*cmds*/) {}

void Console::ucinewgame(StringList /*cmds*/) {
    WorkThread::stopAll();
    game->clearHash(); }

void Console::position(StringList cmds) {
    if (cmds.size() < 2) return;
    auto pos = cmds.parse(StringList() << "startpos" << "fen" << "test" << "moves");
    WorkThread::stopAll();
    if (pos.count("startpos"))
        game->setup();
    else if (pos.count("fen")) {
        game->setup(pos["fen"].join(" ")); }
    else if (pos.count("test")) {
        if (cmds.size() < 3) return;
        std::string search = pos["test"].join(" ");
        for (unsigned int i = 0; testPositions[i]; ++i) {
            std::string pos(testPositions[i]);
            if (pos.find(search) != pos.npos) {
                game->setup(pos);
                break; } } }

    if (pos.count("moves")) {
        StringList moves = pos["moves"];
        for (auto imove = moves.begin(); imove != moves.end(); ++imove)
            tryMove(*imove);

    } }

void Console::go(StringList cmds) {
    WorkThread::stopAll();
    std::map<std::string, StringList> subCmds = cmds.parse(StringList() << "searchmoves"
            << "ponder" << "wtime" << "btime" << "winc" << "binc" << "movestogo" << "depth"
            << "nodes" << "mate" << "movetime" << "infinite");

    game->go(subCmds); }

void Console::stop(StringList /*cmds*/) {
    WorkThread::stopAll(); }

void Console::ponderhit(StringList /*cmds*/) {}

void Console::quit(StringList /*cmds*/) {
#ifdef QT_GUI_LIB
    QApplication::quit();
#else
    exit(0);
#endif
}

std::mutex nThreadMutex;
int nThread;
int maxThread;
double tested;
std::condition_variable nThreadCond;
std::vector<Game*> prb;
std::vector<std::future<uint64_t> > results;

uint64_t orderingInit(Game* board, int i) {
    int result;
    {
        board->maxSearchNodes = 5000000;
        board->setup(testPositions[i]);
        board->clearHash();
        if (board->color == White)
            board->rootSearch<White>(40);
        else
            board->rootSearch<Black>(40);
        if (stats.node < 5000000)
            result = 0;
        else
            result = board->depth - board->eval.dMaxExt; }
    {
        std::lock_guard<std::mutex> lock(nThreadMutex);
        --nThread;
        board->color = (Colors)0; }
    nThreadCond.notify_one();
    return result; }

uint64_t orderingTest(Game* board, int i) {
    uint64_t result;
    if (testDepths[i]) {
        board->maxSearchNodes = ~0;
        tested++;
        stats.node=0;
        board->setup(testPositions[i]);
        board->clearHash();
        if (board->color == White)
            board->rootSearch<White>(testDepths[i]+ board->eval.dMaxExt-1);
        else
            board->rootSearch<Black>(testDepths[i]+ board->eval.dMaxExt-1);
        result = stats.node; }
    else {
        result = 1; }
    {
        std::lock_guard<std::mutex> lock(nThreadMutex);
        --nThread;
        board->color = (Colors)0; }
    nThreadCond.notify_one();
    std::cout << std::setw(4) << i << "(" << std::setw(2) << testDepths[i]  << "):" << std::setw(10) << result << std::endl;
    return result; }

void tournament(uint64_t (*ordering)(Game*, int)) {
    for (unsigned int i = 0; testPositions[i]; ++i)  {
        {
            std::unique_lock<std::mutex> lock(nThreadMutex);
            while (nThread >= maxThread)
                nThreadCond.wait(lock);
            ++nThread; }
        Game* free;
        std::for_each(prb.begin(), prb.end(), [&free] (Game* rb) {
            if (!rb->color) free = rb; });
        free->color = (Colors)1;
        results[i]=std::async(std::launch::async, ordering, free, i); } }

void Console::ordering(StringList cmds) {

    nThread = 0;
    maxThread = 8;
    results.clear();
    for (int i = 0; testPositions[i]; ++i)
        results.push_back(std::future<uint64_t>());

    for (int i=0; i<maxThread; ++i) {
        game->infinite = true;
        Game* rb = new Game(this, defaultParameters, 0x2000000, 0x100000);
        rb->infinite = true;
        rb->color = (Colors) 0;
        prb.push_back(rb); }
    Options::quiet = true;
    if (cmds.size() > 1 && cmds[1] == "init") {
        std::thread th(tournament, orderingInit);
        th.detach();
        for (int i = 0; testPositions[i]; ++i) {
            while (!results[i].valid())
                usleep(10000);
            if (!(i % 26)) std::cout << std::endl;
            std::cout << std::setw(2) << results[i].get() << "," << std::flush; }
        std::cout << "0" << std::endl; }
    else {
        double sum=0.0;
        std::thread th(tournament, orderingTest);
        th.detach();
        for (int i = 0; testPositions[i]; ++i) {
            while (!results[i].valid())
                usleep(10000);
            sum += log(results[i].get()); }
        std::cout << std::setw(20) << exp(sum/tested) << std::endl; } }

void Console::eval(StringList) {
    game->eval(game->currentBoard(), game->color); }

#ifdef USE_GENETIC
void Console::selfgame(StringList ) {
//     Options::cpuTime = true;
    evolution->evolve();
//     Parameters a;
//
//     SelfGame sf(this, a, a);
//     sf.tournament();
}

void Console::egtest(StringList cmds) {
    Options::quiet = true;
    if (cmds.size() == 7)
        evolution->parmTest(cmds[1], convert<float>(cmds[2]), convert<float>(cmds[3]), convert(cmds[4]), cmds[5], cmds[6]);
    else
        std::cerr << "expected \"egtest <name> <minimum value> <maximum value> <n> <pieces>\"" << std::endl; }

void Console::parmtest(StringList cmds) {
    Options::quiet = true;
    if (cmds.size() == 5)
        evolution->parmTest(cmds[1], convert<float>(cmds[2]), convert<float>(cmds[3]), convert(cmds[4]), "20000", "");
    else if (cmds.size() == 6)
        evolution->parmTest(cmds[1], convert<float>(cmds[2]), convert<float>(cmds[3]), convert(cmds[4]), cmds[5], "");
    else if (cmds.size() == 9)
        evolution->parmTest(cmds[1], convert<float>(cmds[2]), convert<float>(cmds[3]), convert(cmds[4]), cmds[5], convert<float>(cmds[6]), convert<float>(cmds[7]), convert(cmds[8]));
    else if (cmds.size() == 13)
        evolution->parmTest(cmds[1], convert<float>(cmds[2]), convert<float>(cmds[3]), convert(cmds[4]),
                            cmds[5], convert<float>(cmds[6]), convert<float>(cmds[7]), convert(cmds[8]),
                            cmds[9], convert<float>(cmds[10]), convert<float>(cmds[11]), convert(cmds[12])
                           );
    else
        std::cerr << "expected \"parmtest <name> <minimum value> <maximum value> <n>\"" << std::endl; }
#endif
void Console::outputThread() {
    setvbuf(stdout, NULL, _IOLBF, 0);
    UniqueLock<Mutex> lock(outputMutex);
    while(true) {
        outputCondition.wait(lock, [this] { return !outputData.empty(); });
        printf(outputData.c_str());
        outputData.clear();
    }
}

template<>
Console& Console::operator << (std::string out) {
    if (Options::quiet) return *this;
    LockGuard<Mutex> lock(outputMutex);
    outputData = out;
    answer = out;
    outputCondition.notify_one();
    return *this;
}
