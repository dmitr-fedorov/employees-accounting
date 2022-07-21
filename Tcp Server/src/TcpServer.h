#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "TcpDataTypes.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>

/*
 * Класс сервера, обеспечивающего
 * обмен данными с одним клиентом по протоколу TCP.
 *
 *
 * В его задачи входят:
 *
 * - Получение базы данных от клиента и ее сохранение
 * в папке Backups в директории программы;
 *
 * - Отправка клиенту результата сохранения полученной базы данных;
 *
 * - Отправка клиенту списка баз данных, хранящихся на сервере;
 *
 * - Получение от клиента названия базы данных, которую тот хочет получить
 * и отправка этой базы клиенту.
 */
class TcpServer : public QTcpServer
{
    Q_OBJECT

public:
    TcpServer();
    ~TcpServer();

private:
    QTcpSocket *m_pClientSocket;
    QDir m_databasesDirectory;

    void sendDatabase(const QFileInfo &dbFileInfo);
    void sendDatabaseFileCreationResult(bool dbFileCreated);
    void sendDatabasesList();
    bool createNewDatabaseFile(const QByteArray &dbInBytes, QString dbName);
    QString getDatabaseNameOnly(QString dbFileName);
    QString getDateAndTimeForName();

public slots:
    void incomingConnection(qintptr socketDescriptor);
    void slotReadyRead();
};

#endif // TCPSERVER_H
