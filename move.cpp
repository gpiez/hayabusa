/*
 * move.cpp
 *
 *  Created on: Dec 14, 2009
 *      Author: gpiez
 */

#include "move.h"
#include "constants.h"

QString Move::string() const
{
	QString temp;
	temp += (from & 7) + 'a';
	temp += (from >> 3) + '1';
	temp += capture ? 'x' : '-';
	temp += (to & 7) + 'a';
	temp += (to >> 3) + '1';
	switch (special & 0xf) {
	case promoteQ:
		temp += 'Q';
		break;
	case promoteN:
		temp += 'N';
		break;
	case promoteB:
		temp += 'B';
		break;
	case promoteR:
		temp += 'R';
		break;
	}
	return temp;
}
