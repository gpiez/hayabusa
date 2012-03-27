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

int main(int argc, char* argv[]) {
#if defined(QT_GUI_LIB) || defined(QT_NETWORK_LIB)
    Q_INIT_RESOURCE(hayabusa);
    qRegisterMetaType<std::string>("std::string");
#endif
    console.init(argc, argv);
    return console.exec(); }
