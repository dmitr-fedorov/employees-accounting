#include <QCoreApplication>
#include "include/TcpServer.h"

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");

    QCoreApplication a(argc, argv);
    a.setApplicationName("Учет сотрудников организаций - сервер");

    TcpServer server;

    return a.exec();
}
