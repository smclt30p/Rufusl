#include "ui/rufuswindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RufusWindow w;
    w.show();

    return a.exec();
}
