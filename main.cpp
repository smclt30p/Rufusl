#include "ui/rufuswindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RufusWindow w;
	a.setSetuidAllowed(true);
    return a.exec();
}
