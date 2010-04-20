/*
 * test.h
 *
 *  Created on: 29.09.2009
 *      Author: gpiez
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
