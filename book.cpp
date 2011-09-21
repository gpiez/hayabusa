/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Gunther Piez <gpiez@web.de>

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
#ifndef PCH_H_
#include <pch.h>
#endif
#include <fstream>
#include "book.h"

template<typename T> void bswap(T& x) {
    T res = 0;
    for (unsigned i=0; i<sizeof(T); ++i) {
        res <<= 8;
        res += (x & 0xff);
        x >>= 8;
    }
    x = res;
}

Book::Book():
    book(NULL)
{
    endianess.a = 1;
}

Book::~Book()
{
    delete book;
}

void Book::read(std::string fname) {
    std::ifstream f(fname.c_str(), std::ios_base::in | std::ios_base::binary);
    f.seekg( 0, std::ios_base::end );
    size_t fsize = f.tellg();
    f.seekg( std::ios_base::beg );
    size = fsize / sizeof(BookEntry);
    delete book;
    book = new BookEntry[size];
    
    f.read((char*)book, size*sizeof(BookEntry));
    if (endianess.little) 
        for (size_t i=0; i<size; ++i) {
            bswap( book[i].key );
            bswap( book[i].move );
            bswap( book[i].weight );
            bswap( book[i].learn );
        }
}

void Book::write(std::string fname) {
    std::ofstream f(fname.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

    if (endianess.little)
        for (size_t i=0; i<size; ++i) {
            bswap( book[i].key );
            bswap( book[i].move );
            bswap( book[i].weight );
            bswap( book[i].learn );
        }
    f.write((char*)book, size*sizeof(BookEntry));
    if (endianess.little)
        for (size_t i=0; i<size; ++i) {
            bswap( book[i].key );
            bswap( book[i].move );
            bswap( book[i].weight );
            bswap( book[i].learn );
        }
}
    
void Book::resetWeights() {
    for (size_t i=0; i<size; ++i)
        book[i].weight = 1;
}
