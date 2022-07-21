#include "TcpServer.h"

#include <QCoreApplication>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QTextStream>

TcpServer::TcpServer()
{
    m_databasesDirectory = QCoreApplication::applicationDirPath() + "/Backups";
    if (!m_databasesDirectory.exists())
        m_databasesDirectory.mkpath(".");

    if (this->listen(QHostAddress::Any, 2323))
        qInfo() << "Сервер запущен";
    else
        qInfo() << "При запуске сервера произошла ошибка";
}

TcpServer::~TcpServer()
{

}

void TcpServer::sendDatabase(const QFileInfo &dbFileInfo)
{
    QFile dbFile(dbFileInfo.absoluteFilePath());

    if (!dbFile.exists())
    {
        qInfo() << "ОШИБКА: отправляемый файл не существует: " << dbFileInfo.absoluteFilePath();
        return;
    }

    if (!dbFile.open(QIODevice::ReadOnly))
    {
        qInfo() << "ОШИБКА: не удалось открыть отправляемый файл:" << dbFileInfo.absoluteFilePath();
        return;
    }

    QByteArray data;
    data.clear();

    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_2);

    out << quint16(0);
    out << quint16(TcpDataType::Database);
    out << getDatabaseNameOnly(dbFileInfo.fileName()).toUtf8();
    out << dbFile.readAll();
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    m_pClientSocket->write(data);

    dbFile.close();

    qInfo() << "База данных " << dbFileInfo.fileName() << "была отправлена клиенту";
}

void TcpServer::sendDatabaseFileCreationResult(bool dbFileCreated)
{
    QByteArray data;
    data.clear();

    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_2);

    out << quint16(0);
    if (dbFileCreated)
        out << quint16(TcpDataType::DatabaseFileCreationSuccess);
    else
        out << quint16(TcpDataType::DatabaseFileCreationFailure);
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    m_pClientSocket->write(data);
}

void TcpServer::sendDatabasesList()
{
    QByteArray data;
    data.clear();

    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_2);

    out << quint16(0);
    out << quint16(TcpDataType::DatabasesList);
    out << m_databasesDirectory.entryList(QStringList() << "*.db" << "*.DB", QDir::Files);
    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    m_pClientSocket->write(data);

    qInfo() << "Список баз данных был отправлен клиенту";
}

bool TcpServer::createNewDatabaseFile(const QByteArray &dbInBytes, QString dbName)
{    
    qInfo() << "Размер массива байтов с базой данных:" << dbInBytes.size();

    QString newDbFilePath = m_databasesDirectory.path() + "/" + dbName
            + " " + getDateAndTimeForName() + ".db";

    QFile newDbfile(newDbFilePath);
    if (!newDbfile.open(QIODevice::WriteOnly))
    {
        qInfo() << "ОШИБКА: не удалось открыть файл при создании базы данных: " << newDbFilePath;
        return false;
    }

    if (newDbfile.write(dbInBytes) == -1)
    {
        newDbfile.close();
        newDbfile.remove();
        qInfo() << "ОШИБКА: не удалось создать файл с базой данных: " << dbName;
        return false;
    }

    newDbfile.close();
    return true;
}

/* Базы данных хранятся в папке Backups
 * с именем в формате "Имя день-месяц-год--час-минуты-cекунды.db".
 *
 * Функция отделяет "Имя" от " день-месяц-год--час-минуты-cекунды.db"
 * и возвращает "Имя" отдельной строкой.
 *
 * Возвращает пустую строку, если в названии базы данных нет части "Имя".
 * */
QString TcpServer::getDatabaseNameOnly(QString dbFileName)
{
    for (int i = dbFileName.count() - 1; i > 0; i--)
    {
        if (dbFileName.at(i) == ' ')
            return dbFileName.chopped(dbFileName.count() - i);
    }

    return "";
}

QString TcpServer::getDateAndTimeForName()
{
    QDate today = QDate::currentDate();
    QTime time = QTime::currentTime();

    QString dateAndTimeForName;

    dateAndTimeForName = QString::number(today.day())
                       + "-"
                       + QString::number(today.month())
                       + "-"
                       + QString::number(today.year())
                       + "--"
                       + QString::number(time.hour())
                       + "-"
                       + QString::number(time.minute())
                       + "-"
                       + QString::number(time.second());

    return dateAndTimeForName;
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    m_pClientSocket = new QTcpSocket(this);
    m_pClientSocket->setSocketDescriptor(socketDescriptor);
    connect(m_pClientSocket, &QTcpSocket::readyRead, this, &TcpServer::slotReadyRead);
    connect(m_pClientSocket, &QTcpSocket::disconnected, m_pClientSocket, &TcpServer::deleteLater);

    qInfo() << "Клиент подключился: " << socketDescriptor;
}

void TcpServer::slotReadyRead()
{
    m_pClientSocket = (QTcpSocket*)sender();
    QDataStream in(m_pClientSocket);
    in.setVersion(QDataStream::Qt_6_2);

    QByteArray data;
    data.clear();
    quint16 nextBlockSize = 0;
    quint16 incomingDataType = 0;
    QString receivedDbName;
    QString dbFileForSendName;

    if (in.status() == QDataStream::Ok)
    {
        qInfo() << "Чтение поступивших данных...";

        for (;;)
        {
            if (nextBlockSize == 0)
            {
                if (m_pClientSocket->bytesAvailable() < 2)
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
                    else if (incomingDataType == TcpDataType::SelectedDatabaseName)
                    {
                        QByteArray bArray;
                        in >> bArray;
                        dbFileForSendName = QString::fromUtf8(bArray);
                        nextBlockSize -= sizeof(QChar) * (dbFileForSendName.count() + 2);
                    }
                }
            }

            if (m_pClientSocket->bytesAvailable() < nextBlockSize)
            {
                break;
            }

            in >> data;
            nextBlockSize = 0;
        }
    }

    if (incomingDataType == TcpDataType::Database)
    {
        if (receivedDbName.endsWith(".db"))
            receivedDbName.chop(3);
        qInfo() << "Получена база данных: " << receivedDbName;

        if (createNewDatabaseFile(data, receivedDbName))
            sendDatabaseFileCreationResult(true);
        else
            sendDatabaseFileCreationResult(false);
    }
    else if (incomingDataType == TcpDataType::DatabasesListRequest)
    {
        qInfo() << "Получен запрос на отправку списка баз данных";
        sendDatabasesList();
    }
    else if (incomingDataType == TcpDataType::SelectedDatabaseName)
    {
        if (dbFileForSendName.isEmpty())
        {
            qInfo() << "ОШИБКА: клиент не отправил имя базы данных, которую хочет получить";
            return;
        }

        qInfo() << "Получено имя выбранной клиентом базы данных: " << dbFileForSendName;
        sendDatabase(QFileInfo(m_databasesDirectory, dbFileForSendName));
    }
    else
    {
        qInfo() << "ВНИМАНИЕ: тип получаемых данных неизвестен";
    }
}
