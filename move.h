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
#ifndef MOVE_H_
#define MOVE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"

struct Move {
    int32_t data;

    Move() = default;
    Move(unsigned int from, unsigned int to, unsigned int piece, unsigned int capture=0, bool special=false) {
        ASSERT(from < 64);
        ASSERT(to < 64);
        ASSERT(capture < King);
        ASSERT(piece <= King);
        data = from + (to << 8) + (capture << 16) + (piece << 24) + (special << 31);
    }
    unsigned int from() const {
        return (uint8_t)data;
    }
    unsigned int to() const {
        return (uint8_t)(data >> 8);
    }
    unsigned int fromto() const {
        return (uint16_t)data;
    }
    unsigned int capture() const {
        return (uint8_t)(data >> 16);
    }
    unsigned int piece() const {
        return (uint32_t)data >> 24;
    }
    bool isSpecial() const {
        return data<0;
    }
    std::string string() const;
    std::string algebraic() const;
};
#endif /* MOVE_H_ */
