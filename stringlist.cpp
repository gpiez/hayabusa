/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

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


#include "stringlist.h"

std::string StringList::join(const char* str) {
    auto i=begin();
    if (i == end()) return "";
    std::string sum = *i++;
    while (i!=end()) {
        sum += str + *i;
        ++i;
    }
    return sum;
}

StringList& StringList::operator << (const char* str) {
    push_back(std::string(str));
    return *this;
}

StringList split(std::string str, std::string delims) {
    using namespace std;
    // Skip delims at beginning, find start of first token
    string::size_type lastPos = str.find_first_not_of(delims, 0);
    // Find next delimiter @ end of token
    string::size_type pos     = str.find_first_of(delims, lastPos);

    // output vector
    StringList tokens;

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delims.  Note the "not_of". this is beginning of token
        lastPos = str.find_first_not_of(delims, pos);
        // Find next delimiter at end of token.
        pos     = str.find_first_of(delims, lastPos);
    }

    return tokens;
}

std::string simplified(std::string str) {
    return str;
}

std::string toLower(std::string str) {
    return str;
}
