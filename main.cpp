/*
 * main.cpp
 *
 *  Created on: 21.09.2009
 *      Author: gpiez
 */

#include <pch.h>

#include "console.h"

int main(int argc, char *argv[])
{
#ifdef QT_GUI_LIB
	Q_INIT_RESOURCE(hayabusa);
    QApplication app(argc, argv);
#else
    QCoreApplication app(argc, argv);
#endif
    Console console(&app);
    return app.exec();
}
