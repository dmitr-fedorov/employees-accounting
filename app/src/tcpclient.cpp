#include "tcpclient.h"
#include "dialogselectbackupversion.h"

#include <QFile>
#include <QCoreApplication>
#include <QTextStream>
#include <QDataStream>

TcpClient::TcpClient(QString host, int port, QWidget *parent)
    : QWidget(parent)
{
    m_pServerSocket = new QTcpSocket(this);
    connectToServer(host, port);

    connect(m_pServerSocket, SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(m_pServerSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
    connect(m_pServerSocket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
            this, SLOT(slotError(QAbstractSocket::SocketError)));
}

bool TcpClient::connectToServer(QString host, int port)
{
    m_pServerSocket->connectToHost(host, port);
    m_pServerSocket->waitForConnected(150);

    return m_pServerSocket->state() == QAbstractSocket::ConnectedState;
}

bool TcpClient::isConnectedToServer()
{
    return m_pServerSocket->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::sendDatabase(const QFileInfo &dbFileInfo)
{
    QByteArray data;
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);

    QFile file(dbFileInfo.absoluteFilePath());
    if (!file.exists())
    {
        qInfo() << "Отправляемый файл не существует: " << dbFileInfo.absoluteFilePath();

        return;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        qInfo() << "Не удалось открыть отправляемый файл: " << dbFileInfo.absoluteFilePath();

        return;
    }

    out << quint16(0);
    out << quint16(TcpDataType::Database);
    out << dbFileInfo.fileName().chopped(3).toUtf8();
    out << file.readAll();
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    m_pServerSocket->write(data);

    file.close();

    qInfo() << "Отправлена база данных:" << dbFileInfo.absoluteFilePath();
    qInfo() << "Имя отправленной базы данных:" << dbFileInfo.fileName().chopped(3);
    qInfo() << "Размер отправленных данных:" << data.size();
}

void TcpClient::sendDatabasesListRequest()
{
    QByteArray data;
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);

    out << quint16(0);
    out << quint16(TcpDataType::DatabasesListRequest);
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    m_pServerSocket->write(data);

    qInfo() << "Отправлен запрос на получение списка баз данных на сервере";
}

void TcpClient::sendSelectedDatabaseName(const QString &selDbName)
{
    QByteArray data;
    data.clear();
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);

    out << quint16(0);
    out << quint16(TcpDataType::SelectedDatabaseName);
    out << selDbName.toUtf8();
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    m_pServerSocket->write(data);

    qInfo() << "Отправлено имя базы данных, которую нужно получить с сервера:" << selDbName;
}

void TcpClient::slotReadyRead()
{
    QDataStream in(m_pServerSocket);
    in.setVersion(QDataStream::Qt_5_0);

    quint16 nextBlockSize = 0;
    quint16 incomingDataType = 0;
    QStringList databasesOnServerList;
    databasesOnServerList.clear();
    QString receivedDbName;
    QByteArray data;
    data.clear();

    if (in.status() == QDataStream::Ok)
    {
        while (true)
        {
            qInfo() << "Чтение поступивших от сервера данных...";
            if (nextBlockSize == 0)
            {
                if (m_pServerSocket->bytesAvailable() < 2)
                    break;
                in >> nextBlockSize;

                if (incomingDataType == 0)
                {
                    in >> incomingDataType;
                    nextBlockSize -= sizeof(quint16);

                    if (incomingDataType == TcpDataType::Database)
                    {
                        QByteArray bArray;
                        in >> bArray;
                        receivedDbName = QString::fromUtf8(bArray);
                        nextBlockSize -= sizeof(QChar) * (receivedDbName.count() + 2);
                    }
                    else if (incomingDataType == TcpDataType::DatabasesList)
                        in >> databasesOnServerList;
                }
            }

            if (m_pServerSocket->bytesAvailable() < nextBlockSize)
                break;

            in >> data;
            nextBlockSize = 0;
        }
    }

    if (incomingDataType == TcpDataType::Database)
    {
        if (receivedDbName.isEmpty())
        {
            qInfo() << "ОШИБКА: сервер не отправил имя базы данных";
            return;
        }

        qInfo() << "Получена база данных с сервера:" << receivedDbName;
        emit databaseReceived(data, receivedDbName);
    }
    else if (incomingDataType == TcpDataType::DatabaseFileCreationSuccess)
    {
        qInfo() << "Получено сообщение о том, что сервер успешно сохранил полученную базу даных";
        emit serverCreatedDatabaseFile(true);
    }
    else if (incomingDataType == TcpDataType::DatabaseFileCreationFailure)
    {
        qInfo() << "Получено сообщение о том, что сервер не смог сохранить полученную базу данных";
        emit serverCreatedDatabaseFile(false);
    }
    else if (incomingDataType == TcpDataType::DatabasesList)
    {
        qInfo() << "Получен список баз данных на сервере:" << databasesOnServerList;

        DialogSelectBackupVersion dialog(databasesOnServerList, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            sendSelectedDatabaseName(dialog.getSelectedVersion());
        }
    }
    else
    {
        qInfo() << "ВНИМАНИЕ: тип полученных данных неизвестен";
    }
}

void TcpClient::slotError(QAbstractSocket::SocketError err)
{
    QString strError = "Ошибка: " + (err == QAbstractSocket::HostNotFoundError ?
                                    "Хост не был найден" :
                                    err == QAbstractSocket::ConnectionRefusedError ?
                                    "Отказано в соединении" :
                                    err == QAbstractSocket::RemoteHostClosedError ?
                                    "Работа хоста была прекращена" :
                                    QString(m_pServerSocket->errorString())
                                    );
    qDebug() << strError;
}

void TcpClient::slotConnected()
{
    qInfo() << "TcpClient: подключено к серверу";
}

void TcpClient::slotGetSelectedBackupVersion(QString &version)
{
    sendSelectedDatabaseName(version);
}
