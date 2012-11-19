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
#ifndef TEST_H_
#define TEST_H_

class Game;
class Hayabusa;

#include <QtCore>
#include <Qt/QtTest>

class TestRootBoard: public QObject {
    Q_OBJECT
    Game* b;
    Hayabusa* c;
    void cmpMates(std::string, std::string);
private slots:
    void initTestCase();
    void generateMateMoves();
    void pieceList();
    void generateCaptures();
    void perft(); };

class TestBoardBase: public QObject {
    Q_OBJECT
    Game* b;
    Hayabusa* c;

private slots:
    /*	void setPiece();
        void pieceList();
        void generateCaptures();
        void initTestCase();*/
};

#endif /* TEST_H_ */
