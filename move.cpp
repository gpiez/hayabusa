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
#include "move.h"
#include "constants.h"

std::string Move::string() const
{
	std::string temp;
    temp += (const char*[]){"","R","B","Q","N","","K"}[piece() & 7];
	temp += (from() & 7) + 'a';
	temp += (from() >> 3) + '1';
	temp += capture() ? 'x' : '-';
	temp += (to() & 7) + 'a';
	temp += (to() >> 3) + '1';
    if (isSpecial()) 
        if ((piece() & 7) < Pawn) {
            temp += " RBQN"[piece() & 7];
            temp = temp.substr(1);
        }
    
	return temp;
}
