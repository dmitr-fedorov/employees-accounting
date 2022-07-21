#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");

    QApplication a(argc, argv);
    a.QApplication::setQuitOnLastWindowClosed(true);

    MainWindow w;
    w.show();

    return a.exec();
}
