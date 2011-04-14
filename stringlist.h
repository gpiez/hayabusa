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


#ifndef STRINGLIST_H
#define STRINGLIST_H

#ifndef PCH_H_
#include <pch.h>
#endif

class StringList: public std::vector<std::string> {
public:
    StringList();
    ~StringList();
    std::string join(const char* = "");
    StringList& operator << (const char*);
    std::map<std::string, StringList> parse(const StringList&) const;
};

StringList split(std::string str, std::string delims = std::string(" "));
std::string simplified(std::string str);
std::string toLower(std::string str);
template<typename T=int>
T convert(std::string str) {
    std::stringstream ss(str);
    T x;
    ss >> x;
    return x;
}

#endif // STRINGLIST_H
