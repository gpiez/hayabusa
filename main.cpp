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
#ifdef USE_QTGUI
	Q_INIT_RESOURCE(application);
    QApplication app(argc, argv);
    MainWindow mainWin;
    mainWin.show();
#else
    QCoreApplication app(argc, argv);
#endif
    Console console(&app);
    return app.exec();
}
