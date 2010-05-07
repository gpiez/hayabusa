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

#ifndef PCH_H_
#include <pch.h>
#endif

#include <Qt/QtTest>

class RootBoard;
class Console;

class TestRootBoard: public QObject {
	Q_OBJECT
	RootBoard *b;
	Console *c;
	
private slots:
	void setPiece();
	void pieceList();
	void generateCaptures();
	void initTestCase();
};

class TestBoardBase: public QObject {
	Q_OBJECT
	RootBoard *b;
	Console *c;

private slots:
/*	void setPiece();
	void pieceList();
	void generateCaptures();
	void initTestCase();*/
};

#endif /* TEST_H_ */
