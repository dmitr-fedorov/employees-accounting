#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "tcpdatatypes.h"

#include <QWidget>
#include <QTcpSocket>
#include <QtSql/QSqlDatabase>
#include <QFileInfo>

/*
 * Класс клиента, обеспечивающего
 * обмен данными с сервером по протоколу TCP.
 *
 *
 * В его задачи входят:
 * - Отправка базы данных на сервер;
 *
 * - Получение от сервера подтверждения, что он успешно получил
 * и сохранил отправленную ему базу данных;
 *
 * - Получение списка баз данных, хранящихся на сервере;
 *
 * - Отправка серверу названия базы данных,
 * которую он должен отослать клиенту;
 *
 * - Получение базы данных с сервера;
 */
class TcpClient : public QWidget
{
    Q_OBJECT

public:
    explicit TcpClient(QString host, int port, QWidget *parent = nullptr);

    bool connectToServer(QString host, int port);
    bool isConnectedToServer();

    void sendDatabase(const QFileInfo &dbFileInfo);
    void sendDatabasesListRequest();
    void sendSelectedDatabaseName(const QString &selDbName);

private:
    QTcpSocket *m_pServerSocket;

private slots:
    void slotReadyRead();

    void slotError(QAbstractSocket::SocketError err);

    void slotConnected();

    void slotGetSelectedBackupVersion(QString &version);

signals:
    void databaseReceived(const QByteArray &dbInBytes, QString dbName);
    void serverCreatedDatabaseFile(bool dbFileCreated);
};

#endif // TCPCLIENT_H
